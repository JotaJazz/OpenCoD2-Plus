#!/usr/bin/env python3
"""The real client signs a Plus ID at connect (docs/architecture/plus-id.md).

    python3 scripts/plusid_client_test.py /path/to/CoD2 [--binary PATH] [--server PATH]

Runs the dedicated server (OpenCoD2-Plus Server's, which verifies the ID) and
connects the client (cod2_linux) to it with a temporary fs_homepath, several
times: the first run makes fs_homepath/plusid.key, one line of 64 lowercase
hex, and the server's rcon plusIdStatus shows the ID computed here from that
file; the console command plusid prints it with the file's path; a second run
keeps the file and the ID; a corrupt key file is left alone and the client
plays without an ID; and so does a client whose userinfo has no room for the
keys. A home without hard links (link() failing with EPERM through an
LD_PRELOAD shim, as on vfat or exFAT) still gets its key by rename. The client opens a window and needs a CD key in ~/.cod2/preferences;
without one the test is skipped (exit 0). Exits with 0 when all of that holds.
"""
import argparse, os, re, stat, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import plusid_test, smoke_test
from hit_location_test import PREFERENCES


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--binary', default=os.path.join(ROOT, 'build/check-linux/cod2_linux'), help='the client')
    ap.add_argument('--server', default=os.path.join(ROOT, 'build/check-linux/cod2_lnxded'))
    ap.add_argument('--map', default='mp_toujane')
    args = ap.parse_args()
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    game_dir = os.path.abspath(args.game_dir)
    port = smoke_test.free_port()
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    def play(home, name, extra=(), env=None):
        """Connects a client with fs_homepath home as name; returns (its ID in
        plusIdStatus or None when it never joined, its console)."""
        log_path = os.path.join(home, f'{name}.log')
        with open(log_path, 'wb') as log:
            client = subprocess.Popen([os.path.abspath(args.binary), '+set', 'fs_basepath', game_dir,
                                       '+set', 'fs_homepath', home, '+set', 'net_port', str(port + 1),
                                       '+set', 'name', name, *extra, '+connect', f'127.0.0.1:{port}'],
                                      stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL, cwd=home,
                                      env=dict(os.environ, COD2_CONSOLE_STDOUT='1', COD2_CRASH_NOPOPUP='1', **(env or {})))
        try:
            smoke_test.wait_for(lambda: client.poll() is not None or name in plusid_test.status(port), 120)
            plus_id = plusid_test.status(port).get(name)
        finally:
            plusid_test.stop(client)
        return plus_id, open(log_path, encoding='latin-1').read()

    def read(path):
        return open(path).read() if os.path.exists(path) else ''

    def key_id(path):
        return plusid_test.plus_id(plusid_test.public_key(bytes.fromhex(open(path).read().strip())))

    with tempfile.TemporaryDirectory(prefix='cod2_plusids_') as server_home, \
            tempfile.TemporaryDirectory(prefix='cod2_plusidc_') as home, \
            tempfile.TemporaryDirectory(prefix='cod2_plusidk_') as corrupt_home, \
            tempfile.TemporaryDirectory(prefix='cod2_plusidl_') as long_home, \
            tempfile.TemporaryDirectory(prefix='cod2_plusidn_') as nolink_home:
        plusid_test.make_home(server_home)
        server_log = os.path.join(server_home, 'out.log')
        server = plusid_test.launch(args.server, game_dir, server_home, port, args.map, server_log)
        try:
            up = smoke_test.wait_for(lambda: server.poll() is None and smoke_test.query(port, 'getinfo x'), 150)
            check('the server runs the map', bool(up))
            if up:
                key = os.path.join(home, 'plusid.key')
                plus_id, console = play(home, 'pidfirst', ['+plusid'])
                text = read(key)
                check('first_run: plusid.key is one line of 64 lowercase hex', re.fullmatch(r'[0-9a-f]{64}\n', text),
                      repr(text[:8]))
                check('first_run: only its owner reads it', text and stat.S_IMODE(os.stat(key).st_mode) == 0o600)
                check('first_run: no temporary file is left', not os.path.exists(key + '.tmp'))
                expected = key_id(key) if text else None
                check('first_run: the server shows the ID of that key', plus_id is not None and plus_id == expected,
                      plus_id)
                check('console: plusid prints the ID and the file',
                      f'Plus ID: {expected} (key: {key}; back it up to keep your playtime)' in console)

                plus_id, _ = play(home, 'pidsecond')
                check('same_id: a second run has the same ID', plus_id == expected, plus_id)
                check('same_id: the file does not change', read(key) == text)

                corrupt = os.path.join(corrupt_home, 'plusid.key')
                with open(corrupt, 'w') as f:
                    f.write('zz')
                plus_id, console = play(corrupt_home, 'pidcorrupt')
                check('corrupt_key: the client joins without an ID', plus_id == '-', plus_id)
                check('corrupt_key: the file stays', read(corrupt) == 'zz')
                check('corrupt_key: the console says so',
                      'plusid: plusid.key is unreadable, playing without a Plus ID' in console)

                # ~890 bytes with the connect keys: the stock ones still fit (900 would not), pk/ps/pa do not
                plus_id, console = play(long_home, 'pidlong', ['+setu', 'pad', 'x' * 700])
                check('long_userinfo: the client joins without an ID', plus_id == '-', plus_id)
                check('long_userinfo: the console says so',
                      'plusid: userinfo too long, connecting without a Plus ID' in console)
                # a shim of the client's word size: byte 4 of the ELF header is 1 for 32-bit
                shim = os.path.join(nolink_home, 'nolink.so')
                bits = '-m32' if open(args.binary, 'rb').read(5)[4] == 1 else '-m64'
                built = subprocess.run(['gcc', bits, '-shared', '-fPIC', '-o', shim, '-x', 'c', '-'], capture_output=True,
                                       input=b'#include <errno.h>\nint link(const char *a, const char *b) { errno = EPERM; return -1; }\n')
                if built.returncode:
                    print('skipped no_hard_links: gcc cannot build the shim')
                else:
                    plus_id, _ = play(nolink_home, 'pidnolink', env={'LD_PRELOAD': shim})
                    key = os.path.join(nolink_home, 'plusid.key')
                    check('no_hard_links: rename makes the key', re.fullmatch(r'[0-9a-f]{64}\n', read(key))
                          and plus_id == key_id(key) and not os.path.exists(key + '.tmp'), plus_id)
                check('the dedicated server makes no key', not os.path.exists(os.path.join(server_home, 'plusid.key')))
            check('the server still runs', server.poll() is None, f'exit code {server.poll()}')
        finally:
            plusid_test.stop(server)
        if failures:
            print('\n--- the server\'s plusid lines ---')
            print('\n'.join(l for l in open(server_log, encoding='latin-1').read().splitlines() if 'plusid' in l))
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
