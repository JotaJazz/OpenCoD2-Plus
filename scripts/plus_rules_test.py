#!/usr/bin/env python3
"""Checks the rules a Plus server plays by with a Plus client (Plus version 123,
docs/architecture/plus-protocol.md#version-123).

    python3 scripts/plus_rules_test.py /path/to/CoD2 [--binary bin/cod2rev_lnxded] [--only NAME,...] [--fps N]

Runs the server with a mod whose dm gametype spawns the player with
thompson_mp and a bolt-action, and a minimal client (cod2client) that sends
scripted usercmds and reads its playerstate from the snapshots:

- fast reload: for each bolt-action (and the shotgun), it fires, presses
  reload, switches to the thompson and back while the rifle still drops, at
  several times around the bolt, then fires as soon as it can. No time between
  the two shots may be shorter than with no switch, give or take a command
  (50 ms, how closely the snapshots time a shot). The shotgun gains about
  that much on a stock build, so only the rifles show the bug there.
- prone peek: crouched and leaning, it goes prone, crouches and goes prone
  again, to each side. The lean, which halves going prone, may come back no
  faster than when the lean key is let go (0.5 in 280 ms): it used to halve
  in one frame.
- mg clip: it takes a turret from 20 units to its side, which moves it behind
  the gun, and leaves it with the use button. It must stay where the turret
  held it: the stock game takes it back to where it stood when it took the
  turret, behind cover if it stood there. Leaving adds no playerstate event
  but the one that restores the stance.
- shotgun spread: turned to a wall the whole spread meets, with
  g_shotgun_spread_fix 1, the distances of a shot's 8 impacts from the aim
  must be one of the two patterns', whatever the turn, and both must come;
  with 0 they must not. This is not a Plus rule: it passes on every build.
- mg rate: on the turret, aimed at the ground, it holds fire: one impact
  every 50 ms (mg42's fireTime), timed from the first to the last; left, the
  turret turns back at 200 degrees a second. With --fps N the server runs at
  that sv_fps, with a snapshot every frame; other builds fail above 20 Hz.
- snapshot rate: with sv_lanForceRate 0, so the test client counts as one on
  the internet, it gets a snapshot every frame and a rate of 1,000 bytes a
  second per Hz (25,000 at least), and g_forceSnaps 10 set by rcon gives it 10
  snapshots a second at once. Other builds have no sv_lanForceRate.
- archive time: a bot moves 100 units a second, and the player follows it a
  second back, as a killcam: the bot's origin must be from the newest frame
  at least a second old. Stock counted sv_fps frames a second, 960 ms at
  sv_fps 60.
- lean rewind: a second client 600 ms behind, down the sights with no
  spread, shoots at where the player's head was while it leaned, after it
  let go of the lean: the antilag must hit the head, which only the Plus
  build's pose rewind does. A shot into the wall over the head first measures
  how far the bullet leaves from the view. It sets g_antilag 1, which a game
  folder's config can turn off.
- stance rewind: the same client shoots at where the player's head was
  before it crouched, went prone or stood up from a crouch, once its head
  has left it: the antilag must rewind the animation of the stance, not only its
  flags, which only the Plus build does.

Exits with 0 when every check passes. On a build without the rules (any
build but make PLUS=1) the other checks fail: that is how they catch the bugs.
"""
import argparse, math, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import cod2client, smoke_test
from cod2client import CLC_MOVE_NO_DELTA, hash_key

RCON = 'plusrules'
LOG = None  # the server's console log, set by main
BUTTON_ATTACK, BUTTON_RELOAD, BUTTON_LEANLEFT, BUTTON_LEANRIGHT = 1, 16, 64, 128  # q_shared.h
PRONE, CROUCH = 256, 512  # BUTTON_PRONE, BUTTON_CROUCH: the stance held
BUTTON_USE = 8
BUTTON_ADS = 4096  # q_shared.h
EF_TURRET_ACTIVE = 0x300  # bg_public.h
STANCE_EVENTS = {140, 141, 142}  # EV_STANCE_FORCE_STAND, _CROUCH, _PRONE
WEAPON_READY, WEAPON_FIRING = 0, 3     # bg_public.h weaponstate_t
OTHER = 'thompson_mp'
# zPAM's list of the weapons fast reload works with
RIFLES = ('kar98k_mp', 'kar98k_sniper_mp', 'enfield_mp', 'enfield_scope_mp', 'mosin_nagant_mp',
          'mosin_nagant_sniper_mp', 'springfield_mp', 'shotgun_mp')

GAMETYPE = '''main()
{
	maps\\mp\\gametypes\\_callbacksetup::SetupCallbacks();
	level.callbackStartGameType = ::Callback_StartGameType;
	level.callbackPlayerConnect = ::Callback_PlayerConnect;
	level.callbackPlayerDisconnect = ::Callback_Nothing;
	level.callbackPlayerDamage = ::Callback_PlayerDamage;
	level.callbackPlayerKilled = ::Callback_Nothing;
}

// moves along x at 100 units a second, set every frame, so where it is
// tells when it was there; plustest_move holds the start time and x
plustest_move(base)
{
	t0 = getTime();
	setCvar("plustest_move", t0 + " " + base[0] + " " + self getEntityNumber());
	for(;;)
	{
		self setOrigin(base + ((getTime() - t0) * 0.1, 0, 0));
		wait 1 / getCvarInt("sv_fps");
	}
}

// records where a bullet hit; the damage is not applied
Callback_PlayerDamage(eInflictor, eAttacker, iDamage, iDFlags, sMeansOfDeath, sWeapon, vPoint, vDir, sHitLoc, psOffsetTime)
{
	setCvar("plustest_hit", sHitLoc);
}

Callback_Nothing(a, b, c, d, e, f, g, h, i, j)
{
}

Callback_StartGameType()
{
	character\\mp_american_normandy_braeburn::precache();
	// in this order the rifles take the indexes after the thompson's
	precacheItem("%(other)s");
%(precache)s
	precacheTurret("mg42_bipod_stand_mp");
	precacheModel("xmodel/weapon_mg42");
	// the frame archive, which the antilag and the killcam read, as stock gametypes
	setArchive(true);
	setCvar("plustest_weapon", "");
	setCvar("plustest_turret", "");
	setCvar("plustest_aim", "");
	setCvar("plustest_pitch", "");
	setCvar("plustest_killcam", "");
	setCvar("plustest_duel", "");
	setCvar("plustest_head", "");
	setCvar("plustest_fire", "");
	setCvar("plustest_hit", "");
	setCvar("plustest_wait", "");
	level thread waits();
	level thread errors();
}

// plustest_error (rcon) makes the same runtime error every frame for 11 s
errors()
{
	for(;;)
	{
		if(getCvar("plustest_error") != "")
		{
			setCvar("plustest_error", "");
			for(i = 0; i < 11 * 20; i++)
			{
				x = level.plustest_undefined + 7777;
				wait 0.05;
			}
			setCvar("plustest_errored", "1");
		}
		wait 0.05;
	}
}

// plustest_wait (rcon) times 40 x wait 0.05 and 4 x wait 1 into plustest_waited
waits()
{
	for(;;)
	{
		if(getCvar("plustest_wait") != "")
		{
			setCvar("plustest_wait", "");
			t = getTime();
			for(i = 0; i < 40; i++)
				wait 0.05;
			short = getTime() - t;
			t = getTime();
			for(i = 0; i < 4; i++)
				wait 1;
			setCvar("plustest_waited", short + " " + (getTime() - t));
		}
		wait 0.05;
	}
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
	// mp_toujane's second spawn point is level: a turret can be used there
	spawnpoints = getentarray("mp_dm_spawn", "classname");
	self spawn(spawnpoints[1].origin, spawnpoints[1].angles);
	self character\\mp_american_normandy_braeburn::main();
	self giveWeapon("%(other)s");
	self giveMaxAmmo("%(other)s");
	self setSpawnWeapon("%(other)s");
	logPrint("plustest spawned\\n");
	self thread weapons();
}

// plustest_weapon (rcon) gives a rifle next to the thompson; a reload
// is only allowed with room in the clip, and a shot needs a round
weapons()
{
	last = "";
	for(;;)
	{
		weapon = getCvar("plustest_weapon");
		if(weapon != last && weapon != "")
		{
			last = weapon;
			self takeAllWeapons();
			self giveWeapon(weapon);
			self giveMaxAmmo(weapon);
			self setWeaponSlotWeapon("primaryb", "%(other)s");
			self giveMaxAmmo("%(other)s");
			logPrint("plustest weapon " + weapon + "\\n");
		}
		if(last != "" && self getWeaponSlotClipAmmo("primary") < 2)
			self setWeaponSlotClipAmmo("primary", 2);
		// plustest_turret (rcon) puts a turret in front and has the player
		// take it from 20 units to its left, where it moves them behind it
		// takes it again from where leaving it put them
		if(getCvar("plustest_turret") == "1")
		{
			setCvar("plustest_turret", "");
			if(!isdefined(self.turret))
			{
				self.turret = spawnTurret("misc_turret", self.origin + maps\\mp\\_utility::vectorScale(anglesToForward(self.angles), 60) + (0, 0, 30), "mg42_bipod_stand_mp");
				self.turret.angles = self.angles;
				self.turret setModel("xmodel/weapon_mg42");
				self setOrigin(self.origin - maps\\mp\\_utility::vectorScale(anglesToRight(self.angles), 20));
				wait 0.2;
			}
			logPrint("plustest turret " + self useTurret(self.turret) + "\\n");
		}
		// plustest_killcam (rcon): a bot joins, and the player follows it that
		// many seconds back, as a killcam does; only the test client (entity 0)
		if(getCvar("plustest_killcam") != "" && self getEntityNumber() == 0)
		{
			archive = getCvarFloat("plustest_killcam");
			setCvar("plustest_killcam", "");
			// the bot needs more than that many seconds in the archive
			bot = addtestclient();
			wait 0.5;
			bot thread plustest_move(self.origin);
			wait archive + 2;
			self.sessionstate = "spectator";
			self.spectatorclient = bot getEntityNumber();
			self.archivetime = archive;
			self.psoffsettime = 0;
			logPrint("plustest killcam " + self.spectatorclient + "\\n");
		}
		// plustest_duel (rcon): the player goes to a spawn point with 200 units
		// clear in front, away from the turret of mg_clip; the second client
		// (entity 1) stands there, facing the player, and shoots with no spread. plustest_head
		// records where the player's head is (plustest_pos: head, then the
		// shooter's eye); plustest_aim turns the shooter to plustest_up units
		// above it, plus plustest_dp and plustest_dy degrees; plustest_fire
		// has it shoot
		if(self getEntityNumber() == 0 && getCvar("plustest_duel") != "")
		{
			setCvar("plustest_duel", "");
			players = getentarray("player", "classname");
			for(i = 0; i < players.size; i++)
				if(players[i] getEntityNumber() == 1)
					level.plustest_shooter = players[i];
			spawnpoints = getentarray("mp_dm_spawn", "classname");
			for(i = 0; i < spawnpoints.size; i++)
			{
				forward = anglesToForward((0, spawnpoints[i].angles[1], 0));
				eye = spawnpoints[i].origin + (0, 0, 60);
				if(i != 1 && bulletTrace(eye, eye + maps\\mp\\_utility::vectorScale(forward, 240), false, undefined)["fraction"] == 1)
					break;
			}
			self setOrigin(spawnpoints[i].origin);
			self setPlayerAngles((0, spawnpoints[i].angles[1], 0));
			wait 0.5;
			forward = anglesToForward((0, self.angles[1], 0));
			level.plustest_shooter setOrigin(self.origin + maps\\mp\\_utility::vectorScale(forward, 200));
			level.plustest_shooter setPlayerAngles((0, self.angles[1] + 180, 0));
			level.plustest_shooter setWeaponSpreadScale(0);
			logPrint("plustest duel\\n");
		}
		if(self getEntityNumber() == 0 && getCvar("plustest_head") != "")
		{
			setCvar("plustest_head", "");
			level.plustest_headpos = self getTagOrigin("j_head");
			h = level.plustest_headpos;
			e = level.plustest_shooter getEye();
			setCvar("plustest_pos", h[0] + " " + h[1] + " " + h[2] + " " + e[0] + " " + e[1] + " " + e[2]);
		}
		if(self getEntityNumber() == 0 && getCvar("plustest_aim") == "head")
		{
			setCvar("plustest_aim", "");
			shooter = level.plustest_shooter;
			target = level.plustest_headpos + (0, 0, getCvarFloat("plustest_up"));
			shooter setPlayerAngles(vectorToAngles(target - shooter getEye()) + (getCvarFloat("plustest_dp"), getCvarFloat("plustest_dy"), 0));
		}
		if(self getEntityNumber() == 0 && getCvar("plustest_fire") != "")
		{
			setCvar("plustest_fire", "");
			setCvar("plustest_hit", "none");
			level.plustest_shooter forceShot();
		}
		// plustest_pitch (rcon) sets the player's pitch, on a turret too
		if(getCvar("plustest_pitch") != "")
		{
			self setPlayerAngles((getCvarFloat("plustest_pitch"), self.angles[1], 0));
			setCvar("plustest_pitch", "");
		}
		// plustest_aim (rcon) turns the player level to a wall that the whole
		// shotgun spread (6 degrees) meets within the pellets' 800 units
		if(getCvar("plustest_aim") == "1")
		{
			setCvar("plustest_aim", "");
			for(yaw = 0; yaw < 360; yaw += 5)
			{
				near = true;
				for(i = 0; i < 9 && near; i++)
				{
					end = self getEye() + maps\\mp\\_utility::vectorScale(anglesToForward((i %% 3 * 7 - 7, yaw + int(i / 3) * 7 - 7, 0)), 800);
					fraction = bulletTrace(self getEye(), end, false, self)["fraction"];
					near = fraction > 0.4 && fraction < 0.9;
				}
				if(near)
					break;
			}
			self setPlayerAngles((0, yaw, 0));
			logPrint("plustest aim " + yaw + "\\n");
		}
		wait 0.05;
	}
}
''' % {'other': OTHER, 'precache': '\n'.join('\tprecacheItem("%s");' % r for r in RIFLES)}


class Player(cod2client.Client):
    """A minimal client that sends its buttons and weapon with every usercmd:
    the next of script's (buttons, weapon) pairs, then no buttons and the
    weapon of the last one."""
    weapon = 0
    script = ()
    lag = 0  # ms its commands' serverTime stays behind the server's
    hold = 0  # buttons held in every command

    def _parse(self, sequence, data):
        # the commands follow the server's clock, which clamps commands that
        # fall behind it
        result = super()._parse(sequence, data)
        if result.get('snapshot'):
            self.server_time = max(self.server_time, result['snapshot']['server_time'])
        return result

    def _write_move(self, payload, forward):
        message_ack = self.incoming_sequence
        key = (self.checksum_feed ^ message_ack ^ hash_key(self._acknowledged()[1])) & 0xffffffff
        self.server_time += 50
        if self.script:
            buttons, self.weapon = self.script[0]
            self.script = self.script[1:]
        else:
            buttons = 0
        buttons |= self.hold
        # MSG_ReadDeltaUsercmdKey against the null command; a field left
        # unchanged keeps the previous command's value, so all are sent
        payload.bits(CLC_MOVE_NO_DELTA, 3)
        payload.byte(1)                                  # command count
        payload.bits(0, 1)                               # absolute server time
        command_time = self.server_time - self.lag
        payload.long(command_time)
        payload.bits(1 ^ (key & 1), 1)                   # something changed
        payload.bits(1 ^ (key & 1), 1)                   # not only buttons, angles and move
        payload.bits((buttons & BUTTON_ATTACK) ^ (key & 1), 1)
        payload.bits(0, 1)                               # pitch unchanged
        payload.bits(0, 1)                               # yaw unchanged
        payload.bits(1, 1)
        payload.bits(0 ^ (key & 0xf), 4)                 # no move keys
        key ^= command_time
        payload.bits(0, 1)                               # roll unchanged
        payload.bits(1, 1)
        payload.bits((buttons >> 1) ^ (key & 0x3ffff), 18)
        payload.bits(1, 1)
        payload.bits(self.weapon ^ (key & 0x7f), 7)
        payload.bits(0, 1)                               # no offhand


def frames_until(player, done, seconds):
    """Snapshots' playerstates until done(playerstates) or seconds pass."""
    states = []
    deadline = time.time() + seconds
    while time.time() < deadline and not done(states):
        states += [f['ps'] for f in player.read_frames(0.1, False)]
    return states


def settle(player, weapon, seconds=6):
    """Holds weapon until it is ready and stays so for half a second."""
    player.weapon = weapon
    ready = lambda s: (len(s) >= 6 and all(p.get('weapon') == weapon and p.get('weaponstate', 0) == WEAPON_READY
                                           and not p.get('weaponTime') for p in s[-6:]))
    return ready(frames_until(player, ready, seconds))


def shot_interval(player, script, seconds=6):
    """Runs script from a ready weapon; the time from its first shot to the
    next one, in ms, or None."""
    player.script = tuple(script)
    starts = lambda s: [p.get('commandTime', 0) for i, p in enumerate(s)
                        if p.get('weaponstate') == WEAPON_FIRING and (i == 0 or s[i - 1].get('weaponstate') != WEAPON_FIRING)]
    states = frames_until(player, lambda s: len(starts(s)) >= 2, seconds)
    player.script = ()
    shots = starts(states)
    return shots[1] - shots[0] if len(shots) >= 2 else None


def check_fast_reload(port, player, other, check):
    for i, rifle in enumerate(RIFLES):
        index = other + 1 + i
        smoke_test.rcon(port, RCON, f'set plustest_weapon {rifle}')
        if not settle(player, index):
            check(f'fast reload: {rifle} is ready in hand', False, f'weapon {index}')
            continue
        fire_again = [(BUTTON_ATTACK, index), (0, index)] * 40
        # the snapshots time a shot to a command (50 ms): the shortest of two
        plains = []
        for _ in range(2):
            settle(player, index)
            plains.append(shot_interval(player, [(BUTTON_ATTACK, index), (0, index)] + fire_again))
        plain = min(plains) if None not in plains else None
        worst = None
        # reload 100 ms after the shot; the switch back follows the switch
        # away by one command, within the 250 ms drop
        for wait in range(6, 26, 2):
            settle(player, index)
            script = [(BUTTON_ATTACK, index), (0, index), (BUTTON_RELOAD, index)] + [(0, index)] * wait
            script += [(0, other)] + fire_again
            interval = shot_interval(player, script)
            if interval is not None and (worst is None or interval < worst[0]):
                worst = (interval, wait)
        check(f'fast reload: switching does not shorten {rifle}\'s shots',
              plain is not None and worst is not None and worst[0] >= plain - 50,
              f'{plain} ms between shots, {worst[0] if worst else None} ms with the switch '
              f'{(worst[1] + 3) * 50 if worst else "?"} ms after the shot')
        settle(player, index)


def check_prone_peek(port, player, other, check):
    # prone, crouch and back to prone while leaning, as a peek over cover
    for side, lean in (('right', BUTTON_LEANRIGHT), ('left', BUTTON_LEANLEFT)):
        settle(player, other)
        held = [CROUCH | lean] * 30 + [PRONE | lean] * 20 + [CROUCH | lean] * 6 + [PRONE | lean] * 20
        player.script = tuple((b, other) for b in held + [CROUCH] * 30)
        states = frames_until(player, lambda s: not player.script, len(player.script) * 0.05 + 5)
        fastest, leaned = 0.0, 0.0
        for a, b in zip(states, states[1:]):
            dt = b.get('commandTime', 0) - a.get('commandTime', 0)
            leaned = max(leaned, abs(b.get('leanf', 0)))
            if dt > 0:
                fastest = max(fastest, (abs(a.get('leanf', 0)) - abs(b.get('leanf', 0))) / dt)
        # letting go of a lean brings 0.5 back in 280 ms (PM_UpdateLean)
        check(f'prone peek: going prone leaning {side}, the lean comes back no faster than let go',
              leaned >= 0.45 and fastest <= 0.5 / 280 * 1.01,
              f'leaned up to {leaned:.2f}, back at up to {fastest * 280:.2f} per 280 ms')


def take_turret(port, player):
    """Has the player take the turret, twice if the first useTurret did not
    take (now and then it does not); the playerstates until it is on."""
    for _ in range(2):
        smoke_test.rcon(port, RCON, 'set plustest_turret 1')
        on = frames_until(player, lambda s: s and s[-1].get('eFlags', 0) & EF_TURRET_ACTIVE, 5)
        if on and on[-1].get('eFlags', 0) & EF_TURRET_ACTIVE:
            break
    return on


def check_mg_clip(port, player, other, check):
    settle(player, other)
    on = take_turret(port, player)
    check('mg clip: the player takes the turret', bool(on) and on[-1].get('eFlags', 0) & EF_TURRET_ACTIVE)
    if not on or not on[-1].get('eFlags', 0) & EF_TURRET_ACTIVE:
        return
    on += frames_until(player, lambda s: len(s) >= 10, 2)
    held = [on[-1].get(f'origin[{i}]', 0) for i in range(3)]
    events = {on[-1].get(f'events[{i}]') for i in range(4)}
    # the use button leaves it
    player.script = tuple([(BUTTON_USE, other)] * 2)
    off = frames_until(player, lambda s: len(s) >= 20 and not s[-1].get('eFlags', 0) & EF_TURRET_ACTIVE, 5)
    left = [off[-1].get(f'origin[{i}]', 0) for i in range(3)] if off else held
    moved = sum((a - b) ** 2 for a, b in zip(held, left)) ** 0.5
    check('mg clip: leaving the turret keeps the player where it held them, behind it', moved < 2,
          f'{moved:.1f} units from there')
    new = {p.get(f'events[{i}]') for p in off for i in range(4)} - events - {None, 0}
    check('mg clip: leaving the turret adds no event but the stance one', new <= STANCE_EVENTS, f'events {sorted(new)}')


ET_TURRET, ET_EVENTS, EV_BULLET_HIT_SMALL, EV_SHOTGUN_HIT = 9, 10, 181, 183  # g_shared.h
# g_shotgun_spread_fix's patterns (g_weapon_mp.cpp): each pellet's distance
# from the aim, as a fraction of the farthest one's
SPREAD_PATTERNS = [sorted(x for x in (0, .4, .4, .4, .4, .8, .72 ** .5, .72 ** .5)),
                   [.1, .1, .5, .5, 1, 1, 1, 1]]
SPREAD_PATTERNS = [[r / p[-1] for r in p] for p in SPREAD_PATTERNS]


def shotgun_pellets(player, index):
    """Fires the shotgun once; its pellets' distances from the aim, as above,
    from the impacts the server sends, or the count of impacts if not 8."""
    settle(player, index)
    ps = frames_until(player, lambda s: s, 2)[-1]
    eye = [ps.get('origin[0]', 0), ps.get('origin[1]', 0), ps.get('origin[2]', 0) + ps.get('viewHeightCurrent', 0)]
    pitch, yaw = (math.radians(ps.get(f'viewangles[{i}]', 0)) for i in (0, 1))
    forward = [math.cos(pitch) * math.cos(yaw), math.cos(pitch) * math.sin(yaw), -math.sin(pitch)]
    right = [math.sin(yaw), -math.cos(yaw), 0]
    up = [right[1] * forward[2] - right[2] * forward[1], right[2] * forward[0] - right[0] * forward[2],
          right[0] * forward[1] - right[1] * forward[0]]
    player.script = ((BUTTON_ATTACK, index), (0, index))
    hits = {}
    for frame in player.read_frames(1.5, False):
        for n, e in frame['entities'].items():
            if e.get('eType', 0) - ET_EVENTS in range(EV_BULLET_HIT_SMALL, EV_SHOTGUN_HIT + 1):
                hits[n] = e['pos.trBase']
    if len(hits) != 8:
        return len(hits)
    radii = []
    for hit in hits.values():
        v = [h - o for h, o in zip(hit, eye)]
        depth = sum(a * b for a, b in zip(v, forward))
        radii.append(math.hypot(sum(a * b for a, b in zip(v, right)), sum(a * b for a, b in zip(v, up))) / depth)
    radii.sort()
    return [r / radii[-1] for r in radii]


def check_shotgun_spread(port, player, other, check):
    index = other + RIFLES.index('shotgun_mp') + 1
    smoke_test.rcon(port, RCON, 'set plustest_weapon shotgun_mp')
    smoke_test.rcon(port, RCON, 'set g_shotgun_spread_fix 1')
    smoke_test.rcon(port, RCON, 'set plustest_aim 1')
    # a pellet that meets nothing within its 800 units sends no impact: those
    # shots are left out, though the aim at a wall should leave none
    shots = [r for r in (shotgun_pellets(player, index) for _ in range(12)) if isinstance(r, list)]
    smoke_test.rcon(port, RCON, 'set g_shotgun_spread_fix 0')
    # the impacts are whole units, 320 to 720 units away; the patterns differ by .37 or more
    which = lambda r: [i for i, p in enumerate(SPREAD_PATTERNS) if max(abs(a - b) for a, b in zip(r, p)) < .08]
    fit = lambda r: bool(which(r))
    # each pattern is picked at random: 12 shots miss one with odds of 1 in 2000
    seen = {i for r in shots for i in which(r)}
    check('shotgun spread: with g_shotgun_spread_fix every shot is one of the patterns, and both come',
          len(shots) >= 10 and all(fit(r) for r in shots) and seen == {0, 1},
          f'{len(shots)} shots with 8 impacts, patterns {sorted(seen)}')
    stock = [r for r in (shotgun_pellets(player, index) for _ in range(4)) if isinstance(r, list)]
    check('shotgun spread: without it the pellets are random', stock and not any(fit(r) for r in stock),
          f'{len(stock)} shots')


MG42_FIRE_TIME = 50  # mg42_bipod_stand_mp's fireTime, ms
TURRET_RETURN_SPEED = 200  # degrees a second, turret_UpdateTargetAngles unmanned


def check_mg_rate(port, player, other, check):
    """Manned, the MG fires once a fireTime at any sv_fps; left, it turns back
    to its rest at the same speed."""
    settle(player, other)
    on = take_turret(port, player)
    if not on or not on[-1].get('eFlags', 0) & EF_TURRET_ACTIVE:
        check('mg rate: the player takes the turret', False)
        return
    # aimed at the ground, every bullet sends an impact
    smoke_test.rcon(port, RCON, 'set plustest_pitch 15')
    frames_until(player, lambda s: len(s) >= 10, 2)
    hold = 60
    player.script = tuple([(BUTTON_ATTACK, other)] * hold)
    times, last = [], {}
    for frame in player.read_frames(hold * 0.05 + 1.5, False):
        # an impact lasts several snapshots: count it when it appears
        now = {n: tuple(e['pos.trBase']) for n, e in frame['entities'].items()
               if e.get('eType', 0) - ET_EVENTS in range(EV_BULLET_HIT_SMALL, EV_SHOTGUN_HIT + 1)}
        times += [frame['server_time']] * sum(1 for n, o in now.items() if last.get(n) != o)
        last = now
    # how long the server holds the button is not known here: the rate is
    # timed from the first impact to the last
    interval = (times[-1] - times[0]) / (len(times) - 1) if len(times) > 20 else 0
    check(f'mg rate: a manned MG fires once a fireTime at sv_fps {player.fps}',
          abs(interval - MG42_FIRE_TIME) <= MG42_FIRE_TIME * 0.05,
          f'{len(times)} shots, one each {interval:.1f} ms, {MG42_FIRE_TIME} expected')
    turret = next((n for n, e in frame['entities'].items() if e.get('eType') == ET_TURRET), None)
    player.script = tuple([(BUTTON_USE, other)] * 2)
    angles = []
    for frame in player.read_frames(3, False):
        e = frame['entities'].get(turret)
        if e:
            angles.append((frame['server_time'], e['angles2'][0]))
    # the fastest it turns over a quarter of a second, which holds whole steps
    # of a 50 ms think as of a frame
    rates = []
    for i, a in enumerate(angles):
        b = next((b for b in angles[i:] if b[0] - a[0] >= 250), None)
        if b:
            rates.append(abs(b[1] - a[1]) * 1000 / (b[0] - a[0]))
    fastest = max(rates, default=0)
    check(f'mg rate: a turret left turns back at {TURRET_RETURN_SPEED} degrees a second at sv_fps {player.fps}',
          abs(fastest - TURRET_RETURN_SPEED) <= TURRET_RETURN_SPEED * 0.1, f'{fastest:.0f} degrees a second')


def snapshots_per_second(player, seconds=3):
    frames = player.read_frames(seconds, False)
    span = frames[-1]['server_time'] - frames[0]['server_time'] if len(frames) > 1 else 0
    return (len(frames) - 1) * 1000 / span if span else 0


def check_snapshot_rate(port, player, other, check):
    """With sv_lanForceRate 0 the test client gets an internet client's rate:
    on a Plus server, a snapshot every frame and 1,000 bytes a second per Hz
    (25,000 at least), whatever its snaps and rate; g_forceSnaps and
    g_forceRate set by rcon take effect at once."""
    frame = 1000 // player.fps
    hz = 1000 / frame
    for command in ('set sv_lanForceRate 0', 'set g_forceSnaps 0', 'set g_forceRate 0'):
        smoke_test.rcon(port, RCON, command)
    player.read_frames(1, False)
    got = snapshots_per_second(player)
    check(f'snapshot rate: a snapshot every frame at sv_fps {player.fps}', abs(got - hz) <= hz * 0.1,
          f'{got:.1f} a second, {hz:.1f} expected')
    rate = re.search(r'plustest\S*\s.*\s(\d+)\s*$', smoke_test.rcon(port, RCON, 'status'), re.M)
    expected = max(25000, 1000 * 1000 // frame)
    check(f'snapshot rate: the rate is {expected} at sv_fps {player.fps}', rate is not None and int(rate.group(1)) == expected,
          rate.group(1) if rate else 'no status line')
    smoke_test.rcon(port, RCON, 'set g_forceSnaps 10')
    player.read_frames(1, False)
    got = snapshots_per_second(player)
    check('snapshot rate: g_forceSnaps 10 by rcon takes effect at once', abs(got - 10) <= 1.5, f'{got:.1f} a second')
    for command in ('set g_forceSnaps 0', 'set sv_lanForceRate 1'):
        smoke_test.rcon(port, RCON, command)


def check_archive_time(port, player, other, check):
    """Following a bot a second back, as a killcam, the player gets the bot's
    playerstate from the last frame at least a second old: the bot moves at
    100 units a second, so its origin tells how far back that was. The
    antilag finds its frames the same way. (The archived commandTime is moved
    to the present, so it cannot tell.)"""
    frame = 1000 // player.fps
    smoke_test.rcon(port, RCON, 'set plustest_killcam 1')
    player.read_frames(4.5, False)
    m = re.search(r'"plustest_move" is: ?"(\d+) (-?[\d.]+) (\d+)', smoke_test.rcon(port, RCON, 'plustest_move'))
    behind = []
    if m:
        t0, base, bot = int(m.group(1)), float(m.group(2)), int(m.group(3))
        for f in player.read_frames(2, False):
            if f['ps'].get('clientNum') == bot:
                behind.append(f['server_time'] - (t0 + (f['ps'].get('origin[0]', 0) - base) / 0.1))
    behind.sort()
    middle = behind[len(behind) // 2] if behind else None
    # the frame found is the newest that old: at most a frame short of it
    check(f'archive time: a second back is a second back at sv_fps {player.fps}',
          middle is not None and 1000 - frame - 1 <= middle <= 1001,
          f'{middle if middle is None else round(middle)} ms back, {1000 - frame} to 1000 expected')


def pump(players, seconds):
    """Keeps every client sending commands for that long."""
    deadline = time.time() + seconds
    while time.time() < deadline:
        for p in players:
            p.read_frames(0.03, False)


class Duel:
    """A second client, 600 ms of lag behind, that stands in front of the
    player, down the sights with no spread (plustest_duel). It joins once: a
    client that leaves keeps its slot until it times out."""
    shooter = None

    def __init__(self, port, player, other):
        self.port, self.player = port, player
        if Duel.shooter is None:
            m = re.search(r'"sv_serverid" is: ?"(-?\d+)', smoke_test.rcon(port, RCON, 'sv_serverid'))
            shooter = Player(port)
            shooter.lag = 600
            if shooter.connect(118, 'plustest2') is not None or not shooter.request_gamestate(int(m.group(1))):
                shooter.close()
                return
            shooter.enter_game(int(m.group(1)), 1.0)
            # down the sights: the hip's idle sway moves a bullet off the view
            shooter.weapon, shooter.hold = other, BUTTON_ADS
            Duel.shooter = shooter
            settle(player, other)
            self.pump(2)
            # a game folder's config can turn it off
            smoke_test.rcon(port, RCON, 'set g_antilag 1')
            smoke_test.rcon(port, RCON, 'set plustest_duel 1')
            self.pump(1.5)
        self.shooter = Duel.shooter

    def pump(self, seconds):
        pump([self.player, Duel.shooter], seconds)

    def shot(self):
        """Fires; where it hit the player, and the impacts on the world."""
        smoke_test.rcon(self.port, RCON, 'set plustest_fire 1')
        impacts = set()
        deadline = time.time() + 1.0
        while time.time() < deadline:
            for f in self.player.read_frames(0.03, False):
                impacts |= {tuple(e['pos.trBase']) for e in f['entities'].values()
                            if e.get('eType', 0) - ET_EVENTS in range(EV_BULLET_HIT_SMALL, EV_SHOTGUN_HIT + 1)}
            self.shooter.read_frames(0.03, False)
        hit = re.search(r'"plustest_hit" is: ?"([^"^]*)', smoke_test.rcon(self.port, RCON, 'plustest_hit'))
        return (hit.group(1) if hit else None), impacts

    def aim(self, up, dp, dy):
        for name, value in (('plustest_up', up), ('plustest_dp', dp), ('plustest_dy', dy), ('plustest_aim', 'head')):
            smoke_test.rcon(self.port, RCON, f'set {name} {value}')
        # the gun follows a turn of the view over a while
        self.pump(1.0)

    def aim_at_head(self):
        """Records where the player's head is and aims at it; the gun's
        offset, or None if the shot over the head missed the wall."""
        smoke_test.rcon(self.port, RCON, 'set plustest_head 1')
        self.pump(0.2)
        pos = re.search(r'"plustest_pos" is: ?"([^"^]*)', smoke_test.rcon(self.port, RCON, 'plustest_pos'))
        pos = [float(x) for x in pos.group(1).split()] if pos else None
        # the bullet leaves a few degrees off the view (the gun's own
        # angles): one over the head, into the wall, measures by how much
        self.aim(40, 0, 0)
        _, impacts = self.shot()
        if not pos or len(impacts) != 1:
            return None
        def angles(v):
            return -math.degrees(math.atan2(v[2], math.hypot(v[0], v[1]))), math.degrees(math.atan2(v[1], v[0]))
        head, eye = pos[:3], pos[3:]
        wanted = angles([h - e for h, e in zip(head[:2] + [head[2] + 40], eye)])
        went = angles([i - e for i, e in zip(next(iter(impacts)), eye)])
        dp, dy = wanted[0] - went[0], wanted[1] - went[1]
        self.aim(0, dp, dy)
        return dp, dy


def check_lean_rewind(port, player, other, check):
    """The shooter shoots at where the player's head was while it leaned,
    after the player let go of the lean: the antilag must put the head back
    where the shooter saw it."""
    duel = Duel(port, player, other)
    if not duel.shooter:
        check('lean rewind: a second client joins', False)
        return
    for side, lean in (('left', BUTTON_LEANLEFT), ('right', BUTTON_LEANRIGHT)):
        player.script = tuple([(lean, other)] * 140)
        duel.pump(1.5)
        offset = duel.aim_at_head()
        if not offset:
            check(f'lean rewind: a shot over the head leaning {side} hits the wall once', False)
            player.script = ()
            continue
        # with the player still leaning: it must hit the head
        held, _ = duel.shot()
        while player.script:
            duel.pump(0.1)
        # let go: the lean comes back over about half a second
        duel.pump(0.25)
        hit, _ = duel.shot()
        check(f'lean rewind: a shot at the head leaning {side} {duel.shooter.lag} ms ago hits it',
              held in ('head', 'helmet') and hit in ('head', 'helmet'),
              f'hit {hit}; still leaning, {held}; the gun {offset[0]:.1f} and {offset[1]:.1f} degrees off')
        duel.pump(1.0)


def check_stance_rewind(port, player, other, check):
    """The shooter shoots at where the player's head was before it changed
    stance, once the player's head has left it: the antilag must rewind the
    stance's animation too, not only its flags. On the server going down
    holds the head a little over standing for about 450 ms (crouch) or 700 ms
    (prone) before it drops, and standing up raises it at once, so each shot
    comes when the 600 ms ago it rewinds to still has the head there."""
    duel = Duel(port, player, other)
    if not duel.shooter:
        check('stance rewind: a second client joins', False)
        return
    for name, before, after, wait in (('stand to crouch', 0, CROUCH, 0.75), ('stand to prone', 0, PRONE, 0.9),
                                      ('crouch to stand', CROUCH, 0, 0.35)):
        player.hold = before
        duel.pump(2.5)
        offset = duel.aim_at_head()
        if not offset:
            check(f'stance rewind: {name}, a shot over the head hits the wall once', False)
            continue
        held, _ = duel.shot()
        player.hold = after
        duel.pump(wait)
        hit, _ = duel.shot()
        check(f'stance rewind: {name}, a shot at the head {duel.shooter.lag} ms ago hits it',
              held in ('head', 'helmet') and hit in ('head', 'helmet'),
              f'hit {hit}; before, {held}; the gun {offset[0]:.1f} and {offset[1]:.1f} degrees off')
    player.hold = 0
    duel.pump(2.5)


def check_vote_reconnect(port, player, other, check):
    """A player who voted and reconnects in the same vote cannot vote again
    (ClientConnect clears the whole client). With two more clients the vote
    neither passes nor fails while the voter is away."""
    sid = int(re.search(r'"sv_serverid" is: ?"(-?\d+)', smoke_test.rcon(port, RCON, 'sv_serverid')).group(1))
    clients = [Player(port) for _ in range(4)]
    try:
        third, fourth, voter, again = clients
        for c, name in ((third, 'plusvote3'), (fourth, 'plusvote4'), (voter, 'plusvote')):
            if c.connect(118, name) is not None or not c.request_gamestate(sid):
                check('vote reconnect: the clients join', False)
                return
            c.enter_game(sid, 1.0)
        player.command(sid, 'callvote map_restart')
        pump([player, third, fourth, voter], 1)

        def vote(c):
            c.command(sid, 'vote no')
            got = []
            deadline = time.time() + 1.5
            while time.time() < deadline:
                for p in (player, third, fourth):
                    p.read_frames(0.03, False)
                got += [t for f in c.read_frames(0.03, False) for t in f['commands']]
            return got
        first = vote(voter)
        check('vote reconnect: the first vote counts', any('GAME_VOTECAST' in t for t in first), str(first[-3:]))
        voter.command(sid, 'disconnect')
        pump([player, third, fourth], 1)
        if again.connect(118, 'plusvote') is not None or not again.request_gamestate(sid):
            check('vote reconnect: the voter joins again', False)
            return
        again.enter_game(sid, 1.0)
        second = vote(again)
        check('vote reconnect: a second vote after a reconnect is refused',
              any('GAME_VOTEALREADYCAST' in t for t in second) and not any('GAME_VOTECAST' in t for t in second),
              str(second[-3:]))
    finally:
        for c in clients:
            c.close()


def check_error_repeats(port, player, other, check):
    """With logErrors 1 the same runtime error every frame for 11 s prints once,
    then as a count after 10 s, then the count it left when it stopped, not 220
    times."""
    # other threads of this gametype make errors too: count the blocks of this one
    mine = re.compile(r'script runtime error[^\n]*\n[^\n]*7777')
    count = lambda: len(mine.findall(open(LOG, encoding='latin-1').read()))
    before = count()
    for command in ('set logErrors 1', 'set plustest_errored ""', 'set plustest_error 1'):
        smoke_test.rcon(port, RCON, command)
    deadline = time.time() + 25
    while time.time() < deadline and 'is: "1' not in (smoke_test.rcon(port, RCON, 'plustest_errored') or '').replace('^7', ''):
        player.read_frames(0.5, False)
    smoke_test.rcon(port, RCON, 'set logErrors 0')
    deadline = time.time() + 15
    while time.time() < deadline and count() - before < 3:
        player.read_frames(0.5, False)
    text = open(LOG, encoding='latin-1').read()
    printed = count() - before
    check('error repeats: one error a frame for 11 s prints three times', printed == 3 and len(re.findall(r'runtime error, \d+ times in 1\d s[^\n]*\n[^\n]*7777', text)) >= 2,
          f'{printed} prints')


def check_wait_time(port, player, other, check):
    """40 x wait 0.05 take 2 s and 4 x wait 1 take 4 s, to a frame, at any
    sv_fps: the fraction of a frame a wait leaves carries to the next."""
    frame = 1000 // player.fps
    smoke_test.rcon(port, RCON, 'set plustest_waited ""')
    smoke_test.rcon(port, RCON, 'set plustest_wait 1')
    got = None
    deadline = time.time() + 15
    while time.time() < deadline and not got:
        player.read_frames(0.5, False)
        m = re.search(r'"plustest_waited" is: ?"(\d+) (\d+)', smoke_test.rcon(port, RCON, 'plustest_waited'))
        got = m and (int(m.group(1)), int(m.group(2)))
    check(f'wait time: wait 0.05 and wait 1 at sv_fps {player.fps}',
          got is not None and abs(got[0] - 2000) <= frame and abs(got[1] - 4000) <= frame, f'{got} ms')


# in this order: the turret mg_clip spawns stays in front of the player, where
# shotgun_spread's pellets would hit it; lean_rewind and stance_rewind move the
# player away from it, and leave a second client that archive_time's bot must
# not take
CHECKS = {'fast_reload': check_fast_reload, 'prone_peek': check_prone_peek, 'shotgun_spread': check_shotgun_spread,
          'mg_clip': check_mg_clip, 'mg_rate': check_mg_rate, 'snapshot_rate': check_snapshot_rate,
          'lean_rewind': check_lean_rewind, 'stance_rewind': check_stance_rewind,
          'archive_time': check_archive_time, 'vote_reconnect': check_vote_reconnect, 'wait_time': check_wait_time,
          'error_repeats': check_error_repeats}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--binary', default=os.path.join(ROOT, 'bin', 'cod2rev_lnxded'))
    ap.add_argument('--only', help='comma-separated checks: ' + ', '.join(CHECKS))
    ap.add_argument('--map', default='mp_toujane')
    ap.add_argument('--fps', type=int, default=20, help='sv_fps (mg_rate checks the rate at it)')
    args = ap.parse_args()
    only = [name for name in CHECKS if not args.only or name in args.only.split(',')]
    port = smoke_test.free_port()
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2_plusrules_') as home:
        mod = os.path.join(home, 'plustest', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(GAMETYPE)
        command = [os.path.abspath(args.binary), '+set', 'fs_basepath', os.path.abspath(args.game_dir),
                   '+set', 'fs_homepath', home, '+set', 'fs_game', 'plustest', '+set', 'net_port', str(port),
                   '+set', 'sv_cracked', '1', '+set', 'rcon_password', RCON, '+set', 'g_gametype', 'dm',
                   '+set', 'g_log', 'games_mp.log', '+set', 'g_logSync', '1', '+set', 'dedicated', '1',
                   '+set', 'sv_maxclients', '8', '+set', 'sv_fps', str(args.fps)]
        if args.fps > 20:
            # the settings for that rate (plus-protocol.md, Tick rate): a snapshot every frame
            command += ['+set', 'g_forceSnaps', str(args.fps), '+set', 'g_forceRate', str(args.fps * 1000), '+set', 'sv_maxRate', '0']
        command += ['+map', args.map]
        log_path = os.path.join(home, 'out.log')
        global LOG
        LOG = log_path
        with open(log_path, 'wb') as log:
            server = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, cwd=home)
        player = None
        try:
            smoke_test.wait_for(lambda: server.poll() is not None or smoke_test.query(port, 'getinfo x'), 150)
            check('the server runs the map', server.poll() is None and smoke_test.query(port, 'getinfo x') is not None)
            if not failures:
                m = re.search(r'"sv_serverid" is: ?"(-?\d+)', smoke_test.rcon(port, RCON, 'sv_serverid'))
                player = Player(port)
                player.fps = args.fps
                refused = player.connect(118, 'plustest')
                ok = refused is None and m is not None and player.request_gamestate(int(m.group(1)))
                check('a Plus client joins', ok, refused or '')
            if not failures:
                player.enter_game(int(m.group(1)), 1.0)
                states = frames_until(player, lambda s: any(p.get('weapon') for p in s), 10)
                other = next((p['weapon'] for p in states if p.get('weapon')), 0)
                check('the player spawns with the thompson', other != 0 and settle(player, other), f'weapon {other}')
            for name in only:
                if not failures:
                    CHECKS[name](port, player, other, check)
            check('the server still runs', server.poll() is None, f'exit code {server.poll()}')
        finally:
            if player:
                player.close()
            if Duel.shooter:
                Duel.shooter.close()
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    server.kill()
        if failures:
            print('\n--- last lines of the server log ---')
            print(open(log_path, encoding='latin-1').read()[-3000:])
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
