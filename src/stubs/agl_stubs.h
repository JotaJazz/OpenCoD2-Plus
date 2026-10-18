#ifndef STUBS_AGL_H
#define STUBS_AGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <GLES3/gl3.h>

typedef void *AGLContext;
typedef void *AGLPixelFormat;
typedef void *AGLDrawable;
typedef int AGLError;
typedef unsigned int GLenum;
typedef int GLint;

AGLPixelFormat aglChoosePixelFormat(void *gdevs, int ndev, const int *attribs);
AGLContext aglCreateContext(AGLPixelFormat pix, AGLContext share);
int aglDestroyContext(AGLContext ctx);
int aglSetCurrentContext(AGLContext ctx);
int aglSetDrawable(AGLContext ctx, AGLDrawable draw);
int aglSetFullScreen(AGLContext ctx, int width, int height, int freq, int device);
void aglDestroyPixelFormat(AGLPixelFormat pix);
int aglUpdateContext(AGLContext ctx);
void aglSwapBuffers(AGLContext ctx);
AGLError aglGetError(void);
const char *aglErrorString(AGLError code);
int aglSetInteger(AGLContext ctx, GLenum pname, const GLint *params);
int aglGetInteger(AGLContext ctx, GLenum pname, GLint *params);

#ifdef __cplusplus
}
#endif
#endif
