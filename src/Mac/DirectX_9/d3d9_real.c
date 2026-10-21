#ifdef GFX_REAL_D3D9

#    include <windows.h>
#    include <stdio.h>

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
