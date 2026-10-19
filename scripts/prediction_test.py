#!/usr/bin/env python3
"""Counts a real client's prediction misses while it moves, jumps and fires.

    python3 scripts/prediction_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--server PATH] [--max-misses N]

Runs the dedicated server with scripts/contact_test.py's mod and a minimal
client as its target, and a real client (cod2_linux, cg_showmiss 1) as the
second player, which a config file drives: it walks forward, back and to both
sides, runs into walls, crouches, goes prone, stands, jumps and fires its
rifle. The client predicts each move with its own bgame and checks the
prediction against the server's playerstate in every snapshot; with
cg_showmiss 1 (which the reconstruction reads as dvar_developer) it prints "Prediction miss: <distance>" when they differ by more
than 0.1. Prints the number of misses and their total and largest distance,
and exits with 0 when the client still runs, left no crash report, and the
misses are at most --max-misses. The client opens a window and needs a CD key
in ~/.cod2/preferences; without one the test is skipped (exit 0).
"""
import argparse, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test
from contact_test import GAMETYPE, game_log
from hit_location_test import PREFERENCES, RCON, join

DEFAULT_CLIENT = {115: 'build/check-linux-10/cod2_linux', 118: 'build/check-linux/cod2_linux'}
DEFAULT_SERVER = {115: 'build/check-linux-10/cod2_lnxded', 118: 'build/check-linux/cod2_lnxded'}

# frames at the client's framerate (about 20 under WSLg): 'wait 30' is over a second
MOVES = '''wait 300
+forward; wait 60; -forward
+back; wait 60; -back
+moveleft; wait 45; -moveleft
+moveright; wait 45; -moveright
+forward; +moveleft; wait 60; -moveleft; -forward
+forward; wait 120; -forward
gocrouch; wait 30
+forward; wait 45; -forward
goprone; wait 45
+forward; wait 45; -forward
gostand; wait 30
+gostand; wait 5; -gostand; wait 30
+forward; +gostand; wait 5; -gostand; wait 30; -forward
+attack; wait 5; -attack; wait 30
+attack; wait 5; -attack; wait 30
+back; wait 120; -back
+moveright; wait 60; -moveright
wait 30
'''
MOVES_SECONDS = 100


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    ap.add_argument('--server')
    ap.add_argument('--map', default='mp_toujane')
    ap.add_argument('--max-misses', type=int, default=None, help='fail above this many misses')
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

    with tempfile.TemporaryDirectory(prefix='cod2_pred_') as home, \
            tempfile.TemporaryDirectory(prefix='cod2_predc_') as client_home:
        mod = os.path.join(home, 'contact', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(GAMETYPE)
        os.makedirs(os.path.join(client_home, 'main'))
        with open(os.path.join(client_home, 'main', 'moves.cfg'), 'w') as f:
            f.write(MOVES)
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
                check('the target joins', target is not None, why)
            if target:
                with open(client_log, 'wb') as log:
                    client = subprocess.Popen([client_bin, '+set', 'fs_basepath', game_dir, '+set', 'fs_homepath',
                                               client_home, '+set', 'r_fullscreen', '0', '+set', 'cg_showmiss', '1',
                                               '+set', 'net_port', str(port + 1), '+connect', f'127.0.0.1:{port}',
                                               '+exec', 'moves.cfg'],
                                              stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                              cwd=client_home)
                procs.append(client)
                end = time.time() + 120
                while time.time() < end and client.poll() is None and 'contact mover' not in game_log(home):
                    target.read_frames(1, False)
                joined = 'contact mover' in game_log(home)
                check('the client joins', joined, f'exit code {client.poll()}')
                if joined:
                    # the moves take about a minute at WSLg's framerate; the
                    # client writes its log at exit, so it is read after
                    end = time.time() + MOVES_SECONDS
                    while time.time() < end and client.poll() is None:
                        target.read_frames(0.5, False)
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
        misses = [float(m) for m in re.findall(r'Prediction miss: ([0-9.]+)', text)]
        print(f'misses: {len(misses)}, total {sum(misses):.1f}, largest {max(misses, default=0):.1f}')
        if args.max_misses is not None:
            check(f'at most {args.max_misses} prediction misses', len(misses) <= args.max_misses, f'{len(misses)}')
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
