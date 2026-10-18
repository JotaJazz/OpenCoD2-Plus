/* The dedicated server links the same objects as the client, so the renderer,
 * the Mac GL layer, SDL2 input, the crash popup and the client's HTTP download
 * still reference GL, SDL2, X11 and curl. It never calls them; these stand in for the libraries so cod2_lnxded
 * runs on a machine without them, as win32_stubs.c does for the MinGW server.
 * Each returns 0, which reads as NULL or failure where a value is expected. */

/* curl */
int curl_easy_cleanup(void) { return 0; }
int curl_easy_getinfo(void) { return 0; }
int curl_easy_init(void) { return 0; }
int curl_easy_setopt(void) { return 0; }
int curl_easy_strerror(void) { return 0; }
int curl_multi_add_handle(void) { return 0; }
int curl_multi_info_read(void) { return 0; }
int curl_multi_init(void) { return 0; }
int curl_multi_perform(void) { return 0; }
int curl_multi_remove_handle(void) { return 0; }

/* OpenGL */
int glActiveTextureARB(void) { return 0; }
int glAlphaFunc(void) { return 0; }
int glBindProgramARB(void) { return 0; }
int glBindTexture(void) { return 0; }
int glBindVertexArray(void) { return 0; }
int glBlendFunc(void) { return 0; }
int glBlendFuncSeparateEXT(void) { return 0; }
int glClear(void) { return 0; }
int glClearColor(void) { return 0; }
int glClearDepth(void) { return 0; }
int glClearStencil(void) { return 0; }
int glClientActiveTextureARB(void) { return 0; }
int glColor4f(void) { return 0; }
int glColorMask(void) { return 0; }
int glColorPointer(void) { return 0; }
int glCompressedTexImage2DARB(void) { return 0; }
int glCompressedTexImage3DARB(void) { return 0; }
int glCompressedTexSubImage2D(void) { return 0; }
int glCompressedTexSubImage3D(void) { return 0; }
int glDeleteFencesAPPLE(void) { return 0; }
int glDeleteProgramsARB(void) { return 0; }
int glDeleteTextures(void) { return 0; }
int glDepthFunc(void) { return 0; }
int glDepthMask(void) { return 0; }
int glDepthRange(void) { return 0; }
int glDisable(void) { return 0; }
int glDisableClientState(void) { return 0; }
int glDrawBuffer(void) { return 0; }
int glDrawElements(void) { return 0; }
int glEnable(void) { return 0; }
int glEnableClientState(void) { return 0; }
int glFogf(void) { return 0; }
int glFogfv(void) { return 0; }
int glFogi(void) { return 0; }
int glGenFencesAPPLE(void) { return 0; }
int glGenProgramsARB(void) { return 0; }
int glGenTextures(void) { return 0; }
int glGenVertexArraysAPPLE(void) { return 0; }
int glGetError(void) { return 0; }
int glGetIntegerv(void) { return 0; }
int glGetString(void) { return 0; }
int glLoadIdentity(void) { return 0; }
int glLoadMatrixf(void) { return 0; }
int glMatrixMode(void) { return 0; }
int glNormalPointer(void) { return 0; }
int glOrtho(void) { return 0; }
int glPixelStorei(void) { return 0; }
int glPolygonOffset(void) { return 0; }
int glPopAttrib(void) { return 0; }
int glPopClientAttrib(void) { return 0; }
int glProgramEnvParameter4fvARB(void) { return 0; }
int glProgramStringARB(void) { return 0; }
int glPushAttrib(void) { return 0; }
int glPushClientAttrib(void) { return 0; }
int glReadBuffer(void) { return 0; }
int glReadPixels(void) { return 0; }
int glScalef(void) { return 0; }
int glSetFenceAPPLE(void) { return 0; }
int glStencilMask(void) { return 0; }
int glTestFenceAPPLE(void) { return 0; }
int glTexCoordPointer(void) { return 0; }
int glTexEnvf(void) { return 0; }
int glTexEnvfv(void) { return 0; }
int glTexEnvi(void) { return 0; }
int glTexImage2D(void) { return 0; }
int glTexImage3D(void) { return 0; }
int glTexParameteri(void) { return 0; }
int glTexSubImage2D(void) { return 0; }
int glTexSubImage3D(void) { return 0; }
int glVertexArrayParameteriAPPLE(void) { return 0; }
int glVertexPointer(void) { return 0; }
int glViewport(void) { return 0; }

/* SDL2 */
int SDL_AddEventWatch(void) { return 0; }
int SDL_CreateWindow(void) { return 0; }
int SDL_GL_CreateContext(void) { return 0; }
int SDL_GL_GetAttribute(void) { return 0; }
int SDL_GL_MakeCurrent(void) { return 0; }
int SDL_GL_SetAttribute(void) { return 0; }
int SDL_GL_SwapWindow(void) { return 0; }
int SDL_GetError(void) { return 0; }
int SDL_GetWindowWMInfo(void) { return 0; }
int SDL_IsTextInputActive(void) { return 0; }
int SDL_PollEvent(void) { return 0; }
int SDL_PumpEvents(void) { return 0; }
int SDL_RaiseWindow(void) { return 0; }
int SDL_SetRelativeMouseMode(void) { return 0; }
int SDL_SetWindowGrab(void) { return 0; }
int SDL_SetWindowInputFocus(void) { return 0; }
int SDL_ShowWindow(void) { return 0; }
int SDL_StartTextInput(void) { return 0; }
int SDL_WarpMouseInWindow(void) { return 0; }

/* X11 */
int XClearArea(void) { return 0; }
int XClearWindow(void) { return 0; }
int XCloseDisplay(void) { return 0; }
int XCreateGC(void) { return 0; }
int XCreateImage(void) { return 0; }
int XCreateSimpleWindow(void) { return 0; }
int XDrawString(void) { return 0; }
int XFlush(void) { return 0; }
int XGetWindowAttributes(void) { return 0; }
int XInternAtom(void) { return 0; }
int XLoadQueryFont(void) { return 0; }
int XLookupKeysym(void) { return 0; }
int XLookupString(void) { return 0; }
int XMapWindow(void) { return 0; }
int XNextEvent(void) { return 0; }
int XOpenDisplay(void) { return 0; }
int XPending(void) { return 0; }
int XPutImage(void) { return 0; }
int XSelectInput(void) { return 0; }
int XSetFont(void) { return 0; }
int XSetWMProtocols(void) { return 0; }
int XStoreName(void) { return 0; }
