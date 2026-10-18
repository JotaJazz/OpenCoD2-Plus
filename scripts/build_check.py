#!/usr/bin/env python3
"""Configures and builds each variant into build/check-<variant>/ and reports the errors.

    python3 scripts/build_check.py [linux|linux-10|mingw|all ...] [--cxx-dirs "a;b"] [--cxx-all]

linux is the default 1.3 build, linux-10 the 1.0 one and mingw the Windows
cross-compile. Exits with 0 when every requested variant builds with no errors.
"""
import argparse, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

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


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('variants', nargs='*', default=['linux'], choices=list(VARIANTS) + ['all'])
    ap.add_argument('--cxx-dirs', help='passed as -DCOD2_CXX_DIRS (empty when omitted)')
    ap.add_argument('--cxx-all', action='store_true', help='passes -DCOD2_CXX_ALL=ON')
    args = ap.parse_args()

    variants = list(VARIANTS) if 'all' in args.variants else args.variants
    extra = ['-DCOD2_CXX_DIRS=' + (args.cxx_dirs or '')]
    extra.append('-DCOD2_CXX_ALL=' + ('ON' if args.cxx_all else 'OFF'))

    failed = False
    for variant in variants:
        errors = build(variant, extra)
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
