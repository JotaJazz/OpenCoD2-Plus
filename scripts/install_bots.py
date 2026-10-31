#!/usr/bin/env python3
"""Installs the simple bots of mods/bots as a server-side mod folder.

    python3 scripts/install_bots.py /path/to/CoD2 [--dest FOLDER] [--name cod2rev_bots]

It writes two loose script files into FOLDER/<name>:
  maps/mp/bots/*.gsc                     the bots, copied from this repository
  maps/mp/gametypes/_callbacksetup.gsc   the game's own file with one line added

The second file is built from your own game files (read from the .iwd archives
in /path/to/CoD2/main), because the game scripts cannot be redistributed here.
FOLDER defaults to the game folder. Start the server with:

    +set fs_game <name> +set bots_count 6
"""
import argparse, glob, os, shutil, sys, zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BOT_DIR = os.path.join(ROOT, 'mods', 'bots', 'maps', 'mp', 'bots')
CALLBACKS = 'maps/mp/gametypes/_callbacksetup.gsc'
MARKER = '[[level.callbackStartGameType]]();'
HOOK = 'thread maps\\mp\\bots\\_bots::init();'


def read_game_script(game_dir, name):
    # later archives override earlier ones, as in the game
    for path in sorted(glob.glob(os.path.join(game_dir, 'main', 'iw_*.iwd')), reverse=True):
        with zipfile.ZipFile(path) as archive:
            if name in archive.namelist():
                return archive.read(name).decode('latin-1'), os.path.basename(path)
    sys.exit(f'{name} not found in the iw_*.iwd files of {os.path.join(game_dir, "main")}')


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir', help='game folder that contains "main" with the .iwd files')
    ap.add_argument('--dest', help='folder to create the mod in (default: the game folder)')
    ap.add_argument('--name', default='cod2rev_bots', help='mod folder name, used with fs_game')
    args = ap.parse_args()

    text, source = read_game_script(args.game_dir, CALLBACKS)
    if HOOK in text:
        sys.exit(f'{CALLBACKS} in {source} already starts the bots')
    if text.count(MARKER) != 1:
        sys.exit(f'{CALLBACKS} in {source} does not look like the stock file, not patching it')
    newline = '\r\n' if '\r\n' in text else '\n'
    text = text.replace(MARKER, MARKER + newline + '\t\t' + HOOK)

    mod = os.path.join(args.dest or args.game_dir, args.name)
    os.makedirs(os.path.join(mod, 'maps', 'mp', 'bots'), exist_ok=True)
    os.makedirs(os.path.join(mod, 'maps', 'mp', 'gametypes'), exist_ok=True)
    for script in sorted(glob.glob(os.path.join(BOT_DIR, '*.gsc'))):
        shutil.copyfile(script, os.path.join(mod, 'maps', 'mp', 'bots', os.path.basename(script)))
    with open(os.path.join(mod, *CALLBACKS.split('/')), 'w', encoding='latin-1', newline='') as out:
        out.write(text)

    print(f'Installed in {mod} ({CALLBACKS} taken from {source})')
    print(f'Start the server with: +set fs_game {args.name} +set bots_count 6')


if __name__ == '__main__':
    sys.exit(main())
