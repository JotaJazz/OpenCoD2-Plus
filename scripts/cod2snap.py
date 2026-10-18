"""Parser for the messages a CoD2 server sends to a client, for tests.

It mirrors the server's writers (SV_SendClientGameState, SV_SendClientSnapshot,
SV_WriteSnapshotToClient and the MSG_WriteDelta* functions of msg_mp.cpp), so a
message that parses must end exactly on its svc_EOF. 1.0 and 1.3 write these
messages the same way; only the netchan differs, and cod2client handles that.

Only non-delta snapshots are read (the client sends CLC_MOVE_NO_DELTA): the
playerstate is a delta from a zeroed one, the entities deltas from their
gamestate baselines and the client states deltas from zeroed ones.
"""
import struct

SVC_NOP, SVC_GAMESTATE, SVC_CONFIGSTRING, SVC_BASELINE, SVC_SERVERCOMMAND, \
    SVC_DOWNLOAD, SVC_SNAPSHOT, SVC_EOF = range(8)

GENTITYNUM_BITS = 10
MAX_GENTITIES = 1 << GENTITYNUM_BITS
CLIENTNUM_BITS = 6
MAX_STATS = 6
MAX_OBJECTIVES = 16
OBJECTIVE_STATE_BITS = 3        # written with OBJST_DONE (3) as the bit count
HUDELEM_BITS = 5
MAX_HUDELEMS = 31
FLOAT_INT_BIAS = 1 << 12        # FLOAT_INT_BITS 13
HUDELEM_COORD_BIAS = 1 << 9     # HUDELEM_COORD_BITS 10

ET_PLAYER = 1

# netField_t tables in the order of msg_mp.cpp. Bits 0 is a float, -99 a hud
# element coordinate, -100 an angle sent as 16 bits; other negative counts are
# signed integers of that many bits.
_PS = """
commandTime 32; origin[1] 0; origin[0] 0; bobCycle 8; viewangles[1] -100;
origin[2] 0; velocity[1] 0; velocity[0] 0; viewangles[0] -100; movementDir -8;
velocity[2] 0; eventSequence 8; legsAnim 10; aimSpreadScale 0; weaponTime -16;
pm_flags 27; events[0] 8; events[1] 8; events[2] 8; events[3] 8; weapAnim 10;
viewHeightCurrent 0; torsoTimer 16; torsoAnim 10; eFlags 24; fWeaponPosFrac 0;
holdBreathScale 0; weaponstate 5; viewHeightTarget -8; weaponDelay -16;
legsTimer 16; viewHeightLerpTarget -8; groundEntityNum 10; pm_time -16;
eventParms[3] 8; eventParms[1] 8; eventParms[0] 8; eventParms[2] 8; weapon 7;
weapons[0] 32; viewHeightLerpDown 1; weaponslots[0] 32; delta_angles[0] 16;
delta_angles[1] 16; cursorHintString -8; offHandIndex 7; clientNum 8;
viewlocked_entNum 16; viewmodelIndex 8; viewHeightLerpTime 32; speed 16;
mins[1] 0; mins[0] 0; maxs[2] 0; maxs[1] 0; maxs[0] 0; gravity 16;
damageTimer 16; cursorHint 8; mantleState.flags 4; flinchYaw 16; fWaistPitch 0;
mantleState.timer 32; fTorsoPitch 0; proneTorsoPitch 0; holdBreathTimer 16;
jumpTime 32; viewangles[2] -100; foliageSoundTime 32; weapons[1] 32;
damageEvent 8; damageDuration 16; damageYaw 8; proneDirection 0;
proneDirectionPitch 0; mantleState.yaw 0; mantleState.transIndex 4;
fTorsoHeight 0; damagePitch 8; jumpOriginZ 0; pm_type 8; viewlocked 8;
weaponrechamber[0] 32; vLadderVec[0] 0; weaponslots[4] 32;
weaponRestrictKickTime -16; vLadderVec[1] 0; viewAngleClampRange[1] 0;
viewAngleClampRange[0] 0; viewAngleClampBase[1] 0; weaponrechamber[1] 32;
leanf 0; damageCount 7; grenadeTimeLeft -16; deltaTime 32; shellshockTime 32;
shellshockIndex 4; shellshockDuration 16; vLadderVec[2] 0; delta_angles[2] 16;
viewHeightLerpPosAdj 0; mins[2] 0; viewAngleClampBase[0] 0; adsDelayTime 32;
iCompassFriendInfo 32
"""

_ENTITY = """
pos.trTime 32; pos.trBase[1] 0; pos.trBase[0] 0; pos.trDelta[0] 0;
pos.trDelta[1] 0; angles2[1] 0; apos.trBase[1] -100; pos.trDelta[2] 0;
pos.trBase[2] 0; apos.trBase[0] -100; eventSequence 8; legsAnim 10; eType 8;
eFlags 24; otherEntityNum 10; surfType 8; eventParm 8; scale 8; clientNum 8;
torsoAnim 10; groundEntityNum 10; events[0] 8; events[1] 8; events[2] 8;
angles2[0] 0; events[3] 8; apos.trBase[2] -100; pos.trType 8; fWaistPitch 0;
fTorsoPitch 0; apos.trTime 32; solid 24; apos.trDelta[0] 0; apos.trType 8;
animMovetype 4; fTorsoHeight 0; apos.trDelta[2] 0; weapon 7; index 10;
apos.trDelta[1] 0; eventParms[0] 8; eventParms[1] 8; eventParms[2] 8;
eventParms[3] 8; iHeadIcon 4; pos.trDuration 32; iHeadIconTeam 2; time 32;
leanf 0; attackerEntityNum 10; time2 32; loopSound 8; origin2[2] 0;
origin2[0] 0; origin2[1] 0; angles2[2] 0; constantLight 32;
apos.trDuration 32; dmgFlags 32
"""

_CLIENT = """
team 2; name[0] 32; name[4] 32; modelindex 8; attachModelIndex[1] 8;
attachModelIndex[0] 8; name[8] 32; name[12] 32; name[16] 32; name[20] 32;
name[24] 32; name[28] 32; attachTagIndex[5] 5; attachTagIndex[0] 5;
attachTagIndex[1] 5; attachTagIndex[2] 5; attachTagIndex[3] 5;
attachTagIndex[4] 5; attachModelIndex[2] 8; attachModelIndex[3] 8;
attachModelIndex[4] 8; attachModelIndex[5] 8
"""

_OBJECTIVE = "origin[0] 0; origin[1] 0; origin[2] 0; icon 12; entNum 10; teamNum 4"

_HUDELEM = """
y -99; type 4; color.rgba 32; x -99; alignScreen 6; fontScale 0;
materialIndex 8; width 10; height 10; fadeStartTime 32; fromColor.rgba 32;
fadeTime 16; value 0; time 32; z -99; alignOrg 4; sort 0; text 8; font 4;
scaleStartTime 32; scaleTime 16; fromHeight 10; label 8; fromWidth 10;
moveStartTime 32; moveTime 16; fromX -99; fromY -99; fromAlignScreen 6;
fromAlignOrg 4; duration 32; foreground 1
"""


def _fields(text):
    out = []
    for item in text.split(';'):
        name, bits = item.split()
        out.append((name, int(bits)))
    return out


PLAYERSTATE_FIELDS = _fields(_PS)
ENTITY_FIELDS = _fields(_ENTITY)
CLIENT_FIELDS = _fields(_CLIENT)
OBJECTIVE_FIELDS = _fields(_OBJECTIVE)
HUDELEM_FIELDS = _fields(_HUDELEM)


def _zero(fields):
    return {name: 0.0 if bits in (0, -99, -100) else 0 for name, bits in fields}


class ParseError(Exception):
    pass


class MsgReader:
    """Mirrors the server's msg_t reads: a bit read that starts a new byte takes
    the next unread byte, and byte reads continue after the last byte taken, so
    bits and bytes interleave as MSG_WriteBit and MSG_WriteByte wrote them."""

    def __init__(self, data):
        self.data = data
        self.readcount = 0
        self.bit = 0

    def _need(self, count):
        if self.readcount + count > len(self.data):
            raise ParseError('read past the end at byte %d' % self.readcount)

    def read_bit(self):
        if self.bit & 7 == 0:
            self._need(1)
            self.bit = self.readcount * 8
            self.readcount += 1
        value = (self.data[self.bit >> 3] >> (self.bit & 7)) & 1
        self.bit += 1
        return value

    def read_bits(self, count):
        value = 0
        for i in range(count):
            value |= self.read_bit() << i
        return value

    def read_bytes(self, count):
        self._need(count)
        out = self.data[self.readcount:self.readcount + count]
        self.readcount += count
        return out

    def read_byte(self):
        return self.read_bytes(1)[0]

    def read_short(self):
        """Unsigned, as MSG_ReadShort returns it."""
        return int.from_bytes(self.read_bytes(2), 'little')

    def read_long(self):
        return int.from_bytes(self.read_bytes(4), 'little', signed=True)

    def read_float(self):
        return struct.unpack('<f', self.read_bytes(4))[0]

    def read_string(self):
        end = self.data.find(b'\0', self.readcount)
        if end < 0:
            raise ParseError('unterminated string at byte %d' % self.readcount)
        text = self.data[self.readcount:end].decode('latin-1')
        self.readcount = end + 1
        return text

    def read_value(self, bits):
        """MSG_ReadValue: the low bits & 7 as bits, the rest as whole bytes;
        a negative count means a signed value."""
        count = abs(bits)
        value = self.read_bits(count & 7)
        for shift in range(count & 7, count, 8):
            value |= self.read_byte() << shift
        if (bits < 0 or count == 32) and value >> (count - 1) & 1:
            value -= 1 << count
        return value

    def read_angle16(self):
        return self.read_short() * 360.0 / 65536

    def read_small_float(self, int_bits, bias):
        """A float either as a biased integer (int_bits low bits, then a byte)
        or, after a 1 bit, as its 4 bytes."""
        if self.read_bit():
            return self.read_float()
        trunc = self.read_bits(int_bits)
        return float((self.read_byte() << int_bits) + trunc - bias)


def read_playerstate_field(r, bits):
    """MSG_WriteDeltaPlayerstate: no 'is zero' bit for floats and integers."""
    if bits == 0:
        return r.read_small_float(5, FLOAT_INT_BIAS)
    if bits == -100:
        return r.read_angle16() if r.read_bit() else 0.0
    return r.read_value(bits)


def read_delta_field(r, base, name, bits):
    """MSG_ReadDeltaField. The writer compares the field pointers, not the
    values, so every field up to the last changed one has its changed bit set;
    then a value that is zero is sent as a single 0 bit."""
    if not r.read_bit():
        return base[name]
    if not r.read_bit():
        return 0.0 if bits in (0, -99, -100) else 0
    if bits == 0:
        return r.read_small_float(5, FLOAT_INT_BIAS)
    if bits == -99:
        return r.read_small_float(2, HUDELEM_COORD_BIAS)
    if bits == -100:
        return r.read_angle16()
    return r.read_value(bits)


def read_delta_struct(r, base, fields):
    """MSG_ReadDeltaStruct after the index: returns None when the struct is
    removed, else the new values (base updated with the first lc fields)."""
    if r.read_bit():
        return None
    out = dict(base)
    if not r.read_bit():
        return out
    lc = r.read_byte()
    if lc > len(fields):
        raise ParseError('field count %d > %d' % (lc, len(fields)))
    for name, bits in fields[:lc]:
        out[name] = read_delta_field(r, base, name, bits)
    return out


def read_delta_fields(r, base, fields):
    """MSG_ReadDeltaFields: one bit, then every field or none."""
    out = dict(base)
    if r.read_bit():
        for name, bits in fields:
            out[name] = read_delta_field(r, base, name, bits)
    return out


def read_hudelems(r):
    out = []
    zero = _zero(HUDELEM_FIELDS)
    for _ in range(r.read_bits(HUDELEM_BITS)):
        lc = r.read_bits(HUDELEM_BITS)
        if lc >= len(HUDELEM_FIELDS):
            raise ParseError('hud element field %d' % lc)
        elem = dict(zero)
        # lc is the index of the last changed field here, not a count
        for name, bits in HUDELEM_FIELDS[:lc + 1]:
            elem[name] = read_delta_field(r, zero, name, bits)
        out.append(elem)
    return out


def read_playerstate(r):
    """MSG_ReadDeltaPlayerstate from a zeroed playerstate."""
    ps = _zero(PLAYERSTATE_FIELDS)
    lc = r.read_byte()
    if lc > len(PLAYERSTATE_FIELDS):
        raise ParseError('playerstate field count %d' % lc)
    for name, bits in PLAYERSTATE_FIELDS[:lc]:
        if r.read_bit():
            ps[name] = read_playerstate_field(r, bits)

    stats = [0] * MAX_STATS
    if r.read_bit():
        mask = r.read_bits(MAX_STATS)
        # health, dead yaw, max health, ident client num and health, spawn count
        for i, read in enumerate((r.read_short, r.read_short, r.read_short,
                                  lambda: r.read_bits(CLIENTNUM_BITS),
                                  r.read_short, r.read_byte)):
            if mask & 1 << i:
                stats[i] = read()
    ps['stats'] = stats

    ammo = {}
    if r.read_bit():
        for j in range(4):
            if r.read_bit():
                mask = r.read_short()
                for i in range(16):
                    if mask & 1 << i:
                        ammo[j * 16 + i] = r.read_short()
    ps['ammo'] = ammo
    clip = {}
    for j in range(4):
        if r.read_bit():
            mask = r.read_short()
            for i in range(16):
                if mask & 1 << i:
                    clip[j * 16 + i] = r.read_short()
    ps['ammoclip'] = clip

    objectives = []
    if r.read_bit():
        zero = _zero(OBJECTIVE_FIELDS)
        for _ in range(MAX_OBJECTIVES):
            state = r.read_bits(OBJECTIVE_STATE_BITS)
            objective = read_delta_fields(r, zero, OBJECTIVE_FIELDS)
            objective['state'] = state
            objectives.append(objective)
    ps['objectives'] = objectives

    ps['hud'] = {'archival': [], 'current': []}
    if r.read_bit():
        ps['hud']['archival'] = read_hudelems(r)
        ps['hud']['current'] = read_hudelems(r)
    return ps


def grouped(values):
    """Joins the fields of one array into a list, so 'pos.trBase[0..2]' becomes
    'pos.trBase': [x, y, z]; a client's name ints become its name string."""
    out = {}
    arrays = {}
    for key, value in values.items():
        if key.endswith(']'):
            name, index = key[:-1].split('[')
            arrays.setdefault(name, {})[int(index)] = value
        else:
            out[key] = value
    for name, items in arrays.items():
        if name == 'name':
            raw = b''.join((items.get(i, 0) & 0xffffffff).to_bytes(4, 'little') for i in range(0, 32, 4))
            out[name] = raw.split(b'\0')[0].decode('latin-1')
        else:
            out[name] = [items.get(i, 0) for i in range(max(items) + 1)]
    return out


class ServerState:
    """What a client keeps between server messages: configstrings, entity
    baselines and the reliable server commands received so far."""

    def __init__(self):
        self.configstrings = {}
        self.baselines = {}
        self.command_sequence = 0       # last server command received
        self.commands = {0: ''}         # sequence -> text, for the XOR keys
        self.client_num = None
        self.checksum_feed = None
        self.gamestate_count = 0

    def _read_gamestate(self, r):
        """svc_gamestate: the reliable sequence (commands up to it came just
        before, in the same message), configstrings, baselines, then the
        client number and checksum feed after an svc_EOF."""
        sequence = r.read_long()
        self.configstrings = {}
        self.baselines = {}
        null = _zero(ENTITY_FIELDS)
        while True:
            op = r.read_byte()
            if op == SVC_EOF:
                break
            if op == SVC_CONFIGSTRING:
                index = r.read_short()
                self.configstrings[index] = r.read_string()
            elif op == SVC_BASELINE:
                number = r.read_bits(GENTITYNUM_BITS)
                base = read_delta_struct(r, null, ENTITY_FIELDS)
                if base is None:
                    raise ParseError('removed baseline %d' % number)
                base['number'] = number
                self.baselines[number] = base
            else:
                raise ParseError('bad gamestate op %d' % op)
        self.client_num = r.read_long()
        self.checksum_feed = r.read_long() & 0xffffffff
        self.command_sequence = sequence
        self.gamestate_count += 1

    def _read_snapshot(self, r):
        snap = {'server_time': r.read_long()}
        delta = r.read_byte()
        if delta:
            raise ParseError('delta snapshot from %d messages back' % delta)
        snap['flags'] = r.read_byte()
        snap['ps'] = read_playerstate(r)

        null = _zero(ENTITY_FIELDS)
        entities = {}
        while True:
            number = r.read_bits(GENTITYNUM_BITS)
            if number == MAX_GENTITIES - 1:
                break
            base = self.baselines.get(number, dict(null, number=number))
            ent = read_delta_struct(r, base, ENTITY_FIELDS)
            if ent is not None:
                ent['number'] = number
                entities[number] = ent
        snap['entities'] = {n: grouped(e) for n, e in entities.items()}

        null = _zero(CLIENT_FIELDS)
        clients = {}
        while r.read_bit():
            number = r.read_bits(CLIENTNUM_BITS)
            cs = read_delta_struct(r, null, CLIENT_FIELDS)
            if cs is not None:
                cs['clientIndex'] = number
                clients[number] = grouped(cs)
        snap['clients'] = clients
        return snap

    def parse(self, data):
        """Parses one decoded, decompressed server message (without the 4-byte
        header). Returns a dict with 'commands' (the new server commands, in
        order), 'snapshot' (or None), 'gamestate' (bool), 'downloads' and
        'clean': True when the message ended exactly on its svc_EOF. A message
        that fails to parse sets 'error' instead."""
        r = MsgReader(data)
        out = {'commands': [], 'snapshot': None, 'gamestate': False,
               'downloads': [], 'clean': False, 'error': None}
        try:
            while True:
                op = r.read_byte()
                if op == SVC_EOF:
                    break
                if op == SVC_NOP:
                    continue
                if op == SVC_SERVERCOMMAND:
                    sequence = r.read_long()
                    text = r.read_string()
                    # unacknowledged commands are sent again in every message
                    if sequence > self.command_sequence:
                        if sequence != self.command_sequence + 1:
                            raise ParseError('server command %d after %d' % (sequence, self.command_sequence))
                        self.command_sequence = sequence
                        self.commands[sequence] = text
                        self.commands.pop(sequence - 256, None)
                        out['commands'].append(text)
                elif op == SVC_GAMESTATE:
                    self._read_gamestate(r)
                    out['gamestate'] = True
                elif op == SVC_SNAPSHOT:
                    out['snapshot'] = self._read_snapshot(r)
                elif op == SVC_DOWNLOAD:
                    block = r.read_short()
                    size = r.read_long() if block == 0 else None
                    if size == -1:
                        out['downloads'].append((block, size, r.read_string()))
                        continue
                    out['downloads'].append((block, size, bytes(r.read_bytes(r.read_short()))))
                else:
                    raise ParseError('bad op %d at byte %d' % (op, r.readcount - 1))
        except ParseError as e:
            out['error'] = str(e)
            return out
        out['clean'] = r.readcount == len(data)
        if not out['clean']:
            out['error'] = '%d bytes after svc_EOF' % (len(data) - r.readcount)
        return out
