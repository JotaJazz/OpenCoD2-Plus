#!/bin/bash
# The 64-bit Linux client (-DCOD2_X64=ON) needs SDL2's amd64 headers, which a
# host set up for the 32-bit builds (libsdl2-dev:i386) lacks. This downloads
# libsdl2-dev:amd64 without root and unpacks it into third_party/linux-x64,
# where CMakeLists.txt finds it; the client links the system's amd64
# libSDL2-2.0.so.0 (libsdl2-2.0-0:amd64), as it links GL, X11 and curl.
#
#     scripts/linux_x64_deps.sh
set -e
root=$(cd "$(dirname "$0")/.." && pwd)
dest=$root/third_party/linux-x64
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cd "$work"
apt-get download libsdl2-dev:amd64
mkdir -p "$dest"
dpkg -x libsdl2-dev_*_amd64.deb "$dest"
test -f "$dest/usr/include/x86_64-linux-gnu/SDL2/_real_SDL_config.h"
echo "SDL2 amd64 headers in $dest"
