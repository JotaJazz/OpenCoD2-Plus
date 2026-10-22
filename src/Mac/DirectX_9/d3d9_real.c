#ifdef GFX_REAL_D3D9

#    include <stdio.h>
#    include <string.h>

#    ifdef _WIN32
#        include <windows.h>

typedef void *(__stdcall *PFN_Direct3DCreate9)(UINT SDKVersion);

void *RealD3D9_Direct3DCreate9(void)
{
    static HMODULE d3d9mod;
    static PFN_Direct3DCreate9 pCreate;
    void *iface;

    if (!pCreate) {
        d3d9mod = LoadLibraryA("d3d9.dll");
        if (!d3d9mod) {
            fprintf(stderr, "[real-d3d9] LoadLibrary(d3d9.dll) failed\n");
            return NULL;
        }
        pCreate = (PFN_Direct3DCreate9)GetProcAddress(d3d9mod, "Direct3DCreate9");
        if (!pCreate) {
            fprintf(stderr, "[real-d3d9] GetProcAddress(Direct3DCreate9) failed\n");
            return NULL;
        }
    }
    iface = pCreate(32 );
    fprintf(stderr, "[real-d3d9] Direct3DCreate9 -> %p\n", iface);
    return iface;
}

#    else
/* Linux: DXVK native's Direct3D 9 (scripts/build_linux_vk_deps.sh), from
 * next to the executable first, then the library path */
#        include <dlfcn.h>
#        include <stdlib.h>
#        include <unistd.h>
#        include <SDL2/SDL.h>

typedef void *(*PFN_Direct3DCreate9)(unsigned int SDKVersion);

/* whether the Vulkan loader finds a physical device: a Vulkan instance with
 * nothing enabled, as DXVK's own would find it */
static int RealD3D9_VulkanHasDevice(void)
{
    struct VkInstanceCreateInfoMin {
        int sType; /* VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO */
        const void *pNext;
        unsigned int flags;
        const void *pApplicationInfo;
        unsigned int enabledLayerCount;
        const char *const *ppEnabledLayerNames;
        unsigned int enabledExtensionCount;
        const char *const *ppEnabledExtensionNames;
    } info = { 1 };
    typedef void *(*PFN_GetProc)(void *instance, const char *name);
    typedef int (*PFN_CreateInstance)(const void *info, const void *alloc, void **instance);
    typedef int (*PFN_Enumerate)(void *instance, unsigned int *count, void **devices);
    typedef void (*PFN_DestroyInstance)(void *instance, const void *alloc);
    void *lib = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    PFN_GetProc getProc;
    PFN_CreateInstance createInstance;
    PFN_Enumerate enumerate;
    PFN_DestroyInstance destroyInstance;
    void *instance = NULL;
    unsigned int count = 0;

    if (!lib) {
        fprintf(stderr, "[real-d3d9] no Vulkan loader (libvulkan.so.1): %s\n", dlerror());
        return 0;
    }
    getProc = (PFN_GetProc)dlsym(lib, "vkGetInstanceProcAddr");
    createInstance = getProc ? (PFN_CreateInstance)getProc(NULL, "vkCreateInstance") : NULL;
    if (!createInstance || createInstance(&info, NULL, &instance) != 0 || !instance) {
        fprintf(stderr, "[real-d3d9] vkCreateInstance failed\n");
        dlclose(lib);
        return 0;
    }
    enumerate = (PFN_Enumerate)getProc(instance, "vkEnumeratePhysicalDevices");
    destroyInstance = (PFN_DestroyInstance)getProc(instance, "vkDestroyInstance");
    if (enumerate)
        enumerate(instance, &count, NULL);
    if (destroyInstance)
        destroyInstance(instance, NULL);
    dlclose(lib);
    if (!count)
        fprintf(stderr, "[real-d3d9] Vulkan finds no device\n");
    return count != 0;
}

void *RealD3D9_Direct3DCreate9(void)
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
    /* DXVK ends the process when Vulkan has no device: look first */
    if (!RealD3D9_VulkanHasDevice())
        return NULL;
    /* DXVK native needs its window system named, and SDL's video up to ask
     * Vulkan's instance extensions; a failure is a C++ exception */
    setenv("DXVK_WSI_DRIVER", "SDL2", 0);
    if (!SDL_WasInit(SDL_INIT_VIDEO) && SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[real-d3d9] SDL_InitSubSystem(VIDEO) failed: %s\n", SDL_GetError());
        return NULL;
    }
    iface = NULL;
#        ifdef __cplusplus
    try {
        iface = pCreate(32);
    } catch (...) {
        fprintf(stderr, "[real-d3d9] Direct3DCreate9 failed (DXVK error, no Vulkan device?)\n");
        return NULL;
    }
#        else
    iface = pCreate(32);
#        endif
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

void RealD3D9_FillPresentParams(void *ppOut, void *hwnd, int w, int h)
{
    unsigned char *p = (unsigned char *)ppOut;
    memset(p, 0, 56);
    *(unsigned int *)(p + 0) = (unsigned int)w;
    *(unsigned int *)(p + 4) = (unsigned int)h;
    *(unsigned int *)(p + 8) = 22;
    *(unsigned int *)(p + 12) = 1;

    *(unsigned int *)(p + 24) = 1;
    *(void **)(p + 28) = hwnd;
    *(int *)(p + 32) = 1;
    *(int *)(p + 36) = 1;
    *(unsigned int *)(p + 40) = 75;

    *(unsigned int *)(p + 52) = 0;
}

#endif
