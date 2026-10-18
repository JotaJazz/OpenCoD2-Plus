#ifndef MAC_DIRECTX_9_MACOPENGLUTILS_H_H
#define MAC_DIRECTX_9_MACOPENGLUTILS_H_H

#ifdef __cplusplus
extern "C" {
#endif

#include "PC/gfx_d3d/rb_backend.h"
#include "PC/universal/com_math.h"
#include "PC/universal/com_vector.h"

struct CDirect3DSurface;
struct CardMemory;
struct D3DDISPLAYMODE;
struct GfxCmdApplyLatePostEffects;
struct GfxCmdBeginView;
struct GfxCmdBlendSavedScreen;
struct GfxCmdDrawFullScreenColoredQuad;
struct GfxEntity;
struct GfxImage;
struct GfxImageFilter;
struct GfxImageFilterPass;
struct GfxImageLoadDef;
struct GfxLodParms;
struct GfxSceneDef;
union GfxTexture;
struct GfxViewParms;
struct HDC__;
struct HMONITOR__;
struct HWND__;
struct IDirect3D9;
struct IDirect3DBaseTexture9;
struct IDirect3DCubeTexture9;
struct IDirect3DDevice9;
struct IDirect3DIndexBuffer9;
struct IDirect3DPixelShader9;
struct IDirect3DQuery9;
struct IDirect3DResource9;
struct IDirect3DStateBlock9;
struct IDirect3DSurface9;
struct IDirect3DSwapChain9;
struct IDirect3DTexture9;
union IDirect3DVertexBuffer9;
struct IDirect3DVertexDeclaration9;
struct IDirect3DVertexShader9;
struct IDirect3DVolume9;
struct IDirect3DVolumeTexture9;
struct IID;
struct IUnknown;
struct LARGE_INTEGER;
struct Material;
struct MaterialArgumentCodeConst;
union MaterialArgumentDef;
struct MaterialConstantDefObj;
struct MaterialInfo;
struct MaterialInfoObj_s;
union MaterialPassArray;
struct MaterialPassDx7;
struct MaterialPassDx9;
struct MaterialShader;
struct MaterialShaderArgument;
union MaterialShaderPtr;
struct MaterialStateMap;
struct MaterialStateMapRule;
struct MaterialStateMapRuleSet;
struct MaterialStreamRouting;
struct MaterialTechnique;
struct MaterialTechniqueSet;
struct MaterialTextureDef;
union MaterialTextureDefInfo;
struct MaterialVertexDeclaration;
struct MaterialWaterDef;
struct Picmip;
struct WaterWritable;
struct _D3DADAPTER_IDENTIFIER9;
struct _D3DBOX;
struct _D3DCAPS9;
struct _D3DCLIPSTATUS9;
struct _D3DCOLORVALUE;
struct _D3DDEVICE_CREATION_PARAMETERS;
struct _D3DDISPLAYMODE;
struct _D3DINDEXBUFFER_DESC;
struct _D3DLIGHT9;
struct _D3DLOCKED_BOX;
struct _D3DLOCKED_RECT;
struct _D3DMATERIAL9;
struct _D3DMATRIX;
struct _D3DPRESENT_PARAMETERS_;
struct _D3DPSHADERCAPS2_0;
struct _D3DRASTER_STATUS;
struct _D3DRECT;
struct _D3DRECTPATCH_INFO;
struct _D3DSURFACE_DESC;
struct _D3DTRIPATCH_INFO;
struct _D3DVECTOR;
struct _D3DVERTEXBUFFER_DESC;
struct _D3DVERTEXELEMENT9;
struct _D3DVIEWPORT9;
struct _D3DVOLUME_DESC;
struct _D3DVSHADERCAPS2_0;
struct _GUID;
union _LARGE_INTEGER;
struct _RGNDATA;
struct _RGNDATAHEADER;
struct complex_s;
struct tagPALETTEENTRY;
struct tagPOINT;
struct tagRECT;
struct water_t;

enum D3DBACKBUFFER_TYPE {
    D3DBACKBUFFER_TYPE_MONO = 0,
    D3DBACKBUFFER_TYPE_LEFT = 1,
    D3DBACKBUFFER_TYPE_RIGHT = 2,
    D3DBACKBUFFER_TYPE_FORCE_DWORD = 2147483647
};

enum D3DBASISTYPE {
    D3DBASIS_BEZIER = 0,
    D3DBASIS_BSPLINE = 1,
    D3DBASIS_CATMULL_ROM = 2,
    D3DBASIS_FORCE_DWORD = 2147483647
};

enum D3DDEGREETYPE {
    D3DDEGREE_LINEAR = 1,
    D3DDEGREE_QUADRATIC = 2,
    D3DDEGREE_CUBIC = 3,
    D3DDEGREE_QUINTIC = 5,
    D3DDEGREE_FORCE_DWORD = 2147483647
};

enum D3DDEVTYPE {
    D3DDEVTYPE_HAL = 1,
    D3DDEVTYPE_REF = 2,
    D3DDEVTYPE_SW = 3,
    D3DDEVTYPE_NULLREF = 4,
    D3DDEVTYPE_FORCE_DWORD = 2147483647
};

enum D3DLIGHTTYPE {
    D3DLIGHT_POINT = 1,
    D3DLIGHT_SPOT = 2,
    D3DLIGHT_DIRECTIONAL = 3,
    D3DLIGHT_FORCE_DWORD = 2147483647
};

enum D3DMULTISAMPLE_TYPE {
    D3DMULTISAMPLE_NONE = 0,
    D3DMULTISAMPLE_NONMASKABLE = 1,
    D3DMULTISAMPLE_2_SAMPLES = 2,
    D3DMULTISAMPLE_3_SAMPLES = 3,
    D3DMULTISAMPLE_4_SAMPLES = 4,
    D3DMULTISAMPLE_5_SAMPLES = 5,
    D3DMULTISAMPLE_6_SAMPLES = 6,
    D3DMULTISAMPLE_7_SAMPLES = 7,
    D3DMULTISAMPLE_8_SAMPLES = 8,
    D3DMULTISAMPLE_9_SAMPLES = 9,
    D3DMULTISAMPLE_10_SAMPLES = 10,
    D3DMULTISAMPLE_11_SAMPLES = 11,
    D3DMULTISAMPLE_12_SAMPLES = 12,
    D3DMULTISAMPLE_13_SAMPLES = 13,
    D3DMULTISAMPLE_14_SAMPLES = 14,
    D3DMULTISAMPLE_15_SAMPLES = 15,
    D3DMULTISAMPLE_16_SAMPLES = 16,
    D3DMULTISAMPLE_FORCE_DWORD = 2147483647
};

enum D3DPOOL {
    D3DPOOL_DEFAULT = 0,
    D3DPOOL_MANAGED = 1,
    D3DPOOL_SYSTEMMEM = 2,
    D3DPOOL_SCRATCH = 3,
    D3DPOOL_FORCE_DWORD = 2147483647
};

enum D3DPRIMITIVETYPE {
    D3DPT_POINTLIST = 1,
    D3DPT_LINELIST = 2,
    D3DPT_LINESTRIP = 3,
    D3DPT_TRIANGLELIST = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN = 6,
    D3DPT_FORCE_DWORD = 2147483647
};

enum D3DQUERYTYPE {
    D3DQUERYTYPE_VCACHE = 4,
    D3DQUERYTYPE_RESOURCEMANAGER = 5,
    D3DQUERYTYPE_VERTEXSTATS = 6,
    D3DQUERYTYPE_EVENT = 8,
    D3DQUERYTYPE_OCCLUSION = 9,
    D3DQUERYTYPE_TIMESTAMP = 10,
    D3DQUERYTYPE_TIMESTAMPDISJOINT = 11,
    D3DQUERYTYPE_TIMESTAMPFREQ = 12,
    D3DQUERYTYPE_PIPELINETIMINGS = 13,
    D3DQUERYTYPE_INTERFACETIMINGS = 14,
    D3DQUERYTYPE_VERTEXTIMINGS = 15,
    D3DQUERYTYPE_PIXELTIMINGS = 16,
    D3DQUERYTYPE_BANDWIDTHTIMINGS = 17,
    D3DQUERYTYPE_CACHEUTILIZATION = 18
};

enum D3DRENDERSTATETYPE {
    D3DRS_ZENABLE = 7,
    D3DRS_FILLMODE = 8,
    D3DRS_SHADEMODE = 9,
    D3DRS_ZWRITEENABLE = 14,
    D3DRS_ALPHATESTENABLE = 15,
    D3DRS_LASTPIXEL = 16,
    D3DRS_SRCBLEND = 19,
    D3DRS_DESTBLEND = 20,
    D3DRS_CULLMODE = 22,
    D3DRS_ZFUNC = 23,
    D3DRS_ALPHAREF = 24,
    D3DRS_ALPHAFUNC = 25,
    D3DRS_DITHERENABLE = 26,
    D3DRS_ALPHABLENDENABLE = 27,
    D3DRS_FOGENABLE = 28,
    D3DRS_SPECULARENABLE = 29,
    D3DRS_FOGCOLOR = 34,
    D3DRS_FOGTABLEMODE = 35,
    D3DRS_FOGSTART = 36,
    D3DRS_FOGEND = 37,
    D3DRS_FOGDENSITY = 38,
    D3DRS_RANGEFOGENABLE = 48,
    D3DRS_STENCILENABLE = 52,
    D3DRS_STENCILFAIL = 53,
    D3DRS_STENCILZFAIL = 54,
    D3DRS_STENCILPASS = 55,
    D3DRS_STENCILFUNC = 56,
    D3DRS_STENCILREF = 57,
    D3DRS_STENCILMASK = 58,
    D3DRS_STENCILWRITEMASK = 59,
    D3DRS_TEXTUREFACTOR = 60,
    D3DRS_WRAP0 = 128,
    D3DRS_WRAP1 = 129,
    D3DRS_WRAP2 = 130,
    D3DRS_WRAP3 = 131,
    D3DRS_WRAP4 = 132,
    D3DRS_WRAP5 = 133,
    D3DRS_WRAP6 = 134,
    D3DRS_WRAP7 = 135,
    D3DRS_CLIPPING = 136,
    D3DRS_LIGHTING = 137,
    D3DRS_AMBIENT = 139,
    D3DRS_FOGVERTEXMODE = 140,
    D3DRS_COLORVERTEX = 141,
    D3DRS_LOCALVIEWER = 142,
    D3DRS_NORMALIZENORMALS = 143,
    D3DRS_DIFFUSEMATERIALSOURCE = 145,
    D3DRS_SPECULARMATERIALSOURCE = 146,
    D3DRS_AMBIENTMATERIALSOURCE = 147,
    D3DRS_EMISSIVEMATERIALSOURCE = 148,
    D3DRS_VERTEXBLEND = 151,
    D3DRS_CLIPPLANEENABLE = 152,
    D3DRS_POINTSIZE = 154,
    D3DRS_POINTSIZE_MIN = 155,
    D3DRS_POINTSPRITEENABLE = 156,
    D3DRS_POINTSCALEENABLE = 157,
    D3DRS_POINTSCALE_A = 158,
    D3DRS_POINTSCALE_B = 159,
    D3DRS_POINTSCALE_C = 160,
    D3DRS_MULTISAMPLEANTIALIAS = 161,
    D3DRS_MULTISAMPLEMASK = 162,
    D3DRS_PATCHEDGESTYLE = 163,
    D3DRS_DEBUGMONITORTOKEN = 165,
    D3DRS_POINTSIZE_MAX = 166,
    D3DRS_INDEXEDVERTEXBLENDENABLE = 167,
    D3DRS_COLORWRITEENABLE = 168,
    D3DRS_TWEENFACTOR = 170,
    D3DRS_BLENDOP = 171,
    D3DRS_POSITIONDEGREE = 172,
    D3DRS_NORMALDEGREE = 173,
    D3DRS_SCISSORTESTENABLE = 174,
    D3DRS_SLOPESCALEDEPTHBIAS = 175,
    D3DRS_ANTIALIASEDLINEENABLE = 176,
    D3DRS_MINTESSELLATIONLEVEL = 178,
    D3DRS_MAXTESSELLATIONLEVEL = 179,
    D3DRS_ADAPTIVETESS_X = 180,
    D3DRS_ADAPTIVETESS_Y = 181,
    D3DRS_ADAPTIVETESS_Z = 182,
    D3DRS_ADAPTIVETESS_W = 183,
    D3DRS_ENABLEADAPTIVETESSELLATION = 184,
    D3DRS_TWOSIDEDSTENCILMODE = 185,
    D3DRS_CCW_STENCILFAIL = 186,
    D3DRS_CCW_STENCILZFAIL = 187,
    D3DRS_CCW_STENCILPASS = 188,
    D3DRS_CCW_STENCILFUNC = 189,
    D3DRS_COLORWRITEENABLE1 = 190,
    D3DRS_COLORWRITEENABLE2 = 191,
    D3DRS_COLORWRITEENABLE3 = 192,
    D3DRS_BLENDFACTOR = 193,
    D3DRS_SRGBWRITEENABLE = 194,
    D3DRS_DEPTHBIAS = 195,
    D3DRS_WRAP8 = 198,
    D3DRS_WRAP9 = 199,
    D3DRS_WRAP10 = 200,
    D3DRS_WRAP11 = 201,
    D3DRS_WRAP12 = 202,
    D3DRS_WRAP13 = 203,
    D3DRS_WRAP14 = 204,
    D3DRS_WRAP15 = 205,
    D3DRS_SEPARATEALPHABLENDENABLE = 206,
    D3DRS_SRCBLENDALPHA = 207,
    D3DRS_DESTBLENDALPHA = 208,
    D3DRS_BLENDOPALPHA = 209,
    D3DRS_FORCE_DWORD = 2147483647
};

enum D3DSAMPLERSTATETYPE {
    D3DSAMP_ADDRESSU = 1,
    D3DSAMP_ADDRESSV = 2,
    D3DSAMP_ADDRESSW = 3,
    D3DSAMP_BORDERCOLOR = 4,
    D3DSAMP_MAGFILTER = 5,
    D3DSAMP_MINFILTER = 6,
    D3DSAMP_MIPFILTER = 7,
    D3DSAMP_MIPMAPLODBIAS = 8,
    D3DSAMP_MAXMIPLEVEL = 9,
    D3DSAMP_MAXANISOTROPY = 10,
    D3DSAMP_SRGBTEXTURE = 11,
    D3DSAMP_ELEMENTINDEX = 12,
    D3DSAMP_DMAPOFFSET = 13,
    D3DSAMP_FORCE_DWORD = 2147483647
};

enum D3DSTATEBLOCKTYPE {
    D3DSBT_ALL = 1,
    D3DSBT_PIXELSTATE = 2,
    D3DSBT_VERTEXSTATE = 3,
    D3DSBT_FORCE_DWORD = 2147483647
};

enum D3DSWAPEFFECT {
    D3DSWAPEFFECT_DISCARD = 1,
    D3DSWAPEFFECT_FLIP = 2,
    D3DSWAPEFFECT_COPY = 3,
    D3DSWAPEFFECT_FORCE_DWORD = 2147483647
};

enum D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP = 1,
    D3DTSS_COLORARG1 = 2,
    D3DTSS_COLORARG2 = 3,
    D3DTSS_ALPHAOP = 4,
    D3DTSS_ALPHAARG1 = 5,
    D3DTSS_ALPHAARG2 = 6,
    D3DTSS_BUMPENVMAT00 = 7,
    D3DTSS_BUMPENVMAT01 = 8,
    D3DTSS_BUMPENVMAT10 = 9,
    D3DTSS_BUMPENVMAT11 = 10,
    D3DTSS_TEXCOORDINDEX = 11,
    D3DTSS_BUMPENVLSCALE = 22,
    D3DTSS_BUMPENVLOFFSET = 23,
    D3DTSS_TEXTURETRANSFORMFLAGS = 24,
    D3DTSS_COLORARG0 = 26,
    D3DTSS_ALPHAARG0 = 27,
    D3DTSS_RESULTARG = 28,
    D3DTSS_CONSTANT = 32,
    D3DTSS_FORCE_DWORD = 2147483647
};

enum D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW = 2,
    D3DTS_PROJECTION = 3,
    D3DTS_TEXTURE0 = 16,
    D3DTS_TEXTURE1 = 17,
    D3DTS_TEXTURE2 = 18,
    D3DTS_TEXTURE3 = 19,
    D3DTS_TEXTURE4 = 20,
    D3DTS_TEXTURE5 = 21,
    D3DTS_TEXTURE6 = 22,
    D3DTS_TEXTURE7 = 23,
    D3DTS_FORCE_DWORD = 2147483647
};

enum GfxRenderTargetId {
    R_RENDERTARGET_FRAME_BUFFER = 0,
    R_RENDERTARGET_RESOLVED_POST_SUN = 1,
    R_RENDERTARGET_RESOLVED_SCENE = 2,
    R_RENDERTARGET_DYNAMICSHADOWS = 3,
    R_RENDERTARGET_SHADOWCOOKIE = 4,
    R_RENDERTARGET_SHADOWCOOKIE_BLUR = 5,
    R_RENDERTARGET_FLOAT_Z = 6,
    R_RENDERTARGET_SAVED_SCREEN = 7,
    R_RENDERTARGET_BLURRED_SCREEN = 8,
    R_RENDERTARGET_GLOW_0 = 9,
    R_RENDERTARGET_GLOW_1 = 10,
    R_RENDERTARGET_PINGPONG_0 = 11,
    R_RENDERTARGET_PINGPONG_1 = 12,
    R_RENDERTARGET_COUNT = 13,
    R_RENDERTARGET_NONE = 14
};

enum SurfaceType {
    TEXTURE_SURFACE = 0,
    CUBE_SURFACE = 1,
    VOLUME_SURFACE = 2
};

enum _D3DBACKBUFFER_TYPE {
    D3DBACKBUFFER_TYPE_MONO = 0,
    D3DBACKBUFFER_TYPE_LEFT = 1,
    D3DBACKBUFFER_TYPE_RIGHT = 2,
    D3DBACKBUFFER_TYPE_FORCE_DWORD = 2147483647
};

enum _D3DBASISTYPE {
    D3DBASIS_BEZIER = 0,
    D3DBASIS_BSPLINE = 1,
    D3DBASIS_CATMULL_ROM = 2,
    D3DBASIS_FORCE_DWORD = 2147483647
};

enum _D3DCUBEMAP_FACES {
    D3DCUBEMAP_FACE_POSITIVE_X = 0,
    D3DCUBEMAP_FACE_NEGATIVE_X = 1,
    D3DCUBEMAP_FACE_POSITIVE_Y = 2,
    D3DCUBEMAP_FACE_NEGATIVE_Y = 3,
    D3DCUBEMAP_FACE_POSITIVE_Z = 4,
    D3DCUBEMAP_FACE_NEGATIVE_Z = 5,
    D3DCUBEMAP_FACE_FORCE_DWORD = 2147483647
};

enum _D3DDEGREETYPE {
    D3DDEGREE_LINEAR = 1,
    D3DDEGREE_QUADRATIC = 2,
    D3DDEGREE_CUBIC = 3,
    D3DDEGREE_QUINTIC = 5,
    D3DDEGREE_FORCE_DWORD = 2147483647
};

enum _D3DDEVTYPE {
    D3DDEVTYPE_HAL = 1,
    D3DDEVTYPE_REF = 2,
    D3DDEVTYPE_SW = 3,
    D3DDEVTYPE_NULLREF = 4,
    D3DDEVTYPE_FORCE_DWORD = 2147483647
};

enum _D3DFORMAT {
    D3DFMT_UNKNOWN = 0,
    D3DFMT_R8G8B8 = 20,
    D3DFMT_A8R8G8B8 = 21,
    D3DFMT_X8R8G8B8 = 22,
    D3DFMT_R5G6B5 = 23,
    D3DFMT_X1R5G5B5 = 24,
    D3DFMT_A1R5G5B5 = 25,
    D3DFMT_A4R4G4B4 = 26,
    D3DFMT_R3G3B2 = 27,
    D3DFMT_A8 = 28,
    D3DFMT_A8R3G3B2 = 29,
    D3DFMT_X4R4G4B4 = 30,
    D3DFMT_A2B10G10R10 = 31,
    D3DFMT_A8B8G8R8 = 32,
    D3DFMT_X8B8G8R8 = 33,
    D3DFMT_G16R16 = 34,
    D3DFMT_A2R10G10B10 = 35,
    D3DFMT_A16B16G16R16 = 36,
    D3DFMT_A8P8 = 40,
    D3DFMT_P8 = 41,
    D3DFMT_L8 = 50,
    D3DFMT_A8L8 = 51,
    D3DFMT_A4L4 = 52,
    D3DFMT_V8U8 = 60,
    D3DFMT_L6V5U5 = 61,
    D3DFMT_X8L8V8U8 = 62,
    D3DFMT_Q8W8V8U8 = 63,
    D3DFMT_V16U16 = 64,
    D3DFMT_A2W10V10U10 = 67,
    D3DFMT_UYVY = 1498831189,
    D3DFMT_R8G8_B8G8 = 1195525970,
    D3DFMT_YUY2 = 844715353,
    D3DFMT_G8R8_G8B8 = 1111970375,
    D3DFMT_DXT1 = 827611204,
    D3DFMT_DXT2 = 844388420,
    D3DFMT_DXT3 = 861165636,
    D3DFMT_DXT4 = 877942852,
    D3DFMT_DXT5 = 894720068,
    D3DFMT_D16_LOCKABLE = 70,
    D3DFMT_D32 = 71,
    D3DFMT_D15S1 = 73,
    D3DFMT_D24S8 = 75,
    D3DFMT_D24X8 = 77,
    D3DFMT_D24X4S4 = 79,
    D3DFMT_D16 = 80,
    D3DFMT_D32F_LOCKABLE = 82,
    D3DFMT_D24FS8 = 83,
    D3DFMT_L16 = 81,
    D3DFMT_VERTEXDATA = 100,
    D3DFMT_INDEX16 = 101,
    D3DFMT_INDEX32 = 102,
    D3DFMT_Q16W16V16U16 = 110,
    D3DFMT_MULTI2_ARGB8 = 827606349,
    D3DFMT_R16F = 111,
    D3DFMT_G16R16F = 112,
    D3DFMT_A16B16G16R16F = 113,
    D3DFMT_R32F = 114,
    D3DFMT_G32R32F = 115,
    D3DFMT_A32B32G32R32F = 116,
    D3DFMT_CxV8U8 = 117,
    D3DFMT_FORCE_DWORD = 2147483647
};

enum _D3DLIGHTTYPE {
    D3DLIGHT_POINT = 1,
    D3DLIGHT_SPOT = 2,
    D3DLIGHT_DIRECTIONAL = 3,
    D3DLIGHT_FORCE_DWORD = 2147483647
};

enum _D3DMULTISAMPLE_TYPE {
    D3DMULTISAMPLE_NONE = 0,
    D3DMULTISAMPLE_NONMASKABLE = 1,
    D3DMULTISAMPLE_2_SAMPLES = 2,
    D3DMULTISAMPLE_3_SAMPLES = 3,
    D3DMULTISAMPLE_4_SAMPLES = 4,
    D3DMULTISAMPLE_5_SAMPLES = 5,
    D3DMULTISAMPLE_6_SAMPLES = 6,
    D3DMULTISAMPLE_7_SAMPLES = 7,
    D3DMULTISAMPLE_8_SAMPLES = 8,
    D3DMULTISAMPLE_9_SAMPLES = 9,
    D3DMULTISAMPLE_10_SAMPLES = 10,
    D3DMULTISAMPLE_11_SAMPLES = 11,
    D3DMULTISAMPLE_12_SAMPLES = 12,
    D3DMULTISAMPLE_13_SAMPLES = 13,
    D3DMULTISAMPLE_14_SAMPLES = 14,
    D3DMULTISAMPLE_15_SAMPLES = 15,
    D3DMULTISAMPLE_16_SAMPLES = 16,
    D3DMULTISAMPLE_FORCE_DWORD = 2147483647
};

enum _D3DPOOL {
    D3DPOOL_DEFAULT = 0,
    D3DPOOL_MANAGED = 1,
    D3DPOOL_SYSTEMMEM = 2,
    D3DPOOL_SCRATCH = 3,
    D3DPOOL_FORCE_DWORD = 2147483647
};

enum _D3DPRIMITIVETYPE {
    D3DPT_POINTLIST = 1,
    D3DPT_LINELIST = 2,
    D3DPT_LINESTRIP = 3,
    D3DPT_TRIANGLELIST = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN = 6,
    D3DPT_FORCE_DWORD = 2147483647
};

enum _D3DQUERYTYPE {
    D3DQUERYTYPE_VCACHE = 4,
    D3DQUERYTYPE_RESOURCEMANAGER = 5,
    D3DQUERYTYPE_VERTEXSTATS = 6,
    D3DQUERYTYPE_EVENT = 8,
    D3DQUERYTYPE_OCCLUSION = 9,
    D3DQUERYTYPE_TIMESTAMP = 10,
    D3DQUERYTYPE_TIMESTAMPDISJOINT = 11,
    D3DQUERYTYPE_TIMESTAMPFREQ = 12,
    D3DQUERYTYPE_PIPELINETIMINGS = 13,
    D3DQUERYTYPE_INTERFACETIMINGS = 14,
    D3DQUERYTYPE_VERTEXTIMINGS = 15,
    D3DQUERYTYPE_PIXELTIMINGS = 16,
    D3DQUERYTYPE_BANDWIDTHTIMINGS = 17,
    D3DQUERYTYPE_CACHEUTILIZATION = 18
};

enum _D3DRENDERSTATETYPE {
    D3DRS_ZENABLE = 7,
    D3DRS_FILLMODE = 8,
    D3DRS_SHADEMODE = 9,
    D3DRS_ZWRITEENABLE = 14,
    D3DRS_ALPHATESTENABLE = 15,
    D3DRS_LASTPIXEL = 16,
    D3DRS_SRCBLEND = 19,
    D3DRS_DESTBLEND = 20,
    D3DRS_CULLMODE = 22,
    D3DRS_ZFUNC = 23,
    D3DRS_ALPHAREF = 24,
    D3DRS_ALPHAFUNC = 25,
    D3DRS_DITHERENABLE = 26,
    D3DRS_ALPHABLENDENABLE = 27,
    D3DRS_FOGENABLE = 28,
    D3DRS_SPECULARENABLE = 29,
    D3DRS_FOGCOLOR = 34,
    D3DRS_FOGTABLEMODE = 35,
    D3DRS_FOGSTART = 36,
    D3DRS_FOGEND = 37,
    D3DRS_FOGDENSITY = 38,
    D3DRS_RANGEFOGENABLE = 48,
    D3DRS_STENCILENABLE = 52,
    D3DRS_STENCILFAIL = 53,
    D3DRS_STENCILZFAIL = 54,
    D3DRS_STENCILPASS = 55,
    D3DRS_STENCILFUNC = 56,
    D3DRS_STENCILREF = 57,
    D3DRS_STENCILMASK = 58,
    D3DRS_STENCILWRITEMASK = 59,
    D3DRS_TEXTUREFACTOR = 60,
    D3DRS_WRAP0 = 128,
    D3DRS_WRAP1 = 129,
    D3DRS_WRAP2 = 130,
    D3DRS_WRAP3 = 131,
    D3DRS_WRAP4 = 132,
    D3DRS_WRAP5 = 133,
    D3DRS_WRAP6 = 134,
    D3DRS_WRAP7 = 135,
    D3DRS_CLIPPING = 136,
    D3DRS_LIGHTING = 137,
    D3DRS_AMBIENT = 139,
    D3DRS_FOGVERTEXMODE = 140,
    D3DRS_COLORVERTEX = 141,
    D3DRS_LOCALVIEWER = 142,
    D3DRS_NORMALIZENORMALS = 143,
    D3DRS_DIFFUSEMATERIALSOURCE = 145,
    D3DRS_SPECULARMATERIALSOURCE = 146,
    D3DRS_AMBIENTMATERIALSOURCE = 147,
    D3DRS_EMISSIVEMATERIALSOURCE = 148,
    D3DRS_VERTEXBLEND = 151,
    D3DRS_CLIPPLANEENABLE = 152,
    D3DRS_POINTSIZE = 154,
    D3DRS_POINTSIZE_MIN = 155,
    D3DRS_POINTSPRITEENABLE = 156,
    D3DRS_POINTSCALEENABLE = 157,
    D3DRS_POINTSCALE_A = 158,
    D3DRS_POINTSCALE_B = 159,
    D3DRS_POINTSCALE_C = 160,
    D3DRS_MULTISAMPLEANTIALIAS = 161,
    D3DRS_MULTISAMPLEMASK = 162,
    D3DRS_PATCHEDGESTYLE = 163,
    D3DRS_DEBUGMONITORTOKEN = 165,
    D3DRS_POINTSIZE_MAX = 166,
    D3DRS_INDEXEDVERTEXBLENDENABLE = 167,
    D3DRS_COLORWRITEENABLE = 168,
    D3DRS_TWEENFACTOR = 170,
    D3DRS_BLENDOP = 171,
    D3DRS_POSITIONDEGREE = 172,
    D3DRS_NORMALDEGREE = 173,
    D3DRS_SCISSORTESTENABLE = 174,
    D3DRS_SLOPESCALEDEPTHBIAS = 175,
    D3DRS_ANTIALIASEDLINEENABLE = 176,
    D3DRS_MINTESSELLATIONLEVEL = 178,
    D3DRS_MAXTESSELLATIONLEVEL = 179,
    D3DRS_ADAPTIVETESS_X = 180,
    D3DRS_ADAPTIVETESS_Y = 181,
    D3DRS_ADAPTIVETESS_Z = 182,
    D3DRS_ADAPTIVETESS_W = 183,
    D3DRS_ENABLEADAPTIVETESSELLATION = 184,
    D3DRS_TWOSIDEDSTENCILMODE = 185,
    D3DRS_CCW_STENCILFAIL = 186,
    D3DRS_CCW_STENCILZFAIL = 187,
    D3DRS_CCW_STENCILPASS = 188,
    D3DRS_CCW_STENCILFUNC = 189,
    D3DRS_COLORWRITEENABLE1 = 190,
    D3DRS_COLORWRITEENABLE2 = 191,
    D3DRS_COLORWRITEENABLE3 = 192,
    D3DRS_BLENDFACTOR = 193,
    D3DRS_SRGBWRITEENABLE = 194,
    D3DRS_DEPTHBIAS = 195,
    D3DRS_WRAP8 = 198,
    D3DRS_WRAP9 = 199,
    D3DRS_WRAP10 = 200,
    D3DRS_WRAP11 = 201,
    D3DRS_WRAP12 = 202,
    D3DRS_WRAP13 = 203,
    D3DRS_WRAP14 = 204,
    D3DRS_WRAP15 = 205,
    D3DRS_SEPARATEALPHABLENDENABLE = 206,
    D3DRS_SRCBLENDALPHA = 207,
    D3DRS_DESTBLENDALPHA = 208,
    D3DRS_BLENDOPALPHA = 209,
    D3DRS_FORCE_DWORD = 2147483647
};

enum _D3DRESOURCETYPE {
    D3DRTYPE_SURFACE = 1,
    D3DRTYPE_VOLUME = 2,
    D3DRTYPE_TEXTURE = 3,
    D3DRTYPE_VOLUMETEXTURE = 4,
    D3DRTYPE_CUBETEXTURE = 5,
    D3DRTYPE_VERTEXBUFFER = 6,
    D3DRTYPE_INDEXBUFFER = 7,
    D3DRTYPE_FORCE_DWORD = 2147483647
};

enum _D3DSAMPLERSTATETYPE {
    D3DSAMP_ADDRESSU = 1,
    D3DSAMP_ADDRESSV = 2,
    D3DSAMP_ADDRESSW = 3,
    D3DSAMP_BORDERCOLOR = 4,
    D3DSAMP_MAGFILTER = 5,
    D3DSAMP_MINFILTER = 6,
    D3DSAMP_MIPFILTER = 7,
    D3DSAMP_MIPMAPLODBIAS = 8,
    D3DSAMP_MAXMIPLEVEL = 9,
    D3DSAMP_MAXANISOTROPY = 10,
    D3DSAMP_SRGBTEXTURE = 11,
    D3DSAMP_ELEMENTINDEX = 12,
    D3DSAMP_DMAPOFFSET = 13,
    D3DSAMP_FORCE_DWORD = 2147483647
};

enum _D3DSTATEBLOCKTYPE {
    D3DSBT_ALL = 1,
    D3DSBT_PIXELSTATE = 2,
    D3DSBT_VERTEXSTATE = 3,
    D3DSBT_FORCE_DWORD = 2147483647
};

enum _D3DSWAPEFFECT {
    D3DSWAPEFFECT_DISCARD = 1,
    D3DSWAPEFFECT_FLIP = 2,
    D3DSWAPEFFECT_COPY = 3,
    D3DSWAPEFFECT_FORCE_DWORD = 2147483647
};

enum _D3DTEXTUREFILTERTYPE {
    D3DTEXF_NONE = 0,
    D3DTEXF_POINT = 1,
    D3DTEXF_LINEAR = 2,
    D3DTEXF_ANISOTROPIC = 3,
    D3DTEXF_PYRAMIDALQUAD = 6,
    D3DTEXF_GAUSSIANQUAD = 7,
    D3DTEXF_FORCE_DWORD = 2147483647
};

enum _D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP = 1,
    D3DTSS_COLORARG1 = 2,
    D3DTSS_COLORARG2 = 3,
    D3DTSS_ALPHAOP = 4,
    D3DTSS_ALPHAARG1 = 5,
    D3DTSS_ALPHAARG2 = 6,
    D3DTSS_BUMPENVMAT00 = 7,
    D3DTSS_BUMPENVMAT01 = 8,
    D3DTSS_BUMPENVMAT10 = 9,
    D3DTSS_BUMPENVMAT11 = 10,
    D3DTSS_TEXCOORDINDEX = 11,
    D3DTSS_BUMPENVLSCALE = 22,
    D3DTSS_BUMPENVLOFFSET = 23,
    D3DTSS_TEXTURETRANSFORMFLAGS = 24,
    D3DTSS_COLORARG0 = 26,
    D3DTSS_ALPHAARG0 = 27,
    D3DTSS_RESULTARG = 28,
    D3DTSS_CONSTANT = 32,
    D3DTSS_FORCE_DWORD = 2147483647
};

enum _D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW = 2,
    D3DTS_PROJECTION = 3,
    D3DTS_TEXTURE0 = 16,
    D3DTS_TEXTURE1 = 17,
    D3DTS_TEXTURE2 = 18,
    D3DTS_TEXTURE3 = 19,
    D3DTS_TEXTURE4 = 20,
    D3DTS_TEXTURE5 = 21,
    D3DTS_TEXTURE6 = 22,
    D3DTS_TEXTURE7 = 23,
    D3DTS_FORCE_DWORD = 2147483647
};

enum refEntityType_t {
    RT_DOBJ = 0,
    RT_XMODEL = 1,
    RT_STATICMODEL = 2,
    RT_BRUSHMODEL = 3,
    RT_SPRITE = 4,
    RT_RAIL_CORE = 5,
    RT_PARTICLE_CLOUD = 6,
    RT_ORIENTED_QUAD = 7,
    RT_LINE = 8,
    RT_CYLINDER = 9,
    RT_MAX_REF_ENTITY_TYPE = 10
};

struct CDirect3DSurface {
    int _placeholder;
};

struct CardMemory {
    int platform[2];
};

struct D3DDISPLAYMODE {
    IDirect3DSurface9 *next;
    HWND__ *marker;
    GfxPosTexVertex *original_length;
    GfxPosTexVertex *data_length;
    const void *data;
};

struct GfxCmdApplyLatePostEffects {
    GfxCmdHeader header;
    float blurRadius;
};

struct GfxCmdBeginView {
    GfxCmdHeader header;
    struct GfxSceneDef sceneDef;
    const GfxViewParms *viewParms;
    struct GfxLodParms lodParms;
    int viewCount;
};

struct GfxCmdBlendSavedScreen {
    GfxCmdHeader header;
    int fadeMsec;
    const Material *material;
};

struct GfxCmdDrawFullScreenColoredQuad {
    GfxCmdHeader header;
    const Material *material;
    float s0;
    float t0;
    float s1;
    float t1;
    GfxColor color;
};

struct GfxEntity {
    refEntityType_t reType;
    int renderFxFlags;
    union { struct { FxMemMgr_Emitter * (*colorForDir)[2]; float sunVisibility; } dx7; vec3_t baseCoords; vec3_t origin; } lighting;
    vec3_t axis[3];
    float scale;
    vec3_t origin;
    vec3_t endpos;
    MaterialHandle customMaterial;
    byte materialRGBA[4];
    float materialTime;
    int materialSubimageIndex;
    vec2_t radius;
    float rotation;
    float minScreenRadius;
};

struct GfxImage {
    int mapType;
    union GfxTexture texture;
    struct Picmip picmip;
    TextureSemantic semantic;
    Bool noPicmip;
    byte track;
    struct CardMemory cardMemory;
    short unsigned int width;
    short unsigned int height;
    short unsigned int depth;
    GfxImageCategory category;
    const char *name;
};

struct GfxImageFilter {
    int passCount;
    GfxImageFilterPass passes[16];
    GfxImage *sourceImage;
    GfxRenderTargetId finalTarget;
    GfxRenderTargetId pingPongTargets[2];
};

struct GfxImageFilterPass {
    const Material *material;
    float srcWidth;
    float srcHeight;
    int dstWidth;
    int dstHeight;
    int tapHalfCount;
    vec4_t tapOffsetsAndWeights[8];
};

struct GfxImageLoadDef {
    byte format;
    byte flags;
    short int dimensions[3];
    byte *data;
    int resourceSize;
};

struct GfxLodParms {
    vec3_t origin;
    float scale;
    float bias;
};

struct GfxSceneDef {
    int time;
    float floatTime;
    int entityCount;
    GfxEntity *entities;
};

union GfxTexture {
    IDirect3DBaseTexture9 *basemap;
    IDirect3DTexture9 *map;
    IDirect3DVolumeTexture9 *volmap;
    IDirect3DCubeTexture9 *cubemap;
    GfxImageLoadDef *loadDef;
};

struct GfxViewParms {
    vec3_t origin;
    vec3_t axis[3];
    D3DVIEWPORT9 viewport;
    D3DMATRIX viewMatrix;
    D3DMATRIX projectionMatrix;
    D3DMATRIX viewProjectionMatrix;
    D3DMATRIX inverseViewProjectionMatrix;
    float depthHackNearClip;
};

struct HDC__ {
    int unused;
};

struct HMONITOR__ {
    int unused;
};

struct HWND__ {
    int unused;
};

struct IDirect3D9 {
    int _placeholder;
};

struct IDirect3DBaseTexture9 {
    struct netProfileStream_t send;
    const D3DRECT recieve;
};

struct IDirect3DCubeTexture9 {
    int _placeholder;
};

struct IDirect3DDevice9 {
    int _placeholder;
};

struct IDirect3DIndexBuffer9 {
    int _placeholder;
};

struct IDirect3DPixelShader9 {
    MaterialShaderArgument *adr;
    MaterialPassDx9 netType;
    MaterialPassDx9 clients;
    MaterialPassDx9 maxClients;
    MaterialPassDx9 dirty;
    MaterialPassDx9 allowAnonymous;
    MaterialPassDx9 bPassword;
    MaterialPassDx9 pure;
    int friendlyfire[2];
    int killcam[2];
    int consoleDisabled[2];
    MaterialPassDx9 hardware;
    MaterialPassDx9 mod;
    MaterialPassDx9 voice;
    MaterialPassDx9 requestCount;
    IDirect3D9 *minPing;
    IDirect3D9 *maxPing;
    IDirect3D9 *ping;
    WaterWritable hostName;
    WaterWritable mapName;
    D3DCAPS9 game;
    D3DDISPLAYMODE gameType;
};

struct IDirect3DQuery9 {
    int _placeholder;
};

struct IDirect3DResource9 {
    int _placeholder;
};

struct IDirect3DStateBlock9 {
    int _placeholder;
};

struct IDirect3DSurface9 {
    long unsigned int min;
    long unsigned int max;
};

struct IDirect3DSwapChain9 {
    int _placeholder;
};

struct IDirect3DTexture9 {
    int _placeholder;
};

union IDirect3DVertexBuffer9 {
    struct { float _11; float _12; float _13; float _14; float _21; float _22; float _23; float _24; float _31; float _32; float _33; float _34; float _41; float _42; float _43; float _44; };
    D3DMATRIX m[4];
};

struct IDirect3DVertexDeclaration9 {
    int _placeholder;
};

struct IDirect3DVertexShader9 {
    int _placeholder;
};

struct IDirect3DVolume9 {
    const char *next;
    const TextureInfoType *marker;
    const COpenGLTexture *original_length;
    const COpenGLTexture *data_length;
    unsigned char *data;
};

struct IDirect3DVolumeTexture9 {
    int _placeholder;
};

struct IID {
    long unsigned int Data1;
    short unsigned int Data2;
    short unsigned int Data3;
    unsigned char Data4[8];
};

struct IUnknown {
    int _vptr$IUnknown;
};

struct LARGE_INTEGER {
    void (*Format)();
    netProfilePacket_t Type;
    D3DRESOURCETYPE Usage;
    _D3DSTATEBLOCKTYPE *Pool;
    void (*MultiSampleType)();
    D3DRESOURCETYPE MultiSampleQuality;
    IDirect3DBaseTexture9 *Width;
    IDirect3DBaseTexture9 *Height;
};

struct Material {
    struct MaterialInfo info;
    int stateBits[2];
    short unsigned int textureCount;
    short unsigned int constantCount;
    MaterialTechniqueSet *techniqueSet;
    MaterialTextureDef *textures;
    MaterialConstantDef *constants;
};

struct MaterialArgumentCodeConst {
    short unsigned int index;
    byte firstRow;
    byte rowCount;
};

union MaterialArgumentDef {
    const float16 *literalConst;
    struct MaterialArgumentCodeConst codeConst;
    int codeSampler;
    const char *name;
};

struct MaterialConstantDefObj {
    const char *name;
    vec4_t literal;
};

struct MaterialInfo {
    const char *name;
    const char *refImageName;
    short unsigned int hashIndex;
    short unsigned int sortedIndex;
    byte gameFlags;
    byte sortKey;
    byte textureAtlasRowCount;
    byte textureAtlasColumnCount;
    float maxDeformMove;
    byte deformFlags;
    byte usage;
    short unsigned int toolFlags;
    unsigned int locale;
    short unsigned int autoTexScaleWidth;
    short unsigned int autoTexScaleHeight;
    float tessSize;
    int surfaceFlags;
    int contents;
};

struct MaterialInfoObj_s {
    const char *name;
    const char *refImageName;
    short unsigned int hashIndex;
    short unsigned int sortedIndex;
    byte gameFlags;
    byte sortKey;
    byte textureAtlasRowCount;
    byte textureAtlasColumnCount;
    float maxDeformMove;
    byte deformFlags;
    byte usage;
    short unsigned int toolFlags;
    unsigned int locale;
    short unsigned int autoTexScaleWidth;
    short unsigned int autoTexScaleHeight;
    float tessSize;
    int surfaceFlags;
    int contents;
};

union MaterialPassArray {
    MaterialPassDx7 dx7[1];
    MaterialPassDx9 dx9[1];
};

struct MaterialPassDx7 {
    MaterialStateMap *stateMap;
    Bool gridLighting;
    Bool projectToInfinity;
    Bool ambientLighting;
    Bool objectiveGlow;
    Bool fogToBlack;
    byte genTexCoords[2];
    MaterialShaderArgument samplers[2];
    int colorStageBits[8];
    int alphaStageBits[8];
};

struct MaterialPassDx9 {
    MaterialStateMap *stateMap;
    MaterialVertexDeclaration *vertexDecl;
    MaterialShader *vertexShader;
    MaterialShader *pixelShader;
    short unsigned int vertexArgCount;
    short unsigned int pixelArgCount;
    MaterialShaderArgument *vertexArgs;
    MaterialShaderArgument *pixelArgs;
};

struct MaterialShader {
    const char *name;
    void (*program)();
    short unsigned int programLen;
    byte shaderType;
    byte shaderVersion;
    union MaterialShaderPtr u;
};

struct MaterialShaderArgument {
    short unsigned int type;
    short unsigned int dest;
    union MaterialArgumentDef u;
};

union MaterialShaderPtr {
    GfxVertexShader *vs;
    GfxPixelShader *ps;
};

struct MaterialStateMap {
    const char *name;
    const MaterialStateMapRuleSet * ruleSet[11];
};

struct MaterialStateMapRule {
    int stateBitsMask[2];
    int stateBitsValue[2];
    int stateBitsSet[2];
    int stateBitsClear[2];
};

struct MaterialStateMapRuleSet {
    int ruleCount;
    MaterialStateMapRule rules[1];
};

struct MaterialStreamRouting {
    MaterialStreamSource source;
    MaterialStreamDest dest;
};

struct MaterialTechnique {
    const char *name;
    short unsigned int flags;
    short unsigned int passCount;
    union MaterialPassArray passArray;
};

struct MaterialTechniqueSet {
    const char *name;
    MaterialTechnique * techniques[34];
};

struct MaterialTextureDef {
    const char *name;
    GfxSamplerState samplerState;
    TextureSemantic semantic;
    byte unused_0;
    byte unused_1;
    union MaterialTextureDefInfo u;
};

union MaterialTextureDefInfo {
    GfxImage *image;
    MaterialWaterDef *water;
};

struct MaterialVertexDeclaration {
    MaterialStreamRouting *data;
    int streamCount;
    void (*decl[4])();
};

struct MaterialWaterDef {
    int textureWidth;
    float horizontalWorldLength;
    float verticalWorldLength;
    float amplitude;
    float windSpeed;
    vec2_t windDirection;
    water_t *map;
};

struct Picmip {
    byte platform[2];
};

struct WaterWritable {
    int frameCount;
};

struct _D3DADAPTER_IDENTIFIER9 {
    char Driver[512];
    char Description[512];
    char DeviceName[32];
    struct LARGE_INTEGER DriverVersion;
    DWORD VendorId;
    DWORD DeviceId;
    DWORD SubSysId;
    DWORD Revision;
    GUID DeviceIdentifier;
    DWORD WHQLLevel;
};

struct _D3DBOX {
    UINT Left;
    UINT Top;
    UINT Right;
    UINT Bottom;
    UINT Front;
    UINT Back;
};

struct _D3DCAPS9 {
    D3DDEVTYPE DeviceType;
    UINT AdapterOrdinal;
    DWORD Caps;
    DWORD Caps2;
    DWORD Caps3;
    DWORD PresentationIntervals;
    DWORD CursorCaps;
    DWORD DevCaps;
    DWORD PrimitiveMiscCaps;
    DWORD RasterCaps;
    DWORD ZCmpCaps;
    DWORD SrcBlendCaps;
    DWORD DestBlendCaps;
    DWORD AlphaCmpCaps;
    DWORD ShadeCaps;
    DWORD TextureCaps;
    DWORD TextureFilterCaps;
    DWORD CubeTextureFilterCaps;
    DWORD VolumeTextureFilterCaps;
    DWORD TextureAddressCaps;
    DWORD VolumeTextureAddressCaps;
    DWORD LineCaps;
    DWORD MaxTextureWidth;
    DWORD MaxTextureHeight;
    DWORD MaxVolumeExtent;
    DWORD MaxTextureRepeat;
    DWORD MaxTextureAspectRatio;
    DWORD MaxAnisotropy;
    float MaxVertexW;
    float GuardBandLeft;
    float GuardBandTop;
    float GuardBandRight;
    float GuardBandBottom;
    float ExtentsAdjust;
    DWORD StencilCaps;
    DWORD FVFCaps;
    DWORD TextureOpCaps;
    DWORD MaxTextureBlendStages;
    DWORD MaxSimultaneousTextures;
    DWORD VertexProcessingCaps;
    DWORD MaxActiveLights;
    DWORD MaxUserClipPlanes;
    DWORD MaxVertexBlendMatrices;
    DWORD MaxVertexBlendMatrixIndex;
    float MaxPointSize;
    DWORD MaxPrimitiveCount;
    DWORD MaxVertexIndex;
    DWORD MaxStreams;
    DWORD MaxStreamStride;
    DWORD VertexShaderVersion;
    DWORD MaxVertexShaderConst;
    DWORD PixelShaderVersion;
    float PixelShader1xMaxValue;
    DWORD DevCaps2;
    float MaxNpatchTessellationLevel;
    DWORD Reserved5;
    UINT MasterAdapterOrdinal;
    UINT AdapterOrdinalInGroup;
    UINT NumberOfAdaptersInGroup;
    DWORD DeclTypes;
    DWORD NumSimultaneousRTs;
    DWORD StretchRectFilterCaps;
    D3DVSHADERCAPS2_0 VS20Caps;
    D3DPSHADERCAPS2_0 PS20Caps;
    DWORD VertexTextureFilterCaps;
    DWORD MaxVShaderInstructionsExecuted;
    DWORD MaxPShaderInstructionsExecuted;
    DWORD MaxVertexShader30InstructionSlots;
    DWORD MaxPixelShader30InstructionSlots;
};

struct _D3DCLIPSTATUS9 {
    DWORD ClipUnion;
    DWORD ClipIntersection;
};

struct _D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
};

struct _D3DDEVICE_CREATION_PARAMETERS {
    UINT AdapterOrdinal;
    D3DDEVTYPE DeviceType;
    HWND hFocusWindow;
    DWORD BehaviorFlags;
};

struct _D3DDISPLAYMODE {
    UINT Width;
    UINT Height;
    UINT RefreshRate;
    D3DFORMAT Format;
};

struct _D3DINDEXBUFFER_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    UINT Size;
};

struct _D3DLIGHT9 {
    D3DLIGHTTYPE Type;
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Ambient;
    D3DVECTOR Position;
    D3DVECTOR Direction;
    float Range;
    float Falloff;
    float Attenuation0;
    float Attenuation1;
    float Attenuation2;
    float Theta;
    float Phi;
};

struct _D3DLOCKED_BOX {
    INT RowPitch;
    INT SlicePitch;
    void *pBits;
};

struct _D3DLOCKED_RECT {
    INT Pitch;
    void *pBits;
};

struct _D3DMATERIAL9 {
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Ambient;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Emissive;
    float Power;
};

struct _D3DMATRIX {
    union { struct { float _11; float _12; float _13; float _14; float _21; float _22; float _23; float _24; float _31; float _32; float _33; float _34; float _41; float _42; float _43; float _44; }; float m[4][4]; };
};

struct _D3DPRESENT_PARAMETERS_ {
    UINT BackBufferWidth;
    UINT BackBufferHeight;
    D3DFORMAT BackBufferFormat;
    UINT BackBufferCount;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD MultiSampleQuality;
    D3DSWAPEFFECT SwapEffect;
    HWND hDeviceWindow;
    BOOL Windowed;
    BOOL EnableAutoDepthStencil;
    D3DFORMAT AutoDepthStencilFormat;
    DWORD Flags;
    UINT FullScreen_RefreshRateInHz;
    UINT PresentationInterval;
};

struct _D3DPSHADERCAPS2_0 {
    DWORD Caps;
    INT DynamicFlowControlDepth;
    INT NumTemps;
    INT StaticFlowControlDepth;
    INT NumInstructionSlots;
};

struct _D3DRASTER_STATUS {
    BOOL InVBlank;
    UINT ScanLine;
};

struct _D3DRECT {
    LONG x1;
    LONG y1;
    LONG x2;
    LONG y2;
};

struct _D3DRECTPATCH_INFO {
    UINT StartVertexOffsetWidth;
    UINT StartVertexOffsetHeight;
    UINT Width;
    UINT Height;
    UINT Stride;
    D3DBASISTYPE Basis;
    D3DDEGREETYPE Degree;
};

struct _D3DSURFACE_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD MultiSampleQuality;
    UINT Width;
    UINT Height;
};

struct _D3DTRIPATCH_INFO {
    UINT StartVertexOffset;
    UINT NumVertices;
    D3DBASISTYPE Basis;
    D3DDEGREETYPE Degree;
};

struct _D3DVECTOR {
    float x;
    float y;
    float z;
};

struct _D3DVERTEXBUFFER_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    UINT Size;
    DWORD FVF;
};

struct _D3DVERTEXELEMENT9 {
    WORD Stream;
    WORD Offset;
    BYTE Type;
    BYTE Method;
    BYTE Usage;
    BYTE UsageIndex;
};

struct _D3DVIEWPORT9 {
    DWORD X;
    DWORD Y;
    DWORD Width;
    DWORD Height;
    float MinZ;
    float MaxZ;
};

struct _D3DVOLUME_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    UINT Width;
    UINT Height;
    UINT Depth;
};

struct _D3DVSHADERCAPS2_0 {
    DWORD Caps;
    INT DynamicFlowControlDepth;
    INT NumTemps;
    INT StaticFlowControlDepth;
};

struct _GUID {
    long unsigned int Data1;
    short unsigned int Data2;
    short unsigned int Data3;
    unsigned char Data4[8];
};

union _LARGE_INTEGER {
    struct { DWORD LowPart; LONG HighPart; };
    struct { DWORD LowPart; LONG HighPart; } u;
    LONGLONG QuadPart;
};

struct _RGNDATA {
    RGNDATAHEADER rdh;
    char Buffer[1];
};

struct _RGNDATAHEADER {
    DWORD dwSize;
    DWORD iType;
    DWORD nCount;
    DWORD nRgnSize;
    RECT rcBound;
};

struct complex_s {
    float real;
    float imag;
};

struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
};

struct tagPOINT {
    LONG x;
    LONG y;
};

struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};

struct water_t {
    struct WaterWritable writable;
    complex_t *H0;
    float *wTerm;
    int M;
    int N;
    float Lx;
    float Lz;
    float gravity;
    float windvel;
    vec2_t winddir;
    float amplitude;
    vec4_t codeConstant;
    GfxImage *image;
};

#ifdef __cplusplus
}
#endif
#endif
