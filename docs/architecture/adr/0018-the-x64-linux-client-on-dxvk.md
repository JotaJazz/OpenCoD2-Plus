# 0018. The x64 Linux client on DXVK too

- Status: Accepted
- Date: 2026-11-08

## Context

[0017](0017-dxvk-is-the-default-linux-client.md) made DXVK the default Linux client but kept the 64-bit client (`-DCOD2_X64=ON`) on the GL device, since `scripts/build_linux_vk_deps.sh` built DXVK and vkd3d-shader 32-bit only. The real Direct3D 9 path already runs on 64 bits in the Windows x64 client, and DXVK native needs only SDL2's headers at build time, which `scripts/linux_x64_deps.sh` unpacks for the x64 client anyway.

## Decision

`COD2_LINUX_DXVK` is on by default for the x64 client too. `scripts/build_linux_vk_deps.sh x64` builds the same pinned DXVK and vkd3d-shader 64-bit into `third_party/linux-vk64/`, which the x64 configure reads instead of `third_party/linux-vk/`. `build_check.py`'s `linux-client-x64` builds it on DXVK and skips without those libraries; `-DCOD2_LINUX_DXVK=OFF` still builds it on GL.

## Consequences

- Building the x64 client needs `meson`, `ninja` and the amd64 Vulkan loader (`libvulkan1`) besides `scripts/linux_x64_deps.sh`'s headers.
- No 64-bit Linux client keeps the stand-in shaders unless built with `-DCOD2_LINUX_DXVK=OFF`.
