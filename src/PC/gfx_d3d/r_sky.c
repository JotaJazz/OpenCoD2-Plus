#include "common_types.h"
#include <math.h>
#include "imports.h"
/* dvar globals */
extern const dvar_t *sv_cheats;

extern const dvar_t *r_sunsprite_shader;
extern const dvar_t *r_sunsprite_size;
extern const dvar_t *r_sunflare_shader;
extern const dvar_t *r_sunflare_min_size;
extern const dvar_t *r_sunflare_min_angle;
extern const dvar_t *r_sunflare_max_size;
extern const dvar_t *r_sunflare_max_angle;
extern const dvar_t *r_sunflare_max_alpha;
extern const dvar_t *r_sunflare_fadein;
extern const dvar_t *r_sunflare_fadeout;
extern const dvar_t *r_sunblind_min_angle;
extern const dvar_t *r_sunblind_max_angle;
extern const dvar_t *r_sunblind_max_darken;
extern const dvar_t *r_sunblind_fadein;
extern const dvar_t *r_sunblind_fadeout;
extern const dvar_t *r_sunglare_min_angle;
extern const dvar_t *r_sunglare_max_angle;
extern const dvar_t *r_sunglare_max_lighten;
extern const dvar_t *r_sunglare_fadein;
extern const dvar_t *r_sunglare_fadeout;
/* migrated from the ILP32 data blob to typed C (x64 port Stage 2). */
const char *s_sundvars[21] = {
    "r_sunsprite_shader",
    "r_sunsprite_size",
    "r_sunflare_shader",
    "r_sunflare_min_size",
    "r_sunflare_min_angle",
    "r_sunflare_max_size",
    "r_sunflare_max_angle",
    "r_sunflare_max_alpha",
    "r_sunflare_fadein",
    "r_sunflare_fadeout",
    "r_sunblind_min_angle",
    "r_sunblind_max_angle",
    "r_sunblind_max_darken",
    "r_sunblind_fadein",
    "r_sunblind_fadeout",
    "r_sunglare_min_angle",
    "r_sunglare_max_angle",
    "r_sunglare_max_lighten",
    "r_sunglare_fadein",
    "r_sunglare_fadeout",
    "r_sun_fx_position",
};

extern const dvar_t *r_sun_fx_position;

extern refimport_t ri;
extern r_global_permanent_t rgp;

extern unsigned char *r_sunFlareState;

extern const dvar_t *Dvar_RegisterString(const char *dvarName, const char *value, int flags);
extern const dvar_t *Dvar_RegisterFloat(const char *dvarName, float value, float min, float max, int flags);
extern const dvar_t *Dvar_RegisterVec3(const char *dvarName, float x, float y, float z, float min, float max, int flags);
extern MaterialHandle Material_RegisterHandle(const char *name, int lightmapIndex, int imageTrack);
extern void R_LoadSunThroughDvars(const char *sunName, sunflare_t *sun);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern const char *va(const char *format, ...);

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif
#define DEG2RAD (M_PI / 180.0)

int R_GetSundvarsSize(void);
void R_RegisterSunDvars(void);
void R_Cmd_LoadSun(void);
void R_Cmd_SaveSun(void);
void R_FlushSun(void);
void R_SetSunFromDvars(sunflare_t *sun);

int R_GetSundvarsSize(void)
{
    return 0x15;
}

void R_RegisterSunDvars(void)
{
    r_sunsprite_shader = Dvar_RegisterString("r_sunsprite_shader", "sun", 0x2000);
    r_sunsprite_size = Dvar_RegisterFloat("r_sunsprite_size", 16.0f, 1.0f, 1000.0f, 0x2000);

    r_sunflare_shader = Dvar_RegisterString("r_sunflare_shader", "sun_flare", 0x2000);
    r_sunflare_min_size = Dvar_RegisterFloat("r_sunflare_min_size", 0.0f, 0.0f, 10000.0f, 0x2000);
    r_sunflare_min_angle = Dvar_RegisterFloat("r_sunflare_min_angle", 45.0f, 0.0f, 90.0f, 0x2000);
    r_sunflare_max_size = Dvar_RegisterFloat("r_sunflare_max_size", 2500.0f, 0.0f, 10000.0f, 0x2000);
    r_sunflare_max_angle = Dvar_RegisterFloat("r_sunflare_max_angle", 2.0f, 0.0f, 90.0f, 0x2000);
    r_sunflare_max_alpha = Dvar_RegisterFloat("r_sunflare_max_alpha", 1.0f, 0.0f, 1.0f, 0x2000);
    r_sunflare_fadein = Dvar_RegisterFloat("r_sunflare_fadein", 1.0f, 0.0f, 60.0f, 0x2000);
    r_sunflare_fadeout = Dvar_RegisterFloat("r_sunflare_fadeout", 1.0f, 0.0f, 60.0f, 0x2000);

    r_sunblind_min_angle = Dvar_RegisterFloat("r_sunblind_min_angle", 30.0f, 0.0f, 90.0f, 0x2000);
    r_sunblind_max_angle = Dvar_RegisterFloat("r_sunblind_max_angle", 5.0f, 0.0f, 90.0f, 0x2000);
    r_sunblind_max_darken = Dvar_RegisterFloat("r_sunblind_max_darken", 0.75f, 0.0f, 1.0f, 0x2000);
    r_sunblind_fadein = Dvar_RegisterFloat("r_sunblind_fadein", 0.5f, 0.0f, 60.0f, 0x2000);
    r_sunblind_fadeout = Dvar_RegisterFloat("r_sunblind_fadeout", 3.0f, 0.0f, 60.0f, 0x2000);

    r_sunglare_min_angle = Dvar_RegisterFloat("r_sunglare_min_angle", 30.0f, 0.0f, 90.0f, 0x2000);
    r_sunglare_max_angle = Dvar_RegisterFloat("r_sunglare_max_angle", 5.0f, 0.0f, 90.0f, 0x2000);
    r_sunglare_max_lighten = Dvar_RegisterFloat("r_sunglare_max_lighten", 0.75f, 0.0f, 1.0f, 0x2000);
    r_sunglare_fadein = Dvar_RegisterFloat("r_sunglare_fadein", 0.5f, 0.0f, 60.0f, 0x2000);
    r_sunglare_fadeout = Dvar_RegisterFloat("r_sunglare_fadeout", 3.0f, 0.0f, 60.0f, 0x2000);

    r_sun_fx_position = Dvar_RegisterVec3("r_sun_fx_position", 0.0f, 0.0f, 0.0f, -360.0f, 360.0f, 0x2000);
}

void R_Cmd_LoadSun(void)
{
    int argc;
    const char *sunName;
    GfxWorld *world;
    refimport_t *r = (refimport_t *)&ri;

    argc = r->Cmd_Argc();
    if (argc != 2) {
        r->Printf(0, "usage: loadsun <name>\n");
        return;
    }

    if ((sv_cheats)->current.enabled == 0) {
        r->Printf(0, "must be in a level to loadsun\n");
        return;
    }

    world = rgp.world;
    if (world == NULL) {
        r->Printf(0, "must have a world loaded to loadsun\n");
        return;
    }

    sunName = r->Cmd_Argv(1);
    R_LoadSunThroughDvars(sunName, &world->sun);
}

void R_Cmd_SaveSun(void)
{
    int argc;
    const char *sunName;
    char szFileBuffer[0x2000];
    int len;
    refimport_t *r = (refimport_t *)&ri;

    argc = r->Cmd_Argc();
    if (argc != 2) {
        r->Printf(0, "usage: savesun <name>\n");
        return;
    }

    sunName = r->Cmd_Argv(1);

    if (r->Com_SaveDvarsToBuffer((const char **)s_sundvars, 0x15, szFileBuffer, 0x2000)) {
        const char *filename;
        qboolean (*fsWriteFile)(const char *, const void *, int) = r->FS_WriteFile;
        len = strlen(szFileBuffer);
        filename = va("sun/%s.sun", sunName);
        fsWriteFile(filename, szFileBuffer, len);
    }
}

void R_FlushSun(void)
{
    int i = 0;
    SunFlareDynamic *sfd = (SunFlareDynamic *)r_sunFlareState;
    do {
        sfd->currentBlind = 0.0f;
        sfd->currentGlare = 0.0f;
        sfd->flareIntensity = 0.0f;
        sfd->hitNum = 0;
        sfd->lastDot = 0.0f;
        sfd->lastTime = 0;
        sfd->lastVisibility = 0.0f;
        i++;
        sfd = (SunFlareDynamic *)((byte *)sfd + 0x30);
    } while (i < 4);
}

void R_SetSunFromDvars(sunflare_t *sun)
{
    sun->spriteMaterial = Material_RegisterHandle(r_sunsprite_shader->current.string, 0, 6);

    sun->spriteSize = r_sunsprite_size->current.value;

    sun->flareMaterial = Material_RegisterHandle(r_sunflare_shader->current.string, 0, 6);

    sun->flareMinSize = r_sunflare_min_size->current.value * 0.5f;
    sun->flareMinDot = (float)cos((double)r_sunflare_min_angle->current.value * DEG2RAD);
    sun->flareMaxSize = r_sunflare_max_size->current.value * 0.5f;
    sun->flareMaxDot = (float)cos((double)r_sunflare_max_angle->current.value * DEG2RAD);
    sun->flareMaxAlpha = r_sunflare_max_alpha->current.value;
    sun->flareFadeInTime = (int)floorf(r_sunflare_fadein->current.value * 1000.0f + 0.5f);
    sun->flareFadeOutTime = (int)floorf(r_sunflare_fadeout->current.value * 1000.0f + 0.5f);

    sun->blindMinDot = (float)cos((double)r_sunblind_min_angle->current.value * DEG2RAD);
    sun->blindMaxDot = (float)cos((double)r_sunblind_max_angle->current.value * DEG2RAD);
    sun->blindMaxDarken = r_sunblind_max_darken->current.value;
    sun->blindFadeInTime = (int)floorf(r_sunblind_fadein->current.value * 1000.0f + 0.5f);
    sun->blindFadeOutTime = (int)floorf(r_sunblind_fadeout->current.value * 1000.0f + 0.5f);

    sun->glareMinDot = (float)cos((double)r_sunglare_min_angle->current.value * DEG2RAD);
    sun->glareMaxDot = (float)cos((double)r_sunglare_max_angle->current.value * DEG2RAD);
    sun->glareMaxLighten = r_sunglare_max_lighten->current.value;
    sun->glareFadeInTime = (int)floorf(r_sunglare_fadein->current.value * 1000.0f + 0.5f);
    sun->glareFadeOutTime = (int)floorf(1000.0f * r_sunglare_fadeout->current.value + 0.5f);

    AngleVectors(r_sun_fx_position->current.vector, sun->sunFxPosition, 0, 0);

    sun->hasValidData = 1;

    return;
}
