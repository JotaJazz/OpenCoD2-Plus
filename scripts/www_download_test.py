#!/usr/bin/env python3
"""A client joins a 1.3 server whose mod it lacks and downloads it over HTTP.

    python3 scripts/www_download_test.py /path/to/CoD2 [--binary PATH] [--server PATH] [--wine WINE] [--disconnect]

Serves a mod (one 3 MB .iwd) from a temporary home: the dedicated server runs
it with sv_wwwDownload 1 and sv_wwwBaseURL pointing at an HTTP server this
script runs on 127.0.0.1. A client with cl_allowDownload 1 connects: it must
fetch the .iwd over HTTP (a GET answered 200), keep it byte for byte, and then
join the game. --disconnect sets sv_wwwDlDisconnected 1 (the client leaves,
downloads and reconnects, as most public servers ask). With --wine the client
is a Windows one (cod2_win32.exe, which downloads through WinINet) run by the
given wine command. The client needs a CD key in ~/.cod2/preferences: without
one the test is skipped (exit 0). It opens a window. Only 1.3 (protocol 118)
has redirected downloads. Exits with 0 when all of that holds.
"""
import argparse, functools, http.server, os, shutil, subprocess, sys, tempfile, threading, time, zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test
from listen_test import PREFERENCES, RCON, Run, client, players, wait_for

DEFAULT_BINARY = 'build/check-linux/cod2_linux'
DEFAULT_SERVER = 'build/check-linux/cod2_lnxded'
MOD = 'mods/dltest'
IWD = 'zz_dltest.iwd'


class Handler(http.server.SimpleHTTPRequestHandler):
    requests = []

    def log_message(self, fmt, *args):
        Handler.requests.append(fmt % args)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--binary', help='the client (with --wine, a Windows one)')
    ap.add_argument('--server')
    ap.add_argument('--wine', help='the command that runs --binary (a wine wrapper)')
    ap.add_argument('--disconnect', action='store_true', help='sv_wwwDlDisconnected 1')
    ap.add_argument('--map', default='mp_toujane')
    args = ap.parse_args()
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    if args.wine and not args.binary:
        ap.error('--wine runs --binary, a Windows client')
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_BINARY))
    server_bin = os.path.abspath(args.server or os.path.join(ROOT, DEFAULT_SERVER))
    wine = os.path.abspath(args.wine) if args.wine and os.sep in args.wine else args.wine
    port, http_port = smoke_test.free_port(), smoke_test.free_port()
    run = Run()
    procs = []
    with tempfile.TemporaryDirectory(prefix='cod2_wwwsrv_') as srv_home, \
            tempfile.TemporaryDirectory(prefix='cod2_wwwcli_') as cli_home:
        os.makedirs(os.path.join(srv_home, MOD))
        served = os.path.join(srv_home, MOD, IWD)
        with zipfile.ZipFile(served, 'w', zipfile.ZIP_STORED) as z:
            z.writestr('dltest/blob.bin', os.urandom(3 * 1024 * 1024))
        # the console reads // as a comment: the URL goes quoted, in a cfg
        with open(os.path.join(srv_home, MOD, 'wwwdl.cfg'), 'w') as f:
            f.write(f'set sv_wwwBaseURL "http://127.0.0.1:{http_port}"\n')
        # a Windows program has no HOME: its key is read from the working folder
        os.makedirs(os.path.join(cli_home, '.cod2'))
        shutil.copy(PREFERENCES, os.path.join(cli_home, '.cod2', 'preferences'))

        httpd = http.server.ThreadingHTTPServer(('127.0.0.1', http_port),
                                                functools.partial(Handler, directory=srv_home))
        threading.Thread(target=httpd.serve_forever, daemon=True).start()
        server_log, client_log = os.path.join(srv_home, 'out.log'), os.path.join(cli_home, 'out.log')
        try:
            with open(server_log, 'wb') as log:
                server = subprocess.Popen(
                    [server_bin, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', srv_home,
                     '+set', 'fs_game', MOD, '+set', 'net_port', str(port), '+set', 'sv_cracked', '1',
                     '+set', 'rcon_password', RCON, '+set', 'sv_allowDownload', '1', '+set', 'sv_wwwDownload', '1',
                     '+exec', 'wwwdl.cfg', '+set', 'sv_wwwDlDisconnected', '1' if args.disconnect else '0',
                     '+set', 'dedicated', '1', '+map', args.map],
                    cwd=srv_home, stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
            procs.append(server)
            up = wait_for(lambda: server.poll() is None and smoke_test.query(port, 'getinfo x'), 60)
            run.check('the server is up', bool(up), f'exit code {server.poll()}' if not up else '')
            if up:
                with open(client_log, 'wb') as log:
                    cl = client(binary, args.game_dir, cli_home,
                                ['+set', 'cl_allowDownload', '1', '+set', 'developer', '1',
                                 '+set', 'net_port', str(port + 1), '+connect', f'127.0.0.1:{port}'], log, wine=wine)
                procs.append(cl)
                fetched = os.path.join(cli_home, MOD, IWD)
                got = wait_for(lambda: os.path.exists(fetched) and os.path.getsize(fetched) == os.path.getsize(served),
                               240)
                run.check('the client fetches the .iwd over HTTP',
                          any(f'GET /{MOD}/{IWD}' in r and '" 200' in r for r in Handler.requests),
                          '; '.join(Handler.requests) or 'no request')
                run.check('the .iwd is kept byte for byte',
                          bool(got) and open(fetched, 'rb').read() == open(served, 'rb').read())
                rows = wait_for(lambda: players(port), 240)
                run.check('then the client joins the game', bool(rows))
                run.check('the client still runs', cl.poll() is None, f'exit code {cl.poll()}')
        finally:
            httpd.shutdown()
            for p in procs:
                if p.poll() is None:
                    p.terminate()
                    try:
                        p.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        p.kill()
        text = open(client_log, encoding='latin-1').read() if os.path.exists(client_log) else ''
        run.check('no download failure', 'Failed to initialize download' not in text
                  and 'Download failure' not in text)
        run.check('no crash report from the client', 'CRASH REPORT' not in text)
        if run.failures and text:
            print('\n--- last lines of the client log ---')
            print('\n'.join(l for l in text.splitlines()[-25:] if not l.startswith(('[', 'FX Error'))))
    print('PASS' if not run.failures else f'FAIL: {len(run.failures)} check(s)')
    return 0 if not run.failures else 1


if __name__ == '__main__':
    sys.exit(main())
