#include "common_types.h"
#include "imports.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#define STRINGED_NOINLINE __declspec(noinline)
#else
#define STRINGED_NOINLINE __attribute__((noinline))
#endif

extern CStringEdPackage *TheStringPackage;
static char sString[64];
static char sString_00482f80[64];
static char sTemp[64];

qboolean CStringEdPackage_ReadLine(const CStringEdPackage *_this, const char **psParsePos, char *psDest);
void CStringEdPackage_SetupNewFileParse(const CStringEdPackage *_this, const char *psFileName);
static const char *SE_GetFoundFile(LocalizeString *strResult);
LocalizeString CStringEdPackage_ConvertCRLiterals_Read(const CStringEdPackage *_this, LocalizeString *string);
LocalizeString CStringEdPackage_InsideQuotes(const CStringEdPackage *_this, const char *psLine);
void CStringEdPackage_AddEntry(const CStringEdPackage *_this, const char *psLocalReference);
void CStringEdPackage_SetString(const CStringEdPackage *_this, const char *psLocalReference, const char *psNewString, qboolean bSentenceIsEnglish);
const char *CStringEdPackage_ParseLine(const CStringEdPackage *_this, const char *psLine, int forceEnglish);
const char *SE_Load(const char *psFileName, int forceEnglish);
STRINGED_NOINLINE const char *SE_GetString(const char *psPackageAndStringReference);
void CStringEdPackage_Clear(const CStringEdPackage *_this);
const char *SE_LoadLanguage(int forceEnglish);
void SE_Init(void);
void SE_ShutDown(void);
void ZNSt8_Rb_treeISsSt4pairIKSsSsESt10_Select1stIS2_ESt4lessISsESaIS2_EE11lower_boundERS1_(void);
void ZNSt8_Rb_treeISsSt4pairIKSsSsESt10_Select1stIS2_ESt4lessISsESaIS2_EE4findERS1_(void);
void ZNSt8_Rb_treeISsSt4pairIKSsSsESt10_Select1stIS2_ESt4lessISsESaIS2_EE9_M_insertEPSt18_Rb_tree_node_baseSA_RKS2_(void);
void ZNSt8_Rb_treeISsSt4pairIKSsSsESt10_Select1stIS2_ESt4lessISsESaIS2_EE13insert_uniqueERKS2_(void);
void ZNSt8_Rb_treeISsSt4pairIKSsSsESt10_Select1stIS2_ESt4lessISsESaIS2_EE13insert_uniqueESt17_Rb_tree_iteratorIS2_ERKS2_(void);
void ZNSt8_Rb_treeISsSt4pairIKSsSsESt10_Select1stIS2_ESt4lessISsESaIS2_EE8_M_eraseEPSt13_Rb_tree_nodeIS2_E(void);

qboolean CStringEdPackage_ReadLine(const CStringEdPackage *_this, const char **psParsePos, char *psDest)
{
    const char *p = *psParsePos;
    const char *nl;
    int len, i;

    if (!*p)
        return 0;

    nl = strchr(p, '\n');
    if (nl) {
        int charsToCopy = nl - p;
        strncpy(psDest, p, charsToCopy);
        psDest[charsToCopy] = '\0';
        *psParsePos = p + charsToCopy;

        while (**psParsePos && strchr((const char *)"\r\n", **psParsePos)) {
            (*psParsePos)++;
        }
    } else {
        strcpy(psDest, p);
        *psParsePos = p + strlen(p);
    }

    if (!psDest[0])
        return 1;

    len = strlen(psDest);
    for (i = len - 1; i >= 0; i--) {
        if (!isspace((unsigned char)psDest[i]))
            break;
        psDest[i] = '\0';
    }

    {
        char *s = psDest;
        for (;;) {
            char *comment = strstr(s, (const char *)"//");
            int quoteCount = 0;
            if (!comment)
                return 1;

            for (i = 0; i < (int)(comment - s); i++) {
                if (s[i] == '"')
                    quoteCount++;
            }

            if (quoteCount & 1) {

                s = comment + 1;
                continue;
            }

            *comment = '\0';
            if (!*s)
                return 1;

            len = strlen(s);
            for (i = len - 1; i >= 0; i--) {
                if (!isspace((unsigned char)s[i]))
                    break;
                s[i] = '\0';
            }
            return 1;
        }
    }
}

extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern const char **FS_ListFiles(const char *path, const char *extension, int behavior, int *numfiles, int allocTrackType);
extern void FS_FreeFileList(const char **list, int allocTrackType);
extern unsigned char *SE_LoadFileData(const char *psFileName);
extern void SE_FreeFileDataAfterLoad(unsigned char *psLoadedFile);
extern int SEH_GetCurrentLanguage(void);
extern const char *SEH_GetLanguageName(const int iLanguage);

typedef struct WebStringEdEntry {
    char *key;
    char *value;
    struct WebStringEdEntry *next;
} WebStringEdEntry;

static WebStringEdEntry *webStringEntries;
static char webStringPackageName[64];
static char webStringLocalReference[1024];
static char webStringEnglishValue[4096];
static char webStringError[1024];
static qboolean webStringFoundEndMarker;

static char *WebSE_CopyString(const char *text)
{
    char *copy;
    unsigned int len;

    if (!text) {
        text = "";
    }

    len = strlen(text) + 1;
    copy = (char *)Z_MallocInternal(len);
    memcpy(copy, text, len);
    return copy;
}

static void WebSE_FreeEntry(WebStringEdEntry *entry)
{
    if (entry->key) {
        Z_FreeInternal(entry->key);
    }
    if (entry->value) {
        Z_FreeInternal(entry->value);
    }
    Z_FreeInternal(entry);
}

static void WebSE_ClearEntries(void)
{
    WebStringEdEntry *entry;

    entry = webStringEntries;
    while (entry) {
        WebStringEdEntry *next = entry->next;
        WebSE_FreeEntry(entry);
        entry = next;
    }

    webStringEntries = NULL;
}

static int WebSE_Strnicmp(const char *a, const char *b, unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        ca = (unsigned char)tolower(ca);
        cb = (unsigned char)tolower(cb);
        if (ca != cb || ca == '\0' || cb == '\0') {
            return (int)ca - (int)cb;
        }
    }

    return 0;
}

static int WebSE_Stricmp(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;

    do {
        ca = (unsigned char)tolower(*a++);
        cb = (unsigned char)tolower(*b++);
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
    } while (ca && cb);

    return 0;
}

static const char *WebSE_SkipWhitespace(const char *text)
{
    while (*text == ' ' || *text == '\t') {
        text++;
    }
    return text;
}

static const char *WebSE_Error1(const char *fmt, const char *arg)
{
    snprintf(webStringError, sizeof(webStringError), fmt, arg);
    return webStringError;
}

static const char *WebSE_VersionError(int got)
{
    snprintf(webStringError, sizeof(webStringError), "Unexpected version number %d, expecting %d!\n", got, 1);
    return webStringError;
}

static void WebSE_InsideQuotes(const char *line, char *out, unsigned int outSize)
{
    unsigned int len;

    line = WebSE_SkipWhitespace(line);
    if (*line == '"') {
        line++;
    }

    if (outSize == 0) {
        return;
    }

    strncpy(out, line, outSize - 1);
    out[outSize - 1] = '\0';

    len = strlen(out);
    while (len > 0 && (out[len - 1] == ' ' || out[len - 1] == '\t')) {
        out[--len] = '\0';
    }
    if (len > 0 && out[len - 1] == '"') {
        out[--len] = '\0';
    }
}

static void WebSE_ConvertCRLiterals(char *text)
{
    char *read;
    char *write;

    read = text;
    write = text;
    while (*read) {
        if (read[0] == '\\' && read[1] == 'n') {
            *write++ = '\n';
            read += 2;
        } else {
            *write++ = *read++;
        }
    }
    *write = '\0';
}

static const char *WebSE_ValidateFormatArgs(const char *text)
{
    qboolean usedArgs[9];
    const char *scan;
    int i;

    for (i = 0; i < 9; i++) {
        usedArgs[i] = 0;
    }

    scan = strstr(text, "&&");
    while (scan) {
        int argIndex;

        if (scan[2] < '1' || scan[2] > '9') {
            return WebSE_Error1("Illegal string format \"%s\"\n", text);
        }

        argIndex = scan[2] - '1';
        if (usedArgs[argIndex]) {
            return WebSE_Error1("Illegal string format \"%s\"\n", text);
        }

        usedArgs[argIndex] = 1;
        scan = strstr(scan + 3, "&&");
    }

    return NULL;
}

static void WebSE_BuildFullReference(const char *localReference, char *out, unsigned int outSize)
{
    if (outSize == 0) {
        return;
    }

    snprintf(out, outSize, "%s_%s", webStringPackageName, localReference ? localReference : "");
    out[outSize - 1] = '\0';
}

static WebStringEdEntry *WebSE_FindEntry(const char *key)
{
    WebStringEdEntry *entry;

    for (entry = webStringEntries; entry; entry = entry->next) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
    }

    return NULL;
}

static WebStringEdEntry *WebSE_AddOrFindEntry(const char *key)
{
    WebStringEdEntry *entry;

    entry = WebSE_FindEntry(key);
    if (entry) {
        return entry;
    }

    entry = (WebStringEdEntry *)Z_MallocInternal(sizeof(*entry));
    entry->key = WebSE_CopyString(key);
    entry->value = WebSE_CopyString("");
    entry->next = webStringEntries;
    webStringEntries = entry;
    return entry;
}

static void WebSE_SetEntryValue(WebStringEdEntry *entry, const char *value)
{
    if (entry->value) {
        Z_FreeInternal(entry->value);
    }
    entry->value = WebSE_CopyString(value);
}

static qboolean WebSE_JoinPath(const char *dir, const char *name, char *out, unsigned int outSize)
{
    int written;

    written = snprintf(out, outSize, "%s/%s", dir, name);
    if (written < 0 || (unsigned int)written >= outSize) {
        return 0;
    }
    return 1;
}

void CStringEdPackage_SetupNewFileParse(const CStringEdPackage *_this, const char *psFileName)
{
    const char *base;
    const char *slash;
    const char *backslash;
    const char *dot;
    unsigned int len;
    unsigned int i;

    (void)_this;

    base = psFileName;
    slash = strrchr(psFileName, '/');
    backslash = strrchr(psFileName, '\\');
    if (slash && slash + 1 > base) {
        base = slash + 1;
    }
    if (backslash && backslash + 1 > base) {
        base = backslash + 1;
    }

    dot = strrchr(base, '.');
    len = dot ? (unsigned int)(dot - base) : strlen(base);
    if (len >= sizeof(webStringPackageName)) {
        len = sizeof(webStringPackageName) - 1;
    }

    for (i = 0; i < len; i++) {
        webStringPackageName[i] = (char)toupper(base[i]);
    }
    webStringPackageName[len] = '\0';
    webStringLocalReference[0] = '\0';
    webStringEnglishValue[0] = '\0';
    webStringFoundEndMarker = 0;
}

void CStringEdPackage_AddEntry(const CStringEdPackage *_this, const char *psLocalReference)
{
    char fullReference[1152];

    (void)_this;

    WebSE_BuildFullReference(psLocalReference, fullReference, sizeof(fullReference));
    WebSE_AddOrFindEntry(fullReference);

    strncpy(webStringLocalReference, psLocalReference, sizeof(webStringLocalReference) - 1);
    webStringLocalReference[sizeof(webStringLocalReference) - 1] = '\0';
}

void CStringEdPackage_SetString(const CStringEdPackage *_this, const char *psLocalReference, const char *psNewString, qboolean bSentenceIsEnglish)
{
    WebStringEdEntry *entry;
    char fullReference[1152];
    const char *value;

    (void)_this;

    WebSE_BuildFullReference(psLocalReference, fullReference, sizeof(fullReference));
    entry = WebSE_AddOrFindEntry(fullReference);

    value = psNewString;
    if (bSentenceIsEnglish) {
        strncpy(webStringEnglishValue, psNewString, sizeof(webStringEnglishValue) - 1);
        webStringEnglishValue[sizeof(webStringEnglishValue) - 1] = '\0';
    } else if (WebSE_Stricmp(psNewString, "#same") == 0) {
        value = webStringEnglishValue;
    }

    WebSE_SetEntryValue(entry, value);
}

const char *CStringEdPackage_ParseLine(const CStringEdPackage *_this, const char *psLine, int forceEnglish)
{
    char token[1024];
    char sentence[4096];
    const char *afterKeyword;
    const char *error;
    int isEnglish;
    int isCurrentLanguage;

    if (WebSE_Strnicmp(psLine, "VERSION", 7) == 0) {
        WebSE_InsideQuotes(psLine + 7, token, sizeof(token));
        if (atoi(token) != 1) {
            return WebSE_VersionError(atoi(token));
        }
        return NULL;
    }

    if (WebSE_Strnicmp(psLine, "CONFIG", 6) == 0 ||
        WebSE_Strnicmp(psLine, "FILENOTES", 9) == 0 ||
        WebSE_Strnicmp(psLine, "NOTES", 5) == 0 ||
        WebSE_Strnicmp(psLine, "FLAGS", 5) == 0) {
        return NULL;
    }

    if (WebSE_Strnicmp(psLine, "REFERENCE", 9) == 0) {
        WebSE_InsideQuotes(psLine + 9, token, sizeof(token));
        CStringEdPackage_AddEntry(_this, token);
        return NULL;
    }

    if (WebSE_Strnicmp(psLine, "ENDMARKER", 9) == 0) {
        webStringFoundEndMarker = 1;
        return NULL;
    }

    if (WebSE_Strnicmp(psLine, "LANG_", 5) != 0) {
        return WebSE_Error1("Unknown keyword at linestart: \"%s\"\n", psLine);
    }

    if (webStringLocalReference[0] == '\0') {
        return "Error parsing file: Unexpected \"LANG_\"\n";
    }

    afterKeyword = psLine + 5;
    {
        unsigned int len = 0;
        while (afterKeyword[len] &&
               afterKeyword[len] != ' ' &&
               afterKeyword[len] != '\t' &&
               len < sizeof(token) - 1) {
            token[len] = afterKeyword[len];
            len++;
        }
        token[len] = '\0';
        afterKeyword += len;
    }

    WebSE_InsideQuotes(afterKeyword, sentence, sizeof(sentence));
    WebSE_ConvertCRLiterals(sentence);

    error = WebSE_ValidateFormatArgs(sentence);
    isEnglish = (WebSE_Stricmp(token, "english") == 0);
    isCurrentLanguage = (!forceEnglish &&
                         WebSE_Stricmp(token, SEH_GetLanguageName(SEH_GetCurrentLanguage())) == 0);
    if (!error && (isEnglish || isCurrentLanguage)) {
        CStringEdPackage_SetString(_this, webStringLocalReference, sentence, isEnglish);
    }

    return error;
}

void CStringEdPackage_Clear(const CStringEdPackage *_this)
{
    (void)_this;

    WebSE_ClearEntries();
    webStringPackageName[0] = '\0';
    webStringLocalReference[0] = '\0';
    webStringEnglishValue[0] = '\0';
    webStringFoundEndMarker = 0;
}

const char *SE_Load(const char *psFileName, int forceEnglish)
{
    unsigned char *psLoadedData;
    const char *psParsePos;
    const char *psErrorMessage;
    char sLineBuffer[0x4000];

    psLoadedData = SE_LoadFileData(psFileName);
    if (!psLoadedData) {
        return WebSE_Error1("Unable to load \"%s\"!", psFileName);
    }

    CStringEdPackage_SetupNewFileParse(TheStringPackage, psFileName);
    psParsePos = (const char *)psLoadedData;
    psErrorMessage = NULL;

    while (CStringEdPackage_ReadLine(TheStringPackage, &psParsePos, sLineBuffer)) {
        if (sLineBuffer[0] == '\0') {
            continue;
        }

        psErrorMessage = CStringEdPackage_ParseLine(TheStringPackage, sLineBuffer, forceEnglish);
        if (psErrorMessage) {
            break;
        }
    }

    SE_FreeFileDataAfterLoad(psLoadedData);

    if (!psErrorMessage && !webStringFoundEndMarker) {
        psErrorMessage = WebSE_Error1("Truncated file, failed to find \"%s\" at file end!", "ENDMARKER");
    }

    return psErrorMessage;
}

STRINGED_NOINLINE const char *SE_GetString(const char *psPackageAndStringReference)
{
    WebStringEdEntry *entry;

    if (!TheStringPackage) {
        return NULL;
    }

    entry = WebSE_FindEntry(psPackageAndStringReference);
    if (!entry) {
        return NULL;
    }

    return entry->value;
}

static void WebSE_LoadDirectory(const char *dir, int forceEnglish, const char **firstError, int *fileCount, int depth)
{
    const char **dirFiles;
    const char **sysFiles;
    int numDirs;
    int numFiles;
    int i;

    if (*firstError || depth > 16) {
        return;
    }

    dirFiles = FS_ListFiles(dir, "/", 0, &numDirs, 10);
    if (dirFiles) {
        for (i = 0; i < numDirs && !*firstError; i++) {
            char path[256];

            if (!dirFiles[i] || dirFiles[i][0] == '\0' || dirFiles[i][0] == '.') {
                continue;
            }
            if (!WebSE_JoinPath(dir, dirFiles[i], path, sizeof(path))) {
                *firstError = WebSE_Error1("Unable to load \"%s\"!", dirFiles[i]);
                break;
            }
            WebSE_LoadDirectory(path, forceEnglish, firstError, fileCount, depth + 1);
        }
        FS_FreeFileList(dirFiles, 10);
    }

    if (*firstError) {
        return;
    }

    sysFiles = FS_ListFiles(dir, "str", 0, &numFiles, 10);
    if (sysFiles) {
        for (i = 0; i < numFiles && !*firstError; i++) {
            char path[256];
            const char *error;

            if (!sysFiles[i] || sysFiles[i][0] == '\0') {
                continue;
            }
            if (!WebSE_JoinPath(dir, sysFiles[i], path, sizeof(path))) {
                *firstError = WebSE_Error1("Unable to load \"%s\"!", sysFiles[i]);
                break;
            }

            (*fileCount)++;
            error = SE_Load(path, forceEnglish);
            if (error) {
                *firstError = error;
                break;
            }
        }
        FS_FreeFileList(sysFiles, 10);
    }
}

const char *SE_LoadLanguage(int forceEnglish)
{
    const char *psErrorMessage;
    int fileCount;

    if (!TheStringPackage) {
        SE_Init();
    }

    CStringEdPackage_Clear(TheStringPackage);

    psErrorMessage = NULL;
    fileCount = 0;
    WebSE_LoadDirectory("localizedstrings", forceEnglish, &psErrorMessage, &fileCount, 0);
    return psErrorMessage;
}

void SE_Init(void)
{
    if (TheStringPackage) {
        CStringEdPackage_Clear(TheStringPackage);
        return;
    }

    TheStringPackage = (CStringEdPackage *)Z_MallocInternal(COD2_SIZE_1_3_PADDED(0x28));
    memset(TheStringPackage, 0, COD2_SIZE_1_3_PADDED(0x28));
    CStringEdPackage_Clear(TheStringPackage);
}

void SE_ShutDown(void)
{
    if (!TheStringPackage) {
        return;
    }

    CStringEdPackage_Clear(TheStringPackage);
    Z_FreeInternal(TheStringPackage);
    TheStringPackage = NULL;
}

const unsigned char __ZZN16CStringEdPackage9ParseLineEPKchE5C_208[1120] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x24, 0x04, 0x05, 0x00, 0x4e, 0x04, 0x05, 0x00, 0xea, 0x04, 0x05, 0x00, 0xea, 0x04, 0x05, 0x00,
    0xea, 0x04, 0x05, 0x00, 0xa1, 0x04, 0x05, 0x00, 0xd6, 0x04, 0x05, 0x00, 0xea, 0x04, 0x05, 0x00,
    0xea, 0x04, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
