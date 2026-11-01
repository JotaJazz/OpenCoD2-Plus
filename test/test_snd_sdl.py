import os, subprocess, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAMES = ['mix_silence', 'mix_pcm16_unity', 'mix_pcm8_unsigned', 'mix_stereo_source', 'mix_stereo_source_r',
         'mix_resample_2x', 'mix_resample_half', 'mix_rate_22_to_44', 'mix_loop_wraps',
         'mix_saturates', 'mix_rate_zero', 'mix_mono_output', 'mix_wet']


class SndSdl(unittest.TestCase):
    """The SDL sound engine's pieces that need no SDL and no game."""

    def run_test(self, flags):
        with tempfile.TemporaryDirectory() as tmp:
            exe = os.path.join(tmp, 't')
            subprocess.run(['gcc', '-Wall', '-Werror', *flags, '-I', os.path.join(ROOT, 'src', 'snd_sdl'),
                            os.path.join(ROOT, 'test', 'snd_sdl_test.c'),
                            os.path.join(ROOT, 'src', 'snd_sdl', 'snd_mix.c'), '-o', exe, '-lm'], check=True)
            proc = subprocess.run([exe], capture_output=True, text=True)
        self.assertEqual(proc.returncode, 0, proc.stdout)
        lines = dict(l.split() for l in proc.stdout.splitlines())
        for name in NAMES:
            self.assertEqual(lines.get(name), 'PASS', name)

    def test_64bit(self):
        self.run_test([])

    def test_32bit(self):
        self.run_test(['-m32'])


if __name__ == '__main__':
    unittest.main()
