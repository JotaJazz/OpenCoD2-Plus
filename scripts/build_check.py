#!/usr/bin/env python3
"""Configures and builds each variant into build/check-<variant>/ and reports the errors.

    python3 scripts/build_check.py [linux|linux-10|mingw|all ...] [--c [--cxx-dirs "a;b"]]

linux is the default 1.3 build, linux-10 the 1.0 one and mingw the Windows
cross-compile. The engine compiles as C++ unless --c is given; with --c,
--cxx-dirs picks the src/ modules that still compile as C++. Exits with 0 when
every requested variant builds with no errors and, on Linux, no binary needs a
glibc newer than GLIBC_MAX (the floor of the C build: servers run on older
distributions than the build machine).
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GLIBC_MAX = (2, 34)
LINUX_BINARIES = ('cod2_linux', 'cod2_lnxded')

VARIANTS = {
    'linux': [],
    'linux-10': ['-DCOD2_FEATURE_CFLAGS=-DCOD2_PATCH_10'],
    'mingw': ['-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake'],
}


def build(variant, extra):
    build_dir = os.path.join(ROOT, 'build', 'check-' + variant)
    configure = subprocess.run(['cmake', '-S', ROOT, '-B', build_dir] + VARIANTS[variant] + extra,
                               cwd=ROOT, capture_output=True, text=True)
    if configure.returncode != 0:
        return ['cmake configure failed'] + configure.stdout.splitlines()[-10:] + configure.stderr.splitlines()[-10:]
    result = subprocess.run(['cmake', '--build', build_dir, '-j', str(os.cpu_count() or 4), '--', '-k'],
                            cwd=ROOT, capture_output=True, text=True)
    output = result.stdout + result.stderr
    errors = [line for line in output.splitlines() if 'error:' in line or 'undefined reference' in line]
    if result.returncode != 0 and not errors:
        errors = ['build failed'] + output.splitlines()[-10:]
    return errors


def glibc_too_new(build_dir):
    """'binary: GLIBC_x.y' for each Linux binary that needs a glibc newer than GLIBC_MAX."""
    found = []
    for name in LINUX_BINARIES:
        path = os.path.join(build_dir, name)
        if not os.path.exists(path):
            continue
        out = subprocess.run(['objdump', '-T', path], capture_output=True, text=True).stdout
        versions = {tuple(int(x) for x in v.split('.')) for v in re.findall(r'GLIBC_(\d+(?:\.\d+)+)', out)}
        newest = max(versions, default=(0,))
        if newest > GLIBC_MAX:
            found.append(f'{name}: needs GLIBC_{".".join(map(str, newest))}, more than GLIBC_{".".join(map(str, GLIBC_MAX))}')
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('variants', nargs='*', default=['linux'], choices=list(VARIANTS) + ['all'])
    ap.add_argument('--cxx-dirs', help='passed as -DCOD2_CXX_DIRS (empty when omitted)')
    ap.add_argument('--c', action='store_true', help='builds C instead of C++ (-DCOD2_CXX_ALL=OFF)')
    ap.add_argument('--old-script', action='store_true', help='src/PC/script instead of CoD2rev-Plus\'s VM (-DCOD2_PLUS_SCRIPT=OFF)')
    args = ap.parse_args()

    variants = list(VARIANTS) if 'all' in args.variants else args.variants
    extra = ['-DCOD2_CXX_DIRS=' + (args.cxx_dirs or '')]
    extra.append('-DCOD2_CXX_ALL=' + ('OFF' if args.c else 'ON'))
    extra.append('-DCOD2_PLUS_SCRIPT=' + ('OFF' if args.old_script else 'ON'))

    failed = False
    for variant in variants:
        errors = build(variant, extra)
        if not errors and variant.startswith('linux'):
            errors = glibc_too_new(os.path.join(ROOT, 'build', 'check-' + variant))
        if errors:
            failed = True
            print(f'FAIL {variant} ({len(errors)} errores)')
            for line in errors[:20]:
                print('    ' + line)
        else:
            print(f'ok {variant}')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
