#ifndef COD2_COMPAT_SERVER_DECLS_H
#define COD2_COMPAT_SERVER_DECLS_H

/* Engine functions src/compat/server_missing.cpp calls that CoD2rev-Plus's
 * headers do not declare (they are static there); the signatures are the
 * engine's. */
void FS_BuildOSPath( const char *base, const char *game, const char *qpath, char *ospath );  /* src/PC/universal/com_files.c */

/* CoD2rev-Plus's src/bgame defines these without declaring them in a header;
 * the engine calls them (cgame_mp, client_mp). The signatures are copied from
 * the definitions. */
bool PM_IsBinocularsADS( const playerState_t *ps );                      /* bg_weapons.cpp */
int BG_GetTotalAmmoReserve( const playerState_t *ps, int weaponIndex );  /* bg_weapons.cpp */
int BG_GetFirstEquippedOffhand( const playerState_t *ps, int offhandClass );  /* bg_weapons.cpp */
void CG_SetWeaponDefToDefaultWeapon( int iWeapon );                      /* bg_weapons.cpp */

#endif
