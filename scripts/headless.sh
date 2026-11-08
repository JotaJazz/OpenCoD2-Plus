#!/bin/bash
# Runs a command with its windows in a virtual KWin (kwin_wayland --virtual)
# instead of the desktop: the clients the runtime tests start draw there, on
# the GPU, and take neither the screen nor the mouse. Exits with the command's
# status.
#
#     scripts/headless.sh python3 scripts/fx_test.py GAME
#
# shortcut: needs KDE's kwin_wayland; another desktop needs its own headless compositor.
# shortcut: the new Xwayland is the first new /tmp/.X11-unix socket, so two runs
# started at the same moment could swap displays; run them one after another.
command -v kwin_wayland >/dev/null || { echo "missing kwin_wayland" >&2; exit 1; }
socket=wayland-cod2-$$
log=$(mktemp)
sockets() { ls /tmp/.X11-unix 2>/dev/null; }
before=$(sockets)
kwin_wayland --virtual --socket "$socket" --xwayland --width 1920 --height 1080 >"$log" 2>&1 &
kwin=$!
trap 'kill $kwin 2>/dev/null; wait $kwin 2>/dev/null; rm -f "$log"' EXIT
display=
for _ in $(seq 50); do
    kill -0 $kwin 2>/dev/null || { echo "kwin_wayland exited:" >&2; cat "$log" >&2; exit 1; }
    new=$(comm -13 <(echo "$before") <(sockets) | head -1)
    [ -n "$new" ] && display=:${new#X} && break
    sleep 0.2
done
[ -n "$display" ] || { echo "kwin_wayland started no Xwayland:" >&2; cat "$log" >&2; exit 1; }
DISPLAY=$display WAYLAND_DISPLAY=$socket "$@"
