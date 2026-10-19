#!/usr/bin/env python3
"""A player shoots another and the game reports where each bullet hit.

    python3 scripts/hit_location_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--listen]

Writes a mod in a temporary home folder whose dm gametype spawns the first
player to join as the target, with a body model and a lot of health, and the
second one a few steps in front of it, facing it, held in place, with a
semi-automatic rifle. The second player is a minimal client (cod2client) that
presses and releases the attack button, first aiming level, at the target's
head, then 15 degrees down, at its legs; each hit makes the mod log the hit
location the game passes to Callback_PlayerDamage. So real shots go through
the server's weapon fire, its locational trace, and the target's DObj and
skeleton, which must put the head and the legs where they are.

Last it fires level again at the target looking up, whose torso and head the
player controllers turn. The target is another minimal client, on the
dedicated server or with --listen in a game a client hosts (its local server
is CoD2rev-Plus's, on the engine's collision and xanim; the host stands
aside); --listen needs a CD key in ~/.cod2/preferences and is skipped
without one. Exits with 0 when both aims hit the target, each
hit is a rifle bullet in the body parts of its aim, and nothing crashed.
"""
import argparse, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import cod2client, smoke_test
from cod2client import CLC_MOVE_NO_DELTA, KEY_MASK_MOVE_FORWARD, hash_key

DEFAULT_BINARY = {
    (115, False): 'build/check-linux-10/cod2_lnxded', (118, False): 'build/check-linux/cod2_lnxded',
    (115, True): 'build/check-linux-10/cod2_linux', (118, True): 'build/check-linux/cod2_linux',
}
RCON = 'hitpw'
BUTTON_MELEE, BUTTON_FRAG = 4, 65536  # q_shared.h
PREFERENCES = os.path.expanduser('~/.cod2/preferences')
WEAPON = 'g43_mp'  # semi-automatic, 10 rounds
DISTANCE = 120
# where the shots aim, in degrees down, where the target looks, and the body
# parts the shots must hit: the target's head is at the shooter's eye height,
# and a target looking up turns its torso and head with its controllers
HEAD = {'head', 'helmet', 'neck', 'torso_upper'}
AIMS = (('level', 0.0, 0.0, HEAD),
        ('down', 15.0, 0.0, {'torso_lower', 'right_leg_upper', 'left_leg_upper', 'right_leg_lower', 'left_leg_lower'}),
        ('level at a target looking up', 0.0, -60.0, HEAD))

GAMETYPE = '''main()
{
	maps\\mp\\gametypes\\_callbacksetup::SetupCallbacks();
	level.callbackStartGameType = ::Callback_StartGameType;
	level.callbackPlayerConnect = ::Callback_PlayerConnect;
	level.callbackPlayerDisconnect = ::Callback_PlayerDisconnect;
	level.callbackPlayerDamage = ::Callback_PlayerDamage;
	level.callbackPlayerKilled = ::Callback_PlayerKilled;
}

Callback_StartGameType()
{
	character\\mp_american_normandy_braeburn::precache();
	precacheItem("%(weapon)s");
}

Callback_PlayerConnect()
{
	self.sessionteam = "none";
	self.sessionstate = "playing";
	self.spectatorclient = -1;
	self.archivetime = 0;
	self.psoffsettime = 0;
	self.statusicon = "";
	self.maxhealth = 100000;
	self.health = self.maxhealth;

	if(self.name != "hittarget" && self.name != "hitshooter")
	{
		// the host of a local game, out of the way
		level.host = self;
		spawnpoints = getentarray("mp_dm_spawn", "classname");
		self spawn(spawnpoints[spawnpoints.size - 1].origin, spawnpoints[spawnpoints.size - 1].angles);
		self character\\mp_american_normandy_braeburn::main();
	}
	else if(!isdefined(level.target))
	{
		// the first spawn point with room for the shooter in front of it
		spawnpoints = getentarray("mp_dm_spawn", "classname");
		for(i = 0; i < spawnpoints.size; i++)
		{
			eye = spawnpoints[i].origin + (0, 0, 60);
			trace = bulletTrace(eye, eye + maps\\mp\\_utility::vectorScale(anglesToForward(spawnpoints[i].angles), %(distance)d + 40), false, undefined);
			// away from a local game's host
			if(trace["fraction"] == 1 && (!isdefined(level.host) || distance(spawnpoints[i].origin, level.host.origin) > 300))
				break;
		}
		level.target = self;
		self spawn(spawnpoints[i].origin, spawnpoints[i].angles);
		self character\\mp_american_normandy_braeburn::main();
		logPrint("hittest target " + self getEntityNumber() + "\\n");
	}
	else
	{
		target = level.target;
		origin = target.origin + maps\\mp\\_utility::vectorScale(anglesToForward(target.angles), %(distance)d);
		self spawn(origin, vectorToAngles(target.origin - origin));
		self character\\mp_american_normandy_braeburn::main();
		self giveWeapon("%(weapon)s");
		self giveMaxAmmo("%(weapon)s");
		self setSpawnWeapon("%(weapon)s");
		logPrint("hittest shooter " + self getEntityNumber() + "\\n");
		self thread hold(origin);
		self thread keepLoaded();
	}
}

// three aims take more rounds than a clip holds
keepLoaded()
{
	for(;;)
	{
		if(self getWeaponSlotClipAmmo("primary") < 6)
			self setWeaponSlotClipAmmo("primary", 10);
		wait 0.1;
	}
}

// the ground in front of a spawn point can slope: the shooter would slide
// off the line of fire
hold(origin)
{
	for(;;)
	{
		self setOrigin(origin);
		wait 0.05;
	}
}

Callback_PlayerDisconnect()
{
}

Callback_PlayerDamage(eInflictor, eAttacker, iDamage, iDFlags, sMeansOfDeath, sWeapon, vPoint, vDir, sHitLoc, psOffsetTime)
{
	// the shooter, held in the air, takes falling damage
	if(self != level.target)
		return;
	logPrint("hittest hit " + sHitLoc + " " + sMeansOfDeath + " " + sWeapon + " " + iDamage + "\\n");
	// a knocked back target would leave the line of fire
	self finishPlayerDamage(eInflictor, eAttacker, iDamage, iDFlags | level.iDFLAGS_NO_KNOCKBACK, sMeansOfDeath, sWeapon, vPoint, vDir, sHitLoc, psOffsetTime);
}

Callback_PlayerKilled(eInflictor, eAttacker, iDamage, sMeansOfDeath, sWeapon, vDir, sHitLoc, psOffsetTime, deathAnimDuration)
{
	logPrint("hittest killed " + sHitLoc + "\\n");
}
''' % {'weapon': WEAPON, 'distance': DISTANCE}


class Shooter(cod2client.Client):
    """A minimal client that sends the full usercmd, the one with the weapon
    and the pitch; it stands still unless walking (forward), and while firing
    it presses the attack button on every other command, or the melee one for
    6 commands out of 12; the frag grenade button it holds for frag_commands."""
    weapon = 0
    firing = False
    attack = False
    pitch = 0.0  # degrees down from the view the spawn gives
    walking = False  # the forward key
    melee = False  # the melee button, while firing
    frag_commands = 0  # the frag grenade button, for this many more commands

    def _parse(self, sequence, data):
        # the commands follow the server's clock: from 1000 by 50 a command,
        # they fall seconds behind a server that took long to start, which
        # clamps them and fires nothing
        result = super()._parse(sequence, data)
        if result.get('snapshot'):
            self.server_time = max(self.server_time, result['snapshot']['server_time'])
        return result

    def _write_move(self, payload, forward):
        message_ack = self.incoming_sequence
        key = (self.checksum_feed ^ message_ack ^ hash_key(self._acknowledged()[1])) & 0xffffffff
        self.server_time += 50
        # a press every other command; a melee takes longer, held for 6
        self.presses = getattr(self, 'presses', 0) + 1
        self.attack = self.firing and (self.presses // 6 % 2 == 0 if self.melee else not self.attack)
        frag = self.frag_commands > 0
        self.frag_commands -= frag

        # MSG_ReadDeltaUsercmdKey against the null command
        payload.bits(CLC_MOVE_NO_DELTA, 3)
        payload.byte(1)                          # command count
        payload.bits(0, 1)                       # absolute server time
        payload.long(self.server_time)
        payload.bits(1 ^ (key & 1), 1)           # something changed
        payload.bits(1 ^ (key & 1), 1)           # not only buttons, angles and move
        payload.bits(int(self.attack and not self.melee and not frag) ^ (key & 1), 1)
        # a field left unchanged keeps the server's previous command's value,
        # so the pitch, the move keys and the buttons are always sent
        payload.bits(1, 1)
        payload.bits((int(self.pitch * 65536 / 360) & 0xffff) ^ (key & 0xffff), 16)
        payload.bits(0, 1)                       # yaw unchanged
        payload.bits(1, 1)
        payload.bits((KEY_MASK_MOVE_FORWARD if self.walking else 0) ^ (key & 0xf), 4)
        key ^= self.server_time
        payload.bits(0, 1)                       # roll unchanged
        buttons = (BUTTON_MELEE if self.melee and self.attack else 0) | (BUTTON_FRAG if frag else 0)
        payload.bits(1, 1)
        payload.bits((buttons >> 1) ^ (key & 0x3ffff), 18)
        if self.weapon:
            payload.bits(1, 1)
            payload.bits(self.weapon ^ (key & 0x7f), 7)
        else:
            payload.bits(0, 1)                   # no weapon
        payload.bits(0, 1)                       # no offhand


def game_log(home):
    path = os.path.join(home, 'hittest', 'games_mp.log')
    return open(path, encoding='latin-1').read() if os.path.exists(path) else ''


def server_id(port):
    m = re.search(r'"sv_serverid" is: ?"(-?\d+)', smoke_test.rcon(port, RCON, 'sv_serverid'))
    return int(m.group(1)) if m else None


def join(port, protocol, name):
    """A minimal client in the game, or the reason it is not."""
    sid = server_id(port)
    if sid is None:
        return None, 'no sv_serverid'
    client = Shooter(port)
    try:
        refused = client.connect(protocol, name)
    except OSError as e:
        refused = f'no reply ({e})'
    if refused is not None:
        client.close()
        return None, refused
    if not client.request_gamestate(sid):
        client.close()
        return None, 'no gamestate'
    client.enter_game(sid, 1.0)
    return client, ''


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary')
    ap.add_argument('--listen', action='store_true', help='the target hosts the game in a client')
    ap.add_argument('--map', default='mp_toujane')
    args = ap.parse_args()
    if args.listen and (not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read()):
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_BINARY[(args.protocol, args.listen)]))
    port = smoke_test.free_port()
    failures = []
    clients = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2_hit_') as home:
        mod = os.path.join(home, 'hittest', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(GAMETYPE)
        command = [binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                   '+set', 'fs_game', 'hittest', '+set', 'net_port', str(port), '+set', 'sv_cracked', '1',
                   '+set', 'rcon_password', RCON, '+set', 'g_gametype', 'dm', '+set', 'g_log', 'games_mp.log',
                   '+set', 'g_logSync', '1', '+set', 'developer', '1', '+set', 'sv_maxclients', '8']
        command += ['+set', 'r_fullscreen', '0'] if args.listen else ['+set', 'dedicated', '1']
        log_path = os.path.join(home, 'out.log')
        with open(log_path, 'wb') as log:
            server = subprocess.Popen(command + ['+map', args.map], stdin=subprocess.DEVNULL, stdout=log,
                                      stderr=subprocess.STDOUT, cwd=home)
        try:
            deadline = time.time() + 150
            while time.time() < deadline and server.poll() is None and not smoke_test.query(port, 'getinfo x'):
                time.sleep(1)
            check('the server runs the map', server.poll() is None and smoke_test.query(port, 'getinfo x') is not None)
            if not failures:
                target, why = join(port, args.protocol, 'hittarget')
                check('the target joins', target is not None, why)
                if target:
                    clients.append(target)
            if not failures:
                smoke_test.wait_for(lambda: 'hittest target' in game_log(home), 60)
                check('the target spawns', 'hittest target' in game_log(home))
                # the shooter is placed from where the target stands: let it land
                for _ in range(8):
                    for c in clients:
                        c.read_frames(0.25, False)
                    if not clients:
                        time.sleep(0.25)
            if not failures:
                shooter, why = join(port, args.protocol, 'hitshooter')
                check('the shooter joins', shooter is not None, why)
                if shooter:
                    clients.append(shooter)
            if not failures:
                weapon = 0
                deadline = time.time() + 10
                while not weapon and time.time() < deadline:
                    for frame in shooter.read_frames(0.5, False):
                        for c in clients[:-1]:
                            c.read_frames(0.05, False)
                        weapon = frame['ps'].get('weapon', 0) or weapon
                check('the shooter holds a weapon', weapon != 0, f'weapon {weapon}')
            if not failures:
                shooter.weapon = weapon
                for _ in range(4):  # the rifle's raise
                    shooter.read_frames(0.25, False)
                    for c in clients[:-1]:
                        c.read_frames(0.05, False)
                for name, pitch, target_pitch, expected in AIMS:
                    # aim, hold fire until the last shot at the previous aim lands
                    shooter.firing = False
                    shooter.pitch = pitch
                    clients[0].pitch = target_pitch
                    for _ in range(3):
                        shooter.read_frames(0.25, False)
                        for c in clients[:-1]:
                            c.read_frames(0.05, False)
                    before = game_log(home).count('hittest hit ')
                    shooter.firing = True
                    deadline = time.time() + 8
                    while time.time() < deadline and game_log(home).count('hittest hit ') < before + 3:
                        shooter.read_frames(0.3, False)
                        for c in clients[:-1]:
                            c.read_frames(0.05, False)
                    hits = re.findall(r'hittest hit (\S*) (\S+) (\S+) (\d+)', game_log(home))[before:]
                    parts = [h[0] for h in hits]
                    check(f'the shots aimed {name} hit the target', len(hits) > 0, f'{len(hits)} hits')
                    check(f'the shots aimed {name} hit {"/".join(sorted(expected))}',
                          hits and all(p in expected for p in parts), ', '.join(parts))
                    check(f'the shots aimed {name} are rifle bullets',
                          hits and all(h[1] == 'MOD_RIFLE_BULLET' and h[2] == WEAPON for h in hits),
                          ', '.join(f'{h[1]} {h[2]}' for h in hits))
            time.sleep(1)
            check('the server still runs', server.poll() is None and smoke_test.query(port, 'getinfo x') is not None,
                  f'exit code {server.poll()}')
        finally:
            for c in clients:
                c.close()
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    server.kill()
        text = open(log_path, encoding='latin-1').read()
        check('no crash report', 'CRASH REPORT' not in text)
        if failures:
            print('\n--- hittest lines of the game log ---')
            print('\n'.join(l for l in game_log(home).splitlines() if 'hittest' in l)[-2000:])
            print('\n--- last lines of the server log ---')
            print('\n'.join(l for l in text.splitlines() if not l.startswith(('[ckpt]', '[scan', '[rbdiag', '[cnx]', 'FX Error')))[-3000:])
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
