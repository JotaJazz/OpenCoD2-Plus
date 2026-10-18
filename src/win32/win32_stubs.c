extern const unsigned char declEnd[20] = { 0xff,0,0,0, 0x11,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };
int g_dip_drawflag_zero;
int g_dip_gl_draw;
int g_dip_is_tri;
int g_dip_numelems_zero;
int g_fp_enable_count;

#if !COD2_PLUS_SERVER  /* the dedicated server's bridge stops on it (test/server_fatal.txt) */
void CMacGameEngine_DrawSplashScreen(void) {}
#endif
#ifdef W32_CLIENT

int Linux_PollInputEvent(void *event) { (void)event; return 0; }
#else
void IN_Frame(void) {}
#endif
/* AddDurationToAbsolute/MPDelayUntil/MacDisplay_CreateScreenContext/
 * SDL_GL_SwapWindowDirect/UpTime have real defs in macos_compat.c; the
 * CDirect3D*D0Ev/D1Ev destructors in the reconstructed CDirect3D*.c -- stubbing
 * them here is what required /FORCE:MULTIPLE, so they're dropped. */
void UpdateSystemActivity(void) {}
#ifndef W32_CLIENT
/* macos_compat.c is client-only, so the headless server needs these two. */
void *MacDisplay_CreateScreenContext(int depth, int windowed, int stencil, int multiSample, int fsaa, int *hasAux)
{
    (void)depth; (void)windowed; (void)stencil; (void)multiSample; (void)fsaa; (void)hasAux;
    return 0;
}
void SDL_GL_SwapWindowDirect(void) {}
#endif
void __ZNSs4_Rep11_S_terminalE(void) {}
void __ZNSs6appendEPKcm(void) {}
void __ZNSs6assignEPKcm(void) {}
void __ZNSs7reserveEm(void) {}
void __ZNSt15_List_node_base4hookEPS_(void) {}
void __ZNSt15_List_node_base6unhookEv(void) {}
void __ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base(void) {}
void __ZSt28_Rb_tree_rebalance_for_erasePSt18_Rb_tree_node_baseRS_(void) {}
void __ZTIl(void) {}
/* ___toupper has a real def in macos_compat.c */

void aglDestroyContext(void) {}
void aglSetCurrentContext(void) {}
void aglSetDrawable(void) {}
void aglSwapBuffers(void) {}

#ifndef W32_CLIENT
void glActiveTextureARB(void) {}
void glAlphaFunc(void) {}
void glBindProgramARB(void) {}
void glBindTexture(void) {}
void glBindVertexArray(void) {}
void glBlendFunc(void) {}
void glBlendFuncSeparateEXT(void) {}
void glClear(void) {}
void glClearColor(void) {}
void glClearDepth(void) {}
void glClearStencil(void) {}
void glClientActiveTextureARB(void) {}
void glColor4f(void) {}
void glColorMask(void) {}
void glColorPointer(void) {}
void glCompressedTexImage2DARB(void) {}
void glCompressedTexImage3DARB(void) {}
void glCompressedTexSubImage2D(void) {}
void glCompressedTexSubImage3D(void) {}
void glDeleteFencesAPPLE(void) {}
void glDeleteProgramsARB(void) {}
void glDeleteTextures(void) {}
void glDepthFunc(void) {}
void glDepthMask(void) {}
void glDepthRange(void) {}
void glDisable(void) {}
void glDisableClientState(void) {}
void glDrawBuffer(void) {}
void glDrawElements(void) {}
void glEnable(void) {}
void glEnableClientState(void) {}
void glFogfv(void) {}
void glGenFencesAPPLE(void) {}
void glGenProgramsARB(void) {}
void glGenTextures(void) {}
void glGenVertexArraysAPPLE(void) {}
void glGetBooleanv(void) {}
void glGetError(void) {}
void glGetFloatv(void) {}
void glGetIntegerv(void) {}
void glGetTexImage(void) {}
void glGetTexLevelParameteriv(void) {}
void glLoadIdentity(void) {}
void glLoadMatrixf(void) {}
void glMatrixMode(void) {}
void glNormalPointer(void) {}
void glOrtho(void) {}
void glPolygonOffset(void) {}
void glPopAttrib(void) {}
void glPopClientAttrib(void) {}
void glProgramEnvParameter4fvARB(void) {}
void glProgramStringARB(void) {}
void glPushAttrib(void) {}
void glPushClientAttrib(void) {}
void glReadBuffer(void) {}
void glReadPixels(void) {}
void glScalef(void) {}
void glSetFenceAPPLE(void) {}
void glStencilMask(void) {}
void glTestFenceAPPLE(void) {}
void glTexCoordPointer(void) {}
void glTexEnvf(void) {}
void glTexEnvfv(void) {}
void glTexEnvi(void) {}
void glTexImage2D(void) {}
void glTexImage3D(void) {}
void glTexParameteri(void) {}
void glTexSubImage2D(void) {}
void glTexSubImage3D(void) {}
void glVertexArrayParameteriAPPLE(void) {}
void glVertexPointer(void) {}
void glViewport(void) {}
void sdl_gl_height(void) {}
void sdl_gl_width(void) {}
void sdl_gl_window(void) {}
#endif
int ioctl(int fd, unsigned long request, void *arg) { (void)fd; (void)request; (void)arg; return -1; }
void setjmp(void) {}

void _ZN16CDirect3DTextureD0Ev(void){} void _ZN16CDirect3DTextureD1Ev(void){}
void _ZN20CDirect3DCubeTextureD0Ev(void){} void _ZN20CDirect3DCubeTextureD1Ev(void){}
void _ZN21CDirect3DVertexShaderD0Ev(void){} void _ZN21CDirect3DVertexShaderD1Ev(void){}

void FD_ZERO(void *s){ if(s)*(unsigned*)s=0; }
void FD_SET(unsigned fd, void *s){ unsigned *c=(unsigned*)s; if(*c<64){((unsigned*)((char*)s+4))[*c]=fd;(*c)++;} }

void *curl_easy_init(void){return 0;}
int   curl_easy_setopt(void*a,int b,...){(void)a;(void)b;return 0;}
int   curl_easy_getinfo(void*a,int b,...){(void)a;(void)b;return 0;}
void  curl_easy_cleanup(void*a){(void)a;}
const char *curl_easy_strerror(int c){(void)c;return "";}
void *curl_multi_init(void){return 0;}
int   curl_multi_add_handle(void*a,void*b){(void)a;(void)b;return 0;}
int   curl_multi_remove_handle(void*a,void*b){(void)a;(void)b;return 0;}
int   curl_multi_perform(void*a,int*b){(void)a;if(b)*b=0;return 0;}
void *curl_multi_info_read(void*a,int*b){(void)a;if(b)*b=0;return 0;}
#if !COD2_PLUS_SERVER  /* the dedicated server's bridge stops on it (test/server_fatal.txt) */
void  Sys_OpenURL(const char *url, int activate){(void)url;(void)activate;}
#endif
