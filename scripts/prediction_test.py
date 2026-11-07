#!/usr/bin/env python3
"""Counts a real client's prediction misses while it moves, jumps and fires.

    python3 scripts/prediction_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--server PATH] [--max-misses N] [--max-anim-misses N] [--moves CFG] [--spot 'X Y Z YAW'] [--delay MS] [--maxfps N] [--wine CMD]

Runs the dedicated server with scripts/contact_test.py's mod and a minimal
client as its target, and a real client (cod2_linux, held at 20 fps) as the
second player, which the mod puts in front of a ledge (--spot, by rcon) and a
config file drives: it mantles onto the ledge, turns away from the target,
walks forward, crouches and goes prone twice leaning right, walks back and to
both sides, crouches, goes prone, stands, jumps
and fires its rifle, fires, reloads and switches to its thompson and back
during the drop, then aims down its sight and back twice. It keeps away from
the target: the client clips against
other players with a box one unit smaller on each side than the server's (as
the original does), so running into one misses by design.

With each snapshot the client predicts its moves again from the server's
playerstate, with its own bgame, and compares the result with what it had
predicted for the same time; with cg_showmiss 1 it prints, as 1.3,
"Prediction miss: <distance>" when they
differ by more than 0.1. Prints the number of misses and their total and
largest distance, and exits with 0 when the client still runs, walked over
300 units (as the target sees it), left no crash report, and the misses are
at most --max-misses (60). With COD2_TRACE_PREDANIM the client also prints
"Prediction anim miss" when the moves predicted again give other legs or
torso animations than it had predicted (with the origin's miss); those must
be at most --max-anim-misses (6). They come from what the client cannot
predict, not from its animation script (scripts/anim_script_test.py
compares that with the server's): the weapon the mod gives at spawn, and the
MOVETYPE condition the landing event reads, which lives outside the
playerstate and is not set back when the moves are predicted again (3 or 4
per run, all with no origin miss). With COD2_TRACE_PREDDIFF=all the client
prints every playerstate field it predicted otherwise than the server; the
sight's fraction (fWeaponPosFrac) must match it (--max-ads-misses, 0), and so must
the weapon's state (weaponTime, weaponDelay, weapon, weaponstate) through a
fast reload, a shot cut by a reload and a switch to its thompson and back
during the drop, which a Plus server plays by its own rules (--max-weapon-misses,
1: the weapon the mod gives at spawn), and its lean going prone while leaning
(--max-lean-misses, 0). --delay MS puts a UDP relay between the
client and the server that holds each packet MS milliseconds each way (a
ping of twice that, as online); --maxfps sets the client's com_maxfps
(default 20). --wine CMD runs a Windows client given with --binary, our
cod2_win32.exe or 1.3's CoD2MP_s.exe as a baseline (copy it with its DLLs
off a Windows drive: Wine cannot map mss32.dll from DrvFs), and reads its
console_mp.log. The client opens a window and needs a CD key in
~/.cod2/preferences; without one the test is skipped (exit 0).
"""
import argparse, heapq, os, re, shutil, socket, subprocess, sys, tempfile, threading, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test
from contact_test import GAMETYPE, game_log
from hit_location_test import PREFERENCES, RCON, join
from listen_test import wine_path

DEFAULT_CLIENT = {115: 'build/check-linux-10/cod2_linux', 118: 'build/check-linux/cod2_linux'}
DEFAULT_SERVER = {115: 'build/check-linux-10/cod2_lnxded', 118: 'build/check-linux/cod2_lnxded'}

# frames at the client's framerate, which the test holds at 20 (com_maxfps):
# 'wait 20' is a second
MOVES = '''wait 300
+forward; wait 10; +gostand; wait 10; -gostand; wait 40; -forward
wait 20
cl_yawspeed 180; +right; wait 20; -right
+forward; wait 60; -forward
+leanright; gocrouch; wait 30; goprone; wait 20; gocrouch; wait 6; goprone; wait 20; -leanright; gostand; wait 40
+back; wait 30; -back
+moveleft; wait 45; -moveleft
+moveright; wait 45; -moveright
+forward; +moveleft; wait 40; -moveleft; -forward
+forward; wait 60; -forward
gocrouch; wait 30
+forward; wait 45; -forward
goprone; wait 45
+forward; wait 45; -forward
gostand; wait 30
+gostand; wait 5; -gostand; wait 30
+forward; +gostand; wait 5; -gostand; wait 30; -forward
+attack; wait 5; -attack; wait 30
+attack; wait 5; -attack; wait 30
+attack; wait 2; -attack; wait 2; +reload; wait 2; -reload; wait 13; weapnext; wait 1; weapnext; wait 60
+moveright; wait 40; -moveright
toggleads; wait 40; toggleads; wait 40
toggleads; wait 40; toggleads; wait 40
wait 30
'''
MOVES_SECONDS = 100
# in mp_toujane, in front of a ledge with a mantle_on brush (x 2403 to 2494,
# y 2233 to 2301, top at 107), facing it: the moves start with a mantle
SPOT = '2450 2200 80 90'
# 12 to 34 in 1.3 and 1.0 when the client's prediction follows the server's
# code; 119 to 153 before
MAX_MISSES = 60
# 3 or 4 per run: the spawn's weapon and the landing's MOVETYPE condition
MAX_ANIM_MISSES = 6


class Relay:
    """A UDP relay on 127.0.0.1 that holds each packet `delay` seconds
    before passing it on: client -> listen port -> server, and back."""

    def __init__(self, server_port, delay):
        self.server = ('127.0.0.1', server_port)
        self.delay = delay
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('127.0.0.1', 0))
        self.port = self.sock.getsockname()[1]
        self.client = None
        self.queue = []
        self.lock = threading.Condition()
        self.running = True
        threading.Thread(target=self._receive, daemon=True).start()
        threading.Thread(target=self._send, daemon=True).start()

    def _receive(self):
        self.sock.settimeout(0.5)
        n = 0
        while self.running:
            try:
                data, addr = self.sock.recvfrom(65536)
            except socket.timeout:
                continue
            except OSError:
                return
            if addr == self.server:
                to = self.client
            else:
                self.client = addr
                to = self.server
            if to is None:
                continue
            with self.lock:
                n += 1
                heapq.heappush(self.queue, (time.monotonic() + self.delay, n, data, to))
                self.lock.notify()

    def _send(self):
        while self.running:
            with self.lock:
                while self.running and not self.queue:
                    self.lock.wait(0.5)
                if not self.queue:
                    continue
                due, _, data, to = self.queue[0]
                wait = due - time.monotonic()
                if wait > 0:
                    self.lock.wait(wait)
                    continue
                heapq.heappop(self.queue)
            try:
                self.sock.sendto(data, to)
            except OSError:
                pass

    def close(self):
        self.running = False
        self.sock.close()


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    ap.add_argument('--server')
    ap.add_argument('--map', default='mp_toujane')
    ap.add_argument('--max-misses', type=int, default=MAX_MISSES, help=f'fail above this many misses (default {MAX_MISSES})')
    ap.add_argument('--max-anim-misses', type=int, default=MAX_ANIM_MISSES,
                    help=f'fail above this many legs/torso animation misses (default {MAX_ANIM_MISSES})')
    ap.add_argument('--max-weapon-misses', type=int, default=1,
                    help='snapshots whose weapon state the client predicted otherwise (COD2_TRACE_PREDDIFF=all); '
                    'one is the weapon the mod gives at spawn')
    ap.add_argument('--max-lean-misses', type=int, default=0,
                    help='snapshots whose lean the client predicted otherwise (COD2_TRACE_PREDDIFF=all)')
    ap.add_argument('--max-ads-misses', type=int, default=0,
                    help='fail above this many predicted sight fractions the server disagrees with (default 0)')
    ap.add_argument('--moves', help='a config file of moves instead of the built-in one')
    ap.add_argument('--spot', default=SPOT, help="'x y z yaw' to put the client at when it joins (the mod's spot phase); "
                    "'' leaves it where it spawns")
    ap.add_argument('--delay', type=int, default=0, help='milliseconds a relay holds each packet, each way (default 0: no relay)')
    ap.add_argument('--maxfps', type=int, default=20, help="the client's com_maxfps (default 20)")
    ap.add_argument('--wine', help='the command that runs a Windows client')
    args = ap.parse_args()
    if args.wine and not args.binary:
        ap.error('--wine needs --binary')
    if args.spot and len(args.spot.split()) != 4:
        ap.error("--spot takes 'x y z yaw'")
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    client_bin = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_CLIENT[args.protocol]))
    server_bin = os.path.abspath(args.server or os.path.join(ROOT, DEFAULT_SERVER[args.protocol]))
    game_dir = os.path.abspath(args.game_dir)
    port = smoke_test.free_port()
    failures = []
    procs = []
    target = None
    relay = None

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2_pred_') as home, \
            tempfile.TemporaryDirectory(prefix='cod2_predc_') as client_home:
        mod = os.path.join(home, 'contact', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(GAMETYPE)
        os.makedirs(os.path.join(client_home, 'main'))
        with open(os.path.join(client_home, 'main', 'moves.cfg'), 'w') as f:
            f.write(open(args.moves).read() if args.moves else MOVES)
        client_log = os.path.join(client_home, 'out.log')
        try:
            server = subprocess.Popen([server_bin, '+set', 'fs_basepath', game_dir, '+set', 'fs_homepath', home,
                                       '+set', 'fs_game', 'contact', '+set', 'net_port', str(port),
                                       '+set', 'sv_cracked', '1', '+set', 'rcon_password', RCON,
                                       '+set', 'g_gametype', 'dm', '+set', 'g_log', 'games_mp.log',
                                       '+set', 'g_logSync', '1', '+set', 'sv_cheats', '1', '+set', 'dedicated', '1', '+map', args.map],
                                      stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                      cwd=home)
            procs.append(server)
            up = smoke_test.wait_for(lambda: server.poll() is None and smoke_test.query(port, 'getinfo x'), 120)
            check('the server runs the map', up)
            if up:
                target, why = join(port, args.protocol, 'contacttarget')
                if target:
                    # the first player in the game is the target: the real
                    # client must be the second, on the target's spawn point
                    end = time.time() + 30
                    while time.time() < end and 'contact target' not in game_log(home):
                        target.read_frames(0.5, False)
                    if 'contact target' not in game_log(home):
                        target.close()
                        target, why = None, 'not in the game'
                check('the target joins', target is not None, why)
            if target:
                connect_port = port
                if args.delay:
                    relay = Relay(port, args.delay / 1000.0)
                    connect_port = relay.port
                command, base, homepath = [client_bin], game_dir, client_home
                if args.wine:
                    # 1.3 prints nowhere but its window: logfile 2 writes console_mp.log
                    command, base, homepath = [args.wine, client_bin], wine_path(game_dir), wine_path(client_home)
                    os.makedirs(os.path.join(client_home, '.cod2'))
                    shutil.copy(PREFERENCES, os.path.join(client_home, '.cod2'))  # our client reads it from its cwd
                    # without a profile 1.3 waits in the menu that makes one
                    os.makedirs(os.path.join(client_home, 'main', 'players', 'test'))
                    with open(os.path.join(client_home, 'main', 'players', 'test', 'config_mp.cfg'), 'w') as f:
                        f.write('seta com_introPlayed 1\n')  # its intro would take the place of the connect
                    with open(os.path.join(client_home, 'main', 'players', 'active.txt'), 'w') as f:
                        f.write('test')
                with open(client_log, 'wb') as log:
                    client = subprocess.Popen(command + ['+set', 'fs_basepath', base, '+set', 'fs_homepath',
                                               homepath, '+set', 'logfile', '2', '+set', 'cg_showmiss', '1', '+set', 'com_maxfps', str(args.maxfps),
                                               '+set', 'net_port', str(port + 1), '+connect', f'127.0.0.1:{connect_port}',
                                               '+exec', 'moves.cfg'],
                                              stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                              cwd=client_home, env=dict(os.environ, COD2_TRACE_PREDANIM='1',
                                                                         COD2_TRACE_PREDDIFF='all'))
                procs.append(client)
                end = time.time() + 120
                while time.time() < end and client.poll() is None and 'contact mover' not in game_log(home):
                    target.read_frames(1, False)
                joined = 'contact mover' in game_log(home)
                check('the client joins', joined, f'exit code {client.poll()}')
                if joined:
                    # the moves take about a minute at WSLg's framerate; the
                    # client writes its log at exit, so it is read after
                    if args.spot:
                        for cvar, value in zip(('contact_x', 'contact_y', 'contact_z', 'contact_yaw'), args.spot.split()):
                            smoke_test.rcon(port, RCON, f'set {cvar} {value}')
                        smoke_test.rcon(port, RCON, 'set contact_phase spot')
                    mover = int(re.search(r'contact mover (\d+)', game_log(home)).group(1))
                    path, last, start = 0.0, None, None
                    end = time.time() + MOVES_SECONDS
                    while time.time() < end and client.poll() is None:
                        for frame in target.read_frames(0.5, False):
                            ent = frame['entities'].get(mover)
                            if ent and 'pos.trBase' in ent:
                                pos = ent['pos.trBase']
                                if last is not None:
                                    step = sum((a - b) ** 2 for a, b in zip(pos, last)) ** 0.5
                                    # the spot phase's teleport is not a move
                                    path += step if step < 100 else 0
                                start = start or pos
                                last = pos
                    check('the client still runs', client.poll() is None, f'exit code {client.poll()}')
                    # the target sees the mover walk (no misses mean nothing
                    # if it did not)
                    where = ' '.join(f'{c:.0f}' for c in start) if start else 'unseen'
                    check('the client moves', path > 300, f'{path:.0f} units from {where}')
        finally:
            if target:
                target.close()
            if relay:
                relay.close()
            for p in procs:
                if p.poll() is None:
                    p.terminate()
                    try:
                        p.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        p.kill()
        if args.wine:
            client_log = os.path.join(client_home, 'main', 'console_mp.log')
        text = open(client_log, encoding='latin-1').read() if os.path.exists(client_log) else ''
        reports = [n for n in os.listdir(client_home) if n.startswith('cod2_crash_')]
        check('no crash report from the client', 'CRASH REPORT' not in text and not reports, ', '.join(reports))
        misses = [float(m) for m in re.findall(r'Prediction miss: ([0-9.]+)', text)]
        print(f'misses: {len(misses)}, total {sum(misses):.1f}, largest {max(misses, default=0):.1f}')
        check(f'at most {args.max_misses} prediction misses', len(misses) <= args.max_misses, f'{len(misses)}')
        anim = re.findall(r'Prediction anim miss: (.*)', text)
        for line in anim[:10]:
            print(f'  anim miss: {line}')
        print(f'anim misses: {len(anim)}')
        check(f'at most {args.max_anim_misses} animation misses', len(anim) <= args.max_anim_misses, f'{len(anim)}')
        # COD2_TRACE_PREDDIFF=all: the sight's fraction (fWeaponPosFrac, +0xdc) the client predicted
        # against the server's, as the aim moves in and out
        # (a server built for x87 truncates a 0.3 s transition to 299 ms where the engine's float gets 300:
        # 0.0006 a frame, below what a sight shows, so a miss is a larger difference)
        ads = [(c, s) for c, s in re.findall(r'^  \+0x00dc client \S+ (\S+) server \S+ (\S+)', text, re.M)
               if abs(float(c) - float(s)) > 0.002]
        print(f'sight misses: {len(ads)}' + ''.join(f'\n  predicted {c}, server {s}' for c, s in ads[:5]))
        check(f'at most {args.max_ads_misses} sight misses', len(ads) <= args.max_ads_misses, f'{len(ads)}')
        # and the weapon's state (weaponTime +0x34, weaponDelay +0x38, weapon +0xd4, weaponstate +0xd8), which
        # firing, reloading and switching away and back during the drop change (Plus rules on a Plus server)
        weapon = [(t, re.findall(r'^  \+0x00(?:34|38|d4|d8) client.*', body, re.M))
                  for t, body in re.findall(r'^PredDiff t=(\d+).*\n((?:  .*\n)*)', text, re.M)]
        weapon = [(t, lines) for t, lines in weapon if lines]
        print(f'weapon misses: {len(weapon)}' + ''.join(f'\n  t={t}: ' + '; '.join(l.strip() for l in lines)
                                                         for t, lines in weapon[:5]))
        check(f'at most {args.max_weapon_misses} weapon misses', len(weapon) <= args.max_weapon_misses, f'{len(weapon)}')
        # and the lean (leanf, +0x4c) going prone leaning, which a Plus server brings back gradually
        lean = re.findall(r'^  \+0x004c client .*', text, re.M)
        print(f'lean misses: {len(lean)}' + ''.join(f'\n  {l.strip()}' for l in lean[:5]))
        check(f'at most {args.max_lean_misses} lean misses', len(lean) <= args.max_lean_misses, f'{len(lean)}')
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
