#!/bin/bash
# Sets up a 32-bit Wine without root, to run cod2_win32.exe on a Debian or
# Ubuntu machine with the i386 architecture enabled (as the Linux client's
# build needs):
#
#     scripts/portable_wine.sh DIR
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
dir=$(realpath -m "${1:?usage: $0 DIR}")
root=$dir/root
mkdir -p "$dir/debs" "$root"
cd "$dir/debs"

# the wine32 package and every i386 package of its dependencies not installed
apt-cache depends --recurse --no-recommends --no-suggests --no-conflicts --no-breaks \
    --no-replaces --no-enhances wine32:i386 | grep -v '^ ' | grep -v '^<' | grep ':i386$' | sort -u |
    while read -r pkg; do dpkg -s "$pkg" >/dev/null 2>&1 || echo "$pkg"; done > ../need.txt
echo wine32:i386 >> ../need.txt
echo libz-mingw-w64 >> ../need.txt
apt-get download $(cat ../need.txt)
for deb in *.deb; do dpkg -x "$deb" "$root"; done

# Debian's wineserver script and its data directory are at fixed paths: run
# the server from a build tree layout (DIR/fake/server/wineserver), where it
# reads its .nls files from DIR/fake/nls
mkdir -p "$dir/fake/server"
cp "$root/usr/lib/wine/wineserver32" "$dir/fake/server/wineserver"
ln -sfn "$root/usr/share/wine/nls" "$dir/fake/nls"
sed -i "s|^wineserver32=.*|wineserver32=$dir/fake/server/wineserver|" "$root/usr/lib/wine/wineserver"

cat > "$dir/wine.sh" <<EOF
#!/bin/bash
export LD_LIBRARY_PATH=$root/usr/lib/i386-linux-gnu:$root/lib/i386-linux-gnu:$root/usr/lib/i386-linux-gnu/wine/i386-unix\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}
export WINEDLLPATH=$root/usr/lib/i386-linux-gnu/wine/i386-windows:$root/usr/lib/i386-linux-gnu/wine/i386-unix:$root/usr/i686-w64-mingw32/lib
export WINESERVER=$dir/fake/server/wineserver
export WINELOADER=$root/usr/lib/wine/wine
export WINEPREFIX=\${WINEPREFIX:-$dir/prefix}
export WINEDEBUG=\${WINEDEBUG:--all}
exec $root/usr/lib/wine/wine "\$@"
EOF
chmod +x "$dir/wine.sh"

# create the prefix, then give it Windows' zlib, which user32 imports and wine
# does not find on its dll path
"$dir/wine.sh" cmd /c echo prefix ready >/dev/null 2>&1 || true
for sys in system32 syswow64; do
    if [ -d "$dir/prefix/drive_c/windows/$sys" ]; then
        cp "$root/usr/i686-w64-mingw32/lib/zlib1.dll" "$dir/prefix/drive_c/windows/$sys/"
    fi
done
"$dir/wine.sh" cmd /c echo wine works
