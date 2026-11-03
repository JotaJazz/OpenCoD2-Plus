#!/usr/bin/env python3
"""The client holds more dvars than 1.3 and reports a frozen main thread.

    python3 scripts/stability_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--wine CMD] [--keep DIR]

dvars: a client (cod2_linux, or with --wine a Windows client given with
--binary) archives DVARS dvars from a config, past 1.3's 1280, and quits; a
second client on the same folder reads the last one back from a config_mp.cfg
over the 64 KB of 1.3's command buffer. freeze, with --wine only, as the watch is Windows': com_freezeWatch
2 and the freeze command stop the main thread longer than that, the client
leaves a cod2_freeze_ report and still quits by itself; with com_freezeWatch 0
it leaves none. Exits with 0 when all of that holds and there is no crash report.
The client opens a window and needs a CD key in ~/.cod2/preferences; without one
the test is skipped (exit 0).
"""
import argparse, glob, os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from hit_location_test import PREFERENCES
from fx_test import DEFAULT_CLIENT
from sound_test import wine_path

SECONDS = 120
DVARS = 3000
FREEZE = 'wait 60\nfreeze 5\nwait 40\nquit\n'
CHUNK = 100  # dvars per config: a whole one would overflow the command buffer


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    ap.add_argument('--wine', help='the command that runs a Windows client')
    ap.add_argument('--keep', help="copy each case's log and report here")
    args = ap.parse_args()
    if args.wine and not args.binary:
        ap.error('--wine needs --binary')
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_CLIENT[args.protocol]))
    game = os.path.abspath(args.game_dir)
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    def client(name, cfg, extra=(), parts=(), home=None):
        """Runs a client to its config's quit; (exit code or None, log, the report's text or None)."""
        kept = home
        with tempfile.TemporaryDirectory(prefix='cod2_stab_') as home:
            home = kept or home
            os.makedirs(os.path.join(home, 'main'), exist_ok=True)
            with open(os.path.join(home, 'main', 'test.cfg'), 'w') as f:
                f.write(cfg)
            for i, part in enumerate(parts):
                with open(os.path.join(home, 'main', f'part{i}.cfg'), 'w') as f:
                    f.write(part)
            prefs = os.path.join(home, '.cod2')
            os.makedirs(prefs, exist_ok=True)
            shutil.copy(PREFERENCES, prefs)
            base, homepath, command = game, home, [binary]
            if args.wine:
                base, homepath, command = wine_path(game), wine_path(home), [args.wine, binary]
            log_path = os.path.join(home, 'out.log')
            with open(log_path, 'wb') as log:
                proc = subprocess.Popen(command + ['+set', 'fs_basepath', base, '+set', 'fs_homepath', homepath, *extra,
                                                   '+exec', 'test.cfg'],
                                        cwd=home, stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                        env=dict(os.environ, HOME=home, COD2_CONSOLE_STDOUT='1', COD2_CRASH_NOPOPUP='1',
                                                 SDL_AUDIODRIVER='dummy'))
                try:
                    code = proc.wait(timeout=SECONDS)
                except subprocess.TimeoutExpired:
                    code = None
                    proc.terminate()
                    try:
                        proc.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        proc.kill()
            text = open(log_path, encoding='latin-1').read()
            reports = glob.glob(os.path.join(home, 'cod2_freeze_*'))
            report = open(reports[0], encoding='latin-1').read() if reports else None
            check(f'{name}: no crash report', 'CRASH REPORT' not in text and not glob.glob(os.path.join(home, 'cod2_crash_*')))
            if args.keep:
                os.makedirs(args.keep, exist_ok=True)
                shutil.copy(log_path, os.path.join(args.keep, f'{name}.log'))
                if reports:
                    shutil.copy(reports[0], os.path.join(args.keep, f'{name}_report.txt'))
            return code, text, report

    parts = []
    for first in range(0, DVARS, CHUNK):
        lines = [f'seta stability_{i} "{i:024}"\n' for i in range(first, first + CHUNK)]
        following = len(parts) + 1
        parts.append(''.join(lines) + (f'exec part{following}.cfg\n' if first + CHUNK < DVARS
                                       else f'wait 20\nstability_{DVARS - 1}\nwait 20\nquit\n'))
    with tempfile.TemporaryDirectory(prefix='cod2_stab_') as home:
        code, text, _ = client('dvars', 'exec part0.cfg\n', parts=parts, home=home)
        check('dvars: the client quits', code == 0, str(code))
        check('dvars: none refused', "Can't create dvar" not in text)
        configs = glob.glob(os.path.join(home, '*', 'players', '*', 'config_mp.cfg'))
        size = os.path.getsize(configs[0]) if configs else 0
        check('dvars: the config passes 64 KB', size > 65536, str(size))
        code, text, _ = client('dvars_again', f'wait 20\nstability_{DVARS - 1}\nwait 20\nquit\n', home=home)
        check('dvars: the last one comes back', f'"stability_{DVARS - 1}" is: "{DVARS - 1:024}' in text.replace('^7', ''))
    parts = [''.join(f'set full_{first + i} 1\n' for i in range(CHUNK)) + f'exec part{n + 1}.cfg\n'
             for n, first in enumerate(range(0, 4200, CHUNK))]
    parts.append('wait 20\nquit\n')
    code, text, _ = client('full', 'exec part0.cfg\n', parts=parts)
    check('full: a full pool only prints', code == 0 and "Can't create dvar" in text, str(code))
    if args.wine:
        code, text, report = client('freeze', FREEZE, ['+set', 'developer', '1', '+set', 'com_freezeWatch', '2'])
        check('freeze: the client still quits', code == 0, str(code))
        check('freeze: it reports the frozen thread', bool(report) and 'END FREEZE REPORT' in report and 'cod2_win32' in report.split('backtrace')[-1])
        code, text, report = client('no_freeze', FREEZE, ['+set', 'developer', '1', '+set', 'com_freezeWatch', '0'])
        check('no_freeze: com_freezeWatch 0 reports nothing', code == 0 and report is None, str(code))
    print('PASS' if not failures else f'FAIL: {len(failures)} check(s)')
    return 0 if not failures else 1


if __name__ == '__main__':
    sys.exit(main())
