#!/bin/bash
# Runs a command with its windows in a virtual KWin (kwin_wayland --virtual)
# instead of the desktop: the clients the runtime tests start draw there, on
# the GPU, and take neither the screen nor the mouse.
#
#     scripts/headless.sh python3 scripts/fx_test.py GAME
#
# shortcut: needs KDE's kwin_wayland; another desktop needs its own headless compositor.
set -e
command -v kwin_wayland >/dev/null || { echo "missing kwin_wayland" >&2; exit 1; }
socket=wayland-cod2-$$
before=$(ls /tmp/.X11-unix)
kwin_wayland --virtual --socket "$socket" --xwayland --width 1920 --height 1080 >/dev/null 2>&1 &
kwin=$!
trap 'kill $kwin 2>/dev/null; wait $kwin 2>/dev/null' EXIT
display=
for _ in $(seq 50); do
    new=$(comm -13 <(echo "$before") <(ls /tmp/.X11-unix) | head -1)
    [ -n "$new" ] && display=:${new#X} && break
    sleep 0.2
done
[ -n "$display" ] || { echo "kwin_wayland started no Xwayland" >&2; exit 1; }
DISPLAY=$display WAYLAND_DISPLAY=$socket "$@"
