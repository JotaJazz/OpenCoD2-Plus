#!/usr/bin/env python3
"""Lists the engine symbols a bridge import calls that are not code.

    python3 scripts/check_bridge_symbols.py <executable> <bridge.cpp>... [--nm NM] [--prefix _]

Each import of a generated bridge (src/compat/*_imports.cpp) calls the engine's
C symbol through an extern "C" declaration with an __asm__ label. When the
engine has no such function but a variable of that name (LittleLong, a function
pointer in q_shared.c, whose blob defines the data symbol), the call links and
jumps into data. For each label, the executable's symbol must be code (T, t or
W). Exits with 0 when all are.
"""
import argparse, re, subprocess, sys

LABEL = re.compile(r'__asm__\(COD2_SYM\("(\w+)"\)\)')


def labels(paths):
    """The labels of the import declarations (cod2c_*), whatever their
    parameters (function pointers and arrays have parentheses too)."""
    names = set()
    for path in paths:
        with open(path) as f:
            for line in f:
                if 'cod2c_' in line:
                    names.update(LABEL.findall(line))
    return names


def symbol_types(nm, executable):
    """{name: type letters} of the executable's defined symbols."""
    out = subprocess.run([nm, '--defined-only', executable], capture_output=True, text=True, check=True).stdout
    types = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3:
            types.setdefault(parts[2], set()).add(parts[1])
    return types


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('executable')
    ap.add_argument('bridges', nargs='+')
    ap.add_argument('--nm', default='nm')
    ap.add_argument('--prefix', default='', help="the platform's prefix on C names ('_' on MinGW)")
    args = ap.parse_args()

    types = symbol_types(args.nm, args.executable)
    bad = []
    for name in sorted(labels(args.bridges)):
        found = types.get(args.prefix + name, set())
        if found and not found & {'T', 't', 'W'}:
            bad.append(f'{name}: {"".join(sorted(found))}')
    for line in bad:
        print(line)
    print(f'{len(bad)} imports call data')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
