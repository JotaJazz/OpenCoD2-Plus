#include "common_types.h"
#if defined(COD2_X64) || defined(__x86_64__)
#include <setjmp.h>   /* x64: g_script_error is a real jmp_buf[] array */
#endif

/* BSSINT: a BSS slot originally declared `int` but which frequently
 * holds a POINTER (fs_basepath, sMainWindow, ...). It must be pointer-sized.
 * `long` is 8 bytes on LP64 (Linux x64) but only 4 on LLP64 (Windows x64), so
 * it truncated pointers there. intptr_t is pointer-sized on every arch
 * (4 on x86, 8 on both LP64 and LLP64). (x64 port Stage 4.) */
typedef intptr_t BSSINT;

BSSINT sBuilderProcPtr;
BSSINT sControlValidationUPP;
unsigned char sControlKeyFilterUPP[120];
unsigned char sRectList[12];
BSSINT sSwapCount;
unsigned char sCaptureMedia[8];
BSSINT sMainWindow;
unsigned char sCaptureMovie[24];
unsigned char sCaptureName[256];
BSSINT sCaptureTrack;
BSSINT sCaptureRefNum;
BSSINT sDisplayID;
BSSINT sSystemGammaBlue;
BSSINT sSystemGammaGreen;
BSSINT sSystemGammaRed;
BSSINT sScreenContext;
BSSINT sDisplayRefreshRate;
BSSINT sDisplayDepth;
BSSINT sFadeToken;
BSSINT sInitialized;
unsigned char sDisplayRect[16];
BSSINT sMainDisplayID;
unsigned char sMainRect[120];
unsigned char sResult_00334980[2];
unsigned char sTested[1];
unsigned char hasAltiVec[1];
unsigned char hasAltiVecBeenDetermined[124];
BSSINT sDataFolderDirID;
BSSINT sExecutableDirID;
BSSINT sAppFolderDirID;
unsigned char sAppFolderVRefNum[20];
unsigned char sAppBundleRef[96];
unsigned char sSystemLock[128];
BSSINT sResult_00334b00;
BSSINT sResult_00334b04;
BSSINT sResult_00334b08;
BSSINT sResult_00334b0c;
unsigned char sResult_00334b10[8];
BSSINT sResult_00334b18;
BSSINT sResult_00334b1c;
BSSINT sResult_00334b20;
BSSINT sResult_00334b24;
BSSINT sResult_00334b28;
unsigned char sResult_00334b2c[84];
unsigned char sGlobalMouse[128];
unsigned char sCursorList[12];
BSSINT sCurrentCursor;
BSSINT sSavedWinCursor;
unsigned char sTimerRef[108];
unsigned char sCachedVKMap[128];
BSSINT sATI4CompsConverterABGR;
BSSINT sATI4CompsConverterARGB;
BSSINT sStdConverterABGR;
unsigned char sStdConverterARGB[116];
unsigned char sDirect3DInterface[128];
unsigned char sPointScale[128];
BSSINT sEventTargetRef;
unsigned char sSystemCursorVisible_00334e84[124];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
unsigned char g_threadValues[20];
#endif
unsigned char threadId[108];
unsigned char value1[16384];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
unsigned char g_com_error[96];
unsigned char va_info[2052];
#endif
BSSINT LittleFloatWrite;
BSSINT LittleFloatRead;
BSSINT LittleLong64;
BSSINT LittleLong;
BSSINT LittleShort;
unsigned char valueindex[8];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
BSSINT iWeaponInfoSource;
#endif
BSSINT logfile;
unsigned char errorcode[120];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
unsigned char com_errorMessage[4096];
BSSINT com_lastFrameTime;
BSSINT com_codeTimeScale;
unsigned char com_fullyInitialized[120];
#endif
unsigned char com_pushedEvents[6144];
BSSINT com_pushedEventsTail;
BSSINT com_pushedEventsHead;
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
BSSINT com_safemode;
#endif
BSSINT rd_flush;
BSSINT rd_buffersize;
BSSINT rd_buffer;
BSSINT opening_qconsole;
BSSINT printedWarning;
BSSINT timeClientFrame;
BSSINT errorCount;
unsigned char lastErrorTime[88];
unsigned char g_currentAsian[32];
unsigned char szErrorString[1024];
unsigned char szStrings[2048];
unsigned char iCurrString[96];
unsigned char iString[32];
unsigned char szIwdLanguageName[128];
unsigned char bLanguagesListed[96];
unsigned char g_largeLocalPos[128];
unsigned char g_largeLocalBuf[524288];
unsigned char hunk_high[8];
unsigned char hunk_low[8];
BSSINT s_hunkData;
BSSINT s_hunkTotal;
unsigned char com_hunkData[8];
unsigned char com_fileDataHashTable[4096];
unsigned char s_origHunkData[96];
#if !COD2_PLUS_SERVER  /* defined by src/xanim */
unsigned char g_xAnimInfo[163840];
unsigned char g_notifyListSize[32];
unsigned char g_notifyList[1536];
unsigned char g_end[5];
unsigned char g_anim_developer[91];
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
unsigned char scrStringGlob[65664];
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
unsigned char scrMemTreeGlob[525184];
#endif
unsigned char info6[8192];
unsigned char info8[8192];
unsigned char info5[8192];
unsigned char info4[8192];
unsigned char info3[8192];
unsigned char info2[8192];
unsigned char buf_00482a80[1024];
unsigned char basename_00482e80[128];
unsigned char sString[128];
unsigned char sString_00482f80[64];
unsigned char sTemp[64];
unsigned char cmd_functions[128];
unsigned char cmd_argv[2048];
unsigned char cmd_argc[128];
unsigned char cmd_tokenized[8704];
unsigned char cmd_args1[1024];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
unsigned char cmd_text_buf[65536];
#endif
unsigned char info2_00495f00[8192];
unsigned char info1[1024];
unsigned char dvarHashTable[1024];
unsigned char dvarVectorPool[48];
BSSINT dvarVectorIndex;
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
unsigned char dvar_cheats[12];
unsigned char dvarPool[46080];
#endif
unsigned char isDvarSystemActive[1];
unsigned char isLoadingAutoExecGlobalFlag[63];
unsigned char milesGlob[384];
unsigned char effectListArrayNonBolt[7296];
unsigned char effectListArrayBolt[7296];
unsigned char effectClusterArray[28800];
unsigned char visibleEffectsBolt[14400];
unsigned char visibleEffectsNonBolt[14400];
unsigned char effectTemplateArrayCount[32];
unsigned char effectTemplateArray[1120];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
BSSINT g_bDObjInited;
unsigned char com_lastDObjIndex[124];
unsigned char serverObjMap[2048];
unsigned char clientObjMap[2304];
unsigned char objFreeCount[128];
unsigned char objAlloced[2048];
unsigned char objBuf[204800];
#endif
#if !COD2_PLUS_SERVER  /* defined by src/xanim */
unsigned char g_empty[128];
#endif
unsigned char localization[32];
unsigned char language_buffer[4192];
BSSINT shouldQuitOnError;
unsigned char cml[124];
#if !COD2_PLUS_SERVER  /* defined by src/bgame */
unsigned char bg_iNumAmmoTypes[32];
unsigned char bg_weapAmmoTypes[512];
unsigned char bg_iNumSharedAmmoCaps[32];
unsigned char bg_sharedAmmoCaps[512];
unsigned char bg_iNumWeapClips[32];
unsigned char bg_weapClips[544];
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
unsigned char scrVmGlob[8320];
#endif
#if defined(_M_X64) || defined(__x86_64__)
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
struct scrCompileGlob_t scrCompileGlob;   /* x86 was unsigned char[512]; x64 struct is ~920B (value_start[32] grows) -> blob overflowed */
#endif
#else
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
unsigned char scrCompileGlob[512];
#endif
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
struct scrAnimGlob_t scrAnimGlob;
#endif
#if !COD2_PLUS_SERVER  /* defined by src/bgame */
BSSINT jump_height;
BSSINT jump_spreadAdd;
BSSINT jump_slowdownEnable;
BSSINT jump_ladderPushVel;
unsigned char jump_stepSize[112];
BSSINT mantle_enable;
BSSINT mantle_view_yawcap;
#endif
BSSINT s_mantleAnims;
#if !COD2_PLUS_SERVER  /* defined by src/bgame */
BSSINT mantle_debug;
BSSINT mantle_check_angle;
BSSINT mantle_check_range;
unsigned char mantle_check_radius[104];
#endif
unsigned char token_004ed380[1024];
unsigned char statCount[32];
unsigned char stats[96];
unsigned char initialized[128];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
unsigned char cm_world[24704];
#endif
#if defined(COD2_X64) || defined(__x86_64__)
cin_cache cinTable[16];   /* x86 blob 7360 (460*16); x64 cin_cache=496 -> 7936 > 7360 overflow */
#else
cin_cache cinTable[16];
#endif
#if defined(COD2_X64) || defined(__x86_64__)
cinematics_t cin;   /* x86 blob 2426400; x64 sizeof(cinematics_t)=2688536 -> overflow */
#else
struct cinematics_t cin;
#endif
long int ROQ_YY_tab[256];
long int ROQ_VG_tab[256];
long int ROQ_UG_tab[256];
long int ROQ_VR_tab[256];
long int ROQ_UB_tab[256];
short unsigned int vq2[16384];
short unsigned int vq4[65536];
short unsigned int vq8[262144];
unsigned char sAspyrIntroPlayed[32];
#if !COD2_PLUS_SERVER  /* defined by src/xanim */
unsigned char g_testLods[128];
#endif
unsigned char szReference[1024];
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
int currentPos;
#endif
#if !COD2_PLUS_SERVER  /* defined by src/bgame */
unsigned char bg_defaultWeaponDefs[1568];
#endif
unsigned char g_playerAnimTypeNames[256];
unsigned char g_playerAnimTypeNamesCount[96];
unsigned char sys_info[544];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
unsigned char eventQue[6144];
BSSINT eventTail;
BSSINT eventHead;
#endif
BSSINT sys_configSum;
BSSINT sys_gpu;
BSSINT sys_sysMB;
unsigned char sys_cpuGHz[76];
BSSINT sConsoleEditText;
BSSINT sConsoleTextView;
BSSINT sConsoleData;
unsigned char sConsoleWindow[20];
unsigned char sReturnedText[512];
unsigned char sConsoleText[512];
unsigned char sTimerRef_007f1b20[96];
unsigned char cwd[256];
unsigned char lockPvsViewParms[332];
BSSINT warnCount;
unsigned char warnCount_007f1dd0[48];
GfxCmdArray *s_cmdList;   /* was unsigned char[128] blob; consumers use it as a GfxCmdArray* (assigned &commands, deref'd) */
#if defined(COD2_X64) || defined(__x86_64__)
struct GfxDebugFrameGlob s_debugFrameGlob;   /* x86 blob 2399616; x64 sizeof 3054928 (pointer fields grow) */
#else
unsigned char s_debugFrameGlob[2399616];
#endif
unsigned char s_backEndData[2399596];
unsigned char g_dummyBuf[20];
refexport_t re;
BSSINT warnCount_00c85b00;
BSSINT warnCount_00c85b04;
BSSINT warnCount_00c85b08;
BSSINT warnCount_00c85b0c;
unsigned char warnCount_00c85b10[112];
#if defined(COD2_X64)
/* x64-relaid material registry (must match the definition in r_material.c). */
struct MaterialGlobals {
    int vertexDeclCount;
    struct MaterialVertexDeclaration vertexDecls[32];
    struct MaterialTechniqueSet *techSetTable[1024];
    int techCount;
    struct MaterialTechnique *techTable[1024];
    int literalCount;
    float literals[64];
    struct MaterialStateMap *stateMapTable[32];
    int stringCount;
    const char *stringTable[64];
    int shaderCount;
    struct MaterialShader *shaderTable[256];
};
struct MaterialGlobals materialGlobals;
#else
unsigned char materialGlobals[10752];
#endif
unsigned char s_cache[50304];
unsigned char g_imageProgs[448];
unsigned char imageGlobals[8256];
unsigned char cubeShotGlob[24];
unsigned char lastNumber[104];
unsigned char s_vc_log[128];
unsigned char registeredFontCount[32];
unsigned char registeredFont[96];
unsigned char debugGlobals[128];
unsigned char dpvsConfig[32];
/* x86 sizeof(DpvsGlobals)=224; on x64 its ~8 pointer fields (clipPlanes/occluderList/
   portalQueue/portalPool/...) grow it well past 224, so the r_dpvs.c
   `(DpvsGlobals*)&dpvsGlob` access overflowed into dpvsScene (model cull data) ->
   XModels got culled out / mis-drawn. Over-size the blob so the x64 struct fits. */
#if defined(COD2_X64)
unsigned char dpvsGlob[512];
#else
unsigned char dpvsGlob[224];
#endif
unsigned char dpvsScene[131200];
unsigned char shadowCookieGlob[128];
unsigned char waterGlob[196608];
unsigned char surfBoundsGlob[128];
unsigned char mtlLoadGlob[128];
unsigned char smodelLoadGlob[128];
unsigned char outdoorGlob[128];
unsigned char sOldButtonState[128];
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
sval_t yaccResult;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
BSSINT yy_start;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
YY_BUFFER_STATE yy_current_buffer;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
char ch_buf[16388];
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
sval_t g_dummyVal;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
unsigned char g_parse_user;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
unsigned int g_sourcePos;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
unsigned int g_out_pos;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
char yy_hold_char;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
char *yy_c_buf_p;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
BSSINT yy_n_chars;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
BSSINT yy_did_buffer_switch_on_eof;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
char *yy_last_accepting_cpos;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
yy_state_type yy_last_accepting_state;
#endif
BSSINT sSoundEngine;
unsigned char sHighQualityEngine[124];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
unsigned char comBspGlob[128];
#endif
unsigned char __ZGVZ16GetMacGameEnginevE13theGameEngine[32];
unsigned char theGameEngine[96];
unsigned char sDeviceName[128];
unsigned char sShaderPrograms[24];
unsigned char sInit[104];
unsigned char hasExactMatch[128];
unsigned char shortestMatch[1024];
BSSINT matchCount;
BSSINT completionString;
unsigned char tinystr[120];
itemInfo_t cg_itemsArray[256];
weaponInfo_t cg_weaponsArray[128];
unsigned char cg_entitiesArray[561152];

unsigned char cgsArray[sizeof(cgs_t)];
cg_t cgArray[1];
unsigned char g_mapLoaded[1];
unsigned char g_ambientStarted[3];
unsigned char buffer_00e86a20[1120];
unsigned char input_viewSensitivity[32];
unsigned char szServerIPAddress[128];
unsigned char recursive[96];
#if !COD2_PLUS_SERVER  /* defined by src/server */
unsigned char g_sv_skel_memory_start[128];
unsigned char g_sv_skel_memory[262144];
#endif
unsigned char warnCount_00ec7000[128];
#if !COD2_PLUS_SERVER  /* defined by src/game */
const dvar_t *g_gametype;
#endif
unsigned char g_mapname[64];
BSSINT g_ingameMenusLoaded;
unsigned char ui_serverFilterType[28];
unsigned char menuBuf2[32768];
unsigned char errorString[1024];
unsigned char info[1024];
BSSINT bypassKeyClear;
BSSINT numclean;
unsigned char lastTime[24];
unsigned char clientBuff[32];
unsigned char info_00ecf960[1024];
BSSINT numTimeOuts;
BSSINT numFound;
unsigned char tleIndex[24];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
loopback_t loopbacks[2];
unsigned char net_iProfilingOn[16];
#endif
unsigned char s[96];
unsigned char string_00edae00[1024];
unsigned char con[151588];
BSSINT con_outputWindowColor;
BSSINT con_outputSliderColor;
BSSINT con_outputBarColor;
BSSINT con_inputHintBoxColor;
unsigned char con_inputBoxColor[12];
unsigned char conDrawInputGlob[32];
unsigned char hudMsgIconMaterials[1024];
unsigned char registeredIconMaterialCount[32];
unsigned char s_playerMute[64];
unsigned char rconGlob[64];
BSSINT debugMode;
BSSINT captureData;
BSSINT captureFunc;
BSSINT itemCapture;
unsigned char g_bindItem[16];
unsigned char scrollInfo[32];
BSSINT lastListBoxClickTime;
unsigned char rect_00f00744[24];
unsigned char inHandleKey[36];
unsigned char initialized_00f00780[128];
unsigned char msgInit[32];
#if defined(__x86_64__) || defined(_M_X64)

struct huffman_t msgHuff;
#else
unsigned char msgHuff[57408];
#endif
unsigned char string_00f0e860[1024];
unsigned char string_00f0ec60[8192];
unsigned char string_00f10c60[1056];
unsigned char bigConfigString[8192];
BSSINT warnCount_00f13080;
unsigned char warnCount_00f13084[124];
#if !COD2_PLUS_SERVER  /* defined by src/server */
unsigned char botport[128];
#endif
unsigned char ui_arenaInfos[256];
unsigned char ui_numArenas[128];
unsigned char defineBits[1152];
#if !COD2_PLUS_SERVER  /* defined by src/bgame */
unsigned char weaponStrings[1024];
#endif
BSSINT parseEvent;
BSSINT parseMovetype;
unsigned char defineStringsOffset[24];
unsigned char numDefines[64];
unsigned char defineStrings[10016];
unsigned char defineStr[1152];
BSSINT g_piNumLoadAnims;
BSSINT g_pLoadAnims;
unsigned char globalScriptData[24];
unsigned char input[100000];
unsigned char bScriptFileLoaded[64];
unsigned char playersKb[640];
BSSINT hud_healthOverlay_phaseEnd_pulseDuration;
BSSINT hud_healthOverlay_phaseEnd_toAlpha;
BSSINT hud_healthOverlay_regenPauseTime;
BSSINT hud_healthOverlay_phaseThree_pulseDuration;
BSSINT hud_healthOverlay_phaseThree_toAlphaMultiplier;
BSSINT hud_healthOverlay_phaseTwo_pulseDuration;
BSSINT hud_healthOverlay_phaseTwo_toAlphaMultiplier;
BSSINT hud_healthOverlay_phaseOne_pulseDuration;
BSSINT hud_healthOverlay_pulseStart;
BSSINT hud_enable;
unsigned char hud_fadeout_speed[24];
unsigned char szErrorString_00f2f140[1088];
unsigned char spGlob[128];
unsigned char bloc[128];
unsigned char g_strHandle[8192];
BSSINT lasttime;
unsigned char adr[124];
unsigned char color_00f31700[128];
unsigned char line_00f31780[1024];
unsigned char menuBuf[32768];
unsigned char g_load[1600];
unsigned char menuParseKeywordHash[2048];
unsigned char menuBuf1[4096];
unsigned char string_00f3b9c0[4160];
#if !COD2_PLUS_SERVER  /* defined by src/game */
unsigned char g_clients[665856];
#endif
BSSINT hud_flash_period_offhand;
unsigned char hud_flash_time_offhand[124];
unsigned char cached_models[1024];
#if !COD2_PLUS_SERVER  /* defined by src/game */
unsigned char pushed[32768];
unsigned char pushed_p[128];
#endif
turretInfo_s turretInfo[32];
#if !COD2_PLUS_SERVER  /* defined by src/game */
unsigned char g_HitLocConstNames[128];
#endif
unsigned char numIPFilters[32];
unsigned char ipFilters[8288];
unsigned char str_00fea180[256];
unsigned char index_00fea280[128];
unsigned char rendererStats[64];
unsigned char fps_previousTimes[128];
BSSINT fps_index;
unsigned char previous[60];
unsigned char cg_pmove[248];
unsigned char cg_numTriggerEntities[8];
unsigned char cg_triggerEntities[1024];
unsigned char cg_numSolidEntities[128];
unsigned char cg_solidEntities[1024];
localEntity_t cg_eachClientLocalEntities[128];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
BSSINT ip_socket;
#endif
unsigned char winsockInitialized[28];
unsigned char winsockdata[400];
BSSINT net_socksPassword;
BSSINT net_socksUsername;
BSSINT net_socksPort;
BSSINT net_socksServer;
BSSINT net_socksEnabled;
BSSINT net_noipx;
BSSINT net_noudp;
BSSINT networkingEnabled;
BSSINT socks_socket;
unsigned char socksRelayAddr[16];
unsigned char usingSocks[28];
unsigned char localIP[64];
unsigned char numIP[32];
unsigned char socksBuf[4096];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
unsigned char ipx_socket[32];
#endif
unsigned char hackSize[128];
BSSINT currentRecordingSample;
unsigned char recording[28];
unsigned char s_clientTalkTime[256];
unsigned char s_clientSamples[256];
BSSINT playing_00ff20a0;
unsigned char count_00ff20a4[92];
unsigned char decodeBits[128];
unsigned char encodeBits[36];
unsigned char g_encoder[92];
BSSINT sAudioRecorder;
unsigned char g_current_sample[28];
unsigned char s_recordingSamples[2340];
unsigned char s_recordingSamplePtr[60];
unsigned char dsoundplay_initialized[128];
BSSINT g_High;
BSSINT g_Low;
unsigned char g_special[120];
unsigned char g_WarmOff[1];
unsigned char g_NoTextureID[127];
unsigned char __ZN6CFence15sUnusedFenceIDsE[128];
unsigned char __ZN13CMemoryBuffer20sDelayedFreeRequestsE[128];
unsigned char __ZN7COpenGL7sOpenGLE[4096];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
#if defined(COD2_X64) || defined(__x86_64__)
TraceThreadInfo g_traceThreadInfo[1];   /* x86 blob 28; x64 sizeof 48 (pointer fields grow) */
#else
unsigned char g_traceThreadInfo[28];
#endif
#endif
/* consumer char *com_consoleLines[32]: x86-sized pointer-array blob, too small on x64 */
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
#if defined(COD2_X64) || defined(__x86_64__)
char *com_consoleLines[32];
#else
char *com_consoleLines[32];
#endif
int com_numConsoleLines;
const dvar_t *ui_errorTitle;
const dvar_t *ui_errorMessage;
BSSINT com_fixedConsolePosition;
BSSINT com_errorEntered;
BSSINT com_frameNumber;
BSSINT com_frameTime;
const dvar_t *com_animCheck;
#endif
const dvar_t *com_recommendedSet;
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
const dvar_t *sv_paused;
#endif
#if !COD2_PLUS_SERVER  /* defined by src/server */
const dvar_t *com_expectedHunkUsage;
BSSINT nextmap;
#endif
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
const dvar_t *cl_paused;
const dvar_t *com_introPlayed;
#endif
const dvar_t *shortversion;
BSSINT version_00ff3f60;
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
const dvar_t *com_logfile;
const dvar_t *com_sv_running;
const dvar_t *com_maxfps;
const dvar_t *com_fixedtime;
float com_timescaleValue;
const dvar_t *com_timescale;
#endif
const dvar_t *com_statmon;
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
const dvar_t *com_developer_script;
const dvar_t *com_developer;
const dvar_t *com_viewlog;
const dvar_t *loc_warningsAsErrors;
const dvar_t *loc_warnings;
const dvar_t *loc_translate;
const dvar_t *loc_forceEnglish;
const dvar_t *loc_language;
char lastValidGame[256];
char lastValidBase[256];
#endif
/* consumer char *fs_serverReferencedIwdNames[1024]: x86-sized pointer-array blob, too small on x64 */
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
#if defined(COD2_X64) || defined(__x86_64__)
char *fs_serverReferencedIwdNames[1024];
#else
char *fs_serverReferencedIwdNames[1024];
#endif
#endif
unsigned char fs_serverReferencedIwds[4096];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
int fs_numServerReferencedIwds;
#endif
/* consumer char *fs_serverIwdNames[1024]: x86-sized pointer-array blob, too small on x64 */
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
#if defined(COD2_X64) || defined(__x86_64__)
char *fs_serverIwdNames[1024];
#else
char *fs_serverIwdNames[1024];
#endif
int fs_serverIwds[1024];
#endif
#if defined(__x86_64__) || defined(_M_X64)

fileHandleData_t fsh[74];
#else
fileHandleData_t fsh[74];
#endif
BSSINT fs_checksumFeed;
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
BSSINT fs_fakeChkSum;
const dvar_t *fs_ignoreLocalized;
const dvar_t *fs_restrict;
const dvar_t *fs_gameDirVar;
const dvar_t *fs_copyfiles;
const dvar_t *fs_cdpath;
const dvar_t *fs_useOldAssets;
const dvar_t *fs_basegame;
const dvar_t *fs_basepath;
const dvar_t *fs_homepath;
const dvar_t *fs_debug;
#endif
char fs_gamedir[256];
BSSINT fs_loadStack;
unsigned char com_fileAccessed[96];
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
dvar_t *com_dedicated;   /* dvar pointer-blob retype */
#endif
struct scrMemTreePub_t scrMemTreePub;
#if !COD2_PLUS_SERVER  /* defined by src/xanim */
struct XModelDefault g_default;
#endif
const dvar_t *snd_touchStreamFilesOnLoad;
const dvar_t *snd_enableReverb;
const dvar_t *snd_enableStream;
const dvar_t *snd_enable3D;
const dvar_t *snd_enable2D;
const dvar_t *snd_slaveFadeTime;
const dvar_t *snd_volume;
const dvar_t *snd_stereo;
const dvar_t *snd_bits;
const dvar_t *snd_khz;
const dvar_t *snd_errorOnMissing;
#if defined(__x86_64__) || defined(_M_X64)

struct snd_local_t g_snd;
#else
struct snd_local_t g_snd;
#endif
#if defined(COD2_X64) || defined(__x86_64__)
cmd_t cmd_texts[1];   /* x86 blob 12; x64 sizeof 16 (cmd_t data pointer grows) */
#else
unsigned char cmd_texts[12];
#endif
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
int cmd_wait;
BSSINT dvarCount;
BSSINT dvar_modifiedFlags;
dvar_t *sortedDvars;   /* was unsigned char[116] blob; consumer uses it as dvar_t* (dvar=sortedDvars) -- pointer retype */
#endif
BSSINT mss_q3fs;
const dvar_t *mss_3d_provider;   /* dvar pointer-blob retype */
BSSINT visibleEffectCountBolt;
int visibleEffectCountNonBolt;
FxHelper theFxHelpers[1];
BSSINT effectBlockSightCount;
BSSINT cullEffectCountNonBolt;
BSSINT cullEffectCountBolt;
BSSINT initialEffectActiveCountNonBolt;
BSSINT initialEffectActiveCountBolt;
BSSINT privateEffectActiveCountNonBolt;
BSSINT privateEffectActiveCountBolt;
BSSINT effectActiveCount;
BSSINT effectActiveCountNonBolt;
int effectActiveCountBolt;
EffectVisInfo g_effectVisArray[1800];
BSSINT g_effectVisArrayCount;
int *clusterSort;
int effectClusterCount;
volatile qboolean fx_camera_valid;
unsigned char fxSchedulers[128];
#if !COD2_PLUS_SERVER  /* defined by src/bgame */
const dvar_t *player_dmgtimer_flinchTime;
const dvar_t *player_dmgtimer_stumbleTime;
const dvar_t *player_dmgtimer_minScale;
const dvar_t *player_dmgtimer_maxTime;
const dvar_t *player_dmgtimer_timePerPoint;
const dvar_t *player_turnAnims;
const dvar_t *player_backSpeedScale;
const dvar_t *player_strafeSpeedScale;
const dvar_t *player_moveThreshhold;
const dvar_t *player_adsExitDelay;
const dvar_t *player_scopeExitOnDamage;
const dvar_t *player_toggleBinoculars;
const dvar_t *player_breath_snd_delay;
const dvar_t *player_breath_snd_lerp;
const dvar_t *player_breath_gasp_lerp;
const dvar_t *player_breath_hold_lerp;
const dvar_t *player_breath_gasp_scale;
const dvar_t *player_breath_fire_delay;
const dvar_t *player_breath_gasp_time;
const dvar_t *player_breath_hold_time;
const dvar_t *bg_aimSpreadMoveSpeedThreshold;
const dvar_t *bg_bobMax;
const dvar_t *bg_bobAmplitudeProne;
const dvar_t *bg_bobAmplitudeDucked;
const dvar_t *bg_bobAmplitudeStanding;
const dvar_t *bg_swingSpeed;
const dvar_t *bg_foliagesnd_resetinterval;
const dvar_t *bg_foliagesnd_fastinterval;
const dvar_t *bg_foliagesnd_slowinterval;
const dvar_t *bg_foliagesnd_maxspeed;
const dvar_t *bg_foliagesnd_minspeed;
#endif
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
#if defined(COD2_X64)
/* x86 sizeof(clipMap_t)=384; on x64 its many pointer fields grow so the struct is
   larger -> the byte blob put nodes/planes/leafs at wrong offsets and overflowed
   adjacent bss. Retype so the compiler sizes/lays it out for x64. */
clipMap_t cm;
#else
struct clipMap_t cm;
#endif
#endif
/* Consumer is `WeaponDef *bg_weaponDefs[128]` (bg_weapons.c). As a fixed 608-byte
   blob that holds only 76 pointers on x64 (8 bytes each), high weapon indices read
   past it into adjacent BSS -> garbage WeaponDef* -> crash in BG_FindWeaponIndexForName
   on maps that load many weapons (e.g. mp_breakout). Retype so it holds all 128. */
/* Typed on BOTH arches so this storage and the consumer's declaration agree; 128 slots
   is what the consumer declares and exceeds x86's need (was 608B/152 slots). */
#if !COD2_PLUS_SERVER  /* defined by src/bgame */
struct WeaponDef *bg_weaponDefs[128];
#endif
/* Consumer is `struct scrVmPub_t scrVmPub` (scr_vm.c). On x86 sizeof==17184 (exact
   fit: 32 header + function_frame_t[32]*24 + VariableValue[2048]*8). On x64 it is
   17720 -- 536 bytes larger (the 4 leading pointers grow 4->8 and function_frame_t
   grows 24->40). As a fixed x86-sized blob, the script VM's typed writes to its high
   fields (the tail of stack[2048], reached on script-heavy maps) overflow the blob
   into the immediately-following bg_weaponDefs, NULLing every WeaponDef* -> NULL
   deref in BG_FindWeaponIndexForName during script precache on mp_breakout/mp_rhine.
   Retype so the compiler sizes it for the arch. BSS, so binary-compatible on x86. */
#if defined(COD2_X64) || defined(__x86_64__)
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
struct scrVmPub_t scrVmPub;
#endif
#else
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
unsigned char scrVmPub[17184];
#endif
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
int g_script_error_level;
#endif
#if defined(COD2_X64) || defined(__x86_64__)
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
jmp_buf g_script_error[33];   /* x86 blob 2400 (72B jmp_buf); x64 sizeof jmp_buf 256 -> 8448; setjmp/longjmp(g_script_error[level]) overflowed the blob at nesting >=9 */
#endif
#else
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
jmp_buf g_script_error[33];
#endif
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
unsigned char scrVarPub[262240];
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
unsigned char scrVarGlob[1048608];
#endif
#if defined(__x86_64__) || defined(_M_X64)
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
struct scrCompilePub_t scrCompilePub;   /* typed so the x64-wider func_table (intptr_t) is sized correctly */
#endif
#else
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
struct scrCompilePub_t scrCompilePub;
#endif
#endif
#if defined(COD2_X64) || defined(__x86_64__)
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
scrParserPub_t scrParserPub;   /* x86 blob 28; x64 sizeof(scrParserPub_t)=32 -> overflow */
#endif
#else
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
struct scrParserPub_t scrParserPub;
#endif
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
struct scrParserGlob_t scrParserGlob;
#endif
#if defined(__x86_64__) || defined(_M_X64)
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
struct scrAnimPub_t scrAnimPub;   /* typed: xanim_lookup[2][128] of scr_animtree_t grows on x64 (blob was x86-sized 1152) */
#endif
#else
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
struct scrAnimPub_t scrAnimPub;
#endif
#endif
#if defined(__x86_64__) || defined(_M_X64)

struct g_sa_type g_sa;
#else
struct g_sa_type g_sa;
#endif
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
int sys_timeBase;
#endif
unsigned char legacyHacksArray[1792];
struct saLoadObjGlob_type saLoadObjGlob;
int giFilesFound;
/* consumer source_t *sourceFiles[64]: x86-sized pointer-array blob, too small on x64 */
#if defined(COD2_X64) || defined(__x86_64__)
source_t *sourceFiles[64];
#else
source_t * sourceFiles[64];
#endif
define_t *globaldefines;
int numtokens;
#if defined(COD2_X64) || defined(__x86_64__)
WinVars_t g_wv;   /* x86 blob 32; x64 sizeof(WinVars_t)=48 -> overflow */
#else
unsigned char g_wv[32];
#endif
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
unsigned char sys_packetReceived[16480];
#endif
#if defined(COD2_X64)
/* x86 sizeof(GfxScene)=124292; on x64 the GfxEntity/GfxSceneEntity arrays + pointer
   fields grow so the struct is larger -> the byte blob put def.entityCount / sceneEnts /
   def.entities at wrong offsets and R_AddRefEntityToScene's memcpy overflowed adjacent
   bss -> dynamic models (viewmodel/players) never rendered. Retype for x64. */
struct GfxScene scene;
#else
GfxScene scene;
#endif
GfxBackEndData *frontEndDataOut;
SkinBuffers g_skinBuffers[1];
const dvar_t *r_aspectRatio;
BSSINT r_rendererInUse;
const dvar_t *r_rendererPreference;
const dvar_t *r_displayRefresh;
const dvar_t *r_mode;
const dvar_t *r_monitor;
const dvar_t *r_fullscreen;
const dvar_t *r_sse_skinning;
const dvar_t *sys_SSE;
const dvar_t *developer;
const dvar_t *vid_ypos;
const dvar_t *vid_xpos;
const dvar_t *r_testFillEnable;
const dvar_t *r_testFill;
const dvar_t *r_testTransform;
const dvar_t *r_sun_from_dvars;
const dvar_t *r_outdoorFeather;
const dvar_t *r_outdoorDownBias;
const dvar_t *r_outdoorAwayBias;
const dvar_t *r_glowBloomDesaturation;
const dvar_t *r_glowBloomCutoff;
/* consumers const dvar_t *NAME[2]: x86-sized pointer-array blobs, too small on x64 */
#if defined(COD2_X64) || defined(__x86_64__)
const dvar_t *r_glowBloomIntensity[2];
const dvar_t *r_glowSkyBleedIntensity[2];
const dvar_t *r_glowRadius[2];
#else
const dvar_t *r_glowBloomIntensity[2];
const dvar_t *r_glowSkyBleedIntensity[2];
const dvar_t *r_glowRadius[2];
#endif
const dvar_t *r_glow;
const dvar_t *r_distortion;
const dvar_t *r_blur;
const dvar_t *sc_offscreenCasterLodScale;
const dvar_t *sc_offscreenCasterLodBias;
const dvar_t *sc_length;
const dvar_t *sc_shadowOutRate;
const dvar_t *sc_shadowInRate;
const dvar_t *sc_fadeRange;
const dvar_t *sc_wantCountMargin;
const dvar_t *sc_wantCount;
const dvar_t *sc_showDebug;
const dvar_t *sc_showOverlay;
const dvar_t *sc_debugReceiverCount;
const dvar_t *sc_debugCasterCount;
const dvar_t *sc_count;
const dvar_t *sc_blur;
const dvar_t *sc_enable;
const dvar_t *r_forceLod;
const dvar_t *r_lowestLodDist;
const dvar_t *r_lowLodDist;
const dvar_t *r_mediumLodDist;
const dvar_t *r_highLodDist;
const dvar_t *r_showGroundLit;
const dvar_t *r_showFloatZDebug;
const dvar_t *r_showFbColorDebug;
const dvar_t *r_showSModelNames;
const dvar_t *r_showPortals;
const dvar_t *r_portalMinClipArea;
const dvar_t *r_portalWalkLimit;
const dvar_t *r_singleCell;
const dvar_t *r_portalBevelsOnly;
const dvar_t *r_portalBevels;
const dvar_t *r_portalFineCull;
const dvar_t *r_pvsStats;
const dvar_t *r_skipPvs;
const dvar_t *r_lockPvs;
const dvar_t *r_depthPrepassModels;
const dvar_t *r_drawWater;
const dvar_t *r_drawPrimFloor;
const dvar_t *r_drawPrimCap;
const dvar_t *r_dlightLimit;
const dvar_t *r_drawXModels;
const dvar_t *r_drawSModels;
const dvar_t *r_drawBModels;
const dvar_t *r_drawEntities;
const dvar_t *r_drawDecals;
const dvar_t *r_drawWorld;
const dvar_t *r_drawSun;
const dvar_t *r_clearColor2;
const dvar_t *r_clearColor;
const dvar_t *r_aaSamples;
const dvar_t *r_aaAlpha;
const dvar_t *r_swapInterval;
const dvar_t *r_norefresh;
const dvar_t *r_skipBackEnd;
const dvar_t *r_logFile;
const dvar_t *r_objectiveColorDx7Max;
const dvar_t *r_objectiveColorDx7Min;
const dvar_t *r_lightTweakSunDirection;
const dvar_t *r_lightTweakSunDiffuseColor;
const dvar_t *r_lightTweakSunColor;
const dvar_t *r_lightTweakAmbientColor;
const dvar_t *r_lightTweakSunLight;
const dvar_t *r_lightTweakDiffuseFraction;
const dvar_t *r_lightTweakAmbient;
const dvar_t *r_showMissingLightGrid;
const dvar_t *r_showLightGrid;
const dvar_t *r_vc_showlog;
const dvar_t *r_vc_makelog;
const dvar_t *r_railCoreWidth;
const dvar_t *r_xdebug;
const dvar_t *r_showVertCounts;
const dvar_t *r_showSurfCounts;
const dvar_t *r_showTriCounts;
const dvar_t *r_showTris;
const dvar_t *r_cosinePowerMapShift;
const dvar_t *r_specularColorScale;
const dvar_t *r_specularMap;
const dvar_t *r_normalMap;
const dvar_t *r_colorMap;
const dvar_t *r_lightMap;
const dvar_t *r_picmip_spec;
const dvar_t *r_picmip_bump;
const dvar_t *r_picmip;
const dvar_t *r_picmip_manual;
const dvar_t *r_polygonOffsetBias;
const dvar_t *r_polygonOffsetScale;
const dvar_t *r_fog;
const dvar_t *r_zfar;
const dvar_t *r_znear_depthhack;
const dvar_t *r_znear;
const dvar_t *r_lodBias;
const dvar_t *r_lodScale;
const dvar_t *r_smc_enable;
const dvar_t *r_skinCache;
const dvar_t *r_multiGpu;
const dvar_t *r_gpuSync;
const dvar_t *r_optimizeXModels;
const dvar_t *r_optimizeLightmaps;
const dvar_t *r_optimize;
const dvar_t *r_debugEntCounts;
const dvar_t *r_debugShader;
const dvar_t *r_fullbright;
const dvar_t *r_anisotropy;
const dvar_t *r_textureMode;
const dvar_t *r_ignoreHwGamma;
const dvar_t *r_gamma;
const dvar_t *r_overbrightBits;
const dvar_t *r_ignore;
struct DxGlobals dx;
unsigned char vidConfig[64];
refimport_t ri;
r_globals_t rg;
r_global_permanent_t rgp;
int g_disableRendering;
struct DxState dxState;   /* was unsigned char[8580] (x86 size); x64 sizeof is larger -> overflowed into g_disableRendering */
GLuint g_FenceID;
materialCommands_t tess;
r_backEndGlobals_t backEnd;
GfxBackEndData *backEndData;   /* pointer-blob retype */
#if defined(COD2_X64) || defined(__x86_64__)
SunFlareDynamic sunFlareArray[4];   /* x86 blob 228; x64 SunFlareDynamic=64 -> 4*64=256 > 228 overflow */
#else
SunFlareDynamic sunFlareArray[4];
#endif
#if defined(COD2_X64) || defined(__x86_64__)
r_globals_load_t rgl;   /* x86 blob 28; x64 sizeof(r_globals_load_t)=48 -> overflow */
#else
struct r_globals_load_t rgl;
#endif
#if defined(COD2_X64) || defined(__x86_64__)
GfxWorld s_world;   /* x86 blob 640; x64 sizeof(GfxWorld)=656 -> overflow */
#else
GfxWorld s_world;
#endif
#if defined(COD2_X64) || defined(__x86_64__)
lightGlob_type lightGlob;   /* x86 blob 352; x64 sizeof(lightGlob_type)=520 -> overflow */
#else
struct lightGlob_type lightGlob;
#endif
GfxDrawGroupCommands delayedGroup[5];
const dvar_t *r_sun_fx_position;
const dvar_t *r_sunglare_fadeout;
const dvar_t *r_sunglare_fadein;
const dvar_t *r_sunglare_max_lighten;
const dvar_t *r_sunglare_max_angle;
const dvar_t *r_sunglare_min_angle;
const dvar_t *r_sunblind_fadeout;
const dvar_t *r_sunblind_fadein;
const dvar_t *r_sunblind_max_darken;
const dvar_t *r_sunblind_max_angle;
const dvar_t *r_sunblind_min_angle;
const dvar_t *r_sunflare_fadeout;
const dvar_t *r_sunflare_fadein;
const dvar_t *r_sunflare_max_alpha;
const dvar_t *r_sunflare_max_angle;
const dvar_t *r_sunflare_max_size;
const dvar_t *r_sunflare_min_angle;
const dvar_t *r_sunflare_min_size;
const dvar_t *r_sunflare_shader;
const dvar_t *r_sunsprite_size;
const dvar_t *r_sunsprite_shader;
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
char *yytext;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
BSSINT yyleng;
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
BSSINT yynerrs;
#endif
#if defined(COD2_X64) || defined(__x86_64__)
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
stype_t yylval;   /* x86 blob 8; x64 sizeof(stype_t)=16 -> overflow */
#endif
#else
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
stype_t yylval;
#endif
#endif
#if !COD2_PLUS_SCRIPT /* CoD2rev-Plus's src/script defines it */
int yychar;
#endif
const dvar_t *ui_playerProfileAlreadyChosen;
const dvar_t *com_playerProfile;
unsigned char __ZN10CVAOPacket14sGenericPacketE[688];
unsigned char __ZN10CVAOPacket11sAllPacketsE[80];
unsigned char __ZN12CStreamSound10sQTStreamsE[128];
#if defined(COD2_X64)
/* PlayerKeyState: keys[256] of qkey_t (16B on x64 vs 12B x86) -> 292 + 256*16 = 4388.
 * The 3392 was the x86 size; the data.c `keys=playerKeys+292` byte offsets are
 * arch-neutral, but the array must be big enough for the x64 qkey_t stride. */
unsigned char playerKeys[4416];
#else
unsigned char playerKeys[3392];
#endif
field_t g_consoleField;
BSSINT historyLine;
BSSINT nextHistoryLine;
field_t historyEditLines[32];
const dvar_t *cg_weaponrightbone;
const dvar_t *cg_weaponleftbone;
const dvar_t *cg_blood;
const dvar_t *cg_headIconMinScreenRadius;
const dvar_t *cg_constantSizeHeadIcons;
const dvar_t *cg_voiceIconSize;
const dvar_t *cg_connectionIconSize;
const dvar_t *cg_scriptIconSize;
const dvar_t *cg_youInKillCamSize;
const dvar_t *cg_shock_mouse_fadeTime;
const dvar_t *cg_shock_mouse_sensitivityscale;
const dvar_t *cg_shock_mouse_maxyawspeed;
const dvar_t *cg_shock_mouse_maxpitchspeed;
const dvar_t *cg_shock_mouse;
const dvar_t *cg_shock_volume_shellshock;
const dvar_t *cg_shock_volume_announcer;
const dvar_t *cg_shock_volume_music;
const dvar_t *cg_shock_volume_local;
const dvar_t *cg_shock_volume_body;
const dvar_t *cg_shock_volume_item;
const dvar_t *cg_shock_volume_voice;
const dvar_t *cg_shock_volume_weapon;
const dvar_t *cg_shock_volume_menu;
const dvar_t *cg_shock_volume_auto2d;
const dvar_t *cg_shock_volume_auto;
const dvar_t *cg_shock_soundModEndDelay;
const dvar_t *cg_shock_soundWetLevel;
const dvar_t *cg_shock_soundDryLevel;
const dvar_t *cg_shock_soundRoomType;
const dvar_t *cg_shock_soundLoopEndDelay;
const dvar_t *cg_shock_soundLoopFadeTime;
const dvar_t *cg_shock_soundFadeOutTime;
const dvar_t *cg_shock_soundFadeInTime;
const dvar_t *cg_shock_sound;
const dvar_t *cg_shock_viewKickRadius;
const dvar_t *cg_shock_viewKickFadeTime;
const dvar_t *cg_shock_viewKickPeriod;
const dvar_t *cg_shock_screenBlendFadeTime;
const dvar_t *cg_shock_screenBlendTime;
const dvar_t *cg_scoreboardItemHeight;
const dvar_t *cg_scoreboardBannerHeight;
const dvar_t *cg_scoreboardScrollStep;
const dvar_t *cg_drawGameMessages;
const dvar_t *cg_gameBoldMessageWidth;
const dvar_t *cg_gameMessageWidth;
const dvar_t *cg_subtitleCharHeight;
const dvar_t *cg_subtitlePosY;
const dvar_t *cg_subtitlePosX;
const dvar_t *cg_subtitleWidthWidescreen;
const dvar_t *cg_subtitleWidthStandard;
const dvar_t *cg_subtitleMinTime;
const dvar_t *cg_subtitles;
const dvar_t *cg_minicon;
const dvar_t *cg_developer;
const dvar_t *cg_dumpAnims;
const dvar_t *cg_descriptiveText;
const dvar_t *cg_voiceSpriteTime;
const dvar_t *cg_noTaunt;
const dvar_t *cg_predictItems;
const dvar_t *cg_paused;
const dvar_t *cg_chatHeight;
const dvar_t *cg_chatTime;
const dvar_t *cg_synchronousClients;
const dvar_t *cg_thirdPersonAngle;
const dvar_t *cg_thirdPersonRange;
const dvar_t *cg_thirdPerson;
const dvar_t *cg_fovMin;
const dvar_t *cg_fovScale;
const dvar_t *cg_fov;
const dvar_t *cg_tracerScaleDistRange;
const dvar_t *cg_tracerScaleMinDist;
const dvar_t *cg_tracerScale;
const dvar_t *cg_tracerSpeed;
const dvar_t *cg_tracerLength;
const dvar_t *cg_tracerWidth;
const dvar_t *cg_tracerChance;
const dvar_t *cg_gun_move_minspeed;
const dvar_t *cg_gun_move_rate;
const dvar_t *cg_gun_ofs_u;
const dvar_t *cg_gun_ofs_r;
const dvar_t *cg_gun_ofs_f;
const dvar_t *cg_gun_move_u;
const dvar_t *cg_gun_move_r;
const dvar_t *cg_gun_move_f;
const dvar_t *cg_gun_z;
const dvar_t *cg_gun_y;
const dvar_t *cg_gun_x;
const dvar_t *cg_hintFadeTime;
const dvar_t *cg_cursorHints;
const dvar_t *cg_drawGun;
BSSINT cg_viewsize;
const dvar_t *cg_brass;
const dvar_t *cg_marksLimit;
const dvar_t *cg_marks;
const dvar_t *cg_footsteps;
const dvar_t *cg_showmiss;
const dvar_t *cg_nopredict;
const dvar_t *cg_errorDecay;
const dvar_t *cg_debugEvents;
const dvar_t *cg_debugPosition;
const dvar_t *cg_drawMantleHint;
const dvar_t *cg_drawBreathHint;
const dvar_t *cg_drawHealth;
const dvar_t *cg_teamChatsOnly;
const dvar_t *cg_draw2D;
const dvar_t *cg_crosshairEnemyColor;
const dvar_t *cg_crosshairDynamic;
const dvar_t *cg_crosshairAlphaMin;
const dvar_t *cg_crosshairAlpha;
const dvar_t *cg_weaponCycleDelay;
const dvar_t *cg_drawLagometer;
const dvar_t *cg_hudProneY;
const dvar_t *cg_centerPrintY;
const dvar_t *cg_hudSayPosition;
const dvar_t *cg_hudChatPosition;
const dvar_t *cg_hudGrenadePointerPulseMin;
const dvar_t *cg_hudGrenadePointerPulseMax;
const dvar_t *cg_hudGrenadePointerPulseFreq;
const dvar_t *cg_hudGrenadePointerPivot;
const dvar_t *cg_hudGrenadePointerWidth;
const dvar_t *cg_hudGrenadePointerHeight;
const dvar_t *cg_hudGrenadeIconWidth;
const dvar_t *cg_hudGrenadeIconHeight;
const dvar_t *cg_hudGrenadeIconOffset;
const dvar_t *cg_hudGrenadeIconMaxHeight;
const dvar_t *cg_hudGrenadeIconMaxRange;
const dvar_t *cg_hudGrenadeIconInScope;
const dvar_t *cg_hudDamageIconInScope;
const dvar_t *cg_hudDamageIconTime;
const dvar_t *cg_hudDamageIconOffset;
const dvar_t *cg_hudDamageIconHeight;
const dvar_t *cg_hudDamageIconWidth;
const dvar_t *cg_hudStanceHintPrints;
const dvar_t *cg_hudStanceFlash;
const dvar_t *cg_hudObjectiveMinAlpha;
const dvar_t *cg_hudObjectiveMaxRange;
const dvar_t *cg_hudObjectiveMinHeight;
const dvar_t *cg_hudCompassSoundPingFadeTime;
const dvar_t *cg_hudCompassSpringyPointers;
const dvar_t *cg_hudCompassMinRadius;
const dvar_t *cg_hudCompassMinRange;
const dvar_t *cg_hudCompassMaxRange;
const dvar_t *cg_hudCompassSize;
const dvar_t *cg_drawCrosshairNamesPosY;
const dvar_t *cg_drawCrosshairNamesPosX;
const dvar_t *cg_drawCrosshairNames;
const dvar_t *cg_drawTurretCrosshair;
const dvar_t *cg_drawCrosshair;
const dvar_t *cg_drawSnapshot;
const dvar_t *cg_drawScriptUsage;
const dvar_t *cg_drawSoundOverlay;
const dvar_t *cg_drawMaterial;
const dvar_t *cg_drawFPS;
const dvar_t *cg_centertime;
#if defined(COD2_X64)
/* x86 sizeof(displayContextDef_t)=640; on x64 the Menus[128]/menuStack[16] pointer
   arrays grow so the struct is ~1216 -> the byte blob put menuCount/Menus at the
   wrong offsets (garbage menuCount). Retype so the compiler sizes it for x64. */
displayContextDef_t cgDC;
#else
displayContextDef_t cgDC;
#endif
BSSINT old_com_frameTime;
unsigned int frame_msec;
unsigned char re_0121c6a0[352];
#if COD2_IS_PATCH_13
ping_t cl_pinglist[16];
#else
unsigned char cl_pinglist[16704];
#endif
Bool g_waitingForServer;
#if defined(COD2_X64) || defined(__x86_64__)
clientStatic_t cls;   /* x86 blob 2755264; x64 sizeof(clientStatic_t)=2756928 -> overflow */
#elif COD2_IS_PATCH_13
unsigned char cls[0x2c8a18];
#else
unsigned char cls[2755264];
#endif

unsigned char clientConnections[sizeof(clientConnection_t)];

#if defined(COD2_X64) || defined(__x86_64__)
clientActive_t clients[1];   /* x86 blob 1662356; x64 sizeof(clientActive_t)=1662368 -> overflow */
#else
unsigned char clients[1662356];
#endif
const dvar_t *cl_voice;
BSSINT name_01683858;
const dvar_t *nextdemo;
const dvar_t *cl_ingame;
const dvar_t *fx_profile;
const dvar_t *fx_visMinTraceDist;
const dvar_t *fx_count;
const dvar_t *fx_freeze;
const dvar_t *fx_debugBolt;
const dvar_t *fx_debug;
const dvar_t *fx_sort;
const dvar_t *fx_cull;
const dvar_t *fx_draw;
const dvar_t *fx_enable;
const dvar_t *cl_serverStatusResendTime;
const dvar_t *cl_inGameVideo;
const dvar_t *cl_allowDownload;
const dvar_t *cl_motdString;
const dvar_t *cl_activeAction;
const dvar_t *m_filter;
const dvar_t *m_side;
const dvar_t *m_forward;
const dvar_t *m_yaw;
const dvar_t *m_pitch;
const dvar_t *cl_showMouseRate;
const dvar_t *cl_mouseAccel;
const dvar_t *cl_sensitivity;
const dvar_t *cl_freelook;
const dvar_t *cl_forceavidemo;
const dvar_t *cl_avidemo;
const dvar_t *cl_showServerCommands;
const dvar_t *cl_showSend;
const dvar_t *cl_shownuments;
const dvar_t *cl_freezeDemo;
const dvar_t *cl_showTimeDelta;
const dvar_t *cl_packetdup;
const dvar_t *cl_maxpackets;
const dvar_t *cl_connectTimeout;
const dvar_t *cl_timeout;
const dvar_t *cl_noprint;
const dvar_t *cl_nodelta;
#if !COD2_PLUS_SERVER  /* defined by src/server */
qboolean gameInitialized;
#endif
const dvar_t *ui_playerProfileNameNew;
const dvar_t *ui_playerProfileSelected;
const dvar_t *ui_playerProfileCount;
const dvar_t *ui_serverStatusTimeOut;
const dvar_t *ui_currentMap;
const dvar_t *ui_browserKillcam;
const dvar_t *ui_browserFriendlyfire;
const dvar_t *ui_browserMod;
const dvar_t *ui_browserShowDedicated;
const dvar_t *ui_browserShowPure;
const dvar_t *ui_browserShowNoPassword;
const dvar_t *ui_browserShowPassword;
const dvar_t *ui_browserShowEmpty;
const dvar_t *ui_browserShowFull;
const dvar_t *ui_currentNetMap;
const dvar_t *ui_dedicated;
const dvar_t *ui_joinGameType;
const dvar_t *ui_netGameTypeName;
const dvar_t *ui_netGameType;
const dvar_t *ui_netSource;
const dvar_t *ui_extraBigFont;
const dvar_t *ui_bigFont;
const dvar_t *ui_smallFont;
const dvar_t *ui_gametype;
#if defined(COD2_X64) || defined(__x86_64__)
uiInfo_t uiInfoArray[1];   /* x86 blob 4288; x64 sizeof(uiInfo_t)=5112 -> overflow */
#else
unsigned char uiInfoArray[4288];
#endif
#if defined(COD2_X64) || defined(__x86_64__)
sharedUiInfo_t sharedUiInfo;   /* x86 blob 115392; x64 sizeof(sharedUiInfo_t)=123184 -> overflow */
#else
sharedUiInfo_t sharedUiInfo;
#endif
#if !COD2_PLUS_SERVER  /* defined by CoD2rev-Plus's universal, qcommon or platform code */
const dvar_t *net_lanauthorize;
const dvar_t *net_showprofile;
const dvar_t *net_profile;
const dvar_t *packetDebug;
const dvar_t *showdrop;
const dvar_t *showpackets;
#endif
#if !COD2_PLUS_SERVER  /* defined by src/server */
const dvar_t *sv_allowedClan2;
const dvar_t *sv_allowedClan1;
const dvar_t *sv_referencedIwdNames;
const dvar_t *sv_referencedIwds;
const dvar_t *sv_iwdNames;
const dvar_t *sv_iwds;
const dvar_t *sv_voiceQuality;
const dvar_t *sv_voice;
BSSINT sv_disableClientConsole;
const dvar_t *sv_kickBanTime;
const dvar_t *sv_mapRotationCurrent;
const dvar_t *sv_mapRotation;
const dvar_t *sv_showAverageBPS;
const dvar_t *sv_packet_info;
const dvar_t *sv_showCommands;
const dvar_t *sv_allowAnonymous;
const dvar_t *sv_cheats;
const dvar_t *sv_floodProtect;
const dvar_t *sv_pure;
const dvar_t *sv_debugReliableCmds;
const dvar_t *sv_debugRate;
const dvar_t *sv_gametype;
const dvar_t *sv_maxPing;
const dvar_t *sv_minPing;
const dvar_t *sv_maxRate;
const dvar_t *sv_serverid;
const dvar_t *sv_mapname;
const dvar_t *sv_padPackets;
const dvar_t *sv_reconnectlimit;
const dvar_t *sv_hostname;
const dvar_t *sv_privateClients;
BSSINT sv_maxclients;
const dvar_t *sv_allowDownload;
const dvar_t *sv_privatePassword;
const dvar_t *rcon_password;
const dvar_t *sv_zombietime;
const dvar_t *sv_timeout;
const dvar_t *sv_fps;
#endif
/* Typed in every build: 1.0's serverStatic_t is 0xc108 bytes, and the
   41216 once reserved for it let challenges, redirectAddress and tempBans
   run into sv's configstrings. */
#if !COD2_PLUS_SERVER  /* defined by src/server */
server_t sv;
serverStatic_t svs;
#endif
const dvar_t *con_restricted;
const dvar_t *con_miniconlines;
const dvar_t *con_minicontime;
const dvar_t *con_boldgamemessagetime;
const dvar_t *con_gamemessagetime;
#if COD2_IS_PATCH_13
unsigned char cl_serverStatusList[0x20280];
#else
unsigned char cl_serverStatusList[131680];
#endif
qboolean scr_initialized;
MarkVertAssemblyBuffer markVerts;
MarkPoly cg_markPolys[1024];
MarkPoly *cg_freeMarkPolys;
unsigned char buf_017dd900[128];
const dvar_t *cl_bypassMouseInput;
const dvar_t *cl_talking;
const dvar_t *cl_anglespeedkey;
const dvar_t *cl_pitchspeed;
const dvar_t *cl_yawspeed;
const dvar_t *cl_stanceHoldTime;
const dvar_t *cl_analog_attack_threshold;
const dvar_t *hud_deathQuoteFadeTime;
const dvar_t *hud_health_pulserate_critical;
const dvar_t *hud_health_pulserate_injured;
const dvar_t *hud_health_startpulse_critical;
const dvar_t *hud_health_startpulse_injured;
const dvar_t *hud_fade_offhand;
const dvar_t *hud_fade_stance;
const dvar_t *hud_fade_compass;
const dvar_t *hud_fade_healthbar;
const dvar_t *hud_fade_ammodisplay;
#if !COD2_PLUS_SERVER  /* defined by src/game */
#if defined(COD2_X64) || defined(__x86_64__)
scr_data_t g_scr_data;   /* x86 blob 14080; x64 sizeof(scr_data_t)=14328 -> overflow */
#else
scr_data_t g_scr_data;
#endif
#endif
/* consumer keywordHash_t *itemParseKeywordHash[512]: x86-sized pointer-array blob, too small on x64 */
#if defined(COD2_X64) || defined(__x86_64__)
keywordHash_t *itemParseKeywordHash[512];
#else
keywordHash_t *itemParseKeywordHash[512];
#endif
#if !COD2_PLUS_SERVER  /* defined by src/game */
const dvar_t *g_dumpAnims;
const dvar_t *g_voteAbstainWeight;
const dvar_t *g_oldVoting;
const dvar_t *g_antilag;
const dvar_t *player_meleeHeight;
const dvar_t *player_meleeWidth;
const dvar_t *player_meleeRange;
const dvar_t *g_friendlyNameDist;
const dvar_t *g_friendlyfireDist;
const dvar_t *g_debugLocDamage;
#endif
const dvar_t *g_NoScriptSpam;
#if !COD2_PLUS_SERVER  /* defined by src/game */
const dvar_t *g_TeamColor_Axis;
const dvar_t *g_TeamColor_Allies;
const dvar_t *g_TeamName_Axis;
const dvar_t *g_TeamName_Allies;
const dvar_t *g_ScoresBanner_Spectators;
const dvar_t *g_ScoresBanner_None;
const dvar_t *g_ScoresBanner_Axis;
const dvar_t *g_ScoresBanner_Allies;
const dvar_t *g_smoothClients;
const dvar_t *g_banIPs;
#endif
#ifdef __EMSCRIPTEN__

extern int g_banIPs_dvar __attribute__((alias("g_banIPs")));
#elif defined(_MSC_VER)
/* MSVC equivalent of the GAS .set alias: resolve the (otherwise undefined)
 * alias to the target at link. x86 C symbols carry one leading underscore. */
COD2_ALT("g_banIPs_dvar", "g_banIPs")
#else
__asm__(".globl g_banIPs_dvar\n.set g_banIPs_dvar, g_banIPs\n");
#endif
#if !COD2_PLUS_SERVER  /* defined by src/game */
const dvar_t *g_listEntity;
const dvar_t *g_deadChat;
const dvar_t *g_allowVote;
const dvar_t *g_logSync;
const dvar_t *g_log;
const dvar_t *g_voiceChatTalkingDuration;
const dvar_t *voice_deadChat;
const dvar_t *voice_global;
const dvar_t *voice_localEcho;
const dvar_t *g_mantleBlockTimeBuffer;
const dvar_t *g_clonePlayerMaxVelocity;
const dvar_t *g_dropUpSpeedRand;
const dvar_t *g_dropUpSpeedBase;
const dvar_t *g_dropForwardSpeed;
const dvar_t *g_playerCollisionEjectSpeed;
const dvar_t *g_synchronousClients;
const dvar_t *g_motd;
const dvar_t *g_maxDroppedWeapons;
const dvar_t *g_weaponAmmoPools;
const dvar_t *g_debugBullets;
const dvar_t *g_debugDamage;
const dvar_t *g_inactivity;
const dvar_t *g_useholdspawndelay;
const dvar_t *g_useholdtime;
const dvar_t *g_knockback;
const dvar_t *g_cheats;
const dvar_t *g_gravity;
const dvar_t *g_speed;
const dvar_t *g_dedicated;
const dvar_t *g_maxclients;
const dvar_t *g_password;
#endif
unsigned char g_gametype_017e1a58[40];
/* 573440 = 1024 * 560 (x86 sizeof(gentity_s)). On x64 the struct is larger, so the blob must grow
   to hold all 1024 entity slots (indices up to 0x3FF incl. the world entity 1022) -- otherwise high
   indices overflow into adjacent BSS. BSS, so re-sizing is binary-compatible. */
#if !COD2_PLUS_SERVER  /* defined by src/game */
#if defined(COD2_X64) || defined(__x86_64__)
struct gentity_s g_entities[1024];
#else
unsigned char g_entities[573440];
#endif
#endif
/* 817664 = x86 sizeof(bgs_t). On x64 bgs_t is larger (embedded animScriptData and
   clientinfo[] hold pointers that grow 4->8), so as a fixed x86-sized blob its high
   fields (AllocXAnim @ +1062856, clientinfo[]) overflow into the adjacent BSS global:
   G_FreeEntity's memset of an entity slot was landing on level_bgs.AllocXAnim, NULLing
   it and crashing anim-tree precache on any map with a misc_model (e.g. mp_carentan).
   Retype so the compiler sizes it for the arch. BSS, so binary-compatible on x86. */
#if !COD2_PLUS_SERVER  /* defined by src/bgame */
#if defined(COD2_X64) || defined(__x86_64__)
struct bgs_t level_bgs;
#else
bgs_t level_bgs;
#endif
#endif
/* 13952 = the original 32-bit symbol size (MSVC x86 sizeof(level_locals_t)=13860
   fits with tail slack). On x64 sizeof(level_locals_t)=14544 -- 592 bytes larger
   as its pointer/pointer-array fields grow (clients/gentities/firstFreeEnt/
   lastFreeEnt, droppedWeaponCue[32], openScriptIOFileBuffers[1], plus embedded
   SpawnVar.spawnVars[64][2]). As a fixed x86-sized blob, G_InitGame's
   `memset(&level,0,sizeof(level))` and the per-trigger writes into
   level.currentTriggerList[256] overflow the blob's 592-byte tail into the
   adjacent BSS symbol (level_bgs in the current link) -- a latent corruption of
   the same class as the level_bgs->g_entities bug. (This is NOT the mp_breakout/
   mp_rhine weapon-precache crash; that is scrVmPub->bg_weaponDefs above.) Retype so
   the compiler sizes it for the arch. BSS, so binary-compatible on x86. */
#if !COD2_PLUS_SERVER  /* defined by src/game */
#if defined(COD2_X64) || defined(__x86_64__)
struct level_locals_t level;
#else
struct level_locals_t level;
#endif
#endif
qboolean itemRegistered[256];
#if !COD2_PLUS_SERVER  /* defined by src/game */
game_hudelem_t g_hudelems[1024];
#endif
unsigned char __ZN12UI_Component1gE[224];
#if !COD2_PLUS_SERVER  /* defined by src/game */
float g_fHitLocDamageMult[19];
scr_const_t scr_const;
#endif
struct lagometer_t lagometer;
int cl_connectedToPureServer;
BSSINT removeMeWhenMPStopsCrashingInHere;
vec3_t ejectBrassCasingOrigin;
localEntity_t *cg_freeLocalEntities;
unsigned char cg_eachClientFreeLocalEntities[28];
localEntity_t cg_eachClientActiveLocalEntities[1];
float levelSamples[6];
float voice_current_voicelevel;
char old_rec_source[256];
BSSINT mic_current_reclevel;
BSSINT mic_old_reclevel;
const dvar_t *winvoice_mic_scaler;
const dvar_t *winvoice_save_voice;
const dvar_t *winvoice_mic_reclevel;
const dvar_t *winvoice_mic_mute;
short int partial_audio_buffer[640];
char enc_buffer[4096];
int g_decode_frame_size;
unsigned char current_audioCallback[128];
BSSINT catch_exception_raise;
BSSINT catch_exception_raise_state;
BSSINT catch_exception_raise_state_identity;
BSSINT clock_alarm_reply;
BSSINT do_mach_notify_dead_name;
BSSINT do_mach_notify_no_senders;
BSSINT do_mach_notify_port_deleted;
BSSINT do_mach_notify_send_once;
BSSINT do_seqnos_mach_notify_dead_name;
BSSINT do_seqnos_mach_notify_no_senders;
BSSINT do_seqnos_mach_notify_port_deleted;
BSSINT do_seqnos_mach_notify_send_once;
BSSINT receive_samples;
