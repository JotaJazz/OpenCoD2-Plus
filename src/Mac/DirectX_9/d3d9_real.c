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
