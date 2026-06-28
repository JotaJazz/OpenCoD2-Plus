#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#define stricmp strcasecmp

extern qboolean scr_initialized;
extern Bool updateScreenCalled;

__attribute__((used, packed, aligned(4)))
const char *szShotName[] = {
    "_up",
    "_dn",
    "_lf",
    "_rt",
    "_ft",
    "_bk",
    NULL,
};

#define re_ptr_195eca8 ((refexport_t *)imp_re)
#define cls_ptr_195ecac ((byte *)imp_cls)
#define dvar_ptr_195ee78 ((byte *)imp_cl)
#define clc_ptr_195ee8c ((byte *)imp_clc)
#define ptr_195eea4 ((byte *)imp_com_errorEntered)
#define ptr_195f58c ((byte *)imp_colorBlack)
#define ptr_195f5e0 ((byte *)imp_net_showprofile)

extern const dvar_t *net_showprofile;

typedef void (*re_void_func)(void);
typedef void (*re_int_func)(int);
typedef void (*re_int2_func)(int, int);
typedef void (*re_int4_func)(int, int, int, int);
typedef void (*re_floatp_func)(float *);
/* the font parameter is a FontHandle (pointer); the decomp typed it as int,
 * which truncates the 8-byte handle on x64 -> R_TextHeight etc. deref garbage. */
typedef float (*re_font_height_func)(FontHandle, float);
typedef int (*re_font_iheight_func)(FontHandle);
typedef void (*re_draw_string_func)(const char *, int, FontHandle, float, float, float, float, const float *, int);
typedef void (*re_draw_console_func)(const short int *, int, int, float, float, float, float, const float *, int);
typedef void (*re_write_cubemap_func)(const char *, int, float, float);

/* The offsets are x86 byte offsets into refexport_t's function-pointer table
 * (field index * 4). On x64 the pointers are 8 bytes, so the same field lives at
 * field index * 8 == double the x86 byte offset. refexport_t is a pure vtable
 * (all function pointers) so the doubling is exact. */
#if defined(COD2_X64)
#define RE_FUNC(re, offset, type) ((type)(*(void **)((byte *)(re) + (offset) * 2)))
#else
#define RE_FUNC(re, offset, type) ((type)(*(void **)((byte *)(re) + (offset))))
#endif

extern void Com_Printf(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern int Cmd_Argc(void);
extern char *Cmd_Argv(int arg);
extern const char *va(const char *fmt, ...);
extern int CG_DrawActiveFrame(int serverTime, int needRender, int side, int size, int serverDemo);
extern float UI_GetBlurRadius(void);
extern void UI_Refresh(void);
extern int UI_IsFullscreen(void);
extern int UI_GetActiveMenu(void);
extern void UI_DrawConnectScreen(void);
extern qboolean UI_SetActiveMenu(int menu);
extern void UI_UpdateTime(int time);
extern void CL_ClearScene(void);
extern void CL_ResetSkeletonCache(int flags);
extern void CL_SendCmdInternal(void);
extern void CL_SetCGameTime(void);
extern qboolean CL_AnyLocalClientChallenging(void);
extern void CL_DrawLogo(void);
extern void CL_LookupColor(int colorIndex, float *outColor);
extern void Con_DrawConsole(void);
extern void SND_StopSounds(snd_stopsounds_arg_t flags);
extern void SCR_DrawCinematic(void);
extern qboolean Sys_IsMainThread(void);
extern void Sys_LoadingKeepAlive(void);
extern void Net_DisplayProfile(void);
extern int FS_FTell(int fh);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);

void SCR_DrawSmallStringExt(int x, int y, const char *string, const vec_t *setColor);
void SCR_DrawConsoleString(int x, int y, const short int *string, int maxChars, const vec_t *setColor);
void SCR_Init(void);
float CL_GetMenuBlurRadius(void);
static void CL_CubemapShotUsage(void);
void CL_CubemapShot_f(void);
static void SCR_UpdateFrame(void);
void SCR_UpdateScreenInternal(void);
void SCR_UpdateScreen(void);

void SCR_DrawSmallStringExt(int x, int y, const char *string, const vec_t *setColor)
{
    refexport_t *re = re_ptr_195eca8;
    byte *cls = cls_ptr_195ecac;
    FontHandle font = (((clientStatic_t *)(cls))->consoleFont);

    int fontHeight = RE_FUNC(re, 0x118, re_font_iheight_func)(font);

    RE_FUNC(re, 0x11c, re_draw_string_func)(
        string, 0x7fffffff, font, (float)x, (float)y + (float)fontHeight,
        1.0f, 1.0f, setColor, 0);
}

void SCR_DrawConsoleString(int x, int y, const short int *string, int maxChars, const vec_t *setColor)
{
    refexport_t *re = re_ptr_195eca8;
    byte *cls = cls_ptr_195ecac;
    FontHandle font = (((clientStatic_t *)(cls))->consoleFont);

    int fontHeight = RE_FUNC(re, 0x118, re_font_iheight_func)(font);

    RE_FUNC(re, 0x128, re_draw_console_func)(
        string, maxChars, (int)font, (float)x, (float)y + (float)fontHeight,
        1.0f, 1.0f, setColor, 0);
}

void SCR_Init(void)
{
    scr_initialized = 1;
}

float CL_GetMenuBlurRadius(void)
{
    clientActive_t *cl_local = *(clientActive_t **)dvar_ptr_195ee78;
    if (!(cl_local->keyCatchers & 8))
        return 0.0f;

    byte *cls = cls_ptr_195ecac;
    if ((((clientStatic_t *)(cls))->uiStarted) == 0)
        return 0.0f;

    clientConnection_t *clc = *(clientConnection_t **)clc_ptr_195ee8c;
    if (clc->state == 1)
        return 0.0f;

    return UI_GetBlurRadius();
}

static void CL_CubemapShotUsage(void)
{
    Com_Printf("Syntax: cubemapShot size basefilename [water r0 g0 b0 r90 g90 b90 | fresnel n0 n1]\n");
    Com_Printf("size must be a power of 2 >= 4 and <= 1024\n");
    Com_Printf("screenshots will be written to 'env/basefilename_*.tga'\n");
    Com_Printf("basefilename must not exceed %i chars\n", 40);
    Com_Printf("If 'water' is specified, a diffuse water color cubemap is generated using local lighting.\n");
    Com_Printf("The water has the given colors at the given angles, and blends between them in the middle.\n");
    Com_Printf("If 'fresnel' is specified, the alpha channel of the cubemap contains the reflection factor.\n");
    Com_Printf("n0 and n1 are the index of refraction of the 'air' and 'water' surfaces, respectively.\n");
    Com_Printf("The index of refraction must always be 1 or greater.\n");
    Com_Printf("This is always calculated, and defaults to air-water interface (n0 = 1, n1 = 1.333).\n");
}

void CL_CubemapShot_f(void)
{
    char szBaseName[100];
    float rgb[3];
    float n0, n1;
    byte isLightingShot;
    int side;

    clientActive_t *cl_local = *(clientActive_t **)dvar_ptr_195ee78;
    if (cl_local->cgameInitialized == 0) {
        Com_Printf("must be in a map to use this command\n");
        return;
    }

    if (Cmd_Argc() <= 2) {
        CL_CubemapShotUsage();
        return;
    }

    const char *baseArg = Cmd_Argv(2);
    if (strlen(baseArg) > 40) {
        CL_CubemapShotUsage();
        return;
    }

    strcpy(szBaseName, Cmd_Argv(2));
    int size = atoi(Cmd_Argv(1));

    if ((unsigned int)(size - 4) > 0x3fc) {
        CL_CubemapShotUsage();
        return;
    }
    if ((size - 1) & size) {
        CL_CubemapShotUsage();
        return;
    }

    rgb[0] = 0.0f;
    rgb[1] = 0.0f;
    rgb[2] = 0.0f;

    if (Cmd_Argc() == 7) {
        if (stricmp(Cmd_Argv(3), "lighting") != 0) {
            CL_CubemapShotUsage();
            return;
        }
        rgb[0] = (float)atof(Cmd_Argv(4));
        rgb[1] = (float)atof(Cmd_Argv(5));
        rgb[2] = (float)atof(Cmd_Argv(6));
        isLightingShot = 1;
        n0 = 1.0f;
        n1 = 1.333f;
    } else if (Cmd_Argc() == 6) {
        if (stricmp(Cmd_Argv(3), "fresnel") != 0) {
            CL_CubemapShotUsage();
            return;
        }
        n0 = (float)atof(Cmd_Argv(4));
        n1 = (float)atof(Cmd_Argv(5));
        if (n0 < 1.0f) {
            CL_CubemapShotUsage();
            return;
        }
        if (n1 < 1.0f) {
            CL_CubemapShotUsage();
            return;
        }
        isLightingShot = 0;
    } else if (Cmd_Argc() == 3) {
        isLightingShot = 0;
        n0 = 1.0f;
        n1 = 1.333f;
    } else {
        CL_CubemapShotUsage();
        return;
    }

    CL_ResetSkeletonCache(0);

    refexport_t *re = re_ptr_195eca8;
    for (side = 1; side <= 6; side++) {
        RE_FUNC(re, 0xec, re_int2_func)(size, 1);
        RE_FUNC(re, 0xa8, re_void_func)();
        CL_ClearScene();

        clientConnection_t *clc_local = *(clientConnection_t **)clc_ptr_195ee8c;
        int needRender = clc_local->demoplaying != 0;
        clientActive_t *dv = *(clientActive_t **)dvar_ptr_195ee78;
        int serverTime = dv->serverTime;

        CG_DrawActiveFrame(serverTime, needRender, side, size, 0);
        RE_FUNC(re, 0xac, re_void_func)();
        RE_FUNC(re, 0xf0, re_int_func)(side);
    }

    if (isLightingShot) {
        RE_FUNC(re, 0xf8, re_floatp_func)(rgb);
    }

    for (side = 1; side <= 6; side++) {
        re_write_cubemap_func writeFn = RE_FUNC(re_ptr_195eca8, 0xf4, re_write_cubemap_func);
        const char *filename = va("env/%s%s.tga", szBaseName, szShotName[side - 1]);
        writeFn(filename, side, n0, n1);
    }
}

static void SCR_UpdateFrame(void)
{
    refexport_t *re = re_ptr_195eca8;
    static int s_prevConnstate = -1;
    int endViewBeforeUi = 0;
    int viewEndedBeforeUi = 0;
    int viewsIssuedBeforeUi = 0;

    if (!re || !re->BeginFrame) {
        return;
    }

    {
        extern int g_cginit_loading;
        clientConnection_t *clc_check = *(clientConnection_t **)clc_ptr_195ee8c;
        clientActive_t *cl_check = *(clientActive_t **)dvar_ptr_195ee78;
        int cs = clc_check->state;
        if (g_cginit_loading)
            return;
        if (cs == CA_PRIMED && cl_check->cgameInitialized && cl_check->newSnapshots) {
            CL_SetCGameTime();
            cs = clc_check->state;
        }
    }

    RE_FUNC(re, 0xa8, re_void_func)();
    CL_ClearScene();
    CL_ResetSkeletonCache(0);

    byte *cls = cls_ptr_195ecac;
    int gameLoaded = (((clientStatic_t *)(cls))->uiStarted);

    {
        static int diagOnce2;
        if (!diagOnce2) {
            diagOnce2 = 1;
            Com_Printf("[diag] SCR_UpdateFrame REACHED gameLoaded(uiStarted)=%d\n", gameLoaded);
        }
    }

    if (!gameLoaded) {
        re->ClearScreen(1, (const vec_t *)ptr_195f58c, 0.0f, 0);
        goto end_frame;
    }

    clientConnection_t *clc = *(clientConnection_t **)clc_ptr_195ee8c;
    int connstate = clc->state;
    if (connstate != 8 && connstate != 1) {
        re->ClearScreen(1, (const vec_t *)ptr_195f58c, 0.0f, 0);
    }

    UI_UpdateTime((((clientStatic_t *)(cls))->realtime));

    if (connstate >= 3 && connstate <= 8 && UI_GetActiveMenu() == 1) {
        UI_SetActiveMenu(0);
    }

    s_prevConnstate = connstate;

    if (UI_IsFullscreen()) {

        clc = *(clientConnection_t **)clc_ptr_195ee8c;
        connstate = clc->state;

        if (connstate < 0) {
            Com_Error(0, "SCR_DrawScreenField: bad clc->state");
        } else if (connstate <= 7) {
            goto check_ui;
        } else if (connstate == 8) {

            int needRender = clc->demoplaying != 0;
            clientActive_t *dv = *(clientActive_t **)dvar_ptr_195ee78;
            int serverTime = dv->serverTime;

            int result = CG_DrawActiveFrame(serverTime, needRender, 0, 0, 0);
            endViewBeforeUi = 1;
            if (result == 0) {
                CL_SendCmdInternal();
            }
            goto check_ui;
        } else {
            Com_Error(0, "SCR_DrawScreenField: bad clc->state");
        }
    } else {

        clc = *(clientConnection_t **)clc_ptr_195ee8c;
        connstate = clc->state;

        if ((unsigned int)connstate > 8) {
            Com_Error(0, "SCR_DrawScreenField: bad clc->state");
        }

        switch (connstate) {
        case 0:
            SND_StopSounds((snd_stopsounds_arg_t)0);
            if (Sys_IsMainThread()) {
                UI_SetActiveMenu(1);
            } else if (CL_AnyLocalClientChallenging()) {
                UI_Refresh();
                UI_DrawConnectScreen();
            }
            goto check_ui;

        case 1:
            SCR_DrawCinematic();
            goto check_ui;

        case 2:
            CL_DrawLogo();
            clc = *(clientConnection_t **)clc_ptr_195ee8c;
            if (clc->state != 2) {
                goto end_frame_draw;
            }
            goto check_ui;

        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            UI_Refresh();
            UI_DrawConnectScreen();
            goto check_ui;

        case 8:
        {
            int needRender = clc->demoplaying != 0;
            clientActive_t *dv = *(clientActive_t **)dvar_ptr_195ee78;
            int serverTime = dv->serverTime;

            CG_DrawActiveFrame(serverTime, needRender, 0, 0, 1);
            endViewBeforeUi = 1;

            clc = *(clientConnection_t **)clc_ptr_195ee8c;
            if (clc->demorecording) {
                int pos = FS_FTell(clc->demofile);
                int posKB = (pos + 0x3ff) >> 10;
                char buf[1024];
                float color[4];
                snprintf(buf, sizeof(buf), "RECORDING %s: %ik", clc->demoName, posKB);

                CL_LookupColor(0x37, color);
                float x = 5.0f;
                float y = 479.0f;

                re = re_ptr_195eca8;
                cls = cls_ptr_195ecac;
                FontHandle font = (((clientStatic_t *)(cls))->consoleFont);
                float fontH = RE_FUNC(re, 0x110, re_font_height_func)(font, 0.333333f);
                float w = fontH;
                float h = fontH;

                CalcScreenPlacement(&x, &y, &w, &h, 1, 1);

                RE_FUNC(re, 0x11c, re_draw_string_func)(
                    buf, 0x7fffffff, font, x, y, w, h, color, 0);
            }
            goto check_ui;
        }
        }
    }

check_ui:
    if (endViewBeforeUi) {
        RE_FUNC(re_ptr_195eca8, 0xb8, re_int_func)(0);
        viewEndedBeforeUi = 1;
        RE_FUNC(re_ptr_195eca8, 0xbc, re_void_func)();
        viewsIssuedBeforeUi = 1;
    }

    {
        clientActive_t *dv = *(clientActive_t **)dvar_ptr_195ee78;
        static int uiTraceCount;
        int activeMenu = UI_GetActiveMenu();

        {
            static int diagOnce;
            if (!diagOnce) {
                diagOnce = 1;
                Com_Printf("[diag] check_ui REACHED state=%d activeMenu=%d keyCatchers=0x%x displayHUD=%d\n",
                           (*(clientConnection_t **)clc_ptr_195ee8c)->state, activeMenu,
                           dv->keyCatchers, dv->displayHUDWithKeycatchUI);
            }
        }

        if (uiTraceCount < 80 && (activeMenu || (dv->keyCatchers & 8) || dv->displayHUDWithKeycatchUI)) {
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] SCR check_ui active=%d keyCatchers=0x%x displayHUD=%d state=%d\n",
                           activeMenu, dv->keyCatchers, dv->displayHUDWithKeycatchUI,
                           (*(clientConnection_t **)clc_ptr_195ee8c)->state);
            ++uiTraceCount;
        }

        if (dv->keyCatchers & 8)
            UI_Refresh();
    }

    if (net_showprofile->current.integer) {
        Net_DisplayProfile();
    }

    if (!viewEndedBeforeUi)
        RE_FUNC(re_ptr_195eca8, 0xb8, re_int_func)(0);

end_frame_draw:
    re = re_ptr_195eca8;
    RE_FUNC(re, 0xbc, re_void_func)();
    Con_DrawConsole();
    {
        re_void_func fn = re->EndFrame;
        if (fn)
            fn();
    }
    Sys_IsMainThread();
    return;

end_frame:
    if (!viewsIssuedBeforeUi)
        RE_FUNC(re_ptr_195eca8, 0xbc, re_void_func)();
    Con_DrawConsole();
    {
        refexport_t *re2 = re_ptr_195eca8;
        re_void_func fn = re2->EndFrame;
        if (fn)
            fn();
    }
    Sys_IsMainThread();
}

void SCR_UpdateScreenInternal(void)
{
    if (updateScreenCalled)
        return;

    clientConnection_t *clc_tmp = *(clientConnection_t **)clc_ptr_195ee8c;
    if (clc_tmp->state == 6) {
        Sys_LoadingKeepAlive();
    }

    if (!scr_initialized)
        return;

    byte *guard = ptr_195eea4;
    if (*(int *)guard)
        return;

    updateScreenCalled = 1;
    SCR_UpdateFrame();
    updateScreenCalled = 0;
}

extern int g_cginit_loading;
void SCR_UpdateScreen(void)
{
    if (g_cginit_loading || updateScreenCalled)
        return;

    clientConnection_t *clc_tmp = *(clientConnection_t **)clc_ptr_195ee8c;
    if (clc_tmp->state == 6) {
        Sys_LoadingKeepAlive();
    }

    if (!scr_initialized)
        return;

    byte *guard = ptr_195eea4;
    if (*(int *)guard)
        return;

    updateScreenCalled = 1;
    SCR_UpdateFrame();
    updateScreenCalled = 0;
}
