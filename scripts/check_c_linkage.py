#!/usr/bin/env python3
"""Lists the C++-mangled symbols defined by the object files of a build.

    python3 scripts/check_c_linkage.py <build_dir> [--nm NM] [--allowlist FILE]

The data blobs, the .S files and the --defsym aliases refer to the engine's
symbols by their C names, so a function or a global compiled as C++ without C
linkage (a _Z... name) breaks them. Exits with 0 when no object defines a
mangled global symbol other than those in the allowlist (one name per line,
or re:<regex>; # comments; default test/c_linkage_allowlist.txt). The objects
of cod2_plus_script and cod2_plus_server (CoD2rev-Plus's C++ script VM and
server, and their bridges) are skipped: their C++ names are meant to be C++.
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MANGLED = re.compile(r'^_{1,2}Z')
CMAKE_PROBE = re.compile(r'CMakeFiles/\d')
CXX_TARGETS = ('cod2_plus_script.dir', 'cod2_plus_server.dir')


def objects(build_dir):
    for dirpath, dirnames, filenames in os.walk(build_dir):
        if CMAKE_PROBE.search(dirpath.replace(os.sep, '/') + '/') or any(t in dirpath for t in CXX_TARGETS):
            continue
        for name in filenames:
            if name.endswith(('.o', '.obj')):
                yield os.path.join(dirpath, name)


def load_allowlist(path):
    """(names, regexes) from an allowlist file."""
    names, regexes = set(), []
    if path and os.path.exists(path):
        with open(path) as f:
            for line in (l.strip() for l in f):
                if not line or line.startswith('#'):
                    continue
                if line.startswith('re:'):
                    regexes.append(re.compile(line[3:]))
                else:
                    names.add(line)
    return names, regexes


def mangled_symbols(nm, path):
    out = subprocess.run([nm, '-g', '--defined-only', path], capture_output=True, text=True).stdout
    return [line.split()[-1] for line in out.splitlines() if line.split() and MANGLED.match(line.split()[-1])]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('build_dir')
    ap.add_argument('--nm', default='nm')
    ap.add_argument('--allowlist', default=os.path.join(ROOT, 'test', 'c_linkage_allowlist.txt'))
    args = ap.parse_args()

    names, regexes = load_allowlist(args.allowlist)
    found = 0
    for path in sorted(objects(args.build_dir)):
        for symbol in mangled_symbols(args.nm, path):
            if symbol not in names and not any(r.search(symbol) for r in regexes):
                print(f'{os.path.relpath(path, args.build_dir)}: {symbol}')
                found += 1
    if found:
        print(f'{found} mangled symbols')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
