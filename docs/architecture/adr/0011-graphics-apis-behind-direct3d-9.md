# 0011. Graphics APIs behind Direct3D 9

- Status: Accepted
- Date: 2026-10-31

## Context

The goal is to run on Vulkan, Direct3D 12 and modern OpenGL. The renderer is 38,000 lines that talk to an `IDirect3DDevice9` and the game's shaders are Direct3D 9 HLSL. Only the device behind that interface changes between the builds ([0008](0008-windows-client-on-real-direct3d-9.md), [0009](0009-linux-client-on-dxvk.md)).

## Decision

The Windows client takes the API from `r_gfxApi`, once per process: `d3d9`, `vulkan` loads DXVK from `dxvk/d3d9.dll` next to the executable, and `d3d12` asks the system's `d3d9.dll` for D3D9On12. An API that cannot run falls back to `d3d9` and says so. Direct3D 11 gets no path of its own: 12 covers the same machines through the system.

## Consequences

- The renderer does not change, and the picture matches the Direct3D 9 path.
- These are translation layers: they cost some speed and open no new effects.
- The executable is large address aware, since DXVK maps its memory into the 32-bit process.
- `r_gfxApi` is not a Linux setting: `cod2_linux_vk` is always on Vulkan.
