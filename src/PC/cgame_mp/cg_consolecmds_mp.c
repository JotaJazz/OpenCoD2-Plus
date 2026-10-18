#include "cod2_feature_config.h"
#include <math.h>
#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include <stdlib.h>

extern dvar_t *cg_viewsize;

extern int CG_CrosshairPlayer(void);
extern void Cmd_ArgvBuffer(int arg, char *buf, int bufSize);
extern const char *va(const char *fmt, ...);
extern void Cbuf_AddText(const char *text);
extern void Dvar_SetInt(const dvar_t *dvar, int value);
extern void Com_Printf(const char *fmt, ...);
extern int CG_ScoreboardDisplayed(void);
extern int Cmd_Argc(void);
extern void CG_SetShellShockParmsFromDvars(shellshock_parms_t *parms);
extern int CG_LoadShellShockDvars(const char *name);
extern qboolean CG_SaveShellShockDvars(const char *name);
extern void Cmd_ArgsBuffer(char *buf, int bufSize);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern void CL_AddReliableCommand(const char *cmd);
extern const char *CG_Argv(int arg);
extern int I_stricmp(const char *s1, const char *s2);
extern void CL_AddCgameCommand(const char *cmdName);
extern qboolean CL_Popup(const char *menu);
extern const char *UI_SafeTranslateString(const char *key);

void CG_TargetCommand_f(void);
static void CG_SizeUp_f(void);
static void CG_SizeDown_f(void);
static void CG_Viewpos_f(void);
void CG_ScoresUp_f(void);
void CG_ScoresDown_f(void);
void CG_PrevWeapon_f(void);
void CG_NextWeapon_f(void);
void CG_WeaponSlot_f(qboolean next, qboolean ignoreEmpty);
void CG_FxSetTestPosition(void);
void CG_FxTest(void);
void CG_FxRestart(void);
static void CG_ShellShock_f(void);
static void CG_ShellShock_Load_f(void);
static void CG_ShellShock_Save_f(void);
static void CG_TellTarget_f(void);
static void CG_QuickMessage_f(void);
static void CG_VoiceChat_f(void);
static void CG_TeamVoiceChat_f(void);
qboolean CG_ConsoleCommand(void);
qboolean CG_IsConsoleCommandName(const char *cmd);
void CG_InitConsoleCommands(void);

static const consoleCommand_t commandsList[] = {
    { "tcmd", CG_TargetCommand_f },
    { "sizeup", CG_SizeUp_f },
    { "sizedown", CG_SizeDown_f },
    { "viewpos", CG_Viewpos_f },
    { "+scores", CG_ScoresDown_f },
    { "-scores", CG_ScoresUp_f },
    { "weapprev", CG_PrevWeapon_f },
    { "weapnext", CG_NextWeapon_f },
    { "weaponslot", (void (*)(void))CG_WeaponSlot_f },
    { "fxSetTestPosition", CG_FxSetTestPosition },
    { "fxTest", CG_FxTest },
    { "fxRestart", CG_FxRestart },
    { "cg_shellshock", CG_ShellShock_f },
    { "cg_shellshock_load", CG_ShellShock_Load_f },
    { "cg_shellshock_save", CG_ShellShock_Save_f },
    { "tell_target", CG_TellTarget_f },
    { "quickmessage", CG_QuickMessage_f },
    { "VoiceChat", CG_VoiceChat_f },
    { "VoiceTeamChat", CG_TeamVoiceChat_f },
    { NULL, NULL }
};

void CG_TargetCommand_f(void)
{
    int targetNum;
    char test[4];

    targetNum = CG_CrosshairPlayer();
    if (targetNum == 0)
        return;

    Cmd_ArgvBuffer(1, test, 4);
    Cbuf_AddText(va("gc %i %i", targetNum, atoi(test)));
}

static void CG_SizeUp_f(void)
{
    dvar_t *dvar = cg_viewsize;
    int val = dvar->current.integer;

    Dvar_SetInt(dvar, val + 10);
}

static void CG_SizeDown_f(void)
{
    dvar_t *dvar = cg_viewsize;
    int val = dvar->current.integer;

    Dvar_SetInt(dvar, val - 10);
}

static void CG_Viewpos_f(void)
{

    Com_Printf("(%i %i %i) : %i\n",
               (int)cg->refdef.vieworg[0],
               (int)cg->refdef.vieworg[1],
               (int)cg->refdef.vieworg[2],
               (int)cg->refdefViewAngles[1]);
}

void CG_ScoresUp_f(void)
{

    if (!CG_ScoreboardDisplayed())
        return;

    cg->showScores = 0;
    cg->scoreFadeTime = cg->time;
}

void CG_ScoresDown_f(void)
{
    int currentTime = cg->time;
    int lastScoreTime = cg->scoresRequestTime;

    if (lastScoreTime + 2000 < currentTime) {
        cg->scoresRequestTime = currentTime;
        CL_AddReliableCommand("score");

        if (!CG_ScoreboardDisplayed()) {
            cg->numScores = 0;
            cg->scoresTop = 0;
            cg->showScores = 1;
        }
        return;
    }

    cg->showScores = 1;
}

static void CG_ShellShock_f(void)
{
    char arg[256];
    int argc;
    double duration;

    argc = Cmd_Argc();

    switch (argc) {
    case 2:
        break;
    case 3:
        Cmd_ArgvBuffer(2, arg, 256);
        if (!CG_LoadShellShockDvars(arg))
            return;
        break;
    default:
        Com_Printf("USAGE: cg_shellshock <duration> <filename?>\n");
        return;
    }

    Cmd_ArgvBuffer(1, arg, 256);
    duration = atof(arg);

    CG_SetShellShockParmsFromDvars( (shellshock_parms_t *)((byte *)cgs->shellshockParms));

    cg->testShock.time = cg->time;
    cg->testShock.duration = (int)floorf((float)duration * 1000.0f + 0.5f);
}

static void CG_ShellShock_Load_f(void)
{
    char name[64];

    if (Cmd_Argc() != 2) {
        Com_Printf("USAGE: cg_shellshock_load <name>\n");
        return;
    }

    Cmd_ArgvBuffer(1, name, 64);
    CG_LoadShellShockDvars(name);
}

static void CG_ShellShock_Save_f(void)
{
    char name[64];

    if (Cmd_Argc() != 2) {
        Com_Printf("USAGE: cg_shellshock_save <name>\n");
        return;
    }

    Cmd_ArgvBuffer(1, name, 64);
    CG_SaveShellShockDvars(name);
}

static void CG_TellTarget_f(void)
{
    int clientNum;
    char command[128];
    char message[128];

    clientNum = CG_CrosshairPlayer();
    if (clientNum == -1)
        return;

    Cmd_ArgsBuffer(message, 128);
    Com_sprintf(command, 128, "tell %i \"%s\"", clientNum, message);
    CL_AddReliableCommand(command);
}

static void CG_QuickMessage_f(void)
{
    snapshot_t *nextSnap = cg->nextSnap;

    if (nextSnap == NULL)
        return;
    if (!(nextSnap->ps.pm_flags & 0x800000))
        return;

    CL_Popup("UIMENU_WM_QUICKMESSAGE");
}

static void CG_VoiceChat_f(void)
{
    char chatCmd[64];
    snapshot_t *nextSnap;

    if (Cmd_Argc() != 2)
        return;

    nextSnap = cgArray[0].nextSnap;

    if (nextSnap != NULL && nextSnap->ps.pm_type != 5 && !(nextSnap->ps.pm_flags & 0x800000)) {
        Com_Printf("%s\n", UI_SafeTranslateString("CGAME_NOSPECTATORVOICECHAT"));
        return;
    }

    Cmd_ArgvBuffer(1, chatCmd, 64);
    Cbuf_AddText(va("cmd vsay %s\n", chatCmd));
}

static void CG_TeamVoiceChat_f(void)
{
    char chatCmd[64];
    snapshot_t *nextSnap;

    if (Cmd_Argc() != 2)
        return;

    nextSnap = cgArray[0].nextSnap;

    if (nextSnap != NULL && nextSnap->ps.pm_type != 5 && !(nextSnap->ps.pm_flags & 0x800000)) {
        Com_Printf("%s\n", UI_SafeTranslateString("CGAME_NOSPECTATORVOICECHAT"));
        return;
    }

    Cmd_ArgvBuffer(1, chatCmd, 64);
    Cbuf_AddText(va("cmd vsay_team %s\n", chatCmd));
}

qboolean CG_IsConsoleCommandName(const char *cmd)
{
    byte *cmdList = (byte *)commandsList;
    const char *name;

    if (!cmd)
        return 0;

    name = *(const char **)cmdList;
    while (name != NULL) {
        if (I_stricmp(cmd, name) == 0)
            return 1;
        cmdList += 8;
        name = *(const char **)cmdList;
    }

    return 0;
}

qboolean CG_ConsoleCommand(void)
{
    const char *cmd;
    const consoleCommand_t *cmd_p;
    const char *name;
    int i;

    if (cg->nextSnap == NULL)
        return 0;

    cmd = CG_Argv(0);
    name = commandsList[0].cmd;
    if (name == NULL)
        return 0;

    i = 0;
    cmd_p = commandsList;
    for (;;) {
        if (I_stricmp(cmd, name) == 0) {
            if (commandsList[i].function != NULL)
                commandsList[i].function();
            return 1;
        }
        i++;
        cmd_p++;
        name = cmd_p->cmd;
        if (name == NULL)
            return 0;
    }
}

void CG_InitConsoleCommands(void)
{
    byte *cmdList = (byte *)commandsList;
    const char *name;

    name = *(const char **)cmdList;
    while (name != NULL) {
        CL_AddCgameCommand(name);
        cmdList += 8;
        name = *(const char **)cmdList;
    }

    CL_AddCgameCommand("kill");
    CL_AddCgameCommand("give");
    CL_AddCgameCommand("take");
    CL_AddCgameCommand("god");
    CL_AddCgameCommand("demigod");
    CL_AddCgameCommand("notarget");
    CL_AddCgameCommand("noclip");
    CL_AddCgameCommand("ufo");
    CL_AddCgameCommand("levelshot");
    CL_AddCgameCommand("setviewpos");
    CL_AddCgameCommand("jumptonode");
    CL_AddCgameCommand("stats");
    CL_AddCgameCommand("say");
    CL_AddCgameCommand("say_team");
    CL_AddCgameCommand("tell");
    CL_AddCgameCommand("team");
    CL_AddCgameCommand("follow");
    CL_AddCgameCommand("callvote");
    CL_AddCgameCommand("vote");
    CL_AddCgameCommand("follownext");
    CL_AddCgameCommand("followprev");
    CL_AddCgameCommand("printentities");
    CL_AddCgameCommand("muteplayer");
    CL_AddCgameCommand("unmuteplayer");

#if COD2_FEATURE_RUMBLE

    {
        extern void CG_InitRumble(void);
        CG_InitRumble();
    }
#endif
}
