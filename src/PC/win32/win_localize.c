#include "common_types.h"
#include <string.h>
#include "imports.h"
#include <stdio.h>

extern FILE *FS_FileOpen(const char *filename, const char *mode);
extern size_t FS_FileRead(void *buffer, size_t size, size_t count, FILE *stream);
extern int FS_FileClose(FILE *stream);
extern int FS_FileSeek(FILE *file, long int offset, int origin);
extern qboolean SEH_GetLanguageIndexForName(const char *name, language_t *lang);
extern void Com_BeginParseSession(const char *name);
extern void Com_EndParseSession(void);
extern const char *Com_Parse(const char **buf);
extern char *va(const char *fmt, ...);

static char *localization;
static char *localized_strings;
static char language_buffer[4096];

language_t Win_InitLocalization(void)
{
    FILE *fp;
    int size;
    char *p;
    language_t lang;

    localization = 0;
    localized_strings = 0;

    fp = FS_FileOpen("localization.txt", "r");
    if (!fp)
        return (language_t)(0);

    FS_FileSeek(fp, 0, 2);
    size = (int)ftell(fp);
    FS_FileSeek(fp, 0, 0);

    localization = language_buffer;
    size = (int)FS_FileRead(language_buffer, 1, (size_t)size, fp);
    FS_FileClose(fp);

    if (!size) {
        localization = 0;
        return (language_t)(0);
    }

    localization[size] = '\0';

    lang = (language_t)(0);
    p = localization;
    while (*p) {
        if (*p == '\n') {
            *p = '\0';
            p++;
            localized_strings = p;
            SEH_GetLanguageIndexForName(localization, &lang);
            break;
        }
        p++;
    }

    return lang;
}

void Win_ShutdownLocalization(void)
{
    localization = 0;
    localized_strings = 0;
}

const char *Win_LocalizeRef(const char *ref)
{
    const char *strings;
    const char *key;
    const char *value;
    int match;

    Com_BeginParseSession("localization");
    strings = localized_strings;

    for (;;) {
        key = Com_Parse(&strings);
        if (*key == '\0')
            break;

        match = strcmp(key, ref);

        value = Com_Parse(&strings);
        if (*value == '\0')
            break;

        if (match == 0) {
            Com_EndParseSession();
            return va("%s", value);
        }
    }

    Com_EndParseSession();
    return va("%s", ref);
}
