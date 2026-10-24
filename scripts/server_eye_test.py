#!/usr/bin/env python3
"""The server's eye of a player bobs while walking and not in the air, as 1.3.

    python3 scripts/server_eye_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH]

Writes a mod in a temporary home folder whose dm gametype logs, every server
frame, whether the first player is on the ground, its origin and its eye
(libcod's getViewOrigin, the G_GetPlayerViewOrigin shots start from; the
stock getEye is the origin raised 40 units). A minimal client
(hit_location_test's) keeps walking forward; three times the mod throws it
straight up with libcod's setVelocity (the minimal client has no jump key),
and in the air it clears the horizontal velocity the air control adds just
before it reads the eye. 1.3 (0x4fddb6) bobs the eye sideways
by the bob cycle and the horizontal speed (0x4fde06): the cycle a walk left
stays in the air, the horizontal speed there is 0. (Standing still resets the
cycle, so a jump from a stop shows nothing.)
Exits with 0 when the walking eye moves sideways off the origin, the eye in
the air stays over it, and nothing crashed.
"""
import argparse, math, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test
import hit_location_test as hit
from hit_location_test import DEFAULT_BINARY, RCON

MOD = 'eyetest'
WALK_BOB = 0.5  # sideways eye offset a walk must reach at least
AIR_BOB = 0.05  # and the most a jump in place may have

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

	// the first spawn point with room to walk in front of it
	spawnpoints = getentarray("mp_dm_spawn", "classname");
	for(i = 0; i < spawnpoints.size - 1; i++)
	{
		eye = spawnpoints[i].origin + (0, 0, 40);
		trace = bulletTrace(eye, eye + maps\\mp\\_utility::vectorScale(anglesToForward(spawnpoints[i].angles), 500), false, undefined);
		if(trace["fraction"] == 1)
			break;
	}
	self spawn(spawnpoints[i].origin, spawnpoints[i].angles);
	self character\\mp_american_normandy_braeburn::main();
	logPrint("eyetest spawned\\n");
	self thread trace();
	self thread jumps();
}

trace()
{
	self endon("disconnect");
	for(;;)
	{
		if(!self isOnGround())
		{
			v = self getVelocity();
			self setVelocity((0, 0, v[2]));
		}
		logPrint("eyetest frame " + self isOnGround() + " " + self.origin + " " + self getViewOrigin() + "\\n");
		wait 0.05;
	}
}

// while walking, three throws straight up
jumps()
{
	self endon("disconnect");
	wait 1.5;
	for(i = 0; i < 3; i++)
	{
		self setVelocity((0, 0, 300));
		wait 0.2;
		while(!self isOnGround())
			wait 0.05;
		wait 0.6;
	}
	logPrint("eyetest done\\n");
}

Callback_PlayerDisconnect()
{
}

Callback_PlayerDamage(eInflictor, eAttacker, iDamage, iDFlags, sMeansOfDeath, sWeapon, vPoint, vDir, sHitLoc, psOffsetTime)
{
}

Callback_PlayerKilled(eInflictor, eAttacker, iDamage, sMeansOfDeath, sWeapon, vDir, sHitLoc, psOffsetTime, deathAnimDuration)
{
}
'''

VEC = r'\((-?[\d.e+-]+), (-?[\d.e+-]+), (-?[\d.e+-]+)\)'
FRAME = re.compile(r'eyetest frame (\d) ' + VEC + ' ' + VEC)


class Walker(hit.Shooter):
    """hit_location_test's minimal client, walking from the first command."""
    walking = True


def game_log(home):
    path = os.path.join(home, MOD, 'games_mp.log')
    return open(path, encoding='latin-1').read() if os.path.exists(path) else ''


def frames(home):
    """(on ground, origin, sideways offset of the eye from the origin) per
    traced frame."""
    out = []
    for m in FRAME.finditer(game_log(home)):
        v = [float(x) for x in m.groups()[1:]]
        out.append((m.group(1) == '1', v[:3], math.hypot(v[3] - v[0], v[4] - v[1])))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary')
    ap.add_argument('--map', default='mp_toujane')
    args = ap.parse_args()
    binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_BINARY[(args.protocol, False)]))
    port = smoke_test.free_port()
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2_eye_') as home:
        mod = os.path.join(home, MOD, 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(GAMETYPE)
        command = [binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                   '+set', 'fs_game', MOD, '+set', 'net_port', str(port), '+set', 'sv_cracked', '1',
                   '+set', 'rcon_password', RCON, '+set', 'g_gametype', 'dm', '+set', 'g_log', 'games_mp.log',
                   '+set', 'g_logSync', '1', '+set', 'developer', '1', '+set', 'sv_maxclients', '8',
                   '+set', 'dedicated', '1', '+map', args.map]
        log_path = os.path.join(home, 'out.log')
        with open(log_path, 'wb') as log:
            server = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, cwd=home)
        client = None
        try:
            deadline = time.time() + 150
            while time.time() < deadline and server.poll() is None and not smoke_test.query(port, 'getinfo x'):
                time.sleep(1)
            check('the server runs the map', server.poll() is None and smoke_test.query(port, 'getinfo x') is not None)
            if not failures:
                hit.Shooter = Walker  # join() makes a Shooter
                client, why = hit.join(port, args.protocol, 'eyewalker')
                check('the player joins', client is not None, why)
            if not failures:
                deadline = time.time() + 20
                while time.time() < deadline and 'eyetest done' not in game_log(home):
                    client.read_frames(0.25, False)
                check('the jumps ran', 'eyetest done' in game_log(home))
            traced = frames(home)
            ground = [d for on, _, d in traced if on]
            air = [d for on, _, d in traced if not on]
            check('the eye is traced', len(air) >= 20, f'{len(traced)} frames, {len(air)} in the air')
            walked = math.dist(traced[0][1][:2], traced[-1][1][:2]) if traced else 0
            check('the player walked', walked > 100, f'{walked:.0f} units')
            check('walking, the eye bobs sideways', ground and max(ground) > WALK_BOB,
                  f'most {max(ground, default=0):.3f}')
            check('in the air, the eye stays over the origin', air and max(air) < AIR_BOB,
                  f'most {max(air, default=0):.3f}')
            check('the server still runs', server.poll() is None and smoke_test.query(port, 'getinfo x') is not None,
                  f'exit code {server.poll()}')
        finally:
            if client:
                client.close()
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    server.kill()
        text = open(log_path, encoding='latin-1').read()
        check('no crash report', 'CRASH REPORT' not in text)
        if failures:
            print('\n--- last lines of the server log ---')
            print('\n'.join(text.splitlines()[-40:]))
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
