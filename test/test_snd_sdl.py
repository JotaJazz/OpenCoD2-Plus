import os, subprocess, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAMES = ['mix_silence', 'mix_pcm16_unity', 'mix_pcm8_unsigned', 'mix_stereo_source', 'mix_stereo_source_r',
         'mix_resample_2x', 'mix_resample_half', 'mix_rate_22_to_44', 'mix_loop_wraps',
         'mix_saturates', 'mix_rate_zero', 'mix_mono_output', 'mix_wet',
         'wav_pcm16_mono', 'wav_stereo_44', 'wav_chunks_extra', 'wav_adpcm', 'wav_adpcm_stereo', 'wav_adpcm_cap',
         'wav_adpcm_bad', 'convert_downsample', 'convert_stereo_to_mono', 'convert_identity_stereo16',
         'wav_truncated', 'wav_data_len_lies', 'wav_block_zero', 'wav_mp3_rejected',
         'spatial_center', 'spatial_full_left', 'spatial_behind', 'spatial_inside_min', 'spatial_beyond_max',
         'spatial_rolloff', 'spatial_zero_distance', 'volume_pan_extremes']


class SndSdl(unittest.TestCase):
    """The SDL sound engine's pieces that need no SDL and no game."""

    def run_test(self, flags):
        with tempfile.TemporaryDirectory() as tmp:
            exe = os.path.join(tmp, 't')
            subprocess.run(['gcc', '-Wall', '-Werror', '-g', *flags, '-I', os.path.join(ROOT, 'src', 'snd_sdl'),
                            os.path.join(ROOT, 'test', 'snd_sdl_test.c'),
                            os.path.join(ROOT, 'src', 'snd_sdl', 'snd_mix.c'), os.path.join(ROOT, 'src', 'snd_sdl', 'snd_wav.c'),
                            os.path.join(ROOT, 'src', 'snd_sdl', 'snd_spatial.c'), '-o', exe, '-lm'], check=True)
            proc = subprocess.run([exe], capture_output=True, text=True)
        self.assertEqual(proc.returncode, 0, proc.stdout)
        lines = dict(l.split() for l in proc.stdout.splitlines())
        for name in NAMES:
            self.assertEqual(lines.get(name), 'PASS', name)

    def test_64bit(self):
        self.run_test(['-fsanitize=address'])

    def test_32bit(self):
        self.run_test(['-m32'])


if __name__ == '__main__':
    unittest.main()
