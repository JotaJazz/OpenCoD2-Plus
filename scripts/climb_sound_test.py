#!/usr/bin/env python3
"""A real client hears another player's weapon go down and up as it climbs.

    python3 scripts/climb_sound_test.py /path/to/CoD2 [--client PATH] [--server PATH]

Runs the dedicated server with scripts/plus_rules_test.py's mod, a real client
(cod2_linux) as the observer, and a minimal Plus client that the mod puts in
front of mp_toujane's ledge (prediction_test.py's spot) and that mantles onto
it. Going into a mantle (or onto a ladder) the weapon is put away and no
weapon is raised: EV_RAISE_WEAPON with weapon 0, which the climber's own
cgame plays with the weapon it held, so it hears weap_raise, and other
clients played with nothing. On a Plus server they play weap_raise too
(cg_event_mp.c). With COD2_TRACE_SOUND the observer prints each alias it
plays; the test exits with 0 when it played weap_raise on the climber's
entity during the mantle and nothing crashed. Needs a CD key in
~/.cod2/preferences and opens a window; without a key it is skipped (exit 0).
"""
import argparse, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import smoke_test
import plus_rules_test as rules
from cod2client import CLC_MOVE_NO_DELTA, KEY_MASK_MOVE_FORWARD, hash_key
from hit_location_test import PREFERENCES

BUTTON_JUMP = 1024  # q_shared.h
# prediction_test.py's SPOT: in front of a ledge with a mantle_on brush, facing it
GAMETYPE = rules.GAMETYPE.replace('''		wait 0.05;
	}
}''', '''		if(getCvar("plustest_spot") == "1" && self.name == "climber")
		{
			setCvar("plustest_spot", "");
			self setOrigin((2450, 2200, 80));
			self setPlayerAngles((0, 90, 0));
		}
		wait 0.05;
	}
}''')


class Climber(rules.Player):
    """rules.Player that also walks forward while self.forward is set."""
    forward = False

    def _write_move(self, payload, forward):
        message_ack = self.incoming_sequence
        key = (self.checksum_feed ^ message_ack ^ hash_key(self._acknowledged()[1])) & 0xffffffff
        self.server_time += 50
        if self.script:
            (buttons, self.forward), self.script = self.script[0], self.script[1:]
        else:
            buttons, self.forward = 0, False
        payload.bits(CLC_MOVE_NO_DELTA, 3)
        payload.byte(1)
        payload.bits(0, 1)
        payload.long(self.server_time)
        payload.bits(1 ^ (key & 1), 1)
        payload.bits(1 ^ (key & 1), 1)
        payload.bits((buttons & 1) ^ (key & 1), 1)
        payload.bits(0, 1)
        payload.bits(0, 1)
        payload.bits(1, 1)
        payload.bits((KEY_MASK_MOVE_FORWARD if self.forward else 0) ^ (key & 0xf), 4)
        key ^= self.server_time
        payload.bits(0, 1)
        payload.bits(1, 1)
        payload.bits((buttons >> 1) ^ (key & 0x3ffff), 18)
        payload.bits(1, 1)
        payload.bits(self.weapon ^ (key & 0x7f), 7)
        payload.bits(0, 1)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--client', default=os.path.join(ROOT, 'build/check-linux/cod2_linux'))
    ap.add_argument('--server', default=os.path.join(ROOT, 'build/check-linux/cod2_lnxded'))
    args = ap.parse_args()
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    failures = []

    def check(name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            failures.append(name)

    port = smoke_test.free_port()
    procs, climber = [], None
    with tempfile.TemporaryDirectory(prefix='cod2_climb_') as home, \
            tempfile.TemporaryDirectory(prefix='cod2_climbc_') as client_home:
        mod = os.path.join(home, 'plustest', 'maps', 'mp', 'gametypes')
        os.makedirs(mod)
        with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
            f.write(GAMETYPE)
        os.makedirs(os.path.join(client_home, 'main'))
        game_log = os.path.join(home, 'plustest', 'games_mp.log')
        logged = lambda: open(game_log, encoding='latin-1').read() if os.path.exists(game_log) else ''
        client_log = os.path.join(client_home, 'out.log')
        try:
            procs.append(subprocess.Popen(
                [os.path.abspath(args.server), '+set', 'fs_basepath', os.path.abspath(args.game_dir), '+set', 'fs_homepath', home,
                 '+set', 'fs_game', 'plustest', '+set', 'net_port', str(port), '+set', 'sv_cracked', '1',
                 '+set', 'rcon_password', rules.RCON, '+set', 'g_gametype', 'dm', '+set', 'g_log', 'games_mp.log',
                 '+set', 'g_logSync', '1', '+set', 'dedicated', '1', '+map', 'mp_toujane'],
                stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL, cwd=home))
            up = smoke_test.wait_for(lambda: smoke_test.query(port, 'getinfo x'), 150)
            check('the server runs the map', up)
            if up:
                with open(client_log, 'wb') as log:
                    procs.append(subprocess.Popen(
                        [os.path.abspath(args.client), '+set', 'fs_basepath', os.path.abspath(args.game_dir),
                         '+set', 'fs_homepath', client_home, '+set', 'net_port', str(port + 1),
                         '+connect', f'127.0.0.1:{port}'],
                        stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL, cwd=client_home,
                        env=dict(os.environ, COD2_TRACE_SOUND='1')))
                joined = smoke_test.wait_for(lambda: 'plustest spawned' in logged(), 120)
                check('the observer joins', joined)
            if not failures:
                time.sleep(5)  # its raise at spawn
                m = re.search(r'"sv_serverid" is: ?"(-?\d+)', smoke_test.rcon(port, rules.RCON, 'sv_serverid'))
                climber = Climber(port)
                ok = climber.connect(118, 'climber') is None and climber.request_gamestate(int(m.group(1)))
                check('the climber joins', ok)
            if not failures:
                climber.enter_game(int(m.group(1)), 1.0)
                states = rules.frames_until(climber, lambda s: any(p.get('weapon') for p in s), 10)
                weapon = next((p['weapon'] for p in states if p.get('weapon')), 0)
                rules.settle(climber, weapon)
                smoke_test.rcon(port, rules.RCON, 'set plustest_spot 1')
                rules.frames_until(climber, lambda s: False, 1.0)
                number = states[-1].get('clientNum', 0)
                # walk at the ledge and jump: the mantle
                climber.script = tuple([(0, True)] * 6 + [(BUTTON_JUMP, True)] * 4 + [(0, True)] * 30)
                states = rules.frames_until(climber, lambda s: False, 4)
                check('the climber mantles', any(p.get('pm_flags', 0) & 0x4 for p in states))  # PMF_MANTLE
                time.sleep(2)
            for p in procs:
                check(f'{"the server" if p is procs[0] else "the observer"} still runs', p.poll() is None)
        finally:
            if climber:
                climber.close()
            for p in reversed(procs):
                if p.poll() is None:
                    p.terminate()
                    try:
                        p.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        p.kill()
        text = open(client_log, encoding='latin-1').read() if os.path.exists(client_log) else ''
        if climber and not failures[:1]:
            heard = re.findall(rf'^SoundTrace: (weap_\w+) ent {number}$', text, re.M)
            check('the observer hears the climber\'s weapon go up', 'weap_raise' in heard, ', '.join(heard) or 'nothing')
        reports = [n for n in os.listdir(client_home) if n.startswith('cod2_crash_')]
        check('no crash report from the observer', 'CRASH REPORT' not in text and not reports, ', '.join(reports))
    print('\n' + ('FAILED: ' + ', '.join(failures) if failures else 'All checks passed'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
