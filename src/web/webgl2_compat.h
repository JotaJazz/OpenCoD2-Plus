#ifndef WEB_WEBGL2_COMPAT_H
#define WEB_WEBGL2_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __EMSCRIPTEN__

void glActiveTextureARB(unsigned int texture);
void glAlphaFunc(unsigned int func, float ref);
void glBindProgramARB(unsigned int target, unsigned int program);
void glBindTexture(unsigned int target, unsigned int texture);
void glBindVertexArray(unsigned int array);
void glBlendFunc(unsigned int sfactor, unsigned int dfactor);
void glClear(unsigned int mask);
void glClearColor(float red, float green, float blue, float alpha);
void glClearDepth(double depth);
void glClearStencil(int s);
void glClientActiveTextureARB(unsigned int texture);
void glColor4f(float red, float green, float blue, float alpha);
void glColorMask(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
void glColorPointer(int size, unsigned int type, int stride, const void *pointer);
void glDepthFunc(unsigned int func);
void glDepthMask(unsigned char flag);
void glDepthRange(double zNear, double zFar);
void glDisableClientState(unsigned int array);
void glEnableClientState(unsigned int array);
void glFogfv(unsigned int pname, const float *params);
unsigned int glGetError(void);
void glGetIntegerv(unsigned int pname, int *data);
void glLoadIdentity(void);
void glLoadMatrixf(const float *m);
void glMatrixMode(unsigned int mode);
void glProgramEnvParameter4fvARB(unsigned int target, unsigned int index, const float *params);
void glScalef(float x, float y, float z);
void glTexCoordPointer(int size, unsigned int type, int stride, const void *pointer);
void glTexEnvfv(unsigned int target, unsigned int pname, const float *params);
void glTexEnvi(unsigned int target, unsigned int pname, int param);
void glTexParameteri(unsigned int target, unsigned int pname, int param);
void glVertexPointer(int size, unsigned int type, int stride, const void *pointer);
void glViewport(int x, int y, int width, int height);

void webgl2_glDisable(unsigned int cap);
void webgl2_glDrawArrays(unsigned int mode, int first, int count);
void webgl2_glDrawElements(unsigned int mode, int count, unsigned int type, const void *indices);
void webgl2_glDrawRangeElements(unsigned int mode, unsigned int start, unsigned int end, int count, unsigned int type, const void *indices);
void webgl2_glEnable(unsigned int cap);

#endif

#ifdef __cplusplus
}
#endif
#endif
