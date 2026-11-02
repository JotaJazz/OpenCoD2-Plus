#!/usr/bin/env python3
"""The client's own HUD and config options work.

    python3 scripts/hud_options_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--keep DIR]

Starts a client (cod2_linux) on its own game of mp_toujane, in the gametype of
hud_align_test.py, looking up at the sky, and takes three screenshots (TGA):
as it starts, with cg_drawCompass 0, and with the compass back at
cg_hudCompassOffsetX and Y. What the first two differ by in the screen's lower
left corner is the compass; the third has to differ from the second OFFSET
away instead of there. A second game with +set com_writeConfig 0 changes a dvar
and leaves no config_mp.cfg, where the first game wrote one. Exits with 0 when
all of that holds and the client leaves no crash report. --keep copies the
screenshots and the logs to DIR. The client opens a window and needs a CD key
in ~/.cod2/preferences; without one the test is skipped (exit 0).
"""
import argparse, glob, os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from hit_location_test import PREFERENCES
from fx_test import DEFAULT_CLIENT, tga
from hud_align_test import gametype

SECONDS = 180
OFFSET = (100, -100)  # 640x480 units
MIN_PIXELS = 200
START = 'wait 300\ncg_drawGun 0\nwait 40\nscreenshot opt_on\nwait 40\ncg_drawCompass 0\nwait 40\nscreenshot opt_off\nwait 40\n'
MOVED = f'cg_drawCompass 1\ncg_hudCompassOffsetX {OFFSET[0]}\ncg_hudCompassOffsetY {OFFSET[1]}\nwait 40\nscreenshot opt_moved\nwait 40\nquit\n'
NO_WRITE = 'wait 100\ncg_hudCompassOffsetX 5\nwait 60\nwriteconfig probe\nwait 5\nquit\n'


def differing(path_a, path_b):
    """The set of (x, y) where two screenshots differ, in file order, and their size. Only compared with each other."""
    w, h, bpp, a = tga(path_a)
    _, _, _, b = tga(path_b)
    out = set()
    for row in range(h):
        for col in range(w):
            i = (row * w + col) * bpp
            if sum(abs(a[i + c] - b[i + c]) for c in range(3)) > 48:
                out.add((col, row))
    return w, h, out


def flipped(path):
    data = open(path, 'rb').read()
    return not data[17] & 0x20


def run(binary, game_dir, home, cfg, extra, log_path):
    with open(os.path.join(home, 'main', 'opt.cfg'), 'w') as f:
        f.write(cfg)
    with open(log_path, 'wb') as log:
        client = subprocess.Popen([binary, '+set', 'fs_basepath', os.path.abspath(game_dir), '+set', 'fs_homepath', home,
                                   '+set', 'com_maxfps', '20', '+set', 'in_mouse', '0', '+set', 'fs_game', 'hudtest', '+set', 'g_gametype', 'dm',
                                   *extra, '+map', 'mp_toujane', '+exec', 'opt.cfg'],
                                  cwd=home, stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
        try:
            client.wait(timeout=SECONDS)
        except subprocess.TimeoutExpired:
            client.terminate()
            try:
                client.wait(timeout=15)
            except subprocess.TimeoutExpired:
                client.kill()


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

    with tempfile.TemporaryDirectory(prefix='cod2_opt_') as home:
        os.makedirs(os.path.join(home, 'main'))
        mod = os.path.join(home, 'hudtest', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(gametype())
        logs = [os.path.join(home, 'first.log'), os.path.join(home, 'second.log')]
        run(binary, args.game_dir, home, START + MOVED, [], logs[0])
        shots = {n: next(iter(glob.glob(os.path.join(home, '*', 'screenshots', f'opt_{n}.tga'))), None)
                 for n in ('on', 'off', 'moved')}
        check('the screenshots are taken', all(shots.values()))
        if all(shots.values()):
            w, h, compass = differing(shots['on'], shots['off'])
            up = flipped(shots['on'])
            corner = {(x, y) for x, y in compass if x < w / 2 and (y < h / 2 if up else y >= h / 2)}
            check('cg_drawCompass 0 takes the compass away', len(corner) >= MIN_PIXELS, f'{len(corner)} pixels')
            _, _, moved = differing(shots['off'], shots['moved'])
            s = h / 480
            dx, dy = round(OFFSET[0] * s), round(OFFSET[1] * s) * (-1 if up else 1)
            there = sum((x + dx, y + dy) in moved for x, y in corner)
            here = sum((x, y) in moved for x, y in corner)
            check('the offsets move it', corner and there >= len(corner) * 0.9, f'{there} of {len(corner)} pixels moved')
            check('it leaves its place', corner and here <= len(corner) / 2, f'{here} of {len(corner)} pixels stayed')
        configs = glob.glob(os.path.join(home, '*', 'players', '*', 'config_mp.cfg'))
        check('a game writes its config', bool(configs))
        for config in configs:
            os.remove(config)
        run(binary, args.game_dir, home, NO_WRITE, ['+set', 'com_writeConfig', '0'], logs[1])
        left = glob.glob(os.path.join(home, '*', 'players', '*', 'config_mp.cfg'))
        check('com_writeConfig 0 writes none', not left, ' '.join(left))
        check('the second game got that far', bool(glob.glob(os.path.join(home, '*', 'probe.cfg')) + glob.glob(os.path.join(home, '*', '*', '*', 'probe.cfg'))))
        text = ''.join(open(l, encoding='latin-1').read() for l in logs)
        check('no crash report', 'CRASH REPORT' not in text and not glob.glob(os.path.join(home, 'cod2_crash_*')))
        if args.keep:
            os.makedirs(args.keep, exist_ok=True)
            for f in [s for s in shots.values() if s] + logs:
                shutil.copy(f, args.keep)
    print('PASS' if not failures else f'FAIL: {len(failures)} check(s)')
    return 0 if not failures else 1


if __name__ == '__main__':
    sys.exit(main())
