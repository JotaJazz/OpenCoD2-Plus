#!/usr/bin/env python3
"""Runs smoke_test.py against this repo's server and compares its failures with a list.

    python3 scripts/opencod2_smoke.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--wine WINE] [--update]

smoke_test.py comes unchanged from CoD2rev-Plus, so it also checks features this
server does not have yet. Those failures are listed in
test/smoke_expected_failures_<protocol>.txt. Exit codes: 0 when the failures
match the list, 1 when a check not on the list fails, 2 when a listed check now
passes (the list only shrinks: run again with --update), 3 when smoke_test.py
printed no results. The checks in test/smoke_flaky.txt are timing-dependent
with this server and count neither way.

With --wine, --binary is the Windows server (cod2_win32_ded.exe) and WINE the
command that runs it (scripts/portable_wine.sh). smoke_test.py runs a launcher
that gives wine the paths as Windows sees them; the list is then
test/smoke_expected_failures_wine.txt, since the Windows server prints only to
its console window and a signal does not reach it as one under wine.
net_strictPort, which smoke_test.py checks in the output, is checked here in
its console log instead.
"""
import argparse, os, re, shlex, socket, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RESULT = re.compile(r'^(ok  |FAIL) (?P<name>.+?)( \(.*\))?$')
HEADER = '# Fallos esperados de smoke_test.py con el servidor de opencod2; solo se achica.\n'
DEFAULT_BINARY = {115: 'build/check-linux-10/cod2_lnxded', 118: 'build/check-linux/cod2_lnxded'}


def parse_results(text):
    """{check name: passed} for every ok/FAIL line of smoke_test.py's output."""
    results = {}
    for line in text.splitlines():
        m = RESULT.match(line)
        if m:
            results[m.group('name')] = m.group(1) == 'ok  '
    return results


def load_expected(path):
    """The check names in a list file, one per line; blank lines and # comments are skipped."""
    if not os.path.exists(path):
        return set()
    with open(path) as f:
        return {l.strip() for l in f if l.strip() and not l.strip().startswith('#')}


def compare(results, expected, flaky=frozenset()):
    """(failures not on the list, listed checks that no longer fail or no longer exist), flaky ones aside."""
    failed = {name for name, ok in results.items() if not ok}
    return failed - expected - flaky, expected - failed - flaky


LAUNCHER = r"""#!/bin/bash
# runs {exe} under wine with smoke_test.py's arguments, the paths in
# fs_basepath and fs_homepath as Windows sees them (drive Z: is /)
args=()
prev=
for a in "$@"; do
    if [ "$prev" = fs_basepath ] || [ "$prev" = fs_homepath ]; then
        args+=("Z:$(realpath -m -- "$a" | tr / '\\')")
    else
        args+=("$a")
    fi
    prev=$a
done
# no debugger window for a crash
export WINEDLLOVERRIDES=winedbg.exe=d
exec {wine} {exe} "${{args[@]}}"
"""


def write_launcher(directory, wine, exe):
    path = os.path.join(directory, 'server')
    with open(path, 'w') as f:
        f.write(LAUNCHER.format(wine=shlex.quote(wine), exe=shlex.quote(exe)))
    os.chmod(path, 0o755)
    return path


def strict_port(launcher, game_dir):
    """net_strictPort 1 under wine: on a busy port the server stops with the
    error in its console log instead of taking the next port (it shows the
    error in a message box and waits there, so it is ended here)."""
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as busy, \
            tempfile.TemporaryDirectory(prefix='opencod2_strict_') as home:
        busy.bind(('0.0.0.0', 0))
        port = busy.getsockname()[1]
        log = os.path.join(home, 'main', 'console_mp_server.log')
        server = subprocess.Popen([launcher, '+set', 'fs_basepath', game_dir, '+set', 'fs_homepath', home,
                                   '+set', 'logfile', '2', '+set', 'dedicated', '1',
                                   '+set', 'net_port', str(port), '+set', 'net_strictPort', '1'],
                                  stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        text = ''
        end = time.time() + 60
        while time.time() < end and server.poll() is None and 'net_strictPort' not in text:
            time.sleep(1)
            text = open(log, encoding='latin-1').read() if os.path.exists(log) else ''
        if server.poll() is None:
            server.kill()
            server.wait()
        return 'net_strictPort' in text and f'localhost:{port + 1}' not in text


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary')
    ap.add_argument('--wine', help='the command that runs --binary, the Windows server (a wine wrapper)')
    ap.add_argument('--update', action='store_true', help='rewrite the list with the current failures')
    args = ap.parse_args()

    if args.wine and not args.binary:
        ap.error('--wine runs --binary, the Windows server')
    binary = args.binary or os.path.join(ROOT, DEFAULT_BINARY[args.protocol])
    with tempfile.TemporaryDirectory(prefix='opencod2_wine_') as tmp:
        if args.wine:
            wine = os.path.abspath(args.wine) if os.sep in args.wine else args.wine
            binary = write_launcher(tmp, wine, os.path.abspath(binary))
        run = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'smoke_test.py'), args.game_dir,
                              '--binary', binary, '--protocol', str(args.protocol)],
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace')
        print(run.stdout, end='')
        results = parse_results(run.stdout)
        if not results:
            print('\nsmoke_test.py printed no results')
            return 3
        if args.wine:
            name = 'net_strictPort stops on a busy port, in its console log'
            results[name] = strict_port(binary, os.path.abspath(args.game_dir))
            print(f'{"ok  " if results[name] else "FAIL"} {name}')

    path = os.path.join(ROOT, 'test', f'smoke_expected_failures_{"wine" if args.wine else args.protocol}.txt')
    if args.update:
        with open(path, 'w') as f:
            flaky = load_expected(os.path.join(ROOT, 'test', 'smoke_flaky.txt'))
            f.write(HEADER + ''.join(name + '\n' for name in sorted(n for n, ok in results.items() if not ok and n not in flaky)))
        print(f'\nwrote {os.path.relpath(path, ROOT)}')
        return 0

    new, fixed = compare(results, load_expected(path), load_expected(os.path.join(ROOT, 'test', 'smoke_flaky.txt')))
    if new:
        print('\nnew failures: ' + ', '.join(sorted(new)))
        return 1
    if fixed:
        print('\nno longer failing (run with --update): ' + ', '.join(sorted(fixed)))
        return 2
    print('\nfailures match ' + os.path.relpath(path, ROOT))
    return 0


if __name__ == '__main__':
    sys.exit(main())
