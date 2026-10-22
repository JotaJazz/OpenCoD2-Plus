/* Linux real Direct3D 9 (GFX_REAL_D3D9): the game's HLSL compiled by
 * vkd3d-shader in place of d3dx9's D3DXCompileShader */
#if defined(GFX_REAL_D3D9) && !defined(_WIN32)

typedef long HRESULT;
typedef unsigned int UINT;

HRESULT VkD3DX_CompileShader(const char *src, UINT len, const void *defines, void *include,
                             const char *entry, const char *profile, void **shader, void **errors)
{
    (void)src, (void)len, (void)defines, (void)include, (void)entry, (void)profile;
    if (shader)
        *shader = 0;
    if (errors)
        *errors = 0;
    return (HRESULT)0x80004005;
}

int VkD3DX_GetShaderConstantTable(const void *function, void **constantTable)
{
    (void)function;
    if (constantTable)
        *constantTable = 0;
    return (int)0x80004005;
}

#endif
