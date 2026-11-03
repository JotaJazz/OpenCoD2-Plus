#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <string.h>
#include <stdio.h>

extern char *strlwr(char *s);

extern void PrimitiveTemplate_Shutdown(const PrimitiveTemplate *prim);
extern void PrimitiveTemplate_Init(const PrimitiveTemplate *prim);
extern Bool PrimitiveTemplate_ParsePrimitive(const PrimitiveTemplate *prim, GPGroup *group);

extern EffectTemplate *FX_TryRegisterEffect(const char *name);
extern void Com_Error(int level, const char *fmt, ...);
extern void Z_FreeInternal(void *ptr);
extern void *Z_MallocInternal(int size);
extern int FS_FOpenFileRead(const char *filename, fileHandle_t *f, qboolean uniqueFILE);
extern int FS_Read(void *buffer, int len, int f);
extern void FS_FCloseFile(fileHandle_t f);
extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void Hunk_FreeTempMemory(void *buf);
extern Bool GenericParser2_Parse(const GenericParser2 *parser, char **bufPtr, int cleanFirst, int writeable);
extern void *Hunk_AllocAlignInternal(int size, int align);
extern void FX_Print(const char *fmt, ...);
extern void Com_StripExtension(const char *in, char *out);
extern int strnicmp(const char *s1, const char *s2, size_t n);
extern int stricmp(const char *s1, const char *s2);

extern byte *fx_developer_check_ptr;

extern FxScheduler *theFxScheduler;
/* defaultEffect: migrated from the ILP32 data blob (4-byte NULL) to a typed
 * pointer so it re-lays-out per target (4 B x86 / 8 B x64). Assigned at runtime
 * by FX_InitSystem. (x64 port Stage 2.) */
EffectTemplate *defaultEffect = NULL;

void FX_CleanTemplate(EffectTemplate *fx);
void FX_CreateDefaultEffect(void);
void MediaHandles_Shutdown(const MediaHandles *_this);
void MediaHandles_AddHandle(const MediaHandles *_this, TMediaElement item);
static Bool BM_NOINLINE FX_ParseEffectFile(const char *name, GenericParser2 *parser);
EffectTemplate *FX_ParseEffect(GenericParser2 *parser, const char *name);
EffectTemplate *FX_RegisterEffect(const char *fileName);
void MediaHandles_AddEffect(const MediaHandles *_this, EffectTemplate *fx);

void FX_CleanTemplate(EffectTemplate *fx)
{
    int count;
    int j;

    count = fx->mPrimitiveCount;
    if (count > 0) {
        for (j = 0; j < count; j++) {
            PrimitiveTemplate_Shutdown(fx->mPrimitives[j]);
        }
    }
}

void FX_CreateDefaultEffect(void)
{
    defaultEffect = FX_TryRegisterEffect("misc/missing_fx");
    if (defaultEffect == NULL) {
        Com_Error(1, "^1ERROR: could not load default effect file '%s'", "misc/missing_fx");
    }
}

void MediaHandles_Shutdown(const MediaHandles *_this)
{
    MediaHandles *self = (MediaHandles *)_this;

    if (self->mMediaList.elements != NULL) {
        Z_FreeInternal(self->mMediaList.elements);
        self->mMediaList.elements = NULL;
        self->mMediaList.size = 0;
        self->mMediaList.maxSize = 0;
    }
}

void MediaHandles_AddHandle(const MediaHandles *_this, TMediaElement item)
{
    MediaHandles *self = (MediaHandles *)_this;
    unsigned short count;
    unsigned short capacity;
    void *newElements;

    count = self->mMediaList.size;
    capacity = self->mMediaList.maxSize;

    if (count == capacity) {
        if (count != 0) {
            self->mMediaList.maxSize = count * 2;
        } else {
            self->mMediaList.maxSize = 4;
        }
        newElements = Z_MallocInternal(self->mMediaList.maxSize * sizeof(void *));
        if (self->mMediaList.elements != NULL) {
            memcpy(newElements, self->mMediaList.elements, self->mMediaList.size * sizeof(void *));
            Z_FreeInternal(self->mMediaList.elements);
        }
        self->mMediaList.elements = (TMediaElement *)(newElements);
    }

    ((void **)self->mMediaList.elements)[self->mMediaList.size] = item.data;
    self->mMediaList.size += 1;
}

static Bool BM_NOINLINE FX_ParseEffectFile(const char *name, GenericParser2 *parser)
{
    char fileName[64];
    fileHandle_t fileHandle;
    int fileLength;
    char *buf;
    char *bufParse;

    sprintf(fileName, "fx/%s.efx", name);
    fileLength = FS_FOpenFileRead(fileName, &fileHandle, 0);
    if (fileLength < 0) {
        FX_Print("Effect file load failed: %s: file not found\n", fileName);
        return 0;
    }

    buf = (char *)Hunk_AllocateTempMemoryInternal(fileLength + 1);
    FS_Read(buf, fileLength, fileHandle);
    FS_FCloseFile(fileHandle);
    buf[fileLength] = '\0';

    bufParse = buf;
    GenericParser2_Parse(parser, &bufParse, 1, 0);
    Hunk_FreeTempMemory(buf);
    return 1;
}

EffectTemplate *FX_ParseEffect(GenericParser2 *parser, const char *name)
{
    EffectTemplate *effect;
    int currentPrimitiveIndex;
    GPGroup *primitiveGroup;
    char *grpName;
    int type;
    PrimitiveTemplate *prim;
    int count;

    if (!FX_ParseEffectFile(name, parser)) {
        return NULL;
    }

    effect = (EffectTemplate *)Hunk_AllocAlignInternal(sizeof(EffectTemplate), 4);

    effect->mEffectName = (char *)Hunk_AllocAlignInternal(strlen(name) + 1, 4);
    strcpy((char *)effect->mEffectName, name);

    primitiveGroup = parser->group.subGroupList;
    currentPrimitiveIndex = 0;
    if (primitiveGroup == NULL)
        return effect;

    while (primitiveGroup != NULL) {
        grpName = *(char **)primitiveGroup;

        if (stricmp(grpName, "particle") == 0) {
            type = 1;
        } else if (stricmp(grpName, "line") == 0) {
            type = 2;
        } else if (stricmp(grpName, "tail") == 0) {
            type = 3;
        } else if (stricmp(grpName, "cylinder") == 0) {
            type = 4;
        } else if (stricmp(grpName, "emitter") == 0) {
            type = 5;
        } else if (stricmp(grpName, "decal") == 0) {
            type = 6;
        } else if (stricmp(grpName, "orientedparticle") == 0) {
            type = 7;
        } else if (stricmp(grpName, "fxrunner") == 0) {
            type = 8;
        } else if (stricmp(grpName, "light") == 0) {
            type = 9;
        } else if (stricmp(grpName, "cameraShake") == 0) {
            type = 10;
        } else if (stricmp(grpName, "flash") == 0) {
            type = 11;
        } else if (stricmp(grpName, "cloud") == 0) {
            type = 12;
        } else {
            goto next_group;
        }

        prim = (PrimitiveTemplate *)Hunk_AllocAlignInternal(COD2_SIZEOF_1_3(PrimitiveTemplate, 0x2a4), 4);
        PrimitiveTemplate_Init(prim);
        prim->mType = (PrimType)type;
        prim->mParentPrimIndex = currentPrimitiveIndex;

        if (PrimitiveTemplate_ParsePrimitive(prim, primitiveGroup)) {
            if (prim->mType == 1 || prim->mType == 7 || prim->mType == 3) {
                if (*fx_developer_check_ptr != 0 && prim->mMediaHandles.mMediaList.size == 0) {
                    FX_Print("^1FX Error, no materials defined for primitive template of type '%i'\n", prim->mType);
                    PrimitiveTemplate_Shutdown(prim);
                    FX_CleanTemplate(effect);
                    FX_Print("^1FX Error, invalid primitive template for effect '%s'\n", name);
                    return NULL;
                }
            }

            count = effect->mPrimitiveCount;
            if (count >= 0x18) {
                FX_Print("FxScheduler:  Error--too many primitives in an effect\n");
            } else {
                effect->mPrimitives[count] = prim;
                effect->mPrimitiveCount++;
            }
        } else {
            PrimitiveTemplate_Shutdown(prim);
            FX_CleanTemplate(effect);
            FX_Print("^1FX Error while parsing segment type '%s'\n", *(char **)primitiveGroup);
            return NULL;
        }

    next_group:
        primitiveGroup = (GPGroup *)primitiveGroup->nextUnsorted;
        currentPrimitiveIndex++;
    }

    return effect;
}

EffectTemplate *FX_RegisterEffect(const char *fileName)
{
    char strippedFileName[64];
    EffectTemplate *result;

    if (*fileName == '/' || *fileName == '\\')
        fileName++;

    if (strnicmp(fileName, "fx/", 3) == 0) {
        Com_StripExtension(fileName + 3, strippedFileName);
        strlwr(strippedFileName);

        result = FX_TryRegisterEffect(strippedFileName);
        if (result == NULL)
            return defaultEffect;
        return result;
    }

    FX_Print("Effect file '%s' must start with fx/.\n", fileName);
    return defaultEffect;
}

void MediaHandles_AddEffect(const MediaHandles *_this, EffectTemplate *fx)
{
    MediaHandles *self = (MediaHandles *)_this;
    unsigned short count;
    unsigned short capacity;
    void *newElements;

    count = self->mMediaList.size;
    capacity = self->mMediaList.maxSize;

    if (count == capacity) {
        if (count != 0) {
            self->mMediaList.maxSize = count * 2;
        } else {
            self->mMediaList.maxSize = 4;
        }
        newElements = Z_MallocInternal(self->mMediaList.maxSize * sizeof(void *));
        if (self->mMediaList.elements != NULL) {
            memcpy(newElements, self->mMediaList.elements, self->mMediaList.size * sizeof(void *));
            Z_FreeInternal(self->mMediaList.elements);
        }
        self->mMediaList.elements = (TMediaElement *)(newElements);
    }

    ((void **)self->mMediaList.elements)[self->mMediaList.size] = (void *)fx;
    self->mMediaList.size += 1;
}

const unsigned char __ZZN17PrimitiveTemplate15ParseGroupFlagsEPKcPiE5C_148[608] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x69, 0x18, 0x06, 0x00, 0x8a, 0x18, 0x06, 0x00, 0x47, 0x1a, 0x06, 0x00, 0x1e, 0x1a, 0x06, 0x00,
    0xf5, 0x19, 0x06, 0x00, 0xcc, 0x19, 0x06, 0x00, 0xa3, 0x19, 0x06, 0x00, 0x7a, 0x19, 0x06, 0x00,
    0x51, 0x19, 0x06, 0x00, 0x28, 0x19, 0x06, 0x00, 0xff, 0x18, 0x06, 0x00, 0xd6, 0x18, 0x06, 0x00,
    0xb0, 0x18, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
