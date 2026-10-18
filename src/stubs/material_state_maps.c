#include <stddef.h>

typedef struct {
    const char *name;
    int bits;
} MtlStateMapBitName;

typedef struct {
    const char *name;
    const MtlStateMapBitName *bitNames;
    int stateBitsMask[2];
} MtlStateMapBitGroup;

const MtlStateMapBitName s_alphaTestBitNames[] = {
    { "Always", 0x00000800 },
    { "GE128", 0x00003000 },
    { "GT0", 0x00001000 },
    { "LT128", 0x00002000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_blendOpRgbBitNames[] = {
    { "Disable", 0x00000000 },
    { "Add", 0x00000100 },
    { "Subtract", 0x00000200 },
    { "RevSubtract", 0x00000300 },
    { "Min", 0x00000400 },
    { "Max", 0x00000500 },
    { NULL, 0 }
};

const MtlStateMapBitName s_srcBlendRgbBitNames[] = {
    { "Zero", 0x00000001 },
    { "One", 0x00000002 },
    { "SrcColor", 0x00000003 },
    { "InvSrcColor", 0x00000004 },
    { "SrcAlpha", 0x00000005 },
    { "InvSrcAlpha", 0x00000006 },
    { "DestAlpha", 0x00000007 },
    { "InvDestAlpha", 0x00000008 },
    { "DestColor", 0x00000009 },
    { "InvDestColor", 0x0000000a },
    { "BlendFactor", 0x0000000c },
    { "InvBlendFactor", 0x0000000d },
    { NULL, 0 }
};

const MtlStateMapBitName s_dstBlendRgbBitNames[] = {
    { "Zero", 0x00000010 },
    { "One", 0x00000020 },
    { "SrcColor", 0x00000030 },
    { "InvSrcColor", 0x00000040 },
    { "SrcAlpha", 0x00000050 },
    { "InvSrcAlpha", 0x00000060 },
    { "DestAlpha", 0x00000070 },
    { "InvDestAlpha", 0x00000080 },
    { "DestColor", 0x00000090 },
    { "InvDestColor", 0x000000a0 },
    { "BlendFactor", 0x000000c0 },
    { "InvBlendFactor", 0x000000d0 },
    { NULL, 0 }
};

const MtlStateMapBitName s_blendOpAlphaBitNames[] = {
    { "Disable", 0x00000000 },
    { "Add", 0x01000000 },
    { "Subtract", 0x02000000 },
    { "RevSubtract", 0x03000000 },
    { "Min", 0x04000000 },
    { "Max", 0x05000000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_srcBlendAlphaBitNames[] = {
    { "Zero", 0x00010000 },
    { "One", 0x00020000 },
    { "SrcColor", 0x00030000 },
    { "InvSrcColor", 0x00040000 },
    { "SrcAlpha", 0x00050000 },
    { "InvSrcAlpha", 0x00060000 },
    { "DestAlpha", 0x00070000 },
    { "InvDestAlpha", 0x00080000 },
    { "DestColor", 0x00090000 },
    { "InvDestColor", 0x000a0000 },
    { "BlendFactor", 0x000c0000 },
    { "InvBlendFactor", 0x000d0000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_dstBlendAlphaBitNames[] = {
    { "Zero", 0x00100000 },
    { "One", 0x00200000 },
    { "SrcColor", 0x00300000 },
    { "InvSrcColor", 0x00400000 },
    { "SrcAlpha", 0x00500000 },
    { "InvSrcAlpha", 0x00600000 },
    { "DestAlpha", 0x00700000 },
    { "InvDestAlpha", 0x00800000 },
    { "DestColor", 0x00900000 },
    { "InvDestColor", 0x00a00000 },
    { "BlendFactor", 0x00c00000 },
    { "InvBlendFactor", 0x00d00000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_cullFaceBitNames[] = {
    { "None", 0x00004000 },
    { "Back", 0x00008000 },
    { "Front", 0x0000c000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_depthTestBitNames[] = {
    { "Disable", 0x00000002 },
    { "LessEqual", 0x00000004 },
    { "Equal", 0x00000008 },
    { "Always", 0x0000000c },
    { NULL, 0 }
};

const MtlStateMapBitName s_depthWriteBitNames[] = {
    { "Enable", 0x00000001 },
    { "Disable", 0x00000000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_colorWriteRgbBitNames[] = {
    { "Enable", 0x08000000 },
    { "Disable", 0x00000000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_colorWriteAlphaBitNames[] = {
    { "Enable", 0x10000000 },
    { "Disable", 0x00000000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_fogBitNames[] = {
    { "Enable", 0x20000000 },
    { "Disable", 0x00000000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_polygonOffsetBitNames[] = {
    { "0", 0x00000000 },
    { "1", 0x00000010 },
    { "2", 0x00000020 },
    { NULL, 0 }
};

const MtlStateMapBitName s_wireframeBitNames[] = {
    { "Enable", (int)0x80000000u },
    { "Disable", 0x00000000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_stencilBitNames[] = {
    { "Disable", 0x00000000 },
    { "OneSided", 0x00000040 },
    { "TwoSided", 0x000000c0 },
    { NULL, 0 }
};

const MtlStateMapBitName s_stencilFuncFrontBitNames[] = {
    { "Never", 0x00000000 },
    { "Less", 0x00020000 },
    { "Equal", 0x00040000 },
    { "LessEqual", 0x00060000 },
    { "Greater", 0x00080000 },
    { "NotEqual", 0x000a0000 },
    { "GreaterEqual", 0x000c0000 },
    { "Always", 0x000e0000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_stencilOpFrontPassBitNames[] = {
    { "Keep", 0x00000000 },
    { "Zero", 0x00000100 },
    { "Replace", 0x00000200 },
    { "IncrSat", 0x00000300 },
    { "DecrSat", 0x00000400 },
    { "Invert", 0x00000500 },
    { "Incr", 0x00000600 },
    { "Decr", 0x00000700 },
    { NULL, 0 }
};

const MtlStateMapBitName s_stencilOpFrontFailBitNames[] = {
    { "Keep", 0x00000000 },
    { "Zero", 0x00000800 },
    { "Replace", 0x00001000 },
    { "IncrSat", 0x00001800 },
    { "DecrSat", 0x00002000 },
    { "Invert", 0x00002800 },
    { "Incr", 0x00003000 },
    { "Decr", 0x00003800 },
    { NULL, 0 }
};

const MtlStateMapBitName s_stencilOpFrontZFailBitNames[] = {
    { "Keep", 0x00000000 },
    { "Zero", 0x00004000 },
    { "Replace", 0x00008000 },
    { "IncrSat", 0x0000c000 },
    { "DecrSat", 0x00010000 },
    { "Invert", 0x00014000 },
    { "Incr", 0x00018000 },
    { "Decr", 0x0001c000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_stencilFuncBackBitNames[] = {
    { "Never", 0x00000000 },
    { "Less", 0x20000000 },
    { "Equal", 0x40000000 },
    { "LessEqual", 0x60000000 },
    { "Greater", (int)0x80000000u },
    { "NotEqual", (int)0xa0000000u },
    { "GreaterEqual", (int)0xc0000000u },
    { "Always", (int)0xe0000000u },
    { NULL, 0 }
};

const MtlStateMapBitName s_stencilOpBackPassBitNames[] = {
    { "Keep", 0x00000000 },
    { "Zero", 0x00100000 },
    { "Replace", 0x00200000 },
    { "IncrSat", 0x00300000 },
    { "DecrSat", 0x00400000 },
    { "Invert", 0x00500000 },
    { "Incr", 0x00600000 },
    { "Decr", 0x00700000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_stencilOpBackFailBitNames[] = {
    { "Keep", 0x00000000 },
    { "Zero", 0x00800000 },
    { "Replace", 0x01000000 },
    { "IncrSat", 0x01800000 },
    { "DecrSat", 0x02000000 },
    { "Invert", 0x02800000 },
    { "Incr", 0x03000000 },
    { "Decr", 0x03800000 },
    { NULL, 0 }
};

const MtlStateMapBitName s_stencilOpBackZFailBitNames[] = {
    { "Keep", 0x00000000 },
    { "Zero", 0x04000000 },
    { "Replace", 0x08000000 },
    { "IncrSat", 0x0c000000 },
    { "DecrSat", 0x10000000 },
    { "Invert", 0x14000000 },
    { "Incr", 0x18000000 },
    { "Decr", 0x1c000000 },
    { NULL, 0 }
};

extern const MtlStateMapBitGroup s_stateMapSrcBitGroup[] = {
    { "mtlAlphaTest", s_alphaTestBitNames, { 0x00003800, 0x00000000 } },
    { "mtlBlendOp", s_blendOpRgbBitNames, { 0x00000700, 0x00000000 } },
    { "mtlSrcBlend", s_srcBlendRgbBitNames, { 0x0000000f, 0x00000000 } },
    { "mtlDestBlend", s_dstBlendRgbBitNames, { 0x000000f0, 0x00000000 } },
    { "mtlBlendOpAlpha", s_blendOpAlphaBitNames, { 0x07000000, 0x00000000 } },
    { "mtlSrcBlendAlpha", s_srcBlendAlphaBitNames, { 0x000f0000, 0x00000000 } },
    { "mtlDestBlendAlpha", s_dstBlendAlphaBitNames, { 0x00f00000, 0x00000000 } },
    { "mtlCullFace", s_cullFaceBitNames, { 0x0000c000, 0x00000000 } },
    { "mtlColorWriteRgb", s_colorWriteRgbBitNames, { 0x08000000, 0x00000000 } },
    { "mtlColorWriteAlpha", s_colorWriteAlphaBitNames, { 0x10000000, 0x00000000 } },
    { "mtlFog", s_fogBitNames, { 0x20000000, 0x00000000 } },
    { "mtlDepthTest", s_depthTestBitNames, { 0x00000000, 0x0000000e } },
    { "mtlDepthWrite", s_depthWriteBitNames, { 0x00000000, 0x00000001 } },
    { "mtlPolygonOffset", s_polygonOffsetBitNames, { 0x00000000, 0x00000030 } },
    { "mtlStencil", s_stencilBitNames, { 0x00000000, 0x000000c0 } },
    { "mtlStencilFuncFront", s_stencilFuncFrontBitNames, { 0x00000000, 0x000e0000 } },
    { "mtlStencilOpFrontPass", s_stencilOpFrontPassBitNames, { 0x00000000, 0x00000700 } },
    { "mtlStencilOpFrontFail", s_stencilOpFrontFailBitNames, { 0x00000000, 0x00003800 } },
    { "mtlStencilOpFrontZFail", s_stencilOpFrontZFailBitNames, { 0x00000000, 0x0001c000 } },
    { "mtlStencilFuncBack", s_stencilFuncBackBitNames, { 0x00000000, 0x000e0000 } },
    { "mtlStencilOpBackPass", s_stencilOpBackPassBitNames, { 0x00000000, 0x00000700 } },
    { "mtlStencilOpBackFail", s_stencilOpBackFailBitNames, { 0x00000000, 0x00003800 } },
    { "mtlStencilOpBackZFail", s_stencilOpBackZFailBitNames, { 0x00000000, 0x0001c000 } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstAlphaTestBitGroup[] = {
    { "alphaTest", s_alphaTestBitNames, { 0x00003800, 0x00000000 } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstBlendFuncRgbBitGroup[] = {
    { "blendFuncRgb", s_blendOpRgbBitNames, { 0x00000700, 0x00000000 } },
    { "blendFuncRgb", s_srcBlendRgbBitNames, { 0x0000070f, 0x00000000 } },
    { "blendFuncRgb", s_dstBlendRgbBitNames, { 0x000007f0, 0x00000000 } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstBlendFuncAlphaBitGroup[] = {
    { "blendFuncAlpha", s_blendOpAlphaBitNames, { 0x07000000, 0x00000000 } },
    { "blendFuncAlpha", s_srcBlendAlphaBitNames, { 0x000f0000, 0x00000000 } },
    { "blendFuncAlpha", s_dstBlendAlphaBitNames, { 0x00f00000, 0x00000000 } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstCullFaceBitGroup[] = {
    { "cullFace", s_cullFaceBitNames, { 0x0000c000, 0x00000000 } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstDepthTestBitGroup[] = {
    { "depthTest", s_depthTestBitNames, { 0x00000000, 0x0000000e } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstDepthWriteBitGroup[] = {
    { "depthWrite", s_depthWriteBitNames, { 0x00000000, 0x00000001 } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstColorWriteBitGroup[] = {
    { "colorWrite", s_colorWriteRgbBitNames, { 0x08000000, 0x00000000 } },
    { "colorWrite", s_colorWriteAlphaBitNames, { 0x10000000, 0x00000000 } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstFogBitGroup[] = {
    { "fog", s_fogBitNames, { 0x20000000, 0x00000000 } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstPolygonOffsetBitGroup[] = {
    { "polygonOffset", s_polygonOffsetBitNames, { 0x00000000, 0x00000030 } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstStencilBitGroup[] = {
    { "stencil", s_stencilBitNames, { 0x00000000, 0x000000c0 } },
    { "stencil", s_stencilFuncFrontBitNames, { 0x00000000, 0x000e0000 } },
    { "stencil", s_stencilOpFrontPassBitNames, { 0x00000000, 0x00000700 } },
    { "stencil", s_stencilOpFrontFailBitNames, { 0x00000000, 0x00003800 } },
    { "stencil", s_stencilOpFrontZFailBitNames, { 0x00000000, 0x0001c000 } },
    { "stencil", s_stencilFuncBackBitNames, { 0x00000000, 0x000e0000 } },
    { "stencil", s_stencilOpBackPassBitNames, { 0x00000000, 0x00000700 } },
    { "stencil", s_stencilOpBackFailBitNames, { 0x00000000, 0x00003800 } },
    { "stencil", s_stencilOpBackZFailBitNames, { 0x00000000, 0x0001c000 } },
    { NULL, NULL, { 0, 0 } }
};

extern const MtlStateMapBitGroup s_stateMapDstWireframeBitGroup[] = {
    { "wireframe", s_wireframeBitNames, { (int)0x80000000u, 0x00000000 } },
    { NULL, NULL, { 0, 0 } }
};

const MtlStateMapBitGroup s_mtlStateMapBitGroups[] = {
    { "alphaTest", s_alphaTestBitNames, { 0x00003800, 0x00000000 } },
    { "blendFuncRgb", s_srcBlendRgbBitNames, { 0x000007ff, 0x00000000 } },
    { "blendFuncAlpha", s_srcBlendAlphaBitNames, { 0x07ff0000, 0x00000000 } },
    { "cullFace", s_cullFaceBitNames, { 0x0000c000, 0x00000000 } },
    { "depthTest", s_depthTestBitNames, { 0x00000000, 0x0000000e } },
    { "depthWrite", s_depthWriteBitNames, { 0x00000000, 0x00000001 } },
    { "colorWriteRgb", s_colorWriteRgbBitNames, { 0x08000000, 0x00000000 } },
    { "colorWriteAlpha", s_colorWriteAlphaBitNames, { 0x10000000, 0x00000000 } },
    { NULL, NULL, { 0, 0 } }
};
