#!/usr/bin/env python3
"""A fake server sends a real client the messages only a hostile server sends.

    python3 scripts/hostile_server_test.py /path/to/CoD2 [--binary PATH] [--server PATH] [--case NAME]

Runs the dedicated server once to learn a real map's configstrings, stops it,
then plays a server on 127.0.0.1 itself: it answers the handshake and sends the
client a gamestate and snapshots of its own. Each case starts a new client and
sends it one malformed message. The client must not crash: it runs on or leaves
by its error path, writes no crash report, and logs the text the case expects.
The client needs a CD key in ~/.cod2/preferences: without one the test is
skipped (exit 0). It opens a window. Exits with 0 when every case holds. Set
COD2_HOSTILE_LOG to a path to keep the last client's log.
"""
import argparse, glob, http.server, os, re, shutil, socket, subprocess, sys, tempfile, threading, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import cod2client, smoke_test
from cod2client import FRAGMENT_BIT, MsgWriter, huffman_compress, xor_codec
from listen_test import PREFERENCES, RCON, Run, client, wait_for

DEFAULT_BINARY = 'build/check-linux/cod2_linux'
DEFAULT_SERVER = 'build/check-linux/cod2_lnxded'
MAP = 'mp_toujane'
OOB = b'\xff\xff\xff\xff'
CHALLENGE = 0x1234567
SERVER_ID = 77
PROTOCOL = 118
ACTIVE = 'Done with smoke grenades'     # cgame logs it on its first snapshots
SVC_GAMESTATE, SVC_CONFIGSTRING, SVC_BASELINE, SVC_SERVERCOMMAND, SVC_DOWNLOAD, SVC_SNAPSHOT, SVC_EOF = 1, 2, 3, 4, 5, 6, 7
EMPTY_COMMAND_SLOT = 100
FRAGMENT_SIZE = 1300            # a fragment this long says more follow
NO_ENTITY = 1023
ET_PLAYER = 1
ET_CORPSE = 2
ET_SCRIPTMOVER = 6
LONG_KEY = 'EXE_ERR_GET_NEWEST_DRIVERS'     # its English text is 441 characters
ESCAPE = f'cod2_hostile_escape_{os.getpid()}'
LONG_NAME = 'y' * 80        # cut at originalDownloadName's 64 bytes, it loses its .iwd
PAYLOAD = b'#!/bin/sh\n'


class AnyFile(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-Length', str(len(PAYLOAD)))
        self.end_headers()
        self.wfile.write(PAYLOAD)

    def log_message(self, fmt, *args):
        pass


def captured_configstrings(game_dir, server_bin):
    """A real server's configstrings for MAP, as a client gets them."""
    port = smoke_test.free_port()
    with tempfile.TemporaryDirectory(prefix='cod2_hostile_') as home:
        server = subprocess.Popen(
            [server_bin, '+set', 'fs_basepath', os.path.abspath(game_dir), '+set', 'fs_homepath', home,
             '+set', 'net_port', str(port), '+set', 'sv_cracked', '1', '+set', 'rcon_password', RCON,
             '+set', 'dedicated', '1', '+map', MAP],
            cwd=home, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
        try:
            if not wait_for(lambda: smoke_test.query(port, 'getinfo x'), 60):
                raise RuntimeError('the dedicated server did not start')
            m = re.search(r'"sv_serverid" is: ?"(-?\d+)', smoke_test.rcon(port, RCON, 'sv_serverid'))
            c = cod2client.Client(port)
            if c.connect(PROTOCOL, 'capture') is not None or not c.request_gamestate(int(m.group(1)), 15):
                raise RuntimeError('no gamestate from the dedicated server')
            strings = dict(c.state.configstrings)
            c.close()
            return strings
        finally:
            server.terminate()
            server.wait(timeout=15)


def set_info(info, key, value):
    pairs = info.split('\\')[1:]
    fields = dict(zip(pairs[::2], pairs[1::2]))
    fields[key] = value
    return ''.join(f'\\{k}\\{v}' for k, v in fields.items())


def gamestate(strings, sequence=0, raw_index=None):
    """The configstrings; raw_index adds one with that index, for an out of
    range one."""
    w = MsgWriter()
    w.byte(SVC_GAMESTATE)
    w.long(sequence)
    for index, text in sorted(strings.items()):
        w.byte(SVC_CONFIGSTRING)
        w.data += (index & 0xffff).to_bytes(2, 'little')
        w.string(text)
    if raw_index is not None:
        w.byte(SVC_CONFIGSTRING)
        w.data += (raw_index & 0xffff).to_bytes(2, 'little')
        w.string('x')
    w.byte(SVC_EOF)
    w.long(0)           # client number
    w.long(0x5555)      # checksum feed
    w.byte(SVC_EOF)
    return bytes(w.data)


def entity(w, number, fields):
    """A delta from the zeroed baseline: only the fields in {index: (value,
    bits)}, written up to the last one, from cod2snap.ENTITY_FIELDS."""
    last = max(fields) + 1
    w.bits(number, 10)
    w.bits(0, 1)        # not removed
    w.bits(1, 1)        # has fields
    w.byte(last)
    for i in range(last):
        if i in fields:
            value, bits = fields[i]
            w.bits(3, 2)        # changed, not zero
            w.bits(value, bits & 7)
            for shift in range(bits & 7, bits, 8):
                w.byte(value >> shift)
        else:
            w.bits(0, 1)


def viewlocked_ps():
    """Only viewlocked_entNum, 60000: (playerstate lc, its bits)."""
    from cod2snap import PLAYERSTATE_FIELDS
    names = [f[0] for f in PLAYERSTATE_FIELDS]
    index = names.index('viewlocked_entNum')
    return index + 1, 60000


def snapshot(server_time, lc=0, junk=b'', entities=(), ps_fields=None):
    w = MsgWriter()
    w.byte(SVC_SNAPSHOT)
    w.long(server_time)
    w.byte(0)           # no delta
    w.byte(0)           # flags
    if ps_fields:
        lc, value = ps_fields
        w.byte(lc)
        w.bits(0, lc - 1)       # no change, up to the last field
        w.bits(1, 1)
        w.data += value.to_bytes(2, 'little')
    else:
        w.byte(lc)
    w.data += junk      # the fields a bad lc reads
    if not junk:
        w.bits(0, 1)    # stats
        w.bits(0, 1)    # ammo
        w.bits(0, 4)    # ammo clip groups
        w.bits(0, 1)    # objectives
        w.bits(0, 1)    # hud
    for number, fields in entities:
        entity(w, number, fields)
    w.bits(NO_ENTITY, 10)
    w.bits(0, 1)        # no client states
    w.byte(SVC_EOF)
    return bytes(w.data)


def server_command(sequence, text):
    w = MsgWriter()
    w.byte(SVC_SERVERCOMMAND)
    w.long(sequence)
    w.string(text)
    return bytes(w.data)


def download_block(block, size, count, data=b''):
    w = MsgWriter()
    w.byte(SVC_DOWNLOAD)
    w.data += (block & 0xffff).to_bytes(2, 'little')
    if block == 0:
        w.long(size)
    w.data += (count & 0xffff).to_bytes(2, 'little')
    w.data += data
    w.byte(SVC_EOF)
    return bytes(w.data)


class FakeServer:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('127.0.0.1', 0))
        self.sock.settimeout(0.02)
        self.port = self.sock.getsockname()[1]
        self.addr = None
        self.sequence = 0
        self.connected = False
        self.connect_reply = b'connectResponse'
        self.userinfo = b''

    def close(self):
        self.sock.close()

    def poll(self):
        """Answers the handshake; True once the client is connected."""
        try:
            data, addr = self.sock.recvfrom(65535)
        except socket.timeout:
            return self.connected
        self.addr = addr
        if data.startswith(OOB + b'getchallenge'):
            self.sock.sendto(OOB + b'challengeResponse %d' % CHALLENGE, addr)
        elif data.startswith(OOB + b'connect'):
            self.userinfo = data
            self.sock.sendto(OOB + self.connect_reply, addr)
            self.connected = True
        return self.connected

    def raw(self, data):
        self.sock.sendto(data, self.addr)

    def send(self, payload):
        """One message: the sequence, the reliable acknowledge, then the
        payload compressed and XORed. The client's command slot 0 is empty, so
        the XOR's string is empty too."""
        self.sequence += 1
        body = xor_codec(huffman_compress(payload), (CHALLENGE ^ self.sequence) & 0xff, '')
        message = EMPTY_COMMAND_SLOT.to_bytes(4, 'little') + body
        if len(message) < FRAGMENT_SIZE:
            self.raw(self.sequence.to_bytes(4, 'little') + message)
            return
        header = (self.sequence | FRAGMENT_BIT).to_bytes(4, 'little')
        start = 0
        while True:
            chunk = message[start:start + FRAGMENT_SIZE]
            self.raw(header + start.to_bytes(4, 'little') + len(chunk).to_bytes(2, 'little') + chunk)
            start += len(chunk)
            if len(chunk) < FRAGMENT_SIZE:
                return
            time.sleep(0.0005)


class Case:
    """stage: 'gamestate' sends strings/raw_index in the gamestate and goes no
    further than `after`; 'active' first brings the client to its first
    snapshot, then sends `after`. expect: text the log must hold. absent: files
    that must not exist, relative to the client's main folder's parent. forbid:
    text the log must not hold. args: the client's. hold: seconds to watch it
    after that. silent: no more snapshots after that. limit: (text, the most times
    the log may hold it). connect_reply: the server's answer to connect."""

    def __init__(self, name, stage='active', strings=None, raw_index=None, after=None, expect=None, absent=(),
                 forbid=None, args=(), hold=12, silent=False, limit=None, connect_reply=None):
        self.name, self.stage, self.strings, self.raw_index = name, stage, strings or {}, raw_index
        self.after, self.expect, self.absent, self.forbid = after, expect, absent, forbid
        self.args, self.hold, self.silent, self.limit = list(args), hold, silent, limit
        self.connect_reply = connect_reply


def command(text):
    return lambda srv, run: run.commands(text)


def missing_iwd(strings, name='main/zz_hostile'):
    """The systeminfo of a server that asks the client to download one iwd."""
    info = set_info(strings[1], 'sv_referencedIwds', '12345')
    return {1: set_info(info, 'sv_referencedIwdNames', name)}


def huffman_bomb(srv, run):
    # nops take two bits: under MAX_MSGLEN bytes decompress to four times that
    srv.send(b'\0' * 0x7f000)


def bad_lc(srv, run):
    srv.send(snapshot(run.time(), lc=200, junk=b'\xff' * 400))


def negative_block(srv, run):
    srv.send(download_block(0, 100, -1))


def info_response(srv, run):
    srv.raw(OOB + b'infoResponse\n' + b'\\protocol\\118\0' + b'A' * 1023 + b'\0')


def wwwdl(url, size, flags):
    w = MsgWriter()
    w.byte(SVC_DOWNLOAD)
    w.data += (0xffff).to_bytes(2, 'little')
    w.string(url)
    w.long(size)
    w.long(flags)
    w.byte(SVC_EOF)
    return bytes(w.data)


def http_redirect(srv, run):
    """A redirect to an HTTP server that serves anything: the client writes it
    under the name the server gave for the iwd."""
    srv.send(wwwdl(f'http://127.0.0.1:{run.http_port}/x.iwd', len(PAYLOAD), 0))


def open_file_url(srv, run):
    srv.send(wwwdl('file:///etc/passwd', 0, 2))


def entities(*items):
    return lambda srv, run: srv.send(snapshot(run.time(), entities=items))


def make_cases(strings):
    # cod2snap.ENTITY_FIELDS: eType and clientNum's positions
    from cod2snap import ENTITY_FIELDS
    names = [f[0] for f in ENTITY_FIELDS]
    etype, clientnum, eflags = names.index('eType'), names.index('clientNum'), names.index('eFlags')
    xmodel = names.index('index')
    return [
        Case('configstring_negative', 'gamestate', raw_index=-1, expect='MAX_CONFIGSTRINGS'),
        Case('playerstate_lc', after=bad_lc, expect='bad field count'),
        Case('download_negative_size', 'gamestate', strings=missing_iwd(strings), after=negative_block,
             expect='bad download block size'),
        Case('download_traversal', 'gamestate', strings=missing_iwd(strings, '../' + ESCAPE), after=http_redirect,
             absent=['../' + ESCAPE + '.iwd', '../' + ESCAPE + '.iwd.tmp']),
        Case('download_long_name', 'gamestate', strings=missing_iwd(strings, 'main/' + LONG_NAME),
             after=http_redirect, absent=['main/' + LONG_NAME[:58]]),
        Case('open_file_url', 'gamestate', strings=missing_iwd(strings), after=open_file_url, expect='non-http'),
        # with snapshots on time the clock stays in step: no jumps to catch up
        Case('client_clock_steady', args=['+set', 'cl_showTimeDelta', '1'], hold=30,
             limit=('cl_showTimeDelta: average', 2)),
        # a connection times out when packets stop, not some time after connecting
        Case('timeout_keeps_playing', args=['+set', 'cl_timeout', '15'], hold=30, forbid='timed out'),
        Case('timeout_when_silent', args=['+set', 'cl_timeout', '5'], hold=15, silent=True, expect='timed out'),
        # not hostile: a mod's physics, which prediction needs
        Case('codinfo_dvars_apply', strings={0x8e: 'jump_height', 0xee: '60'},
             forbid='protected cvar'),
        # not hostile: a Plus server's answer, and one claiming another version
        Case('plus_server', connect_reply=b'connectResponse 121', expect='CoD2 Plus server'),
        Case('plus_other_version', connect_reply=b'connectResponse 999', forbid='CoD2 Plus server'),
        Case('huffman_bomb', after=huffman_bomb),
        Case('disconnect_percent', after=command('w "%s%s%s%s%n"')),
        Case('translate_long', after=command('e "' + ('\x14' + LONG_KEY) * 6 + '"')),
        Case('items_long', 'gamestate', strings={8: 'f' * 2000}),
        Case('weapons_many', 'gamestate', strings={7: ' a' * 500}),
        Case('vote_long', 'gamestate', strings={16: 'f' * 8000}),
        Case('reverb_prio', after=command('r 99 generic 1 1 1')),
        Case('reverb_off_prio', after=command('D 99 1')),
        Case('channelvol_off_prio', after=command('F 99 1')),
        Case('mute_negative', after=command('J -5000')),
        Case('info_response_full', 'gamestate', after=info_response),
        Case('channelvol', after=command('E 99 99 1')),
        Case('corpse_entnum', after=entities((1000, {etype: (ET_CORPSE, 8), eflags: (8, 24)}))),
        Case('player_clientnum', after=entities((5, {etype: (ET_PLAYER, 8), clientnum: (200, 8)}))),
        Case('viewlocked', after=lambda srv, run: srv.send(snapshot(run.time(), ps_fields=viewlocked_ps()))),
        Case('xmodel_index', after=entities((100, {etype: (ET_SCRIPTMOVER, 8), xmodel: (1000, 10)}))),
    ]


class ClientRun:
    def __init__(self, srv, strings, case):
        self.srv, self.base = srv, strings
        self.case = case
        self.server_time = 10000
        self.started = time.monotonic()
        self.http_port = None
        self.sequence = 0

    def time(self):
        """The clock of a server: it does not depend on when a snapshot leaves."""
        return self.server_time + int((time.monotonic() - self.started) * 1000)

    def commands(self, text):
        self.sequence += 1
        self.srv.send(server_command(self.sequence, text) + snapshot(self.time()))

    def tick(self):
        """Keeps a snapshot going, for the client's timeouts."""
        self.srv.send(snapshot(self.time()))


def run_case(case, base, binary, game_dir, timeout=120):
    """'' when the case holds, otherwise what went wrong."""
    strings = {**base, **case.strings}
    srv = FakeServer()
    if case.connect_reply:
        srv.connect_reply = case.connect_reply
    httpd = http.server.ThreadingHTTPServer(('127.0.0.1', 0), AnyFile)
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    with tempfile.TemporaryDirectory(prefix='cod2_hostile_') as home:
        log_path = os.path.join(home, 'out.log')
        with open(log_path, 'wb') as log:
            cl = client(binary, game_dir, home,
                        ['+set', 'cl_allowDownload', '1', '+set', 'net_port', str(smoke_test.free_port())]
                        + case.args + ['+connect', f'127.0.0.1:{srv.port}'], log)
        text = lambda: open(log_path, encoding='latin-1').read()
        run = ClientRun(srv, strings, case)
        run.http_port = httpd.server_address[1]
        try:
            end = time.time() + 45
            while not srv.poll():
                if time.time() > end or cl.poll() is not None:
                    return f'no connection (exit code {cl.poll()})'
            if b'\\plus\\121' not in srv.userinfo:
                return f'no Plus version in the connect packet: {srv.userinfo[:80]!r}'
            srv.send(gamestate(strings, raw_index=case.raw_index))
            if case.stage == 'active':
                end = time.time() + timeout
                while ACTIVE not in text():
                    if time.time() > end:
                        return 'the client never reached a snapshot'
                    if cl.poll() is not None:
                        return f'the client left before its first snapshot (exit code {cl.poll()})'
                    srv.poll()
                    run.tick()
                    time.sleep(0.05)
            else:
                time.sleep(2)
            if case.after:
                case.after(srv, run)
            end = time.time() + case.hold
            while time.time() < end and cl.poll() is None:
                srv.poll()
                if case.stage == 'active' and not case.silent:
                    run.tick()
                time.sleep(0.05)
            code = cl.poll()
            crashes = glob.glob(os.path.join(home, 'cod2_crash_*.txt'))
            log_text = text()
            if crashes:
                return 'crash report: ' + os.path.basename(crashes[0])
            if code not in (None, 0):
                return f'exit code {code}'
            if case.expect and case.expect not in log_text:
                return f'no {case.expect!r} in the log'
            if case.forbid and case.forbid in log_text:
                return f'{case.forbid!r} in the log'
            if case.limit and log_text.count(case.limit[0]) > case.limit[1]:
                return f'{log_text.count(case.limit[0])} times {case.limit[0]!r} in the log'
            for name in case.absent:
                path = os.path.normpath(os.path.join(home, name))
                if os.path.exists(path):
                    os.remove(path)
                    return f'a file at {name}'
            return ''
        finally:
            httpd.shutdown()
            if os.environ.get('COD2_HOSTILE_LOG'):
                shutil.copy(log_path, os.environ['COD2_HOSTILE_LOG'])
            if cl.poll() is None:
                cl.terminate()
                try:
                    cl.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    cl.kill()
                    cl.wait()
            srv.close()


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--binary')
    ap.add_argument('--server')
    ap.add_argument('--case', action='append', help='run only this case; repeat it for more')
    args = ap.parse_args()
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_BINARY))
    server_bin = os.path.abspath(args.server or os.path.join(ROOT, DEFAULT_SERVER))
    base = captured_configstrings(args.game_dir, server_bin)
    run = Run()
    for case in make_cases(base):
        if args.case and case.name not in args.case:
            continue
        problem = run_case(case, base, binary, args.game_dir)
        run.check(case.name, not problem, problem)
    return 1 if run.failures else 0


if __name__ == '__main__':
    sys.exit(main())
