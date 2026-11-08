# 0017. DXVK is the default Linux client

- Status: Superseded by 0018
- Date: 2026-11-07

## Context

[0009](0009-linux-client-on-dxvk.md) added a second Linux client, `cod2_linux_vk`, on DXVK native and vkd3d-shader, with the game's own shaders; `cod2_linux` kept the GL device and its stand-in shaders (no sun, a white world). Making the DXVK client the default waited on testing it natively with a GPU, which was done on Kubuntu 26.04 with an RTX 5080 on NVIDIA's driver: the runtime suite passes with it, but for `anim_script_test.py` and two heartbeat checks of the 1.0 smoke test, which fail the same with the GL client and a GL-built server.

## Decision

`cod2_linux` is the DXVK client: `COD2_LINUX_DXVK` is on by default, and configuring without `scripts/build_linux_vk_deps.sh`'s libraries stops with a message rather than falling back to GL. `-DCOD2_LINUX_DXVK=OFF` builds the same `cod2_linux` on the GL device; the build directory, not the name, tells them apart. The x64 client stays on GL by default, since DXVK's libraries are 32-bit. `build_check.py`'s `linux`, `linux-10` and `linux-13` build the DXVK client, `linux-gl` the GL one, and the runtime tests run the DXVK client.

## Consequences

- Building the Linux client needs `meson`, `ninja` and `libvulkan1:i386` for the dependency script, and running it needs a Vulkan device; without a GPU (WSL) that is llvmpipe, slow but working.
- A dedicated-server-only build also needs the libraries, or `-DCOD2_LINUX_DXVK=OFF`.
- The GL device stays, for machines without Vulkan and as the base of a future Mac client.
- [0011](0011-graphics-apis-behind-direct3d-9.md)'s `cod2_linux_vk` is now `cod2_linux`.
