#include "common_types.h"
#include "imports.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern punctuation_t default_punctuations[54];

extern void Com_Printf(const char *fmt, ...);

extern void FreeMemory(void *ptr);
extern void *GetMemory(unsigned long size);
extern void *GetClearedMemory(unsigned long size);
#include "l_menu_gfxapi.h"
extern int FS_FOpenFileRead(const char *filename, fileHandle_t *file, qboolean uniqueFILE);
extern int FS_Read(void *buffer, int len, fileHandle_t f);
extern void FS_FCloseFile(fileHandle_t f);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int Com_Compress(char *data_p);

void PS_CreatePunctuationTable(script_t *script, punctuation_t *punctuations);
void ScriptError(script_t *script, char *str, ...);
void ScriptWarning(script_t *script, char *str, ...);
int PS_ReadEscapeCharacter(script_t *script, char *ch);
void NumberValue(char *string, int subtype, long unsigned int *intvalue, long double *floatvalue);
int PS_ReadNumber(script_t *script, token_t *token);
void StripDoubleQuotes(char *string);
int EndOfScript(script_t *script);
void FreeScript(script_t *script);
script_t * LoadScriptFile(const char *filename);
int PS_ReadString(script_t *script, token_t *token, int quote);
int PS_ReadToken(script_t *script, token_t *token);

static int PS_IsNameStart(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int PS_IsNameChar(int c)
{
    return PS_IsNameStart(c) || (c >= '0' && c <= '9');
}

static int PS_IsHexTokenChar(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == 'A';
}

static int PS_SkipWhitespaceAndComments(script_t *script)
{
    char *p;

    for (;;) {
        int c;

        p = script->script_p;
        c = (unsigned char)*p;

        if (c <= ' ') {
            if (!c) {
                return 0;
            }
            if (c == '\n') {
                ++script->line;
            }
            script->script_p = p + 1;
            continue;
        }

        if (c != '/') {
            return 1;
        }

        if (p[1] == '/') {
            script->script_p = p + 1;
            p += 2;
            while (*p) {
                script->script_p = p;
                if (*p == '\n') {
                    ++script->line;
                    script->script_p = p + 1;
                    break;
                }
                ++p;
            }
            if (!*p) {
                return 0;
            }
            continue;
        }

        if (p[1] != '*') {
            return 1;
        }

        script->script_p = p + 1;
        p += 2;
        for (;;) {
            script->script_p = p;
            if (!*p) {
                return 0;
            }
            if (*p == '\n') {
                ++script->line;
            }
            if (p[0] == '*' && p[1] == '/') {
                script->script_p = p + 2;
                if (!p[2]) {
                    return 0;
                }
                break;
            }
            ++p;
        }
    }
}

static void Script_SanitizeBuffer(struct script_s *script)
{
    char *p;

    for (p = script->buffer; p < script->end_p; ++p) {
        if (*p == '`')
            *p = ' ';
    }
}

void PS_CreatePunctuationTable(script_t *script, punctuation_t *punctuations) {
    int **table;
    byte *p;
    int i;

    /* x64: typed punctuation_t access (was x86 stride 0xc, next@+8, table 256*4). */
    if (!script->punctuationtable) {
        script->punctuationtable = (punctuation_t **)(GetMemory(256 * (int)sizeof(punctuation_t *)));
    }

    {
        punctuation_t **tbl = (punctuation_t **)script->punctuationtable;
        punctuation_t *pp;
        (void)table; (void)p; (void)i;
        memset(tbl, 0, 256 * sizeof(punctuation_t *));

        for (pp = punctuations; pp->p; pp++) {
            int firstChar = (signed char)pp->p[0];
            punctuation_t *existing = tbl[firstChar];
            if (!existing) {
                pp->next = 0;
                tbl[firstChar] = pp;
            } else {
                int newLen = (int)strlen(pp->p);
                punctuation_t *prev = NULL;
                punctuation_t *cur = existing;
                while (cur) {
                    if ((int)strlen(cur->p) < newLen)
                        break;
                    prev = cur;
                    cur = (punctuation_t *)cur->next;
                }
                pp->next = (intptr_t)cur;
                if (prev)
                    prev->next = (intptr_t)pp;
                else
                    tbl[firstChar] = pp;
            }
        }
    }
}

void ScriptError(script_t *script, char *str, ...)
{
    char text[1024];
    va_list ap;

    if (script->flags & 1)
        return;

    va_start(ap, str);
    vsnprintf(text, sizeof(text), str, ap);
    va_end(ap);

    Com_Printf("^1Error: file %s, line %d: %s\n", script->filename, script->line, text);
}

void ScriptWarning(script_t *script, char *str, ...)
{
    char text[1024];
    va_list ap;

    if (script->flags & 2)
        return;

    va_start(ap, str);
    vsnprintf(text, sizeof(text), str, ap);
    va_end(ap);

    Com_Printf("^3Warning: file %s, line %d: %s\n", script->filename, script->line, text);
}

int PS_ReadEscapeCharacter(script_t *script, char *ch)
{
    char *p = script->script_p;
    int c;
    int value;

    script->script_p = p + 1;
    c = (unsigned char)p[1];

    switch (c) {
    case '"': *ch = '"'; script->script_p = p + 2; return 1;
    case '\'': *ch = '\''; script->script_p = p + 2; return 1;
    case '?': *ch = '?'; script->script_p = p + 2; return 1;
    case '\\': *ch = '\\'; script->script_p = p + 2; return 1;
    case 'a': *ch = '\a'; script->script_p = p + 2; return 1;
    case 'b': *ch = '\b'; script->script_p = p + 2; return 1;
    case 'f': *ch = '\f'; script->script_p = p + 2; return 1;
    case 'n': *ch = '\n'; script->script_p = p + 2; return 1;
    case 'r': *ch = '\r'; script->script_p = p + 2; return 1;
    case 't': *ch = '\t'; script->script_p = p + 2; return 1;
    case 'v': *ch = '\v'; script->script_p = p + 2; return 1;
    case 'x':
        value = 0;
        p += 2;
        while (1) {
            int digit;

            c = (unsigned char)*p;
            if (c >= '0' && c <= '9') {
                digit = c - '0';
            } else if (c >= 'A' && c <= 'Z') {
                digit = c - 'A' + 10;
            } else if (c >= 'a' && c <= 'z') {
                digit = c - 'a' + 10;
            } else {
                break;
            }
            value = (value << 4) + digit;
            script->script_p = ++p;
        }
        if (value > 255) {
            ScriptWarning(script, "too large value in escape character");
            value = -1;
        }
        *ch = (char)value;
        return 1;
    default:
        if (c < '0' || c > '9') {
            ScriptError(script, "unknown escape char");
            *ch = 0;
            return 1;
        }

        value = 0;
        p = script->script_p;
        while (*p >= '0' && *p <= '9') {
            value = value * 10 + (*p - '0');
            script->script_p = ++p;
        }
        if (p > script->script_p) {
            script->script_p = p;
        }
        if (value > 255) {
            ScriptWarning(script, "too large value in escape character");
            value = -1;
        }
        *ch = (char)value;
        return 1;
    }
}

void NumberValue(char *string, int subtype, long unsigned int *intvalue, long double *floatvalue)
{
    *intvalue = 0;
    *floatvalue = 0.0L;

    if (subtype & 0x800) {
        int dotfound = 0;

        while (*string) {
            if (*string == '.') {
                if (dotfound) {
                    *intvalue = (long unsigned int)*floatvalue;
                    return;
                }
                dotfound = 10;
                ++string;
                continue;
            }

            if (dotfound) {
                *floatvalue += (long double)(*string - '0') / dotfound;
                dotfound *= 10;
            } else {
                *floatvalue = *floatvalue * 10.0L + (*string - '0');
            }
            ++string;
        }
        *intvalue = (long unsigned int)*floatvalue;
        return;
    }

    if (subtype & 0x008) {
        while (*string) {
            *intvalue = *intvalue * 10 + (*string - '0');
            ++string;
        }
    } else if (subtype & 0x100) {
        string += 2;
        while (*string) {
            *intvalue <<= 4;
            if (*string >= 'a' && *string <= 'f') {
                *intvalue += *string - 'a' + 10;
            } else if (*string >= 'A' && *string <= 'F') {
                *intvalue += *string - 'A' + 10;
            } else {
                *intvalue += *string - '0';
            }
            ++string;
        }
    } else if (subtype & 0x200) {
        ++string;
        while (*string) {
            *intvalue = *intvalue * 8 + (*string - '0');
            ++string;
        }
    } else if (subtype & 0x400) {
        string += 2;
        while (*string) {
            *intvalue = *intvalue * 2 + (*string - '0');
            ++string;
        }
    } else {
        return;
    }

    *floatvalue = (long double)*intvalue;
}

int PS_ReadNumber(script_t *script, token_t *token)
{
    char *p;
    int len;
    int subtype;
    int dot;
    int octal;

    token->type = 3;
    token->subtype = 0;
    p = script->script_p;

    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        token->string[0] = '0';
        token->string[1] = p[1];
        script->script_p = p + 2;
        len = 2;

        while (PS_IsHexTokenChar((unsigned char)*script->script_p)) {
            token->string[len++] = *script->script_p++;
            if (len == 1024) {
                ScriptError(script, "hexadecimal number longer than MAX_TOKEN = %d", 1024);
                return 0;
            }
        }

        token->subtype |= 0x100;
    } else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) {
        token->string[0] = '0';
        token->string[1] = p[1];
        script->script_p = p + 2;
        len = 2;

        if (*script->script_p == '0' || *script->script_p == '1') {
            token->string[len++] = *script->script_p++;
            while (*script->script_p == '0' || *script->script_p == '1') {
                token->string[len++] = *script->script_p++;
                if (len == 1024) {
                    ScriptError(script, "binary number longer than MAX_TOKEN = %d", 1024);
                    return 0;
                }
            }
        }

        token->subtype |= 0x400;
    } else {
        len = 0;
        dot = 0;
        octal = (*p == '0');

        for (;;) {
            int c = (unsigned char)*script->script_p;

            if (c == '.') {
                dot = 1;
            } else if (c == '8' || c == '9') {
                octal = 0;
            } else if (c < '0' || c > '9') {
                break;
            }

            token->string[len++] = (char)c;
            ++script->script_p;
            if (len == 1023) {
                ScriptError(script, "number longer than MAX_TOKEN = %d", 1024);
                return 0;
            }
        }

        if (octal) {
            token->subtype |= 0x200;
        } else {
            token->subtype |= 0x800;
        }
        if (dot) {
            token->subtype |= 0x800;
        }
    }

    subtype = token->subtype;
    for (int i = 0; i < 2; ++i) {
        int c = (unsigned char)*script->script_p;

        if ((c == 'l' || c == 'L') && !(subtype & 0x2000)) {
            ++script->script_p;
            subtype = token->subtype | 0x2000;
            token->subtype = subtype;
        }

        c = (unsigned char)*script->script_p;
        if ((c == 'u' || c == 'U') && !(subtype & 0x4800)) {
            ++script->script_p;
            subtype = token->subtype | 0x4000;
            token->subtype = subtype;
        }
    }

    token->string[len] = '\0';
    NumberValue(token->string, token->subtype, &token->intvalue, &token->floatvalue);
    if (!(token->subtype & 0x800)) {
        token->subtype |= 0x1000;
    }

    return 1;
}

void StripDoubleQuotes(char *string)
{
    size_t len;

    if (string == NULL) {
        return;
    }

    if (string[0] == '"') {
        len = strlen(string + 1);
        memmove(string, string + 1, len + 1);
    }

    len = strlen(string);
    if (len > 0 && string[len - 1] == '"') {
        string[len - 1] = '\0';
    }
}

int EndOfScript(script_t *script)
{
    /* was x86 offsets 0x44/0x48 (script_p/end_p) */
    return script->script_p >= script->end_p;
}

void FreeScript(script_t *script)
{
    /* was x86 offset 0x70 (punctuationtable) */
    if (script->punctuationtable)
        FreeMemory(script->punctuationtable);
    FreeMemory(script);
}

script_t * LoadScriptFile(const char *filename)
{
    char pathname[64];
    fileHandle_t fp;
    int length;
    struct script_s *script;

    Com_sprintf(pathname, sizeof(pathname), "%s", filename);
    length = FS_FOpenFileRead(pathname, &fp, 1);
    if (!fp)
        return (script_t *)0;

    script = (script_s *)(GetClearedMemory(length + (int)sizeof(*script) + 1));   /* 0x4d1 was x86 sizeof(script_s)+1 */
    if (!script) {
        FS_FCloseFile(fp);
        return (script_t *)0;
    }

    strcpy(script->filename, filename);
    script->buffer = (char *)script + sizeof(*script);   /* 0x4d0 was x86 sizeof(script_s) */
    script->buffer[length] = '\0';
    script->length = length;
    script->script_p = script->buffer;
    script->lastscript_p = script->buffer;
    script->end_p = script->buffer + length;
    script->tokenavailable = 0;
    script->line = 1;
    script->lastline = 1;

    PS_CreatePunctuationTable((script_t *)script, default_punctuations);
    script->punctuations = default_punctuations;

    FS_Read(script->buffer, length, fp);
    FS_FCloseFile(fp);

#if defined(_WIN32) && defined(GFX_REAL_D3D9)
    if (!strcmp(filename, "ui/options_graphics.menu"))
        Script_PatchGraphicsMenu(script->buffer);
#endif
    script->length = Com_Compress(script->buffer);
    script->end_p = script->buffer + script->length;
    Script_SanitizeBuffer(script);
    return (script_t *)script;
}

int PS_ReadString(script_t *script, token_t *token, int quote)
{
    int len;

    token->type = (quote == '"') ? 1 : 2;
    token->string[0] = *script->script_p++;
    len = 1;

    while (len <= 1021) {
        char *p = script->script_p;
        int c = (unsigned char)*p;

        if (c == '\\' && !(script->flags & 8)) {
            if (!PS_ReadEscapeCharacter(script, &token->string[len])) {
                token->string[len] = '\0';
                return 0;
            }
            ++len;
            continue;
        }

        if (c == quote) {
            char *after_quote = p + 1;
            int saved_line;

            script->script_p = after_quote;
            if (!(script->flags & 4)) {
                saved_line = script->line;
                if (!PS_SkipWhitespaceAndComments(script)) {
                    script->script_p = after_quote;
                    script->line = saved_line;
                } else if (*script->script_p == quote) {
                    ++script->script_p;
                    continue;
                } else {
                    script->script_p = after_quote;
                    script->line = saved_line;
                }
            }

            token->string[len++] = (char)quote;
            token->string[len] = '\0';
            token->subtype = len;
            return 1;
        }

        if (!c) {
            token->string[len] = '\0';
            ScriptError(script, "missing trailing quote");
            return 0;
        }

        if (c == '\n') {
            token->string[len] = '\0';
            ScriptError(script, "newline inside string %s", token->string);
            return 0;
        }

        token->string[len++] = (char)c;
        script->script_p = p + 1;
    }

    ScriptError(script, "string longer than MAX_TOKEN = %d", 1024);
    return 0;
}

int PS_ReadToken(script_t *script, token_t *token)
{
    int len;
    int c;
    punctuation_t *punc;

    if (script->tokenavailable) {
        script->tokenavailable = 0;
        memcpy(token, &script->token, sizeof(*token));
        return 1;
    }

    script->lastscript_p = script->script_p;
    script->lastline = script->line;
    memset(token, 0, sizeof(*token));
    script->whitespace_p = script->script_p;
    token->whitespace_p = script->script_p;

    if (!PS_SkipWhitespaceAndComments(script)) {
        return 0;
    }

    script->endwhitespace_p = script->script_p;
    token->endwhitespace_p = script->script_p;
    token->line = script->line;
    token->linescrossed = script->line - script->lastline;

    c = (unsigned char)*script->script_p;
    if (c == '"') {
        if (!PS_ReadString(script, token, '"')) {
            return 0;
        }
        memcpy(&script->token, token, sizeof(*token));
        return 1;
    }

    if (c == '\'') {
        if (!PS_ReadString(script, token, '\'')) {
            return 0;
        }
        memcpy(&script->token, token, sizeof(*token));
        return 1;
    }

    if ((c >= '0' && c <= '9') || (c == '.' && script->script_p[1] >= '0' && script->script_p[1] <= '9')) {
        if (!PS_ReadNumber(script, token)) {
            return 0;
        }
        memcpy(&script->token, token, sizeof(*token));
        return 1;
    }

    if (script->flags & 0x10) {
        len = 0;
        while ((c = (unsigned char)*script->script_p) > ' ' && c != ';') {
            token->string[len++] = (char)c;
            ++script->script_p;
            if (len == 1024) {
                ScriptError(script, "primitive token longer than MAX_TOKEN = %d", 1024);
                return 0;
            }
        }
        token->string[len] = '\0';
        memcpy(&script->token, token, sizeof(*token));
        return 1;
    }

    if (PS_IsNameStart(c)) {
        token->type = 4;
        len = 0;
        do {
            token->string[len++] = *script->script_p++;
            if (len == 1024) {
                ScriptError(script, "name longer than MAX_TOKEN = %d", 1024);
                return 0;
            }
            c = (unsigned char)*script->script_p;
        } while (PS_IsNameChar(c));

        token->string[len] = '\0';
        token->subtype = len;
        memcpy(&script->token, token, sizeof(*token));
        return 1;
    }

    punc = script->punctuationtable[(signed char)c];
    while (punc) {
        int punc_len = strlen(punc->p);

        if (script->script_p + punc_len <= script->end_p && !strncmp(script->script_p, punc->p, punc_len)) {
            strncpy(token->string, punc->p, sizeof(token->string));
            script->script_p += punc_len;
            token->type = 5;
            token->subtype = punc->n;
            memcpy(&script->token, token, sizeof(*token));
            return 1;
        }
        punc = *(punctuation_t **)&punc->next;
    }

    ScriptError(script, "unknown punctuation");
    return 0;
}

punctuation_t default_punctuations[54] = {
    { (char *)&str_002228d8, 0x1, 0x0 },
    { (char *)&str_002228dc, 0x2, 0x0 },
    { (char *)&str_002228e0, 0x3, 0x0 },
    { (char *)&str_00222814, 0x4, 0x0 },
    { (char *)&str_00216c3c, 0x5, 0x0 },
    { (char *)&str_0021e864, 0x6, 0x0 },
    { (char *)&str_0021e86c, 0x7, 0x0 },
    { (char *)&str_0021e868, 0x8, 0x0 },
    { (char *)&str_0021e870, 0x9, 0x0 },
    { (char *)&str_0021e874, 0xa, 0x0 },
    { (char *)&str_0021e84c, 0xb, 0x0 },
    { (char *)&str_0021e850, 0xc, 0x0 },
    { (char *)&str_002228e4, 0xd, 0x0 },
    { (char *)&str_0021e844, 0xe, 0x0 },
    { (char *)&str_0021e848, 0xf, 0x0 },
    { (char *)&str_0021e85c, 0x10, 0x0 },
    { (char *)&str_0021e860, 0x11, 0x0 },
    { (char *)&str_0021e854, 0x12, 0x0 },
    { (char *)&str_0021e858, 0x13, 0x0 },
    { (char *)&str_002228e8, 0x14, 0x0 },
    { (char *)&str_002228ec, 0x15, 0x0 },
    { (char *)&str_002228f0, 0x16, 0x0 },
    { (char *)&str_002228f4, 0x17, 0x0 },
    { (char *)&str_00215bf8, 0x18, 0x0 },
    { (char *)&str_002228f8, 0x19, 0x0 },
    { (char *)&str_00215b8c, 0x1a, 0x0 },
    { (char *)&str_00216f38, 0x1b, 0x0 },
    { (char *)&str_002228fc, 0x1c, 0x0 },
    { (char *)&str_0021e990, 0x1d, 0x0 },
    { (char *)&str_00222900, 0x1e, 0x0 },
    { (char *)&str_00222904, 0x1f, 0x0 },
    { (char *)&str_00222908, 0x20, 0x0 },
    { (char *)&str_0022290c, 0x21, 0x0 },
    { (char *)&str_00222910, 0x22, 0x0 },
    { (char *)&str_00222914, 0x23, 0x0 },
    { (char *)&str_00222918, 0x24, 0x0 },
    { (char *)&str_0022291c, 0x25, 0x0 },
    { (char *)&str_00222920, 0x26, 0x0 },
    { (char *)&str_00217ff8, 0x27, 0x0 },
    { (char *)&str_0021f88c, 0x28, 0x0 },
    { (char *)&str_00222924, 0x29, 0x0 },
    { (char *)&str_00222928, 0x2a, 0x0 },
    { (char *)&str_0022292c, 0x2b, 0x0 },
    { (char *)&str_002221e8, 0x2c, 0x0 },
    { (char *)&str_00222224, 0x2d, 0x0 },
    { (char *)&str_0021e50c, 0x2e, 0x0 },
    { (char *)&str_0021e508, 0x2f, 0x0 },
    { (char *)&str_0021e510, 0x30, 0x0 },
    { (char *)&str_0021e504, 0x31, 0x0 },
    { (char *)&str_00222630, 0x32, 0x0 },
    { (char *)&str_00222930, 0x33, 0x0 },
    { (char *)&str_00222934, 0x34, 0x0 },
    { 0, 0x0, 0x0 },
    { 0 }
};

const char str_00215b8c[] = "*";
const char str_00215bf8[] = "::";
const char str_00216c3c[] = "&&";
const char str_00216f38[] = "/";
const char str_00217ff8[] = ".";
const char str_0021e504[] = "]";
const char str_0021e508[] = "}";
const char str_0021e50c[] = "{";
const char str_0021e510[] = "[";
const char str_0021e844[] = "+=";
const char str_0021e848[] = "-=";
const char str_0021e84c[] = "*=";
const char str_0021e850[] = "/=";
const char str_0021e854[] = "&=";
const char str_0021e858[] = "|=";
const char str_0021e85c[] = "++";
const char str_0021e860[] = "--";
const char str_0021e864[] = "||";
const char str_0021e868[] = "<=";
const char str_0021e86c[] = ">=";
const char str_0021e870[] = "==";
const char str_0021e874[] = "!=";
const char str_0021e990[] = "+";
const char str_0021f88c[] = ",";
const char str_002221e8[] = "(";
const char str_00222224[] = ")";
const char str_00222630[] = "\\";
const char str_00222814[] = "##";
const char str_002228d8[] = ">>=";
const char str_002228dc[] = "<<=";
const char str_002228e0[] = "...";
const char str_002228e4[] = "%=";
const char str_002228e8[] = "^=";
const char str_002228ec[] = ">>";
const char str_002228f0[] = "<<";
const char str_002228f4[] = "->";
const char str_002228f8[] = ".*";
const char str_002228fc[] = "%";
const char str_00222900[] = "-";
const char str_00222904[] = "=";
const char str_00222908[] = "&";
const char str_0022290c[] = "|";
const char str_00222910[] = "^";
const char str_00222914[] = "~";
const char str_00222918[] = "!";
const char str_0022291c[] = ">";
const char str_00222920[] = "<";
const char str_00222924[] = ";";
const char str_00222928[] = ":";
const char str_0022292c[] = "?";
const char str_00222930[] = "#";
const char str_00222934[] = "$";
