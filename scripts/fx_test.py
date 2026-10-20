#!/usr/bin/env python3
"""A client draws effects: each one changes the picture where it plays.

    python3 scripts/fx_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--effects FX,...] [--spot 'X Y Z YAW'] [--keep DIR]

Starts a client (cod2_linux) on its own game of mp_toujane with a gametype
that spawns the player at a fixed spot as it connects, looking up at the sky,
hides the HUD and the gun, and takes screenshots (TGA) by a config file. For
each effect it takes two shots without it, 12 frames apart, to measure how
much the picture changes by itself (the last effect's smoke drifting by), then
one 12 frames after fxTest started playing the effect again and again in
front of the player (fxSetTestPosition). Counts the pixels of a shot that
differ from the one before it by more than DIFF in some channel. Exits with 0
when every effect changes over MIN_PIXELS pixels more than the picture does
by itself, every effect loads (an effect that fails plays the placeholder
instead), and the client still runs and leaves no crash report. --keep copies
the screenshots, the log and any crash report to DIR. The client opens a window and needs a CD
key in ~/.cod2/preferences; without one the test is skipped (exit 0).
"""
import argparse, glob, os, shutil, struct, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from hit_location_test import PREFERENCES

DEFAULT_CLIENT = {115: 'build/check-linux-10/cod2_linux', 118: 'build/check-linux/cod2_linux'}
# a single particle (the placeholder sprite), 50 units at its largest,
# rising; the dust of a bullet's impact (a decal, delayed particles and
# runners); a grenade's water splash (oriented particles among others); a
# mine's dirt plumes (lines); a vehicle's explosion (smoke, dirt and
# fireballs, most of them delayed, and velocities with random ranges); a
# tank's (emitters flinging models that play effects, and the vehicle's
# explosion through a runner). The explosions go last: their smoke lingers.
EFFECTS = 'fx/misc/missing_fx,fx/impacts/default_hit,fx/explosions/grenadeExp_water,fx/impacts/minefield,fx/explosions/large_vehicle_explosion,fx/explosions/tank_explosion'
# effects the stock effects name and the stock iwds lack (minefield's death
# effects): their load failures are the data's
MISSING = ('fx/impacts/fluff1.efx', 'fx/impacts/dirthit_mortar.efx')
# in mp_toujane, on the spawn point farthest from the map's ambient effects
# (mp_toujane_fx.gsc's wind blown dust), looking up at the sky
SPOT = '3051 2178 80 210'
PITCH = -45
# spawns whoever connects at the spot, facing along its yaw
GAMETYPE = '''main()
{
	maps\\mp\\gametypes\\_callbacksetup::SetupCallbacks();
	level.callbackStartGameType = ::blank;
	level.callbackPlayerConnect = ::Callback_PlayerConnect;
	level.callbackPlayerDisconnect = ::blank;
	level.callbackPlayerDamage = ::blank;
	level.callbackPlayerKilled = ::blank;
}

blank(a, b, c, d, e, f, g, h, i, j)
{
}

Callback_PlayerConnect()
{
	self.sessionteam = "none";
	self.sessionstate = "playing";
	self.spectatorclient = -1;
	self.archivetime = 0;
	self.psoffsettime = 0;
	self.statusicon = "";
	self.maxhealth = 100;
	self.health = self.maxhealth;
	self spawn((%(x)s, %(y)s, %(z)s), (%(pitch)s, %(yaw)s, 0));
}
'''
DIFF = 40
MIN_PIXELS = 500
SECONDS = 420


def tga(path):
    """(width, height, rows of bytes) of an uncompressed 24 or 32 bit TGA."""
    data = open(path, 'rb').read()
    idlen, kind, width, height, bpp = data[0], data[2], *struct.unpack('<HH', data[12:16]), data[16]
    if kind != 2 or bpp not in (24, 32):
        raise ValueError(f'{path}: not an uncompressed truecolor TGA')
    return width, height, bpp // 8, data[18 + idlen:]


def changed(a, b):
    """The pixels of b that differ from a by more than DIFF in some channel."""
    wa, ha, pa, da = tga(a)
    wb, hb, pb, db = tga(b)
    if (wa, ha, pa) != (wb, hb, pb):
        raise ValueError(f'{a} and {b} differ in size')
    n = 0
    for i in range(0, wa * ha * pa, pa):
        if abs(da[i] - db[i]) > DIFF or abs(da[i + 1] - db[i + 1]) > DIFF or abs(da[i + 2] - db[i + 2]) > DIFF:
            n += 1
    return n


def config(effects):
    lines = ['wait 300', 'cg_draw2D 0; cg_drawGun 0', 'wait 60', 'fxSetTestPosition', 'wait 5']
    for n, fx in enumerate(effects):
        # two shots without the effect (the last one's smoke may still drift
        # by), then one after it has played every 0.2 s for a while, so the
        # shot sees it whatever its life; fxTest without a time plays it once
        # more and stops repeating it, then the smoke has 15 seconds to clear
        # (a screenshot is taken a few frames after its command, so the
        # effect starts 6 frames after the second one)
        lines += [f'screenshot fx_{n}_none', 'wait 12', f'screenshot fx_{n}_still', 'wait 6',
                  f'fxTest {fx} 0.2', 'wait 12', f'screenshot fx_{n}', f'fxTest {fx}', 'wait 300']
    return '\n'.join(lines + ['quit']) + '\n'


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    ap.add_argument('--effects', default=EFFECTS, help=f'comma separated (default {EFFECTS})')
    ap.add_argument('--spot', default=SPOT, help="'x y z yaw'")
    ap.add_argument('--keep', help='copy the screenshots and the log here')
    args = ap.parse_args()
    if len(args.spot.split()) != 4:
        ap.error("--spot takes 'x y z yaw'")
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_CLIENT[args.protocol]))
    effects = [e for e in args.effects.split(',') if e]
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2_fx_') as home:
        os.makedirs(os.path.join(home, 'main'))
        with open(os.path.join(home, 'main', 'fx.cfg'), 'w') as f:
            f.write(config(effects))
        mod = os.path.join(home, 'fxtest', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(GAMETYPE % dict(zip(('x', 'y', 'z', 'yaw'), args.spot.split()), pitch=PITCH))
        log_path = os.path.join(home, 'out.log')
        with open(log_path, 'wb') as log:
            client = subprocess.Popen([binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir),
                                       '+set', 'fs_homepath', home, '+set', 'r_fullscreen', '0',
                                       '+set', 'com_maxfps', '20', '+set', 'fs_game', 'fxtest',
                                       '+set', 'g_gametype', 'dm', '+map', 'mp_toujane', '+exec', 'fx.cfg'],
                                      cwd=home, stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
            try:
                client.wait(timeout=SECONDS)
            except subprocess.TimeoutExpired:
                pass
            ran = client.poll() == 0
            if client.poll() is None:
                client.terminate()
                try:
                    client.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    client.kill()
        shots = next((d for d in glob.glob(os.path.join(home, '*', 'screenshots'))), os.path.join(home, 'fxtest', 'screenshots'))
        if args.keep:
            os.makedirs(args.keep, exist_ok=True)
            for f in glob.glob(os.path.join(shots, '*.tga')) + glob.glob(os.path.join(home, 'cod2_crash_*')) + [log_path]:
                shutil.copy(f, args.keep)
        check('the client runs the config to its end', ran, f'exit code {client.poll()}')
        text = open(log_path, encoding='latin-1').read() if os.path.exists(log_path) else ''
        reports = [n for n in os.listdir(home) if n.startswith('cod2_crash_')]
        check('no crash report from the client', 'CRASH REPORT' not in text and not reports, ', '.join(reports))
        failed = [l.strip() for l in text.splitlines() if ('FX Error' in l or 'Effect file load failed' in l)
                  and not any(m in l for m in MISSING)]
        check('every effect loads', not failed, '; '.join(failed[:3]))
        shot = lambda name: os.path.join(shots, f'{name}.tga')
        for n, fx in enumerate(effects):
            names = (f'fx_{n}_none', f'fx_{n}_still', f'fx_{n}')
            if not all(os.path.exists(shot(name)) for name in names):
                check(f'{fx} is drawn', False, 'no screenshot')
                continue
            # how much the picture changes by itself over as many frames
            noise = changed(shot(names[0]), shot(names[1]))
            pixels = changed(shot(names[1]), shot(names[2]))
            check(f'{fx} is drawn', pixels - noise > MIN_PIXELS, f'{pixels} pixels, {noise} without it')
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
