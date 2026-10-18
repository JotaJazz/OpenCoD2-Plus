#!/usr/bin/env python3
"""Runs the script builtins that read animations, through a test mod.

    python3 scripts/anim_builtins_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH]

Writes a mod in a temporary home folder whose maps/mp/<map>.gsc calls
getanimlength, animhasnotetrack, getmovedelta and getangledelta on a
multiplayer animation, starts the server on it, and checks that the map
loads, the server keeps answering and no crash report is written. These
builtins call Scr_GetAnim, which returns a struct by value.
"""
import argparse, os, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test

DEFAULT_BINARY = {115: 'build/check-linux-10/cod2_lnxded', 118: 'build/check-linux/cod2_lnxded'}

LEVEL_SCRIPT = '''#using_animtree("multiplayer");

main()
{
	length = getanimlength(%pt_stand_shoot);
	hasnote = animhasnotetrack(%pt_stand_shoot, "end");
	move = getmovedelta(%pt_stand_shoot, 0, 1);
	angle = getangledelta(%pt_stand_shoot, 0, 1);
	logprint("animtest " + length + "\\n");
}
'''


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary')
    ap.add_argument('--map', default='mp_toujane')
    args = ap.parse_args()
    binary = args.binary or os.path.join(ROOT, DEFAULT_BINARY[args.protocol])
    port = smoke_test.free_port()
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2_anim_') as home:
        mod = os.path.join(home, 'animtest', 'maps', 'mp')
        os.makedirs(mod)
        with open(os.path.join(mod, args.map + '.gsc'), 'w') as f:
            f.write(LEVEL_SCRIPT)
        log_path = os.path.join(home, 'server.log')
        with open(log_path, 'wb') as log:
            server = subprocess.Popen(
                [binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                 '+set', 'fs_game', 'animtest', '+set', 'dedicated', '1', '+set', 'net_port', str(port),
                 '+set', 'sv_cracked', '1', '+set', 'g_gametype', 'tdm', '+map', args.map],
                stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, cwd=home)
        try:
            deadline = time.time() + 120
            while time.time() < deadline and server.poll() is None and not smoke_test.query(port, 'getinfo x'):
                time.sleep(1)
            time.sleep(2)
            check('server alive after the level script', server.poll() is None and smoke_test.query(port, 'getinfo x') is not None)
        finally:
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    server.kill()
        text = open(log_path, encoding='latin-1').read()
        check('no crash report', 'CRASH REPORT' not in text)
        if failures:
            print('\n--- last lines of the server log ---')
            print('\n'.join(l for l in text.splitlines()[-30:] if not l.startswith(('[ckpt]', '[scan'))))
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
