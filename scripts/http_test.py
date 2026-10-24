#!/usr/bin/env python3
"""Tests httpFetch and webSocket* against local HTTP, HTTPS, WS and WSS servers.

    python3 scripts/http_test.py /path/to/CoD2 [--binary bin/cod2rev_lnxded]

The servers run in this process (Python standard library only; the HTTPS and
WSS certificates are made with the openssl command). A test script is
installed as a mod in a temporary folder, next to the game's own
_callbacksetup.gsc with one line added; it writes HTTPTEST lines to
games_mp.log, which are checked here. While a slow request is in progress the
server must keep answering queries, and after a map change the old map's
requests and connections must be dropped. Exits with 0 when every check passes.
"""
import argparse, base64, hashlib, http.server, os, re, secrets, shutil, socket, socketserver, ssl, struct, \
    subprocess, sys, tempfile, threading, time

from install_bots import read_game_script
from smoke_test import free_port, query, rcon

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MOD = 'httptest'
CALLBACKS = 'maps/mp/gametypes/_callbacksetup.gsc'
MARKER = '[[level.callbackStartGameType]]();'
HOOK = 'thread maps\\mp\\gametypes\\_httptest::main();'
WS_GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11'

ERROR_CALLBACK = r'''
CodeCallback_Error(critical, function, message)
{
	logprint("HTTPERR " + message + "\n");
}
'''

TEST_SCRIPT = r'''
main()
{
	// the command line would take "//" for a comment, so the cvars hold host:port
	level.ht_http = "http://" + getCvar("ht_http");
	level.ht_https = "https://" + getCvar("ht_https");
	level.ht_ws = "ws://" + getCvar("ht_ws");
	level.ht_wss = "wss://" + getCvar("ht_wss");
	level.ht_ca = getCvar("ht_ca");
	level.ht_closed = getCvar("ht_closed");
	if(getCvar("ht_phase") == "2")
	{
		thread phase2();
		return;
	}
	if(getCvar("ht_phase") == "3")
	{
		thread phase3();
		return;
	}
	wait 1;
	logprint("HTTPTEST start\n");
	httpTests();
	wsTests();
	floodTests();
	stale("stale1", "phase1 done");
}

floodTests()
{
	level.burst_count = 0;
	level.burst_max = 0;
	level.burst_frame = 0;
	level.burst_time = -1;
	id = webSocketConnect(level.ht_ws + "/burst", "", ::floodNothing, ::burstMessage, ::floodClose, ::floodError, 0, 0);
	for(i = 0; level.burst_count < 200 && i < 200; i++)
		wait 0.05;
	log("burst count=" + level.burst_count + " max=" + level.burst_max);
	webSocketClose(id);
	wait 0.5;

	level.flood_count = 0;
	id = webSocketConnect(level.ht_ws + "/flood", "", ::floodNothing, ::floodMessage, ::floodClose, ::floodError, 0, 0);
	wait 3;
	log("flood count=" + level.flood_count);
	webSocketClose(id);
	wait 0.5;
}

floodNothing()
{
}

floodClose(closedByRemote, fullyDisconnected)
{
}

floodError(error)
{
	log("flood error " + error);
}

// counts the messages each frame delivers
burstMessage(text)
{
	t = getTime();
	if(t != level.burst_time)
	{
		level.burst_time = t;
		level.burst_frame = 0;
	}
	level.burst_frame++;
	if(level.burst_frame > level.burst_max)
		level.burst_max = level.burst_frame;
	level.burst_count++;
}

floodMessage(text)
{
	level.flood_count++;
}

log(text)
{
	logprint("HTTPTEST " + text + "\n");
}

fetch(name, url, method, data, headers, timeout, insecure)
{
	level.ht_pending = true;
	level.ht_name = name;
	level.ht_start = getTime();
	if(isDefined(insecure))
		httpFetch(url, method, data, headers, timeout, ::onDone, ::onError, insecure);
	else
		httpFetch(url, method, data, headers, timeout, ::onDone, ::onError);
	for(i = 0; level.ht_pending && i < 400; i++)
		wait 0.05;
	if(level.ht_pending)
		log(name + " hang");
}

onDone(status, body, headers)
{
	ct = "";
	if(isDefined(headers["content-type"]))
		ct = headers["content-type"];
	xt = "";
	if(isDefined(headers["x-test"]))
		xt = headers["x-test"];
	loc = "";
	if(isDefined(headers["location"]))
		loc = headers["location"];
	len = body.size;
	if(len > 200)
		body = "(long)";
	log(level.ht_name + " done " + status + " len=" + len + " ct=" + ct + " xt=" + xt + " loc=" + loc + " body=" + body);
	level.ht_pending = false;
}

onError(error)
{
	log(level.ht_name + " error ms=" + (getTime() - level.ht_start) + " " + error);
	level.ht_pending = false;
}

httpTests()
{
	fetch("json", level.ht_http + "/json", "GET", "", "", 10);
	fetch("echo", level.ht_http + "/echo?q=1", "POST", "payload", "X-Custom: hello\r\nContent-Type: text/plain\r\nX-Bad: x\nX-Injected: y\r\nHost: evil", 10);
	fetch("chunked", level.ht_http + "/chunked", "GET", "", "", 10);
	fetch("max", level.ht_http + "/max", "GET", "", "", 10);
	fetch("big", level.ht_http + "/big", "GET", "", "", 10);
	fetch("bigchunked", level.ht_http + "/bigchunked", "GET", "", "", 10);
	log("slow begin");
	fetch("slow", level.ht_http + "/slow", "GET", "", "", 2);
	fetch("notfound", level.ht_http + "/404", "GET", "", "", 10);
	fetch("redirect", level.ht_http + "/redirect", "GET", "", "", 10);
	fetch("eof", level.ht_http + "/eof", "GET", "", "", 10);
	raw = strTok("nulheader ctlheader manyheaders folded hexlength samelength twolengths continue endless", " ");
	for(i = 0; i < raw.size; i++)
		fetch(raw[i], level.ht_http + "/" + raw[i], "GET", "", "", 10);
	fetch("head", level.ht_http + "/json", "HEAD", "", "", 10);
	fetch("auth", "http://user:secret@" + getSubStr(level.ht_http, 7) + "/auth", "GET", "", "", 10);
	fetch("invalid", "ftp://example.com/", "GET", "", "", 10);
	fetch("badmethod", level.ht_http + "/json", "G ET", "", "", 10);
	fetch("refused", "http://127.0.0.1:" + level.ht_closed + "/", "GET", "", "", 10);
	setCvar("sv_httpAllowedHosts", "example.com, *.foo.org");
	fetch("notallowed", level.ht_http + "/json", "GET", "", "", 10);
	setCvar("sv_httpAllowedHosts", "example.com 127.0.0.1");
	fetch("allowed", level.ht_http + "/json", "GET", "", "", 10);
	setCvar("sv_httpAllowedHosts", "");
	// read once at startup: a script cannot turn it off (or on)
	setCvar("sv_httpAllowPrivate", "0");
	fetch("private_setcvar", level.ht_http + "/json", "GET", "", "", 10);

	fetch("https_noca", level.ht_https + "/json", "GET", "", "", 10);
	setCvar("sv_httpCaFile", level.ht_ca);
	fetch("https_ca", level.ht_https + "/json", "GET", "", "", 10);
	fetch("https_localhost", "https://localhost:" + getCvar("ht_https_port") + "/json", "GET", "", "", 10);
	setCvar("sv_httpCaFile", level.ht_ca + ".missing");
	fetch("https_badca", level.ht_https + "/json", "GET", "", "", 10);
	setCvar("sv_httpCaFile", "");
	fetch("https_insecure", level.ht_https + "/json", "GET", "", "", 10, 1);

	level.cap_done = 0;
	level.cap_errors = 0;
	level.cap_error = "";
	for(i = 0; i < 20; i++)
		httpFetch(level.ht_http + "/slow2", "GET", "", "", 10, ::capDone, ::capError);
	for(i = 0; level.cap_done + level.cap_errors < 20 && i < 300; i++)
		wait 0.05;
	log("cap done=" + level.cap_done + " errors=" + level.cap_errors + " " + level.cap_error);
}

capDone(status, body, headers)
{
	level.cap_done++;
}

capError(error)
{
	level.cap_errors++;
	level.cap_error = error;
}

wsConnect(tag, url, headers, reconnect, ping, insecure)
{
	level.ws_tag = tag;
	level.ws_count = 0;
	if(isDefined(insecure))
		id = webSocketConnect(url, headers, ::wsOpen, ::wsMessage, ::wsClose, ::wsError, reconnect, ping, insecure);
	else
		id = webSocketConnect(url, headers, ::wsOpen, ::wsMessage, ::wsClose, ::wsError, reconnect, ping);
	log(tag + " id " + id);
	return id;
}

wsWait(count)
{
	for(i = 0; level.ws_count < count && i < 200; i++)
		wait 0.05;
	if(level.ws_count < count)
		log(level.ws_tag + " hang at " + level.ws_count);
}

wsOpen()
{
	level.ws_count++;
	log(level.ws_tag + " " + level.ws_count + " open");
}

wsMessage(text)
{
	level.ws_count++;
	if(text.size > 200)
		text = "(" + text.size + " bytes)";
	log(level.ws_tag + " " + level.ws_count + " msg " + text);
}

wsClose(closedByRemote, fullyDisconnected)
{
	level.ws_count++;
	log(level.ws_tag + " " + level.ws_count + " close " + closedByRemote + " " + fullyDisconnected);
}

wsError(error)
{
	level.ws_count++;
	log(level.ws_tag + " " + level.ws_count + " error " + error);
}

wsTests()
{
	id = wsConnect("ws", level.ht_ws + "/echo", "X-Hello: there", 300, 0);
	wsWait(1);
	log("ws sent " + webSocketSendText(id, "hello"));
	wsWait(2);
	webSocketSendText(id, "frag");
	wsWait(3);
	webSocketSendText(id, "ping");
	wsWait(4);
	webSocketSendText(id, "big");
	webSocketSendText(id, "binary");
	wsWait(6);
	webSocketSendText(id, "max");
	wsWait(7);
	webSocketSendText(id, "close");
	wsWait(9);
	webSocketSendText(id, "drop");
	wsWait(11);
	webSocketSendText(id, "again");
	wsWait(12);
	log("ws closed " + webSocketClose(id));
	wsWait(13);
	wait 0.5;
	log("ws after close sent " + webSocketSendText(id, "x") + " closed " + webSocketClose(id));

	id = wsConnect("keepalive", level.ht_ws + "/echo", "", 0, 500);
	wsWait(1);
	wait 2;
	webSocketSendText(id, "still");
	wsWait(2);
	webSocketClose(id);
	wsWait(3);

	id = wsConnect("nopong", level.ht_ws + "/nopong", "", 0, 1000);
	wsWait(3);

	id = wsConnect("refused", "ws://127.0.0.1:" + level.ht_closed + "/", "", 0, 0);
	wsWait(1);
	wait 1;
	log("refused id after " + webSocketSendText(id, "x"));

	log("invalid id " + webSocketConnect("http://127.0.0.1/", "", ::wsOpen, ::wsMessage, ::wsClose, ::wsError));

	setCvar("sv_httpCaFile", level.ht_ca);
	id = wsConnect("wss", level.ht_wss + "/echo", "", 0, 0);
	wsWait(1);
	webSocketSendText(id, "hello");
	wsWait(2);
	webSocketClose(id);
	wsWait(3);
	setCvar("sv_httpCaFile", "");
	id = wsConnect("wss_noca", level.ht_wss + "/echo", "", 0, 0);
	wsWait(1);
	wait 1;
	id = wsConnect("wss_insecure", level.ht_wss + "/echo", "", 0, 0, 1);
	wsWait(1);
	webSocketClose(id);
	wsWait(2);
	wait 0.5;

	level.ws_tag = "cap";
	level.ws_count = 0;
	ids = [];
	for(i = 0; i < 16; i++)
		ids[i] = webSocketConnect(level.ht_ws + "/echo", "", ::wsOpen, ::wsMessage, ::wsClose, ::wsError, 0, 0);
	wsWait(16);
	text = "";
	for(i = 0; i < 16; i++)
		text = text + ids[i] + ",";
	log("cap ids " + text);
	thread connect17();
	wait 0.5;
	for(i = 0; i < 16; i++)
		webSocketClose(ids[i]);
	wsWait(32);
	wait 0.5;
	id = wsConnect("reuse", level.ht_ws + "/echo", "", 0, 0);
	wsWait(1);
	webSocketClose(id);
	wsWait(2);
}

connect17()
{
	log("cap 17 before");
	id = webSocketConnect(level.ht_ws + "/echo", "", ::wsOpen, ::wsMessage, ::wsClose, ::wsError, 0, 0);
	// the script error leaves id undefined
	log("cap 17 after " + isDefined(id));
}

// requests and connections the next level must not hear from
stale(tag, done)
{
	level.stale_tag = tag;
	httpFetch(level.ht_http + "/slow", "GET", "", "", 0, ::staleDone, ::staleError);
	webSocketConnect(level.ht_ws + "/stream", "", ::staleOpen, ::staleMessage, ::staleClose, ::staleError, 500, 0);
	wait 1;
	log(done);
}

staleDone(status, body, headers)
{
	log(level.stale_tag + " done " + status);
}

staleError(error)
{
	log(level.stale_tag + " error " + error);
}

staleOpen()
{
	log(level.stale_tag + " open");
}

staleMessage(text)
{
	log(level.stale_tag + " msg " + text);
}

staleClose(closedByRemote, fullyDisconnected)
{
	log(level.stale_tag + " close");
}

phase2()
{
	wait 1;
	log("phase2 start");
	fetch("phase2", level.ht_http + "/json", "GET", "", "", 10);
	id = wsConnect("phase2ws", level.ht_ws + "/echo", "", 0, 0);
	wsWait(1);
	webSocketSendText(id, "after-map-change");
	wsWait(2);
	webSocketClose(id);
	wsWait(3);
	wait 6;
	stale("stale2", "phase2 end");
}

phase3()
{
	wait 1;
	log("phase3 start");
	wait 6;
	log("phase3 end");
}
'''


class State:
    def __init__(self):
        self.lock = threading.Lock()
        self.slow_started = []
        self.ws_connects = {}
        self.ws_pings = 0
        self.ws_unmasked = 0
        self.ws_closed = {}
        self.ws_bad_handshake = 0
        self.echo_headers = []


STATE = State()

# responses written as they are, for the parser's checks
RAW_RESPONSES = {
    '/nulheader': b'HTTP/1.1 200 OK\r\nX-A\0b: 1\r\nX-A\0c: 2\r\nContent-Length: 2\r\n\r\nok',
    '/ctlheader': b'HTTP/1.1 200 OK\r\nX\x01A: 1\r\nContent-Length: 2\r\n\r\nok',
    '/manyheaders': b'HTTP/1.1 200 OK\r\n' + b''.join(b'X-H%d: %d\r\n' % (i, i) for i in range(150))
                    + b'Content-Length: 2\r\n\r\nok',
    '/folded': b'HTTP/1.1 200 OK\r\nX-Test: one\r\n two\r\nContent-Length: 2\r\n\r\nok',
    '/hexlength': b'HTTP/1.1 200 OK\r\nContent-Length: 0x2\r\n\r\nok',
    '/samelength': b'HTTP/1.1 200 OK\r\nContent-Length: 2\r\nContent-Length: 2\r\n\r\nok',
    '/twolengths': b'HTTP/1.1 200 OK\r\nContent-Length: 2\r\nContent-Length: 1\r\n\r\nok',
    '/continue': b'HTTP/1.1 100 Continue\r\n\r\n' * 3 + b'HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok',
    '/endless': b'HTTP/1.1 100 Continue\r\n\r\n' * 20 + b'HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok',
}


class HttpHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, *args):
        pass

    def reply(self, status, body, headers=(), head=False):
        self.send_response(status)
        for name, value in headers:
            self.send_header(name, value)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        if not head:
            self.wfile.write(body)

    def chunked(self, parts):
        self.send_response(200)
        self.send_header('Transfer-Encoding', 'chunked')
        self.send_header('Content-Type', 'text/plain')
        self.end_headers()
        for part in parts:
            self.wfile.write(b'%x;ext=1\r\n%s\r\n' % (len(part), part))
        self.wfile.write(b'0\r\nX-Trailer: 1\r\n\r\n')

    def route(self, method):
        path = self.path
        length = int(self.headers.get('Content-Length', 0) or 0)
        body = self.rfile.read(length) if length else b''
        try:
            if path.startswith('/json'):
                self.reply(200, b'{"ok":true,"n":42}', [('Content-Type', 'application/json'), ('X-Test', 'abc')],
                           head=method == 'HEAD')
            elif path.startswith('/echo'):
                text = '|'.join([method, path, self.headers.get('X-Custom', 'none'),
                                 self.headers.get('X-Injected', 'none'), body.decode('latin-1'),
                                 self.headers.get('Content-Type', 'none'), self.headers.get('Host', 'none')])
                self.reply(200, text.encode('latin-1'))
            elif path == '/auth':
                self.reply(200, self.headers.get('Authorization', 'none').encode())
            elif path == '/chunked':
                self.chunked([b'part1-', b'part2-', b'part3'])
            elif path == '/max':
                self.reply(200, b'm' * 65000)
            elif path == '/big':
                self.reply(200, b'x' * 70000)
            elif path == '/bigchunked':
                self.chunked([b'y' * 7000] * 10)
            elif path == '/slow':
                with STATE.lock:
                    STATE.slow_started.append(time.time())
                time.sleep(5)
                self.reply(200, b'slow')
            elif path == '/slow2':
                time.sleep(3)
                self.reply(200, b'slow2')
            elif path == '/404':
                self.reply(404, b'nope')
            elif path == '/redirect':
                self.reply(302, b'', [('Location', '/json')])
            elif path in RAW_RESPONSES:
                self.wfile.write(RAW_RESPONSES[path])
                self.close_connection = True
            elif path == '/eof':
                self.send_response(200)
                self.send_header('Connection', 'close')
                self.end_headers()
                self.wfile.write(b'until-close')
                self.close_connection = True
            else:
                self.reply(400, b'unknown')
        except OSError:
            pass

    def do_GET(self):
        self.route('GET')

    def do_POST(self):
        self.route('POST')

    def do_HEAD(self):
        self.route('HEAD')


class HttpServer(http.server.ThreadingHTTPServer):
    daemon_threads = True
    request_queue_size = 64
    tls = None

    def finish_request(self, request, client_address):
        if self.tls:
            # the handshake runs in the request's own thread
            try:
                request = self.tls.wrap_socket(request, server_side=True)
            except (OSError, ssl.SSLError):
                return
        self.RequestHandlerClass(request, client_address, self)


def ws_frame(opcode, payload, fin=True):
    head = bytes([(0x80 if fin else 0) | opcode])
    n = len(payload)
    if n < 126:
        head += bytes([n])
    elif n <= 0xffff:
        head += bytes([126]) + struct.pack('!H', n)
    else:
        head += bytes([127]) + struct.pack('!Q', n)
    return head + payload


class WsHandler(socketserver.BaseRequestHandler):
    def recv_exact(self, n):
        data = b''
        while len(data) < n:
            chunk = self.sock.recv(n - len(data))
            if not chunk:
                raise EOFError
            data += chunk
        return data

    def read_frame(self):
        b0, b1 = self.recv_exact(2)
        n = b1 & 0x7f
        if n == 126:
            n = struct.unpack('!H', self.recv_exact(2))[0]
        elif n == 127:
            n = struct.unpack('!Q', self.recv_exact(8))[0]
        if not b1 & 0x80:
            with STATE.lock:
                STATE.ws_unmasked += 1
            mask = b'\0\0\0\0'
        else:
            mask = self.recv_exact(4)
        payload = bytes(c ^ mask[i & 3] for i, c in enumerate(self.recv_exact(n)))
        return b0 & 0x80 != 0, b0 & 0x0f, payload

    def handle(self):
        self.sock = self.request
        if self.server.tls:
            try:
                self.sock = self.server.tls.wrap_socket(self.sock, server_side=True)
            except (OSError, ssl.SSLError):
                return
        try:
            self.session()
        except (EOFError, OSError):
            pass
        finally:
            path = getattr(self, 'path', None)
            if path:
                with STATE.lock:
                    STATE.ws_closed.setdefault(path, []).append(time.time())

    def session(self):
        data = b''
        while b'\r\n\r\n' not in data:
            chunk = self.sock.recv(4096)
            if not chunk:
                return
            data += chunk
        lines = data.split(b'\r\n\r\n')[0].decode('latin-1').split('\r\n')
        self.path = lines[0].split()[1]
        headers = {}
        for line in lines[1:]:
            name, _, value = line.partition(':')
            headers[name.strip().lower()] = value.strip()
        key = headers.get('sec-websocket-key', '')
        if headers.get('upgrade', '').lower() != 'websocket' or headers.get('sec-websocket-version') != '13' or \
                len(base64.b64decode(key)) != 16:
            with STATE.lock:
                STATE.ws_bad_handshake += 1
            self.sock.sendall(b'HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n')
            return
        accept = base64.b64encode(hashlib.sha1((key + WS_GUID).encode()).digest()).decode()
        self.sock.sendall(('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n'
                           f'Sec-WebSocket-Accept: {accept}\r\n\r\n').encode())
        with STATE.lock:
            STATE.ws_connects[self.path] = STATE.ws_connects.get(self.path, 0) + 1

        if self.path == '/stream':
            for i in range(1000):
                self.sock.sendall(ws_frame(1, b'tick%d' % i))
                time.sleep(0.2)
            return
        if self.path == '/burst':
            self.sock.sendall(b''.join(ws_frame(1, b'b%d' % i) for i in range(200)))
        elif self.path == '/flood':
            frame = ws_frame(1, b'f' * 60000)
            for i in range(100):
                self.sock.sendall(frame)

        send = lambda op, payload, fin=True: self.sock.sendall(ws_frame(op, payload, fin))
        while True:
            fin, op, payload = self.read_frame()
            if op == 8:
                send(8, payload[:2])
                return
            if op == 9:
                if self.path != '/nopong':
                    with STATE.lock:
                        STATE.ws_pings += 1
                    send(10, payload)
                continue
            if op == 10:
                if payload == b'hi':
                    send(1, b'pong-ok:' + payload)
                continue
            msg = payload.decode('latin-1')
            if msg == 'hello':
                send(1, ('echo:hello|' + headers.get('x-hello', '')).encode())
            elif msg == 'frag':
                send(1, b'fragmented-', False)
                send(9, b'mid')
                send(0, b'mess', False)
                send(0, b'age')
            elif msg == 'ping':
                send(9, b'hi')
            elif msg == 'big':
                send(1, b'z' * 70000)
                send(1, b'q' * 30000, False)
                send(0, b'q' * 30000, False)
                send(0, b'q' * 30000)
                send(1, b'after-big')
            elif msg == 'max':
                send(1, b'w' * 65000)
            elif msg == 'binary':
                send(2, b'\x00\x01\x02')
                send(1, b'after-binary')
            elif msg == 'close':
                send(8, struct.pack('!H', 1000))
                while self.read_frame()[1] != 8:
                    pass
                return
            elif msg == 'drop':
                self.sock.shutdown(socket.SHUT_RDWR)
                return
            else:
                send(1, ('echo:' + msg).encode())


class WsServer(socketserver.ThreadingTCPServer):
    daemon_threads = True
    request_queue_size = 64
    allow_reuse_address = True
    tls = None


def make_certs(folder):
    """A CA (RSA) and two server certificates it signs: ECDSA for HTTPS, RSA for WSS."""
    def run(*args):
        subprocess.run(['openssl'] + list(args), cwd=folder, check=True, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL)
    with open(os.path.join(folder, 'ext.cnf'), 'w') as f:
        f.write('subjectAltName=DNS:localhost,IP:127.0.0.1\nbasicConstraints=CA:FALSE\n'
                'extendedKeyUsage=serverAuth\n')
    run('req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', 'ca.key', '-out', 'ca.pem', '-days', '2',
        '-subj', '/CN=CoD2rev test CA', '-addext', 'basicConstraints=critical,CA:TRUE',
        '-addext', 'keyUsage=critical,keyCertSign,cRLSign')
    for name, key in (('ec', ['-newkey', 'ec', '-pkeyopt', 'ec_paramgen_curve:prime256v1']),
                      ('rsa', ['-newkey', 'rsa:2048'])):
        run('req', *key, '-nodes', '-keyout', f'{name}.key', '-out', f'{name}.csr', '-subj', '/CN=localhost')
        run('x509', '-req', '-in', f'{name}.csr', '-CA', 'ca.pem', '-CAkey', 'ca.key', '-CAcreateserial',
            '-out', f'{name}.pem', '-days', '2', '-extfile', 'ext.cnf')
    return os.path.join(folder, 'ca.pem')


def tls_context(folder, name, max_version=None):
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.load_cert_chain(os.path.join(folder, f'{name}.pem'), os.path.join(folder, f'{name}.key'))
    if max_version:
        ctx.maximum_version = max_version
    return ctx


def start(server):
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server.server_address[1]


def log_lines(path):
    if not os.path.exists(path):
        return []
    return [re.sub(r'^\s*\d+:\d+ ', '', l) for l in open(path, encoding='latin-1').read().splitlines()]


def wait_line(path, pattern, seconds):
    deadline = time.time() + seconds
    while time.time() < deadline:
        if any(re.search(pattern, l) for l in log_lines(path)):
            return True
        time.sleep(0.5)
    return False


class Pinger(threading.Thread):
    """Queries the server every 100 ms and records how long each reply takes."""
    def __init__(self, port):
        super().__init__(daemon=True)
        self.port = port
        self.samples = []
        self.running = True

    def run(self):
        while self.running:
            t = time.time()
            ok = query(self.port, 'getinfo', timeout=3.0) is not None
            self.samples.append((t, time.time() - t if ok else 3.0))
            time.sleep(0.1)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--binary', default=os.path.join(ROOT, 'bin', 'cod2rev_lnxded'))
    ap.add_argument('--map', default='mp_toujane')
    ap.add_argument('--keep', help='copy games_mp.log and the server log to this folder')
    args = ap.parse_args()

    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''))
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2rev_http_') as home:
        certs = os.path.join(home, 'certs')
        os.makedirs(certs)
        ca = make_certs(certs)

        http_port = start(HttpServer(('127.0.0.1', 0), HttpHandler))
        https = HttpServer(('127.0.0.1', 0), HttpHandler)
        https.tls = tls_context(certs, 'ec')
        https_port = start(https)
        ws_port = start(WsServer(('127.0.0.1', 0), WsHandler))
        wss = WsServer(('127.0.0.1', 0), WsHandler)
        wss.tls = tls_context(certs, 'rsa', ssl.TLSVersion.TLSv1_2)
        wss_port = start(wss)
        # a TCP port nothing listens on
        with socket.socket() as s:
            s.bind(('127.0.0.1', 0))
            closed_port = s.getsockname()[1]

        text, _ = read_game_script(args.game_dir, CALLBACKS)
        if text.count(MARKER) != 1:
            sys.exit(f'{CALLBACKS} does not look like the stock file')
        newline = '\r\n' if '\r\n' in text else '\n'
        text = text.replace(MARKER, MARKER + newline + '\t\t' + HOOK) + ERROR_CALLBACK
        folder = os.path.join(home, MOD, 'maps', 'mp', 'gametypes')
        os.makedirs(folder)
        with open(os.path.join(folder, '_callbacksetup.gsc'), 'w', encoding='latin-1', newline='') as f:
            f.write(text)
        with open(os.path.join(folder, '_httptest.gsc'), 'w', encoding='latin-1', newline='') as f:
            f.write(TEST_SCRIPT)

        game_log = os.path.join(home, MOD, 'games_mp.log')
        server_log = os.path.join(home, 'server.log')
        port = free_port()
        password = secrets.token_hex(8)
        pinger = Pinger(port)
        with open(server_log, 'wb') as out:
            server = subprocess.Popen(
                [args.binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                 '+set', 'fs_game', MOD, '+set', 'dedicated', '1', '+set', 'net_port', str(port),
                 '+set', 'sv_cracked', '1', '+set', 'rcon_password', password, '+set', 'g_gametype', 'tdm',
                 '+set', 'scr_tdm_timelimit', '0', '+set', 'g_logSync', '1', '+set', 'sv_maxclients', '8',
                 '+set', 'sv_httpAllowPrivate', '1', '+set', 'ht_http', f'127.0.0.1:{http_port}',
                 '+set', 'ht_https', f'127.0.0.1:{https_port}', '+set', 'ht_https_port', str(https_port),
                 '+set', 'ht_ws', f'127.0.0.1:{ws_port}', '+set', 'ht_wss', f'127.0.0.1:{wss_port}',
                 '+set', 'ht_ca', ca, '+set', 'ht_closed', str(closed_port), '+set', 'ht_phase', '1',
                 '+map', args.map],
                stdin=subprocess.DEVNULL, stdout=out, stderr=subprocess.STDOUT)
        map_changed = restarted = None
        try:
            check('test script started', wait_line(game_log, r'^HTTPTEST start', 120))
            pinger.start()
            phase1 = wait_line(game_log, r'^HTTPTEST phase1 done', 180)
            check('first map finished its tests', phase1)
            pinger.running = False
            if phase1:
                rcon(port, password, 'set ht_phase 2')
                map_changed = time.time()
                rcon(port, password, 'map ' + args.map)
                phase2 = wait_line(game_log, r'^HTTPTEST phase2 end', 120)
                check('second map finished its tests', phase2)
                if phase2:
                    rcon(port, password, 'set ht_phase 3')
                    restarted = time.time()
                    # fast_restart reloads no scripts, so InitLibcodCallbacks
                    # does not run; the 'map' above covers the full reload
                    rcon(port, password, 'fast_restart')
                    check('fast_restart finished', wait_line(game_log, r'^HTTPTEST phase3 end', 120))
            check('server still running', server.poll() is None and query(port, 'getinfo') is not None)
        finally:
            pinger.running = False
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    server.kill()

        if args.keep:
            os.makedirs(args.keep, exist_ok=True)
            for name in (game_log, server_log):
                if os.path.exists(name):
                    shutil.copy(name, args.keep)

        lines = [l[len('HTTPTEST '):] for l in log_lines(game_log) if l.startswith('HTTPTEST ')]
        errors = [l for l in log_lines(game_log) if l.startswith('HTTPERR ')]
        console = open(server_log, encoding='latin-1').read() if os.path.exists(server_log) else ''

        def result(name):
            for l in lines:
                if l.startswith(name + ' done ') or l.startswith(name + ' error ') or l == name + ' hang':
                    return l[len(name) + 1:]
            return 'no result'

        def expect(name, pattern):
            r = result(name)
            check(f'http {name}', re.search(pattern, r) is not None, r[:120])

        # HTTP
        expect('json', r'^done 200 len=18 ct=application/json xt=abc loc= body=\{"ok":true,"n":42\}$')
        expect('echo', r'^done 200 .* body=POST\|/echo\?q=1\|hello\|none\|payload\|text/plain\|127\.0\.0\.1:\d+$')
        expect('chunked', r'^done 200 len=17 .* body=part1-part2-part3$')
        expect('max', r'^done 200 len=65000 ')
        expect('big', r'^error .*Response body too large: 70000 bytes')
        expect('bigchunked', r'^error .*Response body too large')
        expect('slow', r'^error ms=(19|20|21|22)\d\d Timeout$')
        expect('notfound', r'^done 404 .* body=nope$')
        expect('redirect', r'^done 302 .* loc=/json ')
        expect('eof', r'^done 200 .* body=until-close$')
        expect('nulheader', r'^error ms=\d+ Invalid response$')
        expect('ctlheader', r'^error ms=\d+ Invalid response$')
        expect('manyheaders', r'^error ms=\d+ Response headers too large$')
        expect('folded', r'^done 200 len=2 .* xt=one two .* body=ok$')
        expect('hexlength', r'^error ms=\d+ Invalid response$')
        expect('samelength', r'^done 200 len=2 .* body=ok$')
        expect('twolengths', r'^error ms=\d+ Invalid response$')
        expect('continue', r'^done 200 len=2 .* body=ok$')
        expect('endless', r'^error ms=\d+ Too many interim responses$')
        expect('head', r'^done 200 len=0 ct=application/json')
        expect('auth', r'^done 200 .* body=Basic ' + re.escape(base64.b64encode(b'user:secret').decode()) + '$')
        expect('invalid', r'^error ms=\d+ Invalid URL$')
        expect('badmethod', r'^error ms=\d+ Invalid method$')
        expect('refused', r'^error ms=\d+ Connection refused$')
        expect('notallowed', r'^error ms=\d+ Host not allowed by sv_httpAllowedHosts$')
        expect('allowed', r'^done 200 ')
        expect('private_setcvar', r'^done 200 ')
        expect('https_noca', r'^error ms=\d+ TLS: certificate verification failed')
        expect('https_ca', r'^done 200 len=18 ')
        expect('https_localhost', r'^done 200 len=18 ')
        expect('https_badca', r'^error ms=\d+ TLS: cannot read the CA file')
        expect('https_insecure', r'^done 200 len=18 ')
        check('insecure warning printed', 'httpFetch: certificate verification is off for 127.0.0.1' in console)
        cap = next((l for l in lines if l.startswith('cap done=')), '')
        check('at most 16 requests at once', cap == 'cap done=16 errors=4 Too many requests in progress', cap)

        # the game thread is not blocked during the slow request
        slow = STATE.slow_started[0] if STATE.slow_started else None
        window = [lat for t, lat in pinger.samples if slow and slow <= t <= slow + 2.0]
        check('server answers during a slow request', len(window) >= 10 and max(window) < 0.25,
              f'{len(window)} queries, slowest {max(window) * 1000:.0f} ms' if window else 'no samples')

        # WebSocket
        def events(tag):
            return [l[len(tag) + 1:] for l in lines if l.startswith(tag + ' ') and re.match(r'\d+ ', l[len(tag) + 1:])]

        ws = events('ws')
        expected = ['1 open', '2 msg echo:hello|there', '3 msg fragmented-message', '4 msg pong-ok:hi',
                    '5 msg after-big', '6 msg after-binary', '7 msg (65000 bytes)', '8 close 1 0', '9 open',
                    '10 close 1 0', '11 open', '12 msg echo:again', '13 close 0 1']
        check('ws session', ws == expected, '; '.join(ws))
        check('ws send and close results', 'ws sent 1' in lines and 'ws closed 1' in lines
              and 'ws after close sent 0 closed 0' in lines)
        check('ws dropped message logged', 'dropped a message over 65000 bytes' in console)
        check('ws client frames masked', STATE.ws_unmasked == 0 and STATE.ws_bad_handshake == 0)
        keep = events('keepalive')
        check('ws keepalive pings', keep == ['1 open', '2 msg echo:still', '3 close 0 1'] and STATE.ws_pings >= 3,
              f'{"; ".join(keep)}; {STATE.ws_pings} pings')
        check('ws ping timeout', events('nopong') == ['1 open', '2 error Ping timeout (no pong received within time limit)',
                                                     '3 close 1 1'], '; '.join(events('nopong')))
        check('ws connection refused', events('refused') == ['1 error Connection refused']
              and 'refused id after 0' in lines, '; '.join(events('refused')))
        check('ws invalid url', 'invalid id -1' in lines)
        check('wss with the CA', events('wss') == ['1 open', '2 msg echo:hello|', '3 close 0 1'], '; '.join(events('wss')))
        noca = events('wss_noca')
        check('wss without the CA', len(noca) == 1 and noca[0].startswith('1 error TLS: certificate verification failed'),
              '; '.join(noca))
        check('wss insecure', events('wss_insecure') == ['1 open', '2 close 0 1'], '; '.join(events('wss_insecure')))
        cap_ids = next((l for l in lines if l.startswith('cap ids ')), '')
        cap_events = events('cap')
        check('16 websockets at once', cap_ids == 'cap ids ' + ''.join(f'{i},' for i in range(16))
              and sum(1 for e in cap_events if e.endswith(' open')) == 16
              and sum(1 for e in cap_events if e.endswith(' close 0 1')) == 16, cap_ids)
        check('17th websocket refused', 'cap 17 after 0' in lines
              and any('no free websocket client slots' in e for e in errors), '; '.join(errors))
        check('slot reused', events('reuse') == ['1 open', '2 close 0 1'], '; '.join(events('reuse')))
        burst = next((l for l in lines if l.startswith('burst count=')), '')
        m = re.match(r'burst count=(\d+) max=(\d+)$', burst)
        check('ws burst delivered at most 64 per frame', m is not None and m.group(1) == '200'
              and 1 <= int(m.group(2)) <= 64, burst)
        flood = next((l for l in lines if l.startswith('flood count=')), '')
        m = re.match(r'flood count=(\d+)$', flood)
        check('ws flood dropped over 1 MB queued', m is not None and 0 < int(m.group(1)) < 100
              and re.search(r'webSocket ws://[^ ]*/flood: dropped \d+ messages', console) is not None, flood)
        check('ws flood had no errors', not [l for l in lines if l.startswith('flood error')])

        # map change and fast_restart
        for phase, tag, since in (('phase2', 'stale1', map_changed), ('phase3', 'stale2', restarted)):
            start_index = lines.index(phase + ' start') if phase + ' start' in lines else len(lines)
            check(f'old level got callbacks ({tag})', any(l.startswith(tag + ' msg ') for l in lines[:start_index]))
            stale = [l for l in lines[start_index:] if l.startswith(tag)]
            check(f'old callbacks dropped ({phase})', phase + ' start' in lines and not stale, '; '.join(stale[:3]))
            closed = [t for t in STATE.ws_closed.get('/stream', []) if since and t >= since]
            check(f'old websockets closed on level load ({phase})', len(closed) >= 1)
        check('no reconnect after level load', STATE.ws_connects.get('/stream') == 2,
              f'{STATE.ws_connects.get("/stream")} connects')
        expect('phase2', r'^done 200 ')
        check('websocket on the new map', events('phase2ws') == ['1 open', '2 msg echo:after-map-change', '3 close 0 1'],
              '; '.join(events('phase2ws')))
        hangs = [l for l in lines if l.endswith(' hang') or ' hang at ' in l]
        check('no callback missing', not hangs, '; '.join(hangs))
        other = [e for e in errors if 'no free websocket client slots' not in e]
        check('no script errors', not other, '; '.join(other[:3]))

    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
