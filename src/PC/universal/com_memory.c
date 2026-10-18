#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

extern void Com_Printf(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Memset(void *dest, const int val, int count);
extern void Sys_OutOfMemErrorInternal(const char *filename, int line);
extern void *VirtualAlloc(void *lpAddress, int dwSize, int flAllocationType, int flProtect);
extern int VirtualFree(void *lpAddress, int dwSize, int dwFreeType);

#if defined(COD2_X64) && defined(COD2_GUARDHEAP)
/* DIAGNOSTIC guard-page allocator: each block's END butts against a no-access page so a heap
   overrun faults exactly at the bad write (caught by the SEH handler). Base ptr is stored 16
   bytes before the user ptr for Z_FreeInternal. Enable with -DCOD2_GUARDHEAP. */
/* NOTE: in this build VirtualAlloc/VirtualFree are REDEFINED in MacWin32.c as calloc/free (CRT
   heap, not OS pages).  We need the REAL kernel32 functions for guard pages, via GetProcAddress. */
extern void *__stdcall GetModuleHandleA(const char *);
extern void *__stdcall GetProcAddress(void *, const char *);
typedef void *(__stdcall *gp_VA_t)(void *, unsigned long long, unsigned long, unsigned long);
typedef int(__stdcall *gp_VP_t)(void *, unsigned long long, unsigned long, unsigned long *);
typedef int(__stdcall *gp_VF_t)(void *, unsigned long long, unsigned long);
static gp_VA_t gp_realVA;
static gp_VP_t gp_realVP;
gp_VF_t gp_realVF; /* used by Z_FreeInternal */
#define GP_PAGE 4096
#define GP_MAGIC ((void *)0x4755415244504721ull) /* "GUARDP!" */
static void gp_init(void)
{
    void *k = GetModuleHandleA("kernel32.dll");
    gp_realVA = (gp_VA_t)GetProcAddress(k, "VirtualAlloc");
    gp_realVP = (gp_VP_t)GetProcAddress(k, "VirtualProtect");
    gp_realVF = (gp_VF_t)GetProcAddress(k, "VirtualFree");
}
static void *gp_alloc(int size, int zero)
{
    int need = (size <= 0) ? 16 : ((size + 15) & ~15);
    int nbytes = ((need + 32 + GP_PAGE - 1) / GP_PAGE) * GP_PAGE;
    unsigned long oldp;
    char *base;
    char *user;
    if (!gp_realVA)
        gp_init();
    base = (char *)gp_realVA(0, (unsigned long long)(nbytes + GP_PAGE), 0x3000 /*COMMIT|RESERVE*/, 4 /*RW*/);
    if (!base) {
        user = (char *)malloc(need);
        if (user && zero) Com_Memset(user, 0, size);
        return user;
    }
    gp_realVP(base + nbytes, GP_PAGE, 1 /*PAGE_NOACCESS*/, &oldp);
    user = base + nbytes - need;
    *(void **)(user - 16) = base;
    *(void **)(user - 8) = GP_MAGIC;
    if (zero) Com_Memset(user, 0, size);
    return user;
}
#endif
extern long FS_HashFileName(const char *fname, int hashSize);
extern int FS_LoadStack(void);
extern int stricmp(const char *s1, const char *s2);
extern void XModelPartsFree(XModelParts *model);
extern void XModelFree(XModel *model);
extern void XAnimFree(XAnimParts *parts);
extern void XAnimFreeList(XAnim *anims);
extern void Cmd_AddCommand(const char *cmdName, void (*function)());
extern dvar_t *Dvar_RegisterInt(const char *dvarName, int value, int min, int max, int flags);
extern int Sys_Milliseconds(void);

extern const dvar_t *com_dedicated;
static fileData_t *com_fileDataHashTable[1024];
static fileData_t *com_hunkData;
static int g_largeLocalPos;
static byte g_largeLocalBuf[524288];
static struct hunkUsed_t hunk_low;
static hunkUsed_t hunk_high;
static byte *s_hunkData;
static byte *s_origHunkData;
static int s_hunkTotal;

static void Hunk_ClearData(void);
static void Com_Meminfo_f(void);

#define TO_MB(x) ((x) / (1024 * 1024))

static void Hunk_ClearFileData(fileData_t **pFileData, byte *low, byte *high)
{
    fileData_t *fd;

    while ((fd = *pFileData) != 0) {
        if ((byte *)fd < low || (byte *)fd >= high) {
            pFileData = (fileData_t **)&fd->next;
            continue;
        }

        *pFileData = *(fileData_t **)&fd->next;
        void *data = fd->data;

        switch (fd->type) {
        case 3:
            XModelPartsFree( (XModelParts *)(data));
            break;
        case 4:
            XModelFree( (XModel *)(data));
            break;
        case 5:
            XAnimFree( (XAnimParts *)(data));
            break;
        case 6:
            XAnimFreeList( (XAnim *)(data));
            break;
        default:
            break;
        }
    }
}

void Z_FreeInternal(void *ptr)
{
#if defined(COD2_X64) && defined(COD2_GUARDHEAP)
    extern gp_VF_t gp_realVF;
    if (ptr) {
        if (*(void **)((char *)ptr - 8) == GP_MAGIC && gp_realVF)
            gp_realVF(*(void **)((char *)ptr - 16), 0, 0x8000 /*MEM_RELEASE*/);
        else
            free(ptr); /* malloc fallback block */
    }
#else
    free(ptr);
#endif
}

void Z_VirtualFreeInternal(void *ptr)
{
    VirtualFree(ptr, 0, 0x8000);
}

void Z_VirtualDecommitInternal(void *ptr, int size)
{
    VirtualFree(ptr, size, 0x4000);
}

void *Z_TryMallocInternal(int size)
{
    void *buf;
#if defined(COD2_X64) && defined(COD2_GUARDHEAP)
    return gp_alloc(size, 1);
#endif
    buf = malloc(size);
    if (buf) {
        Com_Memset(buf, 0, size);
    }
    return buf;
}

void *Z_MallocInternal(int size)
{
    void *buf;
#if defined(COD2_X64) && defined(COD2_GUARDHEAP)
    return gp_alloc(size, 1);
#endif
    buf = malloc(size);
    if (buf) {
        Com_Memset(buf, 0, size);
        return buf;
    }
    Sys_OutOfMemErrorInternal(__FILE__, __LINE__);
    return buf;
}

void *Z_MallocGarbageInternal(int size)
{
    void *buf;
#if defined(COD2_X64) && defined(COD2_GUARDHEAP)
    return gp_alloc(size, 0);
#endif
    buf = malloc(size);
    if (!buf) {
        Sys_OutOfMemErrorInternal(__FILE__, __LINE__);
    }
    return buf;
}

void *Z_VirtualAllocInternal(int size)
{
    void *buf;

    buf = VirtualAlloc(0, size, 0x1000, 4);
    if (!buf) {
        Sys_OutOfMemErrorInternal(__FILE__, __LINE__);
    }
    return buf;
}

void *Z_VirtualReserveInternal(int size)
{
    return VirtualAlloc(0, size, 0x2000, 4);
}

void Z_VirtualCommitInternal(void *ptr, int size)
{
    if (!VirtualAlloc(ptr, size, 0x1000, 4)) {
        Sys_OutOfMemErrorInternal(__FILE__, __LINE__);
    }
}

char *CopyStringInternal(const char *in)
{
    int len;
    char *buf;

    len = strlen(in) + 1;
    buf = (char *)malloc(len);
    if (buf) {
        Com_Memset(buf, 0, len);
    } else {
        Sys_OutOfMemErrorInternal(__FILE__, __LINE__);
    }
    strcpy(buf, in);
    return buf;
}

void ReplaceStringInternal(char **str, const char *in)
{
    int len;
    int newLen;
    int oldLen;
    char *newStr;

    len = strlen(in) + 1;
    newLen = len - 1;

    newStr = *str;
    if (newStr) {
        oldLen = strlen(newStr);
        if (oldLen >= newLen) {

            strcpy(newStr, in);
            return;
        }
        free(newStr);
    }

    newStr = (char *)malloc(len);
    if (newStr) {
        Com_Memset(newStr, 0, len);
    } else {
        Sys_OutOfMemErrorInternal(__FILE__, __LINE__);
    }
    *str = newStr;
    strcpy(newStr, in);
}

static void Com_Meminfo_f(void)
{
    Com_Printf("%8i bytes total hunk\n", s_hunkTotal);
    Com_Printf("\n");
    Com_Printf("%8i low permanent\n", hunk_low.permanent);
    if (hunk_low.temp != hunk_low.permanent) {
        Com_Printf("%8i low temp\n", hunk_low.temp);
    }
    Com_Printf("\n");
    Com_Printf("%8i high permanent\n", hunk_high.permanent);
    if (hunk_high.temp != hunk_high.permanent) {
        Com_Printf("%8i high temp\n", hunk_high.temp);
    }
    Com_Printf("\n");
    Com_Printf("%8i total hunk in use\n", hunk_low.permanent + hunk_high.permanent);
    Com_Printf("\n");
}

void Com_TouchMemory(void)
{
    int start;
    int i;
    int sum;
    int totalIntCount;
    int highStart;
    int highEnd;

    start = Sys_Milliseconds();

    sum = 0;
    totalIntCount = hunk_low.permanent >> 2;
    for (i = 0; i < totalIntCount; i += 64) {
        sum += ((int *)s_hunkData)[i];
    }

    highEnd = hunk_high.permanent >> 2;
    highStart = (s_hunkTotal - hunk_high.permanent) >> 2;
    for (i = highStart; i < highEnd; i += 64) {
        sum += ((int *)s_hunkData)[i];
    }

    Com_Printf("Com_TouchMemory: %i msec. Using sum: %d\n", Sys_Milliseconds() - start, sum);
}

void Com_InitHunkMemory(void)
{
    dvar_t *cv;
    int nMegs;

    if (FS_LoadStack() != 0) {
        Com_Error(0, "Hunk initialization failed. File system load stack not zero");
    }

    cv = Dvar_RegisterInt("com_hunkMegs", 0xa0, 1, 0x200, 0x1021);
    nMegs = cv->current.integer;
    if (nMegs <= 0x4f) {
        Com_Printf("Minimum com_hunkMegs for a dedicated server is %i, allocating %i megs.\n", 0x50, 0x50);
        s_hunkTotal = 0x5000000;
    } else {
        s_hunkTotal = nMegs << 20;
    }

    s_hunkData = (byte *)VirtualAlloc(0, s_hunkTotal, 0x1000, 4);
    if (!s_hunkData) {
        Sys_OutOfMemErrorInternal(__FILE__, __LINE__);
    }

    s_origHunkData = s_hunkData;

    hunk_low.permanent = 0;
    hunk_low.temp = 0;
    hunk_high.permanent = 0;
    hunk_high.temp = 0;
    Hunk_ClearData();

    Cmd_AddCommand("meminfo", Com_Meminfo_f);
}

static void *Hunk_FindFileData(int type, const char *name, int hash)
{
    fileData_t *fd;

    for (fd = com_fileDataHashTable[hash]; fd; fd = *(fileData_t **)&fd->next) {
        if (fd->type == type && !stricmp(fd->name, name)) {
            return fd->data;
        }
    }
    return 0;
}

void *Hunk_FindDataForFile(int type, const char *name)
{
    int hash;

    hash = FS_HashFileName(name, 0x400);
    return Hunk_FindFileData(type, name, hash);
}

qboolean Hunk_DataOnHunk(void *data)
{
    if ((byte *)data < s_hunkData) {
        return 0;
    }
    if ((byte *)data >= s_hunkData + s_hunkTotal) {
        return 0;
    }
    return 1;
}

const char *Hunk_SetDataForFile(int type, const char *name, void *data, Alloc_t alloc)
{
    int hash;
    int nameLen;
    fileData_t *fd;

    hash = FS_HashFileName(name, 0x400);
    nameLen = strlen(name) + 1;
    /* header before name[]: x86 was 9 (data4+next4+type1); on x64 the two pointers
     * grow it to offsetof(name)=17, so `+9` under-allocated and strcpy overran by 8
     * -- hundreds of times (every file/model name), stomping adjacent hunk data. */
    fd = (fileData_t *)alloc(nameLen + (int)offsetof(fileData_t, name));
    fd->data = data;
    fd->type = (byte)type;
    strcpy(fd->name, name);
    fd->next = (intptr_t)com_fileDataHashTable[hash];
    com_fileDataHashTable[hash] = fd;

    return fd->name;
}

void Hunk_AddData(int type, void *data, Alloc_t alloc)
{
    fileData_t *fd;

    fd = (fileData_t *)alloc((int)offsetof(fileData_t, name));   /* was 9 (x86 header); x64 header is 17 */
    fd->data = data;
    fd->type = (byte)type;
    fd->next = (intptr_t)com_hunkData;
    com_hunkData = fd;
}

void Hunk_OverrideDataForFile(int type, const char *name, void *data)
{
    int hash;
    fileData_t *fd;

    hash = FS_HashFileName(name, 0x400);
    for (fd = com_fileDataHashTable[hash]; fd; fd = *(fileData_t **)&fd->next) {
        if (fd->type == type && !stricmp(fd->name, name)) {
            fd->data = data;
            return;
        }
    }
}

static void Hunk_ClearData(void)
{
    int hash;
    byte *low;
    byte *high;

    low = s_hunkData + hunk_low.permanent;
    high = s_hunkData + s_hunkTotal - hunk_high.permanent;

    for (hash = 0; hash < 1024; hash++) {
        Hunk_ClearFileData(&com_fileDataHashTable[hash], low, high);
    }
    Hunk_ClearFileData(&com_hunkData, low, high);
}

void DB_EnumXAssets(int type, XAssetEnum func, void *inData, int includeOverride)
{
    int i;
    fileData_t *fd;

    if (type != 1) {
        return;
    }

    for (i = 0; i < 1024; i++) {
        for (fd = com_fileDataHashTable[i]; fd; fd = *(fileData_t **)&fd->next) {
            if (fd->type == 4) {
                func(fd->data, inData);
            }
        }
    }
}

int Hunk_SetMark(void)
{
    return hunk_high.permanent;
}

void Hunk_ClearToMark(int mark)
{
    hunk_high.temp = mark;
    hunk_high.permanent = mark;
    Hunk_ClearData();
}

void Hunk_ClearToMarkLow(int mark)
{
    hunk_low.temp = mark;
    hunk_low.permanent = mark;
    Hunk_ClearData();
}

void Hunk_Clear(void)
{
    hunk_low.permanent = 0;
    hunk_low.temp = 0;
    hunk_high.permanent = 0;
    hunk_high.temp = 0;
    Hunk_ClearData();
}

int Hunk_Used(void)
{
    return hunk_low.permanent + hunk_high.permanent;
}

void DBG_Hunk_PrintUsage(const char *label)
{
    (void)label;
}

/* ---- hunk overflow detector (env COD2_HUNKGUARD) ----
   Each block gets a canary immediately after its requested size; the hunk grows
   downward so an overflow past a block's end writes toward the previous (higher)
   block and stomps that block's canary first. hunk_guard_check() reports the
   overflowing block + its allocation-site return addresses (preferred-VA @
   0x140000000 base, matches the .map / crash report). */
#if defined(COD2_X64)
extern unsigned short __stdcall RtlCaptureStackBackTrace(unsigned long, unsigned long, void **, unsigned long *);
extern void *__stdcall GetModuleHandleA(const char *);
#define HG_CANARY 0xA5A5A5A5A5A5A5A5ull
#define HG_MAX 8192
static struct { byte *end; int size; void *ra[4]; } hg_rec[HG_MAX];
static int hg_count;
static int hg_on = -1;
static unsigned long long hg_base;
static int hg_enabled(void) {
    if (hg_on < 0) { hg_on = getenv("COD2_HUNKGUARD") ? 1 : 0; if (hg_on) hg_base = (unsigned long long)GetModuleHandleA((const char *)0); }
    return hg_on;
}
void hunk_guard_check(const char *where)
{
    if (!hg_enabled()) return;
    int i;
    for (i = 0; i < hg_count; i++) {
        if (*(unsigned long long *)hg_rec[i].end != HG_CANARY) {
            unsigned long long b = hg_base;
            Com_Printf("[HunkGuard@%s] OVERFLOW size=%d end=%p RA: %llx %llx %llx %llx\n",
                       where, hg_rec[i].size, (void *)hg_rec[i].end,
                       (unsigned long long)hg_rec[i].ra[0] - b + 0x140000000ull,
                       (unsigned long long)hg_rec[i].ra[1] - b + 0x140000000ull,
                       (unsigned long long)hg_rec[i].ra[2] - b + 0x140000000ull,
                       (unsigned long long)hg_rec[i].ra[3] - b + 0x140000000ull);
            *(unsigned long long *)hg_rec[i].end = HG_CANARY; /* re-arm to avoid spam */
        }
    }
}
#else
void hunk_guard_check(const char *where) { (void)where; }
#endif

void *Hunk_AllocInternal(int size)
{
    int newHighUsed;
    byte *buf;
    int reqSize = size;

#if defined(COD2_X64)
    if (hg_enabled()) reqSize = size + 16;
#endif

    newHighUsed = (hunk_high.permanent + reqSize + 31) & ~31;
    hunk_high.permanent = newHighUsed;
    buf = s_hunkData + s_hunkTotal - newHighUsed;
    hunk_high.temp = newHighUsed;

    if (s_hunkTotal < newHighUsed + hunk_low.temp) {
        Com_Error(1, "Hunk_AllocAlign failed on %i bytes (total %i MB, low %i MB, high %i MB)",
                  size, TO_MB(s_hunkTotal), TO_MB(hunk_low.temp), TO_MB(newHighUsed));
    }

    memset(buf, 0, size);
#if defined(COD2_X64)
    if (hg_enabled() && hg_count < HG_MAX) {
        *(unsigned long long *)(buf + size) = HG_CANARY;
        hg_rec[hg_count].end = buf + size;
        hg_rec[hg_count].size = size;
        RtlCaptureStackBackTrace(1, 4, hg_rec[hg_count].ra, (unsigned long *)0);
        hg_count++;
    }
#endif
    return buf;
}

void *Hunk_AllocNoZeroInternal(int size)
{
    int newHighUsed;
    byte *buf;

    newHighUsed = (hunk_high.permanent + size + 31) & ~31;
    hunk_high.permanent = newHighUsed;
    buf = s_hunkData + s_hunkTotal - newHighUsed;
    hunk_high.temp = newHighUsed;

    if (s_hunkTotal < newHighUsed + hunk_low.temp) {
        Com_Error(1, "Hunk_AllocAlign failed on %i bytes (total %i MB, low %i MB, high %i MB)",
                  size, TO_MB(s_hunkTotal), TO_MB(hunk_low.temp), TO_MB(newHighUsed));
    }

    return buf;
}

void *Hunk_AllocAlignInternal(int size, int alignment)
{
    int newHighUsed;
    byte *buf;

    newHighUsed = (hunk_high.permanent + size + alignment - 1) & ~(alignment - 1);
    hunk_high.permanent = newHighUsed;
    buf = s_hunkData + s_hunkTotal - newHighUsed;
    hunk_high.temp = newHighUsed;

    if (s_hunkTotal < newHighUsed + hunk_low.temp) {
        Com_Error(1, "Hunk_AllocAlign failed on %i bytes (total %i MB, low %i MB, high %i MB)",
                  size, TO_MB(s_hunkTotal), TO_MB(hunk_low.temp), TO_MB(newHighUsed));
    }

    memset(buf, 0, size);
    return buf;
}

void *Hunk_AllocateTempMemoryHighInternal(int size)
{
    int newTempHigh;

    newTempHigh = (hunk_high.temp + size + 15) & ~15;
    hunk_high.temp = newTempHigh;

    if (newTempHigh + hunk_low.temp > s_hunkTotal) {
        Com_Error(1, "Hunk_AllocateTempMemoryHigh: failed on %i bytes (total %i MB, low %i MB, high %i MB)",
                  size, TO_MB(s_hunkTotal), TO_MB(hunk_low.temp), TO_MB(newTempHigh));
    }

    return s_hunkData + s_hunkTotal - newTempHigh;
}

void Hunk_ClearTempMemoryHigh(void)
{
    hunk_high.temp = hunk_high.permanent;
}

/* CoD2rev-Plus's script VM resets the high hunk to a mark (scr_evaluate.cpp). */
void Hunk_ClearHigh(int memory)
{
    hunk_high.temp = memory;
    hunk_high.permanent = memory;
    Hunk_ClearData();
}

void *Hunk_AllocLowInternal(int size)
{
    int aligned;
    byte *buf;
    int newLow;

    aligned = (hunk_low.permanent + 31) & ~31;
    buf = s_hunkData + aligned;
    newLow = aligned + size;
    hunk_low.permanent = newLow;
    hunk_low.temp = newLow;

    if (newLow + hunk_high.temp > s_hunkTotal) {
        Com_Error(1, "Hunk_AllocLowAlign failed on %i bytes (total %i MB, low %i MB, high %i MB)",
                  size, TO_MB(s_hunkTotal), TO_MB(newLow), TO_MB(hunk_high.temp));
    }

    memset(buf, 0, size);
    return buf;
}

void *Hunk_AllocLowAlignInternal(int size, int alignment)
{
    int aligned;
    byte *buf;
    int newLow;

    --alignment;
    aligned = (hunk_low.permanent + alignment) & ~alignment;
    buf = s_hunkData + aligned;
    newLow = aligned + size;
    hunk_low.permanent = newLow;
    hunk_low.temp = newLow;

    if (newLow + hunk_high.temp > s_hunkTotal) {
        Com_Error(1, "Hunk_AllocLowAlign failed on %i bytes (total %i MB, low %i MB, high %i MB)",
                  size, TO_MB(s_hunkTotal), TO_MB(newLow), TO_MB(hunk_high.temp));
    }

    memset(buf, 0, size);
    return buf;
}

void Hunk_ConvertTempToPermLowInternal(void)
{
    hunk_low.permanent = hunk_low.temp;
}

void *Hunk_AllocateTempMemoryInternal(int size)
{
    int allocSize;
    int prevTemp;
    int aligned;
    byte *base;
    int newTemp;
    byte *buf;

    if (!s_hunkData) {

        return Z_MallocInternal(size);
    }

    allocSize = size + 16;
    prevTemp = hunk_low.temp;
    aligned = (prevTemp + 15) & ~15;
    base = s_hunkData + aligned;
    newTemp = aligned + allocSize;
    hunk_low.temp = newTemp;

    if (newTemp + hunk_high.temp > s_hunkTotal) {
        Com_Error(1, "Hunk_AllocateTempMemory: failed on %i bytes (total %i MB, low %i MB, high %i MB), needs %i more hunk bytes",
                  allocSize, TO_MB(s_hunkTotal), TO_MB(newTemp), TO_MB(hunk_high.temp),
                  newTemp + hunk_high.temp - s_hunkTotal);
    }

    buf = base + 16;
    *(int *)base = 0x89537892;
    *(int *)(base + 4) = hunk_low.temp - prevTemp;
    return buf;
}

void *Hunk_ReallocateTempMemoryInternal(int size)
{
    int aligned;
    byte *buf;
    int newTemp;

    aligned = (hunk_low.permanent + 31) & ~31;
    buf = s_hunkData + aligned;
    newTemp = aligned + size;
    hunk_low.temp = newTemp;

    if (newTemp + hunk_high.temp > s_hunkTotal) {
        Com_Error(1, "Hunk_ReallocateTempMemory: failed on %i bytes (total %i MB, low %i MB, high %i MB)",
                  size, TO_MB(s_hunkTotal), TO_MB(newTemp), TO_MB(hunk_high.temp));
    }

    return buf;
}

void Hunk_FreeTempMemory(void *buf)
{
    byte *hdr;
    byte *hd = s_hunkData;

    if (!hd) {
        Z_FreeInternal(buf); /* matches Hunk_AllocateTempMemoryInternal's Z_MallocInternal path */
        return;
    }

    hdr = (byte *)buf - 16;
    if (*(int *)hdr != 0x89537892) {
        Com_Error(0, "\025Hunk_FreeTempMemory: bad magic");
    }
    *(int *)hdr = 0x89537893;
    hunk_low.temp -= *(int *)(hdr + 4);
}

void Hunk_ClearTempMemory(void)
{
    byte *hd = s_hunkData;
    if (hd) {
        hunk_low.temp = hunk_low.permanent;
    }
}

int Hunk_HideTempMemory(void)
{
    int mark;

    mark = hunk_low.permanent;
    hunk_low.permanent = hunk_low.temp;
    return mark;
}

void Hunk_ShowTempMemory(int mark)
{
    hunk_low.permanent = mark;
}

void LargeLocal_LargeLocal(const LargeLocal *_this, int size)
{
    int pos;

    pos = g_largeLocalPos;
    size = (size + 3) & ~3;
    g_largeLocalPos = pos + size;
    ((LargeLocal *)_this)->_placeholder = pos;
}

void ZN10LargeLocalD1Ev(LargeLocal *_this)
{
    g_largeLocalPos = _this->_placeholder;
}

void *LargeLocal_GetBuf(const LargeLocal *_this)
{
    return g_largeLocalBuf + _this->_placeholder;
}

void LargeLocalReset(void)
{
    g_largeLocalPos = 0;
}
