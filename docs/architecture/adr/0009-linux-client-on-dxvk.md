# 0009. Linux client on DXVK native

- Status: Accepted
- Date: 2026-10-22

## Context

The Linux client draws with the Mac port's GL layer and its stand-in ARB programs (see [0008](0008-windows-client-on-real-direct3d-9.md)). The Windows client's real Direct3D 9 path draws the game's shaders. The two ways to bring them to Linux: translate the game's HLSL to GLSL for the GL layer, or run the real Direct3D 9 path on a Direct3D 9 implementation for Linux.

## Decision

A second Linux client, `cod2_linux_vk` (`-DCOD2_LINUX_DXVK=ON`), runs the real Direct3D 9 path on DXVK native (Direct3D 9 over Vulkan) and compiles the game's HLSL with vkd3d-shader. Both are built from pinned releases by `scripts/build_linux_vk_deps.sh`, not committed, and loaded with `dlopen` from next to the executable. `cod2_linux` keeps the GL device.

## Consequences

- Linux runs the same renderer code as Windows; only the device, the window and the shader compiler differ.
- `cod2_linux_vk` needs a Vulkan device; under WSL that is llvmpipe, on the processor.
- Making it the default Linux client waits on testing with a GPU and on shipping the two libraries.
