#!/usr/bin/env python3
"""Counts the C++ errors of each C source of a build, compiled the way COD2_CXX_DIRS compiles it.

    python3 scripts/cxx_errors.py <build_dir> [--module PC/qcommon ...] [--max 20] [--jobs N] [--return-type]

Reads <build_dir>/compile_commands.json, and compiles each C file of the
modules (paths relative to src/) as C++ inside extern "C" { #include "x.c" },
with the C++ compiler that matches the C one and the file's own flags. Prints
"<module>: <n> errors" and the first --max errors of each module. Exits with 0
when there are none.

--return-type looks only for non-void functions that can end without a return:
C allows it when the caller ignores the value, in C++ it is undefined and GCC
drops the code after the call. It compiles without -w and with -O1, because the
check needs the flow analysis.
"""
import argparse, collections, concurrent.futures, json, os, re, shlex, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'src')
C_ONLY = ('-Wno-error=implicit-function-declaration', '-Wno-error=implicit-int', '-Wno-error=int-conversion',
          '-Wno-error=incompatible-pointer-types', '-Wno-error=return-mismatch', '-fcommon')


def wrapper_text(source):
    return f'extern "C" {{\n#include "{source}"\n}}\n'


def cxx_compiler(c_compiler):
    base = os.path.basename(c_compiler)
    if base.endswith('gcc'):
        return os.path.join(os.path.dirname(c_compiler), base[:-3] + 'g++')
    if base in ('cc', 'clang'):
        return os.path.join(os.path.dirname(c_compiler), 'c++' if base == 'cc' else 'clang++')
    return c_compiler


def module_of(path):
    rel = os.path.relpath(path, SRC).replace(os.sep, '/')
    parts = rel.split('/')
    return '/'.join(parts[:2]) if parts[0] == 'PC' and len(parts) > 2 else parts[0] if len(parts) > 1 else rel


def real_source(entry):
    """The C file of an entry: the file itself, or the one a cxx_wrap wrapper includes."""
    path = os.path.normpath(os.path.join(entry['directory'], entry['file']))
    if path.endswith('.c.cpp') and os.sep + 'cxx_wrap' + os.sep in path:
        with open(path) as f:
            return os.path.normpath(re.search(r'#include "([^"]+)"', f.read()).group(1)), path
    return path, path


def errors_of(entry, tmpdir, return_type=False):
    args = shlex.split(entry['command']) if 'command' in entry else list(entry['arguments'])
    source, compiled = real_source(entry)
    flags, skip = [], False
    for a in args[1:]:
        if skip:
            skip = False
            continue
        if a == '-o':
            skip = True
            continue
        if a == '-c' or a.startswith('-std=') or a in C_ONLY or os.path.normpath(os.path.join(entry['directory'], a)) == compiled \
                or (return_type and a == '-w'):
            continue
        flags.append(a)
    wrapper = os.path.join(tmpdir, os.path.relpath(source, SRC).replace(os.sep, '_') + '.cpp')
    with open(wrapper, 'w') as f:
        f.write(wrapper_text(source))
    mode = ['-fsyntax-only'] if not return_type else \
        ['-O1', '-Wreturn-type', '-Werror=return-type', '-c', '-o', os.devnull]
    run = subprocess.run([cxx_compiler(args[0]), '-std=gnu++17', '-fmax-errors=0'] + flags + mode + [wrapper],
                         cwd=entry['directory'], capture_output=True, text=True, errors='replace')
    errors = [l for l in run.stderr.splitlines() if ' error: ' in l or l.startswith('error:')]
    if return_type:
        errors = [l for l in errors if 'return-type' in l]
    return source, errors


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('build_dir')
    ap.add_argument('--module', action='append', help='path relative to src/, e.g. PC/qcommon (repeatable)')
    ap.add_argument('--max', type=int, default=20)
    ap.add_argument('--jobs', type=int, default=os.cpu_count() or 4)
    ap.add_argument('--return-type', action='store_true', help='only non-void functions that can end without a return')
    args = ap.parse_args()

    with open(os.path.join(args.build_dir, 'compile_commands.json')) as f:
        entries = [e for e in json.load(f) if e['file'].endswith(('.c', '.c.cpp'))]
    entries = [e for e in entries if not e['file'].endswith('.c.cpp') or 'cxx_wrap' in e['file']]
    wanted = [m.rstrip('/') for m in args.module] if args.module else None
    if wanted:
        def rel(e):
            return os.path.relpath(real_source(e)[0], SRC).replace(os.sep, '/')
        entries = [e for e in entries if any(rel(e).startswith(m + '/') or rel(e) == m for m in wanted)]
    if not entries:
        print('no C sources matched')
        return 2

    by_module = collections.defaultdict(list)
    with tempfile.TemporaryDirectory(prefix='cxx_errors_') as tmpdir, \
            concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        for source, errors in pool.map(lambda e: errors_of(e, tmpdir, args.return_type), entries):
            by_module[module_of(source)].extend(errors)

    total = 0
    for module in sorted(by_module):
        errors = by_module[module]
        total += len(errors)
        print(f'{module}: {len(errors)} errors')
        for line in errors[:args.max]:
            print('    ' + line)
    return 1 if total else 0


if __name__ == '__main__':
    sys.exit(main())
