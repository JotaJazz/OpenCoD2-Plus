#!/bin/bash
# The 64-bit Windows client (-DCOD2_X64=ON with cmake/toolchain-mingw64.cmake)
# needs MinGW-w64's x86_64 compilers and SDL2's x86_64 static libraries. This
# downloads both without root: Debian's x86_64 MinGW-w64 packages unpacked into
# third_party/mingw64 (the toolchain file uses them when no system compiler
# exists) and SDL2's official MinGW package, the version the 32-bit client
# builds, into third_party/mingw64-sdl2.
#
#     scripts/mingw64_deps.sh
set -e
root=$(cd "$(dirname "$0")/.." && pwd)
sdl2_version=2.30.9
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cd "$work"
apt-get download binutils-mingw-w64-x86-64 gcc-mingw-w64-x86-64-posix gcc-mingw-w64-x86-64-posix-runtime \
    g++-mingw-w64-x86-64-posix mingw-w64-x86-64-dev mingw-w64-common
mkdir -p "$root/third_party/mingw64"
for deb in *.deb; do dpkg -x "$deb" "$root/third_party/mingw64"; done
curl -sSLf -o sdl2.tar.gz \
    "https://github.com/libsdl-org/SDL/releases/download/release-$sdl2_version/SDL2-devel-$sdl2_version-mingw.tar.gz"
mkdir -p "$root/third_party/mingw64-sdl2"
tar xzf sdl2.tar.gz -C "$root/third_party/mingw64-sdl2"
test -x "$root/third_party/mingw64/usr/bin/x86_64-w64-mingw32-g++-posix"
test -f "$root/third_party/mingw64-sdl2/SDL2-$sdl2_version/x86_64-w64-mingw32/lib/libSDL2.a"
echo "MinGW-w64 x86_64 in third_party/mingw64, SDL2 in third_party/mingw64-sdl2"
