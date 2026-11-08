#!/usr/bin/env python3
"""An English install runs in Spanish on its english localized files.

    python3 scripts/language_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH]

GAME must hold localized_english iwds and no localized_spanish ones. A client
started with loc_language 4 keeps it (1.3 falls back to 0, leaving 4 latched)
and still sees the english localized .str files, whose LANG_SPANISH lines it
then reads; loc_language 6 (Russian, which the stock fonts cannot draw) falls
back to English, as on 1.3. Exits with 0 when both hold. The client opens a window
and needs a CD key in ~/.cod2/preferences; without one the test is skipped.
"""
import argparse, os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from hit_location_test import PREFERENCES
from fx_test import DEFAULT_CLIENT

CASES = [(4, 4), (6, 0)]
CFG = 'dir localizedstrings str\nloc_language\nquit\n'


def run(binary, game, language):
    with tempfile.TemporaryDirectory(prefix='cod2_lang_') as home:
        os.makedirs(os.path.join(home, '.cod2'))
        shutil.copy(PREFERENCES, os.path.join(home, '.cod2'))
        os.makedirs(os.path.join(home, 'main'))
        with open(os.path.join(home, 'main', 'test.cfg'), 'w') as f:
            f.write(CFG)
        proc = subprocess.run([binary, '+set', 'fs_basepath', game, '+set', 'fs_homepath', home,
                               '+set', 'loc_language', str(language), '+wait', '+exec', 'test.cfg'],
                              cwd=home, capture_output=True, text=True, errors='replace', timeout=120,
                              stdin=subprocess.DEVNULL,
                              env=dict(os.environ, HOME=home, COD2_CONSOLE_STDOUT='1', COD2_CRASH_NOPOPUP='1',
                                       SDL_AUDIODRIVER='dummy'))
        return proc.stdout + proc.stderr


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    args = ap.parse_args()
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_CLIENT[args.protocol]))
    failed = False
    for language, expected in CASES:
        log = run(binary, os.path.abspath(args.game_dir), language)
        ok = f'"loc_language" is: "{expected}^7"' in log and '88ridge.str' in log
        print(f'{"ok  " if ok else "FAIL"} loc_language {language} -> {expected}', flush=True)
        failed |= not ok
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
