#!/usr/bin/env python3
"""Checks the Plus ID: the connect key pair proof (docs/architecture/plus-id.md).

    python3 scripts/plusid_test.py /path/to/CoD2 [--binary bin/cod2rev_lnxded] [--protocol 118]

Minimal clients sign the connect challenge with keys made by the openssl CLI
and the server's verdict is read back with rcon plusIdStatus. The folder must
hold only the stock iwds. Exits with 0 when every check passes. The helpers
(make_key, sign, connect_with_id...) are for the other Plus ID tests.
"""
import argparse, base64, functools, hashlib, http.server, json, os, re, secrets, subprocess, sys, tempfile, threading, time, zipfile

import banlist, cod2client, smoke_test

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RCON = 'plusid_rcon'
# the P-256 group order: a private key is 1..N-1
N = 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
NOTICE = 'This server requires a Plus ID'
GAMETYPE = r"""main()
{
	maps\mp\gametypes\tdm::main();
	level.plusidConnect = level.callbackPlayerConnect;
	level.callbackPlayerConnect = ::connected;
}

connected()
{
	logPrint("plusid " + self getPlusId() + "\n");
	logPrint("plusidflags " + self getPlusIdFlags() + "\n");
	logPrint("plusidtime " + self getPlusIdPlaytime() + " " + self getPlusIdFirstSeen() + "\n");
	self [[level.plusidConnect]]();
}
"""


def make_key():
    """A random private key, 32 bytes big-endian."""
    while True:
        d = int.from_bytes(secrets.token_bytes(32), 'big')
        if 0 < d < N:
            return d.to_bytes(32, 'big')


def _der_key(priv):
    """priv as an SEC1 ECPrivateKey on prime256v1, which openssl reads."""
    return bytes.fromhex('30310201010420') + priv + bytes.fromhex('a00a06082a8648ce3d030107')


def _openssl(args, priv, data=b''):
    with tempfile.TemporaryDirectory(prefix='plusid_key_') as tmp:
        key = os.path.join(tmp, 'key.der')
        with open(key, 'wb') as f:
            f.write(_der_key(priv))
        return subprocess.run(['openssl'] + args + [key], input=data, capture_output=True, check=True).stdout


def public_key(priv):
    """The compressed public point, 33 bytes."""
    spki = _openssl(['pkey', '-inform', 'DER', '-pubout', '-outform', 'DER', '-ec_conv_form', 'compressed', '-in'], priv)
    return spki[-33:]


def plus_id(pk):
    """P- and the base32 of the first 10 bytes of SHA-256(pk), in groups of 4."""
    b = base64.b32encode(hashlib.sha256(pk).digest()[:10]).decode()
    return 'P-' + '-'.join(b[i:i + 4] for i in range(0, 16, 4))


def signed_text(challenge, adr):
    return f'cod2plus-id-v1|{challenge}|{adr}'.encode()


def _der_int(der, i):
    assert der[i] == 2
    n = der[i + 1]
    return int.from_bytes(der[i + 2:i + 2 + n], 'big'), i + 2 + n


def sign(priv, challenge, adr):
    """(pk hex, ps hex): the public key and the raw r||s signature of the signed text."""
    der = _openssl(['dgst', '-sha256', '-keyform', 'DER', '-sign'], priv, signed_text(challenge, adr))
    # SEQUENCE { INTEGER r, INTEGER s }, short lengths for P-256
    r, i = _der_int(der, 2)
    s, _ = _der_int(der, i)
    return public_key(priv).hex(), (r.to_bytes(32, 'big') + s.to_bytes(32, 'big')).hex()


def _private(ip):
    """PlusId_IsPrivate: RFC 1918, loopback and link-local only (not the host's
    own /24, which Sys_IsLANAddress also takes)."""
    a = [int(x) for x in ip.split('.')]
    return (a[0] in (10, 127) or a[:2] in ([169, 254], [192, 168])
            or (a[0] == 172 and a[1] & 0xF0 == 16))


def address_accepted(signed_ip, from_ip, public, net_ip='', signed_port=28960, net_port=28960):
    """The server's rule (PlusId_AddressAccepted) for the address the client
    signed, branch for branch: the server's port; a private address only from
    a private source, even when it is net_ip; then the public address or net_ip;
    then any private address."""
    if signed_port != net_port:
        return False
    if _private(signed_ip) and not _private(from_ip):
        return False
    if signed_ip in (public, net_ip) and signed_ip not in ('', '0.0.0.0'):
        return True
    return _private(signed_ip)


def id_keys(pk_hex, ps_hex, adr):
    return f'\\pk\\{pk_hex}\\ps\\{ps_hex}\\pa\\{adr}'


def connect_with_id(port, priv, adr=None, name='plusid', protocol=115, plus=123):
    """Connects a client that signs adr (by default 127.0.0.1:port) with priv;
    returns (client, None or the server's reply). The caller closes the client."""
    adr = adr or f'127.0.0.1:{port}'
    client = cod2client.Client(port)
    reply = client.connect(protocol, name, plus, lambda challenge: id_keys(*sign(priv, challenge, adr), adr))
    return client, reply


def status_rows(port):
    """{name: (id, playtime, first_seen_ago)} from rcon plusIdStatus, whose lines are
    "num name id playtime first_seen_ago flags"."""
    lines = smoke_test.rcon(port, RCON, 'plusIdStatus').splitlines()
    rows = (re.match(r'\s*\d+ (.*) (\S+) (\d+) (\d+) \S+$', l) for l in lines[1:])
    return {m.group(1): (m.group(2), int(m.group(3)), int(m.group(4))) for m in rows if m}


def status_extra(port, name):
    """(client number, flags) of a player in rcon plusIdStatus; flags is "-" for none."""
    for l in smoke_test.rcon(port, RCON, 'plusIdStatus').splitlines()[1:]:
        m = re.match(r'\s*(\d+) (.*) \S+ \d+ \d+ (\S+)$', l)
        if m and m.group(2) == name:
            return int(m.group(1)), m.group(3)
    return None, None


def status(port):
    """{name: id} from rcon plusIdStatus."""
    return {name: row[0] for name, row in status_rows(port).items()}


def seen_file(path):
    """{id: (first_seen, seconds)} of a seen file."""
    if not os.path.exists(path):
        return {}
    return {f[0]: (int(f[1]), int(f[2])) for f in (l.split() for l in open(path).read().splitlines())}


def launch(binary, game_dir, home, port, map_name, log_path, extra=()):
    """Starts a dedicated server with the test gametype; returns the process."""
    command = [os.path.abspath(binary), '+set', 'fs_basepath', os.path.abspath(game_dir),
               '+set', 'fs_homepath', home, '+set', 'net_port', str(port), '+set', 'sv_cracked', '1',
               '+set', 'rcon_password', RCON, '+set', 'dedicated', '1', '+set', 'sv_maxclients', '16',
               '+set', 'g_gametype', 'plusid', '+set', 'g_log', 'games_mp.log', '+set', 'g_logSync', '1',
               *extra, '+map', map_name]
    with open(log_path, 'wb') as log:
        return subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, cwd=home)


def make_home(home):
    os.makedirs(os.path.join(home, 'main'))
    with zipfile.ZipFile(os.path.join(home, 'main', 'zz_svr_plusid.iwd'), 'w') as z:
        z.writestr('maps/mp/gametypes/plusid.gsc', GAMETYPE)


def stop(server):
    if server.poll() is None:
        server.terminate()
        try:
            server.wait(timeout=15)
        except subprocess.TimeoutExpired:
            server.kill()


def check_shared_seen_file(args, check):
    """Two servers with one seen file and the same ID connected 5 s to each."""
    with tempfile.TemporaryDirectory(prefix='cod2_plusid_a_') as a, tempfile.TemporaryDirectory(prefix='cod2_plusid_b_') as b, \
            tempfile.TemporaryDirectory(prefix='cod2_plusid_seen_') as shared:
        seen = os.path.join(shared, 'seen.txt')
        servers, clients = [], []
        try:
            ports = [smoke_test.free_port(), smoke_test.free_port()]
            for home, port in zip((a, b), ports):
                make_home(home)
                servers.append(launch(args.binary, args.game_dir, home, port, args.map, os.path.join(home, 'out.log'),
                                      ['+set', 'sv_plusIdSeenFile', seen]))
            up = all(smoke_test.wait_for(lambda p=p, s=s: s.poll() is not None or smoke_test.query(p, 'getinfo x'), 150)
                     and s.poll() is None for p, s in zip(ports, servers))
            check('shared servers run', up)
            if not up:
                return
            priv = make_key()
            for port in ports:
                client, reply = connect_with_id(port, priv, None, 'shared', args.protocol)
                clients.append(client)
            time.sleep(5)
            # one second apart: simultaneous saves are the documented shortcut
            smoke_test.rcon(ports[0], RCON, 'kick shared')
            time.sleep(1)
            smoke_test.rcon(ports[1], RCON, 'kick shared')
            want = plus_id(public_key(priv))
            smoke_test.wait_for(lambda: seen_file(seen).get(want, (0, 0))[1] >= 9, 10)
            row = seen_file(seen).get(want)
            check('shared_seen_file', row is not None and row[1] >= 8, str(row))

            # the shutdown save: a player still connected when the server quits
            priv2 = make_key()
            client, _ = connect_with_id(ports[0], priv2, None, 'quitter', args.protocol)
            clients.append(client)
            time.sleep(3)
            smoke_test.rcon(ports[0], RCON, 'quit')
            want2 = plus_id(public_key(priv2))
            smoke_test.wait_for(lambda: seen_file(seen).get(want2, (0, 0))[1] >= 2, 15)
            check('shutdown_save', seen_file(seen).get(want2, (0, 0))[1] >= 2, str(seen_file(seen)))
            leftovers = [f for f in os.listdir(shared) if f != 'seen.txt']
            check('no_tmp_left', not leftovers, str(leftovers))
        finally:
            for client in clients:
                client.close()
            for server in servers:
                stop(server)


def check_address_rule(check):
    # (signed, from, sv_publicAddress, net_ip, signed port, expected)
    table = [('10.0.0.5', '203.0.113.9', '203.0.113.7', '', 28960, False),  # relay from a public address
             ('10.0.0.5', '192.168.1.2', '', '', 28960, True),
             ('127.0.0.1', '127.0.0.1', '', '', 28960, True),
             ('203.0.113.7', '198.51.100.1', '203.0.113.7', '', 28960, True),
             ('203.0.113.7', '198.51.100.1', '', '203.0.113.7', 28960, True),
             ('203.0.113.8', '198.51.100.1', '203.0.113.7', '', 28960, False),
             ('203.0.113.8', '127.0.0.1', '203.0.113.7', '', 28960, False),
             ('203.0.113.7', '198.51.100.1', '203.0.113.7', '', 28961, False),  # another port
             # private net_ip relayed from a public address
             ('192.168.1.5', '198.51.100.1', '', '192.168.1.5', 28960, False),
             # a public neighbour in the server's /24 (a shared VPN) relaying its own signed IP
             ('203.0.113.50', '203.0.113.50', '', '203.0.113.7', 28960, False),
             ('26.10.0.9', '26.10.0.20', '', '26.10.0.5', 28960, False)]
    bad = [row for row in table if address_accepted(*row[:4], signed_port=row[4]) != row[5]]
    check('address_rule', not bad, str(bad))


def run_cases(port, protocol, log_path, game_log, seen_path, check):
    clients = []

    def join(name, priv=None, adr=None, extra=None):
        if priv:
            client, reply = connect_with_id(port, priv, adr, name, protocol)
        else:
            client = cod2client.Client(port)
            reply = client.connect(protocol, name, 123, extra)
        clients.append(client)
        return reply

    try:
        priv = make_key()
        want = plus_id(public_key(priv))
        reply = join('valid', priv)
        got = status(port).get('valid')
        check('valid_id', reply is None and got == want, f'{reply} {got} {want}')
        smoke_test.wait_for(lambda: any(l == 'plusid ' + want for l in smoke_test.log_lines(game_log)))
        check('getplusid', 'plusid ' + want in smoke_test.log_lines(game_log))

        reply = join('noid')
        check('no_id', reply is None and status(port).get('noid') == '-', f'{reply} {status(port)}')
        check('no_id_playtime', status_rows(port)['noid'][1:] == (0, 0)
              and smoke_test.wait_for(lambda: 'plusidtime 0 0' in smoke_test.log_lines(game_log)), str(status_rows(port)))

        # a player kept 5 s is counted, saved at the drop, and keeps first_seen when back
        time.sleep(5)
        row = status_rows(port)['valid']
        check('playtime', row[1] >= 4, str(row))
        smoke_test.rcon(port, RCON, 'kick valid')
        smoke_test.wait_for(lambda: want in seen_file(seen_path), 5)
        first, seconds = seen_file(seen_path).get(want, (0, 0))
        check('playtime_saved', 4 <= seconds <= 8 and first > 0, str(seen_file(seen_path)))
        time.sleep(2)
        join('valid', priv)
        row = status_rows(port).get('valid', ('', 0, 0))
        check('first_seen', row[1] >= 4 and row[2] >= 2 and seen_file(seen_path).get(want, (0,))[0] == first, f'{row} {first}')
        check('getplusidplaytime', smoke_test.wait_for(lambda: any(
            re.match(r'plusidtime ([4-9]|\d\d+) ([2-9]|\d\d+)$', l) for l in smoke_test.log_lines(game_log))))

        def tampered(challenge):
            adr = f'127.0.0.1:{port}'
            pk, ps = sign(make_key(), challenge, adr)
            return id_keys(pk, ps[:-1] + '%x' % (int(ps[-1], 16) ^ 1), adr)
        reply = join('badsig', extra=tampered)
        logged = 'plusid: invalid signature' in open(log_path, encoding='latin-1').read()
        check('bad_signature', reply is None and status(port).get('badsig') == '-' and logged, f'{reply} {logged}')

        dump = smoke_test.rcon(port, RCON, 'dumpuser valid')
        check('stripped', re.search(r'^name +valid$', dump, re.M) and not re.search(r'^(pk|ps|pa) ', dump, re.M), dump[-300:])

        smoke_test.rcon(port, RCON, f'set sv_publicAddress 203.0.113.7:{port}')
        # both private: accepted; the real relay (from a public address) is in address_rule
        priv = make_key()
        reply = join('relay', priv, f'10.9.9.9:{port}')
        check('relay', reply is None and status(port).get('relay') == plus_id(public_key(priv)), f'{reply} {status(port)}')

        # the new sv_publicAddress is resolved on the first connect after it changed
        priv = make_key()
        reply = join('public', priv, f'203.0.113.7:{port}')
        check('public_match', reply is None and status(port).get('public') == plus_id(public_key(priv)), f'{reply} {status(port)}')

        reply = join('mismatch', make_key(), f'203.0.113.8:{port}')
        logged = f'signed for another address, 203.0.113.8:{port}' in open(log_path, encoding='latin-1').read()
        check('public_mismatch', reply is None and status(port).get('mismatch') == '-' and logged,
              f'{reply} {logged} {status(port)}')
        smoke_test.rcon(port, RCON, 'set sv_publicAddress ""')

        smoke_test.rcon(port, RCON, 'set sv_requirePlusId 1')
        reply = join('required')
        check('require', reply is not None and reply.startswith('error') and NOTICE in reply, str(reply))
        reply = join('requiredok', make_key())
        check('require with id', reply is None, str(reply))
        smoke_test.rcon(port, RCON, 'set sv_requirePlusId 0')
    finally:
        for client in clients:
            client.close()


def parse_time(text):
    import calendar
    return calendar.timegm(time.strptime(text, '%Y-%m-%dT%H:%M:%SZ'))


def read_bans(path):
    return json.load(open(path))['entries'] if os.path.exists(path) else []


def run_ban_cases(port, protocol, log_path, game_log, bans_path, check):
    clients = []

    def join(name, priv):
        client, reply = connect_with_id(port, priv, None, name, protocol)
        clients.append(client)
        return reply

    def gone(name):
        return smoke_test.wait_for(lambda: name not in status(port), 5)

    try:
        priv = make_key()
        want = plus_id(public_key(priv))
        join('byid', priv)
        smoke_test.rcon(port, RCON, f'banPlusId {want} 0 cheating')
        reply = join('byid', priv)
        check('ban_by_id', gone('byid') and reply is not None and reply.startswith('error') and 'cheating' in reply
              and 'byid' not in status(port), str(reply))
        smoke_test.rcon(port, RCON, f'unbanPlusId {want}')
        reply = join('byid', priv)
        check('unban', reply is None and status(port).get('byid') == want and not read_bans(bans_path), str(reply))

        priv = make_key()
        want = plus_id(public_key(priv))
        join('byslot', priv)
        num, _ = status_extra(port, 'byslot')
        smoke_test.rcon(port, RCON, f'banPlusId {num} 1 test')
        entry = ([b for b in read_bans(bans_path) if b['id'] == want] or [{}])[0]
        left = parse_time(entry.get('expires', '')) - time.time() if re.match(r'\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ$', entry.get('expires', '')) else 0
        check('ban_by_slot', gone('byslot') and entry.get('ip') == '127.0.0.1' and 'guid' in entry
              and 50 <= left <= 61 and entry.get('date') == time.strftime('%Y-%m-%d', time.gmtime())
              and entry.get('by') == 'rcon' and entry.get('reason') == 'test', f'{entry} {left}')

        # edited by hand: the file is read again when its time changes
        time.sleep(1.1)
        bans = json.load(open(bans_path))
        bans['entries'][0]['expires'] = time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime(time.time() - 5))
        json.dump(bans, open(bans_path, 'w'))
        reply = join('byslot', priv)
        check('expiry', reply is None and status(port).get('byslot') == want, str(reply))
        smoke_test.rcon(port, RCON, f'unbanPlusId {want}')

        # a ban by slot keeps the IP, so another ID from it is marked and stays
        priv = make_key()
        join('banned', priv)
        smoke_test.rcon(port, RCON, f'banPlusId {status_extra(port, "banned")[0]} 0 evasion')
        priv2 = make_key()
        reply = join('evader', priv2)
        flags = status_extra(port, 'evader')[1]
        logged = f'plusid: possible evasion {plus_id(public_key(priv2))} matches {plus_id(public_key(priv))} by ' in open(log_path, encoding='latin-1').read()
        check('evasion_flag', reply is None and flags in ('ip', 'guid,ip') and logged
              and smoke_test.wait_for(lambda: 'plusidflags ' + flags in smoke_test.log_lines(game_log)), f'{reply} {flags} {logged}')
        check('no_flag_without_ban', status_extra(port, 'byslot')[1] in (None, '-'), str(status_extra(port, 'byslot')))

        # a hostile file keeps the bans in memory: the banned ID is still refused
        time.sleep(1.1)
        before = len(read_bans(bans_path))
        open(bans_path, 'w').write('{"entries":[{"id":"nope"}]}')
        reply = join('banned', priv)
        check('bad_file_keeps_bans', before == 1 and reply is not None and reply.startswith('error') and 'evasion' in reply, str(reply))

        # and nothing is written over it, not even by a new ban
        bad = open(bans_path, 'rb').read()
        out = smoke_test.rcon(port, RCON, f'banPlusId {plus_id(public_key(make_key()))} 0 lost')
        out2 = smoke_test.rcon(port, RCON, 'unbanPlusId P-AAAA-AAAA-AAAA-AAAA')
        check('bad_file_no_write', open(bans_path, 'rb').read() == bad and 'fix it first' in out and 'fix it first' in out2, f'{out} {out2}')

        # a file that reads again clears the state; a date format that is not the spec's does not read
        time.sleep(1.1)
        open(bans_path, 'w').write(json.dumps({'entries': [{'id': plus_id(public_key(make_key())), 'expires': 1794178571}]}))
        out = smoke_test.rcon(port, RCON, 'unbanPlusId P-AAAA-AAAA-AAAA-AAAA')
        check('bad_date_rejected', 'fix it first' in out, out)
        time.sleep(1.1)
        open(bans_path, 'w').write(json.dumps({'entries': [{'id': plus_id(public_key(make_key())), 'date': '2026-11-01', 'expires': None}]}))
        out = smoke_test.rcon(port, RCON, 'unbanPlusId P-AAAA-AAAA-AAAA-AAAA')
        check('good_file_clears', 'fix it first' not in out and 'is not banned' in out, out)
    finally:
        for client in clients:
            client.close()


def check_lists(args, check):
    """Shared ban lists served by a local HTTP server, refreshed every minute:
    five lists checked at the same refresh, then a restart with the HTTP
    server down."""
    with tempfile.TemporaryDirectory(prefix='cod2_plusid_lists_') as home, \
            tempfile.TemporaryDirectory(prefix='cod2_plusid_www_') as www, \
            tempfile.TemporaryDirectory(prefix='cod2_plusid_maint_') as maint:
        make_home(home)
        key = os.path.join(maint, 'maint')
        banlist.keygen(key)
        pub = banlist.read(key + '.pub', 'r').strip()
        requests = []
        handler = functools.partial(type('Quiet', (http.server.SimpleHTTPRequestHandler,), {'log_message': lambda self, *a: requests.append(self.path)}),
                                    directory=www)
        # a refused body closes the connection early, which is no error here
        httpd = type('QuietServer', (http.server.ThreadingHTTPServer,), {'handle_error': lambda *a: None})(('127.0.0.1', 0), handler)
        threading.Thread(target=httpd.serve_forever, daemon=True).start()
        base = f'http://127.0.0.1:{httpd.server_address[1]}/'
        names = ['ban', 'badsig', 'rollback', 'big', 'kick']
        ids = {}

        def publish(name, entries, updated='2026-11-08T12:00:00Z', pad=0, good=True):
            path = os.path.join(www, name + '.json')
            doc = {'format': 1, 'name': 'List ' + name, 'appeal': 'https://example.com/appeal', 'updated': updated,
                   'entries': [{'id': i, 'reason': 'aimbot', 'date': '2026-11-01', 'expires': None} for i in entries]}
            with open(path, 'w') as f:
                f.write(json.dumps(doc) + ' ' * pad)
            banlist.sign(path, key + '.key')
            if not good:
                with open(path, 'a') as f:
                    f.write(' ')

        keys = {n: make_key() for n in names + ['added', 'rolled']}
        ids = {n: plus_id(public_key(k)) for n, k in keys.items()}
        for n in names:
            publish(n, [ids[n]] if n != 'kick' else [])
        lists = ';'.join(f'{base}{n}.json|{pub}' for n in names) + f';{base}x.json|zz'
        port = smoke_test.free_port()
        log_path = os.path.join(home, 'out.log')
        extra = ['+set', 'sv_banLists', f'"{lists}"', '+set', 'sv_banListRefresh', '1', '+set', 'sv_httpAllowPrivate', '1']
        clients = []

        def join(name):
            client, reply = connect_with_id(port, keys[name], None, name, args.protocol)
            clients.append(client)
            return reply

        def state(name):
            for line in smoke_test.rcon(port, RCON, 'banListStatus').splitlines():
                if f'{base}{name}.json' in line:
                    return line
            return ''

        def log():
            return open(log_path, encoding='latin-1').read()

        server = launch(args.binary, args.game_dir, home, port, args.map, log_path, extra)
        try:
            up = smoke_test.wait_for(lambda: server.poll() is not None or smoke_test.query(port, 'getinfo x'), 150) and server.poll() is None
            check('lists server runs', up)
            if not up:
                return
            check('list_bad_key_skipped', smoke_test.wait_for(lambda: f'plusid: malformed key for list {base}x.json' in log(), 10), log()[-500:])
            check('lists_fetched', smoke_test.wait_for(lambda: all(' ok' in state(n) for n in names), 15),
                  smoke_test.rcon(port, RCON, 'banListStatus'))
            reply = join('ban')
            check('list_ban', reply is not None and 'You are banned by List ban: aimbot. Appeal: https://example.com/appeal' in reply, str(reply))
            cached = os.listdir(os.path.join(home, 'plusid_lists')) if os.path.isdir(os.path.join(home, 'plusid_lists')) else []
            check('list_cache_written', len(cached) == 2 * len(names), str(cached))
            join('kick')
            check('list kick player in', 'kick' in status(port), str(status(port)))

            publish('badsig', [ids['added']], '2026-11-09T00:00:00Z', good=False)
            publish('rollback', [ids['rolled']], '2026-11-07T00:00:00Z')
            publish('big', [ids['big']], '2026-11-09T00:00:00Z', pad=4 * 1024 * 1024)
            publish('kick', [ids['kick']], '2026-11-09T00:00:00Z')
            # the next refresh is at most a minute away
            kicked = smoke_test.wait_for(lambda: 'kick' not in status(port), 75)
            check('list_kicks_connected', kicked and 'plusid: dropping banned ' + ids['kick'] in log()
                  and 'List kick' in log(), log()[-800:])
            smoke_test.wait_for(lambda: 'signature' in state('badsig') and 'older' in state('rollback') and 'large' in state('big'), 15)
            reply = join('badsig')
            check('bad_sig_keeps_last', 'bad signature' in state('badsig') and reply is not None and 'List badsig' in reply
                  and join('added') is None, f'{state("badsig")} {reply}')
            check('rollback', 'older' in state('rollback') and join('rolled') is None and join('rollback') is not None, state('rollback'))
            check('too_big', 'large' in state('big') and join('big') is not None, state('big'))
            # the list that did not change was fetched again but not taken again
            smoke_test.wait_for(lambda: requests.count('/ban.json') >= 2, 15)
            adopted = log().count(f'plusid: list List ban ({base}ban.json) updated')
            check('unchanged_not_adopted', requests.count('/ban.json') >= 2 and adopted == 1 and ' ok' in state('ban'),
                  f'{requests.count("/ban.json")} fetches, {adopted} adopted, {state("ban")}')
        finally:
            for client in clients:
                client.close()
            clients.clear()
            stop(server)
            httpd.shutdown()
            httpd.server_close()

        # down: the cached copies apply and the log says so
        open(log_path, 'w').close()
        server = launch(args.binary, args.game_dir, home, port, args.map, log_path, extra)
        try:
            up = smoke_test.wait_for(lambda: server.poll() is not None or smoke_test.query(port, 'getinfo x'), 150) and server.poll() is None
            smoke_test.wait_for(lambda: 'keeping the copy of 2026-11-08T12:00:00Z' in log(), 15)
            reply = join('ban') if up else None
            check('list_cached_when_offline', up and reply is not None and 'List ban' in reply and 'from the cache' in log()
                  and 'keeping the copy of 2026-11-08T12:00:00Z' in log(), log()[-800:])
            check('the lists server still runs', server.poll() is None, f'exit code {server.poll()}')
        finally:
            for client in clients:
                client.close()
            stop(server)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--binary', default=os.path.join(ROOT, 'bin', 'cod2rev_lnxded'))
    ap.add_argument('--protocol', type=int, default=115)
    ap.add_argument('--map', default='mp_toujane')
    args = ap.parse_args()
    port = smoke_test.free_port()
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    check_address_rule(check)
    with tempfile.TemporaryDirectory(prefix='cod2_plusid_') as home:
        make_home(home)
        log_path = os.path.join(home, 'out.log')
        server = launch(args.binary, args.game_dir, home, port, args.map, log_path)
        try:
            smoke_test.wait_for(lambda: server.poll() is not None or smoke_test.query(port, 'getinfo x'), 150)
            up = server.poll() is None and smoke_test.query(port, 'getinfo x') is not None
            check('the server runs the map', up)
            if up:
                check('lan warning', 'plusid: no sv_publicAddress, only LAN players get a Plus ID'
                      in open(log_path, encoding='latin-1').read())
                run_cases(port, args.protocol, log_path, os.path.join(home, 'main', 'games_mp.log'),
                          os.path.join(home, 'plusid_seen.txt'), check)
                run_ban_cases(port, args.protocol, log_path, os.path.join(home, 'main', 'games_mp.log'),
                              os.path.join(home, 'plusid_bans.json'), check)
            check('the server still runs', server.poll() is None, f'exit code {server.poll()}')
        finally:
            stop(server)
        if failures:
            print('\n--- last lines of the server log ---')
            print(open(log_path, encoding='latin-1').read()[-3000:])
    check_shared_seen_file(args, check)
    check_lists(args, check)
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
