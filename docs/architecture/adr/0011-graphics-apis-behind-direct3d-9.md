# 0011. Graphics APIs behind Direct3D 9

- Status: Accepted
- Date: 2026-10-31

## Context

The goal is to run on Vulkan, Direct3D 12 and modern OpenGL. The renderer is 38,000 lines that talk to an `IDirect3DDevice9` and the game's shaders are Direct3D 9 HLSL. Only the device behind that interface changes between the builds ([0008](0008-windows-client-on-real-direct3d-9.md), [0009](0009-linux-client-on-dxvk.md)).

## Decision

The Windows client takes the API from `r_gfxApi` whenever it makes a device, at the start and at `vid_restart`: `d3d9`, `vulkan` loads DXVK from `dxvk/d3d9.dll` next to the executable, and `d3d12` asks the system's `d3d9.dll` for D3D9On12. An API that cannot run, or cannot make a device, falls back to `d3d9` and says so; `vulkan` first asks Vulkan for a 1.3 device, because DXVK throws out of `Direct3DCreate9` without one. Options > Graphics offers `d3d9` and `vulkan` in the game's rendering method item, patched in memory when the menu loads. The item sets the dvar at once, unlike the stock items, and the device changes at the menu's Apply. Direct3D 11 gets no path of its own: 12 covers the same machines through the system.

## Consequences

- The renderer does not change, and on Vulkan the picture matches the Direct3D 9 path.
- `d3d12` is experimental: it shows a black screen ([debt](../../roadmap/debt.md)).
- These are translation layers: they cost some speed and open no new effects.
- The client is large address aware, since DXVK maps its memory into the 32-bit process.
- `r_gfxApi` is not a Linux setting: `cod2_linux_vk` is always on Vulkan.
