#!/usr/bin/env python3
"""A client leans, crouches, jumps, walks, fires and is hit; its view and view weapon move as 1.3's.

    python3 scripts/view_weapon_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--server PATH]

Runs the dedicated server with a mod that spawns each player with a rifle,
and a real client (cod2_linux) with COD2_TRACE_VIEWWEAPON set and a config
that loops: lean right, lean left, crouch, stand, jump. The client prints the
view weapon's offset from the eye along the view axis each frame; the test
reads it. 1.3's CG_CalculateWeaponPosition (0x4d5970) moves the weapon
sideways while leaning, to the weapon's stance offset while crouched, and
down by landChange * 0.25 at most after a landing (150 ms down, 300 ms back).
The mod also hurts the player every few seconds; with COD2_TRACE_VIEWDAMAGE
the client prints the angles the view adds after each hit. 1.3 times that
kick from cg->damageTime minus ps->deltaTime (0x4cee35): it eases to
v_dmg_pitch and v_dmg_roll in 100 ms and back in 400 and never goes past them.
Walking, the eye bobs sideways along the view's right (COD2_TRACE_VIEWBOB).
Firing, reloading and melee move the camera by the view weapon's tag_camera
bone (0x4cf730, COD2_TRACE_VIEWANIM).
The client opens a window and needs a CD key in ~/.cod2/preferences; without
one the test is skipped (exit 0). Exits with 0 when all of that holds.
"""
import argparse, os, re, statistics, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test
from hit_location_test import PREFERENCES

DEFAULT_CLIENT = {115: 'build/check-linux-10/cod2_linux', 118: 'build/check-linux/cod2_linux'}
DEFAULT_SERVER = {115: 'build/check-linux-10/cod2_lnxded', 118: 'build/check-linux/cod2_lnxded'}
WEAPON = 'kar98k_mp'

GAMETYPE = '''main()
{
	maps\\mp\\gametypes\\_callbacksetup::SetupCallbacks();
	level.callbackStartGameType = ::Callback_StartGameType;
	level.callbackPlayerConnect = ::Callback_PlayerConnect;
	level.callbackPlayerDisconnect = ::Callback_PlayerDisconnect;
	level.callbackPlayerDamage = ::Callback_PlayerDamage;
	level.callbackPlayerKilled = ::Callback_PlayerKilled;
}

Callback_StartGameType()
{
	character\\mp_american_normandy_braeburn::precache();
	precacheItem("%(weapon)s");
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
	spawnpoints = getentarray("mp_dm_spawn", "classname");
	self spawn(spawnpoints[0].origin, spawnpoints[0].angles);
	self character\\mp_american_normandy_braeburn::main();
	self giveWeapon("%(weapon)s");
	self giveMaxAmmo("%(weapon)s");
	self setSpawnWeapon("%(weapon)s");
	logPrint("viewweapon spawned\\n");
	self thread hurt();
}

// a 20 point hit without knockback every 4 seconds, healed at once
hurt()
{
	self endon("disconnect");
	for(;;)
	{
		wait 4;
		self finishPlayerDamage(self, self, 20, 4, "MOD_PISTOL_BULLET", "%(weapon)s", self.origin + (0, 0, 40), (1, 0, 0), "torso_upper", 0);
		self.health = self.maxhealth;
		logPrint("viewweapon hurt\\n");
	}
}

Callback_PlayerDisconnect()
{
}

Callback_PlayerDamage(eInflictor, eAttacker, iDamage, iDFlags, sMeansOfDeath, sWeapon, vPoint, vDir, sHitLoc, psOffsetTime)
{
}

Callback_PlayerKilled(eInflictor, attacker, iDamage, sMeansOfDeath, sWeapon, vDir, sHitLoc, psOffsetTime, deathAnimDuration)
{
}
''' % {'weapon': WEAPON}

# wait counts frames; com_maxfps 100 makes 100 about a second
CONFIG = '''set com_maxfps 100
set vw_loop "+leanright; wait 300; -leanright; wait 150; +leanleft; wait 300; -leanleft; wait 150; gocrouch; wait 400; +gostand; wait 20; -gostand; wait 150; +moveup; wait 20; -moveup; wait 300; +forward; wait 150; -forward; +back; wait 150; -back; wait 100; +attack; wait 5; -attack; wait 200; +reload; wait 5; -reload; wait 400; +melee; wait 5; -melee; wait 200; vstr vw_loop"
set vw_start "wait 1000; vstr vw_loop"
vstr vw_start
'''

LINE = re.compile(r'\[viewweapon\] t=(-?\d+) lean=(\S+) vht=(-?\d+) xy=(\S+) land=(-?\d+) lc=(\S+) ads=(\S+) '
                  r'ofs=(\S+) (\S+) (\S+)')


def game_log(home):
    path = os.path.join(home, 'viewweapon', 'games_mp.log')
    return open(path, encoding='latin-1').read() if os.path.exists(path) else ''


def parse(text):
    """The trace's frames, in order, as dicts."""
    frames = []
    for m in LINE.finditer(text):
        t, lean, vht, xy, land, lc, ads, f, r, u = m.groups()
        frames.append({'t': int(t), 'lean': float(lean), 'vht': int(vht), 'xy': float(xy), 'land': int(land),
                       'lc': float(lc), 'ads': float(ads), 'ofs': (float(f), float(r), float(u))})
    return frames


def still(frame):
    """Not moving, not aiming, not in a landing."""
    return frame['xy'] < 1.0 and frame['ads'] == 0.0 and frame['land'] > 450


def landings(frames):
    """[(reference u, [(land, u)])] for each landing: the frames 0..450 ms after it."""
    out = []
    i = 1
    while i < len(frames):
        if 0 <= frames[i]['land'] < frames[i - 1]['land'] and frames[i]['lc'] != 0.0:
            ref = frames[i - 1]['ofs'][2]
            window = []
            j = i
            while j < len(frames) and 0 <= frames[j]['land'] < 450:
                window.append((frames[j]['land'], frames[j]['ofs'][2], frames[j]['lc']))
                j += 1
            out.append((ref, window))
            i = j
        else:
            i += 1
    return out


KICK = re.compile(r'\[viewdamage\] t=(-?\d+) dmg=(-?\d+) delta=(-?\d+) pitch=(\S+) roll=(\S+) kick=(\S+) (\S+)')


def check_damage_kick(text, check):
    """The view's damage kick never goes past v_dmg_pitch and v_dmg_roll (the
    idle sway adds well under a degree), and a hit is felt."""
    frames = [tuple(float(x) for x in m.groups()) for m in KICK.finditer(text)]
    hits = [f for f in frames if abs(f[3]) >= 1.0 or abs(f[4]) >= 1.0]
    check('the view damage kick is traced', len(hits) > 0, f'{len(frames)} frames, {len(hits)} after a hit')
    over = [f for f in hits if abs(f[5]) > abs(f[3]) + 1.0 or abs(f[6]) > abs(f[4]) + 1.0]
    check('the view damage kick stays within its angles', not over,
          f'{len(over)} frames over, worst {max((abs(f[5]) - abs(f[3]) for f in over), default=0):+.1f} deg')
    felt = [f for f in hits if abs(f[3]) >= 1.0 and abs(f[5]) >= 0.5 * abs(f[3])]
    check('a hit kicks the view', len(felt) > 0, f'{len(felt)} frames')


BOB = re.compile(r'\[viewbob\] hbob=(\S+) fwd=(\S+) right=(\S+)')


def check_view_bob(text, check):
    """1.3 moves the eye by the horizontal bob along the view's right
    (0x4cef6c), not forward."""
    bobs = [tuple(float(x) for x in m.groups()) for m in BOB.finditer(text)]
    moving = [b for b in bobs if abs(b[0]) > 0.1]
    check('the horizontal bob is traced', len(moving) > 0, f'{len(bobs)} frames, {len(moving)} bobbing')
    off = [b for b in moving if abs(abs(b[2]) - abs(b[0])) > 0.01 or abs(b[1]) > 0.01]
    check('the horizontal bob moves the eye sideways', moving and not off,
          f'{len(off)} of {len(moving)} frames not along the right'
          + (f', e.g. hbob {off[0][0]:+.2f} fwd {off[0][1]:+.2f} right {off[0][2]:+.2f}' if off else ''))


ANIM = re.compile(r'\[viewanim\] t=(-?\d+) turn=(\S+) move=(\S+) (\S+) (\S+)')


def check_view_animation(text, check):
    """1.3 moves the camera by the view weapon's tag_camera bone (0x4cf730):
    firing, reloading and melee animate it."""
    frames = [tuple(float(x) for x in m.groups()) for m in ANIM.finditer(text)]
    moved = [f for f in frames if f[1] > 1e-5 or max(abs(f[2]), abs(f[3]), abs(f[4])) > 0.01]
    check('the view weapon\'s animations move the camera', len(moved) > 0,
          f'{len(frames)} frames, {len(moved)} moved'
          + (f', most {max(f[1] for f in moved):.5f} turn' if moved else ''))


def check_jump_speed(frames, check):
    """1.3's xyspeed (0x4cfa44) is the horizontal speed only: a jump in place
    leaves it near 0 in the 300 ms before the landing."""
    worst = []
    for i in range(1, len(frames)):
        if 0 <= frames[i]['land'] < frames[i - 1]['land'] and frames[i]['lc'] != 0.0:
            t = frames[i]['t']
            air = [f['xy'] for f in frames[max(0, i - 200):i] if t - 300 <= f['t'] < t]
            if air:
                worst.append(max(air))
    # the loop jumps standing still only
    check('a jump in place keeps the horizontal speed', worst and all(w < 5.0 for w in worst),
          ', '.join(f'{w:.1f}' for w in worst))


def check_trace(frames, check):
    check('trace', len(frames) >= 200, f'{len(frames)} frames')
    bad = [f for f in frames if any(v != v or abs(v) == float('inf') for v in f['ofs'])]
    check('no nan', not bad, f'{len(bad)} frames')

    level = [f['ofs'][1] for f in frames if still(f) and f['vht'] == 60 and abs(f['lean']) < 0.01]
    right = [f['ofs'][1] for f in frames if still(f) and f['vht'] == 60 and f['lean'] > 0.4]
    left = [f['ofs'][1] for f in frames if still(f) and f['vht'] == 60 and f['lean'] < -0.4]
    if level and right and left:
        base = statistics.mean(level)
        dr = statistics.mean(right) - base
        dl = statistics.mean(left) - base
        # leaning right moves the hand right, against viewaxis[1] (left)
        check('lean moves the weapon sideways', dr < -0.1 and dl > 0.1,
              f'right {dr:+.3f}, left {dl:+.3f}')
    else:
        check('lean moves the weapon sideways', False, f'{len(level)} level, {len(right)} right, {len(left)} left')

    # a second into the stance: the move offset has reached its target
    stand = [f['ofs'] for f in frames if still(f) and f['vht'] == 60 and abs(f['lean']) < 0.01]
    crouch = []
    since = None
    for f in frames:
        if f['vht'] == 40:
            since = f['t'] if since is None else since
            if still(f) and f['t'] - since > 1000 and abs(f['lean']) < 0.01:
                crouch.append(f['ofs'])
        else:
            since = None
    if stand and crouch:
        diff = max(abs(statistics.mean(c[k] for c in crouch) - statistics.mean(s[k] for s in stand)) for k in range(3))
        spread = max(max(c[k] for c in crouch) - min(c[k] for c in crouch) for k in range(3))
        check('crouching moves the weapon to its stance offset', diff > 0.1 and spread < 0.05,
              f'moved {diff:.3f}, spread {spread:.3f}')
    else:
        check('crouching moves the weapon to its stance offset', False, f'{len(stand)} standing, {len(crouch)} crouched')

    dips = []
    for ref, window in landings(frames):
        lc = window[0][2]
        if abs(lc) < 2.0:
            continue
        dip = min(u - ref for _, u, _ in window)
        dips.append((dip, lc))
    if dips:
        ok = all(0.5 * abs(lc) * 0.25 <= -dip <= abs(lc) * 0.25 + 0.1 for dip, lc in dips)
        check('a landing dips the weapon', ok, ', '.join(f'{dip:+.2f} of {lc * 0.25:+.2f}' for dip, lc in dips))
    else:
        check('a landing dips the weapon', False, 'no landing with landChange')


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    ap.add_argument('--server')
    ap.add_argument('--map', default='mp_toujane')
    ap.add_argument('--keep-log', help='copy the client log here')
    ap.add_argument('--seconds', type=int, default=45, help='how long the client loops after it spawns')
    args = ap.parse_args()
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    client_bin = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_CLIENT[args.protocol]))
    server_bin = os.path.abspath(args.server or os.path.join(ROOT, DEFAULT_SERVER[args.protocol]))
    game_dir = os.path.abspath(args.game_dir)
    port = smoke_test.free_port()
    failures = []
    procs = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2_vw_') as home, \
            tempfile.TemporaryDirectory(prefix='cod2_vwc_') as client_home:
        mod = os.path.join(home, 'viewweapon', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(GAMETYPE)
        os.makedirs(os.path.join(client_home, 'main'))
        with open(os.path.join(client_home, 'main', 'vw.cfg'), 'w') as f:
            f.write(CONFIG)
        client_log = os.path.join(client_home, 'out.log')
        try:
            server = subprocess.Popen([server_bin, '+set', 'fs_basepath', game_dir, '+set', 'fs_homepath', home,
                                       '+set', 'fs_game', 'viewweapon', '+set', 'net_port', str(port),
                                       '+set', 'sv_cracked', '1', '+set', 'g_gametype', 'dm',
                                       '+set', 'g_log', 'games_mp.log', '+set', 'g_logSync', '1',
                                       '+set', 'dedicated', '1', '+map', args.map],
                                      stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                      cwd=home)
            procs.append(server)
            up = smoke_test.wait_for(lambda: server.poll() is None and smoke_test.query(port, 'getinfo x'), 120)
            check('the server runs the map', up)
            if up:
                env = dict(os.environ, COD2_TRACE_VIEWWEAPON='1', COD2_TRACE_VIEWDAMAGE='1', COD2_TRACE_VIEWBOB='1', COD2_TRACE_VIEWANIM='1')
                with open(client_log, 'wb') as log:
                    client = subprocess.Popen([client_bin, '+set', 'fs_basepath', game_dir, '+set', 'fs_homepath',
                                               client_home, '+set', 'r_fullscreen', '0', '+set', 'net_port',
                                               str(port + 1), '+connect', f'127.0.0.1:{port}', '+exec', 'vw.cfg'],
                                              stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                              cwd=client_home, env=env)
                procs.append(client)
                end = time.time() + 120
                while time.time() < end and client.poll() is None and 'viewweapon spawned' not in game_log(home):
                    time.sleep(1)
                check('the client spawns', 'viewweapon spawned' in game_log(home), f'exit code {client.poll()}')
                end = time.time() + args.seconds
                while time.time() < end and client.poll() is None:
                    time.sleep(1)
                check('the client still runs', client.poll() is None, f'exit code {client.poll()}')
        finally:
            for p in procs:
                if p.poll() is None:
                    p.terminate()
                    try:
                        p.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        p.kill()
        text = open(client_log, encoding='latin-1').read() if os.path.exists(client_log) else ''
        if args.keep_log:
            with open(args.keep_log, 'w') as f:
                f.write(text)
        check_trace(parse(text), check)
        check_jump_speed(parse(text), check)
        check_damage_kick(text, check)
        check_view_bob(text, check)
        check_view_animation(text, check)
        check('no crash report from the client', 'CRASH REPORT' not in text)
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
