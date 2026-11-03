#!/bin/bash
# Sets up a 32-bit Wine without root, to run cod2_win32.exe on a Debian or
# Ubuntu machine with the i386 architecture enabled (as the Linux client's
# build needs); with 64, a 64-bit Wine for the 64-bit client (-DCOD2_X64=ON):
#
#     scripts/portable_wine.sh DIR [64]
#
# It downloads wine32:i386, the i386 libraries of its dependencies the system
# lacks and Windows' zlib (apt-get download, no root), extracts them into
# DIR/root, and writes DIR/wine.sh, which runs a Windows program with that wine
# and a prefix in DIR/prefix:
#
#     DIR/wine.sh build/check-mingw-client/cod2_win32.exe +set fs_basepath 'Z:\path\to\CoD2' ...
#
# The engine prints to its console window on Windows; COD2_CONSOLE_STDOUT=1
# copies that to stdout too. scripts/listen_test.py --host-wine DIR/wine.sh
# hosts its game on the Windows client.
set -e
dir=$(realpath -m "${1:?usage: $0 DIR [64]}")
if [ "$2" = 64 ]; then
    arch=amd64 triplet=x86_64-linux-gnu wine_pkg=wine64 loader=wine64 server=wineserver64
    windows=x86_64-windows unix=x86_64-unix mingw=x86_64-w64-mingw32
else
    arch=i386 triplet=i386-linux-gnu wine_pkg=wine32:i386 loader=wine server=wineserver32
    windows=i386-windows unix=i386-unix mingw=i686-w64-mingw32
fi
root=$dir/root
mkdir -p "$dir/debs" "$root"
cd "$dir/debs"

# the wine package and every package of its architecture among its
# dependencies not installed (apt names amd64 packages without a suffix)
apt-cache depends --recurse --no-recommends --no-suggests --no-conflicts --no-breaks \
    --no-replaces --no-enhances "$wine_pkg" | grep -v '^ ' | grep -v '^<' |
    if [ "$arch" = i386 ]; then grep ':i386$'; else grep -v ':'; fi | sort -u |
    while read -r pkg; do dpkg -s "$pkg" >/dev/null 2>&1 || echo "$pkg"; done > ../need.txt
# always these: wine's own packages may be installed already, but their
# files are needed under DIR
printf '%s\n' "$wine_pkg" "libwine:$arch" libz-mingw-w64 >> ../need.txt
sort -u -o ../need.txt ../need.txt
# one package that fails (an index older than the mirror) does not stop the rest
apt-get download $(cat ../need.txt) || while read -r pkg; do
    ls "${pkg%%:*}"_*.deb >/dev/null 2>&1 || apt-get download "$pkg" || echo "skipped $pkg" >&2
done < ../need.txt
for deb in *.deb; do dpkg -x "$deb" "$root"; done

# Debian's wineserver script and its data directory are at fixed paths: run
# the server from a build tree layout (DIR/fake/server/wineserver), where it
# reads its .nls files from DIR/fake/nls
mkdir -p "$dir/fake/server"
cp "$root/usr/lib/wine/$server" "$dir/fake/server/wineserver"
ln -sfn "$root/usr/share/wine/nls" "$dir/fake/nls"
printf '#!/bin/sh\nexec %q -p0 "$@"\n' "$dir/fake/server/wineserver" > "$root/usr/lib/wine/wineserver"
chmod +x "$root/usr/lib/wine/wineserver"

q() { printf '%q' "$1"; }
{
    echo '#!/bin/bash'
    echo "export LD_LIBRARY_PATH=$(q "$root/usr/lib/$triplet:$root/lib/$triplet:$root/usr/lib/$triplet/wine/$unix")\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
    echo "export WINEDLLPATH=$(q "$root/usr/lib/$triplet/wine/$windows:$root/usr/lib/$triplet/wine/$unix:$root/usr/$mingw/lib")"
    echo "export WINESERVER=$(q "$dir/fake/server/wineserver")"
    echo "export WINELOADER=$(q "$root/usr/lib/wine/$loader")"
    echo "export WINEPREFIX=\${WINEPREFIX:-$(q "$dir/prefix")}"
    echo 'export WINEDEBUG=${WINEDEBUG:--all}'
    echo "exec $(q "$root/usr/lib/wine/$loader") \"\$@\""
} > "$dir/wine.sh"
chmod +x "$dir/wine.sh"

# create the prefix, then give it Windows' zlib, which user32 imports and wine
# does not find on its dll path
"$dir/wine.sh" cmd /c echo prefix ready >/dev/null 2>&1 || true
for sys in system32 syswow64; do
    if [ -d "$dir/prefix/drive_c/windows/$sys" ]; then
        cp "$root/usr/$mingw/lib/zlib1.dll" "$dir/prefix/drive_c/windows/$sys/"
    fi
done
"$dir/wine.sh" cmd /c echo wine works
