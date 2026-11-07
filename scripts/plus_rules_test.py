#!/usr/bin/env python3
"""Checks the rules a Plus server plays by with a Plus client (Plus version 123,
docs/architecture/plus-protocol.md#version-123).

    python3 scripts/plus_rules_test.py /path/to/CoD2 [--binary bin/cod2rev_lnxded] [--only NAME,...]

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

Exits with 0 when every check passes. On a build without the rules (any
build but make PLUS=1) the checks fail: that is how they catch the bugs.
"""
import argparse, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import cod2client, smoke_test
from cod2client import CLC_MOVE_NO_DELTA, hash_key

RCON = 'plusrules'
BUTTON_ATTACK, BUTTON_RELOAD, BUTTON_LEANLEFT, BUTTON_LEANRIGHT = 1, 16, 64, 128  # q_shared.h
PRONE, CROUCH = 256, 512  # BUTTON_PRONE, BUTTON_CROUCH: the stance held
BUTTON_USE = 8
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
	level.callbackPlayerDamage = ::Callback_Nothing;
	level.callbackPlayerKilled = ::Callback_Nothing;
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
	setCvar("plustest_weapon", "");
	setCvar("plustest_turret", "");
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
		if(getCvar("plustest_turret") == "1" && !isdefined(self.turret))
		{
			self.turret = spawnTurret("misc_turret", self.origin + maps\\mp\\_utility::vectorScale(anglesToForward(self.angles), 60) + (0, 0, 30), "mg42_bipod_stand_mp");
			self.turret.angles = self.angles;
			self.turret setModel("xmodel/weapon_mg42");
			self setOrigin(self.origin - maps\\mp\\_utility::vectorScale(anglesToRight(self.angles), 20));
			wait 0.2;
			logPrint("plustest turret " + self useTurret(self.turret) + "\\n");
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
        # MSG_ReadDeltaUsercmdKey against the null command; a field left
        # unchanged keeps the previous command's value, so all are sent
        payload.bits(CLC_MOVE_NO_DELTA, 3)
        payload.byte(1)                                  # command count
        payload.bits(0, 1)                               # absolute server time
        payload.long(self.server_time)
        payload.bits(1 ^ (key & 1), 1)                   # something changed
        payload.bits(1 ^ (key & 1), 1)                   # not only buttons, angles and move
        payload.bits((buttons & BUTTON_ATTACK) ^ (key & 1), 1)
        payload.bits(0, 1)                               # pitch unchanged
        payload.bits(0, 1)                               # yaw unchanged
        payload.bits(1, 1)
        payload.bits(0 ^ (key & 0xf), 4)                 # no move keys
        key ^= self.server_time
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


def check_mg_clip(port, player, other, check):
    settle(player, other)
    smoke_test.rcon(port, RCON, 'set plustest_turret 1')
    on = frames_until(player, lambda s: s and s[-1].get('eFlags', 0) & EF_TURRET_ACTIVE, 5)
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


CHECKS = {'fast_reload': check_fast_reload, 'prone_peek': check_prone_peek, 'mg_clip': check_mg_clip}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--binary', default=os.path.join(ROOT, 'bin', 'cod2rev_lnxded'))
    ap.add_argument('--only', help='comma-separated checks: ' + ', '.join(CHECKS))
    ap.add_argument('--map', default='mp_toujane')
    args = ap.parse_args()
    only = args.only.split(',') if args.only else list(CHECKS)
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
                   '+set', 'sv_maxclients', '8', '+map', args.map]
        log_path = os.path.join(home, 'out.log')
        with open(log_path, 'wb') as log:
            server = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, cwd=home)
        player = None
        try:
            smoke_test.wait_for(lambda: server.poll() is not None or smoke_test.query(port, 'getinfo x'), 150)
            check('the server runs the map', server.poll() is None and smoke_test.query(port, 'getinfo x') is not None)
            if not failures:
                m = re.search(r'"sv_serverid" is: ?"(-?\d+)', smoke_test.rcon(port, RCON, 'sv_serverid'))
                player = Player(port)
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
