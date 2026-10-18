#ifndef IMPORTS_AGL_H
#define IMPORTS_AGL_H

#ifdef __cplusplus
extern "C" {
#endif


int aglChoosePixelFormat();
int aglCreateContext();
int aglDescribePixelFormat();
int aglDestroyContext();
int aglDestroyPixelFormat();
int aglGetDrawable();
int aglSetCurrentContext();
int aglSetDrawable();
int aglSetFullScreen();
int aglSetInteger();
int aglSwapBuffers();


#ifdef __cplusplus
}
#endif
#endif
