#ifndef MAC_DIRECTX_9_CVAOPACKET_H_H
#define MAC_DIRECTX_9_CVAOPACKET_H_H

#ifdef __cplusplus
extern "C" {
#endif

struct CBaseVA;
struct CColorArray;
struct CDirect3DDevice;
struct CDirect3DPixelShader;
struct CDirect3DVertexShader;
struct CNormalArray;
struct COpenGLMatrix;
struct COpenGLVAO;
struct COpenGLVertexProgram;
struct CSecondaryColorArray;
struct CTexCoordArray;
struct CTexStage;
struct CVAOPacket;
struct CVAOPacketFixedFunction;
struct CVAOPacketProgrammable;
struct CVertexArray;
struct D3DTEXTUREOP;
struct DXVector3;
struct DXVector4;
struct LightInfoType;
struct ProgrammableShaderType;
struct VertexProgramState;
struct VertexProgramStreamState;
struct _Alloc_hider;
struct _Rb_tree_node_base;
struct _Rep;
struct _Rep_base;
struct _Vector_impl;
struct allocator_char;
struct binary_function_UINT32_UINT32_bool;
struct less_UINT32;
struct new_allocator_char;
struct pair_constUINT32_CVAOPacket;

enum D3DTEXTUREOP {
    D3DTOP_DISABLE = 1,
    D3DTOP_SELECTARG1 = 2,
    D3DTOP_SELECTARG2 = 3,
    D3DTOP_MODULATE = 4,
    D3DTOP_MODULATE2X = 5,
    D3DTOP_MODULATE4X = 6,
    D3DTOP_ADD = 7,
    D3DTOP_ADDSIGNED = 8,
    D3DTOP_ADDSIGNED2X = 9,
    D3DTOP_SUBTRACT = 10,
    D3DTOP_ADDSMOOTH = 11,
    D3DTOP_BLENDDIFFUSEALPHA = 12,
    D3DTOP_BLENDTEXTUREALPHA = 13,
    D3DTOP_BLENDFACTORALPHA = 14,
    D3DTOP_BLENDTEXTUREALPHAPM = 15,
    D3DTOP_BLENDCURRENTALPHA = 16,
    D3DTOP_PREMODULATE = 17,
    D3DTOP_MODULATEALPHA_ADDCOLOR = 18,
    D3DTOP_MODULATECOLOR_ADDALPHA = 19,
    D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,
    D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,
    D3DTOP_BUMPENVMAP = 22,
    D3DTOP_BUMPENVMAPLUMINANCE = 23,
    D3DTOP_DOTPRODUCT3 = 24,
    D3DTOP_MULTIPLYADD = 25,
    D3DTOP_LERP = 26,
    D3DTOP_FORCE_DWORD = 2147483647
};

enum D3DTEXTURETRANSFORMFLAGS {
    D3DTTFF_DISABLE = 0,
    D3DTTFF_COUNT1 = 1,
    D3DTTFF_COUNT2 = 2,
    D3DTTFF_COUNT3 = 3,
    D3DTTFF_COUNT4 = 4,
    D3DTTFF_PROJECTED = 256,
    D3DTTFF_FORCE_DWORD = 2147483647
};

enum OpenGLPixelShaderType {
    ATITextFragmentProgram = 0,
    NVidiaRegisterCombinersProgram = 1,
    ARBFragmentProgram = 2
};

enum VAOStatus {
    UNINITIALIZED_VAO_STATUS = 0,
    USING_GENERIC_VAO = 1,
    USING_VIRGIN_VAO = 2,
    USING_CACHED_VAO = 3
};

enum _D3DTEXTUREADDRESS {
    D3DTADDRESS_WRAP = 1,
    D3DTADDRESS_MIRROR = 2,
    D3DTADDRESS_CLAMP = 3,
    D3DTADDRESS_BORDER = 4,
    D3DTADDRESS_MIRRORONCE = 5,
    D3DTADDRESS_FORCE_DWORD = 2147483647
};

enum _D3DTEXTUREOP {
    D3DTOP_DISABLE = 1,
    D3DTOP_SELECTARG1 = 2,
    D3DTOP_SELECTARG2 = 3,
    D3DTOP_MODULATE = 4,
    D3DTOP_MODULATE2X = 5,
    D3DTOP_MODULATE4X = 6,
    D3DTOP_ADD = 7,
    D3DTOP_ADDSIGNED = 8,
    D3DTOP_ADDSIGNED2X = 9,
    D3DTOP_SUBTRACT = 10,
    D3DTOP_ADDSMOOTH = 11,
    D3DTOP_BLENDDIFFUSEALPHA = 12,
    D3DTOP_BLENDTEXTUREALPHA = 13,
    D3DTOP_BLENDFACTORALPHA = 14,
    D3DTOP_BLENDTEXTUREALPHAPM = 15,
    D3DTOP_BLENDCURRENTALPHA = 16,
    D3DTOP_PREMODULATE = 17,
    D3DTOP_MODULATEALPHA_ADDCOLOR = 18,
    D3DTOP_MODULATECOLOR_ADDALPHA = 19,
    D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,
    D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,
    D3DTOP_BUMPENVMAP = 22,
    D3DTOP_BUMPENVMAPLUMINANCE = 23,
    D3DTOP_DOTPRODUCT3 = 24,
    D3DTOP_MULTIPLYADD = 25,
    D3DTOP_LERP = 26,
    D3DTOP_FORCE_DWORD = 2147483647
};

enum _D3DTEXTURETRANSFORMFLAGS {
    D3DTTFF_DISABLE = 0,
    D3DTTFF_COUNT1 = 1,
    D3DTTFF_COUNT2 = 2,
    D3DTTFF_COUNT3 = 3,
    D3DTTFF_COUNT4 = 4,
    D3DTTFF_PROJECTED = 256,
    D3DTTFF_FORCE_DWORD = 2147483647
};

enum _Rb_tree_color {
    _S_red = 0,
    _S_black = 1
};

struct CBaseVA {
    int _vptr$CBaseVA;
};

struct CColorArray {
    int _placeholder;
};

struct CDirect3DDevice {
    int _placeholder;
};

struct CDirect3DPixelShader {
    int _placeholder;
};

struct CDirect3DVertexShader {
    int _placeholder;
};

struct CNormalArray {
    int _placeholder;
};

struct COpenGLMatrix {
    int _placeholder;
};

struct COpenGLVAO {
    int _vptr$COpenGLVAO;
    const void *mpVAStart;
    UINT32 mSize;
    struct CColorArray mColorArray;
    struct CSecondaryColorArray mSecondaryColorArray;
    struct CNormalArray mNormalArray;
    struct CVertexArray mVertexArray;
    CTexCoordArray mTexCoordArrays[8];
    VertexProgramStreamState mGenericArrays[16];
};

struct COpenGLVertexProgram {
    int _vptr$COpenGLVertexProgram;
};

struct CSecondaryColorArray {
    int _placeholder;
};

struct CTexCoordArray {
    int _placeholder;
};

struct CTexStage {
    int _placeholder;
};

struct CVAOPacket {
    int _placeholder;
};

struct CVAOPacketFixedFunction {
    int _placeholder;
};

struct CVAOPacketProgrammable {
    int _placeholder;
};

struct CVertexArray {
    int _placeholder;
};

struct D3DTEXTUREOP {
    bool unused;
};

struct DXVector3 {
    float m[3];
};

struct DXVector4 {
    float m[4];
};

struct LightInfoType {
    bool Enabled;
    DWORD Type;
    float Range;
    struct DXVector4 Diffuse;
    DXVector4 Specular;
    DXVector4 Ambient;
    DXVector3 Direction;
    DXVector3 Position;
    DXVector3 DirectionCS;
    float Exponent;
    float Cutoff;
    float Theta;
    float Phi;
    float Falloff;
    float Attenuation0;
    float Attenuation1;
    float Attenuation2;
    DXVector3 CSDirection;
    DXVector3 CSPosition;
    float Range2;
    DXVector3 VdLd;
    float CosHalfTheta;
    float CosHalfPhi;
    float C1;
};

struct ProgrammableShaderType {
    bool mUsed;
    UINT32 mStream;
    UINT32 mStreamOffset;
    GLint mVSize;
    GLenum mVType;
    GLboolean mNormalized;
    GLsizei mStride;
};

struct VertexProgramState {
    GLuint mOpenGLProgramID;
    VertexProgramStreamState mStreams[16];
};

struct VertexProgramStreamState {
    bool mNeedsValidation;
    bool mEnabled;
    GLint mVSize;
    GLenum mVType;
    GLboolean mNormalized;
    GLsizei mStride;
    const void *mpStream;
};

struct _Alloc_hider {
    char *_M_p;
};

struct _Rb_tree_node_base {
    _Rb_tree_color _M_color;
    int _M_parent;
    int _M_left;
    int _M_right;
};

struct _Rep {
    int _placeholder;
};

struct _Rep_base {
    size_t _M_length;
    size_t _M_capacity;
    _Atomic_word _M_refcount;
};

struct _Vector_impl {
    MacRect *_M_start;
    MacRect *_M_finish;
    MacRect *_M_end_of;
};

struct allocator_char {
    int _placeholder;
};

struct binary_function_UINT32_UINT32_bool {
    int _placeholder;
};

struct less_UINT32 {
    int _placeholder;
};

struct new_allocator_char {
    int _placeholder;
};

struct pair_constUINT32_CVAOPacket {
    const UINT32 first;
    CVAOPacket second;
};

#ifdef __cplusplus
}
#endif
#endif
