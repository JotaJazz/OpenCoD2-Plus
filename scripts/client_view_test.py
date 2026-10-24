#!/usr/bin/env python3
"""A client is shot, blasted and killed, and watches a player die.

    python3 scripts/client_view_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--server PATH]

Runs the dedicated server with scripts/contact_test.py's mod, joins its target
as a minimal client and then a real client (cod2_linux), which the mod places
in front of the target, facing it. The target shoots the client and throws a
grenade at it, then twice the test has the mod kill the target (rcon
contact_phase kill): the client draws the corpse the server clones and
predicts its own moves meanwhile. Last the mod kills the client
(killmover), which spawns again. The client must still run without a crash
report, and the hits, deaths and spawns must all have happened; the
players' bone controllers it traces (COD2_TRACE_CONTROLLERS) must stay finite
and within 180 degrees.
The client opens a window and needs a CD key in ~/.cod2/preferences; without
one the test is skipped (exit 0). Exits with 0 when all of that holds.
"""
import argparse, math, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test
from contact_test import GAMETYPE, game_log
from hit_location_test import PREFERENCES, RCON, join

DEFAULT_CLIENT = {115: 'build/check-linux-10/cod2_linux', 118: 'build/check-linux/cod2_linux'}
DEFAULT_SERVER = {115: 'build/check-linux-10/cod2_lnxded', 118: 'build/check-linux/cod2_lnxded'}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    ap.add_argument('--server')
    ap.add_argument('--map', default='mp_toujane')
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
    target = None

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    def run(seconds, client=None):
        """Keeps the target sending commands for seconds, or until the client stops."""
        end = time.time() + seconds
        while time.time() < end and (client is None or client.poll() is None):
            target.read_frames(0.5, False)

    with tempfile.TemporaryDirectory(prefix='cod2_view_') as home, \
            tempfile.TemporaryDirectory(prefix='cod2_viewc_') as client_home:
        mod = os.path.join(home, 'contact', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(GAMETYPE)
        client_log = os.path.join(client_home, 'out.log')
        try:
            server = subprocess.Popen([server_bin, '+set', 'fs_basepath', game_dir, '+set', 'fs_homepath', home,
                                       '+set', 'fs_game', 'contact', '+set', 'net_port', str(port),
                                       '+set', 'sv_cracked', '1', '+set', 'rcon_password', RCON,
                                       '+set', 'g_gametype', 'dm', '+set', 'g_log', 'games_mp.log',
                                       '+set', 'g_logSync', '1', '+set', 'dedicated', '1', '+map', args.map],
                                      stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                      cwd=home)
            procs.append(server)
            up = smoke_test.wait_for(lambda: server.poll() is None and smoke_test.query(port, 'getinfo x'), 120)
            check('the server runs the map', up)
            if up:
                target, why = join(port, args.protocol, 'contacttarget')
                check('the target joins', target is not None, why)
            if target:
                with open(client_log, 'wb') as log:
                    client = subprocess.Popen([client_bin, '+set', 'fs_basepath', game_dir, '+set', 'fs_homepath',
                                               client_home, '+set', 'net_port',
                                               str(port + 1), '+connect', f'127.0.0.1:{port}'],
                                              stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                              cwd=client_home, env=dict(os.environ, COD2_TRACE_CONTROLLERS='1'))
                procs.append(client)
                end = time.time() + 120
                while time.time() < end and client.poll() is None and 'contact mover' not in game_log(home):
                    run(1, client)
                check('the client joins', 'contact mover' in game_log(home), f'exit code {client.poll()}')
                run(5, client)  # in the game, in front of the target
                weapon = 0
                for frame in target.read_frames(1, False):
                    weapon = frame['ps'].get('weapon', 0) or weapon
                target.weapon = weapon

                # the target shoots the client, then throws a grenade at it
                smoke_test.rcon(port, RCON, 'set contact_phase face')  # held in the line of fire
                run(1, client)
                hits = game_log(home).count('contact mover hit')
                target.firing = True
                run(3, client)
                target.firing = False
                check('the target shoots the client', game_log(home).count('contact mover hit') > hits,
                      f'{game_log(home).count("contact mover hit") - hits} hits, weapon {weapon}')
                target.pitch = 10.0
                for _ in range(3):  # a grenade can roll out of reach: up to three
                    target.frag_commands = 6
                    run(6, client)
                    if 'contact mover hit MOD_GRENADE' in game_log(home):
                        break
                target.pitch = 0.0
                check('a grenade blasts the client', 'contact mover hit MOD_GRENADE' in game_log(home))
                for death in (1, 2):
                    smoke_test.rcon(port, RCON, 'set contact_phase kill')
                    end = time.time() + 8
                    while time.time() < end and client.poll() is None and \
                            game_log(home).count('contact respawned') < death:
                        run(0.5, client)
                    deaths = game_log(home).count('contact killed')
                    spawns = game_log(home).count('contact respawned')
                    check(f'death {death}: the target dies and spawns again', deaths >= death and spawns >= death,
                          f'{deaths} deaths, {spawns} spawns')
                    smoke_test.rcon(port, RCON, 'set contact_phase none')
                    run(2, client)
                smoke_test.rcon(port, RCON, 'set contact_phase killmover')
                run(5, client)
                check('the client dies and spawns again',
                      'contact mover killed' in game_log(home) and 'contact mover respawned' in game_log(home))
                run(2, client)
                check('the client still runs', client.poll() is None, f'exit code {client.poll()}')
        finally:
            if target:
                target.close()
            for p in procs:
                if p.poll() is None:
                    p.terminate()
                    try:
                        p.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        p.kill()
        text = open(client_log, encoding='latin-1').read() if os.path.exists(client_log) else ''
        check('no crash report from the client', 'CRASH REPORT' not in text)
        # the players' bone controllers (COD2_TRACE_CONTROLLERS, once a second)
        angles = [float(x) for line in re.findall(r'\[controllers\] (.*)', text)
                  for t in re.findall(r'(?:low|up|neck|head)=(\S+) (\S+) (\S+)', line) for x in t]
        lines = len(re.findall(r'\[controllers\]', text))
        check('the players\' controllers are traced', lines >= 5, f'{lines} lines')
        bad = [a for a in angles if not math.isfinite(a) or abs(a) > 180]
        check('their angles stay within 180 degrees', not bad, ', '.join(f'{a:g}' for a in bad[:5]))
        if failures and 'CRASH REPORT' in text:
            print('\n'.join(l for l in text[text.index('CRASH REPORT'):].splitlines() if l.strip().startswith('#')))
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
