#include "common_types.h"

#if defined(__x86_64__) || defined(_M_X64)
#    define COD2_SUBTABLE_PTR(p) ((intptr_t)(p))
#else
#    define COD2_SUBTABLE_PTR(p) ((int)(p))
#endif

extern const CodeConstantSource s_cameraConsts[];
extern const CodeConstantSource s_codeConsts[];
extern const CodeSamplerSource s_codeSamplers[];
extern const CodeConstantSource s_defaultCodeConsts[];
extern const CodeSamplerSource s_defaultCodeSamplers[];
extern const CodeConstantSource s_lightConsts[];
extern const CodeConstantSource s_lightGridConsts[];
extern const CodeSamplerSource s_lightGridSamplers[];
extern const CodeSamplerSource s_lightSamplers[];
extern const CodeSamplerSource s_lightmapSamplers[];
extern const CodeConstantSource s_nearPlaneConsts[];

const CodeConstantSource s_codeConsts[] = {
    { "camera", (MaterialTextureSource)0, COD2_SUBTABLE_PTR(s_cameraConsts), 0, 0 },
    { "nearPlane", (MaterialTextureSource)0, COD2_SUBTABLE_PTR(s_nearPlaneConsts), 0, 0 },
    { "light", (MaterialTextureSource)0, COD2_SUBTABLE_PTR(s_lightConsts), 2, 1 },
    { "lightGrid", (MaterialTextureSource)0, COD2_SUBTABLE_PTR(s_lightGridConsts), 0, 0 },
    { "baseLightingCoords", (MaterialTextureSource)152, 0, 0, 0 },
    { "lightingLookupScale", (MaterialTextureSource)153, 0, 0, 0 },
    { "debugWeights", (MaterialTextureSource)154, 0, 0, 0 },
    { "ambientColor", (MaterialTextureSource)158, 0, 0, 0 },
    { "entityAmbient", (MaterialTextureSource)162, 0, 0, 0 },
    { "materialColor", (MaterialTextureSource)155, 0, 0, 0 },
    { "devgui", (MaterialTextureSource)179, 0, 0, 0 },
    { "fogConsts", (MaterialTextureSource)156, 0, 0, 0 },
    { "fogColor", (MaterialTextureSource)157, 0, 0, 0 },
    { "meanBrightness", (MaterialTextureSource)159, 0, 0, 0 },
    { "glowSetup", (MaterialTextureSource)160, 0, 0, 0 },
    { "glowApply", (MaterialTextureSource)161, 0, 0, 0 },
    { "filterTap", (MaterialTextureSource)163, 0, 8, 1 },
    { "renderTargetSize", (MaterialTextureSource)171, 0, 0, 0 },
    { "shadowmapSize", (MaterialTextureSource)172, 0, 0, 0 },
    { "shadowParms", (MaterialTextureSource)173, 0, 0, 0 },
    { "clipSpaceLookupScale", (MaterialTextureSource)174, 0, 0, 0 },
    { "clipSpaceLookupOffset", (MaterialTextureSource)175, 0, 0, 0 },
    { "outdoorFeatherParms", (MaterialTextureSource)178, 0, 0, 0 },
    { "gameTime", (MaterialTextureSource)130, 0, 0, 0 },
    { "particleCloudColor", (MaterialTextureSource)176, 0, 0, 0 },
    { "particleCloudMatrix", (MaterialTextureSource)177, 0, 0, 0 },
    { "worldMatrix", (MaterialTextureSource)188, 0, 0, 0 },
    { "inverseWorldMatrix", (MaterialTextureSource)189, 0, 0, 0 },
    { "transposeWorldMatrix", (MaterialTextureSource)190, 0, 0, 0 },
    { "inverseTransposeWorldMatrix", (MaterialTextureSource)191, 0, 0, 0 },
    { "viewMatrix", (MaterialTextureSource)192, 0, 0, 0 },
    { "inverseViewMatrix", (MaterialTextureSource)193, 0, 0, 0 },
    { "transposeViewMatrix", (MaterialTextureSource)194, 0, 0, 0 },
    { "inverseTransposeViewMatrix", (MaterialTextureSource)195, 0, 0, 0 },
    { "projectionMatrix", (MaterialTextureSource)196, 0, 0, 0 },
    { "inverseProjectionMatrix", (MaterialTextureSource)197, 0, 0, 0 },
    { "transposeProjectionMatrix", (MaterialTextureSource)198, 0, 0, 0 },
    { "inverseTransposeProjectionMatrix", (MaterialTextureSource)199, 0, 0, 0 },
    { "worldViewMatrix", (MaterialTextureSource)200, 0, 0, 0 },
    { "inverseWorldViewMatrix", (MaterialTextureSource)201, 0, 0, 0 },
    { "transposeWorldViewMatrix", (MaterialTextureSource)202, 0, 0, 0 },
    { "inverseTransposeWorldViewMatrix", (MaterialTextureSource)203, 0, 0, 0 },
    { "viewProjectionMatrix", (MaterialTextureSource)204, 0, 0, 0 },
    { "inverseViewProjectionMatrix", (MaterialTextureSource)205, 0, 0, 0 },
    { "transposeViewProjectionMatrix", (MaterialTextureSource)206, 0, 0, 0 },
    { "inverseTransposeViewProjectionMatrix", (MaterialTextureSource)207, 0, 0, 0 },
    { "worldViewProjectionMatrix", (MaterialTextureSource)208, 0, 0, 0 },
    { "inverseWorldViewProjectionMatrix", (MaterialTextureSource)209, 0, 0, 0 },
    { "transposeWorldViewProjectionMatrix", (MaterialTextureSource)210, 0, 0, 0 },
    { "inverseTransposeWorldViewProjectionMatrix", (MaterialTextureSource)211, 0, 0, 0 },
    { "OpenGLworldViewProjectionMatrix", (MaterialTextureSource)212, 0, 0, 0 },
    { "OpenGinverseWorldViewProjectionMatrix", (MaterialTextureSource)213, 0, 0, 0 },
    { "OpenGtransposeWorldViewProjectionMatrix", (MaterialTextureSource)214, 0, 0, 0 },
    { "OpenGinverseTransposeWorldViewProjectionMatrix", (MaterialTextureSource)215, 0, 0, 0 },
    { "normalizedWorldMatrix", (MaterialTextureSource)216, 0, 0, 0 },
    { "inverseNormalizedWorldMatrix", (MaterialTextureSource)217, 0, 0, 0 },
    { "transposeNormalizedWorldMatrix", (MaterialTextureSource)218, 0, 0, 0 },
    { "inverseTransposeNormalizedWorldMatrix", (MaterialTextureSource)219, 0, 0, 0 },
    { "normalizedWorldViewMatrix", (MaterialTextureSource)220, 0, 0, 0 },
    { "inverseNormalizedWorldViewMatrix", (MaterialTextureSource)221, 0, 0, 0 },
    { "transposeNormalizedWorldViewMatrix", (MaterialTextureSource)222, 0, 0, 0 },
    { "inverseTransposeNormalizedWorldViewMatrix", (MaterialTextureSource)223, 0, 0, 0 },
    { "inverseNormalizedWorldViewProjectionMatrix", (MaterialTextureSource)225, 0, 0, 0 },
    { "normalizedWorldViewProjectionMatrix", (MaterialTextureSource)224, 0, 0, 0 },
    { "transposeNormalizedWorldViewProjectionMatrix", (MaterialTextureSource)226, 0, 0, 0 },
    { "inverseTransposeNormalizedWorldViewProjectionMatrix", (MaterialTextureSource)227, 0, 0, 0 },
    { "shadowLookupMatrix", (MaterialTextureSource)228, 0, 0, 0 },
    { "inverseShadowLookupMatrix", (MaterialTextureSource)229, 0, 0, 0 },
    { "transposeShadowLookupMatrix", (MaterialTextureSource)230, 0, 0, 0 },
    { "inverseTransposeShadowLookupMatrix", (MaterialTextureSource)231, 0, 0, 0 },
    { "lightGridLookupMatrix", (MaterialTextureSource)232, 0, 0, 0 },
    { "inverseLightGridLookupMatrix", (MaterialTextureSource)233, 0, 0, 0 },
    { "transposeLightGridLookupMatrix", (MaterialTextureSource)234, 0, 0, 0 },
    { "inverseTransposeLightGridLookupMatrix", (MaterialTextureSource)235, 0, 0, 0 },
    { "worldOutdoorLookupMatrix", (MaterialTextureSource)236, 0, 0, 0 },
    { "inverseWorldOutdoorLookupMatrix", (MaterialTextureSource)237, 0, 0, 0 },
    { "transposeWorldOutdoorLookupMatrix", (MaterialTextureSource)238, 0, 0, 0 },
    { "inverseTransposeWorldOutdoorLookupMatrix", (MaterialTextureSource)239, 0, 0, 0 },
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};

const CodeSamplerSource s_codeSamplers[] = {
    { "specularity", (MaterialTextureSource)3, 0, 0, 0 },
    { "white", (MaterialTextureSource)1, 0, 0, 0 },
    { "black", (MaterialTextureSource)0, 0, 0, 0 },
    { "identityNormalMap", (MaterialTextureSource)2, 0, 0, 0 },
    { "lightmap", (MaterialTextureSource)8, COD2_SUBTABLE_PTR(s_lightmapSamplers), 0, 0 },
    { "outdoor", (MaterialTextureSource)20, 0, 0, 0 },
    { "shadowCookie", (MaterialTextureSource)12, 0, 0, 0 },
    { "dynamicShadow", (MaterialTextureSource)19, 0, 0, 0 },
    { "feedback", (MaterialTextureSource)13, 0, 0, 0 },
    { "resolvedPostSun", (MaterialTextureSource)14, 0, 0, 0 },
    { "resolvedScene", (MaterialTextureSource)15, 0, 0, 0 },
    { "sky", (MaterialTextureSource)16, 0, 0, 0 },
    { "light", (MaterialTextureSource)17, COD2_SUBTABLE_PTR(s_lightSamplers), 2, 1 },
    { "lightGrid", (MaterialTextureSource)5, COD2_SUBTABLE_PTR(s_lightGridSamplers), 0, 0 },
    { "floatZ", (MaterialTextureSource)21, 0, 0, 0 },
    { "waterColor", (MaterialTextureSource)23, 0, 0, 0 },
    { "sunHalfAngle", (MaterialTextureSource)22, 0, 0, 0 },
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};

const CodeConstantSource s_defaultCodeConsts[] = {
    { "eyePosition", (MaterialTextureSource)139, 0, 0, 0 },
    { "eyeForward", (MaterialTextureSource)140, 0, 0, 0 },
    { "eyeLeft", (MaterialTextureSource)141, 0, 0, 0 },
    { "eyeUp", (MaterialTextureSource)142, 0, 0, 0 },
    { "nearPlaneOrg", (MaterialTextureSource)143, 0, 0, 0 },
    { "nearPlaneDx", (MaterialTextureSource)144, 0, 0, 0 },
    { "nearPlaneDy", (MaterialTextureSource)145, 0, 0, 0 },
    { "lightPosition0", (MaterialTextureSource)131, 0, 0, 0 },
    { "lightPosition1", (MaterialTextureSource)132, 0, 0, 0 },
    { "lightColor0", (MaterialTextureSource)133, 0, 0, 0 },
    { "lightColor1", (MaterialTextureSource)134, 0, 0, 0 },
    { "lightAmbient0", (MaterialTextureSource)135, 0, 0, 0 },
    { "lightAmbient1", (MaterialTextureSource)136, 0, 0, 0 },
    { "lightSpecular0", (MaterialTextureSource)137, 0, 0, 0 },
    { "lightSpecular1", (MaterialTextureSource)138, 0, 0, 0 },
    { "lightGridColorsR0", (MaterialTextureSource)146, 0, 0, 0 },
    { "lightGridColorsR1", (MaterialTextureSource)147, 0, 0, 0 },
    { "lightGridColorsG0", (MaterialTextureSource)148, 0, 0, 0 },
    { "lightGridColorsG1", (MaterialTextureSource)149, 0, 0, 0 },
    { "lightGridColorsB0", (MaterialTextureSource)150, 0, 0, 0 },
    { "lightGridColorsB1", (MaterialTextureSource)151, 0, 0, 0 },
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};

const CodeSamplerSource s_defaultCodeSamplers[] = {
    { "specularitySampler", (MaterialTextureSource)3, 0, 0, 0 },
    { "shadowCookieSampler", (MaterialTextureSource)12, 0, 0, 0 },
    { "feedbackSampler", (MaterialTextureSource)13, 0, 0, 0 },
    { "dynamicShadowSampler", (MaterialTextureSource)19, 0, 0, 0 },
    { "floatZSampler", (MaterialTextureSource)21, 0, 0, 0 },
    { "attenuationSampler", (MaterialTextureSource)17, 0, 0, 0 },
    { "lightmapWeightSampler", (MaterialTextureSource)7, 0, 0, 0 },
    { "lightmapSamplerR", (MaterialTextureSource)8, 0, 0, 0 },
    { "lightmapSamplerG", (MaterialTextureSource)9, 0, 0, 0 },
    { "lightmapSamplerB", (MaterialTextureSource)10, 0, 0, 0 },
    { "lightmapSamplerSun", (MaterialTextureSource)11, 0, 0, 0 },
    { "lightGridWeightSampler0", (MaterialTextureSource)5, 0, 0, 0 },
    { "lightGridWeightSampler1", (MaterialTextureSource)6, 0, 0, 0 },
    { "smodelLightingSampler", (MaterialTextureSource)4, 0, 0, 0 },
    { "skySampler", (MaterialTextureSource)16, 0, 0, 0 },   /* not 1.3's: r_water reflects the map's sky */
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};

const CodeConstantSource s_cameraConsts[] = {
    { "position", (MaterialTextureSource)139, 0, 0, 0 },
    { "forward", (MaterialTextureSource)140, 0, 0, 0 },
    { "left", (MaterialTextureSource)141, 0, 0, 0 },
    { "up", (MaterialTextureSource)142, 0, 0, 0 },
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};

const CodeConstantSource s_lightConsts[] = {
    { "position", (MaterialTextureSource)131, 0, 0, 0 },
    { "color", (MaterialTextureSource)133, 0, 0, 0 },
    { "ambient", (MaterialTextureSource)135, 0, 0, 0 },
    { "specular", (MaterialTextureSource)137, 0, 0, 0 },
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};

const CodeConstantSource s_lightGridConsts[] = {
    { "colorsR", (MaterialTextureSource)146, 0, 2, 1 },
    { "colorsG", (MaterialTextureSource)148, 0, 2, 1 },
    { "colorsB", (MaterialTextureSource)150, 0, 2, 1 },
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};

const CodeSamplerSource s_lightGridSamplers[] = {
    { "weights", (MaterialTextureSource)5, 0, 2, 1 },
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};

const CodeSamplerSource s_lightSamplers[] = {
    { "attenuation", (MaterialTextureSource)17, 0, 0, 0 },
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};

const CodeSamplerSource s_lightmapSamplers[] = {
    { "weights", (MaterialTextureSource)7, 0, 0, 0 },
    { "colorsR", (MaterialTextureSource)8, 0, 0, 0 },
    { "colorsG", (MaterialTextureSource)9, 0, 0, 0 },
    { "colorsB", (MaterialTextureSource)10, 0, 0, 0 },
    { "sun", (MaterialTextureSource)11, 0, 0, 0 },
    { "traditional", (MaterialTextureSource)8, 0, 0, 0 },
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};

const CodeConstantSource s_nearPlaneConsts[] = {
    { "org", (MaterialTextureSource)143, 0, 0, 0 },
    { "dx", (MaterialTextureSource)144, 0, 0, 0 },
    { "dy", (MaterialTextureSource)145, 0, 0, 0 },
    { 0, (MaterialTextureSource)0, 0, 0, 0 }
};
