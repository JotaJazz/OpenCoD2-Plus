#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

#include <stdarg.h>
#include <stdlib.h>

#ifndef qtrue
#    define qtrue 1
#    define qfalse 0
#endif

extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void *Sys_GetValue(int);
extern void Sys_SetValue(int, void *);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorLtYellow;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorLtCyan;
extern vec4_t colorMdCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern TraceThreadInfo g_traceThreadInfo[1];
static int valueindex;
static va_info_t va_info[1];
static jmp_buf g_com_error[1];
static char value1[2][8192];
static short int (*LittleShort)(int);
static int (*LittleLong)(int);
static long long unsigned int (*LittleLong64)(long long unsigned int);
static float (*LittleFloatRead)(int);
static int (*LittleFloatWrite)(float);

int ColorIndex(int c);
char *Com_SkipPath(char *pathname);
const char *Com_GetExtensionSubString(const char *filename);
short int ShortSwap(int l);
short int ShortNoSwap(int l);
int LongSwap(int l);
int LongNoSwap(int l);
long long unsigned int Long64Swap(long long unsigned int l);
long long unsigned int Long64NoSwap(long long unsigned int ll);
float FloatReadSwap(int n);
float FloatReadNoSwap(int n);
int FloatWriteSwap(float f);
int FloatWriteNoSwap(float f);
Bool I_islower(int c);
Bool I_isupper(int c);
Bool I_isdigit(int c);
void I_strncpyz(char *dest, const char *src, int destsize);
int I_strncmp(const char *s0, const char *s1, int n);
int I_DrawStrlen(const char *str);
char *I_CleanStr(char *string);
int I_CleanChar(char character);
int Com_sprintf(char *dest, int size, const char *fmt, ...);
char *va(const char *format, ...);
void Com_InitThreadData(int threadContext);
void Info_NextPair(const char **head, char *key, char *value);
void Info_RemoveKey(char *s, const char *key);
void Info_RemoveKey_Big(char *s, const char *key);
qboolean Info_Validate(const char *s);
void Info_SetValueForKey(char *s, const char *key, const char *value);
void Info_SetValueForKey_Big(char *s, const char *key, const char *value);
float GetLeanFraction(const float fFrac);
float UnGetLeanFraction(const float fFrac);
void OrientationPosToWorldPos(const orientation_t *or_, const vec_t *pos, vec_t *out);
void OrientationDirToWorldDir(const orientation_t *or_, const vec_t *dir, vec_t *out);
void OrientationPosFromWorldPos(const orientation_t *or_, const vec_t *pos, vec_t *out);
void OrientationDirFromWorldDir(const orientation_t *or_, const vec_t *dir, vec_t *out);
int I_strnicmp(const char *s0, const char *s1, int n);
char *I_strupr(char *s);
char *I_strlwr(char *s);
void AddLeanToPosition(vec_t *position, const float fViewYaw, const float fLeanFrac, const float fViewRoll, const float fLeanDist);
void Swap_Init(void);
int I_strcmp(const char *s0, const char *s1);
void Com_StripExtension(const char *in, char *out);
int I_isforfilename(int c);
char *Info_ValueForKey(const char *s, const char *key);
int I_stricmp(const char *s0, const char *s1);
void Com_DefaultExtension(char *path, int maxSize, const char *extension);
void I_strncat(char *dest, int size, const char *src);
qboolean ParseConfigStringToStruct(byte *pStruct, const cspField_t *pFieldList, const int iNumFields, const char *pszBuffer, const int iMaxFieldTypes, qboolean (*parseSpecialFieldType)(byte *, const char *, const int), void (*parseStrcpy)(byte *, const char *));

int ColorIndex(int c)
{
    unsigned char index = (unsigned char)(c - '0');
    if (index > 9)
        return 7;
    return index;
}

char *Com_SkipPath(char *pathname)
{
    char *last = pathname;
    while (*pathname) {
        if (*pathname == '/')
            last = pathname + 1;
        pathname++;
    }
    return last;
}

const char *Com_GetExtensionSubString(const char *filename)
{
    const char *dot = NULL;
    const char *s = filename;

    while (*s) {
        if (*s == '.') {
            dot = s;
        } else if (*s == '/' || *s == '\\') {
            dot = NULL;
        }
        s++;
    }

    if (dot)
        return dot;
    return s;
}

short int ShortSwap(int l)
{
    unsigned short s = (unsigned short)l;
    s = (unsigned short)((s << 8) | (s >> 8));
    return (short)s;
}

short int ShortNoSwap(int l)
{
    return (short)l;
}

int LongSwap(int l)
{
    byte b1, b2, b3, b4;
    b1 = l & 255;
    b2 = (l >> 8) & 255;
    b3 = (l >> 16) & 255;
    b4 = (l >> 24) & 255;
    return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

int LongNoSwap(int l)
{
    return l;
}

long long unsigned int Long64Swap(long long unsigned int l)
{
    union {
        long long unsigned int ll;
        unsigned char b[8];
    } in, out;

    in.ll = l;
    out.b[0] = in.b[7];
    out.b[1] = in.b[6];
    out.b[2] = in.b[5];
    out.b[3] = in.b[4];
    out.b[4] = in.b[3];
    out.b[5] = in.b[2];
    out.b[6] = in.b[1];
    out.b[7] = in.b[0];
    return out.ll;
}

long long unsigned int Long64NoSwap(long long unsigned int ll)
{
    return ll;
}

float FloatReadSwap(int n)
{

    volatile union {
        float f;
        int i;
    } u;
    byte b1, b2, b3, b4;
    b1 = n & 255;
    b2 = (n >> 8) & 255;
    b3 = (n >> 16) & 255;
    b4 = (n >> 24) & 255;
    u.i = ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
    return u.f;
}

float FloatReadNoSwap(int n)
{

    volatile union {
        float f;
        int i;
    } u;
    u.i = n;
    return u.f;
}

int FloatWriteSwap(float f)
{
    union {
        float f;
        unsigned int i;
    } u;
    u.f = f;
    unsigned int x = u.i;
    return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}

int FloatWriteNoSwap(float f)
{
    union {
        float fl;
        int i;
    } u;
    u.fl = f;
    return u.i;
}

Bool I_islower(int c)
{
    return (unsigned)(c - 'a') <= 25;
}

Bool I_isupper(int c)
{
    return (unsigned)(c - 'A') <= 25;
}

Bool I_isdigit(int c)
{
    return (unsigned)(c - '0') <= 9;
}

static inline __attribute__((always_inline)) void I_strncpyz_core(char *dest, const char *src, int destsize)
{
    strncpy(dest, src, destsize - 1);
    dest[destsize - 1] = '\0';
}

void I_strncpyz(char *dest, const char *src, int destsize)
{
    I_strncpyz_core(dest, src, destsize);
}

int I_strncmp(const char *s0, const char *s1, int n)
{
    int c0, c1;
    int i = 0;

    do {
        c0 = (signed char)*s0++;
        c1 = (signed char)*s1++;
        if (i == n)
            return 0;
        if (c0 != c1)
            return c0 < c1 ? -1 : 1;
        i++;
    } while (c0);

    return 0;
}

int I_DrawStrlen(const char *str)
{
    int count = 0;

    while (*str) {
        if (*str == '^' && str[1] && str[1] != '^' &&
            str[1] >= '0' && str[1] <= '9') {
            str += 2;
        } else {
            str++;
            count++;
        }
    }
    return count;
}

char *I_CleanStr(char *string)
{
    char *d = string;
    char *s = string;

    while (*s) {
        if (*s == '^' && s[1] && s[1] != '^' &&
            s[1] >= '0' && s[1] <= '9') {
            s += 2;
            continue;
        }
        if (*s >= 0x20 && *s <= 0x7e)
            *d++ = *s;
        s++;
    }
    *d = '\0';
    return string;
}

int I_CleanChar(char character)
{
    if ((unsigned char)character == 0x92)
        return '\'';
    return character;
}

int Com_sprintf(char *dest, int size, const char *fmt, ...)
{
    int len;
    va_list argptr;

    va_start(argptr, fmt);
    len = vsnprintf(dest, size, fmt, argptr);
    va_end(argptr);
    dest[size - 1] = '\0';
    return len;
}

char *va(const char *format, ...)
{
    va_list argptr;
    va_info_t *info;
    char *buf;
    int len;

    va_start(argptr, format);
    info = (va_info_t *)Sys_GetValue(1);
    buf = info->va_string[info->index];
    info->index = (info->index + 1) % 2;

    len = vsnprintf(buf, 1024, format, argptr);
    va_end(argptr);
    buf[1023] = '\0';

    if ((unsigned)len > 1023) {
        Com_Error(1, "va string too long");
    }

    return buf;
}

void Com_InitThreadData(int threadContext)
{
    Sys_SetValue(1, &va_info[threadContext]);
    Sys_SetValue(2, &g_com_error[threadContext]);
    Sys_SetValue(3, &g_traceThreadInfo[threadContext]);
}

void Info_NextPair(const char **head, char *key, char *value)
{
    const char *s;
    char *o;

    s = *head;
    if (*s == '\\')
        s++;

    key[0] = '\0';
    value[0] = '\0';

    o = key;
#if COD2_FEATURE_NET_HARDENING

    {
        char *kend = key + 8191;
        while (*s != '\\') {
            if (!*s) {
                *o = '\0';
                *head = s;
                return;
            }
            if (o < kend)
                *o++ = *s;
            s++;
        }
    }
#else
    while (*s != '\\') {
        if (!*s) {
            *o = '\0';
            *head = s;
            return;
        }
        *o++ = *s++;
    }
#endif
    *o = '\0';
    s++;

    o = value;
#if COD2_FEATURE_NET_HARDENING
    {
        char *vend = value + 8191;
        while (*s != '\\' && *s) {
            if (o < vend)
                *o++ = *s;
            s++;
        }
    }
#else
    while (*s != '\\' && *s) {
        *o++ = *s++;
    }
#endif
    *o = '\0';

    *head = s;
}

void Info_RemoveKey(char *s, const char *key)
{
    char *start;
    char pkey[1024];
    char value[1024];
    char *o;

    if (strlen(s) >= 1024) {
        Com_Error(1, "\025Info_RemoveKey: oversize infostring");
    }

    if (strchr(key, '\\'))
        return;

    while (1) {
        start = s;
        if (*s == '\\')
            s++;

        o = pkey;
        while (*s != '\\') {
            if (!*s)
                return;
            *o++ = *s++;
        }
        *o = '\0';
        s++;

        o = value;
        while (*s != '\\' && *s) {
            *o++ = *s++;
        }
        *o = '\0';

        if (!strcmp(key, pkey)) {
            memmove(start, s, strlen(s) + 1); /* the two overlap: strcpy shuffles them on x64 */
            return;
        }

        if (!*s)
            return;
    }
}

void Info_RemoveKey_Big(char *s, const char *key)
{
    char *start;
    char pkey[8192];
    char value[8192];
    char *o;

    if (strlen(s) >= 8192) {
        Com_Error(1, "\025Info_RemoveKey_Big: oversize infostring");
    }

    if (strchr(key, '\\'))
        return;

    while (1) {
        start = s;
        if (*s == '\\')
            s++;

        o = pkey;
        while (*s != '\\') {
            if (!*s)
                return;
            *o++ = *s++;
        }
        *o = '\0';
        s++;

        o = value;
        while (*s != '\\' && *s) {
            *o++ = *s++;
        }
        *o = '\0';

        if (!strcmp(key, pkey)) {
            memmove(start, s, strlen(s) + 1); /* the two overlap: strcpy shuffles them on x64 */
            return;
        }

        if (!*s)
            return;
    }
}

qboolean Info_Validate(const char *s)
{
    if (strchr(s, '\"'))
        return qfalse;
    if (strchr(s, ';'))
        return qfalse;
    return qtrue;
}

void Info_SetValueForKey(char *s, const char *key, const char *value)
{
    char cleanValue[1024];
    char newi[1024];
    int i, j;
    int len;

    if (strlen(s) >= 1024) {
        Com_Printf("Info string length exceeded\n");
        return;
    }

    j = 0;
    for (i = 0; value[i] && i < 1023; i++) {
        if (value[i] != '\\' && value[i] != ';' && value[i] != '\"')
            cleanValue[j++] = value[i];
    }
    cleanValue[j] = '\0';

    if (strchr(key, '\\')) {
        Com_Printf("Can't use keys with a \\\nkey: '%s'\nvalue: '%s'\n", key, value);
        return;
    }
    if (strchr(key, ';')) {
        Com_Printf("Can't use keys with a semicolon\nkey: '%s'\nvalue: '%s'\n", key, value);
        return;
    }
    if (strchr(key, '\"')) {
        Com_Printf("Can't use keys with a \"\nkey: '%s'\nvalue: '%s'\n", key, value);
        return;
    }

    Info_RemoveKey(s, key);

    if (!cleanValue[0])
        return;

    len = Com_sprintf(newi, 1024, "\\%s\\%s", key, cleanValue);
    if (len <= 0) {
        Com_Printf("Info_SetValueForKey: Com_sprintf failed\n");
        return;
    }

    if (strlen(newi) + strlen(s) > 1024) {
        Com_Printf("Info string length exceeded\nkey: '%s'\nvalue: '%s'\nInfo string: '%s'\n", key, value, s);
        return;
    }

    strcat(s, newi);
}

void Info_SetValueForKey_Big(char *s, const char *key, const char *value)
{
    char cleanValue[8192];
    char newi[8192];
    int i, j;
    int len;

    if (strlen(s) >= 8192) {
        Com_Printf("Info string length exceeded\n");
        return;
    }

    j = 0;
    for (i = 0; value[i] && i < 8191; i++) {
        if (value[i] != '\\' && value[i] != ';' && value[i] != '\"')
            cleanValue[j++] = value[i];
    }
    cleanValue[j] = '\0';

    if (strchr(key, '\\')) {
        Com_Printf("Can't use keys with a \\\nkey: '%s'\nvalue: '%s'\n", key, value);
        return;
    }
    if (strchr(key, ';')) {
        Com_Printf("Can't use keys with a semicolon\nkey: '%s'\nvalue: '%s'\n", key, value);
        return;
    }
    if (strchr(key, '\"')) {
        Com_Printf("Can't use keys with a \"\nkey: '%s'\nvalue: '%s'\n", key, value);
        return;
    }

    Info_RemoveKey_Big(s, key);

    if (!cleanValue[0])
        return;

    len = Com_sprintf(newi, 8192, "\\%s\\%s", key, cleanValue);
    if (len <= 0) {
        Com_Printf("Info_SetValueForKey: Com_sprintf failed\n");
        return;
    }

    if (strlen(newi) + strlen(s) > 1024) {
        Com_Printf("Info string length exceeded\nkey: '%s'\nvalue: '%s'\nInfo string: '%s'\n", key, value, s);
        return;
    }

    strcat(s, newi);
}

static inline __attribute__((always_inline)) float GetLeanFraction_core(const float fFrac)
{
    return fFrac * (2.0f - __builtin_fabsf(fFrac));
}

float GetLeanFraction(const float fFrac)
{
    return GetLeanFraction_core(fFrac);
}

float UnGetLeanFraction(const float fFrac)
{
    return 1.0f - __builtin_sqrtf(1.0f - fFrac);
}

void OrientationPosToWorldPos(const orientation_t *or_, const vec_t *pos, vec_t *out)
{
    out[0] = or_->origin[0] + pos[0] * or_->axis[0][0] + pos[1] * or_->axis[1][0] + pos[2] * or_->axis[2][0];
    out[1] = or_->origin[1] + pos[0] * or_->axis[0][1] + pos[1] * or_->axis[1][1] + pos[2] * or_->axis[2][1];
    out[2] = or_->origin[2] + pos[0] * or_->axis[0][2] + pos[1] * or_->axis[1][2] + pos[2] * or_->axis[2][2];
}

void OrientationDirToWorldDir(const orientation_t *or_, const vec_t *dir, vec_t *out)
{
    out[0] = dir[0] * or_->axis[0][0] + dir[1] * or_->axis[1][0] + dir[2] * or_->axis[2][0];
    out[1] = dir[0] * or_->axis[0][1] + dir[1] * or_->axis[1][1] + dir[2] * or_->axis[2][1];
    out[2] = dir[0] * or_->axis[0][2] + dir[1] * or_->axis[1][2] + dir[2] * or_->axis[2][2];
}

void OrientationPosFromWorldPos(const orientation_t *or_, const vec_t *pos, vec_t *out)
{
    vec3_t delta;
    delta[0] = pos[0] - or_->origin[0];
    delta[1] = pos[1] - or_->origin[1];
    delta[2] = pos[2] - or_->origin[2];

    out[0] = delta[0] * or_->axis[0][0] + delta[1] * or_->axis[0][1] + delta[2] * or_->axis[0][2];
    out[1] = delta[0] * or_->axis[1][0] + delta[1] * or_->axis[1][1] + delta[2] * or_->axis[1][2];
    out[2] = delta[0] * or_->axis[2][0] + delta[1] * or_->axis[2][1] + delta[2] * or_->axis[2][2];
}

void OrientationDirFromWorldDir(const orientation_t *or_, const vec_t *dir, vec_t *out)
{
    out[0] = dir[0] * or_->axis[0][0] + dir[1] * or_->axis[0][1] + dir[2] * or_->axis[0][2];
    out[1] = dir[0] * or_->axis[1][0] + dir[1] * or_->axis[1][1] + dir[2] * or_->axis[1][2];
    out[2] = dir[0] * or_->axis[2][0] + dir[1] * or_->axis[2][1] + dir[2] * or_->axis[2][2];
}

static inline __attribute__((always_inline)) int I_strnicmp_core(const char *s0, const char *s1, int n)
{
    int c0, c1;
    int i = 0;

    do {
        c0 = (signed char)*s0++;
        c1 = (signed char)*s1++;
        if (i == n)
            return 0;
        if (c0 != c1) {
            if (c0 >= 'a' && c0 <= 'z')
                c0 -= 32;
            if (c1 >= 'a' && c1 <= 'z')
                c1 -= 32;
            if (c0 != c1)
                return c0 < c1 ? -1 : 1;
        }
        i++;
    } while (c0);

    return 0;
}

int I_strnicmp(const char *s0, const char *s1, int n)
{
    return I_strnicmp_core(s0, s1, n);
}

char *I_strupr(char *s)
{
    char *p = s;
    while (*p) {
        int c = (signed char)*p;
        if ((unsigned)(c - 'a') <= 25)
            *p -= 32;
        p++;
    }
    return s;
}

char *I_strlwr(char *s)
{
    char *p = s;
    while (*p) {
        int c = (signed char)*p;
        if (c >= 'A' && c <= 'Z')
            *p += 32;
        p++;
    }
    return s;
}

void AddLeanToPosition(vec_t *position, const float fViewYaw, const float fLeanFrac, const float fViewRoll, const float fLeanDist)
{
    vec3_t vRight, vAng;
    float leanFrac, scale;

    if (fLeanFrac == 0.0f)
        return;

    leanFrac = GetLeanFraction_core(fLeanFrac);

    vAng[0] = 0.0f;
    vAng[1] = fViewYaw;
    vAng[2] = fViewRoll * leanFrac;

    AngleVectors(vAng, NULL, vRight, NULL);

    scale = leanFrac * fLeanDist;
    position[0] += scale * vRight[0];
    position[1] += scale * vRight[1];
    position[2] += scale * vRight[2];
}

void Swap_Init(void)
{
    byte swaptest[2] = { 1, 0 };

    if (*(short *)swaptest == 1) {

        LittleShort = ShortNoSwap;
        LittleLong = LongNoSwap;
        LittleLong64 = Long64NoSwap;
        LittleFloatRead = FloatReadNoSwap;
        LittleFloatWrite = FloatWriteNoSwap;
    } else {

        LittleShort = ShortSwap;
        LittleLong = LongSwap;
        LittleLong64 = Long64Swap;
        LittleFloatRead = FloatReadSwap;
        LittleFloatWrite = FloatWriteSwap;
    }
}

int I_strcmp(const char *s0, const char *s1)
{
    int c0, c1;
    int n = 0x7fffffff;

    do {
        c0 = (signed char)*s0++;
        c1 = (signed char)*s1++;
        if (--n == -1)
            return 0;
        if (c0 != c1)
            return c0 < c1 ? -1 : 1;
    } while (c0);

    return 0;
}

void Com_StripExtension(const char *in, char *out)
{
    const char *dot = NULL;
    const char *s = in;

    while (*s) {
        if (*s == '.')
            dot = s;
        else if (*s == '/' || *s == '\\')
            dot = NULL;
        s++;
    }

    if (!dot)
        dot = s;

    while (in != dot)
        *out++ = *in++;
    *out = '\0';
}

int I_isforfilename(int c)
{
    if ((unsigned)(c - 'a') <= 25 || (unsigned)(c - 'A') <= 25 || (unsigned)(c - '0') <= 9 || c == '_' || c == '-')
        return 1;
    return 0;
}

char *Info_ValueForKey(const char *s, const char *key)
{
    char pkey[8192];
    char *result;
    char *o;
    const char *valStart;

    if (!s || !key)
        return "";

    if (strlen(s) >= 8192) {
        Com_Error(1, "Info_ValueForKey: oversize infostring");
    }

    valueindex ^= 1;
    result = value1[valueindex];

    if (*s == '\\')
        s++;

    while (1) {
        o = pkey;
        while (*s != '\\') {
            if (!*s)
                return "";
            *o++ = *s++;
        }
        *o = '\0';
        s++;

        valStart = s;
        o = result;
        while (*s != '\\' && *s) {
            *o++ = *s++;
        }
        *o = '\0';

        if (!I_strnicmp_core(key, pkey, 0x7fffffff))
            return result;

        if (!*s)
            break;
        s++;
    }

    return "";
}

int I_stricmp(const char *s0, const char *s1)
{
    return I_strnicmp_core(s0, s1, 0x7fffffff);
}

void Com_DefaultExtension(char *path, int maxSize, const char *extension)
{
    const char *src;

    src = path + strlen(path) - 1;

    while (src > path) {
        if (*src == '/')
            break;
        if (*src == '.') {
            return;
        }
        src--;
    }

    {
        char oldPath[64];
        I_strncpyz_core(oldPath, path, sizeof(oldPath));
        Com_sprintf(path, maxSize, "%s%s", oldPath, extension);
    }
}

void I_strncat(char *dest, int size, const char *src)
{
    int destLen;

    destLen = strlen(dest);
    if (destLen >= size) {
        Com_Error(0, "I_strncat: already overflowed");
    }

    I_strncpyz_core(dest + destLen, src, size - destLen);
}

qboolean ParseConfigStringToStruct(byte *pStruct, const cspField_t *pFieldList, const int iNumFields, const char *pszBuffer, const int iMaxFieldTypes, qboolean (*parseSpecialFieldType)(byte *, const char *, const int), void (*parseStrcpy)(byte *, const char *))
{
    int iField;
    const cspField_t *pField;

    if (iNumFields < 0)
        return qfalse;
    if (iNumFields == 0)
        return qtrue;

    for (iField = 0; iField < iNumFields; iField++) {
        const char *val;
        int fieldType;
        byte *dest;

        pField = &pFieldList[iField];
        val = Info_ValueForKey(pszBuffer, pField->szName);
        if (*val == '\0')
            continue;

        fieldType = pField->iFieldType;
        dest = pStruct + pField->iOffset;

        if (fieldType > 7) {
            if (iMaxFieldTypes > 0 && fieldType < iMaxFieldTypes) {
                if (!parseSpecialFieldType(pStruct, val, fieldType))
                    return qfalse;
            } else {
                Com_Error(1, "Bad field type %i\n", fieldType);
            }
            continue;
        }

        if (fieldType < 0)
            continue;

        switch (fieldType) {
        case 0:
            parseStrcpy(dest, val);
            break;
        case 1:
            strncpy((char *)dest, val, 1023);
            dest[1023] = '\0';
            break;
        case 2:
            strncpy((char *)dest, val, 63);
            dest[63] = '\0';
            break;
        case 3:
            strncpy((char *)dest, val, 255);
            dest[255] = '\0';
            break;
        case 4:
            *(int *)dest = atoi(val);
            break;
        case 5:
            *(int *)dest = (atoi(val) != 0);
            break;
        case 6:
            *(float *)dest = (float)atof(val);
            break;
        case 7:
            *(int *)dest = (int)((float)atof(val) * 1000.0f);
            break;
        }
    }

    return qtrue;
}

vec4_t colorLtGrey = { 0.75f, 0.75f, 0.75f, 1.0f };

vec4_t colorWhite = { 1.0f, 1.0f, 1.0f, 1.0f }; /* truncated 8->4 (blob over-capture) */

vec4_t colorMdCyan = { 0.0f, 0.5f, 0.5f, 1.0f };

vec4_t colorLtCyan = { 0.0f, 0.75f, 0.75f, 1.0f };

vec4_t colorCyan = { 0.0f, 1.0f, 1.0f, 1.0f };

vec4_t colorMagenta = { 1.0f, 0.0f, 1.0f, 1.0f }; /* truncated 8->4 (blob over-capture) */

vec4_t colorLtYellow = { 0.75f, 0.75f, 0.0f, 1.0f };

vec4_t colorYellow = { 1.0f, 1.0f, 0.0f, 1.0f }; /* truncated 8->4 (blob over-capture) */

vec4_t colorBlue = { 0.0f, 0.0f, 1.0f, 1.0f }; /* truncated 8->4 (blob over-capture) */

vec4_t colorGreen = { 0.0f, 1.0f, 0.0f, 1.0f };

vec4_t colorRed = { 1.0f, 0.0f, 0.0f, 1.0f };

vec4_t colorBlack = { 0.0f, 0.0f, 0.0f, 1.0f }; /* truncated 20->4 (blob over-capture) */
