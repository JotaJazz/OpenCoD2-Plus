#include "common_types.h"
#include "imports.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

extern directive_t directives[20];
extern directive_t dollardirectives[20];

extern void Com_Printf(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void FreeMemory(void *ptr);
extern void *GetMemory(unsigned long size);
extern void *GetClearedMemory(unsigned long size);
extern void FreeScript(script_t *script);
extern script_t *LoadScriptFile(const char *filename);
extern int PS_ReadToken(script_t *script, token_t *token);
extern int EndOfScript(script_t *script);
#include <stdio.h>
extern char *strncat(char *dest, const char *src, size_t count);
#include <time.h>
extern void free(void *ptr);
extern int I_stricmp(const char *s1, const char *s2);

extern int numtokens;
extern define_t *globaldefines;
extern source_t * sourceFiles[64];

void SourceError(source_t *source, char *str, ...);
void SourceWarning(source_t *source, char *str, ...);
int PC_StringizeTokens(token_t *tokens, token_t *token);
int PC_OperatorPriority(int op);
int PC_Directive_line(source_t *source);
int PC_SourceFileAndLine(int handle, char *filename, int *line);
int PC_ReadSourceToken(source_t *source, token_t *token);
int PC_Directive_error(source_t *source);
int PC_Directive_else(source_t *source);
int PC_Directive_endif(source_t *source);
int PC_EvaluateTokens(source_t *source, token_t *tokens, long int *intvalue, double *floatvalue, int integer);
void FreeSource(source_t *source);
int PC_FreeSourceHandle(int handle);
int PC_ReadDollarDirective(source_t *source);
int PC_ReadDirective(source_t *source);
int PC_Directive_include(source_t *source);
int PC_ReadDefineParms(source_t *source, define_t *define, token_t * *parms, int maxparms);
int PC_ExpandBuiltinDefine(source_t *source, token_t *deftoken, define_t *define, token_t * *firsttoken, token_t * *lasttoken);
int PC_ExpandDefine(source_t *source, token_t *deftoken, define_t *define, token_t * *firsttoken, token_t * *lasttoken);
int PC_ReadLine(source_t *source, token_t *token);
int PC_ReadLineHandle(int handle, pc_token_t *pc_token);
int PC_Directive_pragma(source_t *source);
int PC_Directive_if_def(source_t *source, int type);
int PC_Directive_ifndef(source_t *source);
int PC_Directive_ifdef(source_t *source);
int PC_Directive_undef(source_t *source);
int PC_ReadToken(source_t *source, token_t *token);
int PC_ReadTokenHandle(int handle, pc_token_t *pc_token);
int PC_Directive_define(source_t *source);
define_t * PC_CopyDefine(source_t *source, define_t *define);
source_t * LoadSourceFile(const char *filename);
int PC_LoadSourceHandle(const char *filename);
int PC_Evaluate(source_t *source, long int *intvalue, double *floatvalue, int integer);
int PC_Directive_evalfloat(source_t *source);
int PC_Directive_eval(source_t *source);
int PC_Directive_if(source_t *source);
int PC_Directive_elif(source_t *source);
int PC_DollarEvaluate(source_t *source, long int *intvalue, double *floatvalue, int integer);
int PC_DollarDirective_evalfloat(source_t *source);
int PC_DollarDirective_evalint(source_t *source);
void StripDoubleQuotes(char *string);

static token_t *PC_AllocTokenCopy(const token_t *token)
{
    token_t *copy = (token_t *)GetMemory(sizeof(*copy));

    if (!copy) {
        Com_Error(0, "EXE_ERR_OUT_OF_MEMORY");
    }

    memcpy(copy, token, sizeof(*copy));
    copy->next = NULL;
    ++numtokens;
    return copy;
}

static token_t *PC_CopyTokenList(const token_t *tokens)
{
    token_t *first = NULL;
    token_t *last = NULL;

    while (tokens) {
        token_t *copy = PC_AllocTokenCopy(tokens);

        if (last) {
            last->next = copy;
        } else {
            first = copy;
        }

        last = copy;
        tokens = tokens->next;
    }

    return first;
}

static void PC_UnreadSourceToken(source_t *source, const token_t *token)
{
    token_t *copy = PC_AllocTokenCopy(token);

    copy->next = source->tokens;
    source->tokens = copy;
}

static void PC_InitSyntheticToken(source_t *source, token_t *token)
{
    script_t *script = source->scriptstack;

    memset(token, 0, sizeof(*token));
    if (script) {
        token->line = script->line;
        token->whitespace_p = script->script_p;
        token->endwhitespace_p = script->script_p;
    }
}

static void PC_UnreadEvalMinusToken(source_t *source)
{
    token_t token;

    PC_InitSyntheticToken(source, &token);
    token.string[0] = '-';
    token.string[1] = '\0';
    token.type = 5;
    token.subtype = 0x1e;
    PC_UnreadSourceToken(source, &token);
}

static void PC_UnreadEvalIntToken(source_t *source, int value, qboolean setNumeric)
{
    token_t token;
    int magnitude = value;

    if (value < 0) {
        magnitude = (int)(0u - (unsigned int)value);
    }

    PC_InitSyntheticToken(source, &token);
    sprintf(token.string, "%d", magnitude);
    token.type = 3;
    token.subtype = 0x3008;
    if (setNumeric) {
        token.intvalue = (long unsigned int)value;
        token.floatvalue = (long double)value;
    }
    PC_UnreadSourceToken(source, &token);

    if (value < 0) {
        PC_UnreadEvalMinusToken(source);
    }
}

static void PC_UnreadEvalFloatToken(source_t *source, double value, qboolean setNumeric)
{
    token_t token;
    double magnitude = value;

    if (magnitude < 0.0) {
        magnitude = -magnitude;
    }

    PC_InitSyntheticToken(source, &token);
    sprintf(token.string, "%1.2f", (float)magnitude);
    token.type = 3;
    token.subtype = 0x2808;
    if (setNumeric) {
        token.intvalue = (long unsigned int)(long int)value;
        token.floatvalue = (long double)value;
    }
    PC_UnreadSourceToken(source, &token);

    if (value < 0.0) {
        PC_UnreadEvalMinusToken(source);
    }
}

static unsigned int PC_DefineHash(const char *name)
{
    int hash = 0;
    int i = 119;

    while (*name) {
        hash += (signed char)*name++ * i++;
    }

    return (unsigned int)(hash ^ (hash >> 10) ^ (hash >> 20)) & 0x3ff;
}

static define_t *PC_FindDefineInSource(source_t *source, const char *name)
{
    define_t *define;

    if (!source->definehash) {
        return NULL;
    }

    for (define = source->definehash[PC_DefineHash(name)]; define; define = (define_t *)(uintptr_t)define->hashnext) {
        if (!strcmp(define->name, name)) {
            return define;
        }
    }

    return NULL;
}

static void PC_FreeTokenList(token_t *token)
{
    while (token) {
        token_t *next = token->next;
        FreeMemory(token);
        --numtokens;
        token = next;
    }
}

static void PC_AppendTokenCopy(token_t **firsttoken, token_t **lasttoken, const token_t *token)
{
    token_t *copy = PC_AllocTokenCopy(token);

    copy->next = NULL;
    if (*lasttoken) {
        (*lasttoken)->next = copy;
    } else {
        *firsttoken = copy;
    }
    *lasttoken = copy;
}

static void PC_AppendTokenNode(token_t **firsttoken, token_t **lasttoken, token_t *token)
{
    token->next = NULL;
    if (*lasttoken) {
        (*lasttoken)->next = token;
    } else {
        *firsttoken = token;
    }
    *lasttoken = token;
}

static int PC_FindDefineParmIndex(define_t *define, const char *name)
{
    token_t *parm;
    int index = 0;

    for (parm = define->parms; parm; parm = parm->next, ++index) {
        if (!strcmp(parm->string, name)) {
            return index;
        }
    }

    return -1;
}

static void PC_AppendTokenListCopy(token_t **firsttoken, token_t **lasttoken, token_t *tokens)
{
    token_t *token;

    for (token = tokens; token; token = token->next) {
        PC_AppendTokenCopy(firsttoken, lasttoken, token);
    }
}

static int PC_IsTokenPasteOperator(const token_t *token)
{
    return token->string[0] == '#' && token->string[1] == '#';
}

static int PC_MergeTokenPaste(source_t *source, token_t **firsttoken, token_t **lasttoken)
{
    token_t *left;

    for (left = *firsttoken; left && left->next; ) {
        token_t *paste = left->next;
        token_t *right;

        if (!PC_IsTokenPasteOperator(paste)) {
            left = paste;
            continue;
        }

        right = paste->next;
        if (!right) {
            left = paste;
            continue;
        }

        if (left->type == 4) {
            if (right->type != 3 && right->type != 4) {
                SourceError(source, "can't merge %s with %s", left->string, right->string);
                return 0;
            }
            strcat(left->string, right->string);
        } else if (left->type == 1) {
            size_t len;

            if (right->type != 1) {
                SourceError(source, "can't merge %s with %s", left->string, right->string);
                return 0;
            }

            len = strlen(left->string);
            if (len >= 1) {
                left->string[len - 1] = '\0';
            }
            strcat(left->string, right->string + 1);
        } else {
            SourceError(source, "can't merge %s with %s", left->string, right->string);
            return 0;
        }

        left->next = right->next;
        if (*lasttoken == right) {
            *lasttoken = left;
        }

        FreeMemory(paste);
        --numtokens;
        FreeMemory(right);
        --numtokens;
    }

    return 1;
}

static int PC_ExpandDefineIntoSource(source_t *source, token_t *deftoken, define_t *define)
{
    token_t *firsttoken = NULL;
    token_t *lasttoken = NULL;

    if (!PC_ExpandDefine(source, deftoken, define, &firsttoken, &lasttoken)) {
        return 0;
    }

    if (!firsttoken || !lasttoken) {
        return 0;
    }

    lasttoken->next = source->tokens;
    source->tokens = firsttoken;
    return 1;
}

static int PC_EvaluateTokenForList(source_t *source, token_t *token, token_t **firsttoken, token_t **lasttoken, int *defined)
{
    define_t *define;

    if (token->type == 4) {
        if (*defined) {
            PC_AppendTokenCopy(firsttoken, lasttoken, token);
            *defined = 0;
            return 1;
        }

        if (!strcmp(token->string, "defined")) {
            PC_AppendTokenCopy(firsttoken, lasttoken, token);
            *defined = 1;
            return 1;
        }

        define = PC_FindDefineInSource(source, token->string);
        if (!define) {
            SourceError(source, "can't evaluate %s, not defined", token->string);
            return 0;
        }

        return PC_ExpandDefineIntoSource(source, token, define) ? 2 : 0;
    }

    if (token->type == 3 || token->type == 5) {
        PC_AppendTokenCopy(firsttoken, lasttoken, token);
        return 1;
    }

    SourceError(source, "can't evaluate %s", token->string);
    return 0;
}

static void PC_CollapseIncludePathSeparators(char *path)
{
    char *ptr;

    for (ptr = path; *ptr; ) {
        if ((*ptr == '\\' || *ptr == '/') && (ptr[1] == '\\' || ptr[1] == '/')) {
            strcpy(ptr, ptr + 1);
        } else {
            ++ptr;
        }
    }

    for (ptr = path; *ptr; ++ptr) {
        if (*ptr == '\\' || *ptr == '/') {
            *ptr = '/';
        }
    }
}

static int PC_PushScriptToSource(source_t *source, script_t *script)
{
    script_t *s;

    for (s = source->scriptstack; s; s = s->next) {
        if (!I_stricmp((const char *)s, (const char *)script)) {
            SourceError(source, "%s recursively included", (const char *)script);
            return 1;
        }
    }

    script->next = source->scriptstack;
    source->scriptstack = script;
    return 1;
}

void SourceError(source_t *source, char *str, ...) {
    char text[1024];
    va_list ap;
    va_start(ap, str);
    vsnprintf(text, 0x400, str, ap);
    va_end(ap);
    {
        script_t *scriptfile = source->scriptstack;
        Com_Printf((const char *)"^1Error: file %s, line %d: %s\n", scriptfile, scriptfile->line, text);
    }
}

void SourceWarning(source_t *source, char *str, ...) {
    char text[1024];
    va_list ap;
    va_start(ap, str);
    vsnprintf(text, 0x400, str, ap);
    va_end(ap);
    {
        script_t *scriptfile = source->scriptstack;
        Com_Printf((const char *)"^3Warning: file %s, line %d: %s\n", scriptfile, scriptfile->line, text);
    }
}

int PC_StringizeTokens(token_t *tokens, token_t *token) {
    token_t *t;
    int len;

    token->type = 1;
    token->whitespace_p = NULL;
    token->endwhitespace_p = NULL;
    token->string[0] = '\0';

    len = strlen(token->string);
    token->string[len] = '"';
    token->string[len + 1] = '\0';

    for (t = tokens; t; t = *(token_t **)&t->next) {
        len = strlen((char *)token);
        strncat((char *)token, (char *)t, 0x400 - len);
    }

    len = strlen((char *)token);
    strncat((char *)token, (const char *)"\"", 0x401 - len);

    return 1;
}

int PC_OperatorPriority(int op) {
    static const int priorities[] = {
     7,  6,  12, 12, 11, 11, 0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  13, 13, 0,  0,
     0,  15, 15, 15, 14, 14, 0,  10, 8,  9,
     16, 16, 12, 12, 0,  0,  0,  5,  5
    };
    int idx = op - 5;
    if (idx < 0 || idx > 38)
        return 0;
    return priorities[idx];
}

int PC_Directive_line(source_t *source)
{
    SourceError(source, "#line directive not supported");
    return 0;
}

int PC_SourceFileAndLine(int handle, char *filename, int *line) {
    source_t *source;

    if ((unsigned)(handle - 1) > 0x3e)
        return 0;
    source = sourceFiles[handle];
    if (!source)
        return 0;

    if (source->scriptstack) {
        strcpy(filename, source->scriptstack->filename);
        *line = source->scriptstack->line;
    } else {
        strcpy(filename, source->filename);
        *line = 0;
    }
    return 1;
}

int PC_ReadSourceToken(source_t *source, token_t *token) {
    token_t *tok;
    script_t *script;
    indent_t *indent;

    for (;;) {

        tok = source->tokens;
        if (tok) {
            memcpy(token, tok, sizeof(token_t));
            source->tokens = *(token_t **)&tok->next;
            FreeMemory(tok);
            numtokens--;
            return 1;
        }

        if (!source->scriptstack)
            return 0;
        if (PS_ReadToken(source->scriptstack, token))
            return 1;

        if (EndOfScript(source->scriptstack)) {

            indent = source->indentstack;
            if (indent && indent->script == source->scriptstack) {
                SourceWarning(source, "#if directive not terminated");

                while ((indent = source->indentstack) != NULL) {
                    if (indent->script != source->scriptstack)
                        break;
                    source->indentstack = *(indent_t **)&indent->next;
                    source->skip -= indent->skip;
                    FreeMemory(indent);
                }
            }
        }

        script = source->scriptstack;
        if (!script)
            return 0;
        {
            script_t *nextScript = *(script_t **)&script->next;
            if (!nextScript)
                return 0;
            source->scriptstack = nextScript;
            FreeScript(script);
        }
    }
}

int PC_Directive_error(source_t *source)
{
    token_t token;

    token.string[0] = '\0';
    PC_ReadSourceToken(source, &token);
    SourceError(source, "#error directive: %s", token.string);
    return 0;
}

int PC_Directive_else(source_t *source)
{
    indent_t *indent = source->indentstack;
    int type;
    int skip;

    if (!indent || indent->script != source->scriptstack) {
        SourceError(source, "misplaced #else");
        return 0;
    }

    type = indent->type;
    skip = indent->skip;
    source->indentstack = *(indent_t **)&indent->next;
    source->skip -= skip;
    FreeMemory(indent);

    if (!type) {
        SourceError(source, "misplaced #else");
        return 0;
    }

    if (type == 2) {
        SourceError(source, "#else after #else");
        return 0;
    }

    indent = (indent_t *)(GetMemory(sizeof(*indent)));
    indent->type = 2;
    indent->script = source->scriptstack;
    indent->skip = !skip;
    source->skip += indent->skip;
    indent->next = (intptr_t)source->indentstack;
    source->indentstack = indent;

    return 1;
}

int PC_Directive_endif(source_t *source)
{
    indent_t *indent = source->indentstack;
    int type;
    int skip;

    if (!indent || indent->script != source->scriptstack) {
        SourceError(source, "misplaced #endif");
        return 0;
    }

    type = indent->type;
    skip = indent->skip;
    source->indentstack = *(indent_t **)&indent->next;
    source->skip -= skip;
    FreeMemory(indent);

    if (!type) {
        SourceError(source, "misplaced #endif");
        return 0;
    }

    return 1;
}

static int PC_EvalIsUnaryToken(int op)
{
    return op == 9 || op == 16 || op == 25;
}

static int PC_EvalOperatorArity(int op)
{
    return (op < 0 || op == 16 || op == 25) ? 1 : 2;
}

static int PC_EvalOperatorValid(int op)
{
    return (op >= 5 && op <= 25) || op == -9;
}

static int PC_EvalOperatorPriority(int op)
{
    if (op < 0 || op == 16 || op == 25) {
        return 100;
    }
    return PC_OperatorPriority(op);
}

static int PC_EvalApplyOperator(source_t *source, int op, int integer,
                                long *ival, double *fval, int *valueCount)
{
    int arity = PC_EvalOperatorArity(op);
    long ia, ib;
    double fa, fb;

    if (*valueCount < arity) {
        SourceError(source, "misplaced operator in #if/#elif");
        return 0;
    }

    if (!integer) {
        switch (op < 0 ? -op : op) {
            case 7: case 19: case 20: case 21: case 22: case 23: case 25:
                SourceError(source, "illigal operator %s on floating point operands\n", "");
                return 0;
            default:
                break;
        }
    }

    if (arity == 1) {
        ia = ival[*valueCount - 1];
        fa = fval[*valueCount - 1];
        switch (op) {
            case -9:
                ival[*valueCount - 1] = -ia;
                fval[*valueCount - 1] = -fa;
                return 1;
            case 16:
                ival[*valueCount - 1] = !ia;
                fval[*valueCount - 1] = (fa == 0.0) ? 1.0 : 0.0;
                return 1;
            case 25:
                ival[*valueCount - 1] = ~ia;
                fval[*valueCount - 1] = (double)~ia;
                return 1;
            default:
                SourceError(source, "invalid operator in #if/#elif");
                return 0;
        }
    }

    ib = ival[*valueCount - 1];
    fb = fval[*valueCount - 1];
    ia = ival[*valueCount - 2];
    fa = fval[*valueCount - 2];
    --*valueCount;

    switch (op) {
        case 5:
            ia *= ib;
            fa *= fb;
            break;
        case 6:
            if (!ib || fb == 0.0) {
                SourceError(source, "divide by zero in #if/#elif");
                return 0;
            }
            ia /= ib;
            fa /= fb;
            break;
        case 7:
            if (!ib) {
                SourceError(source, "divide by zero in #if/#elif");
                return 0;
            }
            ia %= ib;
            fa = (double)ia;
            break;
        case 8:
            ia += ib;
            fa += fb;
            break;
        case 9:
            ia -= ib;
            fa -= fb;
            break;
        case 10:
            ia = ia && ib;
            fa = (fa != 0.0 && fb != 0.0) ? 1.0 : 0.0;
            break;
        case 11:
            ia = ia || ib;
            fa = (fa != 0.0 || fb != 0.0) ? 1.0 : 0.0;
            break;
        case 12:
            ia = ia >= ib;
            fa = (fa >= fb) ? 1.0 : 0.0;
            break;
        case 13:
            ia = ia <= ib;
            fa = (fa <= fb) ? 1.0 : 0.0;
            break;
        case 14:
            ia = ia == ib;
            fa = (fa == fb) ? 1.0 : 0.0;
            break;
        case 15:
            ia = ia != ib;
            fa = (fa != fb) ? 1.0 : 0.0;
            break;
        case 17:
            ia = ia > ib;
            fa = (fa > fb) ? 1.0 : 0.0;
            break;
        case 18:
            ia = ia < ib;
            fa = (fa < fb) ? 1.0 : 0.0;
            break;
        case 19:
            ia >>= ib;
            fa = (double)ia;
            break;
        case 20:
            ia <<= ib;
            fa = (double)ia;
            break;
        case 21:
            ia &= ib;
            fa = (double)ia;
            break;
        case 22:
            ia |= ib;
            fa = (double)ia;
            break;
        case 23:
            ia ^= ib;
            fa = (double)ia;
            break;
        default:
            SourceError(source, "invalid operator in #if/#elif");
            return 0;
    }

    ival[*valueCount - 1] = ia;
    fval[*valueCount - 1] = fa;
    return 1;
}

int PC_EvaluateTokens(source_t *source, token_t *tokens, long int *intvalue, double *floatvalue, int integer)
{
    enum { MAX_EVAL_VALUES = 64, MAX_EVAL_OPERATORS = 64 };
    long values[MAX_EVAL_VALUES];
    double floatValues[MAX_EVAL_VALUES];
    int operators[MAX_EVAL_OPERATORS];
    int operatorParens[MAX_EVAL_OPERATORS];
    int valueCount = 0;
    int operatorCount = 0;
    int parentheses = 0;
    int expectValue = 1;
    token_t *t;

    if (intvalue) {
        *intvalue = 0;
    }
    if (floatvalue) {
        *floatvalue = 0.0;
    }
    if (!tokens) {
        SourceError(source, "no value in #if/#elif");
        return 0;
    }

    for (t = tokens; t; t = t->next) {
        if (t->type == 4) {
            int definedValue;
            token_t *nameToken;

            if (!expectValue) {
                SourceError(source, "syntax error in #if/#elif");
                goto fail;
            }
            if (strcmp(t->string, "defined")) {
                SourceError(source, "undefined name %s in #if/#elif", t->string);
                goto fail;
            }

            t = t->next;
            if (!t) {
                SourceError(source, "defined without name in #if/#elif");
                goto fail;
            }

            if (t->type == 5 && t->subtype == 0x2c) {
                t = t->next;
                if (!t || t->type != 4) {
                    SourceError(source, "defined without name in #if/#elif");
                    goto fail;
                }
                nameToken = t;
                t = t->next;
                if (!t || t->type != 5 || t->subtype != 0x2d) {
                    SourceError(source, "missing ) in defined in #if/#elif");
                    goto fail;
                }
            } else if (t->type == 4) {
                nameToken = t;
            } else {
                SourceError(source, "defined without name in #if/#elif");
                goto fail;
            }

            if (valueCount >= MAX_EVAL_VALUES) {
                SourceError(source, "out of values in #if/#elif");
                goto fail;
            }
            definedValue = PC_FindDefineInSource(source, nameToken->string) != NULL;
            values[valueCount] = definedValue;
            floatValues[valueCount] = (double)definedValue;
            ++valueCount;
            expectValue = 0;
            continue;
        }

        if (t->type == 3) {
            if (!expectValue) {
                SourceError(source, "syntax error in #if/#elif");
                goto fail;
            }
            if (valueCount >= MAX_EVAL_VALUES) {
                SourceError(source, "out of values in #if/#elif");
                goto fail;
            }
            values[valueCount] = (long)t->intvalue;
            floatValues[valueCount] = (double)t->floatvalue;
            ++valueCount;
            expectValue = 0;
            continue;
        }

        if (t->type != 5) {
            SourceError(source, "unknown %s in #if/#elif", t->string);
            goto fail;
        }

        if (t->subtype == 0x2c) {
            if (!expectValue) {
                SourceError(source, "misplaced parenthesis in #if/#elif");
                goto fail;
            }
            ++parentheses;
            continue;
        }

        if (t->subtype == 0x2d) {
            if (expectValue) {
                SourceError(source, "misplaced parenthesis in #if/#elif");
                goto fail;
            }
            --parentheses;
            if (parentheses < 0) {
                SourceError(source, "too many ) in #if/#elif");
                goto fail;
            }
            while (operatorCount > 0 && operatorParens[operatorCount - 1] > parentheses) {
                --operatorCount;
                if (!PC_EvalApplyOperator(source, operators[operatorCount], integer,
                                          values, floatValues, &valueCount)) {
                    goto fail;
                }
            }
            continue;
        }

        if (!PC_EvalOperatorValid(t->subtype)) {
            SourceError(source, "invalid operator %s in #if/#elif", t->string);
            goto fail;
        }

        if (expectValue) {
            if (t->subtype == 8) {
                continue;
            }
            if (!PC_EvalIsUnaryToken(t->subtype)) {
                SourceError(source, "operator %s after operator in #if/#elif", t->string);
                goto fail;
            }
        } else if (t->subtype == 16 || t->subtype == 25) {
            SourceError(source, "operator %s after value in #if/#elif", t->string);
            goto fail;
        }

        while (operatorCount > 0) {
            int oldOp = operators[operatorCount - 1];
            int oldParen = operatorParens[operatorCount - 1];
            int oldPriority = PC_EvalOperatorPriority(oldOp);
            int newPriority = PC_EvalOperatorPriority(expectValue && t->subtype == 9 ? -9 : t->subtype);

            if (oldParen < parentheses || (oldParen == parentheses && oldPriority < newPriority)) {
                break;
            }
            --operatorCount;
            if (!PC_EvalApplyOperator(source, oldOp, integer, values, floatValues, &valueCount)) {
                goto fail;
            }
        }

        if (operatorCount >= MAX_EVAL_OPERATORS) {
            SourceError(source, "out of operators in #if/#elif");
            goto fail;
        }
        operators[operatorCount] = (expectValue && t->subtype == 9) ? -9 : t->subtype;
        operatorParens[operatorCount] = parentheses;
        ++operatorCount;
        expectValue = 1;
    }

    if (expectValue) {
        SourceError(source, "trailing operator in #if/#elif");
        goto fail;
    }
    if (parentheses) {
        SourceError(source, "too many ( in #if/#elif");
        goto fail;
    }

    while (operatorCount > 0) {
        --operatorCount;
        if (!PC_EvalApplyOperator(source, operators[operatorCount], integer, values, floatValues, &valueCount)) {
            goto fail;
        }
    }

    if (valueCount != 1) {
        SourceError(source, "no value in #if/#elif");
        goto fail;
    }

    if (intvalue) {
        *intvalue = values[0];
    }
    if (floatvalue) {
        *floatvalue = floatValues[0];
    }
    return 1;

fail:
    if (intvalue) {
        *intvalue = 0;
    }
    if (floatvalue) {
        *floatvalue = 0.0;
    }
    return 0;
}

void FreeSource(source_t *source)
{
    script_t *script;
    token_t *token;
    int i;
    indent_t *indent;

    while ((script = source->scriptstack) != NULL) {
        source->scriptstack = (script_t *)(uintptr_t)script->next;
        FreeScript(script);
    }

    while ((token = source->tokens) != NULL) {
        source->tokens = token->next;
        FreeMemory(token);
        --numtokens;
    }

    for (i = 0; i < 1024; ++i) {
        define_t **bucket = &source->definehash[i];
        define_t *define;

        while ((define = *bucket) != NULL) {
            token_t *list;
            *bucket = (define_t *)(uintptr_t)define->hashnext;

            list = define->parms;
            while (list) {
                token_t *next = list->next;
                FreeMemory(list);
                --numtokens;
                list = next;
            }

            list = define->tokens;
            while (list) {
                token_t *next = list->next;
                FreeMemory(list);
                --numtokens;
                list = next;
            }

            FreeMemory(define);
        }
    }

    while ((indent = source->indentstack) != NULL) {
        source->indentstack = (indent_t *)(uintptr_t)indent->next;
        FreeMemory(indent);
    }

    if (source->definehash) {
        FreeMemory(source->definehash);
    }

    FreeMemory(source);
}

int PC_FreeSourceHandle(int handle)
{
    if ((unsigned int)handle - 1u > 0x3e || !sourceFiles[handle])
        return 0;

    FreeSource(sourceFiles[handle]);
    sourceFiles[handle] = NULL;
    return 1;
}

int PC_ReadDollarDirective(source_t *source)
{
    token_t token;
    directive_t *directive;
    int i;

    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "found $ without name");
        return 0;
    }

    if (token.linescrossed > 0) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "$ must be on the same line");
        return 0;
    }

    if (token.type != 4) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "unknown precompiler directive %s", token.string);
        return 0;
    }

    for (i = 0, directive = dollardirectives; directive->name; ++i, ++directive) {
        if (!strcmp(directive->name, token.string)) {
            return dollardirectives[i].func(source);
        }
    }

    PC_UnreadSourceToken(source, &token);
    SourceError(source, "unknown precompiler directive %s", token.string);
    return 0;
}

int PC_ReadDirective(source_t *source)
{
    token_t token;
    directive_t *directive;
    int i;

    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "found # without name");
        return 0;
    }

    if (token.linescrossed > 0) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "# must be on the same line");
        return 0;
    }

    if (token.type != 4) {
        SourceError(source, "unknown precompiler directive %s", token.string);
        return 0;
    }

    for (i = 0, directive = directives; directive->name; ++i, ++directive) {
        if (!strcmp(directive->name, token.string)) {
            return directives[i].func(source);
        }
    }

    SourceError(source, "unknown precompiler directive %s", token.string);
    return 0;
}

int PC_Directive_include(source_t *source)
{
    token_t token;
    char path[64];
    script_t *script;

    if (source->skip > 0) {
        return 1;
    }

    if (!PC_ReadSourceToken(source, &token) || token.linescrossed > 0) {
        SourceError(source, "#include without file name");
        return 0;
    }

    if (token.type == 1) {
        StripDoubleQuotes(token.string);
        PC_CollapseIncludePathSeparators(token.string);

        script = LoadScriptFile(token.string);
        if (!script) {
            strcpy(path, source->includepath);
            strcat(path, token.string);
            script = LoadScriptFile(path);
        }

        if (!script) {
            SourceError(source, "file %s not found", path);
            return 0;
        }

        return PC_PushScriptToSource(source, script);
    }

    if (token.type != 5 || token.string[0] != '<') {
        SourceError(source, "#include without file name");
        return 0;
    }

    strcpy(path, source->includepath);

    for (;;) {
        if (!PC_ReadSourceToken(source, &token)) {
            break;
        }

        if (token.linescrossed > 0) {
            PC_UnreadSourceToken(source, &token);
            break;
        }

        if (token.type == 5 && token.string[0] == '>') {
            break;
        }

        strncat(path, token.string, 64);
    }

    if (token.string[0] != '>') {
        SourceWarning(source, "#include missing trailing >");
    }

    if (!path[0]) {
        SourceError(source, "#include without file name between < >");
        return 0;
    }

    PC_CollapseIncludePathSeparators(path);

    script = LoadScriptFile(path);
    if (!script) {
        SourceError(source, "file %s not found", path);
        return 0;
    }

    return PC_PushScriptToSource(source, script);
}

int PC_ReadDefineParms(source_t *source, define_t *define, token_t * *parms, int maxparms)
{
    token_t token;
    token_t *copy;
    token_t *last;
    token_t **parm;
    int indent;
    int numparms;
    int lastcomma;
    int i;

    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "define %s missing parms", define->name);
        return 0;
    }

    if (define->numparms > maxparms) {
        SourceError(source, "define with more than %d parameters", maxparms);
        return 0;
    }

    for (i = 0; i < define->numparms; ++i) {
        parms[i] = NULL;
    }

    if (strcmp(token.string, "(")) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "define %s missing parms", define->name);
        return 0;
    }

    if (maxparms <= 0) {
        SourceError(source, "define %s with too many parms", define->name);
        return 0;
    }

    if (define->numparms <= 0) {
        SourceWarning(source, "define %s has too many parms", define->name);
        return 0;
    }

    indent = 0;
    numparms = 0;

    for (;;) {
        parm = &parms[numparms];
        *parm = NULL;
        last = NULL;
        lastcomma = 1;

        for (;;) {
            if (!PC_ReadSourceToken(source, &token)) {
                SourceError(source, "define %s incomplete", define->name);
                return 0;
            }

            if (!strcmp(token.string, ",") && indent <= 0) {
                if (lastcomma) {
                    SourceWarning(source, "too many comma's");
                }

                ++numparms;
                if (numparms == maxparms) {
                    SourceError(source, "define %s with too many parms", define->name);
                    return 0;
                }

                if (define->numparms > numparms) {
                    break;
                }

                SourceWarning(source, "define %s has too many parms", define->name);
                return 0;
            }

            if (!strcmp(token.string, "(")) {
                ++indent;
                lastcomma = 0;
                continue;
            }

            if (!strcmp(token.string, ")")) {
                --indent;
                if (indent <= 0) {
                    if (!parms[define->numparms - 1]) {
                        SourceWarning(source, "too few define parms");
                    }
                    return 1;
                }
            }

            if (define->numparms > numparms) {
                copy = PC_AllocTokenCopy(&token);
                if (last) {
                    last->next = copy;
                } else {
                    *parm = copy;
                }
                last = copy;
            }

            lastcomma = 0;
        }
    }
}

int PC_ExpandBuiltinDefine(source_t *source, token_t *deftoken, define_t *define, token_t * *firsttoken, token_t * *lasttoken)
{
    token_t *token = PC_AllocTokenCopy(deftoken);

    switch (define->builtin) {
    case 1:
        sprintf(token->string, "%d", deftoken->line);
        token->intvalue = (long unsigned int)deftoken->line;
        token->floatvalue = (long double)deftoken->line;
        token->type = 3;
        token->subtype = 0x1008;
        *firsttoken = token;
        *lasttoken = token;
        break;

    case 2:
        strcpy(token->string, source->filename);
        token->type = 4;
        token->subtype = strlen(token->string);
        *firsttoken = token;
        *lasttoken = token;
        break;

    case 3: {
        time_t t;
        char *curtime;
        size_t len;

        t = time(NULL);
        curtime = ctime(&t);
        token->string[0] = '"';
        token->string[1] = '\0';
        strncat(token->string, curtime + 4, 7);
        strncat(token->string + 7, curtime + 20, 4);
        len = strlen(token->string);
        token->string[len] = '"';
        token->string[len + 1] = '\0';
        free(curtime);
        token->type = 4;
        token->subtype = strlen(token->string);
        *firsttoken = token;
        *lasttoken = token;
        break;
    }

    case 4: {
        time_t t;
        char *curtime;
        size_t len;

        t = time(NULL);
        curtime = ctime(&t);
        token->string[0] = '"';
        token->string[1] = '\0';
        strncat(token->string, curtime + 11, 8);
        len = strlen(token->string);
        token->string[len] = '"';
        token->string[len + 1] = '\0';
        free(curtime);
        token->type = 4;
        token->subtype = strlen(token->string);
        *firsttoken = token;
        *lasttoken = token;
        break;
    }

    default:
        *firsttoken = NULL;
        *lasttoken = NULL;
        break;
    }

    return 1;
}

int PC_ExpandDefine(source_t *source, token_t *deftoken, define_t *define, token_t * *firsttoken, token_t * *lasttoken)
{
    token_t *parms[128];
    token_t *first;
    token_t *last;
    token_t *token;
    int i;

    if (define->builtin) {
        return PC_ExpandBuiltinDefine(source, deftoken, define, firsttoken, lasttoken);
    }

    if (define->numparms > 0) {
        if (!PC_ReadDefineParms(source, define, parms, 128)) {
            return 0;
        }
    }

    first = NULL;
    last = NULL;

    for (token = define->tokens; token; token = token->next) {
        int parmnum = -1;

        if (token->type == 4) {
            parmnum = PC_FindDefineParmIndex(define, token->string);
        }

        if (parmnum >= 0) {
            if (parms[parmnum]) {
                PC_AppendTokenListCopy(&first, &last, parms[parmnum]);
            }
            continue;
        }

        if (token->string[0] == '#') {
            token_t *next = token->next;

            if (next) {
                parmnum = PC_FindDefineParmIndex(define, next->string);
                if (parmnum >= 0) {
                    token_t stringized;
                    token_t *copy;

                    if (!PC_StringizeTokens(parms[parmnum], &stringized)) {
                        SourceError(source, "can't stringize tokens");
                        return 0;
                    }

                    copy = PC_AllocTokenCopy(&stringized);
                    PC_AppendTokenNode(&first, &last, copy);
                    token = next;
                    continue;
                }
            }

            SourceWarning(source, "stringizing operator without define parameter");
        }

        PC_AppendTokenCopy(&first, &last, token);
    }

    if (!PC_MergeTokenPaste(source, &first, &last)) {
        return 0;
    }

    *firsttoken = first;
    *lasttoken = last;

    if (define->numparms > 0) {
        for (i = 0; i < define->numparms; ++i) {
            PC_FreeTokenList(parms[i]);
        }
    }

    return 1;
}

int PC_ReadLine(source_t *source, token_t *token)
{
    int allowCrossLine;
    token_t *firsttoken;
    token_t *lasttoken;
    define_t *define;

    allowCrossLine = 0;
    while (1) {
        if (!PC_ReadSourceToken(source, token)) {
            return 0;
        }

        if (allowCrossLine < token->linescrossed) {
            PC_UnreadSourceToken(source, token);
            return 0;
        }

        if (token->type == 4) {
            for (define = source->definehash[PC_DefineHash(token->string)]; define; define = (define_t *)(uintptr_t)define->hashnext) {
                if (!strcmp(define->name, token->string)) {
                    break;
                }
            }

            if (define) {
                if (!PC_ExpandDefine(source, token, define, &firsttoken, &lasttoken)) {
                    return 0;
                }
                if (!firsttoken || !lasttoken) {
                    return 0;
                }

                lasttoken->next = source->tokens;
                source->tokens = firsttoken;

                if (!PC_ReadSourceToken(source, token)) {
                    return 0;
                }
            }
        }

        if (!strcmp(token->string, "\\")) {
            allowCrossLine = 1;
            continue;
        }

        return 1;
    }
}

int PC_ReadLineHandle(int handle, pc_token_t *pc_token)
{
    token_t token;
    int ret;

    if ((unsigned int)handle - 1u > 0x3e || !sourceFiles[handle]) {
        return 0;
    }

    ret = PC_ReadLine(sourceFiles[handle], &token);
    strcpy(pc_token->string, token.string);
    pc_token->type = token.type;
    pc_token->subtype = token.subtype;
    pc_token->intvalue = (int)token.intvalue;
    pc_token->floatvalue = (float)token.floatvalue;

    if (pc_token->type == 1) {
        StripDoubleQuotes(pc_token->string);
    }

    return ret;
}

int PC_Directive_pragma(source_t *source)
{
    token_t token;

    SourceWarning(source, "#pragma directive not supported");
    while (PC_ReadLine(source, &token)) {
    }

    return 1;
}

int PC_Directive_if_def(source_t *source, int type)
{
    token_t token;
    define_t *define;
    indent_t *indent;
    int skip;

    if (!PC_ReadLine(source, &token)) {
        SourceError(source, "no name after #ifdef");
        return 0;
    }

    if (token.type != 4) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "expected name after #ifdef, found %s", token.string);
        return 0;
    }

    define = PC_FindDefineInSource(source, token.string);
    skip = ((type != 8) ^ (define == NULL)) != 0;

    indent = (indent_t *)GetMemory(sizeof(*indent));
    indent->type = type;
    indent->skip = skip;
    indent->script = source->scriptstack;
    source->skip += skip;
    indent->next = (intptr_t)source->indentstack;
    source->indentstack = indent;

    return 1;
}

int PC_Directive_ifndef(source_t *source)
{
    return PC_Directive_if_def(source, 0x10);
}

int PC_Directive_ifdef(source_t *source)
{
    return PC_Directive_if_def(source, 8);
}

int PC_Directive_undef(source_t *source)
{
    token_t token;
    unsigned int hash;
    define_t *define;
    define_t *lastdefine;

    if (source->skip > 0) {
        return 1;
    }

    if (!PC_ReadLine(source, &token)) {
        SourceError(source, "no name after #undef");
        return 0;
    }

    if (token.type != 4) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "expected name, found %s", token.string);
        return 0;
    }

    if (!source->definehash) {
        return 1;
    }

    hash = PC_DefineHash(token.string);
    lastdefine = NULL;
    define = source->definehash[hash];
    while (define) {
        if (!strcmp(define->name, token.string)) {
            break;
        }
        lastdefine = define;
        define = (define_t *)(uintptr_t)define->hashnext;
    }

    if (!define) {
        return 1;
    }

    if (define->flags & 1) {
        SourceWarning(source, "can't undef %s", token.string);
        return 1;
    }

    if (lastdefine) {
        lastdefine->hashnext = define->hashnext;
    } else {
        source->definehash[hash] = (define_t *)(uintptr_t)define->hashnext;
    }

    PC_FreeTokenList(define->parms);
    PC_FreeTokenList(define->tokens);
    FreeMemory(define);
    return 1;
}

int PC_ReadToken(source_t *source, token_t *token)
{
    token_t newtoken;
    define_t *define;
    token_t *firsttoken;
    token_t *lasttoken;
    size_t len;

    for (;;) {
        if (!PC_ReadSourceToken(source, token)) {
            return 0;
        }

        if (token->type == 5) {
            if (token->string[0] == '#') {
                if (!PC_ReadDirective(source)) {
                    return 0;
                }
                continue;
            }

            if (token->string[0] == '$') {
                if (!PC_ReadDollarDirective(source)) {
                    return 0;
                }
                continue;
            }
        } else if (token->type == 1) {
            if (PC_ReadToken(source, &newtoken)) {
                if (newtoken.type == 1) {
                    len = strlen(token->string);
                    if (len > 0) {
                        token->string[len - 1] = '\0';
                    }

                    if (strlen(token->string) + strlen(newtoken.string + 1) >= sizeof(token->string)) {
                        SourceError(source, "string longer than MAX_TOKEN %d\n", (int)sizeof(token->string));
                        return 0;
                    }

                    strcat(token->string, newtoken.string + 1);
                } else {
                    PC_UnreadSourceToken(source, &newtoken);
                }
            }
        }

        if (source->skip) {
            continue;
        }

        if (token->type == 4) {
            define = PC_FindDefineInSource(source, token->string);
            if (define) {
                firsttoken = NULL;
                lasttoken = NULL;

                if (!PC_ExpandDefine(source, token, define, &firsttoken, &lasttoken)) {
                    return 0;
                }
                if (!firsttoken || !lasttoken) {
                    return 0;
                }

                lasttoken->next = source->tokens;
                source->tokens = firsttoken;
                continue;
            }
        }

        memcpy(&source->token, token, sizeof(*token));
        return 1;
    }
}

int PC_ReadTokenHandle(int handle, pc_token_t *pc_token)
{
    source_t *source;
    struct script_s *script;
    token_t token;
    int ret;

    if (handle <= 0 || handle >= 64) {
        return 0;
    }

    if (!sourceFiles[handle]) {
        return 0;
    }

    for (;;) {
        source = sourceFiles[handle];
        if (!source) {
            return 0;
        }

        script = (struct script_s *)source->scriptstack;
        while (script != NULL && script->script_p != NULL &&
               script->script_p < script->end_p && *script->script_p == '`') {
            ++script->script_p;
        }

        ret = PC_ReadToken(sourceFiles[handle], &token);
        if (ret) {
            strcpy(pc_token->string, token.string);
            pc_token->type = token.type;
            pc_token->subtype = token.subtype;
            pc_token->intvalue = token.intvalue;
            pc_token->floatvalue = token.floatvalue;
            if (pc_token->type == 1) {
                StripDoubleQuotes(pc_token->string);
            }
            return ret;
        }

        source = sourceFiles[handle];
        if (!source) {
            return 0;
        }

        script = (struct script_s *)source->scriptstack;
        if (!script || !script->script_p || script->script_p >= script->end_p ||
            *script->script_p != '`') {
            return 0;
        }

        ++script->script_p;
    }
}

int PC_Directive_define(source_t *source)
{
    token_t token;
    token_t tok;
    define_t *define;
    define_t *existing;
    token_t *last;
    token_t *copy;
    unsigned int hash;

    if (source->skip > 0) {
        return 1;
    }

    if (!PC_ReadLine(source, &token)) {
        SourceError(source, "#define without name");
        return 0;
    }

    if (token.type != 4) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "expected name after #define, found %s", token.string);
        return 0;
    }

    existing = PC_FindDefineInSource(source, token.string);
    if (existing) {
        if (existing->flags & 1) {
            SourceError(source, "can't redefine %s", token.string);
            return 0;
        }

        SourceWarning(source, "redefinition of %s", token.string);
        PC_UnreadSourceToken(source, &token);
        if (!PC_Directive_undef(source)) {
            return 0;
        }
    }

    define = (define_t *)GetMemory(sizeof(*define) + strlen(token.string) + 1);
    memset(define, 0, sizeof(*define));
    define->name = (char *)(define + 1);
    strcpy(define->name, token.string);

    hash = PC_DefineHash(define->name);
    define->hashnext = (intptr_t)(uintptr_t)source->definehash[hash];
    source->definehash[hash] = define;

    if (!PC_ReadLine(source, &token)) {
        return 1;
    }

    if (token.endwhitespace_p - token.whitespace_p <= 0 && !strcmp(token.string, "(")) {
        last = NULL;

        if (PC_ReadToken(source, &tok)) {
            if (strcmp(tok.string, ")")) {
                PC_UnreadSourceToken(source, &tok);
            } else if (!PC_ReadLine(source, &token)) {
                return 1;
            } else {
                goto read_body_tokens;
            }
        }

        for (;;) {
            int parmIndex;

            if (!PC_ReadLine(source, &token)) {
                SourceError(source, "expected define parameter");
                return 0;
            }

            if (token.type != 4) {
                SourceError(source, "invalid define parameter");
                return 0;
            }

            parmIndex = PC_FindDefineParmIndex(define, token.string);
            if (parmIndex >= 0) {
                SourceError(source, "two the same define parameters");
                return 0;
            }

            copy = PC_AllocTokenCopy(&token);
            copy->whitespace_p = NULL;
            copy->endwhitespace_p = NULL;
            copy->linescrossed = 0;
            copy->next = NULL;

            if (last) {
                last->next = copy;
            } else {
                define->parms = copy;
            }
            ++define->numparms;

            if (!PC_ReadLine(source, &token)) {
                SourceError(source, "define parameters not terminated");
                return 0;
            }

            if (!strcmp(token.string, ")")) {
                break;
            }

            if (strcmp(token.string, ",")) {
                SourceError(source, "define not terminated");
                return 0;
            }

            last = copy;
        }

        if (!PC_ReadLine(source, &token)) {
            return 1;
        }
    }

read_body_tokens:
    last = NULL;
    do {
        copy = PC_AllocTokenCopy(&token);

        if (copy->type == 4 && !strcmp(copy->string, define->name)) {
            SourceError(source, "recursive define (removed recursion)");
            continue;
        }

        copy->whitespace_p = NULL;
        copy->endwhitespace_p = NULL;
        copy->linescrossed = 0;
        copy->next = NULL;

        if (last) {
            last->next = copy;
        } else {
            define->tokens = copy;
        }
        last = copy;
    } while (PC_ReadLine(source, &token));

    if (last && (!strcmp(define->tokens->string, "##") || !strcmp(last->string, "##"))) {
        SourceError(source, "define with misplaced ##");
        return 0;
    }

    return 1;
}

define_t * PC_CopyDefine(source_t *source, define_t *define)
{
    define_t *newdefine;
    size_t nameLen;

    (void)source;

    nameLen = strlen(define->name) + 1;
    newdefine = (define_t *)GetMemory(sizeof(*newdefine) + nameLen);

    newdefine->name = (char *)(newdefine + 1);
    strcpy(newdefine->name, define->name);

    newdefine->flags = define->flags;
    newdefine->builtin = define->builtin;
    newdefine->numparms = define->numparms;
    newdefine->next = 0;
    newdefine->hashnext = 0;
    newdefine->tokens = PC_CopyTokenList(define->tokens);
    newdefine->parms = PC_CopyTokenList(define->parms);

    return newdefine;
}

source_t * LoadSourceFile(const char *filename)
{
    script_t *script;
    source_t *source;
    define_t *define;

    script = LoadScriptFile(filename);
    if (!script) {
        return NULL;
    }

    script->next = NULL;

    source = (source_t *)GetMemory(sizeof(*source));
    memset(source, 0, sizeof(*source));
    strncpy(source->filename, filename, sizeof(source->filename));
    source->scriptstack = script;
    source->definehash = (define_t **)GetClearedMemory(1024 * sizeof(*source->definehash));

    for (define = globaldefines; define; define = (define_t *)(uintptr_t)define->next) {
        unsigned int hash;
        define_t *copy = PC_CopyDefine(source, define);

        hash = PC_DefineHash(copy->name);
        copy->hashnext = (intptr_t)(uintptr_t)source->definehash[hash];
        source->definehash[hash] = copy;
    }

    return source;
}

int PC_LoadSourceHandle(const char *filename)
{
    int i;
    source_t *source;

    for (i = 1; i < 64; i++) {
        if (!sourceFiles[i]) {
            source = LoadSourceFile(filename);
            if (!source) {
                return 0;
            }

            sourceFiles[i] = source;
            return i;
        }
    }

    return 0;
}

int PC_Evaluate(source_t *source, long int *intvalue, double *floatvalue, int integer)
{
    token_t token;
    token_t *firsttoken = NULL;
    token_t *lasttoken = NULL;
    int defined = 0;

    if (intvalue) {
        *intvalue = 0;
    }
    if (floatvalue) {
        *floatvalue = 0.0;
    }

    if (!PC_ReadLine(source, &token)) {
        SourceError(source, "no value after #if/#elif");
        return 0;
    }

    do {
        int result = PC_EvaluateTokenForList(source, &token, &firsttoken, &lasttoken, &defined);
        if (!result) {
            return 0;
        }
    } while (PC_ReadLine(source, &token));

    if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer)) {
        return 0;
    }

    PC_FreeTokenList(firsttoken);
    return 1;
}

int PC_Directive_evalfloat(source_t *source)
{
    double value;

    if (!PC_Evaluate(source, NULL, &value, 0)) {
        return 0;
    }

    PC_UnreadEvalFloatToken(source, value, 0);
    return 1;
}

int PC_Directive_eval(source_t *source)
{
    long int value;

    if (!PC_Evaluate(source, &value, NULL, 1)) {
        return 0;
    }

    PC_UnreadEvalIntToken(source, (int)value, 0);
    return 1;
}

int PC_Directive_if(source_t *source)
{
    long int value;
    int skip;
    indent_t *indent;

    if (!PC_Evaluate(source, &value, NULL, 1)) {
        return 0;
    }

    skip = (value == 0);
    indent = (indent_t *)(GetMemory(sizeof(*indent)));
    indent->type = 1;
    indent->script = source->scriptstack;
    indent->skip = skip;
    source->skip += skip;
    indent->next = (intptr_t)source->indentstack;
    source->indentstack = indent;

    return 1;
}

int PC_Directive_elif(source_t *source)
{
    indent_t *indent;
    int type;
    long int value;
    int skip;

    indent = source->indentstack;
    if (!indent || indent->script != source->scriptstack) {
        SourceError(source, "misplaced #elif");
        return 0;
    }

    type = indent->type;
    source->indentstack = *(indent_t **)&indent->next;
    source->skip -= indent->skip;
    FreeMemory(indent);

    if (!type || type == 2) {
        SourceError(source, "misplaced #elif");
        return 0;
    }

    if (!PC_Evaluate(source, &value, NULL, 1))
        return 0;

    skip = (value == 0);
    indent = (indent_t *)(GetMemory(sizeof(*indent)));
    indent->type = 4;
    indent->script = source->scriptstack;
    indent->skip = skip;
    source->skip += skip;
    indent->next = (intptr_t)source->indentstack;
    source->indentstack = indent;

    return 1;
}

int PC_DollarEvaluate(source_t *source, long int *intvalue, double *floatvalue, int integer)
{
    token_t token;
    token_t *firsttoken = NULL;
    token_t *lasttoken = NULL;
    int defined = 0;
    int indent = 1;

    if (intvalue) {
        *intvalue = 0;
    }
    if (floatvalue) {
        *floatvalue = 0.0;
    }

    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "no leading ( after $evalint/$evalfloat");
        return 0;
    }

    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "nothing to evaluate");
        return 0;
    }

    do {
        if (token.type == 3 || token.type == 5) {
            if (token.string[0] == '(') {
                ++indent;
            } else if (token.string[0] == ')') {
                --indent;
            }

            if (indent <= 0) {
                break;
            }

            PC_AppendTokenCopy(&firsttoken, &lasttoken, &token);
        } else {
            int result = PC_EvaluateTokenForList(source, &token, &firsttoken, &lasttoken, &defined);
            if (!result) {
                return 0;
            }
        }
    } while (PC_ReadSourceToken(source, &token));

    if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer)) {
        return 0;
    }

    PC_FreeTokenList(firsttoken);
    return 1;
}

int PC_DollarDirective_evalfloat(source_t *source)
{
    double value;

    if (!PC_DollarEvaluate(source, NULL, &value, 0)) {
        return 0;
    }

    PC_UnreadEvalFloatToken(source, value, 1);
    return 1;
}

int PC_DollarDirective_evalint(source_t *source)
{
    long int value;

    if (!PC_DollarEvaluate(source, &value, NULL, 1)) {
        return 0;
    }

    PC_UnreadEvalIntToken(source, (int)value, 1);
    return 1;
}

directive_t directives[20] = {
    { (char *)&str_00222058, &PC_Directive_if },
    { (char *)&str_0022205c, &PC_Directive_ifdef },
    { (char *)&str_00222064, &PC_Directive_ifndef },
    { (char *)&str_0022206c, &PC_Directive_elif },
    { (char *)&str_00222074, &PC_Directive_else },
    { (char *)&str_0022207c, &PC_Directive_endif },
    { (char *)&str_00222084, &PC_Directive_include },
    { (char *)&str_0022208c, &PC_Directive_define },
    { (char *)&str_00222094, &PC_Directive_undef },
    { (char *)&str_00219df4, &PC_Directive_line },
    { (char *)&str_002168a4, &PC_Directive_error },
    { (char *)&str_0022209c, &PC_Directive_pragma },
    { (char *)&str_002220a4, &PC_Directive_eval },
    { (char *)&str_0022204c, &PC_Directive_evalfloat },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 }
};
directive_t dollardirectives[20] = {
    { (char *)&str_00222044, &PC_DollarDirective_evalint },
    { (char *)&str_0022204c, &PC_DollarDirective_evalfloat },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 }
};

const char str_002168a4[] = "error";
const char str_00219df4[] = "line";
const char str_00222044[] = "evalint";
const char str_0022204c[] = "evalfloat";
const char str_00222058[] = "if";
const char str_0022205c[] = "ifdef";
const char str_00222064[] = "ifndef";
const char str_0022206c[] = "elif";
const char str_00222074[] = "else";
const char str_0022207c[] = "endif";
const char str_00222084[] = "include";
const char str_0022208c[] = "define";
const char str_00222094[] = "undef";
const char str_0022209c[] = "pragma";
const char str_002220a4[] = "eval";
