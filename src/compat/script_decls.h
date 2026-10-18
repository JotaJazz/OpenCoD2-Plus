#ifndef COD2_COMPAT_SCRIPT_DECLS_H
#define COD2_COMPAT_SCRIPT_DECLS_H

/* Functions CoD2rev-Plus's src/script defines and the engine calls, but no
 * header of it declares; the signatures are copied from the definitions. */
unsigned int Scr_GetNumScriptThreads();  /* scr_vm.cpp */
unsigned int Scr_GetNumScriptVars();     /* scr_variable.cpp */
int Scr_GetStringUsage();                /* scr_memorytree.cpp */
void Scr_ShutdownGameStrings();          /* scr_stringlist.cpp */

#endif
