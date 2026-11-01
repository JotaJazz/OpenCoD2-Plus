import glob, os, re, shutil, subprocess, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAMES = ['mix_silence', 'mix_pcm16_unity', 'mix_pcm8_unsigned', 'mix_stereo_source', 'mix_stereo_source_r',
         'mix_resample_2x', 'mix_resample_half', 'mix_rate_22_to_44', 'mix_loop_wraps',
         'mix_saturates', 'mix_rate_zero', 'mix_mono_output', 'mix_wet',
         'wav_pcm16_mono', 'wav_stereo_44', 'wav_chunks_extra', 'wav_adpcm', 'wav_adpcm_stereo', 'wav_adpcm_cap',
         'wav_adpcm_bad', 'convert_downsample', 'convert_stereo_to_mono', 'convert_identity_stereo16',
         'wav_truncated', 'wav_data_len_lies', 'wav_block_zero', 'wav_mp3_rejected',
         'spatial_center', 'spatial_full_left', 'spatial_behind', 'spatial_inside_min', 'spatial_beyond_max',
         'spatial_rolloff', 'spatial_zero_distance', 'volume_pan_extremes',
         'reverb_generic_off', 'reverb_impulse_tail', 'reverb_room_differs', 'reverb_decays',
         'reverb_no_alloc_in_callback', 'reverb_rate_change', 'reverb_room_fade', 'reverb_sleeps',
         'reverb_mixes', 'reverb_bad_input']
STREAM_NAMES = ['stream_wav_plays', 'stream_mp3_plays', 'stream_loop', 'stream_seek_ms', 'stream_underrun_silence',
                'stream_close_while_reading', 'stream_garbage', 'stream_open_fails']
AIL_NAMES = ['ail_provider', 'ail_wav_length', 'ail_no_device', 'ail_status', 'ail_hunk_freed', 'ail_positions', 'ail_adpcm',
             'ail_3d', 'ail_device_gone', 'ail_raw', 'ail_pitch_sample', 'ail_pitch_stream_2x', 'ail_pitch_stream_11k',
             'ail_gain_cap', 'ail_53_voices', 'ail_stream_retry', 'ail_stream_life',
             'ail_reverb_2d', 'ail_reverb_3d', 'ail_reverb_stream', 'ail_reverb_generic']
SDL32 = glob.glob('/usr/lib/i386-linux-gnu/libSDL2.so')
SDL64 = glob.glob('/usr/lib/x86_64-linux-gnu/libSDL2-2.0.so.0')
SND_SDL = os.path.join(ROOT, 'src', 'snd_sdl')
MINGW_GCC = shutil.which('i686-w64-mingw32-gcc')
# NULL from SndSdl_Find: the window is DirectSound's
NOT_IN_TABLE = {('AIL_set_DirectSound_HWND', '8')}


class SndSdl(unittest.TestCase):
    """The SDL sound engine's pieces that need no SDL and no game."""

    def run_test(self, flags, sdl=None, prefix=(), env=None):
        """sdl: what to link for the stream cases; without it the mixer builds alone, free of SDL,
        which also keeps its cases running on a machine that lacks that build of the library."""
        names, stream = NAMES, []
        with tempfile.TemporaryDirectory() as tmp:
            if sdl:
                names = NAMES + STREAM_NAMES + AIL_NAMES
                stream = ['-DSND_TEST_STREAMS', '-DSND_TEST_AIL', '-I', os.path.join(ROOT, 'test'), '-I', tmp,
                          os.path.join(ROOT, 'src', 'snd_sdl', 'snd_stream.c'),
                          os.path.join(ROOT, 'src', 'snd_sdl', 'snd_ail.c'), *sdl]
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
                            os.path.join(ROOT, 'src', 'snd_sdl', 'snd_spatial.c'), os.path.join(ROOT, 'src', 'snd_sdl', 'snd_reverb.c'),
                            *stream, '-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc', '-o', exe, '-lm'], check=True)
            proc = subprocess.run([*prefix, exe], capture_output=True, text=True, timeout=300,
                                  env={**os.environ, 'SND_TEST_DIR': tmp, **(env or {})})
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        self.assertEqual(proc.stderr, '')
        lines = dict(l.split() for l in proc.stdout.splitlines() if not l.startswith('#'))
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


class SndBackend(unittest.TestCase):
    """The Windows selector: mss32_dll.c's calls have to find an engine function with their stack size."""

    def miles_calls(self):
        with open(os.path.join(ROOT, 'src', 'PC', 'win32', 'mss32_dll.c')) as f:
            return set(re.findall(r'MSS_FN\((\w+), (\d+)\)', f.read())) - NOT_IN_TABLE

    def test_table_has_every_miles_call(self):
        with open(os.path.join(SND_SDL, 'snd_ail.c')) as f:
            table = set(re.findall(r'\{ "_(\w+)@(\d+)", Sdl_\1 \}', f.read()))
        self.assertGreater(len(self.miles_calls()), 60)
        self.assertEqual(self.miles_calls() - table, set())

    @unittest.skipUnless(MINGW_GCC, 'no MinGW')
    def test_stdcall_sizes(self):
        """A __stdcall callee pops its arguments, so a wrong size wrecks the caller's stack: nm shows the real one."""
        with tempfile.TemporaryDirectory() as tmp:
            obj = os.path.join(tmp, 'snd_ail.o')
            subprocess.run([MINGW_GCC, '-c', '-DCOD2_MSS32_DLL=1', '-I', SND_SDL, '-I',
                            os.path.join(ROOT, 'src', 'win32', 'sdl2', 'include'),
                            os.path.join(SND_SDL, 'snd_ail.c'), '-o', obj], check=True)
            symbols = subprocess.run(['i686-w64-mingw32-nm', obj], capture_output=True, text=True, check=True).stdout
        have = set(re.findall(r' T _Sdl_(\w+)@(\d+)$', symbols, re.M))
        self.assertEqual(self.miles_calls() - have, set())


if __name__ == '__main__':
    unittest.main()
