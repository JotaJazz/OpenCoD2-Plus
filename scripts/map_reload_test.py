#!/usr/bin/env python3
"""Map loads on the dedicated server: map_restart and map, several times over.

    python3 scripts/map_reload_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--loads N] [--log PATH] [--wrap CMD]

Starts the server in LAN mode with a temporary home folder and loads maps
through rcon, alternating map_restart and map between two maps, N times
(8 by default). After each load the server must answer getinfo with the
map it was asked for; at the end its log must hold no crash report and its
game log must be in the home folder. Exits
with 0 when all of that holds.
"""
import argparse, os, shutil, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test

DEFAULT_BINARY = {115: 'build/check-linux-10/cod2_lnxded', 118: 'build/check-linux/cod2_lnxded'}
RCON = 'reloadpw'
MAPS = ('mp_toujane', 'mp_carentan')


def current_map(port):
    reply = smoke_test.query(port, 'getinfo x')
    return None if reply is None else reply[1].get('mapname', '')


def wait_for_map(server, port, want, timeout=90):
    deadline = time.time() + timeout
    while time.time() < deadline and server.poll() is None:
        if current_map(port) == want:
            return True
        time.sleep(0.5)
    return False


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary')
    ap.add_argument('--loads', type=int, default=8)
    ap.add_argument('--log', help='copy the server log here')
    ap.add_argument('--wrap', default='', help='command to run the server under, such as "gdb -batch -x cmds --args"')
    args = ap.parse_args()
    binary = args.binary or os.path.join(ROOT, DEFAULT_BINARY[args.protocol])
    port = smoke_test.free_port()
    failures = []

    def check(name, ok):
        print(f'{"ok  " if ok else "FAIL"} {name}', flush=True)
        if not ok:
            failures.append(name)
        return ok

    with tempfile.TemporaryDirectory(prefix='cod2_reload_') as home:
        log_path = os.path.join(home, 'server.log')
        with open(log_path, 'wb') as log:
            server = subprocess.Popen(
                args.wrap.split() + [binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                 '+set', 'dedicated', '1', '+set', 'net_port', str(port), '+set', 'sv_cracked', '1',
                 '+set', 'rcon_password', RCON, '+map', MAPS[0]],
                stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, cwd=home)
        try:
            if check('load 1: map ' + MAPS[0], wait_for_map(server, port, MAPS[0], 120)):
                mapname = MAPS[0]
                for n in range(2, args.loads + 1):
                    if n % 2 == 0:
                        cmd = 'map_restart'
                    else:
                        mapname = MAPS[(n // 2) % 2]
                        cmd = 'map ' + mapname
                    smoke_test.rcon(port, RCON, cmd)
                    time.sleep(2)
                    if not check(f'load {n}: {cmd}', wait_for_map(server, port, mapname)):
                        break
        finally:
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    server.kill()
        if args.log:
            shutil.copy(log_path, args.log)
        text = open(log_path, encoding='latin-1').read()
        check('no crash report', 'CRASH REPORT' not in text)
        # the game log goes to fs_homepath/main, which the server must create
        check('games_mp.log written', os.path.exists(os.path.join(home, 'main', 'games_mp.log')))
        if failures:
            print('\n--- last lines of the server log ---')
            print('\n'.join(l for l in text.splitlines()[-60:] if not l.startswith(('[ckpt]', '[scan'))))
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
