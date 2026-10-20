#!/usr/bin/env python3
"""Configures and builds each variant into build/check-<variant>/ and reports the errors.

    python3 scripts/build_check.py [linux|linux-10|mingw|all ...] [--c [--cxx-dirs "a;b"]]

linux is the default 1.3 build, linux-10 the 1.0 one and mingw the Windows
cross-compile. The engine compiles as C++ unless --c is given; with --c,
--cxx-dirs picks the src/ modules that still compile as C++. Exits with 0 when
every requested variant builds with no errors and, on Linux, no binary needs a
glibc newer than GLIBC_MAX (the floor of the C build: servers run on older
distributions than the build machine) and the dedicated server needs none of
the client's libraries (CLIENT_LIBRARIES). On every variant the dedicated
server, and the Linux client, must get no C name both from CoD2rev-Plus and
from the engine (scripts/check_duplicates.py), and no engine object of them may
still define a function their bridge exports (scripts/check_single_impl.py), and
no bridge import may call an engine symbol that is data
(scripts/check_bridge_symbols.py).
On Linux no engine source of the client may name a renamed bgame global without
its eng_ name in effect (scripts/check_bgame_names.py), and the constants the
engine's bgame takes from CoD2rev-Plus's must be current
(scripts/bgame_plus_consts.py --check), and no engine function may be called
with another return or parameter type than its definition's
(scripts/check_c_decls.py), nor a global declared larger than it is defined
(scripts/check_var_sizes.py).
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GLIBC_MAX = (2, 34)
LINUX_BINARIES = ('cod2_linux', 'cod2_lnxded')
CLIENT_LIBRARIES = ('libGL.', 'libX11.', 'libSDL2', 'libcurl.')

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


def client_libraries(build_dir):
    """'cod2_lnxded: needs libX' for each client library the dedicated server links."""
    path = os.path.join(build_dir, 'cod2_lnxded')
    if not os.path.exists(path):
        return []
    out = subprocess.run(['readelf', '-d', path], capture_output=True, text=True).stdout
    needed = re.findall(r'\(NEEDED\)\s+Shared library: \[([^\]]+)\]', out)
    return [f'cod2_lnxded: needs {lib}' for lib in needed if lib.startswith(CLIENT_LIBRARIES)]


def duplicates(variant, build_dir):
    """The C names an executable with CoD2rev-Plus's code gets both from it and
    from the engine, and the exported functions an engine object of it still
    defines: the dedicated server, and the Linux client's local server."""
    nm, prefix = ('i686-w64-mingw32-nm', '_') if variant == 'mingw' else ('nm', '')
    targets = [('cod2_win32_ded', 'server')] if variant == 'mingw' else \
        [('cod2_lnxded', 'server'), ('cod2_linux', 'client_server')]
    errors = []
    for target, module in targets:
        if not os.path.isdir(os.path.join(build_dir, 'CMakeFiles', target + '.dir')):
            continue
        interface = os.path.join(ROOT, 'test', f'{module}_interface.txt')
        for script, extra in (('check_duplicates.py', []),
                              ('check_single_impl.py', ['--prefix', prefix, '--interface', interface])):
            run = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', script), build_dir, target,
                                  '--nm', nm] + extra, capture_output=True, text=True)
            if run.returncode != 0:
                errors += [f'{target}: {line}' for line in run.stdout.splitlines()[:-1]]
        executable = os.path.join(build_dir, target + ('.exe' if variant == 'mingw' else ''))
        if os.path.exists(executable):
            # the imports the target links: the client's server and collision
            # with the VM's core thunks, the dedicated server's own
            bridges = [os.path.join(ROOT, 'src', 'compat', name) for name in
                       (['client_server_imports.cpp', 'script_core_imports.cpp'] if module == 'client_server'
                        else ['server_imports.cpp'])]
            run = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'check_bridge_symbols.py'),
                                  executable] + bridges + ['--nm', nm, '--prefix', prefix],
                                 capture_output=True, text=True)
            if run.returncode != 0:
                errors += [f'{target}: import calls data: {line}' for line in run.stdout.splitlines()[:-1]]
    return errors


def bgame_names(build_dir):
    """The engine sources of the Linux client that name one of the engine's
    bgame globals under its plain name, CoD2rev-Plus's."""
    if not os.path.isdir(os.path.join(build_dir, 'CMakeFiles', 'cod2_linux.dir')):
        return []
    run = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'check_bgame_names.py'), build_dir],
                         capture_output=True, text=True)
    if run.returncode == 0:
        return []
    # the last line is the summary; a crash leaves only stderr
    lines = run.stdout.splitlines()[:-1] or (run.stderr.strip().splitlines() or ['check_bgame_names.py failed'])[-3:]
    return ['cod2_linux: ' + line for line in lines]


def bgame_plus_consts():
    """src/PC/bgame/bg_plus_consts.h against the values of CoD2rev-Plus's
    headers."""
    run = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'bgame_plus_consts.py'), '--check'],
                         capture_output=True, text=True)
    if run.returncode == 0:
        return []
    return (run.stdout.strip().splitlines() or run.stderr.strip().splitlines() or ['bgame_plus_consts.py failed'])[-3:]


def c_decls(build_dir):
    """The engine functions called with another type than they are defined."""
    run = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'check_c_decls.py'), build_dir],
                         capture_output=True, text=True)
    if run.returncode == 0:
        return []
    return run.stdout.splitlines()[:-1] or (run.stderr.strip().splitlines() or ['check_c_decls.py failed'])[-3:]


def var_sizes(build_dir):
    """The engine's globals declared larger than they are defined."""
    run = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'check_var_sizes.py'), build_dir],
                         capture_output=True, text=True)
    if run.returncode == 0:
        return []
    return run.stdout.splitlines()[:-1] or (run.stderr.strip().splitlines() or ['check_var_sizes.py failed'])[-3:]


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
        build_dir = os.path.join(ROOT, 'build', 'check-' + variant)
        if not errors and variant.startswith('linux'):
            errors = glibc_too_new(build_dir) + client_libraries(build_dir)
        if not errors:
            errors = duplicates(variant, build_dir)
        if not errors and variant.startswith('linux'):
            errors = bgame_names(build_dir) + bgame_plus_consts() + c_decls(build_dir) + var_sizes(build_dir)
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
