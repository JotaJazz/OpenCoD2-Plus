#ifdef GFX_REAL_D3D9

#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

#    ifdef _WIN32
#        include <windows.h>

typedef void *(__stdcall *PFN_Direct3DCreate9)(UINT SDKVersion);

/* Direct3D 9 on Direct3D 12, in d3d9.dll since Windows 10 (d3d9on12.h) */
typedef struct {
    BOOL Enable9On12;
    void *pD3D12Device;
    void *ppD3D12Queues[2];
    UINT NumQueues;
    UINT NodeMask;
} D3D9ON12_ARGS;
typedef void *(__stdcall *PFN_Direct3DCreate9On12)(UINT SDKVersion, D3D9ON12_ARGS *pOverrideList, UINT NumOverrideEntries);

/* r_gfxApi's values */
enum { GFX_API_D3D9, GFX_API_VULKAN, GFX_API_D3D12 };

/* an interface with an adapter, or NULL */
static void *RealD3D9_Usable(void *iface)
{
    /* IUnknown's three and RegisterSoftwareDevice, then GetAdapterCount */
    void **vtbl = iface ? *(void ***)iface : NULL;

    if (iface && !((UINT(__stdcall *)(void *))vtbl[4])(iface)) {
        ((ULONG(__stdcall *)(void *))vtbl[2])(iface);
        return NULL;
    }
    return iface;
}

static void *RealD3D9_CreateFrom(HMODULE d3d9mod)
{
    PFN_Direct3DCreate9 pCreate = d3d9mod ? (PFN_Direct3DCreate9)GetProcAddress(d3d9mod, "Direct3DCreate9") : NULL;

    return pCreate ? RealD3D9_Usable(pCreate(32)) : NULL;
}

/* the Vulkan API types this asks for, for 32-bit Windows */
typedef struct {
    int sType;
    const void *pNext;
    const char *pApplicationName;
    unsigned int applicationVersion;
    const char *pEngineName;
    unsigned int engineVersion;
    unsigned int apiVersion;
} GfxVkApplicationInfo;
typedef struct {
    int sType;
    const void *pNext;
    unsigned int flags;
    const GfxVkApplicationInfo *pApplicationInfo;
    unsigned int enabledLayerCount;
    const char *const *ppEnabledLayerNames;
    unsigned int enabledExtensionCount;
    const char *const *ppEnabledExtensionNames;
} GfxVkInstanceCreateInfo;
#    define GFX_VK_API_1_3 ((1u << 22) | (3u << 12))

/* whether a device has Vulkan 1.3. DXVK throws out of Direct3DCreate9 without
 * one, and a client saved on vulkan would then die at every start. */
static int RealD3D9_HasVulkan13(HMODULE vulkan)
{
    typedef void *(__stdcall * PFN_GetProc)(void *, const char *);
    typedef int(__stdcall * PFN_CreateInstance)(const GfxVkInstanceCreateInfo *, const void *, void **);
    typedef int(__stdcall * PFN_EnumerateDevices)(void *, unsigned int *, void **);
    typedef void(__stdcall * PFN_GetProperties)(void *, void *);
    typedef void(__stdcall * PFN_DestroyInstance)(void *, const void *);
    PFN_GetProc getProc = (PFN_GetProc)GetProcAddress(vulkan, "vkGetInstanceProcAddr");
    PFN_CreateInstance createInstance;
    PFN_EnumerateDevices enumerateDevices;
    PFN_GetProperties getProperties;
    PFN_DestroyInstance destroyInstance;
    GfxVkApplicationInfo app;
    GfxVkInstanceCreateInfo info;
    void *instance = NULL;
    void *devices[16];
    unsigned int count = 16;
    unsigned int i;
    int found = 0;

    createInstance = getProc ? (PFN_CreateInstance)getProc(NULL, "vkCreateInstance") : NULL;
    if (!createInstance)
        return 0;
    memset(&app, 0, sizeof(app));
    app.sType = 0; /* VK_STRUCTURE_TYPE_APPLICATION_INFO */
    app.apiVersion = GFX_VK_API_1_3;
    memset(&info, 0, sizeof(info));
    info.sType = 1; /* VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO */
    info.pApplicationInfo = &app;
    if (createInstance(&info, NULL, &instance) != 0 || !instance)
        return 0;

    enumerateDevices = (PFN_EnumerateDevices)getProc(instance, "vkEnumeratePhysicalDevices");
    getProperties = (PFN_GetProperties)getProc(instance, "vkGetPhysicalDeviceProperties");
    destroyInstance = (PFN_DestroyInstance)getProc(instance, "vkDestroyInstance");
    if (enumerateDevices && getProperties && enumerateDevices(instance, &count, devices) >= 0) {
        for (i = 0; i < count && i < 16 && !found; i++) {
            /* VkPhysicalDeviceProperties starts with apiVersion */
            unsigned int properties[256];

            getProperties(devices[i], properties);
            found = properties[0] >= GFX_VK_API_1_3;
        }
    }
    if (destroyInstance)
        destroyInstance(instance, NULL);
    return found;
}

static HMODULE s_dxvkModule;

/* DXVK's d3d9.dll, in dxvk/ next to the executable: Direct3D 9 over Vulkan */
static void *RealD3D9_CreateVulkan(void)
{
    char path[MAX_PATH];
    char *slash;
    HMODULE vulkan = LoadLibraryA("vulkan-1.dll");

    if (!vulkan || !RealD3D9_HasVulkan13(vulkan))
        return NULL;
    if (!GetModuleFileNameA(NULL, path, sizeof(path)) || !(slash = strrchr(path, '\\'))
        || (size_t)(slash - path) + sizeof("\\dxvk\\d3d9.dll") > sizeof(path))
        return NULL;
    strcpy(slash, "\\dxvk\\d3d9.dll");
    s_dxvkModule = LoadLibraryA(path);
    return RealD3D9_CreateFrom(s_dxvkModule);
}

/* d3d9.dll by name, unless that would answer with DXVK's module, already
 * loaded under the same name: then the system's, by its path */
static HMODULE RealD3D9_LoadD3D9(void)
{
    char path[MAX_PATH];
    UINT length;

    if (!s_dxvkModule)
        return LoadLibraryA("d3d9.dll");
    length = GetSystemDirectoryA(path, sizeof(path));
    if (!length || length + sizeof("\\d3d9.dll") > sizeof(path))
        return NULL;
    strcpy(path + length, "\\d3d9.dll");
    return LoadLibraryA(path);
}

static void *RealD3D9_CreateD3D12(void)
{
    D3D9ON12_ARGS args;
    HMODULE d3d9mod = RealD3D9_LoadD3D9();
    PFN_Direct3DCreate9On12 pCreate;

    pCreate = d3d9mod ? (PFN_Direct3DCreate9On12)GetProcAddress(d3d9mod, "Direct3DCreate9On12") : NULL;
    if (!pCreate)
        return NULL;
    memset(&args, 0, sizeof(args));
    args.Enable9On12 = TRUE;
    return RealD3D9_Usable(pCreate(32, &args, 1));
}

static int s_apiInUse = GFX_API_D3D9;
/* presents since the interface was made with none S_OK; -1 once one was */
static int s_presentsUnseen;

/* 1 once, when d3d12 has presented 120 frames and none returned S_OK: the
 * black screen of a GPU it was never tried on, which the menu cannot leave.
 * shortcut: a black frame that returns S_OK goes unseen; checked only on an RTX 5080 */
int RealD3D9_D3D12ShowsNothing(long hr)
{
    if (s_apiInUse != GFX_API_D3D12 || s_presentsUnseen < 0)
        return 0;
    if (hr == 0) {
        s_presentsUnseen = -1;
        return 0;
    }
    return ++s_presentsUnseen == 120;
}

/* the API r_gfxApi asks for, or plain Direct3D 9 if that cannot run here */
void *RealD3D9_Direct3DCreate9(int api)
{
    extern void Com_Printf(const char *, ...);
    static const char *const names[] = { "Direct3D 9", "Vulkan (DXVK)", "Direct3D 12 (D3D9On12)" };
    void *iface = NULL;

    if (api == GFX_API_VULKAN)
        iface = RealD3D9_CreateVulkan();
    else if (api == GFX_API_D3D12)
        iface = RealD3D9_CreateD3D12();
    s_presentsUnseen = 0;
    if (iface) {
        Com_Printf("Graphics API: %s\n", names[api]);
        s_apiInUse = api;
        return iface;
    }
    if (api != GFX_API_D3D9)
        Com_Printf("^3%s is not available here, using Direct3D 9\n", names[api]);
    iface = RealD3D9_CreateFrom(RealD3D9_LoadD3D9());
    s_apiInUse = GFX_API_D3D9;
    if (iface)
        Com_Printf("Graphics API: %s\n", names[GFX_API_D3D9]);
    return iface;
}

#    else
/* Linux: DXVK native's Direct3D 9 (scripts/build_linux_vk_deps.sh), from
 * next to the executable first, then the library path */
#        include <dlfcn.h>
#        include <stdlib.h>
#        include <unistd.h>
#        include <SDL2/SDL.h>
#        include <sys/wait.h>
#        include <signal.h>

typedef void *(*PFN_Direct3DCreate9)(unsigned int SDKVersion);

/* DXVK ends the process, from inside, when Vulkan has no device it can use
 * (none, or only ones without Vulkan 1.3 and its features): try it first in a
 * child process, which tells with its exit status whether Direct3DCreate9
 * gave an adapter */
static int RealD3D9_DxvkHasAdapter(PFN_Direct3DCreate9 pCreate)
{
    pid_t pid = fork();
    int status = 0;

    if (pid < 0)
        return 1; /* cannot tell: let the real call decide */
    if (pid == 0) {
        static const int crashSignals[] = { SIGABRT, SIGSEGV, SIGBUS, SIGILL, SIGFPE };
        void *iface;
        unsigned int i;

        /* the client's crash handler would report the child's end as a crash */
        for (i = 0; i < sizeof(crashSignals) / sizeof(crashSignals[0]); i++)
            signal(crashSignals[i], SIG_DFL);
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
            _exit(2);
        iface = pCreate(32);
        /* IDirect3D9::GetAdapterCount, after IUnknown's three and RegisterSoftwareDevice */
        _exit(iface && ((unsigned int (*)(void *))(*(void ***)iface)[4])(iface) ? 0 : 1);
    }
    if (waitpid(pid, &status, 0) != pid)
        return 1;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        return 1;
    fprintf(stderr, "[real-d3d9] DXVK finds no Vulkan device it can use (%s %d)\n",
            WIFSIGNALED(status) ? "signal" : "status", WIFSIGNALED(status) ? WTERMSIG(status) : WEXITSTATUS(status));
    return 0;
}

void *RealD3D9_Direct3DCreate9(int api)
{
    extern void *Sys_LoadLibraryNextToExe(const char *name);
    static PFN_Direct3DCreate9 pCreate;
    void *mod;
    void *iface;

    if (!pCreate) {
        mod = Sys_LoadLibraryNextToExe("libdxvk_d3d9.so.0");
        if (!mod) {
            fprintf(stderr, "[real-d3d9] dlopen(libdxvk_d3d9.so.0) failed: %s\n", dlerror());
            return NULL;
        }
        pCreate = (PFN_Direct3DCreate9)dlsym(mod, "Direct3DCreate9");
        if (!pCreate) {
            fprintf(stderr, "[real-d3d9] dlsym(Direct3DCreate9) failed: %s\n", dlerror());
            return NULL;
        }
    }
    /* DXVK native needs its window system named, and SDL's video up to ask
     * Vulkan's instance extensions */
    setenv("DXVK_WSI_DRIVER", "SDL2", 0);
    if (!RealD3D9_DxvkHasAdapter(pCreate))
        return NULL;
    if (!SDL_WasInit(SDL_INIT_VIDEO) && SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[real-d3d9] SDL_InitSubSystem(VIDEO) failed: %s\n", SDL_GetError());
        return NULL;
    }
    iface = pCreate(32);
    fprintf(stderr, "[real-d3d9] Direct3DCreate9 -> %p\n", iface);
    return iface;
}

/* dlopen of NAME from the executable's directory, else from the library path */
void *Sys_LoadLibraryNextToExe(const char *name)
{
    char path[4096];
    ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
    void *mod = NULL;

    if (n > 0) {
        char *slash;

        path[n] = '\0';
        slash = strrchr(path, '/');
        if (slash && (size_t)(slash + 1 - path) + strlen(name) < sizeof(path)) {
            strcpy(slash + 1, name);
            mod = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        }
    }
    return mod ? mod : dlopen(name, RTLD_NOW | RTLD_LOCAL);
}
#    endif

void *RealD3D9_CreateGameWindow(int w, int h, int fullscreen)
{
    extern void *Sys_CreateD3DWindow(int width, int height, int fullscreen);
    void *hwnd = Sys_CreateD3DWindow(w, h, fullscreen);

    fprintf(stderr, "[real-d3d9] game window hwnd=%p %dx%d%s\n", hwnd, w, h, fullscreen ? " full screen" : "");
    return hwnd;
}

/* D3DPRESENT_PARAMETERS: natural alignment gives d3d9.h's layout on x86 and x64
 * (the window at 28 or 32, 56 or 64 bytes) */
typedef struct {
    unsigned int width, height, format, count, multiSample, multiSampleQuality, swapEffect;
    void *window;
    int windowed, autoDepthStencil;
    unsigned int depthStencilFormat, flags, refreshRate, interval;
} RealPresentParams;

int RealD3D9_PresentParamsSize(void)
{
    return (int)sizeof(RealPresentParams);
}

void RealD3D9_FillPresentParams(void *ppOut, void *hwnd, int w, int h, int swapInterval)
{
    RealPresentParams *pp = (RealPresentParams *)ppOut;

    memset(pp, 0, sizeof(*pp));
    pp->width = (unsigned int)w;
    pp->height = (unsigned int)h;
    pp->format = 22; /* D3DFMT_X8R8G8B8 */
    pp->count = 1;
    pp->swapEffect = 1; /* D3DSWAPEFFECT_DISCARD */
    pp->window = hwnd;
    pp->windowed = 1;
    pp->autoDepthStencil = 1;
    pp->depthStencilFormat = 75; /* D3DFMT_D24S8 */
    /* D3DPRESENT_INTERVAL_ONE or _IMMEDIATE: the default, 0, waits for the display */
    pp->interval = swapInterval ? 1 : 0x80000000u;

#    ifdef _WIN32
    /* D3D9On12 never shows a windowed frame (an RTX 5080, 2026-11-03: discard,
     * copy and flip all stay black, the back buffer holds the frame); exclusive
     * full screen at the desktop's refresh shows it */
    if (s_apiInUse == GFX_API_D3D12) {
        DEVMODEA mode;

        memset(&mode, 0, sizeof(mode));
        mode.dmSize = sizeof(mode);
        pp->windowed = 0;
        if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &mode))
            pp->refreshRate = mode.dmDisplayFrequency;
    }
#    endif
}

#endif
