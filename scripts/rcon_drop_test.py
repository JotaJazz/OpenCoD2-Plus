#!/usr/bin/env python3
"""A drop error raised by an rcon command must not take the server down.

    python3 scripts/rcon_drop_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH]

Starts the server in LAN mode with developer 1 (which registers the "error"
command) and a temporary home folder, sends "rcon error drop", which raises
Com_Error(ERR_DROP) while rcon output is redirected to the client. The
server must stay up, report the error in its log and leave no crash report.
Exits with 0 when all of that holds. With no map running the server answers
no packets, as the original does, so the map cannot be loaded again by rcon.
"""
import argparse, os, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test
from map_reload_test import DEFAULT_BINARY, wait_for_map

RCON = 'droppw'
MAP = 'mp_toujane'


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary')
    args = ap.parse_args()
    binary = args.binary or os.path.join(ROOT, DEFAULT_BINARY[args.protocol])
    port = smoke_test.free_port()
    failures = []

    def check(name, ok):
        print(f'{"ok  " if ok else "FAIL"} {name}', flush=True)
        if not ok:
            failures.append(name)
        return ok

    with tempfile.TemporaryDirectory(prefix='cod2_drop_') as home:
        log_path = os.path.join(home, 'server.log')
        with open(log_path, 'wb') as log:
            server = subprocess.Popen(
                [binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                 '+set', 'dedicated', '1', '+set', 'developer', '1', '+set', 'net_port', str(port),
                 '+set', 'sv_cracked', '1', '+set', 'rcon_password', RCON, '+map', MAP],
                stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, cwd=home)
        try:
            if check('map loads', wait_for_map(server, port, MAP, 120)):
                smoke_test.rcon(port, RCON, 'error drop')
                time.sleep(3)
                check('server alive after the drop error', server.poll() is None)
        finally:
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    server.kill()
        text = open(log_path, encoding='latin-1').read()
        check('the error is reported', 'ERROR: Testing drop error' in text)
        check('no crash report', 'CRASH REPORT' not in text)
        if failures:
            print('\n--- last lines of the server log ---')
            print('\n'.join(l for l in text.splitlines()[-40:] if not l.startswith(('[ckpt]', '[scan'))))
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
