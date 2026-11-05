#!/usr/bin/env python3
"""Builds the MSVC-ABI Windows client from WSL, on a copy of the tree on a Windows drive.

    python3 scripts/msvc_build.py [--src /mnt/e/opencod2-msvc-src] [--variant msvc-client] [cmake -D options ...]

The MSVC tools cannot build from \\\\wsl$, so the tracked and untracked (not
ignored) files are mirrored to --src with rsync; the copy keeps what it holds
besides (its build/ and third_party/SDL2-*/, the SDL2 VC package). Then
cmd.exe runs VS 2019 Build Tools' vcvars32.bat, configures
build/check-<variant> there with cmake/toolchain-msvc-clang.cmake and builds
cod2_win32. The exe, its map and the build log are copied back to
build/check-<variant>/ here. Exits with the build's status.
"""
import argparse, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_SRC = os.environ.get('COD2_MSVC_SRC', '/mnt/e/opencod2-msvc-src')
VS2019 = r'C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools'
VCVARS = VS2019 + r'\VC\Auxiliary\Build\vcvars32.bat'
CMAKE_BIN = VS2019 + r'\Common7\IDE\CommonExtensions\Microsoft\CMake'


def win_path(path):
    return subprocess.run(['wslpath', '-w', path], check=True, capture_output=True, text=True).stdout.strip()


def sync(src):
    files = subprocess.run(['git', 'ls-files', '-co', '--exclude-standard', '-z'], cwd=ROOT, check=True,
                           capture_output=True).stdout
    files = b'\0'.join(f for f in files.split(b'\0') if f and os.path.exists(os.path.join(ROOT, f.decode())))
    os.makedirs(src, exist_ok=True)
    # --checksum: the Windows drive's mtimes are not reliable through drvfs
    subprocess.run(['rsync', '-a', '--checksum', '--from0', '--files-from=-', ROOT + '/', src + '/'],
                   input=files, check=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--src', default=DEFAULT_SRC, help='the copy on a Windows drive (WSL path)')
    ap.add_argument('--variant', default='msvc-client')
    ap.add_argument('defines', nargs=argparse.REMAINDER, help='extra cmake -D options')
    args = ap.parse_args()
    args.defines = [d for d in args.defines if d != '--']

    sync(args.src)
    build = 'build\\check-' + args.variant
    defines = ' '.join(['-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-msvc-clang.cmake', '-DCOD2_WIN32_CLIENT=ON',
                        '-DCOD2_WIN32_REAL_D3D9=ON', '-DCMAKE_BUILD_TYPE=RelWithDebInfo'] + args.defines)
    bat = '\r\n'.join([
        '@echo off',
        f'call "{VCVARS}" >nul || exit /b 1',
        f'set PATH={CMAKE_BIN}\\CMake\\bin;{CMAKE_BIN}\\Ninja;%PATH%',
        f'cd /d {win_path(args.src)}',
        f'cmake -G Ninja -S . -B {build} {defines} || exit /b 1',
        f'cmake --build {build} --target cod2_win32 -- -k 0',
    ]) + '\r\n'
    with open(os.path.join(args.src, 'msvc_build.bat'), 'w', newline='') as f:
        f.write(bat)
    out_dir = os.path.join(ROOT, 'build', 'check-' + args.variant)
    os.makedirs(out_dir, exist_ok=True)
    log = os.path.join(out_dir, 'msvc_build.log')
    with open(log, 'wb') as f:
        run = subprocess.run(['cmd.exe', '/c', 'msvc_build.bat'], cwd=args.src, stdout=f, stderr=subprocess.STDOUT)
    win_build = os.path.join(args.src, 'build', 'check-' + args.variant)
    for name in ('cod2_win32.exe', 'cod2_win32.map', 'cod2_win32.pdb'):
        if os.path.exists(os.path.join(win_build, name)):
            subprocess.run(['cp', os.path.join(win_build, name), out_dir], check=True)
    print(f'{"ok" if run.returncode == 0 else "FAIL"} {args.variant} (log: {os.path.relpath(log, ROOT)})')
    return run.returncode


if __name__ == '__main__':
    sys.exit(main())
