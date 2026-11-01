#!/bin/bash
# Puts DXVK's 32-bit d3d9.dll where the Windows client's r_gfxApi vulkan loads
# it, a dxvk folder next to cod2_win32.exe:
#
#     scripts/fetch_windows_dxvk.sh DIR_WITH_THE_EXE
#
# Pinned: DXVK v2.7.1, the release the Linux client builds from
# (scripts/build_linux_vk_deps.sh).
set -e
version=2.7.1
sha256=d85ce7c79f57ecd765aaa1b9e7007cb875e6fde9f6d331df799bce73d513ce87
out=${1:?usage: $0 DIR_WITH_THE_EXE}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
curl -fsSL -o "$tmp/dxvk.tar.gz" "https://github.com/doitsujin/dxvk/releases/download/v$version/dxvk-$version.tar.gz"
echo "$sha256  $tmp/dxvk.tar.gz" | sha256sum -c --quiet
tar xzf "$tmp/dxvk.tar.gz" -C "$tmp" "dxvk-$version/x32/d3d9.dll"
mkdir -p "$out/dxvk"
cp "$tmp/dxvk-$version/x32/d3d9.dll" "$out/dxvk/d3d9.dll"
