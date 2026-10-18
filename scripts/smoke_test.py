#!/usr/bin/env python3
"""Starts the Linux server against a real game folder and checks that it answers.

    python3 scripts/smoke_test.py /path/to/CoD2 [--protocol 118] [--binary bin/cod2rev_lnxded]

Besides the queries, a minimal client (cod2client.py) connects, receives the
gamestate, enters the game and sends commands, one of them in a fragmented
message. The game folder is only read: logs and configs go to a temporary home
folder. The server runs in LAN mode (dedicated 1), so nothing is sent to the
master servers; a second run in internet mode (dedicated 2) heartbeats to a fake
master on 127.0.0.1 instead of the real ones. Exits with 0 when every check
passes.
"""
import argparse, os, random, re, secrets, signal, socket, string, subprocess, sys, tempfile, time, zipfile

import cod2client

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PREFIX = b'\xff\xff\xff\xff'


def query(port, command, timeout=2.0):
    """Sends a connectionless command and returns the reply as (header, info dict), or None."""
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.settimeout(timeout)
        s.sendto(PREFIX + command.encode(), ('127.0.0.1', port))
        try:
            data, _ = s.recvfrom(65535)
        except OSError:
            return None
    lines = data[len(PREFIX):].decode('latin-1').split('\n')
    parts = lines[1].split('\\')[1:] if len(lines) > 1 else []
    return lines[0], dict(zip(parts[0::2], parts[1::2]))


def players(port):
    """{name: ping} of the clients listed by getstatus, or None without a reply."""
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.settimeout(2.0)
        s.sendto(PREFIX + b'getstatus', ('127.0.0.1', port))
        try:
            data, _ = s.recvfrom(65535)
        except OSError:
            return None
    lines = data[len(PREFIX):].decode('latin-1').split('\n')[2:]
    return {m.group(2): int(m.group(1))
            for m in (re.match(r'-?\d+ (-?\d+) "(.*)"', l) for l in lines) if m}


def rcon(port, password, command):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.settimeout(2.0)
        s.sendto(PREFIX + f'rcon {password} {command}'.encode(), ('127.0.0.1', port))
        try:
            data, _ = s.recvfrom(65535)
        except OSError:
            return ''
    return data[len(PREFIX):].decode('latin-1')


def wait_for(condition, seconds=5.0):
    deadline = time.time() + seconds
    while time.time() < deadline:
        if condition():
            return True
        time.sleep(0.25)
    return False


def check_client(port, protocol, password, game_log, check):
    """Connects a minimal client that receives the gamestate, enters the game
    and sends commands, including one message split into fragments, so the
    netchan, the XOR keys, Huffman decompression, SV_ClientCommand and
    SV_UserMove are all used as a player uses them."""
    reply = rcon(port, password, 'sv_serverid')
    m = re.search(r'"sv_serverid" is: ?"(-?\d+)', reply)
    check('rcon reply', m is not None, reply.strip().replace('\n', ' ')[:60])
    if not m:
        return
    server_id = int(m.group(1))

    def listed(name):
        return name in (players(port) or {})

    client = cod2client.Client(port)
    try:
        try:
            refused = client.connect(protocol, 'smokeclient')
        except OSError as e:
            refused = f'no reply ({e})'
        check('client connects', refused is None, refused or '')
        if refused is not None:
            return
        check('client listed', wait_for(lambda: listed('smokeclient')))

        got = client.request_gamestate(server_id)
        check('client receives the gamestate', got,
              f'client {client.client_num}, {client.messages_received} messages' if got else '')
        if not got:
            return

        # stock and server-only iwds must be refused, any other loaded iwd sent
        refused = b'EXE_CANTAUTODLGAMEIWD'
        reply = client.download(server_id, 'main/iw_00.iwd')
        check('stock iwd download refused', refused in reply)
        reply = client.download(server_id, 'main/zz_svr_smoke.iwd')
        check('server-only iwd download refused', refused in reply)
        reply = client.download(server_id, 'main/zz_smoke_download.iwd')
        check('mod iwd download starts', refused not in reply and b'PK\x03\x04' in reply)

        client.enter_game(server_id, 2.0)
        ping = (players(port) or {}).get('smokeclient')
        check('client enters the game', ping is not None and 0 <= ping < 999, f'ping {ping}')

        # every server message must parse up to its svc_EOF; a spectator sees
        # the map's entities, and an rcon say arrives as a server command
        rcon(port, password, 'say smokesnap')
        frames = client.read_frames(1.5)
        entities = max((len(f['entities']) for f in frames), default=0)
        check('client parses snapshots', frames and entities and client.parsed_clean == client.parsed,
              f'{len(frames)} snapshots, {client.parsed_clean} of {client.parsed} messages clean, '
              f'{entities} entities' + (f', {client.parse_errors[0]}' if client.parse_errors else ''))
        check('client receives server commands', any('smokesnap' in c for f in frames for c in f['commands']))

        client.command(server_id, 'userinfo "\\name\\smokerenamed\\rate\\25000\\snaps\\20"')
        check('client command runs', wait_for(lambda: listed('smokerenamed')),
              ', '.join(players(port) or {}))

        # three userinfo commands of high-entropy text do not fit one packet
        rng = random.Random(1)
        texts = ['userinfo "\\name\\smokefragment%d\\rate\\25000\\snaps\\20\\pad\\%s"'
                 % (i, ''.join(rng.choice(string.ascii_letters + string.digits) for _ in range(850)))
                 for i in range(3)]
        packets = client.commands(server_id, texts)
        check('fragmented client message runs', packets > 1 and wait_for(lambda: listed('smokefragment2')),
              f'{packets} packets')

        # line breaks in a name or a chat message must not start a fake log
        # line, and a say right after a userinfo must not be flood protected
        client.commands(server_id, ['userinfo "\\name\\smokelog\nfakename\\rate\\25000\\snaps\\20"',
                                    'say "smokesay\nK;fakekill"'])
        def game_log_lines():
            if not os.path.exists(game_log):
                return []
            # each line starts with the time, as in " 1:23 say;..."
            return [re.sub(r'^\s*\d+:\d+ ', '', l)
                    for l in open(game_log, encoding='latin-1').read().splitlines()]
        said = wait_for(lambda: any('smokesay' in l for l in game_log_lines()))
        lines = game_log_lines()
        check('say right after userinfo runs', said)
        check('no line break in names', wait_for(lambda: listed('smokelogfakename')), ', '.join(players(port) or {}))
        check('no fake lines in the game log', said and not any(
            l.startswith(('fakename', 'K;fake')) for l in lines),
            next((l for l in lines if 'smokesay' in l), ''))

        # g_forceRate replaces the rate the client asks for
        rcon(port, password, 'g_forceRate 12345')
        client.command(server_id, 'userinfo "\\name\\smokerate\\rate\\25000\\snaps\\20"')
        def forced():
            return any('smokerate' in l and l.rstrip().endswith('12345')
                       for l in rcon(port, password, 'status').splitlines())
        check('g_forceRate applies', wait_for(forced))
        rcon(port, password, 'g_forceRate 0')
    finally:
        client.close()

    # a wrong rcon password must not hold up the right one
    rcon(port, 'wrong' + password, 'sv_serverid')
    check('rcon right after a wrong password', 'sv_serverid' in rcon(port, password, 'sv_serverid'))

    rcon(port, password, 'sv_logRcon 0')
    rcon(port, password, 'set smokehidden 1')
    rcon(port, password, 'sv_logRcon 1')

    # addip must keep the address out; an over-long filter must not crash
    rcon(port, password, 'addip ' + '1' * 300)
    rcon(port, password, 'addip 127.0.*.*')
    banned = cod2client.Client(port)
    try:
        refused = banned.connect(protocol, 'smokebanned')
    except OSError as e:
        refused = f'no reply ({e})'
    finally:
        banned.close()
    check('addip refuses the address', refused is not None and 'banned' in refused,
          (refused or '').replace('\n', ' ').strip())
    rcon(port, password, 'removeip 127.0.*.*')


class FakeMaster:
    """A UDP socket on 127.0.0.1 standing in for a master server: it keeps
    what the server sends and can query the server from the master's port."""

    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('127.0.0.1', 0))
        self.port = self.sock.getsockname()[1]
        self.packets = []

    def receive(self, condition, seconds):
        """Waits until a packet from the server satisfies condition(text) and returns it."""
        deadline = time.time() + seconds
        while time.time() < deadline:
            self.sock.settimeout(max(0.05, deadline - time.time()))
            try:
                data, addr = self.sock.recvfrom(65535)
            except OSError:
                continue
            if not data.startswith(PREFIX):
                continue
            text = data[len(PREFIX):].decode('latin-1')
            self.packets.append((addr, text))
            if condition(text):
                return addr, text
        return None

    def send(self, port, command):
        self.sock.sendto(PREFIX + command.encode(), ('127.0.0.1', port))

    def close(self):
        self.sock.close()


def check_master(args, check):
    """Internet mode (dedicated 2) against a fake master on 127.0.0.1: the
    heartbeats, the status sent on its own, the replies to the master's
    queries and the flatline on quit. The real masters and the authorize
    server are left empty or pointed at an unused local port, so nothing
    leaves the machine."""
    port = free_port()
    password = secrets.token_hex(8)
    master = FakeMaster()
    unused = free_port()
    hostname = 'cod2rev master test'
    server = None
    with tempfile.TemporaryDirectory(prefix='cod2rev_master_') as home:
        log_path = os.path.join(home, 'server.log')
        try:
            with open(log_path, 'wb') as log:
                server = subprocess.Popen(
                    [args.binary,
                     '+set', 'fs_basepath', os.path.abspath(args.game_dir),
                     '+set', 'fs_homepath', home,
                     '+set', 'dedicated', '2',
                     '+set', 'net_port', str(port),
                     '+set', 'sv_hostname', hostname,
                     '+set', 'rcon_password', password,
                     '+set', 'sv_master1', f'127.0.0.1:{master.port}',
                     '+set', 'sv_master2', '',
                     '+set', 'sv_master3', '',
                     '+set', 'sv_master4', '',
                     '+set', 'sv_master5', '',
                     '+set', 'sv_authorizeServer', '127.0.0.1',
                     '+set', 'sv_authorizePort', str(unused),
                     '+set', 'g_gametype', 'tdm',
                     '+map', args.map],
                    stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT)

            beat = master.receive(lambda t: t.startswith('heartbeat '), args.timeout)
            check('heartbeat sent to sv_master1 (host:port)', beat is not None and beat[1] == 'heartbeat COD-2\n',
                  repr(beat[1]) if beat else 'nothing received')
            if not beat:
                return
            server_addr = beat[0]
            check('heartbeat comes from net_port', server_addr[1] == port, f'from port {server_addr[1]}')

            status = master.receive(lambda t: t.startswith('statusResponse'), 10)
            check('status sent to the master on its own', status is not None and f'\\mapname\\{args.map}' in status[1])

            master.send(port, 'getinfo mastercheck')
            info = master.receive(lambda t: t.startswith('infoResponse'), 5)
            check('master getinfo reply', info is not None and '\\challenge\\mastercheck' in info[1]
                  and f'\\hostname\\{hostname}' in info[1])

            master.send(port, 'getstatus mastercheck')
            status = master.receive(lambda t: t.startswith('statusResponse') and 'mastercheck' in t, 5)
            check('master getstatus reply', status is not None and '\\sv_maxclients\\' in status[1])

            # "heartbeat" sends one at once instead of after HEARTBEAT_MSEC
            rcon(port, password, 'heartbeat')
            check('heartbeat command sends one now', master.receive(lambda t: t.startswith('heartbeat '), 5) is not None)

            # SIGTERM quits as the quit command does
            server.send_signal(signal.SIGTERM)
            flat = master.receive(lambda t: t.startswith('heartbeat '), 10)
            check('flatline heartbeat on SIGTERM', flat is not None and flat[1] == 'heartbeat flatline\n',
                  repr(flat[1]) if flat else 'nothing received')
            try:
                server.wait(timeout=10)
            except subprocess.TimeoutExpired:
                pass
            check('server quit on SIGTERM with exit code 0', server.poll() == 0, f'exit code {server.poll()}')
            # g_logSync is 0, so this line only reaches the file if the log was closed
            game_log = os.path.join(home, 'main', 'games_mp.log')
            game_text = open(game_log, encoding='latin-1').read() if os.path.exists(game_log) else ''
            check('games_mp.log closed on SIGTERM', 'ShutdownGame:' in game_text)

            log_text = open(log_path, encoding='latin-1').read()
            others = sorted(set(re.findall(r'Sending heartbeat to (\S+)', log_text)) - {f'127.0.0.1:{master.port}'})
            check('no heartbeat to any other master', not others, ', '.join(others))
        finally:
            master.close()
            if server and server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    server.kill()


def check_console_log(path, check):
    """logfileName, logfileRotate and logTimestamps: the previous log is kept
    as .1, and every line of the new one starts with the date and time."""
    rotated = path + '.1'
    check('console log rotated', os.path.exists(rotated) and 'previous run' in open(rotated).read())
    text = open(path, encoding='latin-1').read() if os.path.exists(path) else ''
    lines = [l for l in text.split('\n') if l]
    stamped = [l for l in lines if re.match(r'\d{4}-\d\d-\d\d \d\d:\d\d:\d\d ', l)]
    check('console log lines have timestamps', lines and len(stamped) == len(lines),
          f'{len(stamped)} of {len(lines)}' + next((f': {l[:60]}' for l in lines if l not in stamped), ''))
    check('sv_logRcon 0 hides rcon commands', 'Rcon from' in text and 'smokehidden' not in text)


def free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir', help='game folder that contains "main" with the .iwd files')
    ap.add_argument('--binary', default=os.path.join(ROOT, 'bin', 'cod2rev_lnxded'))
    ap.add_argument('--protocol', type=int, help='protocol the build must report (115 or 118)')
    ap.add_argument('--map', default='mp_toujane')
    ap.add_argument('--timeout', type=int, default=120, help='seconds to wait for the map to load')
    args = ap.parse_args()

    if not os.path.isdir(os.path.join(args.game_dir, 'main')):
        sys.exit(f'{args.game_dir} has no "main" folder')
    if not os.path.isfile(args.binary):
        sys.exit(f'{args.binary} not found, build it first with "make"')

    port = free_port()
    hostname = 'cod2rev smoke test'
    password = secrets.token_hex(8)
    failures = []

    with tempfile.TemporaryDirectory(prefix='cod2rev_smoke_') as home:
        # two iwds for the download checks: one server-only, one a client may get
        os.makedirs(os.path.join(home, 'main'))
        for name in ('zz_svr_smoke.iwd', 'zz_smoke_download.iwd'):
            with zipfile.ZipFile(os.path.join(home, 'main', name), 'w') as z:
                z.writestr('smoke/%s.txt' % name, 'cod2rev smoke test\n')
        # an older console log, which logfileRotate must keep as .1
        console_log = os.path.join(home, 'main', 'smoke_console.log')
        with open(console_log, 'w') as f:
            f.write('previous run\n')
        log_path = os.path.join(home, 'server.log')
        with open(log_path, 'wb') as log:
            server = subprocess.Popen(
                [args.binary,
                 '+set', 'fs_basepath', os.path.abspath(args.game_dir),
                 '+set', 'fs_homepath', home,
                 '+set', 'dedicated', '1',
                 '+set', 'net_port', str(port),
                 '+set', 'sv_cracked', '1',
                 '+set', 'sv_hostname', hostname,
                 '+set', 'rcon_password', password,
                 '+set', 'g_gametype', 'tdm',
                 '+set', 'g_logSync', '1',
                 '+set', 'logfile', '2',
                 '+set', 'logfileName', 'smoke_console.log',
                 '+set', 'logfileRotate', '2',
                 '+set', 'logTimestamps', '1',
                 '+map', args.map],
                stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT)
        try:
            info = None
            deadline = time.time() + args.timeout
            while time.time() < deadline and server.poll() is None:
                info = query(port, 'getinfo smoke')
                if info and info[1].get('mapname') == args.map:
                    break
                info = None
                time.sleep(1)

            def check(name, ok, detail=''):
                print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''))
                if not ok:
                    failures.append(name)

            check('server started and loaded the map', info is not None,
                  f'{args.map} on port {port}' if info else f'exit code {server.poll()}')
            if info:
                header, values = info
                check('getinfo reply', header == 'infoResponse' and values.get('challenge') == 'smoke')
                check('hostname', values.get('hostname') == hostname, values.get('hostname', ''))
                protocol = values.get('protocol', '')
                if args.protocol:
                    check('protocol', protocol == str(args.protocol), f'reports {protocol}')
                else:
                    print(f'     protocol {protocol}')

                status = query(port, 'getstatus smoke')
                check('getstatus reply', bool(status) and status[0] == 'statusResponse'
                      and status[1].get('mapname') == args.map,
                      f'version {status[1].get("shortversion", "?")}' if status else '')

                if protocol.isdigit():
                    check_client(port, int(protocol), password, os.path.join(home, 'main', 'games_mp.log'), check)

                time.sleep(2)
                check('server still running', server.poll() is None)

            if info:
                check_console_log(console_log, check)

            # a crash must leave its report in crash.log
            if server.poll() is None:
                server.send_signal(signal.SIGSEGV)
                try:
                    server.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    pass
                crash_log = os.path.join(home, 'crash.log')
                report = open(crash_log, encoding='latin-1').read() if os.path.exists(crash_log) else ''
                check('crash report written', 'Server crash: SIGSEGV' in report and 'addresses:' in report)

            log_text = open(log_path, encoding='latin-1').read()
            check('no heartbeat sent in LAN mode', 'Sending heartbeat' not in log_text)
            if failures:
                print('\n--- last lines of the server log ---')
                print('\n'.join(log_text.splitlines()[-25:]))
        finally:
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    server.kill()

    check_master(args, check)

    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
