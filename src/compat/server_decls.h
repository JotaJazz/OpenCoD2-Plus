#ifndef COD2_COMPAT_SERVER_DECLS_H
#define COD2_COMPAT_SERVER_DECLS_H

/* Engine functions src/compat/server_missing.cpp calls that CoD2rev-Plus's
 * headers do not declare (they are static there); the signatures are the
 * engine's. */
void FS_BuildOSPath( const char *base, const char *game, const char *qpath, char *ospath );  /* src/PC/universal/com_files.c */

#endif
