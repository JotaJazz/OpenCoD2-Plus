#!/usr/bin/env python3
"""The Linux client on real Direct3D 9 compiles the game's shaders and draws.

    python3 scripts/shader_compile_test.py /path/to/CoD2 [--binary PATH] [--keep DIR]

Starts cod2_linux_vk (-DCOD2_LINUX_DXVK=ON) on its own game of mp_toujane with
REALD3D9_SHLOG=1, which logs each shader compilation, and VKD3DX_SELFTEST=1,
which compiles a shader that includes a file the game lacks first. A config
file takes a screenshot (TGA) once the map is up, then quits. Exits with 0
when at least MIN_SHADERS distinct shaders compile, none fails but
distortion.hlsl (it wants USE_FLOAT_Z, which the Windows path does not define
either), the self-test fails as it should, the screenshot is not one flat
colour, and the client leaves no crash report. Under WSL Vulkan is llvmpipe,
on the processor: the map takes a while. The client opens a window and needs
a CD key in ~/.cod2/preferences; without one the test is skipped (exit 0).
"""
import argparse, glob, os, re, shutil, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from hit_location_test import PREFERENCES
from fx_test import tga

DEFAULT_CLIENT = 'build/check-linux-vk/cod2_linux_vk'
MIN_SHADERS = 150
ALLOWED_FAILURES = ('distortion.hlsl',)
TIMEOUT = 300


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--binary')
    ap.add_argument('--keep', help='copy the screenshot and the log here')
    args = ap.parse_args()
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_CLIENT))
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2_shader_') as home:
        os.makedirs(os.path.join(home, 'main'))
        with open(os.path.join(home, 'main', 'shot.cfg'), 'w') as f:
            f.write('wait 200\nscreenshot shadertest\nwait 50\nquit\n')
        log_path = os.path.join(home, 'out.log')
        env = dict(os.environ, REALD3D9_SHLOG='1', VKD3DX_SELFTEST='1')
        with open(log_path, 'wb') as log:
            client = subprocess.Popen([binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir),
                                       '+set', 'fs_homepath', home, '+set', 'r_fullscreen', '0',
                                       '+set', 'g_gametype', 'dm', '+map', 'mp_toujane', '+exec', 'shot.cfg'],
                                      stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL, cwd=home,
                                      env=env)
            try:
                client.wait(TIMEOUT)
            except subprocess.TimeoutExpired:
                client.kill()
                client.wait()
        out = open(log_path, errors='replace').read()

        compiled = set(re.findall(r'\[SHLOG\] (\S+)\s+(\S+)\s+hr=\S+ ok', out))
        failed = {name for name, _ in re.findall(r'\[SHLOG\] (\S+)\s+(\S+)\s+hr=\S+ FAIL', out)}
        check(f'at least {MIN_SHADERS} shaders compile', len(compiled) >= MIN_SHADERS, f'{len(compiled)}')
        check('no shader fails but the expected', not (failed - set(ALLOWED_FAILURES)),
              ', '.join(sorted(failed - set(ALLOWED_FAILURES))))
        check('an include the game lacks fails the compilation', '[vkd3dx-selftest] ok' in out)
        shots = glob.glob(os.path.join(home, '*', 'screenshots', 'shadertest*.tga'))
        colours = 0
        if shots:
            width, height, depth, pixels = tga(shots[0])
            colours = len({pixels[i:i + 3] for i in range(0, len(pixels) - depth, depth * 97)})
        check('the map is drawn', colours > 16, f'{colours} colours sampled' if shots else 'no screenshot')
        crashes = glob.glob(os.path.join(home, 'cod2_crash_*.txt'))
        check('no crash report', not crashes)

        # Vulkan has a device DXVK will not take (as an old driver's): a
        # clean renderer error, not DXVK ending the process
        nodev_log = os.path.join(home, 'nodevice.log')
        with open(nodev_log, 'wb') as log:
            nodev = subprocess.run([binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir),
                                    '+set', 'fs_homepath', home], stdout=log, stderr=subprocess.STDOUT,
                                   stdin=subprocess.DEVNULL, cwd=home, timeout=120,
                                   env=dict(os.environ, DXVK_FILTER_DEVICE_NAME='no such device'))
        nodev_out = open(nodev_log, errors='replace').read()
        nodev_crashes = glob.glob(os.path.join(home, 'cod2_crash_*.txt'))
        check('no usable Vulkan device is a clean error',
              nodev.returncode == 0 and "Couldn't initialize Direct3D 9" in nodev_out and not nodev_crashes,
              f'exit {nodev.returncode}, {len(nodev_crashes)} crash report(s)')
        crashes += nodev_crashes
        if args.keep:
            os.makedirs(args.keep, exist_ok=True)
            for path in [log_path] + shots + crashes:
                shutil.copy(path, args.keep)

    print('\nAll checks passed' if not failures else f'\n{len(failures)} check(s) failed')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
