# Renderer

`src/PC/gfx_d3d`: the reconstruction of `gfx_d3d_mp_x86_s.dll` 1.3, linked
into the client (no separate DLL: [ADR 0007](adr/0007-remove-the-renderer-dll-variant.md)).

## Devices

| Build | Device | Shaders |
|---|---|---|
| Windows client (default, `COD2_WIN32_REAL_D3D9=ON`, `GFX_REAL_D3D9`) | real `IDirect3DDevice9` | the game's `materials/shaders/*.hlsl`, compiled with the system's d3dx9 (`d3dx9_43` down to `d3dx9_24`), `//` comments blanked first (Wine's compiler rejects one after an `#include`) |
| Windows client with `COD2_WIN32_REAL_D3D9=OFF`, Linux client | Mac port's GL layer behind a Direct3D interface (`src/Mac`) | hand-written ARB programs picked by HLSL keywords: no sun, no specular, white world |

Decision: [ADR 0008](adr/0008-windows-client-on-real-direct3d-9.md).

## Real Direct3D 9 path

| Item | Detail |
|---|---|
| Window | an SDL window (`Sys_CreateD3DWindow`, `src/stubs/macos_compat.c`) so `IN_Frame` keeps the input; size from `r_mode`; `r_fullscreen 1` covers the desktop and the frame of `r_mode`'s size is stretched over it |
| Modes | `r_mode` lists the display's modes (`Sys_EnumDisplayModes`, via SDL), as the original lists the adapter's |
| Aspect | `r_aspectRatio` (`0x10011610`): 4:3, 16:10, 16:9; auto rounds the window's shape, or the monitor's when full screen |
| Screenshots | `GetRenderTargetData` of the back buffer before `Present` |
| COM calls | through `D3DVTCC` (stdcall on a real device, empty for the GL device) |
| Picmip | by texture memory, as `0x1000d6a0`; Windows system info from `GlobalMemoryStatusEx`, the processor's registry key and the primary display device |
| `vid_restart` | refused while a local server runs ("Listen server cannot video restart"), as the original |

## Tables filled from the original

| Table | Address in the 1.3 dll | Without it |
|---|---|---|
| light grid basis, cube face axes (`$lightgridweights`) | `0x100104c0`, `0x10010180` | view weapon and players black |
| particle cloud quad indices and corners | `0x10011c40` | every quad a point |
| particle cloud draw counts (`RB_TessParticleCloud`) | `0x10041405` | no cloud drawn |
| outdoor map size `{512, 512, 256}` | `0x10190398` | nothing outdoors |

## Outdoor map

`src/PC/gfx_d3d/r_outdoor.c`: a 512×512 height map of the world, traced from
above the world's bounds (max z + 1) down to min z − 1 (`0x10020790`); each
texel is the hit height scaled to 0..255. `particle_cloud_outdoor` hides a
cloud particle below it (indoors). Under Wine that shader does not compile
(see [../roadmap/debt.md](../roadmap/debt.md)).

## Static model lighting

`*smodel_lighting` is a volume texture; `Image_UploadData` writes volumes and
`R_CacheStaticModelLighting` writes texels as B, G, R, A (`A8R8G8B8`). The GL
device's volumes cannot be locked, so that path is unchanged.
