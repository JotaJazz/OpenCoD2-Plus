#!/usr/bin/env python3
"""Two players on the dedicated server: join, play, leave, come back, get kicked.

    python3 scripts/two_players_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH]

Starts the server in LAN mode on tdm with developer 1, which makes a script
error stop the map, and a temporary home folder, joins two
clients of cod2client.py, has both send commands, disconnects the first one
and joins it again, and kicks the second one with rcon. After every step the
server must still answer getinfo; at the end its log must hold no crash
report or script error. Exits with 0 when all of that holds.
"""
import argparse, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import cod2client, smoke_test

DEFAULT_BINARY = {115: 'build/check-linux-10/cod2_lnxded', 118: 'build/check-linux/cod2_lnxded'}
RCON = 'twopw'


class Run:
    def __init__(self, port, protocol, server):
        self.port, self.protocol, self.server = port, protocol, server
        self.failures = []
        self.server_id = None

    def check(self, name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            self.failures.append(name)
        return ok

    def alive(self, step):
        return self.check(f'server alive after {step}',
                          self.server.poll() is None and smoke_test.query(self.port, 'getinfo x') is not None)

    def join(self, name):
        c = cod2client.Client(self.port)
        try:
            refused = c.connect(self.protocol, name)
            got = refused is None and c.request_gamestate(self.server_id)
        except OSError as e:
            refused, got = f'no reply ({e})', False
        if got:
            c.enter_game(self.server_id)
        self.check(f'{name} joins', bool(got), refused or '')
        return c if got else None

    def steps(self):
        m = re.search(r'"sv_serverid" is: ?"(-?\d+)', smoke_test.rcon(self.port, RCON, 'sv_serverid'))
        self.server_id = int(m.group(1))
        a = self.join('alpha')
        if a:
            a.read_frames(1.0)
        self.alive('alpha joined')
        b = self.join('bravo')
        if b:
            b.read_frames(1.0)
        if a:
            a.read_frames(1.0)
        self.alive('bravo joined')
        for c, name in ((a, 'alpha'), (b, 'bravo')):
            if c:
                c.command(self.server_id, f'userinfo "\\name\\{name}2\\rate\\25000\\snaps\\20"')
        time.sleep(1)
        self.alive('both sent commands')
        if a:
            a.command(self.server_id, 'disconnect')
            a.close()
        time.sleep(2)
        self.alive('alpha left')
        self.join('alpha')
        self.alive('alpha came back')
        smoke_test.rcon(self.port, RCON, 'clientkick 1')
        time.sleep(2)
        self.alive('bravo was kicked')


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary')
    ap.add_argument('--map', default='mp_toujane')
    args = ap.parse_args()
    binary = args.binary or os.path.join(ROOT, DEFAULT_BINARY[args.protocol])
    port = smoke_test.free_port()

    with tempfile.TemporaryDirectory(prefix='cod2_two_') as home:
        log_path = os.path.join(home, 'server.log')
        with open(log_path, 'wb') as log:
            server = subprocess.Popen(
                [binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                 '+set', 'dedicated', '1', '+set', 'developer', '1', '+set', 'net_port', str(port), '+set', 'sv_cracked', '1',
                 '+set', 'rcon_password', RCON, '+set', 'g_gametype', 'tdm', '+map', args.map],
                stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, cwd=home)
        run = Run(port, args.protocol, server)
        try:
            deadline = time.time() + 120
            while time.time() < deadline and server.poll() is None and not smoke_test.query(port, 'getinfo x'):
                time.sleep(1)
            if run.alive('loading the map'):
                run.steps()
        finally:
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    server.kill()
        text = open(log_path, encoding='latin-1').read()
        run.check('no crash report', 'CRASH REPORT' not in text)
        run.check('no script error', 'script runtime error' not in text)
        if run.failures:
            print('\n--- last lines of the server log ---')
            print('\n'.join(l for l in text.splitlines()[-40:] if not l.startswith(('[ckpt]', '[scan'))))
    print('\n' + ('FAILED: ' + ', '.join(run.failures) if run.failures else 'All checks passed'))
    return 1 if run.failures else 0


if __name__ == '__main__':
    sys.exit(main())
