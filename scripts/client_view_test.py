#!/usr/bin/env python3
"""A client watches a player die and spawn again.

    python3 scripts/client_view_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--server PATH]

Runs the dedicated server with scripts/contact_test.py's mod, joins its target
as a minimal client and then a real client (cod2_linux), which the mod places
in front of the target, facing it. Twice the test has the mod kill the target
(rcon contact_phase kill): the client draws the corpse the server clones and
predicts its own moves meanwhile. The client must still run without a crash
report, and the server must have cloned the body and spawned the target again.
The client opens a window and needs a CD key in ~/.cod2/preferences; without
one the test is skipped (exit 0). Exits with 0 when all of that holds.
"""
import argparse, os, re, subprocess, sys, tempfile, time

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
                                               client_home, '+set', 'r_fullscreen', '0', '+set', 'net_port',
                                               str(port + 1), '+connect', f'127.0.0.1:{port}'],
                                              stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                              cwd=client_home)
                procs.append(client)
                end = time.time() + 120
                while time.time() < end and client.poll() is None and 'contact mover' not in game_log(home):
                    run(1, client)
                check('the client joins', 'contact mover' in game_log(home), f'exit code {client.poll()}')
                run(5, client)  # in the game, in front of the target
                for death in (1, 2):
                    smoke_test.rcon(port, RCON, f'set contact_phase kill')
                    run(4, client)
                    deaths = game_log(home).count('contact killed')
                    spawns = game_log(home).count('contact respawned')
                    check(f'death {death}: the target dies and spawns again', deaths >= death and spawns >= death,
                          f'{deaths} deaths, {spawns} spawns')
                    smoke_test.rcon(port, RCON, 'set contact_phase none')
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
        if failures and 'CRASH REPORT' in text:
            print('\n'.join(l for l in text[text.index('CRASH REPORT'):].splitlines() if l.strip().startswith('#')))
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
