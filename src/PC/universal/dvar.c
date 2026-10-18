#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <math.h>
#include <ctype.h>

extern int dvar_modifiedFlags;
extern dvar_t *sortedDvars;
extern int dvarCount;
static const dvar_t *dvar_cheats;
static dvar_t dvarPool[1280];
static dvar_t *dvarHashTable[256];
static float dvarVectorPool[12];
static int dvarVectorIndex;

enum {
    DVAR_TYPE_BOOL = 0,
    DVAR_TYPE_FLOAT = 1,
    DVAR_TYPE_VEC2 = 2,
    DVAR_TYPE_VEC3 = 3,
    DVAR_TYPE_VEC4 = 4,
    DVAR_TYPE_INT = 5,
    DVAR_TYPE_ENUM = 6,
    DVAR_TYPE_STRING = 7,
    DVAR_TYPE_COLOR = 8,
    DVAR_TYPE_COUNT = 9
};
enum {
    DVAR_FLAG_WRITEPROTECTED = 0x10,
    DVAR_FLAG_LATCH = 0x20,
    DVAR_FLAG_READONLY = 0x40,
    DVAR_FLAG_CHEAT = 0x80,
    DVAR_FLAG_EXTERNAL = 0x4000,
    DVAR_MAX_COUNT = 0x500,
    CON_CHANNEL_LOGFILEONLY = 4,
    DVAR_INVALID_ENUM_INDEX = -1337
};
static const char dvarDigitStrings[10][2] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
};
/* dvarOnOffStrings: migrated from the ILP32 data blob to typed C (re-lays-out
 * per target). Blob extent was 8 B of pointers + 24 B inter-symbol padding (next
 * symbol theFxHelper distinct; only [0]/[1] read) -- padding dropped. Targets are
 * the reconstructed literals in common.c. (x64 port Stage 2.) */
extern const char str_00219524[], str_00219528[];
const char *dvarOnOffStrings[2] = {
    (const char *)str_00219524,  /* "off" */
    (const char *)str_00219528,  /* "on"  */
};
static Bool isDvarSystemActive;
static Bool isLoadingAutoExecGlobalFlag;

extern char *va(const char *format, ...);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern byte *__DefaultRuneLocale;
extern int ___maskrune(int ch, unsigned int mask);
extern int ___tolower(int c);
extern void Com_BeginParseSession(const char *filename);
extern void Com_EndParseSession(void);
extern void Com_PrintMessage(int channel, const char *msg);
extern const char *Com_Parse(const char **data_p);
extern const char *Com_ParseOnLine(const char **data_p);
extern char *CopyStringInternal(const char *in);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void Com_SkipRestOfLine(const char **data);
extern void Dvar_AddCommands(void);
extern int I_stricmp(const char *s1, const char *s2);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern int stricmp(const char *s1, const char *s2);
extern int strnicmp(const char *s1, const char *s2, size_t n);
extern int atoi(const char *string);
extern double atof(const char *string);
extern int strcmp(const char *s1, const char *s2);
#include <stdio.h>
extern float floorf(float x);

void Dvar_SetInAutoExec(int inAutoExec);
Bool Dvar_IsSystemActive(void);
Bool Dvar_IsValidName(const char *dvarName);
const char *Dvar_EnumToString(const dvar_t *dvar);
const char *__attribute_regparm__(2) Dvar_ValueToString(const dvar_t *dvar, DvarValue value);
static const char *Dvar_ValueToString_impl(const dvar_t *dvar, DvarValue value);
const char *Dvar_DisplayableValue(const dvar_t *dvar);
const char *Dvar_DisplayableResetValue(const dvar_t *dvar);
const char *Dvar_DisplayableLatchedValue(const dvar_t *dvar);
static Bool __attribute_regparm__(3) Dvar_ValuesEqual(byte type, DvarValue val0, DvarValue val1);
Bool Dvar_HasLatchedValue(const dvar_t *dvar);
Bool Dvar_IsAtDefaultValue(const dvar_t *dvar);
void Dvar_ClearModified(const dvar_t *dvar);
void Dvar_SetModified(const dvar_t *dvar);
void Dvar_AddFlags(const dvar_t *dvar, int flags);
void Dvar_ResetScriptInfo(void);
const char *Dvar_IndexStringToEnumString(const dvar_t *dvar, const char *indexString);
static void __attribute_regparm__(2) Dvar_StringToColor(const char *string, byte color[4]);
static DvarValue Dvar_StringToValue_impl(int type, DvarLimits domain, const char *string);
static DvarValue __attribute_regparm__(3) Dvar_StringToValue(
    int type, uint32_t domainLo, uint32_t domainHi, const char *string);
void Dvar_GetUnpackedColor(const dvar_t *dvar, vec_t *expandedColor);
static void __attribute_regparm__(2) Dvar_SetLatchedValue(const dvar_t *dvar, DvarValue value);
void Dvar_Shutdown(void);
Bool Dvar_AnyLatchedValues(void);
qboolean Com_SaveDvarsToBuffer(const char **dvarnames, int numDvars, char *buffer, int bufsize);
void Dvar_GetUnpackedColorByName(const char *dvarName, vec_t *expandedColor);
const char *Dvar_GetVariantString(const char *dvarName);
const char *Dvar_GetString(const char *dvarName);
float Dvar_GetFloat(const char *dvarName);
int Dvar_GetInt(const char *dvarName);
Bool Dvar_GetBool(const char *dvarName);
const dvar_t *Dvar_FindVar(const char *dvarName);
void Dvar_UpdateEnumDomain(const dvar_t *dvar, const char **stringTable);
static const char *__attribute_regparm__(3) Dvar_DomainToString_Internal_impl(
    unsigned char type, DvarLimits domain, char *outBuffer, int outBufferLen, int *outLineCount);
static const char *__attribute_regparm__(3) Dvar_DomainToString_Internal(
    int type, uint32_t domainLo, uint32_t domainHi, char *outBuffer, int outBufferLen, int *outLineCount);
const char *Dvar_DomainToString_GetLines(unsigned char type, DvarLimits domain, char *outBuffer, int outBufferLen, int *outLineCount);
void Dvar_PrintDomain(unsigned char type, DvarLimits domain);
static void __attribute_regparm__(1) Dvar_PerformUnregistration(dvar_t *dvar);
void Dvar_UnregisterSystem(int sysFlag);
static void __attribute_regparm__(2) Dvar_UpdateResetValue(const dvar_t *dvar, DvarValue value);
static Bool Dvar_ValueInDomain(int type, DvarValue value, DvarLimits domain);
static DvarValue Dvar_ClampValueToDomain(int type, DvarValue value, DvarValue resetValue, DvarLimits domain);
static Bool Dvar_CanChangeValue(const dvar_t *dvar, DvarSetSource source);
static void Dvar_UpdateValue(dvar_t *dvar, DvarValue value);
static void __attribute_regparm__(3) Dvar_MakeExplicitType(
    dvar_t *dvar, const char *dvarName, int type, unsigned short flags, DvarValue resetValue, DvarLimits domain);
void Dvar_ChangeResetValue(const dvar_t *dvar, DvarValue value);
static void __attribute_regparm__(3) Dvar_SetVariant(
    const dvar_t *dvar, DvarValue value, DvarSetSource source);
void Dvar_SetCheatState(void);
void Dvar_Reset(const dvar_t *dvar, DvarSetSource setSource);
static void __attribute_regparm__(3) Dvar_SetFromStringFromSource(
    const dvar_t *dvar, const char *string, DvarSetSource source);
void Dvar_SetFromString(const dvar_t *dvar, const char *string);
void Dvar_SetString(const dvar_t *dvar, const char *value);
void Dvar_SetColorFromSource(const dvar_t *dvar, float r, float g, float b, float a, DvarSetSource source);
void Dvar_SetColor(const dvar_t *dvar, float r, float g, float b, float a);
void Dvar_SetFloatFromSource(const dvar_t *dvar, float value, DvarSetSource source);
void Dvar_SetIntFromSource(const dvar_t *dvar, int value, DvarSetSource source);
void Dvar_SetBoolFromSource(const dvar_t *dvar, unsigned char value, DvarSetSource source);
static const dvar_t *Dvar_RegisterVariant(const char *dvarName, short unsigned int flags, DvarValue value, DvarLimits domain);
const dvar_t *Dvar_RegisterColor(const char *dvarName, float r, float g, float b, float a, int flags);
const dvar_t *Dvar_RegisterEnum(const char *dvarName, const char **valueList, int defaultIndex, unsigned short flags);
const dvar_t *Dvar_RegisterString(const char *dvarName, const char *value, unsigned short flags);
const dvar_t *Dvar_RegisterVec4(const char *dvarName, float x, float y, float z, float w, float min, float max, unsigned short flags);
const dvar_t *Dvar_RegisterVec3(const char *dvarName, float x, float y, float z, float min, float max, unsigned short flags);
const dvar_t *Dvar_RegisterVec2(const char *dvarName, float x, float y, float min, float max, unsigned short flags);
const dvar_t *Dvar_RegisterFloat(const char *dvarName, float value, float min, float max, unsigned short flags);
const dvar_t *Dvar_RegisterInt(const char *dvarName, int value, int min, int max, unsigned short flags);
const dvar_t *Dvar_RegisterBool(const char *dvarName, unsigned char value, unsigned short flags);
void Dvar_Init(void);
void Dvar_ResetDvars(unsigned int filter, DvarSetSource setSource);
qboolean Com_LoadDvarsFromBuffer(const char **dvarnames, int numDvars, const char *buffer, const char *filename);
void Dvar_SetBool(const dvar_t *dvar, unsigned char value);
void Dvar_SetFloat(const dvar_t *dvar, float value);
void Dvar_SetInt(const dvar_t *dvar, int value);
void Dvar_SetVec2(const dvar_t *dvar, float x, float y);
void Dvar_SetVec3(const dvar_t *dvar, float x, float y, float z);
void Dvar_SetVec4(const dvar_t *dvar, float x, float y, float z, float w);
void Dvar_SetBoolByName(const char *dvarName, unsigned char value);
void Dvar_SetIntByName(const char *dvarName, int value);
void Dvar_SetFloatByName(const char *dvarName, float value);
void Dvar_SetVec2ByName(const char *dvarName, float x, float y);
void Dvar_SetVec3ByName(const char *dvarName, float x, float y, float z);
void Dvar_SetVec4ByName(const char *dvarName, float x, float y, float z, float w);
void Dvar_SetStringByName(const char *dvarName, const char *value);
void Dvar_SetColorByName(const char *dvarName, int r, int g, int b, int a);
const dvar_t *Dvar_SetFromStringByNameFromSource(const char *dvarName, const char *string, DvarSetSource source);
void Dvar_SetCommand(const char *dvarName, const char *string);
void Dvar_SetFromStringByName(const char *dvarName, const char *string);

typedef const dvar_t *(__attribute_regparm__(3) * DvarRegisterVariantRegparmFn)(
    const char *dvarName, int type, unsigned int flags, DvarValue value, DvarLimits domain);
typedef void(__attribute_regparm__(3) * DvarSetVariantRegparmFn)(
    const dvar_t *dvar, DvarValue value, DvarSetSource source);
typedef void(__attribute_regparm__(3) * DvarSetFromStringFromSourceRegparmFn)(
    const dvar_t *dvar, const char *string, DvarSetSource source);
typedef void(__attribute_regparm__(2) * DvarUpdateResetValueRegparmFn)(
    const dvar_t *dvar, DvarValue value);
typedef void(__attribute_regparm__(1) * DvarPerformUnregistrationRegparmFn)(
    dvar_t *dvar);
typedef const char *(__attribute_regparm__(3) * DvarDomainToStringRegparmFn)(
    int type, uint32_t domainLo, uint32_t domainHi, char *outBuffer, int outBufferLen, int *outLineCount);
typedef DvarValue(__attribute_regparm__(3) * DvarStringToValueRegparmFn)(
    int type, uint32_t domainLo, uint32_t domainHi, const char *string);

static const dvar_t *Dvar_RegisterVariant_impl(
    const char *dvarName, int type, unsigned int flags, DvarValue value, DvarLimits domain);

static const dvar_t *Dvar_RegisterVariantReg(const char *dvarName, int type, unsigned int flags, DvarValue value, DvarLimits domain)
{
    return Dvar_RegisterVariant_impl(dvarName, type, flags, value, domain);
}

static void Dvar_SetVariantReg(const dvar_t *dvar, DvarValue value, DvarSetSource source)
{
    ((DvarSetVariantRegparmFn)Dvar_SetVariant)(dvar, value, source);
}

static void Dvar_SetFromStringFromSourceReg(const dvar_t *dvar, const char *string, DvarSetSource source)
{
    Dvar_SetFromStringFromSource(dvar, string, source);
}

static void Dvar_UpdateResetValueReg(const dvar_t *dvar, DvarValue value)
{
    Dvar_UpdateResetValue(dvar, value);
}

static void Dvar_PerformUnregistrationReg(dvar_t *dvar)
{
    Dvar_PerformUnregistration(dvar);
}

static const char *Dvar_DomainToString_InternalReg(int type, DvarLimits domain, char *outBuffer, int outBufferLen, int *outLineCount)
{
    return Dvar_DomainToString_Internal_impl(type, domain, outBuffer, outBufferLen, outLineCount);
}

static DvarValue Dvar_StringToValueReg(int type, DvarLimits domain, const char *string)
{
    return Dvar_StringToValue_impl(type, domain, string);
}

static inline __attribute__((always_inline)) unsigned int Dvar_GenerateHashValue(const char *name)
{
    unsigned int hash;
    int i;

    if (!name) {
        Com_Error(1, "\x15null name in generateHashValue");
    }

    hash = 0;
    for (i = 0; name[i]; ++i) {
        hash += (unsigned int)___tolower(name[i]) * (unsigned int)(i + 0x77);
    }

    return hash & 0xff;
}

static Bool Dvar_IsFiniteFloat(float value)
{
    return isfinite(value) != 0;
}

static Bool Dvar_FloatInDomain(float value, float min, float max)
{
    return Dvar_IsFiniteFloat(value) && min <= value && value <= max;
}

static float Dvar_FallbackFloatForDomain(float min, float max)
{
    if (min <= 0.0f && max >= 0.0f) {
        return 0.0f;
    }

    return min;
}

static float Dvar_ClampFloatToDomain(float value, float resetValue, float min, float max)
{
    if (!Dvar_IsFiniteFloat(value)) {
        value = Dvar_IsFiniteFloat(resetValue) ? resetValue : Dvar_FallbackFloatForDomain(min, max);
    }

    if (value < min) {
        value = min;
    } else if (value > max) {
        value = max;
    }

    return value;
}

static void Dvar_ClampVectorToDomainWithReset(vec_t *vector, const vec_t *reset, int components, float min, float max)
{
    int i;

    for (i = 0; i < components; ++i) {
        float resetValue = reset ? reset[i] : Dvar_FallbackFloatForDomain(min, max);

        if (!Dvar_IsFiniteFloat(vector[i])) {
            vector[i] = Dvar_IsFiniteFloat(resetValue) ? resetValue : Dvar_FallbackFloatForDomain(min, max);
        }

        if (vector[i] < min) {
            vector[i] = min;
        } else if (vector[i] > max) {
            vector[i] = max;
        }
    }
}

static void Dvar_ClampVectorToDomain(vec_t *vector, int components, float min, float max)
{
    Dvar_ClampVectorToDomainWithReset(vector, NULL, components, min, max);
}

static inline __attribute__((always_inline)) byte Dvar_FloatToColorComponent(float value)
{
    if (!(value < 1.0f)) {
        value = 1.0f;
    } else if (value < 0.0f) {
        value = 0.0f;
    }

    return (byte)(int)floorf(value * 255.0f + 0.5f);
}

static Bool Dvar_StringEqualsRef(const char *value, const char *reference)
{
    if (!value || !reference) {
        return value == reference;
    }

    return value == reference || strcmp(value, reference) == 0;
}

static Bool Dvar_IsStaticValueString(const char *value)
{
    if (!value || !*value) {
        return 1;
    }

    if (!value[1] && value[0] >= '0' && value[0] <= '9') {
        return 1;
    }

    return value == dvarOnOffStrings[0] || value == dvarOnOffStrings[1];
}

static const char *Dvar_CanonicalizeValueString(const char *value)
{
    if (!value || !*value) {
        return "";
    }

    if (!value[1] && value[0] >= '0' && value[0] <= '9') {
        return dvarDigitStrings[value[0] - '0'];
    }

    if (value[0] == 'o') {
        if (!strcmp(value, "on")) {
            return dvarOnOffStrings[1];
        }
        if (!strcmp(value, "off")) {
            return dvarOnOffStrings[0];
        }
    }

    return CopyStringInternal(value);
}

static const char *Dvar_RebuildResetString(const dvar_t *dvar, const char *value)
{
    if (Dvar_StringEqualsRef(value, dvar->current.string)) {
        return dvar->current.string;
    }
    if (Dvar_StringEqualsRef(value, dvar->latched.string)) {
        return dvar->latched.string;
    }

    return Dvar_CanonicalizeValueString(value);
}

static const char *Dvar_RebuildStringValue(const char *value, const char *first, const char *second)
{
    if (Dvar_StringEqualsRef(value, first)) {
        return first;
    }
    if (Dvar_StringEqualsRef(value, second)) {
        return second;
    }

    return Dvar_CanonicalizeValueString(value);
}

static void Dvar_FreeOwnedString(const char *value, const char *keep0, const char *keep1)
{
    if (!value || value == keep0 || value == keep1 || Dvar_IsStaticValueString(value)) {
        return;
    }

    Z_FreeInternal((void *)value);
}

static inline __attribute__((always_inline)) Bool Dvar_VectorEqual(const vec_t *a, const vec_t *b, int count)
{
    int i;

    for (i = 0; i < count; ++i) {
        if (a[i] != b[i]) {
            return 0;
        }
    }

    return 1;
}

static Bool Dvar_VectorInDomain(const vec_t *vector, int components, float min, float max)
{
    int i;

    for (i = 0; i < components; ++i) {
        if (!Dvar_FloatInDomain(vector[i], min, max)) {
            return 0;
        }
    }

    return 1;
}

static DvarLimits Dvar_UnpackLimits(uint32_t lo, uint32_t hi)
{
    union {
        DvarLimits domain;
        struct {
            uint32_t lo;
            uint32_t hi;
        } raw;
    } bits;

    bits.raw.lo = lo;
    bits.raw.hi = hi;
    return bits.domain;
}

static vec_t *Dvar_AllocTempVector(int components)
{
    vec_t *vector;
    int i;
    int index;

    index = dvarVectorIndex;
    if (index + components > 12) {
        index = 0;
    }

    dvarVectorIndex = index + components;
    vector = &dvarVectorPool[index];
    for (i = 0; i < components; ++i) {
        vector[i] = 0.0f;
    }

    return vector;
}

static int Dvar_StringToEnumIndex(DvarLimits domain, const char *string)
{
    int i;
    int index;
    int len;

    if (domain.enumeration.stringCount > 0) {
        for (i = 0; i < domain.enumeration.stringCount; ++i) {
            if (!stricmp(string, domain.enumeration.strings[i])) {
                return i;
            }
        }
    }

    if (!string[0]) {
        index = 0;
    } else {
        index = 0;
        for (i = 0; string[i]; ++i) {
            if (string[i] < '0' || string[i] > '9') {
                index = DVAR_INVALID_ENUM_INDEX;
                break;
            }

            index = index * 10 + (string[i] - '0');
        }
    }

    if (index >= 0 && index < domain.enumeration.stringCount) {
        return index;
    }

    len = 0;
    while (string[len]) {
        ++len;
    }

    for (i = 0; i < domain.enumeration.stringCount; ++i) {
        if (!strnicmp(string, domain.enumeration.strings[i], len)) {
            return i;
        }
    }

    return DVAR_INVALID_ENUM_INDEX;
}

static inline __attribute__((always_inline)) int Dvar_DescribeIntDomain(char *outBuffer, int outBufferLen, int min, int max)
{
    if (min == (-2147483647 - 1)) {
        if (max == 2147483647) {
            return snprintf(outBuffer, outBufferLen, "Domain is any integer");
        }

        return snprintf(outBuffer, outBufferLen, "Domain is any integer %i or smaller", max);
    }

    if (max == 2147483647) {
        return snprintf(outBuffer, outBufferLen, "Domain is any integer %i or bigger", min);
    }

    return snprintf(outBuffer, outBufferLen, "Domain is any integer from %i to %i", min, max);
}

static inline __attribute__((always_inline)) int Dvar_DescribeFloatDomain(char *outBuffer, int outBufferLen, float min, float max)
{
    if (min == -3.4028234663852886e+38f) {
        if (max == 3.4028234663852886e+38f) {
            return snprintf(outBuffer, outBufferLen, "Domain is any number");
        }

        return snprintf(outBuffer, outBufferLen, "Domain is any number %g or smaller", max);
    }

    if (max == 3.4028234663852886e+38f) {
        return snprintf(outBuffer, outBufferLen, "Domain is any number %g or bigger", min);
    }

    return snprintf(outBuffer, outBufferLen, "Domain is any number from %g to %g", min, max);
}

static inline __attribute__((always_inline)) int Dvar_DescribeVectorDomain(char *outBuffer, int outBufferLen, int components, float min, float max)
{
    if (min == -3.4028234663852886e+38f) {
        if (max == 3.4028234663852886e+38f) {
            return snprintf(outBuffer, outBufferLen, "Domain is any %iD vector", components);
        }

        return snprintf(
            outBuffer,
            outBufferLen,
            "Domain is any %iD vector with components %g or smaller",
            components,
            max);
    }

    if (max == 3.4028234663852886e+38f) {
        return snprintf(
            outBuffer,
            outBufferLen,
            "Domain is any %iD vector with components %g or bigger",
            components,
            min);
    }

    return snprintf(
        outBuffer,
        outBufferLen,
        "Domain is any %iD vector with components from %g to %g",
        components,
        min,
        max);
}

static Bool Dvar_ValueInDomain(int type, DvarValue value, DvarLimits domain)
{
    switch (type) {
    case DVAR_TYPE_BOOL:
        return 1;
    case DVAR_TYPE_FLOAT:
        return Dvar_FloatInDomain(value.value, domain.value.min, domain.value.max);
    case DVAR_TYPE_VEC2:
        return Dvar_VectorInDomain(value.vector, 2, domain.vector.min, domain.vector.max);
    case DVAR_TYPE_VEC3:
        return Dvar_VectorInDomain(value.vector, 3, domain.vector.min, domain.vector.max);
    case DVAR_TYPE_VEC4:
        return Dvar_VectorInDomain(value.vector, 4, domain.vector.min, domain.vector.max);
    case DVAR_TYPE_INT:
        return domain.integer.min <= value.integer && value.integer <= domain.integer.max;
    case DVAR_TYPE_ENUM:
        return value.integer >= 0 &&
               (value.integer < domain.enumeration.stringCount || !value.integer);
    case DVAR_TYPE_STRING:
    case DVAR_TYPE_COLOR:
        return 1;
    default:
        return 0;
    }
}

static DvarValue Dvar_ClampValueToDomain(int type, DvarValue value, DvarValue resetValue, DvarLimits domain)
{
    switch (type) {
    case DVAR_TYPE_BOOL:
        value.integer = value.enabled != 0;
        break;
    case DVAR_TYPE_FLOAT:
        value.value = Dvar_ClampFloatToDomain(value.value, resetValue.value, domain.value.min, domain.value.max);
        break;
    case DVAR_TYPE_VEC2:
        Dvar_ClampVectorToDomainWithReset(value.vector, resetValue.vector, 2, domain.vector.min, domain.vector.max);
        break;
    case DVAR_TYPE_VEC3:
        Dvar_ClampVectorToDomainWithReset(value.vector, resetValue.vector, 3, domain.vector.min, domain.vector.max);
        break;
    case DVAR_TYPE_VEC4:
        Dvar_ClampVectorToDomainWithReset(value.vector, resetValue.vector, 4, domain.vector.min, domain.vector.max);
        break;
    case DVAR_TYPE_INT:
        if (value.integer < domain.integer.min) {
            value.integer = domain.integer.min;
        } else if (value.integer > domain.integer.max) {
            value.integer = domain.integer.max;
        }
        break;
    case DVAR_TYPE_ENUM:
        if (value.integer < 0 || value.integer >= domain.enumeration.stringCount) {
            value.integer = resetValue.integer;
        }
        break;
    default:
        break;
    }

    return value;
}

static Bool Dvar_CanChangeValue(const dvar_t *dvar, DvarSetSource source)
{
    if (dvar->flags & DVAR_FLAG_READONLY) {
        Com_Printf("%s is read only.\n", dvar->name);
        return 0;
    }

    if (dvar->flags & DVAR_FLAG_WRITEPROTECTED) {
        Com_Printf("%s is write protected.\n", dvar->name);
        return 0;
    }

    if (source == DVAR_SOURCE_EXTERNAL &&
        (dvar->flags & DVAR_FLAG_CHEAT) &&
        dvar_cheats &&
        !dvar_cheats->current.enabled) {
        Com_Printf("%s is cheat protected.\n", dvar->name);
        return 0;
    }

    return 1;
}

static void Dvar_UpdateValue(dvar_t *dvar, DvarValue value)
{
    const char *oldCurrentString;
    const char *oldLatchedString;
    const char *oldResetString;
    Bool freeOldCurrentString;

    switch (dvar->type) {
    case DVAR_TYPE_VEC2:
        dvar->current.vector[0] = value.vector[0];
        dvar->current.vector[1] = value.vector[1];
        dvar->latched.vector[0] = value.vector[0];
        dvar->latched.vector[1] = value.vector[1];
        return;
    case DVAR_TYPE_VEC3:
        dvar->current.vector[0] = value.vector[0];
        dvar->current.vector[1] = value.vector[1];
        dvar->current.vector[2] = value.vector[2];
        dvar->latched.vector[0] = value.vector[0];
        dvar->latched.vector[1] = value.vector[1];
        dvar->latched.vector[2] = value.vector[2];
        return;
    case DVAR_TYPE_VEC4:
        dvar->current.vector[0] = value.vector[0];
        dvar->current.vector[1] = value.vector[1];
        dvar->current.vector[2] = value.vector[2];
        dvar->current.vector[3] = value.vector[3];
        dvar->latched.vector[0] = value.vector[0];
        dvar->latched.vector[1] = value.vector[1];
        dvar->latched.vector[2] = value.vector[2];
        dvar->latched.vector[3] = value.vector[3];
        return;
    case DVAR_TYPE_STRING:
        if (value.string == dvar->current.string) {
            return;
        }

        oldCurrentString = dvar->current.string;
        oldLatchedString = dvar->latched.string;
        oldResetString = dvar->reset.string;
        freeOldCurrentString =
            oldCurrentString &&
            oldCurrentString != oldLatchedString &&
            oldCurrentString != oldResetString &&
            !Dvar_IsStaticValueString(oldCurrentString);

        dvar->current.string =
            Dvar_RebuildStringValue(value.string, dvar->latched.string, dvar->reset.string);

        if (oldLatchedString != dvar->current.string &&
            oldLatchedString != oldResetString &&
            !Dvar_IsStaticValueString(oldLatchedString)) {
            Z_FreeInternal((void *)oldLatchedString);
        }

        dvar->latched.string = dvar->current.string;

        if (freeOldCurrentString) {
            Z_FreeInternal((void *)oldCurrentString);
        }
        return;
    default:
        dvar->current = value;
        dvar->latched = value;
        return;
    }
}

void Dvar_SetInAutoExec(int inAutoExec)
{
    isLoadingAutoExecGlobalFlag = (byte)inAutoExec;
}

Bool Dvar_IsSystemActive(void)
{
    return isDvarSystemActive;
}

Bool Dvar_IsValidName(const char *dvarName)
{
    const char *s;
    char ch;

    if (!dvarName) {
        return 0;
    }

    s = dvarName;
    ch = *s;

    while (ch) {
        int isAlnum;
        int sch;

        sch = (signed char)ch;
        if (sch & ~0x7f) {
            isAlnum = ___maskrune(sch, 0x500) != 0;
        } else {
            isAlnum = (*(int *)(*(byte **)&__DefaultRuneLocale + 0x34 + sch * 4) & 0x500) != 0;
        }

        if (!isAlnum && ch != '_') {
            return 0;
        }

        ++s;
        ch = *s;
    }

    return 1;
}

const char *Dvar_EnumToString(const dvar_t *dvar)
{
    int sc;

    if (!dvar)
        return "";
    sc = dvar->domain.enumeration.stringCount;
    if (!sc)
        return "";
    return dvar->domain.enumeration.strings[dvar->current.integer];
}

static const char *Dvar_ValueToString_impl(const dvar_t *dvar, DvarValue value)
{
    switch (dvar->type) {
    case DVAR_TYPE_BOOL:
        return value.enabled ? "1" : "0";
    case DVAR_TYPE_FLOAT:
        return va("%g", value.value);
    case DVAR_TYPE_VEC2:
        return va("%g %g", value.vector[0], value.vector[1]);
    case DVAR_TYPE_VEC3:
        return va("%g %g %g", value.vector[0], value.vector[1], value.vector[2]);
    case DVAR_TYPE_VEC4:
        return va("%g %g %g %g", value.vector[0], value.vector[1], value.vector[2], value.vector[3]);
    case DVAR_TYPE_INT:
        return va("%i", value.integer);
    case DVAR_TYPE_ENUM:
        if (dvar->domain.enumeration.stringCount) {
            return dvar->domain.enumeration.strings[value.integer];
        }
        return "";
    case DVAR_TYPE_STRING:
        return va("%s", value.string);
    case DVAR_TYPE_COLOR:
        return va("%g %g %g %g",
                  (float)value.color[0] * (1.0f / 255.0f),
                  (float)(unsigned char)((unsigned int)value.integer >> 8) * (1.0f / 255.0f),
                  (float)value.color[2] * (1.0f / 255.0f),
                  (float)value.color[3] * (1.0f / 255.0f));
    default:
        return "";
    }
}

const char *__attribute_regparm__(2) Dvar_ValueToString(const dvar_t *dvar, DvarValue value)
{
    return Dvar_ValueToString_impl(dvar, value);
}

const char *Dvar_DisplayableValue(const dvar_t *dvar)
{
    return Dvar_ValueToString_impl(dvar, dvar->current);
}

const char *Dvar_DisplayableResetValue(const dvar_t *dvar)
{
    return Dvar_ValueToString_impl(dvar, dvar->reset);
}

const char *Dvar_DisplayableLatchedValue(const dvar_t *dvar)
{
    return Dvar_ValueToString_impl(dvar, dvar->latched);
}

static Bool __attribute_regparm__(3) Dvar_ValuesEqual(byte type, DvarValue val0, DvarValue val1)
{
    switch (type) {
    case DVAR_TYPE_BOOL:
        return val0.enabled == val1.enabled;
    case DVAR_TYPE_FLOAT:
        return val0.value == val1.value;
    case DVAR_TYPE_VEC2:
        return Dvar_VectorEqual(val0.vector, val1.vector, 2);
    case DVAR_TYPE_VEC3:
        return Dvar_VectorEqual(val0.vector, val1.vector, 3);
    case DVAR_TYPE_VEC4:
        return Dvar_VectorEqual(val0.vector, val1.vector, 4);
    case DVAR_TYPE_INT:
    case DVAR_TYPE_ENUM:
    case DVAR_TYPE_COLOR:
        return val0.integer == val1.integer;
    case DVAR_TYPE_STRING:
        return strcmp(val0.string, val1.string) == 0;
    default:
        return 0;
    }
}

Bool Dvar_HasLatchedValue(const dvar_t *dvar)
{
    return !Dvar_ValuesEqual(dvar->type, dvar->current, dvar->latched);
}

Bool Dvar_IsAtDefaultValue(const dvar_t *dvar)
{
    return Dvar_ValuesEqual(dvar->type, dvar->current, dvar->reset);
}

void Dvar_ClearModified(const dvar_t *dvar)
{
    ((dvar_t *)dvar)->modified = 0;
}

void Dvar_SetModified(const dvar_t *dvar)
{
    ((dvar_t *)dvar)->modified = 1;
}

void Dvar_AddFlags(const dvar_t *dvar, int flags)
{
    ((dvar_t *)dvar)->flags |= (unsigned short)flags;
}

void Dvar_ResetScriptInfo(void)
{
    dvar_t *dvar = dvarPool;
    while (dvar) {
        dvar->flags &= ~0x0400;
        dvar = (dvar_t *)(uintptr_t)dvar->next;
    }
}

const char *Dvar_IndexStringToEnumString(const dvar_t *dvar, const char *indexString)
{
    int i;
    int len;

    if (!dvar->domain.enumeration.stringCount) {
        return "";
    }

    len = (int)strlen(indexString);
    for (i = 0; i < len; ++i) {
        if (!isdigit(indexString[i])) {
            return "";
        }
    }

    return dvar->domain.enumeration.strings[atoi(indexString)];
}

static void __attribute_regparm__(2) Dvar_StringToColor(const char *string, byte color[4])
{
    float colorVec[4];

    colorVec[0] = 0.0f;
    colorVec[1] = 0.0f;
    colorVec[2] = 0.0f;
    colorVec[3] = 0.0f;
    sscanf(string, "%g %g %g %g", &colorVec[0], &colorVec[1], &colorVec[2], &colorVec[3]);

    color[0] = Dvar_FloatToColorComponent(colorVec[0]);
    color[1] = Dvar_FloatToColorComponent(colorVec[1]);
    color[2] = Dvar_FloatToColorComponent(colorVec[2]);
    color[3] = Dvar_FloatToColorComponent(colorVec[3]);
}

static DvarValue Dvar_StringToValue_impl(int type, DvarLimits domain, const char *string)
{
    DvarValue value;

    value.integer = 0;

    switch (type) {
    case DVAR_TYPE_BOOL:
        value.enabled = atoi(string) != 0;
        break;
    case DVAR_TYPE_FLOAT:
        value.value = (float)atof(string);
        break;
    case DVAR_TYPE_VEC2:
        value.vector = Dvar_AllocTempVector(2);
        sscanf(string, "%g %g", &value.vector[0], &value.vector[1]);
        break;
    case DVAR_TYPE_VEC3:
        value.vector = Dvar_AllocTempVector(3);
        sscanf(string, "%g %g %g", &value.vector[0], &value.vector[1], &value.vector[2]);
        break;
    case DVAR_TYPE_VEC4:
        value.vector = Dvar_AllocTempVector(4);
        sscanf(string, "%g %g %g %g", &value.vector[0], &value.vector[1], &value.vector[2], &value.vector[3]);
        break;
    case DVAR_TYPE_INT:
        value.integer = atoi(string);
        break;
    case DVAR_TYPE_ENUM:
        value.integer = Dvar_StringToEnumIndex(domain, string);
        break;
    case DVAR_TYPE_STRING:
        value.string = string;
        break;
    case DVAR_TYPE_COLOR:
        Dvar_StringToColor(string, value.color);
        break;
    default:
        value.integer = 0;
        break;
    }

    return value;
}

static DvarValue __attribute_regparm__(3) Dvar_StringToValue(
    int type, uint32_t domainLo, uint32_t domainHi, const char *string)
{
    return Dvar_StringToValue_impl(type, Dvar_UnpackLimits(domainLo, domainHi), string);
}

static inline __attribute__((always_inline)) const dvar_t *Dvar_FindVar_inl(const char *dvarName)
{
    dvar_t *var;

    for (var = dvarHashTable[Dvar_GenerateHashValue(dvarName)]; var; var = (dvar_t *)var->hashNext) {
        if (!I_stricmp(dvarName, var->name)) {
            return var;
        }
    }

    return 0;
}

static inline __attribute__((always_inline)) void Dvar_GetUnpackedColor_inl(const dvar_t *dvar, vec_t *expandedColor)
{
    byte color[4];

    if (dvar->type == DVAR_TYPE_COLOR) {
        *(int *)color = dvar->current.integer;
    } else {
        Dvar_StringToColor(dvar->current.string, color);
    }

    expandedColor[0] = (float)color[0] * (1.0f / 255.0f);
    expandedColor[1] = (float)color[1] * (1.0f / 255.0f);
    expandedColor[2] = (float)color[2] * (1.0f / 255.0f);
    expandedColor[3] = (float)color[3] * (1.0f / 255.0f);
}

void Dvar_GetUnpackedColor(const dvar_t *dvar, vec_t *expandedColor)
{
    Dvar_GetUnpackedColor_inl(dvar, expandedColor);
}

static void __attribute_regparm__(2) Dvar_SetLatchedValue(const dvar_t *dvar, DvarValue value)
{
    dvar_t *mutableDvar;

    mutableDvar = (dvar_t *)dvar;

    switch (dvar->type) {
    case DVAR_TYPE_VEC2:
        mutableDvar->latched.vector[0] = value.vector[0];
        mutableDvar->latched.vector[1] = value.vector[1];
        return;
    case DVAR_TYPE_VEC3:
        mutableDvar->latched.vector[0] = value.vector[0];
        mutableDvar->latched.vector[1] = value.vector[1];
        mutableDvar->latched.vector[2] = value.vector[2];
        return;
    case DVAR_TYPE_VEC4:
        mutableDvar->latched.vector[0] = value.vector[0];
        mutableDvar->latched.vector[1] = value.vector[1];
        mutableDvar->latched.vector[2] = value.vector[2];
        mutableDvar->latched.vector[3] = value.vector[3];
        return;
    case DVAR_TYPE_STRING:
        Dvar_FreeOwnedString(dvar->latched.string, dvar->current.string, dvar->reset.string);
        mutableDvar->latched.string =
            Dvar_RebuildStringValue(value.string, dvar->current.string, dvar->reset.string);
        return;
    default:
        mutableDvar->latched = value;
        return;
    }
}

void Dvar_Shutdown(void)
{
    dvar_t *dvar;

    for (dvar = sortedDvars; dvar; dvar = *(dvar_t **)&dvar->next) {
        if (dvar->type == DVAR_TYPE_STRING) {
            Dvar_FreeOwnedString(dvar->current.string, dvar->latched.string, dvar->reset.string);
            ((dvar_t *)dvar)->current.string = NULL;

            Dvar_FreeOwnedString(dvar->reset.string, dvar->latched.string, NULL);
            ((dvar_t *)dvar)->reset.string = NULL;

            Dvar_FreeOwnedString(dvar->latched.string, NULL, NULL);
            ((dvar_t *)dvar)->latched.string = NULL;
        } else if (dvar->type >= DVAR_TYPE_VEC2 && dvar->type <= DVAR_TYPE_VEC4) {
            Z_FreeInternal(dvar->current.vector);
        }

        if (dvar->flags & 0x4000) {
            Z_FreeInternal((void *)dvar->name);
        }
    }

    dvarCount = 0;
    sortedDvars = NULL;
    dvar_cheats = NULL;
    dvar_modifiedFlags = 0;
    isDvarSystemActive = 0;
    memset(dvarHashTable, 0, sizeof(dvarHashTable));
}

Bool Dvar_AnyLatchedValues(void)
{
    dvar_t *dvar;

    for (dvar = sortedDvars; dvar; dvar = *(dvar_t **)&dvar->next) {
        if (!Dvar_ValuesEqual(dvar->type, dvar->current, dvar->latched)) {
            return 1;
        }
    }

    return 0;
}

qboolean Com_SaveDvarsToBuffer(const char **dvarnames, int numDvars, char *buffer, int bufsize)
{
    int i;

    for (i = 0; i < numDvars; ++i) {
        const dvar_t *var;
        const char *valueString;
        int len;

        var = Dvar_FindVar(dvarnames[i]);
        valueString = Dvar_ValueToString_impl(var, var->current);
        len = snprintf(buffer, bufsize, "%s \"%s\"\n", var->name, valueString);
        if (len < 0) {
            return 0;
        }

        buffer += len;
        bufsize -= len;
    }

    return 1;
}

void Dvar_GetUnpackedColorByName(const char *dvarName, vec_t *expandedColor)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        const float *white = (const float *)imp_colorWhite;

        expandedColor[0] = white[0];
        expandedColor[1] = white[1];
        expandedColor[2] = white[2];
        expandedColor[3] = white[3];
        return;
    }

    Dvar_GetUnpackedColor_inl(var, expandedColor);
}

const dvar_t *Dvar_FindVar(const char *dvarName)
{
    dvar_t *var;

    for (var = dvarHashTable[Dvar_GenerateHashValue(dvarName)]; var; var = (dvar_t *)var->hashNext) {
        if (!I_stricmp(dvarName, var->name)) {
            return var;
        }
    }

    return 0;
}

const char *Dvar_GetVariantString(const char *dvarName)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        return "";
    }

    return Dvar_ValueToString(var, var->current);
}

const char *Dvar_GetString(const char *dvarName)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        return "";
    }

    return Dvar_ValueToString_impl(var, var->current);
}

float Dvar_GetFloat(const char *dvarName)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        return 0.0f;
    }

    if (var->type == DVAR_TYPE_FLOAT) {
        return var->current.value;
    }

    return (float)atof(Dvar_ValueToString_impl(var, var->current));
}

int Dvar_GetInt(const char *dvarName)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        return 0;
    }

    if (var->type == DVAR_TYPE_INT || var->type == DVAR_TYPE_ENUM) {
        return var->current.integer;
    }

    return atoi(Dvar_ValueToString_impl(var, var->current));
}

Bool Dvar_GetBool(const char *dvarName)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        return 0;
    }

    if (var->type == DVAR_TYPE_BOOL) {
        return var->current.enabled;
    }

    return atoi(Dvar_ValueToString_impl(var, var->current)) != 0;
}

void Dvar_UpdateEnumDomain(const dvar_t *dvar, const char **stringTable)
{
    dvar_t *mutableDvar;
    DvarValue value;
    int stringCount;

    stringCount = 0;
    if (stringTable) {
        while (stringTable[stringCount]) {
            ++stringCount;
        }
    }

    mutableDvar = (dvar_t *)dvar;
    mutableDvar->domain.enumeration.stringCount = stringCount;
    mutableDvar->domain.enumeration.strings = stringTable;

    value = mutableDvar->current;
    switch (mutableDvar->type) {
    case DVAR_TYPE_BOOL:
        value.integer = value.enabled != 0;
        break;
    case DVAR_TYPE_FLOAT:
        if (value.value < mutableDvar->domain.value.min) {
            value.value = mutableDvar->domain.value.min;
        } else if (value.value > mutableDvar->domain.value.max) {
            value.value = mutableDvar->domain.value.max;
        }
        break;
    case DVAR_TYPE_VEC2:
        Dvar_ClampVectorToDomain(value.vector, 2, mutableDvar->domain.vector.min, mutableDvar->domain.vector.max);
        break;
    case DVAR_TYPE_VEC3:
        Dvar_ClampVectorToDomain(value.vector, 3, mutableDvar->domain.vector.min, mutableDvar->domain.vector.max);
        break;
    case DVAR_TYPE_VEC4:
        Dvar_ClampVectorToDomain(value.vector, 4, mutableDvar->domain.vector.min, mutableDvar->domain.vector.max);
        break;
    case DVAR_TYPE_INT:
        if (value.integer < mutableDvar->domain.integer.min) {
            value.integer = mutableDvar->domain.integer.min;
        } else if (value.integer > mutableDvar->domain.integer.max) {
            value.integer = mutableDvar->domain.integer.max;
        }
        break;
    case DVAR_TYPE_ENUM:
        if (value.integer < 0 || value.integer >= mutableDvar->domain.enumeration.stringCount) {
            value.integer = mutableDvar->reset.integer;
        }
        break;
    default:
        break;
    }

    mutableDvar->current = value;
    mutableDvar->latched = value;
}

static const char *__attribute_regparm__(3) Dvar_DomainToString_Internal_impl(
    unsigned char type, DvarLimits domain, char *outBuffer, int outBufferLen, int *outLineCount)
{
    char *cursor;
    char *outBufferEnd;
    int i;
    int len;

    outBufferEnd = outBuffer + outBufferLen;

    if (outLineCount) {
        *outLineCount = 0;
    }

    cursor = outBuffer;

    switch (type) {
    case DVAR_TYPE_BOOL:
        snprintf(cursor, outBufferLen, "Domain is 0 or 1");
        break;
    case DVAR_TYPE_FLOAT:
        Dvar_DescribeFloatDomain(cursor, outBufferLen, domain.value.min, domain.value.max);
        break;
    case DVAR_TYPE_VEC2:
        Dvar_DescribeVectorDomain(cursor, outBufferLen, 2, domain.vector.min, domain.vector.max);
        break;
    case DVAR_TYPE_VEC3:
        Dvar_DescribeVectorDomain(cursor, outBufferLen, 3, domain.vector.min, domain.vector.max);
        break;
    case DVAR_TYPE_VEC4:
        Dvar_DescribeVectorDomain(cursor, outBufferLen, 4, domain.vector.min, domain.vector.max);
        break;
    case DVAR_TYPE_INT:
        Dvar_DescribeIntDomain(cursor, outBufferLen, domain.integer.min, domain.integer.max);
        break;
    case DVAR_TYPE_ENUM:
        len = snprintf(cursor, (int)(outBufferEnd - cursor), "Domain is one of the following:");
        if (len < 0) {
            break;
        }
        cursor += len;
        for (i = 0; i < domain.enumeration.stringCount; ++i) {
            len = snprintf(cursor, (int)(outBufferEnd - cursor), "\n  %2i: %s", i, domain.enumeration.strings[i]);
            if (len < 0) {
                break;
            }
            if (outLineCount) {
                ++*outLineCount;
            }
            cursor += len;
        }
        break;
    case DVAR_TYPE_STRING:
        snprintf(cursor, outBufferLen, "Domain is any text");
        break;
    case DVAR_TYPE_COLOR:
        snprintf(cursor, outBufferLen, "Domain is any 4-component color, in RGBA format");
        break;
    default:
        cursor[0] = '\0';
        break;
    }

    outBufferEnd[-1] = '\0';
    return cursor;
}

static const char *__attribute_regparm__(3) Dvar_DomainToString_Internal(
    int type, uint32_t domainLo, uint32_t domainHi, char *outBuffer, int outBufferLen, int *outLineCount)
{
    return Dvar_DomainToString_Internal_impl(
        type,
        Dvar_UnpackLimits(domainLo, domainHi),
        outBuffer,
        outBufferLen,
        outLineCount);
}

const char *Dvar_DomainToString_GetLines(unsigned char type, DvarLimits domain, char *outBuffer, int outBufferLen, int *outLineCount)
{
    return Dvar_DomainToString_InternalReg(type, domain, outBuffer, outBufferLen, outLineCount);
}

void Dvar_PrintDomain(unsigned char type, DvarLimits domain)
{
    char domainBuffer[0x400];

    Com_Printf("  %s\n", Dvar_DomainToString_InternalReg(type, domain, domainBuffer, sizeof(domainBuffer), NULL));
}

static void __attribute_regparm__(1) Dvar_PerformUnregistration(dvar_t *dvar)
{
    vec_t *vectorMem;
    const char *valueString;

    if (!(dvar->flags & 0x4000)) {
        dvar->flags |= 0x4000;
        dvar->name = CopyStringInternal(dvar->name);
    }

    if (dvar->type == DVAR_TYPE_STRING) {
        return;
    }

    vectorMem = NULL;
    if (dvar->type >= DVAR_TYPE_VEC2 && dvar->type <= DVAR_TYPE_VEC4) {
        vectorMem = dvar->current.vector;
    }

    valueString = Dvar_ValueToString_impl(dvar, dvar->latched);
    ((dvar_t *)dvar)->current.string = Dvar_CanonicalizeValueString(valueString);
    ((dvar_t *)dvar)->latched.string = dvar->current.string;

    valueString = Dvar_ValueToString_impl(dvar, dvar->reset);
    ((dvar_t *)dvar)->reset.string = Dvar_RebuildResetString(dvar, valueString);
    ((dvar_t *)dvar)->type = DVAR_TYPE_STRING;

    if (vectorMem) {
        Z_FreeInternal(vectorMem);
    }
}

void Dvar_UnregisterSystem(int sysFlag)
{
    dvar_t *dvar;

    for (dvar = sortedDvars; dvar; dvar = *(dvar_t **)&dvar->next) {
        if (!(dvar->flags & sysFlag) || (dvar->flags & 0x4000)) {
            continue;
        }

        dvar->flags &= (unsigned short)~sysFlag;
        if (!(dvar->flags & 0x7000)) {
            Dvar_PerformUnregistrationReg(dvar);
        }
    }
}

static void __attribute_regparm__(2) Dvar_UpdateResetValue(const dvar_t *dvar, DvarValue value)
{
    dvar_t *mutableDvar;
    const char *oldResetString;

    mutableDvar = (dvar_t *)dvar;

    switch (dvar->type) {
    case DVAR_TYPE_VEC2:
        mutableDvar->reset.vector[0] = value.vector[0];
        mutableDvar->reset.vector[1] = value.vector[1];
        return;
    case DVAR_TYPE_VEC3:
        mutableDvar->reset.vector[0] = value.vector[0];
        mutableDvar->reset.vector[1] = value.vector[1];
        mutableDvar->reset.vector[2] = value.vector[2];
        return;
    case DVAR_TYPE_VEC4:
        mutableDvar->reset.vector[0] = value.vector[0];
        mutableDvar->reset.vector[1] = value.vector[1];
        mutableDvar->reset.vector[2] = value.vector[2];
        mutableDvar->reset.vector[3] = value.vector[3];
        return;
    case DVAR_TYPE_STRING:
        oldResetString = dvar->reset.string;
        if (oldResetString != dvar->current.string &&
            oldResetString != dvar->latched.string &&
            !Dvar_IsStaticValueString(oldResetString)) {
            Z_FreeInternal((void *)oldResetString);
        }

        mutableDvar->reset.string = Dvar_RebuildResetString(dvar, value.string);
        return;
    default:
        mutableDvar->reset = value;
        return;
    }
}

static void __attribute_regparm__(3) Dvar_MakeExplicitType(
    dvar_t *dvar, const char *dvarName, int type, unsigned short flags, DvarValue resetValue, DvarLimits domain)
{
    DvarValue castValue;
    const char *oldCurrentString;
    const char *oldLatchedString;
    const char *oldResetString;
    const char *tempString;
    vec_t *vectorStorage;
    int components;

    dvar->type = (byte)type;
    dvar->domain = domain;

    if ((flags & DVAR_FLAG_READONLY) ||
        ((flags & DVAR_FLAG_CHEAT) && dvar_cheats && !dvar_cheats->current.enabled)) {
        castValue = resetValue;
    } else {
        castValue = Dvar_StringToValue_impl(dvar->type, dvar->domain, dvar->current.string);
        castValue = Dvar_ClampValueToDomain(type, castValue, resetValue, domain);
    }

    tempString = NULL;
    if (dvar->type == DVAR_TYPE_STRING && castValue.string) {
        tempString = CopyStringInternal(castValue.string);
        castValue.string = tempString;
    }

    oldCurrentString = dvar->current.string;
    oldLatchedString = dvar->latched.string;
    oldResetString = dvar->reset.string;

    if (dvar->type != DVAR_TYPE_STRING) {
        Dvar_FreeOwnedString(oldCurrentString, oldLatchedString, oldResetString);
        dvar->current.string = NULL;
    }

    Dvar_FreeOwnedString(oldLatchedString, oldCurrentString, oldResetString);
    dvar->latched.string = NULL;

    Dvar_FreeOwnedString(oldResetString, oldCurrentString, oldLatchedString);
    dvar->reset.string = NULL;

    if (dvar->type >= DVAR_TYPE_VEC2 && dvar->type <= DVAR_TYPE_VEC4) {
        components = dvar->type;
        vectorStorage = (vec_t *)Z_MallocInternal(components * 3 * sizeof(vec_t));
        dvar->current.vector = vectorStorage;
        dvar->latched.vector = vectorStorage + components;
        dvar->reset.vector = vectorStorage + components * 2;
    }

    Dvar_UpdateResetValue(dvar, resetValue);
    Dvar_UpdateValue(dvar, castValue);
    dvar_modifiedFlags |= flags;

    if (tempString) {
        Z_FreeInternal((void *)tempString);
    }

    (void)dvarName;
}

void Dvar_ChangeResetValue(const dvar_t *dvar, DvarValue value)
{
    Dvar_UpdateResetValueReg(dvar, value);
}

static void __attribute_regparm__(3) Dvar_SetVariant(
    const dvar_t *dvarConst, DvarValue value, DvarSetSource source)
{
    dvar_t *dvar;
    char domainBuffer[0x400];

    dvar = (dvar_t *)dvarConst;
    if (!dvar || !dvar->name || !dvar->name[0]) {
        return;
    }

    Com_PrintMessage(
        CON_CHANNEL_LOGFILEONLY,
        va("      dvar set %s %s\n", dvar->name, Dvar_ValueToString_impl(dvar, value)));

    if (!Dvar_ValueInDomain(dvar->type, value, dvar->domain)) {
        Com_Printf("'%s' is not a valid value for dvar '%s'\n", Dvar_ValueToString_impl(dvar, value), dvar->name);
        Com_Printf(
            "  %s\n",
            Dvar_DomainToString_Internal_impl(
                dvar->type,
                dvar->domain,
                domainBuffer,
                sizeof(domainBuffer),
                NULL));

        if (dvar->type == DVAR_TYPE_ENUM) {
            Dvar_SetVariant(dvar, dvar->reset, source);
        }
        return;
    }

    if (source == DVAR_SOURCE_EXTERNAL || source == DVAR_SOURCE_SCRIPT) {
        if (!Dvar_CanChangeValue(dvar, source)) {
            return;
        }

        if (dvar->flags & DVAR_FLAG_LATCH) {
            Dvar_SetLatchedValue(dvar, value);
            if (!Dvar_ValuesEqual(dvar->type, dvar->latched, dvar->current)) {
                Com_Printf("%s will be changed upon restarting.\n", dvar->name);
            }
            return;
        }
    }

    if (Dvar_ValuesEqual(dvar->type, dvar->current, value)) {
        Dvar_SetLatchedValue(dvar, dvar->current);
        return;
    }

    dvar_modifiedFlags |= dvar->flags;
    Dvar_UpdateValue(dvar, value);
    dvar->modified = 1;
}

void Dvar_SetCheatState(void)
{
    dvar_t *var;

    for (var = sortedDvars; var; var = *(dvar_t **)&var->next) {
        if (var->flags & 0x80) {
            Dvar_SetVariantReg(var, var->reset, DVAR_SOURCE_INTERNAL);
        }
    }
}

void Dvar_Reset(const dvar_t *dvar, DvarSetSource setSource)
{
    Dvar_SetVariantReg(dvar, dvar->reset, setSource);
}

static void __attribute_regparm__(3) Dvar_SetFromStringFromSource(
    const dvar_t *dvar, const char *string, DvarSetSource source)
{
    char buf[0x400];
    char domainBuffer[0x400];
    DvarValue value;

    I_strncpyz(buf, string, sizeof(buf));
    value = Dvar_StringToValue_impl(dvar->type, dvar->domain, buf);

    if (dvar->type == DVAR_TYPE_ENUM && value.integer == DVAR_INVALID_ENUM_INDEX) {
        Com_Printf("'%s' is not a valid value for dvar '%s'\n", buf, dvar->name);
        Com_Printf(
            "  %s\n",
            Dvar_DomainToString_Internal_impl(
                dvar->type,
                dvar->domain,
                domainBuffer,
                sizeof(domainBuffer),
                NULL));
        Dvar_SetVariantReg(dvar, dvar->reset, source);
        return;
    }

    Dvar_SetVariantReg(dvar, value, source);
}

void Dvar_SetFromString(const dvar_t *dvar, const char *string)
{
    Dvar_SetFromStringFromSourceReg(dvar, string, (DvarSetSource)0);
}

void Dvar_SetString(const dvar_t *dvar, const char *value)
{
    DvarValue newValue;
    char stringCopy[1024];

    if (dvar->type == DVAR_TYPE_STRING) {
        I_strncpyz(stringCopy, value, sizeof(stringCopy));
        newValue.string = stringCopy;
    } else {
        newValue = Dvar_StringToValueReg(dvar->type, dvar->domain, value);
    }

    Dvar_SetVariantReg(dvar, newValue, DVAR_SOURCE_INTERNAL);
}

void Dvar_SetColorFromSource(const dvar_t *dvar, float r, float g, float b, float a, DvarSetSource source)
{
    DvarValue newValue;
    DvarLimits domain;
    char string[128];

    Com_sprintf(string, sizeof(string), "%g %g %g %g", r, g, b, a);

    if (dvar->type == DVAR_TYPE_COLOR) {
        memset(&domain, 0, sizeof(domain));
        newValue = Dvar_StringToValueReg(DVAR_TYPE_COLOR, domain, string);
    } else {
        newValue = Dvar_StringToValueReg(dvar->type, dvar->domain, string);
    }

    Dvar_SetVariantReg(dvar, newValue, source);
}

void Dvar_SetColor(const dvar_t *dvar, float r, float g, float b, float a)
{
    Dvar_SetColorFromSource(dvar, r, g, b, a, DVAR_SOURCE_INTERNAL);
}

void Dvar_SetFloatFromSource(const dvar_t *dvar, float value, DvarSetSource source)
{
    DvarValue newValue;
    char string[32];

    if (dvar->type == DVAR_TYPE_FLOAT) {
        newValue.value = value;
    } else {
        Com_sprintf(string, sizeof(string), "%g", value);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, source);
}

void Dvar_SetIntFromSource(const dvar_t *dvar, int value, DvarSetSource source)
{
    DvarValue newValue;
    char string[32];

    if (dvar->type == DVAR_TYPE_INT || dvar->type == DVAR_TYPE_ENUM) {
        newValue.integer = value;
    } else {
        Com_sprintf(string, sizeof(string), "%i", value);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, source);
}

void Dvar_SetBoolFromSource(const dvar_t *dvar, unsigned char value, DvarSetSource source)
{
    DvarValue newValue;

    if (dvar->type == DVAR_TYPE_BOOL) {
        newValue.enabled = value;
    } else {
        newValue.string = value ? "1" : "0";
    }

    Dvar_SetVariantReg(dvar, newValue, source);
}

static void Dvar_InitNewDvarValue(dvar_t *dvar, int type, DvarValue value)
{
    if (type >= DVAR_TYPE_VEC2 && type <= DVAR_TYPE_VEC4) {
        int i;
        int components = type;
        vec_t *storage = (vec_t *)Z_MallocInternal(components * 3 * sizeof(vec_t));

        dvar->current.vector = storage;
        dvar->latched.vector = storage + components;
        dvar->reset.vector = storage + components * 2;

        for (i = 0; i < components; ++i) {
            dvar->current.vector[i] = value.vector[i];
            dvar->latched.vector[i] = value.vector[i];
            dvar->reset.vector[i] = value.vector[i];
        }
        return;
    }

    if (type == DVAR_TYPE_STRING) {
        const char *stringValue = Dvar_CanonicalizeValueString(value.string);
        dvar->current.string = stringValue;
        dvar->latched.string = stringValue;
        dvar->reset.string = stringValue;
        return;
    }

    dvar->current = value;
    dvar->latched = value;
    dvar->reset = value;
}

static void Dvar_InsertSorted(dvar_t *dvar)
{
    dvar_t **link;

    link = &sortedDvars;
    while (*link && stricmp(dvar->name, (*link)->name) >= 0) {
        link = (dvar_t **)&(*link)->next;
    }

    dvar->next = (intptr_t)(uintptr_t)*link;
    *link = dvar;
}

static const dvar_t *Dvar_RegisterVariant_impl(
    const char *dvarName, int type, unsigned int flags, DvarValue value, DvarLimits domain)
{
    unsigned int hash;
    unsigned short newFlags;
    dvar_t *dvar;

    if (!dvarName) {
        Com_Error(1, "\x15null name in generateHashValue");
        return NULL;
    }

    hash = Dvar_GenerateHashValue(dvarName);
    newFlags = (unsigned short)flags;

    for (dvar = dvarHashTable[hash]; dvar; dvar = (dvar_t *)(uintptr_t)dvar->hashNext) {
        if (I_stricmp(dvarName, dvar->name) == 0) {
            unsigned short oldFlags = dvar->flags;

            if ((oldFlags ^ newFlags) & 0x7000) {
                if (oldFlags & DVAR_FLAG_EXTERNAL) {
                    if (!(newFlags & DVAR_FLAG_EXTERNAL)) {
                        Dvar_PerformUnregistrationReg(dvar);
                        Z_FreeInternal((void *)dvar->name);
                        dvar->name = dvarName;
                        dvar->flags &= (unsigned short)~DVAR_FLAG_EXTERNAL;
                        Dvar_MakeExplicitType(dvar, dvarName, type, newFlags, value, domain);
                        oldFlags = dvar->flags;

                        if ((newFlags & 0x1000) && !(oldFlags & 0x1000)) {
                            dvar->name = dvarName;
                            if (dvar->type == DVAR_TYPE_ENUM) {
                                dvar->domain = domain;
                            }
                        }
                    }
                } else if (!(newFlags & DVAR_FLAG_EXTERNAL)) {
                    if ((newFlags & 0x1000) && !(oldFlags & 0x1000)) {
                        dvar->name = dvarName;
                        if (dvar->type == DVAR_TYPE_ENUM) {
                            dvar->domain = domain;
                        }
                    }
                }
            }

            if ((oldFlags & DVAR_FLAG_EXTERNAL) && type != dvar->type) {
                Dvar_MakeExplicitType(dvar, dvarName, type, newFlags, value, domain);
            }

            dvar->flags |= newFlags;

            if ((dvar->flags & DVAR_FLAG_CHEAT) && dvar_cheats && !dvar_cheats->current.enabled) {
                Dvar_SetVariantReg(dvar, dvar->reset, DVAR_SOURCE_INTERNAL);
                Dvar_SetLatchedValue(dvar, dvar->reset);
            }

            if (dvar->flags & DVAR_FLAG_LATCH) {
                Dvar_SetVariantReg(dvar, dvar->latched, DVAR_SOURCE_INTERNAL);
            }

            return dvar;
        }
    }

    if (dvarCount > DVAR_MAX_COUNT - 1) {
        Com_Error(0, "Can't create dvar '%s': %i dvars already exist", dvarName, DVAR_MAX_COUNT);
    }

    dvar = &dvarPool[dvarCount++];
    memset(dvar, 0, sizeof(*dvar));
    dvar->type = (byte)type;
    dvar->name = (newFlags & DVAR_FLAG_EXTERNAL) ? CopyStringInternal(dvarName) : dvarName;

    Dvar_InitNewDvarValue(dvar, type, value);
    dvar->domain = domain;
    dvar->modified = 0;
    Dvar_InsertSorted(dvar);
    dvar->flags = newFlags;
    dvar->hashNext = (intptr_t)(uintptr_t)dvarHashTable[hash];
    dvarHashTable[hash] = dvar;

    return dvar;
}

const dvar_t *Dvar_RegisterColor(const char *dvarName, float r, float g, float b, float a, int flags)
{
    DvarValue valueUnion;
    DvarLimits domain;

    valueUnion.color[0] = Dvar_FloatToColorComponent(r);
    valueUnion.color[1] = Dvar_FloatToColorComponent(g);
    valueUnion.color[2] = Dvar_FloatToColorComponent(b);
    valueUnion.color[3] = Dvar_FloatToColorComponent(a);
    domain.integer.min = 0;
    domain.integer.max = 0;

    return Dvar_RegisterVariantReg(dvarName, DVAR_TYPE_COLOR, flags, valueUnion, domain);
}

const dvar_t *Dvar_RegisterEnum(const char *dvarName, const char **valueList, int defaultIndex, unsigned short flags)
{
    DvarValue valueUnion;
    DvarLimits domain;
    int stringCount;

    valueUnion.integer = defaultIndex;
    domain.enumeration.stringCount = 0;
    domain.enumeration.strings = valueList;

    if (valueList) {
        for (stringCount = 0; valueList[stringCount]; ++stringCount) {
        }
        domain.enumeration.stringCount = stringCount;
    }

    return Dvar_RegisterVariantReg(dvarName, DVAR_TYPE_ENUM, flags, valueUnion, domain);
}

const dvar_t *Dvar_RegisterString(const char *dvarName, const char *value, unsigned short flags)
{
    DvarValue valueUnion;
    DvarLimits domain;

    valueUnion.string = value;
    memset(&domain, 0, sizeof(domain));

    return Dvar_RegisterVariantReg(dvarName, DVAR_TYPE_STRING, flags, valueUnion, domain);
}

const dvar_t *Dvar_RegisterVec4(const char *dvarName, float x, float y, float z, float w, float min, float max, unsigned short flags)
{
    DvarValue valueUnion;
    DvarLimits domain;
    vec4_t vector;

    vector[0] = x;
    vector[1] = y;
    vector[2] = z;
    vector[3] = w;
    valueUnion.vector = vector;
    domain.vector.min = min;
    domain.vector.max = max;

    return Dvar_RegisterVariantReg(dvarName, DVAR_TYPE_VEC4, flags, valueUnion, domain);
}

const dvar_t *Dvar_RegisterVec3(const char *dvarName, float x, float y, float z, float min, float max, unsigned short flags)
{
    DvarValue valueUnion;
    DvarLimits domain;
    vec3_t vector;

    vector[0] = x;
    vector[1] = y;
    vector[2] = z;
    valueUnion.vector = vector;
    domain.vector.min = min;
    domain.vector.max = max;

    return Dvar_RegisterVariantReg(dvarName, DVAR_TYPE_VEC3, flags, valueUnion, domain);
}

const dvar_t *Dvar_RegisterVec2(const char *dvarName, float x, float y, float min, float max, unsigned short flags)
{
    DvarValue valueUnion;
    DvarLimits domain;
    vec2_t vector;

    vector[0] = x;
    vector[1] = y;
    valueUnion.vector = vector;
    domain.vector.min = min;
    domain.vector.max = max;

    return Dvar_RegisterVariantReg(dvarName, DVAR_TYPE_VEC2, flags, valueUnion, domain);
}

const dvar_t *Dvar_RegisterFloat(const char *dvarName, float value, float min, float max, unsigned short flags)
{
    DvarValue valueUnion;
    DvarLimits domain;

    valueUnion.value = value;
    domain.value.min = min;
    domain.value.max = max;

    return Dvar_RegisterVariantReg(dvarName, DVAR_TYPE_FLOAT, flags, valueUnion, domain);
}

const dvar_t *Dvar_RegisterInt(const char *dvarName, int value, int min, int max, unsigned short flags)
{
    DvarValue valueUnion;
    DvarLimits domain;

    valueUnion.integer = value;
    domain.integer.min = min;
    domain.integer.max = max;

    return Dvar_RegisterVariantReg(dvarName, DVAR_TYPE_INT, flags, valueUnion, domain);
}

const dvar_t *Dvar_RegisterBool(const char *dvarName, unsigned char value, unsigned short flags)
{
    DvarValue valueUnion;
    DvarLimits domain;

    valueUnion.integer = value;
    memset(&domain, 0, sizeof(domain));

    return Dvar_RegisterVariantReg(dvarName, DVAR_TYPE_BOOL, flags, valueUnion, domain);
}

void Dvar_Init(void)
{
    DvarValue valueUnion;
    DvarLimits domain;

    isDvarSystemActive = 1;
    valueUnion.integer = 0;
    memset(&domain, 0, sizeof(domain));
    dvar_cheats = Dvar_RegisterVariantReg("sv_cheats", DVAR_TYPE_BOOL, 0x1018, valueUnion, domain);
    Dvar_AddCommands();
}

void Dvar_ResetDvars(unsigned int filter, DvarSetSource setSource)
{
    dvar_t *var;

    for (var = sortedDvars; var; var = *(dvar_t **)&var->next) {
        if (filter & var->flags) {
            Dvar_SetVariantReg(var, var->reset, setSource);
        }
    }
}

qboolean Com_LoadDvarsFromBuffer(const char **dvarnames, int numDvars, const char *buffer, const char *filename)
{
    byte wasRead[0x4000];
    const dvar_t *var;
    const char *token;
    int i;
    int numRead;

    memset(wasRead, 0, numDvars);

    for (i = 0; i < numDvars; ++i) {
        var = Dvar_FindVar(dvarnames[i]);
        if (var) {
            Dvar_SetVariantReg(var, var->reset, DVAR_SOURCE_INTERNAL);
        }
    }

    Com_BeginParseSession(filename);
    numRead = 0;

    for (token = Com_Parse(&buffer); *token; token = Com_Parse(&buffer)) {
        for (i = 0; i < numDvars; ++i) {
            if (!stricmp(token, dvarnames[i])) {
                break;
            }
        }

        if (i == numDvars) {
            Com_Printf("^3WARNING: unknown dvar '%s' in file '%s'\n", token, filename);
            Com_SkipRestOfLine(&buffer);
            continue;
        }

        var = Dvar_FindVar(dvarnames[i]);
        Dvar_SetFromStringFromSourceReg(var, Com_ParseOnLine(&buffer), DVAR_SOURCE_INTERNAL);

        if (!wasRead[i]) {
            wasRead[i] = 1;
            ++numRead;
        }
    }

    Com_EndParseSession();

    if (numRead == numDvars) {
        return 1;
    }

    Com_Printf("^1ERROR: the following dvars were not specified in file '%s'", filename);
    for (i = 0; i < numDvars; ++i) {
        if (!wasRead[i]) {
            Com_Printf("^1  %s\n", dvarnames[i]);
        }
    }

    return 0;
}

void Dvar_SetBool(const dvar_t *dvar, unsigned char value)
{
    DvarValue newValue;

    if (dvar->type == DVAR_TYPE_BOOL) {
        newValue.enabled = value;
    } else {
        newValue.string = value ? "1" : "0";
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

void Dvar_SetFloat(const dvar_t *dvar, float value)
{
    DvarValue newValue;
    char string[32];

    if (dvar->type == DVAR_TYPE_FLOAT) {
        newValue.value = value;
    } else {
        Com_sprintf(string, sizeof(string), "%g", value);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

void Dvar_SetInt(const dvar_t *dvar, int value)
{
    DvarValue newValue;
    char string[32];

    if (dvar->type == DVAR_TYPE_INT || dvar->type == DVAR_TYPE_ENUM) {
        newValue.integer = value;
    } else {
        Com_sprintf(string, sizeof(string), "%i", value);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

void Dvar_SetVec2(const dvar_t *dvar, float x, float y)
{
    DvarValue newValue;
    vec2_t vector;
    char string[64];

    if (dvar->type == DVAR_TYPE_VEC4) {
        vector[0] = x;
        vector[1] = y;
        newValue.vector = vector;
    } else {
        Com_sprintf(string, sizeof(string), "%g %g", x, y);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

void Dvar_SetVec3(const dvar_t *dvar, float x, float y, float z)
{
    DvarValue newValue;
    vec3_t vector;
    char string[96];

    if (dvar->type == DVAR_TYPE_VEC3) {
        vector[0] = x;
        vector[1] = y;
        vector[2] = z;
        newValue.vector = vector;
    } else {
        Com_sprintf(string, sizeof(string), "%g %g %g", x, y, z);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

void Dvar_SetVec4(const dvar_t *dvar, float x, float y, float z, float w)
{
    DvarValue newValue;
    vec4_t vector;
    char string[128];

    if (dvar->type == DVAR_TYPE_VEC4) {
        vector[0] = x;
        vector[1] = y;
        vector[2] = z;
        vector[3] = w;
        newValue.vector = vector;
    } else {
        Com_sprintf(string, sizeof(string), "%g %g %g %g", x, y, z, w);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

static inline __attribute__((always_inline)) const dvar_t *Dvar_RegisterString_inl(const char *dvarName, const char *value, unsigned short flags)
{
    DvarValue valueUnion;
    DvarLimits domain;

    valueUnion.string = value;
    memset(&domain, 0, sizeof(domain));

    return Dvar_RegisterVariantReg(dvarName, DVAR_TYPE_STRING, flags, valueUnion, domain);
}

static inline __attribute__((always_inline)) void Dvar_SetBool_inl(const dvar_t *dvar, unsigned char value)
{
    DvarValue newValue;

    if (dvar->type == DVAR_TYPE_BOOL) {
        newValue.enabled = value;
    } else {
        newValue.string = value ? "1" : "0";
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

static inline __attribute__((always_inline)) void Dvar_SetInt_inl(const dvar_t *dvar, int value)
{
    DvarValue newValue;
    char string[32];

    if (dvar->type == DVAR_TYPE_INT || dvar->type == DVAR_TYPE_ENUM) {
        newValue.integer = value;
    } else {
        Com_sprintf(string, sizeof(string), "%i", value);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

static inline __attribute__((always_inline)) void Dvar_SetFloat_inl(const dvar_t *dvar, float value)
{
    DvarValue newValue;
    char string[32];

    if (dvar->type == DVAR_TYPE_FLOAT) {
        newValue.value = value;
    } else {
        Com_sprintf(string, sizeof(string), "%g", value);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

static inline __attribute__((always_inline)) void Dvar_SetVec2_inl(const dvar_t *dvar, float x, float y)
{
    DvarValue newValue;
    vec2_t vector;
    char string[64];

    if (dvar->type == DVAR_TYPE_VEC4) {
        vector[0] = x;
        vector[1] = y;
        newValue.vector = vector;
    } else {
        Com_sprintf(string, sizeof(string), "%g %g", x, y);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

static inline __attribute__((always_inline)) void Dvar_SetVec3_inl(const dvar_t *dvar, float x, float y, float z)
{
    DvarValue newValue;
    vec3_t vector;
    char string[96];

    if (dvar->type == DVAR_TYPE_VEC3) {
        vector[0] = x;
        vector[1] = y;
        vector[2] = z;
        newValue.vector = vector;
    } else {
        Com_sprintf(string, sizeof(string), "%g %g %g", x, y, z);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

static inline __attribute__((always_inline)) void Dvar_SetVec4_inl(const dvar_t *dvar, float x, float y, float z, float w)
{
    DvarValue newValue;
    vec4_t vector;
    char string[128];

    if (dvar->type == DVAR_TYPE_VEC4) {
        vector[0] = x;
        vector[1] = y;
        vector[2] = z;
        vector[3] = w;
        newValue.vector = vector;
    } else {
        Com_sprintf(string, sizeof(string), "%g %g %g %g", x, y, z, w);
        newValue.string = string;
    }

    Dvar_SetVariantReg(dvar, newValue, (DvarSetSource)0);
}

static inline __attribute__((always_inline)) void Dvar_SetString_inl(const dvar_t *dvar, const char *value)
{
    DvarValue newValue;
    char stringCopy[1024];

    if (dvar->type == DVAR_TYPE_STRING) {
        I_strncpyz(stringCopy, value, sizeof(stringCopy));
        newValue.string = stringCopy;
    } else {
        newValue = Dvar_StringToValueReg(dvar->type, dvar->domain, value);
    }

    Dvar_SetVariantReg(dvar, newValue, DVAR_SOURCE_INTERNAL);
}

void Dvar_SetBoolByName(const char *dvarName, unsigned char value)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        Dvar_RegisterString_inl(dvarName, value ? "1" : "0", 0x4000);
        return;
    }

    Dvar_SetBool_inl(var, value);
}

void Dvar_SetIntByName(const char *dvarName, int value)
{
    const dvar_t *var;
    char string[32];

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        Com_sprintf(string, sizeof(string), "%i", value);
        Dvar_RegisterString_inl(dvarName, string, 0x4000);
        return;
    }

    Dvar_SetInt_inl(var, value);
}

void Dvar_SetFloatByName(const char *dvarName, float value)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        Dvar_RegisterString_inl(dvarName, va("%g", value), 0x4000);
        return;
    }

    Dvar_SetFloat_inl(var, value);
}

void Dvar_SetVec2ByName(const char *dvarName, float x, float y)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        Dvar_RegisterString_inl(dvarName, va("%g %g", x, y), 0x4000);
        return;
    }

    Dvar_SetVec2_inl(var, x, y);
}

void Dvar_SetVec3ByName(const char *dvarName, float x, float y, float z)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        Dvar_RegisterString_inl(dvarName, va("%g %g %g", x, y, z), 0x4000);
        return;
    }

    Dvar_SetVec3_inl(var, x, y, z);
}

void Dvar_SetVec4ByName(const char *dvarName, float x, float y, float z, float w)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        Dvar_RegisterString_inl(dvarName, va("%g %g %g %g", x, y, z, w), 0x4000);
        return;
    }

    Dvar_SetVec4_inl(var, x, y, z, w);
}

void Dvar_SetStringByName(const char *dvarName, const char *value)
{
    const dvar_t *var;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        Dvar_RegisterString_inl(dvarName, value, 0x4000);
        return;
    }

    Dvar_SetString_inl(var, value);
}

void Dvar_SetColorByName(const char *dvarName, int r, int g, int b, int a)
{
    const dvar_t *var;
    byte red;
    byte green;
    byte blue;
    byte alpha;

    red = (byte)r;
    green = (byte)g;
    blue = (byte)b;
    alpha = (byte)a;

    var = Dvar_FindVar_inl(dvarName);
    if (!var) {
        Dvar_RegisterString_inl(dvarName, va("%g %g %g %g", red, green, blue, alpha), 0x4000);
        return;
    }

    Dvar_SetColorFromSource(var, (float)red, (float)green, (float)blue, (float)alpha, DVAR_SOURCE_INTERNAL);
}

const dvar_t *Dvar_SetFromStringByNameFromSource(const char *dvarName, const char *string, DvarSetSource source)
{
    const dvar_t *dvar;

    dvar = Dvar_FindVar_inl(dvarName);
    if (dvar) {
        Dvar_SetFromStringFromSourceReg(dvar, string, source);
        return dvar;
    }

    return Dvar_RegisterString_inl(dvarName, string, 0x4000);
}

void Dvar_SetCommand(const char *dvarName, const char *string)
{
    const dvar_t *dvar;

    dvar = Dvar_SetFromStringByNameFromSource(dvarName, string, (DvarSetSource)1);
    if (!dvar || !isLoadingAutoExecGlobalFlag) {
        return;
    }

    ((dvar_t *)dvar)->flags |= 0x8000;
    Dvar_UpdateResetValueReg(dvar, dvar->current);
}

void Dvar_SetFromStringByName(const char *dvarName, const char *string)
{
    Dvar_SetFromStringByNameFromSource(dvarName, string, (DvarSetSource)0);   /* C++ needs explicit int->enum */
}
