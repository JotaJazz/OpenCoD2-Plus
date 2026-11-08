#!/usr/bin/env python3
"""Runs the bots mod on a real map and checks that the bots navigate and fight.

    python3 scripts/bots_test.py /path/to/CoD2 [--map mp_toujane] [--seconds 180]

The mod is installed in a temporary folder with bots_debug 1, so it writes
BOTNAV and BOTCOMBAT lines to games_mp.log. The map is loaded twice: the first
load must build and save the waypoint graph, the second must read it back.
Exits with 0 when every check passes.
"""
import argparse, os, re, secrets, shutil, subprocess, sys, tempfile, time

from install_bots import read_game_script
from smoke_test import free_port, log_lines, query, rcon

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MOD = 'bottest'
ERROR_CALLBACK = '''
CodeCallback_Error(critical, function, message)
{
	logprint("BOTERROR " + function + " " + message + "\\n");
}

// Knife fights hardly happen on their own (encounters end at range), so
// every 15 s two enemy bots are put face to face, 40 units apart.
botsTestMelee()
{
	for(;;)
	{
		wait 15;
		players = getentarray("player", "classname");
		a = undefined;
		b = undefined;
		for(i = 0; i < players.size; i++)
		{
			p = players[i];
			if(!isAlive(p) || p.sessionstate != "playing")
				continue;
			if(!isDefined(a))
				a = p;
			else if(!isDefined(b) && p.pers["team"] != a.pers["team"])
				b = p;
		}
		if(!isDefined(a) || !isDefined(b))
			continue;
		forward = anglesToForward((0, a getPlayerAngles()[1], 0));
		b setOrigin(a.origin + (forward[0] * 40, forward[1] * 40, 0));
		b setPlayerAngles(vectorToAngles(a.origin - b.origin));
	}
}
'''
HOOK = 'thread maps\\mp\\bots\\_bots::init();'
SD_SCRIPT = 'maps/mp/gametypes/sd.gsc'


def stock_defuse_race(lines):
    """Indexes of the errors stock sd.gsc raises when the defuser dies on the
    last tick of a defuse: it still defuses, then destroys the progress bars
    the death already removed (sd.gsc lines 1925 and 1926). A stock bug, about
    one SD run in twenty."""
    known = set()
    for i, line in enumerate(lines):
        if not (line.startswith('A;') and line.endswith(';bomb_defuse')):
            continue
        num = line.split(';')[2]
        if not any(k.startswith('K;') and k.split(';')[2] == num for k in lines[max(0, i - 6):i]):
            continue
        for j in range(i + 1, min(len(lines), i + 4)):
            if lines[j].startswith('BOTERROR') and 'removed entity is not an entity' in lines[j]:
                known.add(j)
    return known


def wait_lines(path, pattern, count, seconds):
    deadline = time.time() + seconds
    while time.time() < deadline:
        if sum(1 for l in log_lines(path) if re.search(pattern, l)) >= count:
            return True
        time.sleep(1)
    return False


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--binary', default=os.path.join(ROOT, 'bin', 'cod2rev_lnxded'))
    ap.add_argument('--map', default='mp_toujane')
    ap.add_argument('--gametype', default='tdm')
    ap.add_argument('--seconds', type=int, help='play time on the first load (default: 180, SD 240, CTF and HQ 300)')
    ap.add_argument('--bombmode', type=int, choices=(0, 1, 2),
                    help='SD bomb mode: stock SD always plays 0, so a copy with this mode is installed')
    ap.add_argument('--keep', help='copy games_mp.log and the server log to this folder')
    ap.add_argument('--corrupt-cache', action='store_true',
                    help='start with a garbage waypoint file, which must be rebuilt')
    args = ap.parse_args()
    if args.seconds is None:
        args.seconds = {'sd': 240, 'ctf': 300, 'hq': 300}.get(args.gametype, 180)
    fights = args.gametype in ('tdm', 'dm')
    if fights and args.seconds < 120:
        ap.error('the combat checks need --seconds 120 or more')

    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''))
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory(prefix='cod2rev_bots_') as home:
        subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'install_bots.py'), args.game_dir,
                        '--dest', home, '--name', MOD], check=True, stdout=subprocess.DEVNULL)
        callbacks = os.path.join(home, MOD, 'maps', 'mp', 'gametypes', '_callbacksetup.gsc')
        text = open(callbacks, encoding='latin-1').read()
        # the staged knife duel would teleport bots off their objectives
        if fights:
            text = text.replace(HOOK, HOOK + ' thread botsTestMelee();')
        text += ERROR_CALLBACK
        with open(callbacks, 'w', encoding='latin-1', newline='') as f:
            f.write(text)

        if args.bombmode is not None:
            sd, _ = read_game_script(args.game_dir, SD_SCRIPT)
            if 'level.bombmode = 0;' not in sd:
                ap.error('no stock sd.gsc with level.bombmode in the game folder')
            path = os.path.join(home, MOD, *SD_SCRIPT.split('/'))
            sd = sd.replace('level.bombmode = 0;', f'level.bombmode = {args.bombmode};')
            # stock SD never runs single mode, and there it claims a second
            # bomb site that does not exist
            for call in ('clientclaimtrigger', 'clientreleasetrigger'):
                sd = sd.replace(f'other {call}(bombzone_other);',
                                f'if(isdefined(bombzone_other)) other {call}(bombzone_other);')
            with open(path, 'w', encoding='latin-1', newline='') as f:
                f.write(sd)

        cache = os.path.join(home, MOD, 'waypoints', args.map + '_v2.graph')
        if args.corrupt_cache:
            os.makedirs(os.path.dirname(cache))
            with open(cache, 'wb') as f:
                f.write(b'not a graph file' * 4)

        game_log = os.path.join(home, MOD, 'games_mp.log')
        port = free_port()
        password = secrets.token_hex(8)
        with open(os.path.join(home, 'server.log'), 'wb') as out:
            server = subprocess.Popen(
                [args.binary, '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                 '+set', 'fs_game', MOD, '+set', 'dedicated', '1', '+set', 'net_port', str(port),
                 '+set', 'sv_cracked', '1', '+set', 'rcon_password', password,
                 '+set', 'g_gametype', args.gametype, '+set', f'scr_{args.gametype}_timelimit', '0',
                 '+set', f'scr_{args.gametype}_scorelimit', '0', '+set', 'g_logSync', '1',
                 '+set', 'sv_maxclients', '8', '+set', 'bots_count', '6', '+set', 'bots_debug', '1',
                 '+map', args.map],
                stdin=subprocess.DEVNULL, stdout=out, stderr=subprocess.STDOUT)
        changed_at = None
        try:
            first = wait_lines(game_log, r'^BOTNAV graph ', 1, 120)
            check('graph ready on the first load', first)
            if first:
                time.sleep(args.seconds)
                changed_at = len(log_lines(game_log))
                rcon(port, password, 'map ' + args.map)
                check('graph ready on the second load', wait_lines(game_log, r'^BOTNAV graph ', 2, 120))
                time.sleep(15)
            check('server still running', server.poll() is None and query(port, 'getinfo') is not None)
        finally:
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    server.kill()

        if args.keep:
            os.makedirs(args.keep, exist_ok=True)
            for name in (game_log, os.path.join(home, 'server.log')):
                if os.path.exists(name):
                    shutil.copy(name, args.keep)

        lines = log_lines(game_log)
        graphs = [l for l in lines if l.startswith('BOTNAV graph ')]
        check('first load generates and saves the graph', len(graphs) >= 1 and graphs[0].startswith('BOTNAV graph generated')
              and os.path.exists(cache), ', '.join(graphs))
        check('second load reads the saved graph', len(graphs) >= 2 and graphs[1].startswith('BOTNAV graph loaded'))

        nodes = [l for l in lines if re.match(r'BOTNAV \d+ node ', l)]
        bots = {l.split()[1] for l in nodes}
        check('bots follow paths', len(nodes) >= 60 and len(bots) >= 3, f'{len(nodes)} nodes by {len(bots)} bots')
        after = sum(1 for l in lines[changed_at:] if re.match(r'BOTNAV \d+ node ', l)) if changed_at else 0
        check('bots follow the saved graph', after >= 10, f'{after} nodes after the map change')
        # a moving goal (an enemy, a flag carrier) must not make a bot search
        # every tick (10 a second); a few goal changes in a row are normal
        repaths = {}
        for raw in log_lines(game_log, timed=True):
            m = re.match(r'^\s*(\d+):(\d+) BOTNAV (\d+) repath', raw)
            if m:
                key = (m.group(3), int(m.group(1)) * 60 + int(m.group(2)))
                repaths[key] = repaths.get(key, 0) + 1
        busiest = max(repaths.values(), default=0)
        check('bots search paths sparingly', 0 < busiest <= 6, f'at most {busiest} searches by a bot in a second')
        goals = {l.split()[3] for l in lines if re.match(r'BOTNAV \d+ goal ', l)}
        # in SD, CTF and HQ most bots play the objective instead of patrolling
        if fights:
            check('bots choose patrol and hint goals', {'patrol', 'hint'} <= goals, ', '.join(sorted(goals)))
        else:
            print(f'     goals: {", ".join(sorted(goals))}')
        # a node a bot got stuck on is avoided for 10 s, so the same bot
        # should rarely get stuck on the same node again; the game time
        # prefix gives the seconds
        stuck = []
        for raw in log_lines(game_log, timed=True):
            m = re.match(r'^\s*(\d+):(\d+) BOTNAV (\d+) stuck (\d+)', raw)
            if m:
                stuck.append((int(m.group(1)) * 60 + int(m.group(2)), m.group(3), m.group(4)))
        repeats = sum(1 for i, (t, bot, node) in enumerate(stuck)
                      if any(b == bot and n == node and 0 <= t - t0 < 10 for t0, b, n in stuck[:i]))
        check('stuck nodes are avoided', repeats <= max(3, len(stuck) // 4),
              f'{repeats} of {len(stuck)} stuck events repeat the same node within 10 s')

        combat = {l.split()[2] for l in lines if re.match(r'BOTCOMBAT \d+ ', l)}
        for action in ('ads', 'nade', 'melee'):
            if fights:
                check(f'bots use {action}', action in combat)
            else:
                print(f'     {action}: {"used" if action in combat else "not used"}')

        roles = {l.split()[3] for l in lines if re.match(r'BOTOBJ \d+ role ', l)}
        events = [' '.join(l.split()[2:]) for l in lines if re.match(r'BOTOBJ \d+ ', l)]
        if args.gametype == 'sd':
            zones = [l.split()[2:] for l in lines if l.startswith('BOTOBJ zones ')]
            # only the sites marked for the bomb mode are active: two in the
            # original mode, one in single mode (on several stock maps only
            # one of the two is marked for it)
            active = '1' if args.bombmode == 1 else '2'
            check('bots only use active bomb sites', bool(zones) and zones[0][0] == active, ' '.join(zones[0]) if zones else 'no zones line')
            check('bots take plant and guard roles', {'plant', 'guard'} <= roles, ', '.join(sorted(roles)))
            check('bots start planting', 'plant start' in events)
            plants = [l for l in lines if l.startswith('A;') and l.endswith(';bomb_plant')]
            check('bots plant the bomb', len(plants) >= 1, f'{len(plants)} plants')
            defuses = [l for l in lines if l.startswith('A;') and l.endswith(';bomb_defuse')]
            print(f'     {events.count("defuse start")} defuse starts, {len(defuses)} defuses')
        if args.gametype == 'ctf':
            check('bots take steal and defend roles', {'steal', 'defend'} <= roles, ', '.join(sorted(roles)))
            check('bots pick up the enemy flag', 'pickup' in events, f'{events.count("pickup")} pickups')
            print(f'     {events.count("capture")} captures, {events.count("return")} returns')
        if args.gametype == 'hq':
            radios = [l.split()[2] for l in lines if l.startswith('BOTOBJ radio ')]
            check('bots take the radio role', 'radio' in roles, ', '.join(sorted(roles)))
            # a first radio the graph can't reach stays there until a player takes it
            check('bots capture radios', 'skipped' in radios[:1] or any(t in ('allies', 'axis') for t in radios),
                  f'{sum(t in ("allies", "axis") for t in radios)} captures')
            print(f'     {radios.count("none")} radios neutral again, {radios.count("skipped")} skipped,'
                  f' {", ".join(l.split(maxsplit=1)[1] for l in lines if l.startswith("BOTOBJ radios "))[:40]}')
        kills = [l for l in lines if l.startswith('K;')]
        check('bots kill each other', len(kills) >= 5, f'{len(kills)} kills')
        nade_kills = sum(1 for l in kills if 'MOD_GRENADE_SPLASH' in l)
        melee_kills = sum(1 for l in kills if 'MOD_MELEE' in l)
        # K;victimguid;victimnum;team;name;attackerguid;attackernum;...
        self_nades = sum(1 for l in kills if 'MOD_GRENADE_SPLASH' in l and l.split(';')[2] == l.split(';')[6])
        print(f'     {nade_kills} grenade kills, {melee_kills} melee kills')
        # bots shot while holding a grenade drop it, so a few are normal
        allowed_nades = max(2, len(kills) // 20)
        check('few grenade suicides', self_nades <= allowed_nades, f'{self_nades} of at most {allowed_nades}')

        stock = stock_defuse_race(lines)
        if stock:
            print(f'     {len(stock)} errors of the stock sd.gsc defuse race left out')
        errors = [l for i, l in enumerate(lines) if l.startswith('BOTERROR') and i not in stock]
        if args.gametype == 'hq':
            # hq_obj_think reads level.radio[level.nextradio - 1] with nextradio 0;
            # outside developer mode the value is undefined and the script goes on
            errors = [l for l in errors if 'array index -1 out of range' not in l]
        allowed = 1 if args.corrupt_cache else 0
        check('no script errors', len(errors) <= allowed, '; '.join(errors[:3]))

    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
