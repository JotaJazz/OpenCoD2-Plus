#!/usr/bin/env python3
"""Compares struct layouts between this repo's headers and CoD2rev-Plus's.

    python3 scripts/layout_parity.py [--variant linux|linux-10|mingw]
    python3 scripts/layout_parity.py --headers-a A.h --headers-b B.h --types T.txt --flags "-m32"

Each line of the types file (default test/layout_parity_types.txt) is a type
name, optionally followed by ':field1,field2,...' (# comments); a name ending
in '~' compares only the fields, not sizeof. For each side a
C++ program prints sizeof and offsetof of every field; the two results are
compared. Side A is this repo (common_types.h and imports.h inside extern "C"),
side B is CoD2rev-Plus (qcommon.h, script_public.h, server.h and g_shared.h); both use the flags of the
variant's build in build/check-<variant>/. Prints 'ok <type>' or
'DIFF <type>: <field> <A> != <B>', and exits with 0 when nothing differs. The
mingw variant needs wine to run its programs.
"""
import argparse, json, os, shlex, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'src')
PROTOCOL = {'linux': 118, 'linux-10': 115, 'mingw': 118}


def load_types(path):
    types = []
    with open(path) as f:
        for line in f:
            line = line.split('#')[0].strip()
            if line:
                name, _, fields = line.partition(':')
                name = name.strip()
                types.append((name.rstrip('~'), [x.strip() for x in fields.split(',') if x.strip()], name.endswith('~')))
    return types


def program(prologue, types):
    lines = ['#include <stdio.h>', '#include <stddef.h>', prologue, 'int main(void) {']
    for name, fields, _ in types:
        lines.append(f'    printf("{name} sizeof %u\\n", (unsigned)sizeof({name}));')
        for field in fields:
            lines.append(f'    printf("{name} {field} %u\\n", (unsigned)offsetof({name}, {field}));')
    lines += ['    return 0;', '}']
    return '\n'.join(lines) + '\n'


def measure(compiler, flags, source, tmp, tag, runner):
    src = os.path.join(tmp, tag + '.cpp')
    exe = os.path.join(tmp, tag + '.exe')
    with open(src, 'w') as f:
        f.write(source)
    build = subprocess.run([compiler, '-std=gnu++17', '-w'] + flags + [src, '-o', exe], capture_output=True, text=True)
    if build.returncode != 0:
        sys.exit(f'side {tag} does not compile:\n' + build.stderr[-3000:])
    run = subprocess.run(runner + [exe], capture_output=True, text=True)
    result = {}
    for line in run.stdout.splitlines():
        name, field, value = line.split(' ')
        result[(name, field)] = int(value)
    return result


def variant_flags(variant):
    """(compiler, flags) of an engine C++ source in build/check-<variant>, without -c/-o/source."""
    with open(os.path.join(ROOT, 'build', 'check-' + variant, 'compile_commands.json')) as f:
        entry = next(e for e in json.load(f) if e['file'].endswith('.c.cpp'))
    args = shlex.split(entry['command'])
    flags, skip = [], False
    for a in args[1:]:
        if skip:
            skip = False
            continue
        if a == '-o':
            skip = True
            continue
        if a == '-c' or a.endswith('.c.cpp') or a.startswith('-std='):
            continue
        flags.append(a)
    return args[0], flags


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--variant', default='linux', choices=list(PROTOCOL))
    ap.add_argument('--headers-a')
    ap.add_argument('--headers-b')
    ap.add_argument('--types', default=os.path.join(ROOT, 'test', 'layout_parity_types.txt'))
    ap.add_argument('--flags', help='compiler flags for both sides (tests); default: the variant build')
    args = ap.parse_args()

    types = load_types(args.types)
    runner = []
    if args.flags is not None:
        compiler, flags_a = 'g++', shlex.split(args.flags)
        flags_b = flags_a
    else:
        compiler, flags_a = variant_flags(args.variant)
        flags_b = flags_a + [f'-DPROTOCOL_VERSION={PROTOCOL[args.variant]}', '-DNDEBUG']
        if args.variant == 'mingw':
            if not shutil.which('wine'):
                sys.exit('mingw needs wine to run the layout programs')
            runner = ['wine']
    prologue_a = (f'#include "{args.headers_a}"' if args.headers_a else
                  'extern "C" {\n#include "common_types.h"\n#include "imports.h"\n}')
    prologue_b = (f'#include "{args.headers_b}"' if args.headers_b else
                  f'#include "{os.path.join(SRC, "qcommon", "qcommon.h")}"\n'
                  f'#include "{os.path.join(SRC, "script", "script_public.h")}"\n'
                  f'#include "{os.path.join(SRC, "server", "server.h")}"\n'
                  f'#include "{os.path.join(SRC, "game", "g_shared.h")}"')

    with tempfile.TemporaryDirectory(prefix='layout_parity_') as tmp:
        a = measure(compiler, flags_a, program(prologue_a, types), tmp, 'a', runner)
        b = measure(compiler, flags_b, program(prologue_b, types), tmp, 'b', runner)

    failed = 0
    for name, fields, fields_only in types:
        diffs = [f'{field} {a[(name, field)]} != {b[(name, field)]}' for field in ([] if fields_only else ['sizeof']) + fields
                 if a[(name, field)] != b[(name, field)]]
        if diffs:
            failed += 1
            for d in diffs:
                print(f'DIFF {name}: {d}')
        else:
            print(f'ok   {name}')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
