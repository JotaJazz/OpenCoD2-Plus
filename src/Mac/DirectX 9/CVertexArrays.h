#ifndef MAC_DIRECTX_9_CVERTEXARRAYS_H_H
#define MAC_DIRECTX_9_CVERTEXARRAYS_H_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mac/DirectX 9/CVAOPacket.h"

struct CTexUnit;
struct OpenGLStateInfoType;

enum OGLSITPTypes {
    BOOLEAN = 0,
    INT = 1,
    FLOAT = 2,
    FOUR_BOOLEANS = 3,
    FOUR_FLOATS = 4,
    THREE_FLOATS = 5,
    MATRIX = 6,
    FOUR_INTS = 7,
    LFOUR_FLOATS = 8,
    LFLOAT = 9,
    MFOUR_FLOATS = 10,
    MFLOAT = 11,
    TEFOUR_FLOATS = 12,
    TEINT = 13,
    TGFOUR_FLOATS = 14,
    TGINT = 15
};

struct CTexUnit {
    bool mIsProgramableOnly;
    bool mTargetEnabled[3];
    const GLuint * mTexID[3];
    GLenum mTexWrapS[3];
    GLenum mTexWrapT[3];
    GLenum mTexWrapR[3];
    GLuint mTexBorderColor[3];
    GLenum mTexMinFilter[3];
    GLenum mTexMagFilter[3];
    GLfloat mTexAnisotropicFilter[3];
    GLint mTexLastLevel[3];
    GLenum mCombinerColorOp;
    GLenum mCombinerColorSource0;
    GLenum mCombinerColorOperand0;
    GLenum mCombinerColorSource1;
    GLenum mCombinerColorOperand1;
    GLenum mCombinerColorSource2;
    GLenum mCombinerColorOperand2;
    GLenum mCombinerAlphaOp;
    GLenum mCombinerAlphaSource0;
    GLenum mCombinerAlphaOperand0;
    GLenum mCombinerAlphaSource1;
    GLenum mCombinerAlphaOperand1;
    GLenum mCombinerAlphaSource2;
    GLenum mCombinerAlphaOperand2;
    GLfloat mCombinerRGBScale;
    GLfloat mCombinerAlphaScale;
    GLfloat mTexFactor[4];
    GLfloat mLodBias;
    GLfloat mTexMatrix[16];
    CTexCoordArray mTexCoordArray;
    bool mTexGenEnable[4];
    GLenum mTexGenMode[4];
};

struct OpenGLStateInfoType {
    GLenum PName;
    OGLSITPTypes PType;
    char *SName;
};

#ifdef __cplusplus
}
#endif
#endif
