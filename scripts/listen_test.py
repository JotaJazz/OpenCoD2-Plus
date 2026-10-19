#!/usr/bin/env python3
"""A client hosts a game and another client joins it.

    python3 scripts/listen_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH]

Starts one client with +map on tdm (its local server, CoD2rev-Plus's), waits
until it answers getinfo, starts a second client with +connect, and checks
with rcon that the server holds both: the host on loopback and the guest.
Then g_speed set on the host's command line must survive the game's
registration, and after a map_restart, and after a map command to another
map, with the guest connected, the server must hold both again. Both clients must still run and leave no crash report.
The clients need a CD key in ~/.cod2/preferences (codkey): without one the
test is skipped (exit 0, with a message). They open windows. Exits with 0 when
all of that holds.
"""
import argparse, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test

DEFAULT_BINARY = {115: 'build/check-linux-10/cod2_linux', 118: 'build/check-linux/cod2_linux'}
RCON = 'listenpw'
G_SPEED = '250'  # not the default (190)
PREFERENCES = os.path.expanduser('~/.cod2/preferences')


class Run:
    def __init__(self):
        self.failures = []

    def check(self, name, ok, detail=''):
        print(('ok   ' if ok else 'FAIL ') + name + (f' ({detail})' if detail else ''))
        if not ok:
            self.failures.append(name)


def players(port):
    """The rows of rcon status, one per connected client."""
    return [l for l in smoke_test.rcon(port, RCON, 'status').splitlines() if l.strip()[:1].isdigit()]


def wait_for(cond, timeout, step=2):
    end = time.time() + timeout
    while time.time() < end:
        value = cond()
        if value:
            return value
        time.sleep(step)
    return None


def client(binary, game_dir, home, args, log):
    return subprocess.Popen([binary, '+set', 'fs_basepath', os.path.abspath(game_dir), '+set', 'fs_homepath', home,
                             '+set', 'r_fullscreen', '0'] + args, cwd=home, stdout=log, stderr=subprocess.STDOUT,
                            stdin=subprocess.DEVNULL)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary')
    ap.add_argument('--map', default='mp_toujane')
    ap.add_argument('--second-map', default='mp_carentan')
    args = ap.parse_args()
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_BINARY[args.protocol]))
    port = smoke_test.free_port()
    run = Run()
    procs = []
    with tempfile.TemporaryDirectory(prefix='cod2_host_') as host_home, \
            tempfile.TemporaryDirectory(prefix='cod2_guest_') as guest_home:
        host_log, guest_log = os.path.join(host_home, 'out.log'), os.path.join(guest_home, 'out.log')
        try:
            with open(host_log, 'wb') as log:
                host = client(binary, args.game_dir, host_home,
                              ['+set', 'net_port', str(port), '+set', 'rcon_password', RCON, '+set', 'g_gametype', 'tdm',
                               '+set', 'sv_maxclients', '8', '+set', 'g_speed', G_SPEED, '+map', args.map], log)
            procs.append(host)
            up = wait_for(lambda: host.poll() is None and smoke_test.query(port, 'getinfo x'), 150)
            run.check('the host serves its game', bool(up), f'exit code {host.poll()}' if not up else '')
            if up:
                with open(guest_log, 'wb') as log:
                    guest = client(binary, args.game_dir, guest_home,
                                   ['+set', 'net_port', str(port + 1), '+connect', f'127.0.0.1:{port}'], log)
                procs.append(guest)
                rows = wait_for(lambda: len(players(port)) >= 2 and players(port), 150)
                run.check('the guest joins', bool(rows), f'{len(players(port))} clients' if not rows else '')
                run.check('the host plays on loopback', bool(rows) and any('loopback' in r for r in rows))
                speed = re.search(r'"g_speed" is: ?"(\d+)', smoke_test.rcon(port, RCON, 'g_speed'))
                run.check('g_speed keeps the command line value', speed is not None and speed.group(1) == G_SPEED,
                          speed.group(1) if speed else 'no reply')
                smoke_test.rcon(port, RCON, 'map_restart')
                time.sleep(5)
                rows = wait_for(lambda: len(players(port)) >= 2 and players(port), 120)
                run.check('both are back after map_restart', bool(rows), f'{len(players(port))} clients' if not rows else '')
                smoke_test.rcon(port, RCON, f'map {args.second_map}')
                time.sleep(5)
                rows = wait_for(lambda: len(players(port)) >= 2 and players(port), 150)
                run.check(f'both are back on {args.second_map}', bool(rows),
                          f'{len(players(port))} clients' if not rows else '')
                time.sleep(5)
                run.check('the guest still runs', guest.poll() is None, f'exit code {guest.poll()}')
            run.check('the host still runs', host.poll() is None, f'exit code {host.poll()}')
        finally:
            for p in procs:
                if p.poll() is None:
                    p.terminate()
                    try:
                        p.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        p.kill()
        for name, path in (('host', host_log), ('guest', guest_log)):
            text = open(path, encoding='latin-1').read() if os.path.exists(path) else ''
            run.check(f'no crash report from the {name}', 'CRASH REPORT' not in text)
            if run.failures and text:
                print(f'\n--- last lines of the {name} log ---')
                print('\n'.join(l for l in text.splitlines()[-25:] if not l.startswith(('[', 'FX Error'))))
    print('\n' + ('FAILED: ' + ', '.join(run.failures) if run.failures else 'All checks passed'))
    return 1 if run.failures else 0


if __name__ == '__main__':
    sys.exit(main())
