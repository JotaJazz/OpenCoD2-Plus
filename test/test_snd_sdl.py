import glob, os, subprocess, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAMES = ['mix_silence', 'mix_pcm16_unity', 'mix_pcm8_unsigned', 'mix_stereo_source', 'mix_stereo_source_r',
         'mix_resample_2x', 'mix_resample_half', 'mix_rate_22_to_44', 'mix_loop_wraps',
         'mix_saturates', 'mix_rate_zero', 'mix_mono_output', 'mix_wet',
         'wav_pcm16_mono', 'wav_stereo_44', 'wav_chunks_extra', 'wav_adpcm', 'wav_adpcm_stereo', 'wav_adpcm_cap',
         'wav_adpcm_bad', 'convert_downsample', 'convert_stereo_to_mono', 'convert_identity_stereo16',
         'wav_truncated', 'wav_data_len_lies', 'wav_block_zero', 'wav_mp3_rejected',
         'spatial_center', 'spatial_full_left', 'spatial_behind', 'spatial_inside_min', 'spatial_beyond_max',
         'spatial_rolloff', 'spatial_zero_distance', 'volume_pan_extremes']
STREAM_NAMES = ['stream_wav_plays', 'stream_mp3_plays', 'stream_loop', 'stream_seek_ms', 'stream_underrun_silence',
                'stream_close_while_reading', 'stream_garbage', 'stream_open_fails']
SDL32 = glob.glob('/usr/lib/i386-linux-gnu/libSDL2.so')
SDL64 = glob.glob('/usr/lib/x86_64-linux-gnu/libSDL2-2.0.so.0')


class SndSdl(unittest.TestCase):
    """The SDL sound engine's pieces that need no SDL and no game."""

    def run_test(self, flags, sdl=None, prefix=(), env=None):
        """sdl: what to link for the stream cases; without it the mixer builds alone, free of SDL,
        which also keeps its cases running on a machine that lacks that build of the library."""
        names, stream = NAMES, []
        with tempfile.TemporaryDirectory() as tmp:
            if sdl:
                names = NAMES + STREAM_NAMES
                stream = ['-DSND_TEST_STREAMS', '-I', os.path.join(ROOT, 'test'), '-I', tmp,
                          os.path.join(ROOT, 'src', 'snd_sdl', 'snd_stream.c'), *sdl]
                # Debian's 64-bit SDL_config.h comes with the 64-bit dev package; only the 32-bit one may be here
                if not os.path.exists('/usr/include/x86_64-linux-gnu/SDL2') and '-m32' not in flags:
                    os.mkdir(os.path.join(tmp, 'SDL2'))
                    with open(os.path.join(tmp, 'SDL2', '_real_SDL_config.h'), 'w') as f:
                        f.write('#include "/usr/include/i386-linux-gnu/SDL2/_real_SDL_config.h"\n'
                                '#undef SIZEOF_VOIDP\n#define SIZEOF_VOIDP 8\n')
            exe = os.path.join(tmp, 't')
            subprocess.run(['gcc', '-Wall', '-Werror', '-g', *flags, '-I', os.path.join(ROOT, 'src', 'snd_sdl'),
                            os.path.join(ROOT, 'test', 'snd_sdl_test.c'),
                            os.path.join(ROOT, 'src', 'snd_sdl', 'snd_mix.c'), os.path.join(ROOT, 'src', 'snd_sdl', 'snd_wav.c'),
                            os.path.join(ROOT, 'src', 'snd_sdl', 'snd_spatial.c'), *stream, '-o', exe, '-lm'], check=True)
            proc = subprocess.run([*prefix, exe], capture_output=True, text=True, timeout=300,
                                  env={**os.environ, **(env or {})})
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        self.assertEqual(proc.stderr, '')
        lines = dict(l.split() for l in proc.stdout.splitlines())
        for name in names:
            self.assertEqual(lines.get(name), 'PASS', name)

    def test_no_sdl(self):
        self.run_test([])

    def test_64bit(self):
        self.run_test(['-fsanitize=address'], SDL64)

    def test_32bit(self):
        self.run_test(['-m32'], SDL32 and ['-lSDL2'])

    @unittest.skipUnless(SDL64, 'no 64-bit libSDL2')
    def test_thread_sanitizer(self):
        # setarch -R: TSan's memory layout does not fit this kernel's randomized one
        self.run_test(['-fsanitize=thread'], SDL64, ['setarch', '-R'], {'TSAN_OPTIONS': 'halt_on_error=1'})


if __name__ == '__main__':
    unittest.main()
