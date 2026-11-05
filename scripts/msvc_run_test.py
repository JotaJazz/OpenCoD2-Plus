#!/usr/bin/env python3
"""Runs a Windows client into a local game on Windows (from WSL) and checks it got there.

    python3 scripts/msvc_run_test.py --exe build/check-msvc-client/cod2_win32.exe [--seconds 90]

Copies the exe (with SDL2.dll when the MSVC-ABI build needs it) to
E:\\opencod2-msvc-test\\<build dir name>, starts it with +devmap mp_toujane on
the game data in E:\\Servers\\COD2 and a home of its own, and waits until its
console (consola.txt) shows the map loaded by the local server, the client's
cgame up, the gametype script talking to the client and frames drawn, lets it
play --linger seconds more, then kills it. Exits 1 naming what never showed, or
when the game died or wrote a crash report.
"""
import argparse, glob, os, re, shutil, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEST_ROOT = '/mnt/e/opencod2-msvc-test'
MSVC_SRC = os.environ.get('COD2_MSVC_SRC', '/mnt/e/opencod2-msvc-src')   # as msvc_build.py
GAME = r'E:\Servers\COD2'
MAP = 'mp_toujane'
MARKERS = [
    ('the local server starts the map', f'Server: {MAP}'),
    ('the map loads', f'LOADING... maps/mp/{MAP}.d3dbsp'),
    ("the client's cgame starts", 'CL_InitCGame:'),
    ("the gametype script talks to the client", "script menu 'serverinfo_dm'"),
]
DRAW = re.compile(r'\[rbdiag\] .*\bdraw=([1-9]\d*)')


def missing(log):
    """What a console log does not show yet, in MARKERS order, then frames."""
    out = [what for what, line in MARKERS if line not in log]
    if not DRAW.search(log):
        out.append('frames are drawn')
    return out


def verdict(gone, crashes):
    """What failed: the lines that never showed, then any crash report."""
    return list(gone) + (['no crash report (' + ', '.join(os.path.basename(c) for c in crashes) + ')'] if crashes else [])


def kill_command(win_exe):
    """Ends the copy under test only: the user may be playing another cod2_win32.exe."""
    return ['powershell.exe', '-NoProfile', '-Command',
            f"Get-Process | Where-Object {{ $_.Path -eq '{win_exe}' }} | Stop-Process -Force"]


def win(path):
    return subprocess.run(['wslpath', '-w', path], check=True, capture_output=True, text=True).stdout.strip()


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--exe', required=True)
    ap.add_argument('--seconds', type=int, default=90)
    ap.add_argument('--linger', type=int, default=20, help='seconds the game must keep running in the map')
    args = ap.parse_args()

    exe = os.path.abspath(args.exe)
    run_dir = os.path.join(TEST_ROOT, os.path.basename(os.path.dirname(exe)))
    os.makedirs(run_dir, exist_ok=True)
    shutil.copy(exe, run_dir)
    # the MSVC-ABI build links SDL2.dll; the MinGW one has SDL2 inside
    for dll in sorted(glob.glob(os.path.join(MSVC_SRC, 'third_party', 'SDL2-*', 'lib', 'x86', 'SDL2.dll')))[-1:]:
        shutil.copy(dll, run_dir)
    for old in glob.glob(os.path.join(run_dir, 'cod2_crash_*.txt')):
        os.remove(old)
    log_path = os.path.join(run_dir, 'consola.txt')
    env = dict(os.environ, COD2_CONSOLE_STDOUT='1', WSLENV='COD2_CONSOLE_STDOUT')
    with open(log_path, 'wb') as log:
        proc = subprocess.Popen([os.path.join(run_dir, os.path.basename(exe)), '+set', 'fs_basepath', GAME,
                                 '+set', 'fs_homepath', win(run_dir), '+set', 'r_fullscreen', '0', '+devmap', MAP],
                                cwd=run_dir, stdout=log, stderr=subprocess.STDOUT, env=env)
    gone = None
    deadline = time.time() + args.seconds
    try:
        while time.time() < deadline:
            time.sleep(3)
            with open(log_path, encoding='utf-8', errors='replace') as f:
                gone = missing(f.read())
            if not gone or proc.poll() is not None:
                break
        if not gone:
            time.sleep(args.linger)
            if proc.poll() is not None:
                gone = [f'the game kept running {args.linger} s in the map (it exited with {proc.returncode})']
    finally:
        subprocess.run(kill_command(win(os.path.join(run_dir, os.path.basename(exe)))), capture_output=True)
        proc.wait(timeout=30)
    gone = verdict(gone or [], glob.glob(os.path.join(run_dir, 'cod2_crash_*.txt')))
    if gone:
        print(f'FAIL {args.exe}: ' + '; '.join(gone) + f' (log: {log_path})')
        return 1
    print(f'ok {args.exe}: {MAP} loaded and played (log: {log_path})')
    return 0


if __name__ == '__main__':
    sys.exit(main())
