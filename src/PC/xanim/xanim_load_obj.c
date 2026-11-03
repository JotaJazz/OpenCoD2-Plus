#include "common_types.h"
#include "imports.h"
extern float floorf(float x);
extern float sqrtf(float x);

extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int FS_ReadFile(const char *path, void **buffer);
extern void FS_FreeFile(void *buffer);
extern void Com_Printf(const char *fmt, ...);
extern unsigned int SL_GetString_(const char *str, unsigned int user, int type);
extern unsigned int SL_GetStringOfLen(const char *str, unsigned int user, unsigned int len, int type);

XAnimParts *XAnimLoadFile(const char *name, Alloc_t Alloc);

/* The XAnim*Part data unions are pointer-aligned, so on x64 their indices/frame0 members sit at
   larger byte offsets than the x86 layout the original Alloc()/memcpy magic numbers (8/10/12/14/16)
   assumed. offsetof/sizeof yield the correct value on both ABIs. */
#define DQ_IDX  ((int)offsetof(XAnimDeltaPartQuat,  u.frames.indices))
#define DQ_F0   ((int)(offsetof(XAnimDeltaPartQuat,  u.frame0)  + sizeof(XQuat2)))
#define DT_IDX  ((int)offsetof(XAnimDeltaPartTrans, u.frames.indices))
#define DT_F0   ((int)(offsetof(XAnimDeltaPartTrans, u.frame0)  + sizeof(vec3_t)))
#define PQ_IDX  ((int)offsetof(XAnimPartQuat,  u.frames.indices))
#define PQ_F0   ((int)(offsetof(XAnimPartQuat,  u.frame0)  + sizeof(XQuat)))
#define PT_IDX  ((int)offsetof(XAnimPartTrans, u.frames.indices))
#define PT_F0   ((int)(offsetof(XAnimPartTrans, u.frame0)  + sizeof(vec3_t)))

static short int ConsumeShort(const char **pos)
{
    short int val = *(short int *)(*pos);
    *pos += 2;
    return val;
}

static unsigned short int ConsumeUShort(const char **pos)
{
    unsigned short int val = *(unsigned short int *)(*pos);
    *pos += 2;
    return val;
}

static int ConsumeInt(const char **pos)
{
    int val = *(int *)(*pos);
    *pos += 4;
    return val;
}

static unsigned char ConsumeByte(const char **pos)
{
    unsigned char val = *(unsigned char *)(*pos);
    *pos += 1;
    return val;
}

static short int QuatSqrt(int remainder)
{
    if (remainder > 0) {
        return (short int)floorf(sqrtf((float)remainder) + 0.5f);
    }
    return 0;
}

static void ConsumeQuat_C(const char **pos, short int *out)
{
    short int a, b, c;
    int sum, remainder;

    a = ConsumeShort(pos);
    out[0] = a;

    b = ConsumeShort(pos);
    out[1] = b;

    c = ConsumeShort(pos);
    out[2] = c;

    sum = (int)a * (int)a + (int)b * (int)b + (int)c * (int)c;
    remainder = 0x3fff0001 - sum;
    out[3] = QuatSqrt(remainder);
}

static void ConsumeQuat2(const char **pos, short int *out)
{
    short int a;
    int remainder;

    a = ConsumeShort(pos);
    out[0] = a;

    remainder = 0x3fff0001 - (int)a * (int)a;
    out[1] = QuatSqrt(remainder);
}

XAnimParts *XAnimLoadFile(const char *name, Alloc_t Alloc)
{
    char filename[64];
    void *buf;
    const char *pos;
    XAnimParts *parts;
    XAnimDeltaPart *deltaPart;
    short int quat[4];
    unsigned short int numBoneCount;
    unsigned short int numloopframes;
    unsigned short int numQuatIndices;
    int numNoteTracks;
    short int version;
    unsigned short int *boneNames;
    const char *flipQuatBits;
    char *simpleQuatBits;
    Bool bSmallIndices;
    Bool bLoop;
    Bool bDelta;
    short int sQ2;
    int i;

    if (Com_sprintf(filename, 64, "xanim/%s", name) < 0) {

        Com_Printf("^1ERROR: filename '%s' too long\n", filename);
        return (XAnimParts *)0;
    }

    {
        int fileLen = FS_ReadFile(filename, &buf);
        if (fileLen < 0) {

            Com_Printf("^1ERROR: xanim '%s' not found\n", name);
            return (XAnimParts *)0;
        }
        if (fileLen == 0) {

            Com_Printf("^1ERROR: xanim '%s' has 0 length\n", name);
            FS_FreeFile(buf);
            return (XAnimParts *)0;
        }
    }

    pos = (const char *)buf;

    version = ConsumeShort(&pos);

    if (version != 14) {

        FS_FreeFile(buf);
        Com_Printf("^1ERROR: xanim '%s' out of date (version %d, expecting %d)\n", name, (int)version, 14);
        return (XAnimParts *)0;
    }

    numNoteTracks = (unsigned short int)ConsumeUShort(&pos);
    numBoneCount = ConsumeUShort(&pos);
    sQ2 = (short int)numBoneCount;

    if (numBoneCount != 0) {
        boneNames = (unsigned short int *)Alloc(numBoneCount * 2);
    } else {
        boneNames = (unsigned short int *)0;
    }

    {
        unsigned char flags = ConsumeByte(&pos);
        bLoop = (Bool)(flags & 1);
        bDelta = (Bool)((flags >> 1) & 1);
    }

    sQ2 = ConsumeShort(&pos);

    parts = (XAnimParts *)Alloc((int)sizeof(XAnimParts));   /* was 0x2c (x86); XAnimParts is larger on x64 */

    parts->boneCount = (short int)numBoneCount;

    parts->names = boneNames;

    parts->framerate = (float)sQ2;

    parts->bLoop = bLoop;

    parts->bDelta = bDelta;

    if (bLoop) {
        numloopframes = (unsigned short int)(numNoteTracks + 1);
    } else {
        numloopframes = (unsigned short int)numNoteTracks;
    }

    bSmallIndices = (numloopframes <= 0x100) ? 1 : 0;

    parts->numframes = (unsigned short int)(numloopframes - 1);

    if (parts->numframes != 0) {
        parts->frequency = parts->framerate / (float)(unsigned short int)parts->numframes;
    } else {
        parts->frequency = 0.0f;
    }

    if (bDelta) {

        deltaPart = (XAnimDeltaPart *)Alloc((int)sizeof(XAnimDeltaPart));   /* was 8 (x86); 16B on x64 */
        parts->deltaPart = deltaPart;

        sQ2 = (short int)ConsumeUShort(&pos);

        if (sQ2 == 0) {

            deltaPart->quat = (XAnimDeltaPartQuat *)0;
        } else if (sQ2 == 1) {

            short int dq0;
            int rem;

            dq0 = ConsumeShort(&pos);
            quat[0] = dq0;
            rem = 0x3fff0001 - (int)dq0 * (int)dq0;
            quat[1] = QuatSqrt(rem);

            deltaPart->quat = (XAnimDeltaPartQuat *)Alloc(DQ_F0);
            deltaPart->quat->size = 0;

            deltaPart->quat->u.frame0[0] = quat[0];

            deltaPart->quat->u.frame0[1] = quat[1];
        } else {

            int numFrames;
            if (sQ2 >= (short int)numloopframes) {

                deltaPart->quat = (XAnimDeltaPartQuat *)Alloc(DQ_IDX);
                numFrames = (int)(unsigned short int)sQ2;
            } else if (bSmallIndices) {

                numFrames = (int)(unsigned short int)sQ2;
                deltaPart->quat = (XAnimDeltaPartQuat *)Alloc(DQ_IDX + 2 * numFrames);
                memcpy(((char *)deltaPart->quat) + DQ_IDX, pos, numFrames);
                pos += numFrames;
            } else {

                numFrames = (int)(unsigned short int)sQ2;
                {
                    int idxBytes = numFrames * 2;
                    deltaPart->quat = (XAnimDeltaPartQuat *)Alloc(DQ_IDX + idxBytes);
                    memcpy(((char *)deltaPart->quat) + DQ_IDX, pos, idxBytes);
                    pos += idxBytes;
                }
            }

            deltaPart->quat->size = (unsigned short int)(sQ2 - 1);
            deltaPart->quat->u.frames.frames = (XQuat2 *)Alloc(numFrames * 4);

            {
                int k;
                for (k = 0; k < numFrames; k++) {
                    short int v0, v1;
                    short int *framePtr;
                    v0 = ConsumeShort(&pos);
                    framePtr = (short int *)((char *)deltaPart->quat->u.frames.frames + k * 4);
                    framePtr[0] = v0;
                    v1 = QuatSqrt(0x3fff0001 - (int)v0 * (int)v0);
                    framePtr[1] = v1;
                }
            }

            {
                int k;
                for (k = 1; k < numFrames; k++) {
                    short int *cur = (short int *)((char *)deltaPart->quat->u.frames.frames + k * 4);
                    short int *prev = (short int *)((char *)deltaPart->quat->u.frames.frames + (k - 1) * 4);
                    int dot = (int)cur[0] * (int)prev[0] + (int)cur[1] * (int)prev[1];
                    if (dot < 0) {
                        cur[0] = -cur[0];
                        cur[1] = -cur[1];
                    }
                }
            }
        }

        sQ2 = (short int)ConsumeUShort(&pos);

        if (sQ2 == 0) {

            deltaPart->trans = (XAnimDeltaPartTrans *)0;
        } else if (sQ2 == 1) {

            int v0, v1, v2;
            const char *base = pos;
            v0 = *(int *)(base);
            pos += 4;
            v1 = *(int *)(pos);
            pos += 4;
            v2 = *(int *)(pos);
            pos += 4;

            deltaPart->trans = (XAnimDeltaPartTrans *)Alloc(DT_F0);
            deltaPart->trans->size = 0;

            {
                int *to = (int *)&deltaPart->trans->u.frame0[0];
                to[0] = v0;
                to[1] = v1;
                to[2] = v2;
            }
        } else {

            int numFrames;
            if (sQ2 >= (short int)numloopframes) {

                deltaPart->trans = (XAnimDeltaPartTrans *)Alloc(DT_IDX);
                numFrames = (int)(unsigned short int)sQ2;
            } else if (bSmallIndices) {

                numFrames = (int)(unsigned short int)sQ2;
                deltaPart->trans = (XAnimDeltaPartTrans *)Alloc(DT_IDX + 2 * numFrames);
                memcpy(((char *)deltaPart->trans) + DT_IDX, pos, numFrames);
                pos += numFrames;
            } else {

                numFrames = (int)(unsigned short int)sQ2;
                {
                    int idxBytes = numFrames * 2;
                    deltaPart->trans = (XAnimDeltaPartTrans *)Alloc(DT_IDX + idxBytes);
                    memcpy(((char *)deltaPart->trans) + DT_IDX, pos, idxBytes);
                    pos += idxBytes;
                }
            }

            deltaPart->trans->size = (unsigned short int)(sQ2 - 1);
            deltaPart->trans->u.frames.frames = (vec3_t *)Alloc(numFrames * 12);

            {
                int k;
                for (k = 0; k < numFrames; k++) {
                    int val0, val1, val2;
                    char *frameBase = (char *)deltaPart->trans->u.frames.frames + k * 12;
                    val0 = ConsumeInt(&pos);
                    *(int *)(frameBase + 0) = val0;
                    val1 = ConsumeInt(&pos);
                    *(int *)(frameBase + 4) = val1;
                    val2 = ConsumeInt(&pos);
                    *(int *)(frameBase + 8) = val2;
                }
            }
        }
    }

    if (numBoneCount != 0) {
        int flipSize = (int)(short int)numBoneCount - 1;
        flipSize = (flipSize >> 3) + 1;

        flipQuatBits = pos;
        pos += flipSize;

        simpleQuatBits = (char *)Alloc(flipSize);

        memcpy(simpleQuatBits, pos, flipSize);
        pos += flipSize;

        parts->simpleQuatBits = simpleQuatBits;

        parts->parts = (XAnimPart *)Alloc((int)(short int)numBoneCount * (int)sizeof(XAnimPart));   /* was *8 (x86); XAnimPart is 16B on x64 */
    } else {
        flipQuatBits = (const char *)0;
        simpleQuatBits = (char *)0;
    }

    if ((int)(short int)numBoneCount > 0) {
        const char *namePos;
        int j;
        namePos = pos;
        for (j = 0; j < (int)(short int)numBoneCount; j++) {
            int slen;
            if (j > 0) {
                namePos = pos;
            }
            slen = strlen(namePos) + 1;
            boneNames[j] = (unsigned short int)SL_GetStringOfLen(namePos, 0, slen, 9);
            pos = namePos + slen;
            namePos = pos;
        }

        for (i = 0; i < (int)(short int)numBoneCount; i++) {
            Bool flipQuat;
            Bool isSimpleQuat;
            XAnimPart *part;
            XAnimPartQuat *partQuatAlloc;
            int numQFrames;

            flipQuat = (flipQuatBits[i >> 3] >> (i & 7)) & 1;
            isSimpleQuat = (simpleQuatBits[i >> 3] >> (i & 7)) & 1;

            part = &parts->parts[i];

            numQuatIndices = ConsumeUShort(&pos);

            if (numQuatIndices == 0) {

                part->quat = (XAnimPartQuat *)0;
            } else if (numQuatIndices == 1) {

                if (isSimpleQuat) {

                    short int dq0;
                    int rem;

                    dq0 = ConsumeShort(&pos);
                    quat[0] = dq0;
                    rem = 0x3fff0001 - (int)dq0 * (int)dq0;
                    quat[1] = QuatSqrt(rem);

                    if (flipQuat) {
                        quat[0] = -quat[0];
                        quat[1] = -quat[1];
                    }

                    partQuatAlloc = (XAnimPartQuat *)Alloc((int)(offsetof(XAnimPartQuat, u.frame02) + sizeof(XQuat2)));   /* 8 on x86; frame02 is at 8 on x64 */
                    part->quat = partQuatAlloc;

                    partQuatAlloc->u.frame02[0] = quat[0];
                    partQuatAlloc->u.frame02[1] = quat[1];
                } else {

                    ConsumeQuat_C(&pos, quat);

                    if (flipQuat) {
                        quat[0] = -quat[0];
                        quat[1] = -quat[1];
                        quat[2] = -quat[2];
                        quat[3] = -quat[3];
                    }

                    partQuatAlloc = (XAnimPartQuat *)Alloc(PQ_F0);
                    part->quat = partQuatAlloc;

                    partQuatAlloc->u.frame0[0] = quat[0];
                    partQuatAlloc->u.frame0[1] = quat[1];
                    partQuatAlloc->u.frame0[2] = quat[2];
                    partQuatAlloc->u.frame0[3] = quat[3];
                }

                part->quat->size = 0;
            } else {

                numQFrames = (int)numQuatIndices;

                if (numQuatIndices >= numloopframes) {

                    if (isSimpleQuat) {
                        part->quat = (XAnimPartQuat *)Alloc(PQ_IDX);
                    } else {
                        part->quat = (XAnimPartQuat *)Alloc(PQ_IDX);
                    }
                    numQFrames = (int)numQuatIndices;
                } else if (bSmallIndices) {

                    part->quat = (XAnimPartQuat *)Alloc(PQ_IDX + 2 * numQFrames);
                    memcpy(((char *)part->quat) + PQ_IDX, pos, numQFrames);
                    pos += numQFrames;
                } else {

                    int idxBytes = numQFrames * 2;
                    part->quat = (XAnimPartQuat *)Alloc(PQ_IDX + idxBytes);
                    memcpy(((char *)part->quat) + PQ_IDX, pos, idxBytes);
                    pos += idxBytes;
                }

                if (isSimpleQuat) {

                    part->quat->u.frames.u.frames2 = (XQuat2 *)Alloc(numQFrames * 4);

                    ConsumeQuat2(&pos, quat);

                    if (flipQuat) {
                        quat[0] = -quat[0];
                        quat[1] = -quat[1];
                    }

                    part->quat->u.frames.u.frames2[0][0] = quat[0];
                    part->quat->u.frames.u.frames2[0][1] = quat[1];

                    if (numQFrames > 1) {
                        int k;
                        for (k = 1; k < numQFrames; k++) {
                            ConsumeQuat2(&pos, quat);
                            part->quat->u.frames.u.frames2[k][0] = quat[0];
                            part->quat->u.frames.u.frames2[k][1] = quat[1];
                        }

                        for (k = 1; k < numQFrames; k++) {
                            short int *cur = (short int *)&part->quat->u.frames.u.frames2[k];
                            short int *prev = (short int *)&part->quat->u.frames.u.frames2[k - 1];
                            int dot = (int)cur[0] * (int)prev[0] + (int)cur[1] * (int)prev[1];
                            if (dot < 0) {
                                cur[0] = -cur[0];
                                cur[1] = -cur[1];
                            }
                        }
                    }

                    part->quat->size = (unsigned short int)(numQuatIndices - 1);
                } else {

                    part->quat->u.frames.u.frames = (XQuat *)Alloc(numQFrames * 8);

                    ConsumeQuat_C(&pos, quat);

                    if (flipQuat) {
                        quat[0] = -quat[0];
                        quat[1] = -quat[1];
                        quat[2] = -quat[2];
                        quat[3] = -quat[3];
                    }

                    part->quat->u.frames.u.frames[0][0] = quat[0];
                    part->quat->u.frames.u.frames[0][1] = quat[1];
                    part->quat->u.frames.u.frames[0][2] = quat[2];
                    part->quat->u.frames.u.frames[0][3] = quat[3];

                    if (numQFrames > 1) {
                        int k;
                        for (k = 1; k < numQFrames; k++) {
                            ConsumeQuat_C(&pos, quat);
                            part->quat->u.frames.u.frames[k][0] = quat[0];
                            part->quat->u.frames.u.frames[k][1] = quat[1];
                            part->quat->u.frames.u.frames[k][2] = quat[2];
                            part->quat->u.frames.u.frames[k][3] = quat[3];
                        }

                        for (k = 1; k < numQFrames; k++) {
                            short int *cur = (short int *)part->quat->u.frames.u.frames[k];
                            short int *prev = (short int *)part->quat->u.frames.u.frames[k - 1];
                            int dot = (int)cur[0] * (int)prev[0] + (int)cur[1] * (int)prev[1] + (int)cur[2] * (int)prev[2] + (int)cur[3] * (int)prev[3];
                            if (dot < 0) {
                                cur[0] = -cur[0];
                                cur[1] = -cur[1];
                                cur[2] = -cur[2];
                                cur[3] = -cur[3];
                            }
                        }
                    }

                    part->quat->size = (unsigned short int)(numQuatIndices - 1);
                }
            }

            {
                unsigned short int numTransIndices;
                numTransIndices = ConsumeUShort(&pos);

                if (numTransIndices == 0) {

                    part->trans = (XAnimPartTrans *)0;
                } else if (numTransIndices == 1) {

                    int v0, v1, v2;
                    v0 = ConsumeInt(&pos);
                    v1 = ConsumeInt(&pos);
                    v2 = ConsumeInt(&pos);

                    part->trans = (XAnimPartTrans *)Alloc(PT_F0);
                    part->trans->size = 0;

                    {
                        int *to = (int *)&part->trans->u.frame0[0];
                        to[0] = v0;
                        to[1] = v1;
                        to[2] = v2;
                    }
                } else {

                    int numTFrames = (int)numTransIndices;

                    if (numTransIndices >= numloopframes) {

                        part->trans = (XAnimPartTrans *)Alloc(PT_IDX);
                    } else if (bSmallIndices) {

                        part->trans = (XAnimPartTrans *)Alloc(PT_IDX + 2 * numTFrames);
                        memcpy(((char *)part->trans) + PT_IDX, pos, numTFrames);
                        pos += numTFrames;
                    } else {

                        int idxBytes = numTFrames * 2;
                        part->trans = (XAnimPartTrans *)Alloc(PT_IDX + idxBytes);
                        memcpy(((char *)part->trans) + PT_IDX, pos, idxBytes);
                        pos += idxBytes;
                    }

                    part->trans->size = (unsigned short int)(numTransIndices - 1);
                    part->trans->u.frames.frames = (vec3_t *)Alloc(numTFrames * 12);

                    {
                        int k;
                        for (k = 0; k < numTFrames; k++) {
                            int val0, val1, val2;
                            int *frameBase = (int *)((char *)part->trans->u.frames.frames + k * 12);
                            val0 = ConsumeInt(&pos);
                            frameBase[0] = val0;
                            val1 = ConsumeInt(&pos);
                            frameBase[1] = val1;
                            val2 = ConsumeInt(&pos);
                            frameBase[2] = val2;
                        }
                    }
                }
            }
        }
    }

    {
        int numNT;
        XAnimNotifyInfo *notify;

        numNT = ConsumeByte(&pos);

        parts->notifyCount = (byte)(numNT + 1);

        notify = (XAnimNotifyInfo *)Alloc((int)(unsigned char)parts->notifyCount * 8);

        parts->notify = notify;

        if (numNT > 0) {
            int j;
            for (j = 0; j < numNT; j++) {

                notify[j].name = (ScriptString)SL_GetString_(pos, 0, 3);

                pos += strlen(pos) + 1;

                {
                    unsigned short int timeVal = ConsumeUShort(&pos);

                    if (parts->numframes != 0) {
                        notify[j].time = (float)(int)timeVal / (float)(unsigned short int)parts->numframes;
                    } else {
                        notify[j].time = 0.0f;
                    }
                }
            }
        }

        notify[numNT].name = (ScriptString)SL_GetString_("end", 0, 3);
        *(unsigned int *)&notify[numNT].time = 0x3f800000;
    }

    FS_FreeFile(buf);

    return parts;
}
