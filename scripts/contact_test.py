#!/usr/bin/env python3
"""Players bump into each other, a knife hits, and explosions reach a player.

    python3 scripts/contact_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--listen]

Like scripts/hit_location_test.py, whose minimal client it uses, it writes a dm
gametype that spawns a target and, in front of it, a player that a minimal
client drives. The test picks each phase with rcon (contact_phase), which the
mod polls:

- walk: the second player walks straight at the target for a few seconds; the
  two must collide, so the distance between them never falls under a body's
  width. Player against player clipping goes through the world's sectors.
- trigger: it walks at the target again, through a trigger_radius on its
  way, which must fire; the trigger finds the player through the world's
  sectors.
- melee: it stands next to the target and presses the melee button; the target
  must take MOD_MELEE hits.
- grenade: it stands farther, aims down and throws a frag grenade, whose
  splash must hit the target; the grenade flies and bounces on the world's
  collision.
- kill: with its health down, an explosion kills the target, whose body the
  mod clones, and which spawns again two seconds later.
- radius: the mod calls radiusDamage next to the target, spawned again,
  which must take a MOD_EXPLOSIVE hit; the explosion finds it through the
  world's sectors too.

The target is another minimal client on the dedicated server, or with --listen
the client that hosts the game (with a CD key in ~/.cod2/preferences; skipped
without one). Exits with 0 when every phase holds and nothing crashed.
"""
import argparse, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test
from hit_location_test import DEFAULT_BINARY, PREFERENCES, RCON, join

WEAPON = 'kar98k_mp'
MIN_DISTANCE = 24  # two player boxes are 30 units wide

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
	precacheItem("frag_grenade_american_mp");
	setcvar("contact_phase", "");
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

	if(!isdefined(level.target))
	{
		// the first spawn point with room in front of it
		spawnpoints = getentarray("mp_dm_spawn", "classname");
		for(i = 0; i < spawnpoints.size; i++)
		{
			eye = spawnpoints[i].origin + (0, 0, 60);
			trace = bulletTrace(eye, eye + maps\\mp\\_utility::vectorScale(anglesToForward(spawnpoints[i].angles), 200), false, undefined);
			if(trace["fraction"] == 1)
				break;
		}
		level.target = self;
		level.targetSpawn = spawnpoints[i];
		self spawn(spawnpoints[i].origin, spawnpoints[i].angles);
		self character\\mp_american_normandy_braeburn::main();
		self arm();
		logPrint("contact target " + self getEntityNumber() + "\\n");
	}
	else
	{
		level.mover = self;
		target = level.target;
		origin = target.origin + maps\\mp\\_utility::vectorScale(anglesToForward(target.angles), 120);
		self spawn(origin, vectorToAngles(target.origin - origin));
		self character\\mp_american_normandy_braeburn::main();
		self arm();
		logPrint("contact mover " + self getEntityNumber() + "\\n");
		self thread phases();
	}
}

// a rifle and frag grenades
arm()
{
	self giveWeapon("%(weapon)s");
	self giveMaxAmmo("%(weapon)s");
	self setSpawnWeapon("%(weapon)s");
	self giveWeapon("frag_grenade_american_mp");
	self setWeaponClipAmmo("frag_grenade_american_mp", 3);
	self switchToOffhand("frag_grenade_american_mp");
}

// puts the mover distance units in front of the target, facing it; spawning
// again would take its weapon
place(distance)
{
	target = level.target;
	origin = target.origin + maps\\mp\\_utility::vectorScale(anglesToForward(target.angles), distance);
	self setOrigin(origin);
	self setPlayerAngles(vectorToAngles(target.origin - origin));
	return origin;
}

phases()
{
	self endon("disconnect");
	phase = "";
	for(;;)
	{
		wait 0.05;
		next = getcvar("contact_phase");
		if(next == phase)
			continue;
		phase = next;
		self notify("phase");
		logPrint("contact phase " + phase + "\\n");
		if(phase == "walk")
		{
			self place(120);
			self thread logDistance();
		}
		else if(phase == "trigger")
		{
			self place(120);
			// on the way to the target, the mover's feet inside it
			target = level.target;
			middle = target.origin + maps\\mp\\_utility::vectorScale(anglesToForward(target.angles), 70);
			trigger = spawn("trigger_radius", middle, 0, 16, 64);
			trigger thread logTrigger();
		}
		else if(phase == "melee")
		{
			origin = self place(40);
			self thread hold(origin);
		}
		else if(phase == "grenade")
		{
			origin = self place(200);
			self thread hold(origin);
		}
		else if(phase == "kill")
		{
			level.target.health = 10;
			radiusDamage(level.target.origin + (0, 0, 40), 64, 60, 60);
		}
		else if(phase == "face")
		{
			origin = self place(120);
			self thread hold(origin);
		}
		else if(phase == "killmover")
		{
			self.health = 10;
			radiusDamage(self.origin + (0, 0, 40), 64, 60, 60);
		}
		else if(phase == "spot")
		{
			// a place of the test's own (scripts/prediction_test.py)
			self setOrigin((getcvarfloat("contact_x"), getcvarfloat("contact_y"), getcvarfloat("contact_z")));
			self setPlayerAngles((0, getcvarfloat("contact_yaw"), 0));
		}
		else if(phase == "radius")
		{
			radiusDamage(level.target.origin + (0, 0, 40), 64, 60, 60);
		}
	}
}

logTrigger()
{
	self waittill("trigger", player);
	logPrint("contact trigger " + player getEntityNumber() + "\\n");
	self delete();
}

logDistance()
{
	self endon("phase");
	for(;;)
	{
		logPrint("contact distance " + int(distance(self.origin, level.target.origin)) + "\\n");
		wait 0.1;
	}
}

hold(origin)
{
	self endon("phase");
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
	if(isdefined(level.mover) && self == level.mover)
		logPrint("contact mover hit " + sMeansOfDeath + " " + sHitLoc + " " + iDamage + "\\n");
	else if(self == level.target)
		logPrint("contact hit " + sMeansOfDeath + " " + sHitLoc + " " + iDamage + "\\n");
	else
		return;
	self finishPlayerDamage(eInflictor, eAttacker, iDamage, iDFlags | level.iDFLAGS_NO_KNOCKBACK, sMeansOfDeath, sWeapon, vPoint, vDir, sHitLoc, psOffsetTime);
}

Callback_PlayerKilled(eInflictor, eAttacker, iDamage, sMeansOfDeath, sWeapon, vDir, sHitLoc, psOffsetTime, deathAnimDuration)
{
	body = self clonePlayer(deathAnimDuration);
	if(self == level.target)
	{
		logPrint("contact killed " + sMeansOfDeath + "\\n");
		self thread respawn();
	}
	else
	{
		logPrint("contact mover killed " + sMeansOfDeath + "\\n");
		self thread respawnMover();
	}
}

// in front of the target again, armed
respawnMover()
{
	self endon("disconnect");
	wait 2;
	self.sessionstate = "playing";
	self.health = self.maxhealth;
	target = level.target;
	origin = target.origin + maps\\mp\\_utility::vectorScale(anglesToForward(target.angles), 120);
	self spawn(origin, vectorToAngles(target.origin - origin));
	self detachAll();
	self character\\mp_american_normandy_braeburn::main();
	self arm();
	logPrint("contact mover respawned\\n");
}

// back at its spawn point, as the target again
respawn()
{
	self endon("disconnect");
	wait 2;
	self.sessionstate = "playing";
	self.health = self.maxhealth;
	self spawn(level.targetSpawn.origin, level.targetSpawn.angles);
	self detachAll();
	self character\\mp_american_normandy_braeburn::main();
	self arm();
	logPrint("contact respawned\\n");
}
''' % {'weapon': WEAPON}


def game_log(home):
    path = os.path.join(home, 'contact', 'games_mp.log')
    return open(path, encoding='latin-1').read() if os.path.exists(path) else ''


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

    def run(seconds, mover=None):
        """Keeps every minimal client sending commands for seconds."""
        end = time.time() + seconds
        while time.time() < end:
            for c in clients:
                c.read_frames(0.05 if c is not mover else 0.2, False)
            if not clients:
                time.sleep(0.2)

    def phase(name):
        smoke_test.rcon(port, RCON, f'set contact_phase {name}')
        smoke_test.wait_for(lambda: f'contact phase {name}\n' in game_log(home), 10)
        return game_log(home).rindex(f'contact phase {name}\n') if f'contact phase {name}\n' in game_log(home) else -1

    with tempfile.TemporaryDirectory(prefix='cod2_contact_') as home:
        mod = os.path.join(home, 'contact', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(GAMETYPE)
        command = [binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                   '+set', 'fs_game', 'contact', '+set', 'net_port', str(port), '+set', 'sv_cracked', '1',
                   '+set', 'rcon_password', RCON, '+set', 'g_gametype', 'dm', '+set', 'g_log', 'games_mp.log',
                   '+set', 'g_logSync', '1', '+set', 'developer', '1', '+set', 'sv_maxclients', '8']
        command += [] if args.listen else ['+set', 'dedicated', '1']
        log_path = os.path.join(home, 'out.log')
        with open(log_path, 'wb') as log:
            server = subprocess.Popen(command + ['+map', args.map], stdin=subprocess.DEVNULL, stdout=log,
                                      stderr=subprocess.STDOUT, cwd=home)
        try:
            deadline = time.time() + 150
            while time.time() < deadline and server.poll() is None and not smoke_test.query(port, 'getinfo x'):
                time.sleep(1)
            check('the server runs the map', server.poll() is None and smoke_test.query(port, 'getinfo x') is not None)
            if not failures and not args.listen:
                target, why = join(port, args.protocol, 'contacttarget')
                check('the target joins', target is not None, why)
                if target:
                    clients.append(target)
            if not failures:
                smoke_test.wait_for(lambda: 'contact target' in game_log(home), 60)
                check('the target spawns', 'contact target' in game_log(home))
                run(2)  # let it land
            if not failures:
                mover, why = join(port, args.protocol, 'contactmover')
                check('the mover joins', mover is not None, why)
                if mover:
                    clients.append(mover)
            if not failures:
                weapon = 0
                deadline = time.time() + 10
                while not weapon and time.time() < deadline:
                    for frame in mover.read_frames(0.5, False):
                        weapon = frame['ps'].get('weapon', 0) or weapon
                    run(0.1, mover)
                mover.weapon = weapon
                run(1, mover)

                start = phase('walk')
                mover.walking = True
                run(3, mover)
                mover.walking = False
                distances = [int(d) for d in re.findall(r'contact distance (\d+)', game_log(home)[start:])]
                check('the mover walks up to the target', distances and min(distances) < 60,
                      f'from {distances[0]} to {min(distances)}' if distances else 'no distance')
                check('the mover stops at the target', distances and min(distances) >= MIN_DISTANCE,
                      f'closest {min(distances)}' if distances else 'no distance')

                start = phase('trigger')
                run(0.5, mover)
                mover.walking = True
                end = time.time() + 4
                while time.time() < end and 'contact trigger' not in game_log(home)[start:]:
                    run(0.2, mover)
                mover.walking = False
                check('the mover sets off a trigger on its way', 'contact trigger' in game_log(home)[start:])

                start = phase('melee')
                run(1, mover)
                mover.melee = mover.firing = True
                end = time.time() + 6
                while time.time() < end and game_log(home)[start:].count('contact hit MOD_MELEE') < 2:
                    run(0.3, mover)
                mover.melee = mover.firing = False
                hits = re.findall(r'contact hit (\S+) (\S*) (\d+)', game_log(home)[start:])
                check('the knife hits the target', any(h[0] == 'MOD_MELEE' for h in hits),
                      ', '.join(f'{h[0]} {h[1]}' for h in hits) or 'no hit')

                start = phase('grenade')
                mover.pitch = 20.0
                run(1, mover)
                mover.frag_commands = 6  # one throw: held for 6 commands, then let go
                run(1, mover)
                end = time.time() + 8
                while time.time() < end and 'contact hit MOD_GRENADE' not in game_log(home)[start:]:
                    run(0.3, mover)
                mover.pitch = 0.0
                hits = re.findall(r'contact hit (\S+) (\S*) (\d+)', game_log(home)[start:])
                check('a thrown grenade blasts the target', any(h[0] == 'MOD_GRENADE_SPLASH' for h in hits),
                      ', '.join(f'{h[0]} {h[2]}' for h in hits) or 'no hit')

                start = phase('kill')
                end = time.time() + 6
                while time.time() < end and 'contact respawned' not in game_log(home)[start:]:
                    run(0.3, mover)
                check('an explosion kills the target', 'contact killed MOD_EXPLOSIVE' in game_log(home)[start:])
                check('the target spawns again', 'contact respawned' in game_log(home)[start:])
                run(1, mover)

                start = phase('radius')
                run(1, mover)
                hits = re.findall(r'contact hit (\S+) (\S*) (\d+)', game_log(home)[start:])
                check('the explosion reaches the target', any(h[0] == 'MOD_EXPLOSIVE' for h in hits),
                      ', '.join(f'{h[0]} {h[2]}' for h in hits) or 'no hit')
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
            print('\n--- contact lines of the game log ---')
            print('\n'.join(l for l in game_log(home).splitlines() if 'contact ' in l and 'distance' not in l)[-2000:])
            print('\n--- last lines of the server log ---')
            print('\n'.join(l for l in text.splitlines() if not l.startswith(('[ckpt]', '[scan', '[rbdiag', '[cnx]', 'FX Error')))[-3000:])
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
