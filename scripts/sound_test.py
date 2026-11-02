#!/usr/bin/env python3
"""A client plays sound: what it sends to the device, read from SDL's disk driver.

    python3 scripts/sound_test.py /path/to/CoD2 --protocol {115|118} [--binary PATH] [--wine CMD] [--cases a,b] [--keep DIR]

Each case starts a client (cod2_linux) with SDL_AUDIODRIVER=disk, so that its
output lands in a file as the device would get it, and measures that file.
menu_music: the menu's MP3 stream has an RMS over MIN_RMS. local_2d: a
playLocalSound rises above the silence before it. left_right: a script_origin
to the player's left and then to the right, with playSound: the channel on that
side dominates by at least DOMINANCE. map_change: map_restart, vid_restart,
disconnect and quit with sounds playing leave no crash report. no_device:
SDL_AUDIODRIVER=nonexistent reaches the menu with the failure in its log and no
crash. snd_khz_22: the menu's music at 22 kHz. callback_cost: bots fighting on a
local game (scripts/install_bots.py), the worst callback snd_stats prints is
below half the buffer. Exits with 0 when every case passes. --wine runs a
Windows client (cod2_win32.exe, given with --binary) with that command. --keep copies each case's log and
audio to DIR. The client opens a window and needs a CD key in
~/.cod2/preferences; without one the test is skipped (exit 0).
"""
import argparse, array, glob, math, os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from fx_test import DEFAULT_CLIENT, GAMETYPE, PITCH, SPOT
from hit_location_test import PREFERENCES
from listen_test import wine_path

FPS = 20
MENU_FRAMES = 500
TIMEOUT = 150             # seconds for a case, map load included
BOTS_TIMEOUT = 300
MIN_RMS = 100             # of 32767
DOMINANCE = 3
WINDOW = 0.05             # seconds an RMS is taken over
GAP = 1.0                 # seconds of quiet between two sounds
LOCAL_ALIAS = 'weap_kar98k_fire_plr'   # local, loaded, mono
WORLD_ALIAS = 'weap_kar98k_fire'       # 3D, loaded, mono
START = '''
	self thread soundTest();
'''
LOCAL = '''
soundTest()
{
	ambientStop(0);
	wait 8;
	for(i = 0; i < 3; i++)
	{
		self playLocalSound("%s");
		wait 3;
	}
}
''' % LOCAL_ALIAS
LEFT_RIGHT = '''
soundTest()
{
	ambientStop(0);
	wait 8;
	for(side = -1; side <= 1; side += 2)
	{
		ent = spawn("script_origin", self getEye() + vectorScale(anglesToRight(self getPlayerAngles()), side * 30));
		wait 1;
		ent playSound("%s");
		wait 5;
		ent delete();
	}
}
''' % WORLD_ALIAS
NOISE = '''
soundTest()
{
	ent = spawn("script_origin", self.origin + (100, 0, 0));
	for(;;)
	{
		self playLocalSound("%s");
		ent playSound("%s");
		wait 0.1;
	}
}
''' % (LOCAL_ALIAS, WORLD_ALIAS)


def gametype(test):
    """fx_test's gametype, whose player starts test as it spawns."""
    text = GAMETYPE % dict(zip(('x', 'y', 'z', 'yaw'), SPOT.split()), pitch=PITCH)
    return text.replace('\tself spawn(', START + '\tself spawn(', 1) + test


def read_audio(path):
    """The interleaved 16 bit samples of a disk driver file, or an empty array."""
    samples = array.array('h')
    if os.path.exists(path):
        with open(path, 'rb') as f:
            samples.frombytes(f.read(os.path.getsize(path) // 2 * 2))
    return samples


def rms(samples):
    return math.sqrt(sum(s * s for s in samples) / len(samples)) if len(samples) else 0.0


def windows(samples, rate):
    """(left RMS, right RMS) of each WINDOW of stereo samples."""
    step = int(WINDOW * rate) * 2
    return [(rms(samples[i:i + step:2]), rms(samples[i + 1:i + step:2])) for i in range(0, len(samples), step)]


def events(win):
    """The (left, right) energy of each burst, split by GAP of quiet."""
    top = max((max(w) for w in win), default=0)
    if top < MIN_RMS:
        return []
    out, quiet = [], None
    for w in win:
        if max(w) > top * 0.1:
            if quiet is None or quiet * WINDOW >= GAP:
                out.append([0.0, 0.0])
            out[-1][0] += w[0] ** 2
            out[-1][1] += w[1] ** 2
            quiet = 0
        elif quiet is not None:
            quiet += 1
    return out


class Run:
    def __init__(self, args):
        self.args = args
        self.failures = []
        self.game = os.path.abspath(args.game_dir)
        self.binary = os.path.abspath(args.binary or os.path.join(ROOT, DEFAULT_CLIENT[args.protocol]))

    def check(self, name, ok, detail=''):
        print(f'{"ok  " if ok else "FAIL"} {name}' + (f' ({detail})' if detail else ''), flush=True)
        if not ok:
            self.failures.append(name)

    def client(self, name, cfg, game_args=(), test=None, env=None, driver='disk', setup=None, seconds=TIMEOUT):
        """Runs a client to its config's quit. Returns (samples, log text)."""
        with tempfile.TemporaryDirectory(prefix='cod2_sound_') as home:
            os.makedirs(os.path.join(home, 'main'))
            with open(os.path.join(home, 'main', 'test.cfg'), 'w') as f:
                f.write(cfg)
            if test:
                mod = os.path.join(home, 'soundtest', 'maps', 'mp', 'gametypes')
                os.makedirs(mod)
                with open(os.path.join(mod, 'dm.gsc'), 'w') as f:
                    f.write(gametype(test))
                game_args = ['+set', 'fs_game', 'soundtest', '+set', 'g_gametype', 'dm', '+map', 'mp_toujane', *game_args]
            if setup:
                game_args = setup(home) + list(game_args)
            audio, log_path = os.path.join(home, 'out.raw'), os.path.join(home, 'out.log')
            base, homepath = self.game, home
            command, audio_path = [self.binary], audio
            if self.args.wine:
                base, homepath, audio_path = wine_path(self.game), wine_path(home), wine_path(audio)
                command = [self.args.wine, self.binary]
            with open(log_path, 'wb') as log:
                client = subprocess.Popen(
                    command + ['+set', 'fs_basepath', base, '+set', 'fs_homepath', homepath, '+set', 'com_maxfps', str(FPS),
                               *game_args, '+exec', 'test.cfg'],
                    cwd=home, stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                    env=dict(os.environ, SDL_AUDIODRIVER=driver, SDL_DISKAUDIOFILE=audio_path, COD2_CONSOLE_STDOUT='1',
                             COD2_CRASH_NOPOPUP='1', **(env or {})))
                try:
                    client.wait(timeout=seconds)
                except subprocess.TimeoutExpired:
                    self.check(f'{name}: the client quits in time', False)
                    client.terminate()
                    try:
                        client.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        client.kill()
                self.check(f'{name}: exits 0', client.returncode == 0, f'{client.returncode}')
            text = open(log_path, encoding='latin-1').read()
            crashes = glob.glob(os.path.join(home, 'cod2_crash_*'))
            self.check(f'{name}: no crash report', 'CRASH REPORT' not in text and not crashes)
            samples = read_audio(audio)
            if self.args.keep:
                os.makedirs(self.args.keep, exist_ok=True)
                for path, out in ((log_path, f'{name}.log'), (audio, f'{name}.raw')):
                    if os.path.exists(path):
                        shutil.copy(path, os.path.join(self.args.keep, out))
        return samples, text

    def menu_music(self):
        samples, _ = self.client('menu_music', f'wait {MENU_FRAMES}\nquit\n')
        level = rms(samples)
        self.check('menu_music: RMS over the threshold', level > MIN_RMS, f'{level:.0f}, {len(samples)} samples')

    def local_2d(self):
        samples, _ = self.client('local_2d', f'wait {FPS * 20}\nquit\n', test=LOCAL)
        win = [max(w) for w in windows(samples, 44100)]
        top, floor = max(win, default=0), sorted(win)[len(win) // 2] if win else 0
        self.check('local_2d: the sound rises over what comes before', top > MIN_RMS and top > 4 * floor,
                   f'peak {top:.0f}, median {floor:.0f}')

    def left_right(self):
        samples, _ = self.client('left_right', f'wait {FPS * 45}\nquit\n', test=LEFT_RIGHT)
        found = events(windows(samples, 44100))
        self.check('left_right: both sounds are heard', len(found) >= 2, f'{len(found)} bursts')
        if len(found) >= 2:
            left, right = [[math.sqrt(x) for x in e] for e in found[-2:]]
            self.check('left_right: left dominates on the left', left[0] >= DOMINANCE * left[1],
                       f'{left[0]:.0f} to {left[1]:.0f}')
            self.check('left_right: right dominates on the right', right[1] >= DOMINANCE * right[0],
                       f'{right[1]:.0f} to {right[0]:.0f}')

    def map_change(self):
        frames = FPS * 8
        self.client('map_change', f'wait {frames}\nmap_restart\nwait {frames}\nvid_restart\nwait {frames}\n'
                                  f'disconnect\nwait {frames}\nquit\n', test=NOISE)

    def no_device(self):
        _, text = self.client('no_device', f'wait {MENU_FRAMES // 2}\nquit\n', driver='nonexistent')
        self.check('no_device: the log has the failure', "couldn't initialize 2D provider" in text)
        self.check('no_device: the menu comes up', re.search(r'\[rbdiag\].* draw=[1-9]', text) is not None)

    def snd_khz_22(self):
        samples, text = self.client('snd_khz_22', f'wait {MENU_FRAMES}\nquit\n', ['+set', 'snd_khz', '22'])
        level = rms(samples)
        self.check('snd_khz_22: the device opens at 22 kHz', 'Attempting 22 kHz' in text)
        self.check('snd_khz_22: RMS over the threshold', level > MIN_RMS, f'{level:.0f}, {len(samples)} samples')

    def callback_cost(self):
        def bots(home):
            subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'install_bots.py'), self.game,
                            '--dest', home, '--name', 'soundbots'], check=True, stdout=subprocess.DEVNULL)
            return ['+set', 'fs_game', 'soundbots', '+set', 'g_gametype', 'tdm', '+set', 'scr_tdm_timelimit', '0',
                    '+set', 'bots_count', '6', '+map', 'mp_toujane']
        _, text = self.client('callback_cost', f'wait {FPS * 150}\nsnd_stats\nquit\n', setup=bots, seconds=BOTS_TIMEOUT)
        m = re.search(r'snd_stats: worst callback ([\d.]+) ms, last [\d.]+ ms, buffer ([\d.]+) ms, underruns (\d+)', text)
        self.check('callback_cost: snd_stats answers', m is not None)
        if m:
            worst, buffer = float(m.group(1)), float(m.group(2))
            self.check('callback_cost: the worst callback is under half the buffer', worst < buffer / 2,
                       f'{worst:.2f} of {buffer:.2f} ms, {m.group(3)} underruns')


CASES = ('menu_music', 'local_2d', 'left_right', 'map_change', 'no_device', 'snd_khz_22', 'callback_cost')


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('game_dir')
    ap.add_argument('--protocol', type=int, choices=(115, 118), required=True)
    ap.add_argument('--binary', help='the client')
    ap.add_argument('--wine', help='the command that runs a Windows client')
    ap.add_argument('--cases', help='comma separated, by default all: ' + ','.join(CASES))
    ap.add_argument('--keep', help="copy each case's log and audio here")
    args = ap.parse_args()
    if args.wine and not args.binary:
        ap.error('--wine needs --binary, the .exe')
    if not os.path.exists(PREFERENCES) or 'codkey' not in open(PREFERENCES).read():
        print(f'skipped: no CD key in {PREFERENCES}')
        return 0
    run = Run(args)
    for name in args.cases.split(',') if args.cases else CASES:
        if name not in CASES:
            ap.error(f'no case {name}')
        getattr(run, name)()
    print('PASS' if not run.failures else f'FAIL: {len(run.failures)} check(s)')
    return 0 if not run.failures else 1


if __name__ == '__main__':
    sys.exit(main())
