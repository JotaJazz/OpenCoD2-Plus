#!/usr/bin/env python3
"""Starts the Linux server against a real game folder and checks that it answers.

    python3 scripts/smoke_test.py /path/to/CoD2 [--protocol 118] [--plus] [--binary bin/cod2rev_lnxded]

--plus checks a `make PLUS=1` build: stock and other Plus clients are told to
update, and a Plus client gets Plus snapshots.

Besides the queries, a minimal client (cod2client.py) connects, receives the
gamestate, enters the game and sends commands, one of them in a fragmented
message. The game folder is only read: logs and configs go to a temporary home
folder. The server runs in LAN mode (dedicated 1), so nothing is sent to the
master servers; a second run in internet mode (dedicated 2) heartbeats to a fake
master on 127.0.0.1 instead of the real ones. Exits with 0 when every check
passes.
"""
import argparse, os, random, re, secrets, signal, socket, string, subprocess, sys, tempfile, threading, time, zipfile

import cod2client

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PREFIX = b'\xff\xff\xff\xff'
SMOKE_GAMETYPE = r"""main()
{
	origin = getentarray("mp_tdm_spawn", "classname")[0].origin;
	precacheModel("xmodel/prop_barrel_benzin");
	model = spawn("script_model", origin);
	model setModel("xmodel/prop_barrel_benzin");
	trace = bulletTrace(origin + (0, 0, 100), origin - (0, 0, 10), false, undefined);
	if (isDefined(trace["entity"]) && trace["entity"] == model)
		setCvar("smoke_trace", "hit");
	maps\mp\gametypes\tdm::main();
}
"""


def log_lines(path, timed=False):
    """A log's lines, without the time each one starts with, as in " 1:23 K;..."."""
    if not os.path.exists(path):
        return []
    lines = open(path, encoding='latin-1').read().splitlines()
    return lines if timed else [re.sub(r'^\s*\d+:\d+ ', '', l) for l in lines]


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


def check_client(port, protocol, plus, password, game_log, check):
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

        # each donedl answers with a full gamestate, so a flood of them is capped
        before = client.state.gamestate_count
        client.commands(server_id, ['donedl'] * 40)
        for sequence, data in client.receive(2.0):
            client._parse(sequence, data)
        sent = client.state.gamestate_count - before
        check('donedl flood gets a few gamestates', 1 <= sent <= 5, f'{sent} of 40')

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
        check('a client gets the snapshots of its version', frames and all(f['plus'] == plus for f in frames))
        check('client receives server commands', any('smokesnap' in c for f in frames for c in f['commands']))

        # someone behind the same IP who guesses the qport must not take the
        # client's address, but a client whose port changes keeps playing
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as thief:
            header = (0x7ffffff0).to_bytes(4, 'little') + client.qport.to_bytes(2, 'little')
            thief.sendto(header + bytes(16), ('127.0.0.1', port))
            thief.sendto(PREFIX + b'v\0' + client.qport.to_bytes(2, 'little') + bytes(8), ('127.0.0.1', port))
            time.sleep(0.2)
        client.command(server_id, 'userinfo "\\name\\smokeqport\\rate\\25000\\snaps\\20"')
        check('a guessed qport keeps the client', wait_for(lambda: listed('smokeqport')))
        moved = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        client.sock.close()
        client.sock = moved
        client.command(server_id, 'userinfo "\\name\\smokemoved\\rate\\25000\\snaps\\20"')
        check('a translated port keeps the client', wait_for(lambda: listed('smokemoved')))

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

        # one message runs at most 64 commands, the rest when they come again
        first = client.command_sequence + 1
        cap = 'userinfo "\\name\\smokecap%s\\rate\\25000\\snaps\\20"'
        client.commands(server_id, [cap % 'a'] * 99 + [cap % 'b'])
        capped = wait_for(lambda: listed('smokecapa')) and not listed('smokecapb')
        check('one message runs at most 64 commands', capped, ', '.join(players(port) or {}))
        client.resend(server_id, first + 64)
        check('the rest run when sent again', wait_for(lambda: listed('smokecapb')))

        # line breaks in a name or a chat message must not start a fake log
        # line, and a say right after a userinfo must not be flood protected
        client.commands(server_id, ['userinfo "\\name\\smokelog\nfakename\\rate\\25000\\snaps\\20"',
                                    'say "smokesay\nK;fakekill"'])
        said = wait_for(lambda: any('smokesay' in l for l in log_lines(game_log)))
        lines = log_lines(game_log)
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
    queries and the flatline when SIGTERM makes it quit, and a console log
    that can't be opened (warned about once, nothing rotated). The real masters and the authorize
    server are left empty or pointed at an unused local port, so nothing
    leaves the machine."""
    port = free_port()
    password = secrets.token_hex(8)
    master = FakeMaster()
    unused = free_port()
    alt_port = 0 if args.protocol == 118 else free_port()   # net_altPort exists only in the 1.0 build
    hostname = 'cod2rev master test'
    server = None
    with tempfile.TemporaryDirectory(prefix='cod2rev_master_') as home:
        log_path = os.path.join(home, 'server.log')
        # a console log that can't be opened, inside a regular file
        os.makedirs(os.path.join(home, 'main'))
        not_a_dir = os.path.join(home, 'main', 'notadir')
        with open(not_a_dir, 'w') as f:
            f.write('a file\n')
        try:
            with open(log_path, 'wb') as log:
                server = subprocess.Popen(
                    [args.binary,
                     '+set', 'fs_basepath', os.path.abspath(args.game_dir),
                     '+set', 'fs_homepath', home,
                     '+set', 'logfile', '1',
                     '+set', 'logfileName', 'notadir/console.log',
                     '+set', 'logfileRotate', '2',
                     '+set', 'dedicated', '2',
                     '+set', 'net_port', str(port),
                     '+set', 'net_altPort', str(alt_port),
                     '+set', 'sv_cracked', '1',
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
            if alt_port:
                beat = master.receive(lambda t: t.startswith('heartbeat '), 5)
                check('second heartbeat comes from net_altPort', beat is not None and beat[0][1] == alt_port,
                      f'from port {beat[0][1]}' if beat else 'nothing received')

            status = master.receive(lambda t: t.startswith('statusResponse'), 10)
            check('status sent to the master on its own', status is not None and f'\\mapname\\{args.map}' in status[1])

            master.send(port, 'getinfo mastercheck')
            info = master.receive(lambda t: t.startswith('infoResponse'), 5)
            check('master getinfo reply', info is not None and '\\challenge\\mastercheck' in info[1]
                  and f'\\hostname\\{hostname}' in info[1])

            master.send(port, 'getstatus mastercheck')
            status = master.receive(lambda t: t.startswith('statusResponse') and 'mastercheck' in t, 5)
            check('master getstatus reply', status is not None and '\\sv_maxclients\\' in status[1])

            if alt_port:
                master.send(alt_port, 'getinfo altcheck')
                info = master.receive(lambda t: t.startswith('infoResponse') and 'altcheck' in t, 5)
                check('net_altPort announces protocol 118', info is not None and info[0][1] == alt_port
                      and '\\protocol\\118\\' in info[1])
                master.send(alt_port, 'getstatus altcheck')
                status = master.receive(lambda t: t.startswith('statusResponse') and 'altcheck' in t, 5)
                check('net_altPort status is 1.3', status is not None and status[0][1] == alt_port
                      and '\\shortversion\\1.3\\' in status[1] and '\\protocol\\118\\' in status[1])
                master.send(port, 'getinfo maincheck')
                info = master.receive(lambda t: t.startswith('infoResponse') and 'maincheck' in t, 5)
                check('net_port still announces its own protocol', info is not None and '\\protocol\\115\\' in info[1])
                check_alt_client(alt_port, password, port, check)

            # "heartbeat" sends one at once instead of after HEARTBEAT_MSEC
            rcon(port, password, 'heartbeat')
            check('heartbeat command sends one now', master.receive(lambda t: t.startswith('heartbeat '), 5) is not None)
            if alt_port:
                check('heartbeat command sends one from net_altPort', master.receive(lambda t: t.startswith('heartbeat '), 5) is not None)

            # SIGTERM quits as the quit command does
            server.send_signal(signal.SIGTERM)
            flat = master.receive(lambda t: t.startswith('heartbeat '), 10)
            check('flatline heartbeat on SIGTERM', flat is not None and flat[1] == 'heartbeat flatline\n',
                  repr(flat[1]) if flat else 'nothing received')
            if alt_port:
                flat = master.receive(lambda t: t.startswith('heartbeat '), 5)
                check('flatline heartbeat from net_altPort', flat is not None and flat[0][1] == alt_port
                      and flat[1] == 'heartbeat flatline\n', repr(flat[1]) if flat else 'nothing received')
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
            failed = log_text.count("couldn't open logfile notadir/console.log")
            check('failed console log open reported once', failed == 1 and 'notadir/console.log opened' not in log_text,
                  f'{failed} warnings')
            check('failed console log open rotates nothing', open(not_a_dir).read() == 'a file\n'
                  and not os.path.exists(not_a_dir + '.1'))
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


def check_vote_limit(port, protocol, password, check):
    """A player calls at most three votes a map. Each vote kicks a client of
    its own, so they pass at once and the voter stays."""
    server_id = int(re.search(r'"sv_serverid" is: ?"(-?\d+)', rcon(port, password, 'sv_serverid')).group(1))
    voter = cod2client.Client(port)
    targets = []
    try:
        if voter.connect(protocol, 'smokevoter') is not None or not voter.request_gamestate(server_id):
            check('a player calls at most three votes', False, 'voter did not join')
            return
        voter.enter_game(server_id, 1.0)
        # join a team: spectators cannot call votes
        menus = {name: i for i, name in ((i, voter.state.configstrings.get(1246 + i)) for i in range(32)) if name}
        team = next(name for name in menus if name.startswith('team_'))
        voter.command(server_id, f'mr {server_id} {menus[team]} autoassign')
        voter.read_frames(1.0)
        weapons = {'american': 'm1garand_mp', 'british': 'enfield_mp', 'russian': 'mosin_nagant_mp', 'german': 'kar98k_mp'}
        for name, index in menus.items():
            if name.startswith('weapon_') and name[7:] in weapons:
                voter.command(server_id, f'mr {server_id} {index} {weapons[name[7:]]}')
                voter.read_frames(1.0)  # flood protection: one command each 800 ms
        for n in range(4):
            target = cod2client.Client(port)
            targets.append(target)
            target.connect(protocol, f'smoketarget{n}')
            target.request_gamestate(server_id)
            target.enter_game(server_id, 0.3)
        status = rcon(port, password, 'status')
        nums = [int(m.group(1)) for m in re.finditer(r'^\s*(\d+)\s+\S+\s+\S+\s+\S+\s+smoketarget', status, re.M)]
        replies = []
        for num in nums:
            voter.command(server_id, f'callvote clientkick {num}')
            # wait for the answer and, for a vote called, for it to pass, so
            # the next one does not find it in progress
            reply, deadline = '', time.time() + 4.0
            while time.time() < deadline:
                seen = [c for f in voter.read_frames(0.3) for c in f['commands'] if 'GAME_' in c]
                reply = reply or next((c for c in seen if 'GAME_CALLEDAVOTE' in c or 'GAME_MAXVOTES' in c
                                       or 'GAME_VOTEALREADY' in c or 'GAME_NOSPECTATOR' in c), '')
                if 'GAME_CALLEDAVOTE' not in reply and reply or any('GAME_VOTEPASSED' in c for c in seen):
                    break
            replies.append(reply)
            time.sleep(0.9)  # flood protection: one command each 800 ms
        called = sum('GAME_CALLEDAVOTE' in r for r in replies)
        check('a player calls at most three votes', len(nums) == 4 and called == 3 and 'GAME_MAXVOTESCALLED' in replies[3],
              f'{called} called, then {replies[3][:30] if len(replies) == 4 else replies}' if called == 3
              else f'{len(nums)} targets, replies {[r[:30] for r in replies]}')
    finally:
        voter.close()
        for target in targets:
            target.close()


def check_client_noise(port, protocol, password, console_log, check):
    """A client cannot flood the menu commands, which skip flood protection,
    nor the log with malformed packets. developer 1 puts the debug prints of
    the commands ignored in the console log."""
    server_id = int(re.search(r'"sv_serverid" is: ?"(-?\d+)', rcon(port, password, 'sv_serverid')).group(1))
    rcon(port, password, 'developer 1')
    client = cod2client.Client(port)
    try:
        if client.connect(protocol, 'smokenoise') is not None or not client.request_gamestate(server_id):
            check('menu commands are limited', False, 'client did not join')
            return
        client.enter_game(server_id, 1.0)
        before = len([l for l in log_lines(console_log) if 'menu command ignored' in l])
        for _ in range(3):
            client.commands(server_id, [f'mr {server_id} 0 smokenoise'] * 30)
        client.read_frames(1.0)
        ignored = len([l for l in log_lines(console_log) if 'menu command ignored' in l]) - before
        check('menu commands are limited', ignored >= 50, f'{ignored} of 90 ignored')

        before = len([l for l in log_lines(console_log) if 'bad command byte' in l])
        for _ in range(40):
            payload = cod2client.MsgWriter()
            payload.bits(5, 3)  # no such client message
            client._send(server_id, payload)
        client.read_frames(1.0)
        printed = len([l for l in log_lines(console_log) if 'bad command byte' in l]) - before
        check('bad packets are not printed each time', 1 <= printed <= 15, f'{printed} of 40 printed')

        # the client starts its chat with 0x15; 0x14 there would make the
        # text one of the game's localized messages
        client.read_frames(1.0)  # flood protection: one command each 800 ms
        client.command(server_id, 'say "\x14GAME_VOTEPASSED"')
        chat = [c for f in client.read_frames(1.0) for c in f['commands'] if 'GAME_VOTEPASSED' in c]
        check('chat cannot show the game\'s messages', chat and all(': ^7.GAME_VOTEPASSED' in c for c in chat),
              repr(chat[:1]))
    finally:
        client.close()
        rcon(port, password, 'developer 0')


def check_signed_challenge(port, protocol, password, check):
    """A challenge is signed for its address, so a client whose challenge a
    flood of other addresses pushed out of the 1024-slot table still joins.
    With net_lanauthorize 1 and a fake authorize server, that client also has
    to wait for the authorize reply, which a connect asks for again."""
    authorize = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    authorize.bind(('127.0.0.1', 0))
    authorize.settimeout(0.2)
    rcon(port, password, f'sv_authorizeServer 127.0.0.1')
    rcon(port, password, f'sv_authorizePort {authorize.getsockname()[1]}')
    rcon(port, password, 'net_lanauthorize 1')
    asked = []

    def answer_authorize(seconds):
        """Accepts the requests for 127.0.0.1; the flood's are left unanswered."""
        deadline = time.time() + seconds
        while time.time() < deadline:
            try:
                data, addr = authorize.recvfrom(65535)
            except OSError:
                continue
            parts = data[len(PREFIX):].decode('latin-1').split()
            if parts[:1] == ['getIpAuthorize'] and parts[2] == '127.0.0.1':
                asked.append(int(parts[1]))
                authorize.sendto(PREFIX + f'ipAuthorize {parts[1]} accept KEY_IS_GOOD 4242'.encode(), addr)

    client = cod2client.Client(port)
    flood = []
    try:
        client.sock.sendto(PREFIX + b'getchallenge', client.addr)
        answer_authorize(1.0)
        line = client._oob('getchallenge', 'challengeResponse')
        challenge = int(line.split()[1]) if line.startswith('challengeResponse') else None
        check('an authorized client gets a challenge', challenge is not None and asked, line.strip())
        if challenge is None:
            return

        # push it out of the table: 1100 other addresses, under the global
        # limit of 100 a second
        for n in range(1100):
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.bind(('127.0.0.3', 0))
            s.sendto(PREFIX + b'getchallenge', ('127.0.0.1', port))
            flood.append(s)
            if n % 9 == 8:
                time.sleep(0.1)
        time.sleep(0.5)

        userinfo = ('\\protocol\\%d\\challenge\\%d\\qport\\%d\\name\\smokesigned\\rate\\25000\\snaps\\20\\plus\\123'
                    % (protocol, challenge, client.qport))
        connect = PREFIX + f'connect "{userinfo}"'.encode('latin-1')
        client.sock.sendto(connect, client.addr)
        client.sock.settimeout(1.0)
        try:
            first = client.sock.recvfrom(65535)[0][len(PREFIX):].decode('latin-1')
        except OSError:
            first = None
        asked_before = len(asked)
        answer_authorize(1.0)
        check('a signed challenge out of the table waits for authorize',
              first is None and len(asked) > asked_before and asked[-1] == challenge, repr(first))
        client.protocol = protocol
        reply = client._oob(f'connect "{userinfo}"', 'connectResponse')
        check('a signed challenge out of the table joins', reply.startswith('connectResponse'), reply.strip())
    except OSError as e:
        check('a signed challenge out of the table joins', False, str(e))
    finally:
        for s in flood:
            s.close()
        client.close()
        authorize.close()
        rcon(port, password, 'net_lanauthorize 0')


def check_stateless_challenge(port, protocol, password, check):
    """With sv_challengeStateless 1 getchallenge answers at once with the signed
    challenge, takes no slot and has no global limit, so a burst from other
    addresses doesn't keep a player out; the connect asks the authorize server."""
    authorize = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    authorize.bind(('127.0.0.1', 0))
    authorize.settimeout(0.2)
    rcon(port, password, 'sv_authorizeServer 127.0.0.1')
    rcon(port, password, f'sv_authorizePort {authorize.getsockname()[1]}')
    rcon(port, password, 'net_lanauthorize 1')
    rcon(port, password, 'sv_challengeStateless 1')
    asked = []

    def answer_authorize(seconds):
        deadline = time.time() + seconds
        while time.time() < deadline:
            try:
                data, addr = authorize.recvfrom(65535)
            except OSError:
                continue
            parts = data[len(PREFIX):].decode('latin-1').split()
            if parts[:1] == ['getIpAuthorize'] and parts[2] == '127.0.0.1':
                asked.append(int(parts[1]))
                authorize.sendto(PREFIX + f'ipAuthorize {parts[1]} accept KEY_IS_GOOD 4243'.encode(), addr)

    client = cod2client.Client(port)
    try:
        # a flood of 400 a second, over what the stateful path lets through,
        # while the client asks as a real one does, again until answered
        stop = threading.Event()

        def flood():
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.bind(('127.0.0.3', 0))
                while not stop.is_set():
                    for _ in range(20):
                        s.sendto(PREFIX + b'getchallenge', ('127.0.0.1', port))
                    time.sleep(0.05)

        flooder = threading.Thread(target=flood)
        flooder.start()
        time.sleep(1.0)
        client.sock.settimeout(1.0)
        line = 'no reply'
        try:
            for _ in range(3):
                try:
                    line = client._oob('getchallenge', 'challengeResponse')
                    break
                except OSError:
                    pass
        finally:
            stop.set()
            flooder.join()
        answer_authorize(0.3)
        challenge = int(line.split()[1]) if line.startswith('challengeResponse') else None
        check('a stateless challenge comes through a flood, before authorize',
              challenge is not None and not asked, line.strip())
        if challenge is None:
            return

        userinfo = ('\\protocol\\%d\\challenge\\%d\\qport\\%d\\name\\smokestateless\\rate\\25000\\snaps\\20\\plus\\123'
                    % (protocol, challenge, client.qport))
        client.sock.sendto(PREFIX + f'connect "{userinfo}"'.encode('latin-1'), client.addr)
        try:
            first = client.sock.recvfrom(65535)[0][len(PREFIX):].decode('latin-1')
        except OSError:
            first = None
        answer_authorize(1.0)
        check('a stateless connect waits for authorize', first is None and asked == [challenge], f'{first!r} {asked}')
        client.protocol = protocol
        reply = client._oob(f'connect "{userinfo}"', 'connectResponse')
        check('a stateless connect joins after authorize', reply.startswith('connectResponse'), reply.strip())
    except OSError as e:
        check('a stateless connect joins after authorize', False, str(e))
    finally:
        client.close()
        authorize.close()
        rcon(port, password, 'sv_challengeStateless 0')
        rcon(port, password, 'net_lanauthorize 0')


def check_query_limits(port, check):
    """getinfo and getstatus have global limits of their own: a burst of
    getstatus must not silence getinfo, which browsers use, and getinfo has
    room for 100 a second. 127.0.0.1 has no per-address limit."""
    time.sleep(1.5)  # both buckets refill
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.settimeout(0.5)
        for _ in range(40):
            s.sendto(PREFIX + b'getstatus x', ('127.0.0.1', port))
        s.sendto(PREFIX + b'getinfo afterstatus', ('127.0.0.1', port))
        statuses, info = 0, False
        try:
            while True:
                data, _ = s.recvfrom(65535)
                statuses += data[len(PREFIX):].startswith(b'statusResponse')
                info = info or data[len(PREFIX):].startswith(b'infoResponse')
        except OSError:
            pass
    check('getinfo answers right after a getstatus burst', info, f'{statuses} of 40 getstatus answered')

    time.sleep(1.5)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.settimeout(0.5)
        for _ in range(50):
            s.sendto(PREFIX + b'getinfo x', ('127.0.0.1', port))
        infos = 0
        try:
            while True:
                s.recvfrom(65535)
                infos += 1
        except OSError:
            pass
    check('getinfo has room for a burst', infos >= 45, f'{infos} of 50')


def check_challenge_during_queries(port, check):
    """A burst of queries, which share a low global limit, must not stop a player joining."""
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        for _ in range(50):
            s.sendto(PREFIX + b'getinfo x', ('127.0.0.1', port))
    reply = query(port, 'getchallenge', timeout=0.09)

    check('getchallenge survives a query burst', reply is not None and reply[0].startswith('challengeResponse'))


def check_alt_client(alt_port, password, port, check):
    """A 1.3 client joins through net_altPort. Its socket only hears that port,
    so a reply from net_port would not reach it."""
    server_id = int(re.search(r'"sv_serverid" is: ?"(-?\d+)', rcon(port, password, 'sv_serverid')).group(1))
    client = cod2client.Client(alt_port)
    client.sock.connect(client.addr)
    try:
        refused = client.connect(118, 'smokealt')
        check('1.3 client connects through net_altPort', refused is None, refused or '')
        if refused is not None:
            return
        got = client.request_gamestate(server_id)
        check('net_altPort client receives the gamestate', got)
        if not got:
            return
        client.enter_game(server_id, 1.0)
        frames = client.read_frames(1.0)
        check('net_altPort client parses snapshots', frames and client.parsed_clean == client.parsed,
              f'{len(frames)} snapshots, {client.parsed_clean} of {client.parsed} messages clean')
    finally:
        client.close()


def check_connect_limit(port, protocol, plus, check):
    """A local proxy's players, all from 127.0.0.1, get in together; a flood from another address is cut."""
    joined = []
    for i in range(12):
        client = cod2client.Client(port)
        joined.append(client)
        try:
            refused = client.connect(protocol, f'smokejoin{i}')
        except OSError as e:
            refused = f'no reply ({e})'
        if refused is not None:
            break
    check('clients joining together connect', refused is None, refused or '')
    for client in joined:
        client.close()

    if plus:
        plain, old, current = cod2client.Client(port), cod2client.Client(port), cod2client.Client(port)
        try:
            refused = plain.connect(protocol, 'stock', plus=None)
            check('a stock client is told to update', refused is not None and 'OpenCoD2-Plus' in refused, str(refused))
            refused = old.connect(protocol, 'old', plus=120)
            check('another Plus version is told to update', refused is not None and 'OpenCoD2-Plus' in refused, str(refused))
            refused = current.connect(protocol, 'plus')
            check('a Plus client is told the version', refused is None and current.connect_reply.split()[1:] == ['123'],
                  refused or current.connect_reply)
        finally:
            plain.close()
            old.close()
            current.close()

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.bind(('127.0.0.2', 0))
        s.settimeout(0.5)
        for _ in range(200):
            s.sendto(PREFIX + b'connect "\\protocol\\%d\\challenge\\1\\qport\\7\\name\\x"' % protocol,
                     ('127.0.0.1', port))
        replies = 0
        try:
            while True:
                s.recvfrom(65535)
                replies += 1
        except OSError:
            pass
    check('connect flood is limited', replies <= 15, f'{replies} of 200')


def check_game_log_rotation(path, check):
    """g_logRotateSize: the oversized games_mp.log was kept as .1 and a new one started."""
    rotated = path + '.1'
    check('games_mp.log rotated by size', os.path.exists(rotated)
          and open(rotated).read(14) == 'old games log\n'
          and os.path.exists(path) and 'old games log' not in open(path, encoding='latin-1').read())


def check_strict_port(args, port, check):
    """net_strictPort 1: a second server on the busy port stops instead of moving to the next one."""
    with tempfile.TemporaryDirectory(prefix='cod2rev_strict_') as home:
        server = subprocess.Popen(
            [args.binary,
             '+set', 'fs_basepath', os.path.abspath(args.game_dir),
             '+set', 'fs_homepath', home,
             '+set', 'dedicated', '1',
             '+set', 'net_port', str(port),
             '+set', 'net_strictPort', '1',
             '+set', 'logfile', '2'],
            stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        try:
            out = server.communicate(timeout=30)[0].decode('latin-1')
        except subprocess.TimeoutExpired:
            server.kill()
            out = server.communicate()[0].decode('latin-1')
        check('net_strictPort stops on a busy port', server.returncode not in (None, 0) and 'net_strictPort' in out,
              f'exit code {server.returncode}')
        log_path = os.path.join(home, 'main', 'console_mp_server.log')
        log = open(log_path, encoding='latin-1').read() if os.path.exists(log_path) else ''
        check('fatal error reaches the log', 'FATAL' in log and 'net_strictPort' in log)


def free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir', help='game folder that contains "main" with the .iwd files')
    ap.add_argument('--binary', default=os.path.join(ROOT, 'bin', 'cod2rev_lnxded'))
    ap.add_argument('--protocol', type=int, help='protocol the build must report (115 or 118)')
    ap.add_argument('--plus', action='store_true', help='the binary is a make PLUS=1 build (implies --protocol 118)')
    ap.add_argument('--map', default='mp_toujane')
    ap.add_argument('--timeout', type=int, default=120, help='seconds to wait for the map to load')
    args = ap.parse_args()
    if args.plus:
        args.protocol = 118

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
        # a gametype that traces into a model before the first frame, when the
        # skeleton memory used to be unset and the server crashed
        with zipfile.ZipFile(os.path.join(home, 'main', 'zz_svr_smoke.iwd'), 'a') as z:
            z.writestr('maps/mp/gametypes/smoke.gsc', SMOKE_GAMETYPE)
        # an older console log, which logfileRotate must keep as .1
        console_log = os.path.join(home, 'main', 'smoke_console.log')
        with open(console_log, 'w') as f:
            f.write('previous run\n')
        # a games_mp.log over g_logRotateSize, which must move to .1 at the map start
        game_log = os.path.join(home, 'main', 'games_mp.log')
        with open(game_log, 'w') as f:
            f.write('old games log\n' + 'x' * (1024 * 1024) + '\n')
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
                 '+set', 'g_gametype', 'smoke',
                 '+set', 'g_logSync', '1',
                 '+set', 'logfile', '2',
                 '+set', 'logfileName', 'smoke_console.log',
                 '+set', 'logfileRotate', '2',
                 '+set', 'logTimestamps', '1',
                 '+set', 'g_logRotate', '2',
                 '+set', 'g_logRotateSize', '1',
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
                check('bulletTrace into a model at the map start',
                      re.search(r'"smoke_trace" is: ?"hit', rcon(port, password, 'smoke_trace')) is not None)
                check('getinfo reply', header == 'infoResponse' and values.get('challenge') == 'smoke')
                check('over-long getinfo challenge ignored', query(port, 'getinfo ' + 'x' * 1000, 1.0) is None)
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
                    check_client(port, int(protocol), args.plus, password, os.path.join(home, 'main', 'games_mp.log'), check)
                    check_vote_limit(port, int(protocol), password, check)
                    check_client_noise(port, int(protocol), password, console_log, check)
                    check_connect_limit(port, int(protocol), args.plus, check)
                    check_query_limits(port, check)
                    check_signed_challenge(port, int(protocol), password, check)
                    check_stateless_challenge(port, int(protocol), password, check)
                    check_challenge_during_queries(port, check)

                # manymaps must not take a map name that leaves its library
                os.makedirs(os.path.join(home, 'library'))
                open(os.path.join(home, 'smoke_escape.iwd'), 'wb').close()
                rcon(port, password, 'fs_library ' + os.path.join(home, 'library'))
                reply = rcon(port, password, 'map ../smoke_escape')
                rcon(port, password, 'fs_library ""')
                escaped = os.path.join(os.path.dirname(home), 'smoke_escape.iwd')
                linked = os.path.lexists(escaped)
                if linked:
                    os.remove(escaped)
                check('manymaps refuses ../ in a map name', not linked and 'NEW LINK' not in reply,
                      reply.strip().replace('\n', ' ')[:80])


                time.sleep(2)
                check('server still running', server.poll() is None)

            if info:
                check_console_log(console_log, check)
                check_game_log_rotation(game_log, check)
                check_strict_port(args, port, check)

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
