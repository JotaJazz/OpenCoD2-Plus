#ifndef COD2_COMPAT_SERVER_DECLS_H
#define COD2_COMPAT_SERVER_DECLS_H

/* CoD2rev-Plus's src/bgame defines these without declaring them in a header;
 * the engine calls them (cgame_mp, client_mp). The signatures are copied from
 * the definitions. */
bool PM_IsBinocularsADS( const playerState_t *ps );                      /* bg_weapons.cpp */
int BG_GetTotalAmmoReserve( const playerState_t *ps, int weaponIndex );  /* bg_weapons.cpp */
int BG_GetFirstEquippedOffhand( const playerState_t *ps, int offhandClass );  /* bg_weapons.cpp */
void CG_SetWeaponDefToDefaultWeapon( int iWeapon );                      /* bg_weapons.cpp */

/* CoD2rev-Plus's universal, qcommon, stringed and platform code define these
 * without declaring them in a header; the engine calls them by their C names.
 * The signatures are copied from the definitions. */
int CM_NumInlineModels( void );                                                        /* qcommon/cm_load.cpp */
void Com_CheckSetRecommended( void );                                                 /* qcommon/common.cpp */
char Com_GetDecimalDelimiter( void );                                                 /* qcommon/common.cpp */
float Com_GetTimescaleForSnd( void );                                                 /* qcommon/common.cpp */
void Com_PumpMessageLoop( void );                                                     /* qcommon/common.cpp */
void Com_SetRecommended( int restart );                                               /* qcommon/common.cpp */
void Com_WriteCDKey( void );                                                          /* qcommon/common.cpp */
void Com_ClientDObjClearAllSkel( void );                                              /* qcommon/dobj_management.cpp */
void Com_ClientDObjCreate( DObjModel_s *dobjModels, unsigned short numModels, XAnimTree_s *tree, int handle );  /* qcommon/dobj_management.cpp */
DObj* Com_GetClientDObj( int handle );                                                /* qcommon/dobj_management.cpp */
void Com_SafeClientDObjFree( int handle );                                            /* qcommon/dobj_management.cpp */
char *MSG_ReadBigString( msg_t *msg );                                                /* qcommon/msg_mp.cpp */
void MSG_WriteDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to );  /* qcommon/msg_mp.cpp */
int NET_CompareAdrSigned( netadr_t *a, netadr_t *b );                                 /* qcommon/net_chan_mp.cpp */
void Net_DisplayProfile( void );                                                      /* qcommon/net_chan_mp.cpp */
qboolean NET_OutOfBandData( netsrc_t sock, netadr_t adr, byte *format, int len );     /* qcommon/net_chan_mp.cpp */
void Dvar_SetVec4ByName( const char *dvarName, float x, float y, float z, float w );  /* universal/dvar.cpp */
void FS_BuildOSPath( const char *base, const char *game, const char *qpath, char* ospath );  /* universal/com_files.cpp */
void FS_FileClose( FILE* stream );                                                    /* universal/com_files.cpp */
size_t FS_FileRead( void* ptr, size_t len, FILE* stream );                            /* universal/com_files.cpp */
int FS_FileSeek( FILE* file, int offset, int whence );                                /* universal/com_files.cpp */
size_t FS_FileWrite( const void* ptr, size_t len, FILE* stream );                     /* universal/com_files.cpp */
int FS_FOpenFileReadStream( const char *filename, fileHandle_t *file, qboolean uniqueFILE );  /* universal/com_files.cpp */
int SEH_GetCurrentLanguage( void );                                                   /* stringed/stringed_hooks.cpp */
qboolean SEH_GetLanguageIndexForName( const char* language, int *langindex );         /* stringed/stringed_hooks.cpp */
const char *SEH_GetLanguageName( const int iLanguage );                               /* stringed/stringed_hooks.cpp */
void Field_Clear( field_t *edit );                                                    /* unix/unix_main.cpp */
void Sys_BeginStreamedFile( fileHandle_t f, int readAhead );                          /* unix/unix_main.cpp */
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );  /* unix/unix_main.cpp */
int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f );            /* unix/unix_main.cpp */
void Sys_ShowIP( void );                                                              /* unix/unix_net.cpp */

#endif
