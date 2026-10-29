#!/usr/bin/env python3
"""A client places hud elements by their horzAlign and vertAlign.

    python3 scripts/hud_align_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--keep DIR]

Starts a client (cod2_linux) on its own game of mp_toujane with a gametype
that spawns the player looking up at the sky and gives it three hud elements,
40x40 boxes of the white material in pure colors, whose horzAlign and
vertAlign differ (as the killcam's title, center_safearea and top): magenta
at center_safearea/top, cyan at right/bottom, yellow at left/middle. A
hudelem's alignScreen keeps horzAlign in bits 3-5 and vertAlign in bits 0-2;
read the other way round, the killcam's title went to the left edge and the
middle of the screen. Takes a screenshot (TGA) and finds each color's pixels:
exits with 0 when each box is there, its center within TOLERANCE of where
those alignments put it (a 4:3 window: 640x480 virtual units scale alike on
both axes), and the client leaves no crash report. --keep copies the
screenshot and the log to DIR. The client opens a window and needs a CD key
in ~/.cod2/preferences; without one the test is skipped (exit 0).
"""
import argparse, glob, os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from hit_location_test import PREFERENCES
from fx_test import DEFAULT_CLIENT, GAMETYPE, PITCH, SPOT, tga

SECONDS = 180
TOLERANCE = 0.03  # of the screen's width or height
MIN_PIXELS = 200
# name, color, horzAlign, vertAlign, alignX, alignY, x, y, and the box's
# expected center as fractions of the screen (s = height / 480)
BOXES = [
    ('magenta', (1, 0, 1), 'center_safearea', 'top', 'center', 'top', 0, 40, lambda w, h, s: (w / 2, 60 * s)),
    ('cyan', (0, 1, 1), 'right', 'bottom', 'right', 'bottom', -40, -40, lambda w, h, s: (w - 60 * s, h - 60 * s)),
    ('yellow', (1, 1, 0), 'left', 'middle', 'left', 'middle', 40, 0, lambda w, h, s: (60 * s, h / 2)),
]
HUD = '''
	self.hud_test = [];
%s
'''
BOX = '''	e = newClientHudElem(self);
	e.horzAlign = "%s";
	e.vertAlign = "%s";
	e.alignX = "%s";
	e.alignY = "%s";
	e.x = %d;
	e.y = %d;
	e.color = (%d, %d, %d);
	e.alpha = 1;
	e setShader("white", 40, 40);
	self.hud_test[self.hud_test.size] = e;
'''


def gametype():
    boxes = ''.join(BOX % (h, v, ax, ay, x, y, *c) for _, c, h, v, ax, ay, x, y, _ in BOXES)
    text = GAMETYPE % dict(zip(('x', 'y', 'z', 'yaw'), SPOT.split()), pitch=PITCH)
    text = text.replace('level.callbackStartGameType = ::blank;', 'level.callbackStartGameType = ::start;')
    text += '\nstart()\n{\n\tprecacheShader("white");\n}\n'
    # the boxes once the player has spawned
    return text.replace('\tself spawn(', HUD % boxes + '\tself spawn(', 1) if '\tself spawn(' in text else None


def pixels(path, rgb):
    """The (x, y) of the pixels of a TGA of exactly that color (0 or 255 a channel), top row 0."""
    data = open(path, 'rb').read()
    top_down = data[17] & 0x20
    w, h, bpp, px = tga(path)
    want = [255 * c for c in rgb]
    out = []
    for row in range(h):
        base = row * w * bpp
        y = row if top_down else h - 1 - row
        for col in range(w):
            i = base + col * bpp
            b, g, r = px[i], px[i + 1], px[i + 2]
            if all(abs(a - e) <= 8 for a, e in zip((r, g, b), want)):
                out.append((col, y))
    return w, h, out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    ap.add_argument('--keep', help='copy the screenshot and the log here')
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

    gsc = gametype()
    if gsc is None:
        print('FAIL the gametype template has no spawn call')
        return 1
    with tempfile.TemporaryDirectory(prefix='cod2_hud_') as home:
        os.makedirs(os.path.join(home, 'main'))
        with open(os.path.join(home, 'main', 'hud.cfg'), 'w') as f:
            f.write('wait 300\ncg_drawGun 0\nwait 40\nscreenshot hud_align\nwait 40\nquit\n')
        mod = os.path.join(home, 'hudtest', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(gsc)
        log_path = os.path.join(home, 'out.log')
        with open(log_path, 'wb') as log:
            client = subprocess.Popen([binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir),
                                       '+set', 'fs_homepath', home, '+set', 'com_maxfps', '20',
                                       '+set', 'fs_game', 'hudtest', '+set', 'g_gametype', 'dm',
                                       '+map', 'mp_toujane', '+exec', 'hud.cfg'],
                                      cwd=home, stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
            try:
                client.wait(timeout=SECONDS)
            except subprocess.TimeoutExpired:
                pass
            if client.poll() is None:
                client.terminate()
                try:
                    client.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    client.kill()
        shot = next(iter(glob.glob(os.path.join(home, '*', 'screenshots', 'hud_align.tga'))), None)
        check('the screenshot is taken', shot is not None)
        if shot:
            for name, color, horz, vert, _, _, _, _, where in BOXES:
                w, h, found = pixels(shot, color)
                ex, ey = where(w, h, h / 480)
                if len(found) < MIN_PIXELS:
                    check(f'{name} box ({horz}/{vert}) is drawn', False, f'{len(found)} pixels')
                    continue
                cx = sum(p[0] for p in found) / len(found)
                cy = sum(p[1] for p in found) / len(found)
                ok = abs(cx - ex) <= TOLERANCE * w and abs(cy - ey) <= TOLERANCE * h
                check(f'{name} box ({horz}/{vert}) is in place', ok,
                      f'center {cx:.0f},{cy:.0f}, expected {ex:.0f},{ey:.0f} of {w}x{h}')
        text = open(log_path, encoding='latin-1').read()
        check('no crash report', 'CRASH REPORT' not in text and not glob.glob(os.path.join(home, 'cod2_crash_*')))
        if args.keep:
            os.makedirs(args.keep, exist_ok=True)
            for f in ([shot] if shot else []) + [log_path]:
                shutil.copy(f, args.keep)
    print('PASS' if not failures else f'FAIL: {len(failures)} check(s)')
    return 0 if not failures else 1


if __name__ == '__main__':
    sys.exit(main())
