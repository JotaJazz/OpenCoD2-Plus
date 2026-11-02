#!/usr/bin/env python3
"""The compass shows objectives, as 1.3's owner draw (0x4c5620).

    python3 scripts/compass_objectives_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--keep DIR]

Starts a client (cod2_linux) on its own game of mp_toujane, in the gametype of
hud_align_test.py, looking up at the sky, four times: no objective, one to the
right of the view, one to the left with a white icon of its own, which has to
show white, and one ahead for a player on a team. What each screenshot (TGA)
differs from the first by in the lower left corner is the icon, and it has to
be on that side of where the screen's compass stands. Exits with 0 when all of
that holds and the client leaves no crash report. --keep copies the screenshots
and the logs to DIR. The client opens a window and needs a CD key in
~/.cod2/preferences; without one the test is skipped (exit 0).
"""
import argparse, glob, os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from hit_location_test import PREFERENCES
from fx_test import DEFAULT_CLIENT, tga
from hud_align_test import gametype

SECONDS = 180
CFG = 'wait 300\ncg_drawGun 0\nwait 40\nscreenshot goal\nwait 40\nquit\n'
# 300 units from fx_test.py's SPOT, whose yaw is 210
OBJECTIVES = {'none': None, 'right': '(2901, 2438, 80)', 'left': '(3201, 1918, 80), "white"', 'ahead': '(2791, 2028, 80)'}
COMPASS = (59, 420)  # its center on a 640x480 screen, top row 0
MIN_PIXELS = 20
MIN_WHITE_PIXELS = 100


def game(gsc, objective, team):
    if team:
        gsc = gsc.replace('self.sessionteam = "none";', f'self.sessionteam = "{team}";')
    if objective:
        gsc = gsc.replace('\tself spawn(', f'\tobjective_add(0, "current", {objective});\n\tself spawn(', 1)
    return gsc


def icon(none, goal):
    """(x, y, white) of the pixels that differ in the lower left corner, top row 0; white counts those at 250 up."""
    w, h, bpp, a = tga(none)
    _, _, _, b = tga(goal)
    bottom_up = not open(none, 'rb').read()[17] & 0x20
    out, white = [], 0
    for row in range(h):
        y = h - 1 - row if bottom_up else row
        if y < h / 2:
            continue
        for x in range(w // 2):
            i = (row * w + x) * bpp
            if sum(abs(a[i + c] - b[i + c]) for c in range(3)) > 48:
                out.append((x, y))
                white += min(b[i + c] for c in range(3)) >= 250
    return out, white


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    ap.add_argument('--keep', help='copy the screenshots and the logs here')
    args = ap.parse_args()
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_CLIENT[args.protocol]))
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2_goal_') as home:
        os.makedirs(os.path.join(home, 'main'))
        with open(os.path.join(home, 'main', 'goal.cfg'), 'w') as f:
            f.write(CFG)
        mod = os.path.join(home, 'hudtest', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        logs, goals = [], {}
        for name, objective in OBJECTIVES.items():
            with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
                f.write(game(gametype(), objective, 'allies' if name == 'ahead' else None))
            logs.append(os.path.join(home, f'{name}.log'))
            with open(logs[-1], 'wb') as log:
                client = subprocess.Popen([binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                                           '+set', 'com_maxfps', '20', '+set', 'in_mouse', '0', '+set', 'fs_game', 'hudtest',
                                           '+set', 'g_gametype', 'dm', '+map', 'mp_toujane', '+exec', 'goal.cfg'],
                                          cwd=home, stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
                try:
                    client.wait(timeout=SECONDS)
                except subprocess.TimeoutExpired:
                    client.terminate()
                    try:
                        client.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        client.kill()
            shot = glob.glob(os.path.join(home, '*', 'screenshots', 'goal.tga'))
            check(f'the screenshot with objective {name} is taken', bool(shot))
            if shot:
                goals[name] = os.path.join(home, f'goal_{name}.tga')
                os.rename(shot[0], goals[name])
        if len(goals) == len(OBJECTIVES):
            for name, axis, sign in (('right', 0, 1), ('left', 0, -1), ('ahead', 1, -1)):
                seen, white = icon(goals['none'], goals[name])
                middle = sum(p[axis] for p in seen) / len(seen) if seen else COMPASS[axis]
                check(f'the objective {name} is on that side of the compass',
                      len(seen) >= MIN_PIXELS and (middle - COMPASS[axis]) * sign > 5,
                      f'{len(seen)} pixels at {middle:.0f}, the compass at {COMPASS[axis]}')
                if name == 'left':
                    check('it has its own icon', white >= MIN_WHITE_PIXELS, f'{white} white pixels')
        text = ''.join(open(l, encoding='latin-1').read() for l in logs)
        check('no crash report', 'CRASH REPORT' not in text and not glob.glob(os.path.join(home, 'cod2_crash_*')))
        if args.keep:
            os.makedirs(args.keep, exist_ok=True)
            for f in list(goals.values()) + logs:
                shutil.copy(f, args.keep)
    print('PASS' if not failures else f'FAIL: {len(failures)} check(s)')
    return 0 if not failures else 1


if __name__ == '__main__':
    sys.exit(main())
