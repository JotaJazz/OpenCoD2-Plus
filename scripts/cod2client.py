"""A minimal CoD2 client for tests.

It speaks just enough of the protocol to take a client slot, receive the
gamestate, enter the game and send client commands and moves through the same
path as a real player: netchan (including fragments both ways), the XOR keys,
Huffman compression, SV_ClientCommand and SV_UserMove. Server messages are
parsed by cod2snap: read_frames returns the snapshots and server commands.
"""
import random, socket, time

import cod2snap

PREFIX = b'\xff\xff\xff\xff'

# Code of each byte value (0-255) in the static Huffman tree that
# MSG_InitHuffman builds from its frequency table, bits in the order they are
# sent; each byte of the packet is filled from its lowest bit. Taken from the
# server by compressing eight copies of each byte with MSG_WriteBitsCompress,
# which then gives exactly one output byte per code bit.
HUFFMAN_CODES = """
    01 11011 0001001 0011011 10000101 10001000 0000100 111111 10101 0010110
    1001011 1101000 1100100 101101 10011100 001101010 1010010 000110100
    000011111 000111111 1011101110 1100111111 1101010001 1100010011
    001011110 1011000110 1101010100 1100011011 1101011110 11000010 111100100
    00101011 111011 1100101100 001000100 1011001101 1010001111 1001111111
    1011000011 1011001111 001010010 0011010110 1100110000 0010100000
    1000111001 0000111101 001011111 00011110 1110010 11000011 11110111
    111010011 11001010 10001101 101100000 100001101 111000000 100011110
    100110011 1000100110 1100010010 001111010 001000101 1100111110 11110011
    0000101 0011111 0010000 10000100 10001010 000000010 000011100 101111001
    1100011000 1110101101 1000111011 101110011 1101010101 1110001110
    1100110011 001101001 1011100100 1101011010 1101011011 1100101110
    1000111010 1100011110 1100110001 1110101100 1101001101 000011101
    000101010 111000110 101000001 110101110 00011011 110011110 00111100
    110101011 111100011 111010010 0000110 10000010 111100101 001110 00010100
    101111010 001000110 00011001 111100010 00110000 11001101 10100001
    00110001 11100010 10011110 1001101 0000001 11101000 10011000 11010010
    10000111 11000101 11001110 11110110 0001011 0001000 1010011 111110
    1001010 0011001 1011111 11110000 11000001 10000000 11100001 11100110
    11100111 11101010 00101110 110100111 001000111 100000010 101100100
    100011001 1100011010 000000011 1110101111 1101010000 1010001010
    1101011000 1011110000 1101010011 1001111100 1100011100 1010001001
    1101010010 1000110000 1101011111 1001110110 001010001 1001111110
    1100110010 0011010000 1011101101 1000001110 1011100011 1000101110
    000111001 1000011000 1100011101 1010001011 1101011001 1011001100
    1101001100 1011001010 000111000 1000001111 1011110001 0011110111
    1010001100 0011010111 1100011111 1001100101 1100101111 0010100001
    1100000001 0011110110 1011100101 1000000111 1100011001 1011000100
    00101010 110000001 10111010 000001 1001000 1110001111 1110000011
    00011101 100011111 0011010001 1010001000 1100000000 1011100010
    1010001110 1011001011 1011100000 000000000 1001110111 1011101100
    1000000110 1011000010 1001110100 1011001110 0010100111 1011110110
    0010100110 1011000111 0001101010 1011110111 1000001100 1011101111
    0001111100 100010110 100010010 1001100100 0001111101 1100010000
    1000111000 1100101101 1000100111 1001001 1000110001 1110000010
    1000011001 1100010001 00011000 1010000001 1000001101 1110101110
    0000111100 1010001101 0000000011 1000101111 0001010111 1011100001
    00000000101 1010000000 0001101011 1001111101 0001010110 1001110101
    1011000101 1111010 001001
""".split()
assert len(HUFFMAN_CODES) == 256
HUFFMAN_SYMBOLS = {code: symbol for symbol, code in enumerate(HUFFMAN_CODES)}

FRAGMENT_SIZE = 1300
FRAGMENT_BIT = 1 << 31

CLC_MOVE_NO_DELTA = 1
CLC_CLIENT_COMMAND = 2
CLC_EOF = 3

KEY_MASK_MOVE_FORWARD = 1


def huffman_compress(data):
    out = bytearray()
    bit = 0
    for b in data:
        for c in HUFFMAN_CODES[b]:
            if bit & 7 == 0:
                out.append(0)
            if c == '1':
                out[-1] |= 1 << (bit & 7)
            bit += 1
    return bytes(out)


def huffman_decompress(data):
    """Decodes until the input runs out; a symbol cut short by the end of the
    input (the padding of the last byte) is dropped, as the server does."""
    out = bytearray()
    code = ''
    for i in range(len(data) * 8):
        code += '1' if (data[i >> 3] >> (i & 7)) & 1 else '0'
        symbol = HUFFMAN_SYMBOLS.get(code)
        if symbol is not None:
            out.append(symbol)
            code = ''
    return bytes(out)


def xor_codec(data, key, string):
    """SV_Netchan_Encode / SV_Netchan_Decode: the key is changed by each byte
    of a command string both sides know, cycling through it."""
    # the server's buffers are zero filled, so an empty string reads as zeros
    string = string.encode('latin-1') + b'\0\0'
    out = bytearray(data)
    index = 0
    for i in range(len(out)):
        if string[index] == 0:
            index = 0
        key = (key ^ (string[index] << (i & 1))) & 0xff
        out[i] ^= key
        index += 1
    return bytes(out)


def hash_key(string, maxlen=32):
    """Com_HashKey, with the signed chars of the server."""
    h = 0
    for i, c in enumerate(string.encode('latin-1')[:maxlen]):
        h += (c - 256 if c > 127 else c) * (119 + i)
    h &= 0xffffffff
    h = (h ^ (h >> 10) ^ (h >> 20)) & 0xffffffff
    return h


class MsgWriter:
    """Mirrors the server's msg_t: bit fields share a byte that later whole-byte
    writes skip past, which is how MSG_ReadBits and MSG_ReadLong read them back."""

    def __init__(self):
        self.data = bytearray()
        self.bit = 0

    def bits(self, value, count):
        for i in range(count):
            if self.bit & 7 == 0:
                self.bit = len(self.data) * 8
                self.data.append(0)
            if (value >> i) & 1:
                self.data[self.bit >> 3] |= 1 << (self.bit & 7)
            self.bit += 1

    def byte(self, value):
        self.data.append(value & 0xff)

    def long(self, value):
        self.data += (value & 0xffffffff).to_bytes(4, 'little')

    def string(self, text):
        self.data += text.encode('latin-1') + b'\0'


class Client:
    def __init__(self, port, host='127.0.0.1', timeout=3.0):
        self.addr = (host, port)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(timeout)
        self.qport = random.randint(1, 0xffff)
        self.protocol = 0
        self.challenge = 0
        self.outgoing_sequence = 0
        self.incoming_sequence = 0
        self.command_sequence = 0
        self.last_command = ''
        self.sent_commands = {0: ''}
        self.state = cod2snap.ServerState()
        self.parsed = 0
        self.parsed_clean = 0
        self.parse_errors = []
        self.fragment_sequence = None
        self.fragment_buffer = bytearray()
        self.server_time = 1000
        self.messages_received = 0
        self.voice_packets = []
        self.server_id = None
        self.pending_commands = []

    def close(self):
        self.sock.close()

    # connectionless

    def _oob(self, text, reply):
        """Sends a connectionless packet and returns the reply line that starts
        with reply or with error. Raises socket.timeout when none comes."""
        self.sock.sendto(PREFIX + text.encode('latin-1'), self.addr)
        while True:
            data, _ = self.sock.recvfrom(65535)
            if data.startswith(PREFIX):
                line = data[len(PREFIX):].decode('latin-1')
                if line.startswith(reply) or line.startswith('error'):
                    return line

    def connect(self, protocol, name):
        """Returns None once connected, otherwise the server's reply."""
        self.protocol = protocol
        line = self._oob('getchallenge', 'challengeResponse')
        if not line.startswith('challengeResponse'):
            return line
        self.challenge = int(line.split()[1])
        userinfo = ('\\protocol\\%d\\challenge\\%d\\qport\\%d\\name\\%s\\rate\\25000\\snaps\\20\\cl_voice\\1'
                    % (protocol, self.challenge, self.qport, name))
        line = self._oob('connect "%s"' % userinfo, 'connectResponse')
        return None if line.startswith('connectResponse') else line

    # netchan

    def _fragment_start(self, value=None):
        """1.3 sends fragment offsets as longs, older versions as shorts."""
        size = 4 if self.protocol == 118 else 2
        return size if value is None else (value & (1 << size * 8) - 1).to_bytes(size, 'little')

    def _transmit(self, message):
        """Netchan_Transmit: messages of FRAGMENT_SIZE or more go in fragments,
        and one ending exactly on a fragment boundary gets an empty last one."""
        self.outgoing_sequence += 1
        header = self.outgoing_sequence.to_bytes(4, 'little') + self.qport.to_bytes(2, 'little')
        if len(message) < FRAGMENT_SIZE:
            self.sock.sendto(header + message, self.addr)
            return 1
        header = ((self.outgoing_sequence | FRAGMENT_BIT).to_bytes(4, 'little')
                  + self.qport.to_bytes(2, 'little'))
        start = 0
        fragments = 0
        while True:
            chunk = message[start:start + FRAGMENT_SIZE]
            self.sock.sendto(header + self._fragment_start(start)
                             + len(chunk).to_bytes(2, 'little') + chunk, self.addr)
            fragments += 1
            start += len(chunk)
            if len(chunk) < FRAGMENT_SIZE:
                return fragments
            time.sleep(0.01)

    def _process(self, packet):
        """Netchan_Process for server packets: returns (sequence, message) once a
        message is complete, otherwise None."""
        if len(packet) < 4:
            return None
        sequence = int.from_bytes(packet[:4], 'little')
        body = packet[4:]
        if sequence & FRAGMENT_BIT:
            sequence &= ~FRAGMENT_BIT
            size = self._fragment_start()
            if len(body) < size + 2:
                return None
            start = int.from_bytes(body[:size], 'little')
            length = int.from_bytes(body[size:size + 2], 'little')
            chunk = body[size + 2:size + 2 + length]
            if sequence != self.fragment_sequence:
                self.fragment_sequence = sequence
                self.fragment_buffer = bytearray()
            if start != len(self.fragment_buffer) or len(chunk) != length:
                return None
            self.fragment_buffer += chunk
            if length == FRAGMENT_SIZE:
                return None
            body = bytes(self.fragment_buffer)
            self.fragment_buffer = bytearray()
        if sequence <= self.incoming_sequence:
            return None
        self.incoming_sequence = sequence
        self.messages_received += 1
        return sequence, body

    def receive(self, seconds):
        """Reads server packets for up to seconds; returns the complete messages."""
        messages = []
        deadline = time.time() + seconds
        while time.time() < deadline:
            self.sock.settimeout(max(0.01, deadline - time.time()))
            try:
                packet, _ = self.sock.recvfrom(65535)
            except socket.timeout:
                break
            if packet.startswith(PREFIX):
                self._voice(packet[len(PREFIX):])
                continue
            message = self._process(packet)
            if message:
                messages.append(message)
        return messages

    def _voice(self, data):
        """SV_SendClientVoiceData sends voice outside the netchan: "v", then the
        packet count and, for each packet, the talker, the size and the Speex
        data. Kept in voice_packets as (talker, data)."""
        if not data.startswith(b'v\0') or len(data) < 3:
            return
        pos, count = 3, data[2]
        for _ in range(count):
            if pos + 2 > len(data):
                return
            talker, size = data[pos], data[pos + 1]
            self.voice_packets.append((talker, data[pos + 2:pos + 2 + size]))
            pos += 2 + size

    # server messages

    @property
    def client_num(self):
        return self.state.client_num

    @property
    def checksum_feed(self):
        return self.state.checksum_feed

    def _decode(self, sequence, data):
        """Undoes SV_Netchan_Encode and the Huffman compression. The first 4
        bytes, the last client command the server executed, are neither; the
        key is the challenge XORed with the packet sequence, changed by the
        text of that command (lastClientCommandString)."""
        executed = int.from_bytes(data[:4], 'little', signed=True)
        string = self.sent_commands.get(executed, '')
        return huffman_decompress(xor_codec(data[4:], (self.challenge ^ sequence) & 0xff, string))

    def _parse(self, sequence, data):
        """Decodes and parses one server message; see cod2snap.ServerState.parse."""
        result = self.state.parse(self._decode(sequence, data))
        self.pending_commands += result['commands']
        self.parsed += 1
        if result['clean']:
            self.parsed_clean += 1
        else:
            self.parse_errors.append(result['error'])
        return result

    # client messages

    def _acknowledged(self):
        """The last server command received, which the client acknowledges, and
        its text: the server keys the XOR of client messages and the usercmds
        with that slot of its command buffer."""
        sequence = self.state.command_sequence
        return sequence, self.state.commands.get(sequence, '')

    def _send(self, server_id, payload):
        """Sends a client message. Acknowledging the server commands keeps the
        server from resending them all and from dropping the client once 128
        are pending."""
        message_ack = self.incoming_sequence
        reliable_ack, string = self._acknowledged()
        body = huffman_compress(payload.data)
        body = xor_codec(body, (self.challenge ^ server_id ^ message_ack) & 0xff, string)
        message = (bytes([server_id & 0xff]) + message_ack.to_bytes(4, 'little')
                   + reliable_ack.to_bytes(4, 'little') + body)
        return self._transmit(message)

    def _write_commands(self, payload, texts):
        for text in texts:
            self.command_sequence += 1
            payload.bits(CLC_CLIENT_COMMAND, 3)
            payload.long(self.command_sequence)
            payload.string(text)
            self.last_command = text
            self.sent_commands[self.command_sequence] = text

    def commands(self, server_id, texts):
        """Sends reliable client commands in one message; returns the number of
        packets it took."""
        payload = MsgWriter()
        self._write_commands(payload, texts)
        payload.bits(CLC_EOF, 3)
        return self._send(server_id, payload)

    def command(self, server_id, text):
        return self.commands(server_id, [text])

    def download(self, server_id, name, seconds=2.0):
        """Asks for a file as the client's auto-download does, before entering
        the game, and returns what the svc_download blocks of the following
        messages carried: file data, or the error text of a refusal. The key
        of each message is the command its header says the server executed
        last, so messages sent before the download command decode too."""
        self.command(server_id, 'download %s' % name)
        out = []
        for sequence, data in self.receive(seconds):
            result = self._parse(sequence, data)
            if result['error']:
                out.append(self._decode(sequence, data))
            for block, size, chunk in result['downloads']:
                out.append(chunk.encode('latin-1') if isinstance(chunk, str) else chunk)
        return b''.join(out)

    def request_gamestate(self, server_id, seconds=5.0):
        """Sends a message from an older map, which makes the server send the
        gamestate, and parses it: configstrings, baselines, client number and
        checksum feed. Returns True once it is received."""
        deadline = time.time() + seconds
        count = self.state.gamestate_count
        while time.time() < deadline:
            payload = MsgWriter()
            payload.bits(CLC_EOF, 3)
            self._send(server_id ^ 0xf0, payload)
            for sequence, data in self.receive(1.0):
                self._parse(sequence, data)
            if self.state.gamestate_count > count:
                return True
        return False

    def _write_move(self, payload, forward):
        """One usercmd as MSG_ReadDeltaUsercmdKey reads it against the default
        command: absolute server time, then changed fields XORed with the key."""
        message_ack = self.incoming_sequence
        key = (self.checksum_feed ^ message_ack ^ hash_key(self._acknowledged()[1])) & 0xffffffff
        self.server_time += 50

        payload.bits(CLC_MOVE_NO_DELTA, 3)
        payload.byte(1)                          # command count
        payload.bits(0, 1)                       # absolute server time
        payload.long(self.server_time)
        payload.bits(1 ^ (key & 1), 1)           # something changed
        payload.bits(0 ^ (key & 1), 1)           # only buttons, angles and move
        key ^= self.server_time
        payload.bits(0 ^ (key & 1), 1)           # not attacking
        payload.bits(0, 1)                       # pitch unchanged
        payload.bits(0, 1)                       # yaw unchanged
        payload.bits(1, 1)                       # move keys changed
        payload.bits((KEY_MASK_MOVE_FORWARD if forward else 0) ^ (key & 0xf), 4)

    def enter_game(self, server_id, seconds=2.0):
        """Validates the iwds the way a client that references none would, then
        keeps moving forward. The first move makes the server put the client in
        the world. Needs request_gamestate first. The snapshots received
        meanwhile are dropped; server commands wait for read_frames."""
        payload = MsgWriter()
        # SV_VerifyIwds_f: no iwds, then the checksum feed XORed with their
        # count, as the signed int atoi reads
        feed = self.checksum_feed ^ 0
        self._write_commands(payload, ['cp @ %d' % (feed - (1 << 32) if feed >= 1 << 31 else feed)])
        self._write_move(payload, True)
        self._send(server_id, payload)
        self.server_id = server_id
        self._keep_moving(seconds)

    def _keep_moving(self, seconds, forward=True):
        """Sends a move after each wait for server messages; returns the
        parsed messages."""
        results = []
        deadline = time.time() + seconds
        while time.time() < deadline:
            for sequence, data in self.receive(0.05):
                results.append(self._parse(sequence, data))
            payload = MsgWriter()
            self._write_move(payload, forward)
            self._send(self.server_id, payload)
        return results

    def read_frames(self, seconds, forward=True):
        """Keeps moving as enter_game does for up to seconds and returns one
        frame per snapshot received: {'server_time', 'commands' (server
        commands received since the previous frame, those of the gamestate
        included), 'entities' {number: fields}, 'clients' {number: fields},
        'ps', 'flags'}. Arrays are lists, e.g. entity['pos.trBase']; see
        cod2snap for the field names. Needs enter_game first."""
        frames = []
        # each snapshot gets the commands of the messages up to its own; the
        # ones after the last snapshot wait for the next call
        carry = self.pending_commands
        for result in self._keep_moving(seconds, forward):
            carry = carry + result['commands']
            snap = result['snapshot']
            if snap is not None:
                snap['commands'] = carry
                carry = []
                frames.append(snap)
        self.pending_commands = carry
        return frames
