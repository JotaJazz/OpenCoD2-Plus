#!/bin/bash
# Builds the two libraries the Linux client on real Direct3D 9 (cod2_linux_vk,
# -DCOD2_LINUX_DXVK=ON) loads at run time, 32-bit, into third_party/linux-vk:
#
#     scripts/build_linux_vk_deps.sh
#
#   lib/libdxvk_d3d9.so.0     DXVK native (Direct3D 9 over Vulkan, SDL2 window)
#   lib/libvkd3d-shader.so.1  vkd3d-shader (the game's HLSL to Direct3D 9 bytecode)
#   include/vkd3d_shader.h
#
# Pinned: DXVK v2.7.1 (commit below) and vkd3d 1.19 (tarball SHA-256 below).
# Needs gcc with -m32, meson, ninja, and SDL2 and the Vulkan loader for i386
# (libsdl2-dev:i386, libvulkan1:i386). The build-only tools DXVK and vkd3d
# also need (glslang, flex, bison, perl's JSON, the Vulkan and SPIR-V
# headers) are fetched with apt-get download (no root) when missing.
set -e
root=$(cd "$(dirname "$0")/.." && pwd)
out=$root/third_party/linux-vk
work=$out/work
tools=$work/tools
mkdir -p "$out/lib" "$out/include" "$work" "$tools"

DXVK_TAG=v2.7.1
DXVK_COMMIT=c3dd74be6baec53786d4e064a572185b70347a17
VKD3D_VER=1.19
VKD3D_SHA256=034613605baab8ba84674f8d272cf22b5e86bc6bc03fc5728ef9bce07308baa6

for t in gcc meson ninja git curl pkg-config; do
    command -v $t >/dev/null || { echo "missing $t" >&2; exit 1; }
done
for f in /usr/lib/i386-linux-gnu/pkgconfig/sdl2.pc /usr/lib/i386-linux-gnu/libvulkan.so.1; do
    [ -e "$f" ] || { echo "missing $f (install libsdl2-dev:i386 libvulkan1:i386)" >&2; exit 1; }
done

# build-only tools: the system's if present, otherwise extracted into $tools
fetch() {
    (cd "$work" && apt-get download "$@" >/dev/null && for d in "$@"; do dpkg -x "$d"_*.deb "$tools"; done)
}
command -v glslang >/dev/null || command -v glslangValidator >/dev/null || fetch glslang-tools spirv-tools
command -v flex >/dev/null || fetch flex libfl2
command -v bison >/dev/null || fetch bison m4
perl -MJSON -e1 2>/dev/null || fetch libjson-perl
[ -e /usr/include/vulkan/vulkan.h ] || fetch libvulkan-dev
[ -e /usr/include/spirv/unified1/spirv.h ] || fetch spirv-headers
export PATH=$tools/usr/bin:$PATH
export PERL5LIB=$tools/usr/share/perl5
export BISON_PKGDATADIR=$tools/usr/share/bison
[ -x "$tools/usr/bin/m4" ] && export M4=$tools/usr/bin/m4

# DXVK: d3d9 only, SDL2 window system
if [ ! -d "$work/dxvk" ]; then
    git clone -q --depth 1 --branch "$DXVK_TAG" --recurse-submodules --shallow-submodules \
        https://github.com/doitsujin/dxvk.git "$work/dxvk"
fi
[ "$(git -C "$work/dxvk" rev-parse HEAD)" = "$DXVK_COMMIT" ] || { echo "DXVK is not $DXVK_COMMIT" >&2; exit 1; }
if [ ! -d "$work/dxvk-build" ]; then
    CC="gcc -m32" CXX="g++ -m32" PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig \
        meson setup --buildtype release -Denable_d3d8=false -Denable_d3d10=false \
        -Denable_d3d11=false -Denable_dxgi=false -Dnative_sdl2=enabled -Dnative_sdl3=disabled \
        -Dnative_glfw=disabled "$work/dxvk-build" "$work/dxvk" >/dev/null
fi
ninja -C "$work/dxvk-build" >"$work/dxvk-ninja.log" || { tail -20 "$work/dxvk-ninja.log" >&2; exit 1; }
cp -L "$work/dxvk-build/src/d3d9/libdxvk_d3d9.so.0" "$out/lib/"

# vkd3d-shader
tarball=$work/vkd3d-$VKD3D_VER.tar.xz
[ -e "$tarball" ] || curl -sSLo "$tarball" "https://dl.winehq.org/vkd3d/source/vkd3d-$VKD3D_VER.tar.xz"
echo "$VKD3D_SHA256  $tarball" | sha256sum -c --quiet
[ -d "$work/vkd3d-$VKD3D_VER" ] || tar -C "$work" -xf "$tarball"
mkdir -p "$work/vkd3d-build"
if [ ! -e "$work/vkd3d-build/Makefile" ]; then
    (cd "$work/vkd3d-build" && "../vkd3d-$VKD3D_VER/configure" -q CC="gcc -m32" \
        CPPFLAGS="-I$tools/usr/include" VULKAN_LIBS="-l:libvulkan.so.1" SONAME_LIBVULKAN=libvulkan.so.1 \
        --disable-doxygen-doc --disable-tests --without-ncurses >/dev/null)
fi
make -C "$work/vkd3d-build" include/private/vkd3d_version.h >/dev/null
make -C "$work/vkd3d-build" -j"$(nproc)" libvkd3d-shader.la >/dev/null 2>"$work/vkd3d-make.log" || { tail -20 "$work/vkd3d-make.log" >&2; exit 1; }
cp -L "$work/vkd3d-build/.libs/libvkd3d-shader.so.1" "$out/lib/"
cp "$work/vkd3d-$VKD3D_VER/include/vkd3d_shader.h" "$work/vkd3d-$VKD3D_VER/include/vkd3d_types.h" "$out/include/"

for so in "$out/lib/libdxvk_d3d9.so.0" "$out/lib/libvkd3d-shader.so.1"; do
    [ "$(od -An -tu1 -j4 -N1 "$so" | tr -d ' ')" = 1 ] || { echo "$so is not 32-bit" >&2; exit 1; }
done
echo "built $out/lib/libdxvk_d3d9.so.0 and $out/lib/libvkd3d-shader.so.1"
