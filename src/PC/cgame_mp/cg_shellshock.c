#include "common_types.h"
#include <math.h>
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


extern byte *_dvar_shellshock_fadein;
extern byte *_dvar_shellshock_fadeout;
extern byte *_dvar_shellshock_screenblend;
extern byte *_dvar_shellshock_screentype;
extern byte *_dvar_shellshock_screenenabled;
extern byte *_dvar_shellshock_soundfadein;
extern byte *_dvar_shellshock_soundfadeout;
extern byte *_dvar_shellshock_loopfadein;
extern byte *_dvar_shellshock_loopfadeout;
extern byte *_dvar_shellshock_looptype;
extern byte *_dvar_shellshock_sounddrylevellooptype;
extern byte *_dvar_shellshock_soundwetlevellooptype;
extern byte *_dvar_shellshock_soundloopsilent;
extern byte *_dvar_shellshock_viewkickfadein;
extern byte *_dvar_shellshock_viewkickperiod;
extern byte *_dvar_shellshock_viewkickradius;
extern byte *_dvar_shellshock_viewkickpitch;
extern byte *_dvar_shellshock_viewkickyaw;
extern byte *_dvar_shellshock_soundroomtype;
extern byte *_dvar_shellshock_sounddrylevel;
extern byte *_dvar_shellshock_soundwetlevel;
extern byte *_dvar_shellshock_soundmodenddelay;
extern byte *_dvar_shellshock_soundendduration;
extern byte *_dvar_shellshock_soundfade;
extern byte *_dvar_shellshock_mouseenable;
extern byte *_dvar_shellshock_mousefadein;
extern byte *_dvar_shellshock_mouseturnrate;
extern byte *_dvar_shellshock_mousereducemax;
extern byte *_dvar_shellshock_mousesensitivity;
extern const dvar_t *cg_shock_viewKickFadeTime;

extern int _snd_local_listener;

extern const vec_t Vec3Normalize(vec_t *v);
extern void Vec3Cross(const vec_t *v1, const vec_t *v2, vec_t *out);
extern void AxisCopy(vec3_t *in, vec3_t *out);
extern void MatrixMultiply(const float (*in1)[3], const float (*in2)[3], float (*out)[3]);
extern qboolean Com_SaveDvarsToBuffer(const char **dvar_names, int count, char *buf, int bufsize);
extern qboolean Com_LoadDvarsFromBuffer(const char **dvar_names, int count, const char *buf, const char *path);
extern const char *va(const char *format, ...);
extern int FS_FOpenFileByMode(const char *name, int *fh, int mode);
extern int FS_Write(const void *buf, int len, int fh);
extern int FS_Read(void *buf, int len, int fh);
extern void FS_FCloseFile(fileHandle_t fh);
extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern void Com_Printf(const char *fmt, ...);
extern void CL_SaveScreen(void);
extern void CL_BlendSavedScreen(int blend);
extern const char * Dvar_EnumToString(const dvar_t *dvar);
extern void SND_SetChannelVolumes(int priority, const float *channelvolume, int fademsec);
extern void SND_SetEnvironmentEffects(int type, const char *name, float drylevel, float wetlevel, int flags);
extern void SND_DeactivateChannelVolumes(int type, int flags);
extern void SND_DeactivateEnvironmentEffects(int type, int flags);
extern snd_alias_t * CL_PickSoundAlias(const char *aliasname);
extern void SND_PlayBlendedSoundAliases(void *alias0, void *alias1, float fade, int channel, int entity, int flags, int loop);
extern void SND_PlaySoundAlias(void *alias, int channel, int entity, int duration, int loop);
extern void CL_CapTurnRate(float min_rate, float max_rate);
extern void CL_SetUserCmdInShellshock(int inShellshock);

static inline float dvar_get_float(byte *dvar_pp)
{
    dvar_t *dvar = *(dvar_t **)dvar_pp;
    return dvar->current.value;
}

static inline int dvar_get_int(byte *dvar_pp)
{
    dvar_t *dvar = *(dvar_t **)dvar_pp;
    return dvar->current.integer;
}

static inline int dvar_get_bool(byte *dvar_pp)
{
    dvar_t *dvar = *(dvar_t **)dvar_pp;
    return (int)dvar->current.enabled;
}

static inline dvar_t *dvar_get_ptr(byte *dvar_pp)
{
    return *(dvar_t **)dvar_pp;
}

static inline int float_seconds_to_ms(float val)
{
    return (int)floorf(val * 1000.0f + 0.5f);
}

/* migrated from the ILP32 data blob to typed C (x64 port Stage 2). */
const char *cg_shock_dvar_names[30] = {
    "cg_shock_screenBlendTime",
    "cg_shock_screenBlendFadeTime",
    "cg_shock_viewKickPeriod",
    "cg_shock_viewKickRadius",
    "cg_shock_viewKickFadeTime",
    "cg_shock_sound",
    "cg_shock_soundFadeInTime",
    "cg_shock_soundFadeOutTime",
    "cg_shock_soundLoopFadeTime",
    "cg_shock_soundLoopEndDelay",
    "cg_shock_soundRoomType",
    "cg_shock_soundDryLevel",
    "cg_shock_soundWetLevel",
    "cg_shock_soundModEndDelay",
    "cg_shock_volume_auto",
    "cg_shock_volume_auto2d",
    "cg_shock_volume_menu",
    "cg_shock_volume_weapon",
    "cg_shock_volume_voice",
    "cg_shock_volume_item",
    "cg_shock_volume_body",
    "cg_shock_volume_local",
    "cg_shock_volume_music",
    "cg_shock_volume_announcer",
    "cg_shock_volume_shellshock",
    "cg_shock_mouse",
    "cg_shock_mouse_maxpitchspeed",
    "cg_shock_mouse_maxyawspeed",
    "cg_shock_mouse_sensitivityscale",
    "cg_shock_mouse_fadeTime",
};

/* the shellshock view kick's noise: 128 points and the first 3 again, for
   the cubic between them; the 1.3 binary's table at 0x59e1e0 */
static const vec2_t cg_perturbations[131] = {
    { -0.563551009f, -0.0044300002f }, { -0.282061994f, -0.757933021f }, { 0.413046986f, 0.244246006f },
    { 0.527894974f, -0.72389698f }, { -0.329777002f, 0.669799984f }, { -0.394248009f, -0.763090014f },
    { 0.126206994f, 0.497768998f }, { 0.00498599978f, -0.01413f }, { 0.55991298f, 0.112824999f },
    { -0.333088994f, -0.573283017f }, { 0.335404009f, -0.107175998f }, { -0.569060028f, -0.213140994f },
    { -0.166676f, 0.785084009f }, { 0.299591988f, 0.0375929996f }, { -0.516866982f, 0.510758996f },
    { 0.138008997f, 0.0348230004f }, { -0.156167001f, 0.829047978f }, { -0.999458015f, 0.0203169994f },
    { 0.30002901f, 0.252943993f }, { 0.0302150007f, -0.295731992f }, { -0.917361975f, -0.0507109985f },
    { 0.0441769995f, -0.269288987f }, { 0.588424027f, 0.362576991f }, { -0.379913002f, 0.619213998f },
    { 0.204431996f, -0.0194230005f }, { 0.018499f, 0.468079001f }, { 0.916186988f, -0.247878f },
    { 0.00379900006f, 0.108209997f }, { 0.0573629998f, 0.606239974f }, { 0.324595004f, 0.158732995f },
    { -0.130529001f, -0.183387995f }, { 0.715672016f, -0.363858014f }, { 0.984257996f, 0.106095999f },
    { -0.00331300008f, 0.34553501f }, { -0.320351005f, -0.573935986f }, { 0.0634550005f, -0.00323899998f },
    { -0.570173025f, -0.759312987f }, { 0.106455997f, 0.283726007f }, { -0.668163002f, 0.142388001f },
    { -0.501119018f, -0.720005989f }, { -0.253280997f, 0.524031997f }, { -0.0640840009f, -0.165942997f },
    { -0.194672003f, 0.43355f }, { -0.281800002f, -0.417744011f }, { 0.0457860008f, 0.40298599f },
    { 0.105063997f, -0.558937013f }, { 0.312243998f, 0.688318014f }, { -0.263294011f, -0.256810993f },
    { 0.659186006f, 0.070671998f }, { 0.0936250016f, -0.0468120016f }, { -0.875020027f, 0.288509011f },
    { 0.329358995f, 0.105940998f }, { -0.181309f, 0.259864986f }, { 0.261597008f, -0.0740699992f },
    { -0.29608199f, 0.0318580009f }, { 0.0385840014f, 0.565946996f }, { -0.253444999f, -0.71786499f },
    { -0.211835995f, 0.336521f }, { 0.89012301f, 0.00494999997f }, { -0.97982502f, -0.170790002f },
    { 0.0453459993f, 0.0222399998f }, { -0.345795989f, 0.522711992f }, { 0.108525001f, 0.165424004f },
    { -0.572795987f, -0.473399013f }, { 0.368604988f, -0.865844011f }, { 0.0755710006f, -0.327702999f },
    { -0.466352999f, -0.565594018f }, { -0.358837008f, 0.610301971f }, { 0.603883982f, 0.440023005f },
    { 0.00246499991f, -0.144448996f }, { -0.294914991f, 0.799969971f }, { -0.0283470005f, -0.112071f },
    { -0.00947200041f, 0.686061025f }, { 0.0711499974f, 0.0199100003f }, { 0.962689996f, 0.0249259993f },
    { 0.309208006f, 0.87154901f }, { -0.123782001f, -0.31230101f }, { -0.433055013f, -0.895981014f },
    { 0.962495029f, -0.263776988f }, { -0.511460006f, -0.359477997f }, { -0.044013001f, 0.0202099998f },
    { -0.109339997f, -0.761229992f }, { 0.171002999f, -0.107460998f }, { 0.418911994f, 0.435294002f },
    { 0.444940001f, -0.139642999f }, { 0.518573999f, 0.365965009f }, { -0.506996989f, 0.655596972f },
    { 0.510524988f, 0.508961022f }, { -0.296173006f, -0.67583698f }, { 0.851332009f, 0.307191998f },
    { -0.0084739998f, -0.188743994f }, { 0.552703023f, 0.427085996f }, { 0.0803340003f, -0.00280499994f },
    { 0.0356560014f, 0.610991001f }, { 0.770592988f, 0.398874015f }, { -0.522136986f, 0.32436201f },
    { 0.00604500016f, 0.0427879989f }, { 0.482455999f, 0.848994017f }, { 0.226058006f, -0.522367001f },
    { -0.674606025f, -0.547814012f }, { -0.441998005f, 0.598839998f }, { -0.183956996f, -0.270233989f },
    { 0.518850029f, 0.634945989f }, { 0.430386007f, 0.125257f }, { -0.185496002f, -0.264459014f },
    { 0.0236900002f, 0.312977999f }, { -0.444287002f, 0.849928021f }, { 0.291978002f, -0.897678971f },
    { -0.0458259992f, -0.0471279994f }, { -0.114246003f, 0.51197499f }, { 0.738133013f, 0.607667029f },
    { -0.786889017f, -0.384056985f }, { 0.182992995f, 0.265085995f }, { -0.399450004f, -0.30903101f },
    { -0.482894987f, 0.265662014f }, { 0.0596709996f, 0.0977599993f }, { 0.793174028f, -0.0159719996f },
    { 0.201657996f, 0.492444992f }, { -0.707370996f, -0.0261899997f }, { -0.320881993f, 0.372280002f },
    { 0.572812974f, -0.537254989f }, { 0.337619007f, 0.116292998f }, { -0.606536984f, 0.173372999f },
    { -0.166593f, -0.335112005f }, { -0.583993018f, 0.182916f }, { -0.573518991f, -0.623347998f },
    { -0.39270699f, 0.449474007f }, { 0.151473999f, 0.840400994f }, { -0.563551009f, -0.0044300002f },
    { -0.282061994f, -0.757933021f }, { 0.413046986f, 0.244246006f },
};

void CG_PerturbCamera(void);
qboolean CG_SaveShellShockDvars(const char *name);
qboolean CG_LoadShellShockDvars(const char *name);
qboolean CG_DrawShellShockSavedScreenBlend(const shellshock_parms_t *parms, int start, int duration);
void CG_SetShellShockParmsFromDvars(shellshock_parms_t *parms);
void CG_UpdateShellShock(const shellshock_parms_t *parms, int start, int duration);

/* COD2_TRACE_PERTURB: the shellshocked view axis's determinant (1 when it
   is still a rotation), for scripts/view_weapon_test.py */
static void CG_TracePerturb(float (*axis)[3])
{
    static int enabled = -1;
    float det;

    if (enabled < 0)
        enabled = getenv("COD2_TRACE_PERTURB") != NULL;
    if (!enabled)
        return;
    det = axis[0][0] * (axis[1][1] * axis[2][2] - axis[1][2] * axis[2][1])
          - axis[0][1] * (axis[1][0] * axis[2][2] - axis[1][2] * axis[2][0])
          + axis[0][2] * (axis[1][0] * axis[2][1] - axis[1][1] * axis[2][0]);
    fprintf(stderr, "[perturb] t=%d det=%.4f delta=%.4f %.4f\n", cg->time, det, cg->shellshock.viewDelta[0],
            cg->shellshock.viewDelta[1]);
}

/* 0x4d2320: the view turned toward (1, viewDelta) by the rotation whose
   rows are that direction, up x it and it x that */
void CG_PerturbCamera(void)
{
    cg_t *cgp = cg;
    float rot[3][3];
    float (*refdefAxis)[3];
    float axis[3][3];

    if (cgp->shellshock.viewDelta[0] == 0.0f && cgp->shellshock.viewDelta[1] == 0.0f) {
        return;
    }

    rot[0][0] = 1.0f;
    rot[0][1] = cgp->shellshock.viewDelta[0];
    rot[0][2] = cgp->shellshock.viewDelta[1];

    rot[2][0] = 0.0f;
    rot[2][1] = 0.0f;
    rot[2][2] = 1.0f;

    Vec3Normalize(rot[0]);
    Vec3Cross(rot[2], rot[0], rot[1]);
    Vec3Normalize(rot[1]);
    Vec3Cross(rot[0], rot[1], rot[2]);

    refdefAxis = (float (*)[3])cgp->refdef.viewaxis;
    AxisCopy(refdefAxis, axis);
    MatrixMultiply(rot, axis, refdefAxis);
    CG_TracePerturb(refdefAxis);
}

qboolean CG_SaveShellShockDvars(const char *name)
{
    char filebuf[0x10000];
    int fh;
    const char *path;

    if (!Com_SaveDvarsToBuffer(cg_shock_dvar_names, 29, filebuf, 0x10000)) {
        return 0;
    }

    path = va("shock/%s.shock", name);

    if (FS_FOpenFileByMode(path, &fh, 1) < 0) {
        return 0;
    }

    FS_Write(filebuf, (int)strlen(filebuf), fh);

    FS_FCloseFile(fh);
    return 1;
}

qboolean CG_LoadShellShockDvars(const char *name)
{
    const char *fullpath;
    int fh;
    int filesize;
    char *buf;
    qboolean result;

    fullpath = va("shock/%s.shock", name);

    filesize = FS_FOpenFileByMode(fullpath, &fh, 0);
    if (filesize < 0) {

        Com_Printf("^1couldn't open '%s'.\n", fullpath);

        filesize = FS_FOpenFileByMode("shock/default.shock", &fh, 0);
        if (filesize < 0) {

            Com_Printf("^1couldn't open 'shock/default.shock'.\n");
            filesize = 0;
            return filesize;
        }
    }

    buf = (char *)Z_MallocInternal(filesize + 1);

    FS_Read(buf, filesize, fh);

    buf[filesize] = '\0';

    FS_FCloseFile(fh);

    result = Com_LoadDvarsFromBuffer(cg_shock_dvar_names, 29, buf, fullpath);

    Z_FreeInternal(buf);

    return result;
}

qboolean CG_DrawShellShockSavedScreenBlend(const shellshock_parms_t *parms, int start, int duration)
{
    cg_t *cgp;
    int timeLeft;
    int blend;
    int fadeDuration;
    int fadeIn;

    if (start == 0 || duration <= 0) {

        cgp = cg;
        cgp->shellshock.hasSavedScreen = 0;
        return 0;
    }

    cgp = cg;
    timeLeft = start + duration - cgp->time;

    if (timeLeft <= 0) {

        cgp->shellshock.hasSavedScreen = 0;
        return 0;
    }

    fadeIn = parms->screenBlend.effectTime;
    fadeDuration = parms->screenBlend.fadeTime;

    blend = fadeIn;
    if (timeLeft < fadeDuration) {

        blend = (int)floorf((float)fadeIn * (float)timeLeft / (float)fadeDuration + 0.5f);
    }

    if (cgp->shellshock.hasSavedScreen != 0) {

        CL_BlendSavedScreen(blend);
    }

    CL_SaveScreen();

    cgp->shellshock.hasSavedScreen = 1;
    return 1;
}

void CG_SetShellShockParmsFromDvars(shellshock_parms_t *parms)
{
    float val;
    float epsilon;

    parms->screenBlend.fadeTime = float_seconds_to_ms(dvar_get_float(_dvar_shellshock_fadein));

    parms->screenBlend.effectTime = float_seconds_to_ms(dvar_get_float(_dvar_shellshock_fadeout));

    parms->view.fadeTime = 3000;

    val = dvar_get_float(_dvar_shellshock_screenblend);
    epsilon = 0.001f;
    if (epsilon - val >= 0.0f) {
        parms->view.kickRate = 1.0f;
    } else {
        parms->view.kickRate = epsilon / val;
    }

    parms->view.kickRadius = dvar_get_float(_dvar_shellshock_screentype);

    parms->sound.use = dvar_get_bool(_dvar_shellshock_screenenabled);

    parms->sound.fadeInTime = float_seconds_to_ms(dvar_get_float(_dvar_shellshock_soundfadein));

    parms->sound.fadeOutTime = float_seconds_to_ms(dvar_get_float(_dvar_shellshock_soundfadeout));

    parms->sound.loopFadeTime = float_seconds_to_ms(dvar_get_float(_dvar_shellshock_loopfadein));

    parms->sound.loopEndDelay = float_seconds_to_ms(dvar_get_float(_dvar_shellshock_loopfadeout));

    strncpy(parms->sound.roomtype, Dvar_EnumToString(dvar_get_ptr(_dvar_shellshock_looptype)), 0xf);
    parms->sound.roomtype[15] = 0;

    *(int *)&parms->sound.drylevel = dvar_get_int(_dvar_shellshock_sounddrylevellooptype);

    *(int *)&parms->sound.wetlevel = dvar_get_int(_dvar_shellshock_soundwetlevellooptype);

    parms->sound.modEndDelay = float_seconds_to_ms(dvar_get_float(_dvar_shellshock_soundloopsilent));

    val = dvar_get_float(_dvar_shellshock_viewkickfadein);
    parms->sound.channelvolume[0] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    val = dvar_get_float(_dvar_shellshock_viewkickperiod);
    parms->sound.channelvolume[1] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    val = dvar_get_float(_dvar_shellshock_viewkickradius);
    parms->sound.channelvolume[2] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    val = dvar_get_float(_dvar_shellshock_viewkickpitch);
    parms->sound.channelvolume[5] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    val = dvar_get_float(_dvar_shellshock_viewkickyaw);
    parms->sound.channelvolume[6] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    val = dvar_get_float(_dvar_shellshock_soundroomtype);
    parms->sound.channelvolume[4] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    val = dvar_get_float(_dvar_shellshock_sounddrylevel);
    parms->sound.channelvolume[3] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    val = dvar_get_float(_dvar_shellshock_soundwetlevel);
    parms->sound.channelvolume[7] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    val = dvar_get_float(_dvar_shellshock_soundmodenddelay);
    parms->sound.channelvolume[8] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    val = dvar_get_float(_dvar_shellshock_soundendduration);
    parms->sound.channelvolume[9] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    val = dvar_get_float(_dvar_shellshock_soundfade);
    parms->sound.channelvolume[10] = (val > 0.0f) ? ((val < 1.0f) ? val : 1.0f) : 0.0f;

    parms->mouse.use = dvar_get_bool(_dvar_shellshock_mouseenable);

    parms->mouse.fadeTime = float_seconds_to_ms(dvar_get_float(_dvar_shellshock_mousefadein));

    *(int *)&parms->mouse.maxPitchSpeed = dvar_get_int(_dvar_shellshock_mouseturnrate);

    *(int *)&parms->mouse.maxYawSpeed = dvar_get_int(_dvar_shellshock_mousereducemax);

    *(int *)&parms->mouse.sensitivity = dvar_get_int(_dvar_shellshock_mousesensitivity);
}

static inline __attribute__((always_inline)) void CG_DeactivateShellShockSound(cg_t *cgp)
{
    SND_DeactivateChannelVolumes(3, 0);
    SND_DeactivateEnvironmentEffects(2, 0);

    if (cgp->shellshock.loopEndTime != 0) {
        cgp->shellshock.loopEndTime = 0;

        {
            void *alias = CL_PickSoundAlias("shellshock_loop_end");
            SND_PlaySoundAlias(alias, 0x3ff, _snd_local_listener, 0, 1);
        }
    }
}

static inline __attribute__((always_inline)) void CG_ResetShellShockMotion(cg_t *cgp)
{

    cgp->shellshock.sensitivity = 1.0f;

    CL_CapTurnRate(0.0f, 0.0f);
}

void CG_UpdateShellShock(const shellshock_parms_t *parms, int start, int duration)
{
    cg_t *cgp;
    int time;
    int timeSinceStart;
    float fade;
    float channelvolume[11];
    int i;
    int loopTimeLeft;
    int loopFadeOut;

    cgp = cg;
    time = cgp->time - start;

    if (start == 0 || time < 0) {

        CG_DeactivateShellShockSound(cgp);

        CG_ResetShellShockMotion(cgp);

        cgp->shellshock.viewDelta[0] = 0.0f;
        cgp->shellshock.viewDelta[1] = 0.0f;

        CL_SetUserCmdInShellshock(0);
        return;
    }

    if (parms->sound.use == 0) {

        SND_DeactivateChannelVolumes(3, 0);
        SND_DeactivateEnvironmentEffects(2, 0);

        if (cgp->shellshock.loopEndTime != 0) {
            cgp->shellshock.loopEndTime = 0;
            {
                void *alias = CL_PickSoundAlias("shellshock_loop_end");
                SND_PlaySoundAlias(alias, 0x3ff, _snd_local_listener, 0, 1);
            }
        }
        goto check_mouse;
    }

    {

        int soundFadeOut = parms->sound.fadeOutTime;
        int soundFadeIn = parms->sound.fadeInTime;
        int totalWithFade = duration + parms->sound.modEndDelay + soundFadeOut;
        int soundTimeLeft = totalWithFade - time;

        if (soundFadeOut > soundTimeLeft) {

            if (time < soundFadeIn) {

                fade = (float)time / (float)soundFadeIn;
            } else {
                fade = 1.0f;
            }
        } else {

            fade = (float)soundTimeLeft / (float)soundFadeOut;
        }

        if (fade < 0.0f) {
            fade = 0.0f;
        }

        if (fade == 0.0f) {

            SND_DeactivateChannelVolumes(3, 0);
            SND_DeactivateEnvironmentEffects(2, 0);
        } else {

            for (i = 0; i < 11; i++) {
                channelvolume[i] = (parms->sound.channelvolume[i] - 1.0f) * fade + 1.0f;
            }

            SND_SetChannelVolumes(3, channelvolume, 0);

            SND_SetEnvironmentEffects(2, parms->sound.roomtype,
                                      fade * parms->sound.drylevel,
                                      fade * parms->sound.wetlevel, 0);
        }
    }

    {

        int loopFadeOutVal = parms->sound.loopEndDelay;
        loopTimeLeft = duration + loopFadeOutVal + parms->sound.loopFadeTime - time;

        if (loopTimeLeft > 0) {

            void *pAlias0 = CL_PickSoundAlias("shellshock_loop");
            void *pAlias1 = CL_PickSoundAlias("shellshock_loop2");
            int loopFadeIn = parms->sound.loopFadeTime;

            if (loopFadeIn == 0) {

            } else {

                fade = 1.0f - (float)loopTimeLeft / (float)loopFadeIn;

                if (fade < 0.0f)
                    fade = 0.0f;
            }

            SND_PlayBlendedSoundAliases(pAlias0, pAlias1, fade, 0x3ff, _snd_local_listener, 0, 1);

            loopFadeOutVal = parms->sound.loopEndDelay;
        }

        cgp = cg;
        {
            int cgTime = cgp->time;
            int loopEndTime = cgTime - time + duration + loopFadeOutVal;

            if (cgTime < loopEndTime) {

                if (cgp->shellshock.loopEndTime != 0) {

                    cgp->shellshock.loopEndTime = 0;

                    {
                        void *alias = CL_PickSoundAlias("shellshock_loop_end");
                        SND_PlaySoundAlias(alias, 0x3ff, _snd_local_listener, 0, 1);
                    }
                }
            } else {

                if (cgp->shellshock.loopEndTime != loopEndTime) {

                    cgp->shellshock.loopEndTime = loopEndTime;

                    {
                        int delayMs = cgTime - loopEndTime;
                        void *alias = CL_PickSoundAlias("shellshock_end");
                        SND_PlaySoundAlias(alias, 0x3ff, _snd_local_listener, delayMs, 1);
                    }
                }
            }
        }
    }

check_mouse: {

    int mouseEnabled = parms->mouse.use;
    int timeSinceStart2 = duration - time;

    if (mouseEnabled == 0) {

        cgp = cg;
        CG_ResetShellShockMotion(cgp);

        timeSinceStart2 = duration - time;
        goto check_viewkick;
    }

    {

        int mouseFadeIn = parms->mouse.fadeTime;
        if (timeSinceStart2 >= mouseFadeIn) {

            cgp = cg;
            cgp->shellshock.sensitivity = parms->mouse.sensitivity;

            CL_CapTurnRate(parms->mouse.maxPitchSpeed, parms->mouse.maxYawSpeed);
        } else if (timeSinceStart2 <= 0) {

            cgp = cg;
            CG_ResetShellShockMotion(cgp);
        } else {

            float t = (float)timeSinceStart2 / (float)mouseFadeIn;

            if (t == 1.0f) {

                cgp = cg;
                cgp->shellshock.sensitivity = parms->mouse.sensitivity;
                CL_CapTurnRate(parms->mouse.maxPitchSpeed, parms->mouse.maxYawSpeed);
            } else {

                cgp = cg;
                float sensitivity = parms->mouse.sensitivity;
                cgp->shellshock.sensitivity = (sensitivity - 1.0f) * t + 1.0f;

                float minRate = parms->mouse.maxPitchSpeed / t;
                float maxRate = parms->mouse.maxYawSpeed / t;
                CL_CapTurnRate(minRate, maxRate);
            }
        }
    }
}

check_viewkick:

    timeSinceStart = duration - time;
    if (timeSinceStart <= 0) {

        cgp = cg;
        cgp->shellshock.viewDelta[0] = 0.0f;
        cgp->shellshock.viewDelta[1] = 0.0f;
    } else {

        int viewKickTime = parms->view.fadeTime;
        float t;
        float amplitude;

        if (timeSinceStart >= viewKickTime) {
            t = 1.0f;
            amplitude = (-2.0f + 3.0f) * t * t;
        } else {

            t = (float)timeSinceStart / (float)viewKickTime;
            amplitude = (-2.0f * t + 3.0f) * t * t;
        }

        amplitude = amplitude * parms->view.kickRadius;

        {
            float phase = (float)time * parms->view.kickRate;
            int phaseFloor = (int)floorf(phase);
            float frac = phase - (float)phaseFloor;

            int idx = (phaseFloor + duration * 61) & 0x7f;
            int byteOff = idx * 8;
            byte *base = (byte *)cg_perturbations + byteOff;

            float x0 = *(float *)(base + 0);
            float x1 = *(float *)(base + 8);
            float x2 = *(float *)(base + 16);
            float x3 = *(float *)(base + 24);

            float coeff_a = x3 - x2 + x1 - x0;
            float c_x = x2 - x0;
            float b_x = x0 - x1 - coeff_a;
            b_x = (coeff_a * frac + b_x) * frac;
            c_x = (c_x + b_x) * frac;
            float resultX = x1 + c_x;

            cgp = cg;
            cgp->shellshock.viewDelta[0] = amplitude * resultX;

            float y0 = *(float *)(base + 4);
            float y1 = *(float *)(base + 12);
            float y2 = *(float *)(base + 20);
            float y3 = *(float *)(base + 28);

            float coeff_a_y = y3 - y2 + y1 - y0;
            float yxmm2 = y2 - y0;
            float yxmm1 = y0 - y1 - coeff_a_y;
            float ytmp = coeff_a_y * frac;
            yxmm1 += ytmp;
            yxmm1 *= frac;
            yxmm2 += yxmm1;
            yxmm2 *= frac;
            float resultY = y1 + yxmm2;

            cgp->shellshock.viewDelta[1] = amplitude * resultY;
        }
    }

    CL_SetUserCmdInShellshock(time < duration ? 1 : 0);
}
