#!/usr/bin/env python3
"""The client's animation script picks the same animations as the server's.

    python3 scripts/anim_script_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH]

Starts the Linux client on its own game of mp_toujane with
COD2_CHECK_ANIMSCRIPT=1: once cgame has loaded the animation script, the
client runs the engine's playerstate functions (src/PC/bgame, which predict)
and CoD2rev-Plus's (the local server's) on the same playerstates, every
weapon, stance, aim, pitch, firing, state, movetype and event, and prints
"animscript check: N cases, M differ"; before that it compares the parsed
animations' flags, movetypes, durations and speeds (not the note types, which
the server leaves unset). Exits with 0 when at least MIN_CASES
cases ran, no case or animation differs, and the client leaves no crash report. The client
opens a window; it needs no CD key.
"""
import argparse, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_CLIENT = {115: 'build/check-linux-10/cod2_linux', 118: 'build/check-linux/cod2_linux'}
MIN_CASES = 10000
TIMEOUT = 120


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    args = ap.parse_args()
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_CLIENT[args.protocol]))
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2_animscript_') as home:
        log_path = os.path.join(home, 'out.log')
        with open(log_path, 'wb') as log:
            client = subprocess.Popen([binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir),
                                       '+set', 'fs_homepath', home, '+set', 'g_gametype', 'dm',
                                       '+devmap', 'mp_toujane'],
                                      stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL, cwd=home,
                                      env=dict(os.environ, COD2_CHECK_ANIMSCRIPT='1'))
            end = time.time() + TIMEOUT
            text = ''
            while time.time() < end and client.poll() is None:
                time.sleep(1)
                text = open(log_path, encoding='latin-1').read()
                if re.search(r'animscript check: (\d+ cases|no local server)', text):
                    break
            client.terminate()
            try:
                client.wait(15)
            except subprocess.TimeoutExpired:
                client.kill()
        text = open(log_path, encoding='latin-1').read()
        for line in re.findall(r'animscript check: (?:anim|event|animation) .*', text):
            print('  ' + line)
        anims = re.search(r'animscript check: (\d+) animations, (\d+) differ', text)
        if anims:
            check('the engine parses the animations as CoD2rev-Plus', int(anims.group(1)) > 0 and anims.group(2) == '0',
                  f'{anims.group(2)} of {anims.group(1)} differ')
        result = re.search(r'animscript check: (\d+) cases, (\d+) differ', text)
        reports = [n for n in os.listdir(home) if n.startswith('cod2_crash_')]
        check('the check ran on a local server', result is not None,
              'no local server' if 'no local server' in text else '')
        if result:
            cases, differ = int(result.group(1)), int(result.group(2))
            check(f'at least {MIN_CASES} cases', cases >= MIN_CASES, str(cases))
            check('the engine picks what CoD2rev-Plus picks', differ == 0, f'{differ} differ')
        check('no crash report from the client', 'CRASH REPORT' not in text and not reports, ', '.join(reports))
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
