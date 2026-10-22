/* Linux real Direct3D 9 (GFX_REAL_D3D9): the game's HLSL compiled by
 * vkd3d-shader (scripts/build_linux_vk_deps.sh) in place of d3dx9's
 * D3DXCompileShader, and the constant table d3dx9's
 * D3DXGetShaderConstantTable would give, read from the bytecode's CTAB */
#if defined(GFX_REAL_D3D9) && !defined(_WIN32)

#    include <dlfcn.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
#    define VKD3D_SHADER_NO_PROTOTYPES
#    include <vkd3d_shader.h>

typedef long HRESULT;
typedef unsigned int UINT;

#    define VKD3DX_E_FAIL ((HRESULT)0x80004005)

extern void *Sys_LoadLibraryNextToExe(const char *name);
extern void *D3DX_NewBuffer(const void *data, UINT size);
extern void *D3DX_NewConstantTable(const void *data, UINT size);

static PFN_vkd3d_shader_compile s_compile;
static PFN_vkd3d_shader_free_shader_code s_freeCode;
static PFN_vkd3d_shader_free_messages s_freeMessages;

static int VkD3DX_Load(void)
{
    static int tried;
    void *lib;

    if (tried)
        return s_compile != NULL;
    tried = 1;
    lib = Sys_LoadLibraryNextToExe("libvkd3d-shader.so.1");
    if (!lib) {
        fprintf(stderr, "[vkd3dx] dlopen(libvkd3d-shader.so.1) failed: %s\n", dlerror());
        return 0;
    }
    s_compile = (PFN_vkd3d_shader_compile)dlsym(lib, "vkd3d_shader_compile");
    s_freeCode = (PFN_vkd3d_shader_free_shader_code)dlsym(lib, "vkd3d_shader_free_shader_code");
    s_freeMessages = (PFN_vkd3d_shader_free_messages)dlsym(lib, "vkd3d_shader_free_messages");
    if (!s_compile || !s_freeCode || !s_freeMessages) {
        fprintf(stderr, "[vkd3dx] libvkd3d-shader.so.1 lacks vkd3d_shader_compile\n");
        s_compile = NULL;
        return 0;
    }
    return 1;
}

/* the d3dx9 include object's Open and Close (cdecl here, see D3DXShader.c) */
typedef int (*PFN_IncludeOpen)(void *self, int type, const char *name, const void *parent,
                               const void **data, UINT *bytes);
typedef int (*PFN_IncludeClose)(void *self, const void *data);

static int VkD3DX_OpenInclude(const char *filename, bool local, const char *parent_data, void *context,
                              struct vkd3d_shader_code *out)
{
    void *include = context;
    const void *data = NULL;
    UINT bytes = 0;

    if (!include || ((PFN_IncludeOpen)(*(void ***)include)[0])(include, local ? 0 : 1, filename, parent_data,
                                                                &data, &bytes) < 0)
        return -1; /* VKD3D_ERROR */
    out->code = data;
    out->size = bytes;
    return 0;
}

static void VkD3DX_CloseInclude(const struct vkd3d_shader_code *code, void *context)
{
    if (context)
        ((PFN_IncludeClose)(*(void ***)context)[1])(context, code->code);
}

static HRESULT VkD3DX_Compile(const char *src, UINT len, const void *defines, void *include, const char *entry,
                              const char *profile, void **shader, void **errors)
{
    const char *const *def = (const char *const *)defines;
    struct vkd3d_shader_macro macros[32];
    unsigned int macroCount = 0;
    struct vkd3d_shader_preprocess_info preprocess;
    struct vkd3d_shader_hlsl_source_info hlsl;
    struct vkd3d_shader_compile_info info;
    struct vkd3d_shader_code out = { 0 };
    char *messages = NULL;
    int result;

    if (shader)
        *shader = NULL;
    if (errors)
        *errors = NULL;
    if (!VkD3DX_Load())
        return VKD3DX_E_FAIL;

    /* D3DXMACRO pairs, ended by a NULL name */
    for (; def && def[0] && macroCount < sizeof(macros) / sizeof(macros[0]); def += 2, macroCount++) {
        macros[macroCount].name = def[0];
        macros[macroCount].value = def[1] ? def[1] : "";
    }

    memset(&preprocess, 0, sizeof(preprocess));
    preprocess.type = VKD3D_SHADER_STRUCTURE_TYPE_PREPROCESS_INFO;
    preprocess.macros = macros;
    preprocess.macro_count = macroCount;
    preprocess.pfn_open_include = VkD3DX_OpenInclude;
    preprocess.pfn_close_include = VkD3DX_CloseInclude;
    preprocess.include_context = include;

    memset(&hlsl, 0, sizeof(hlsl));
    hlsl.type = VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO;
    hlsl.next = &preprocess;
    hlsl.entry_point = entry;
    hlsl.profile = profile;

    memset(&info, 0, sizeof(info));
    info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    info.next = &hlsl;
    info.source.code = src;
    info.source.size = len;
    info.source_type = VKD3D_SHADER_SOURCE_HLSL;
    info.target_type = VKD3D_SHADER_TARGET_D3D_BYTECODE;
    info.log_level = VKD3D_SHADER_LOG_WARNING;
    info.source_name = entry;

    result = s_compile(&info, &out, &messages);
    if (result >= 0 && shader)
        *shader = D3DX_NewBuffer(out.code, (UINT)out.size);
    /* d3dx9 gives its messages only on failure; vkd3d warns on most of the
     * game's shaders (implicit truncation) */
    if (result < 0 && messages && errors)
        *errors = D3DX_NewBuffer(messages, (UINT)strlen(messages) + 1);
    if (result >= 0)
        s_freeCode(&out);
    s_freeMessages(messages);
    return result >= 0 ? 0 : VKD3DX_E_FAIL;
}

/* VKD3DX_SELFTEST=1: an include the game lacks is an empty file, as with
 * d3dx9, and an HLSL error fails with a message instead of a crash */
static void VkD3DX_SelfTest(void *include)
{
    static const char src[] = "#include \"vkd3dx_selftest_missing.hlsl\"\n"
                              "float4 ps_main() : COLOR { return vkd3dx_undefined_name; }\n";
    void *shader = NULL;
    void *errors = NULL;
    HRESULT hr = VkD3DX_Compile(src, sizeof(src) - 1, NULL, include, "ps_main", "ps_2_0", &shader, &errors);

    fprintf(stderr, "[vkd3dx-selftest] %s\n", (hr < 0 && errors && !shader) ? "ok" : "FAIL");
}

HRESULT VkD3DX_CompileShader(const char *src, UINT len, const void *defines, void *include,
                             const char *entry, const char *profile, void **shader, void **errors)
{
    static int selfTested;

    if (!selfTested) {
        selfTested = 1;
        if (getenv("VKD3DX_SELFTEST"))
            VkD3DX_SelfTest(include);
    }
    return VkD3DX_Compile(src, len, defines, include, entry, profile, shader, errors);
}

/* the CTAB comment of Direct3D 9 bytecode: after the version token, comment
 * tokens (0xfffe, length in dwords in the high half), the constant table's
 * starting with its fourcc; d3dx9's table points just after the fourcc */
int VkD3DX_GetShaderConstantTable(const void *function, void **constantTable)
{
    const unsigned int *tok = (const unsigned int *)function;
    unsigned int i;

    if (constantTable)
        *constantTable = NULL;
    if (!tok)
        return (int)VKD3DX_E_FAIL;
    for (i = 1; (tok[i] & 0xffff) == 0xfffe; i += 1 + (tok[i] >> 16)) {
        unsigned int length = tok[i] >> 16;

        if (length >= 1 && tok[i + 1] == 0x42415443) { /* 'CTAB' */
            if (constantTable)
                *constantTable = D3DX_NewConstantTable(&tok[i + 2], (length - 1) * 4);
            return 0;
        }
    }
    return (int)VKD3DX_E_FAIL;
}

#endif
