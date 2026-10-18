#!/usr/bin/env python3
"""Runs smoke_test.py against this repo's server and compares its failures with a list.

    python3 scripts/opencod2_smoke.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--update]

smoke_test.py comes unchanged from CoD2rev-Plus, so it also checks features this
server does not have yet. Those failures are listed in
test/smoke_expected_failures_<protocol>.txt. Exit codes: 0 when the failures
match the list, 1 when a check not on the list fails, 2 when a listed check now
passes (the list only shrinks: run again with --update), 3 when smoke_test.py
printed no results. The checks in test/smoke_flaky.txt are timing-dependent
with this server and count neither way.
"""
import argparse, os, re, subprocess, sys

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


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary')
    ap.add_argument('--update', action='store_true', help='rewrite the list with the current failures')
    args = ap.parse_args()

    binary = args.binary or os.path.join(ROOT, DEFAULT_BINARY[args.protocol])
    run = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'smoke_test.py'), args.game_dir,
                          '--binary', binary, '--protocol', str(args.protocol)],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace')
    print(run.stdout, end='')
    results = parse_results(run.stdout)
    if not results:
        print('\nsmoke_test.py printed no results')
        return 3

    path = os.path.join(ROOT, 'test', f'smoke_expected_failures_{args.protocol}.txt')
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
