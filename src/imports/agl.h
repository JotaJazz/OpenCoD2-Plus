#ifndef IMPORTS_AGL_H
#define IMPORTS_AGL_H

#ifdef __cplusplus
extern "C" {
#endif


int aglChoosePixelFormat();
int aglCreateContext();
int aglDescribePixelFormat();
int aglDestroyContext(COD2_KR);
int aglDestroyPixelFormat();
int aglGetDrawable();
int aglSetCurrentContext(COD2_KR);
int aglSetDrawable(COD2_KR);
int aglSetFullScreen();
int aglSetInteger();
int aglSwapBuffers(COD2_KR);


#ifdef __cplusplus
}
#endif
#endif
