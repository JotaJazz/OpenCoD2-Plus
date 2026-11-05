import io, os, subprocess, sys, tempfile, unittest
from contextlib import redirect_stdout
from unittest import mock

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import build_check


class BuildCheck(unittest.TestCase):
    def test_crash_is_reported(self):
        """A script that dies writes to stderr only: that is not a pass."""
        with tempfile.TemporaryDirectory() as tmp:
            os.makedirs(os.path.join(tmp, 'CMakeFiles', 'cod2_lnxded.dir'))
            crash = subprocess.CompletedProcess([], 1, stdout='', stderr='Traceback\nboom')
            with mock.patch.object(build_check.subprocess, 'run', return_value=crash):
                errors = build_check.duplicates('linux', tmp)
        self.assertTrue(any('boom' in e for e in errors), errors)

    def run_main(self, *argv):
        out = io.StringIO()
        with mock.patch.object(sys, 'argv', ['build_check.py'] + list(argv)), redirect_stdout(out):
            return build_check.main(), out.getvalue()

    def test_skip_is_a_failure(self):
        with mock.patch.object(build_check, 'SDL2_MINGW', '/no/such/file'):
            code, out = self.run_main('mingw-client')
        self.assertEqual(code, 1, out)
        self.assertIn('skip', out)

    def test_allow_skip(self):
        with mock.patch.object(build_check, 'SDL2_MINGW', '/no/such/file'):
            code, out = self.run_main('mingw-client', '--allow-skip')
        self.assertEqual(code, 0, out)


    def test_client_x64_skipped_without_its_sdl2(self):
        with mock.patch.object(build_check, 'LINUX_X64_SDL2', '/no/such/file'):
            code, out = self.run_main('linux-client-x64', '--allow-skip')
        self.assertEqual(code, 0, out)
        self.assertIn('skip linux-client-x64', out)

    def test_client_x64_builds_the_64_bit_client(self):
        """The 64-bit client is the engine's build, not the dedicated server's project."""
        self.assertIn('-DCOD2_X64=ON', build_check.VARIANTS['linux-client-x64'])
        self.assertFalse(build_check.is_x64('linux-client-x64'))
        calls = []
        with mock.patch.object(build_check.subprocess, 'run',
                               side_effect=lambda cmd, **kw: calls.append(cmd) or subprocess.CompletedProcess(cmd, 0, '', '')):
            build_check.build('linux-client-x64', [])
        self.assertEqual(calls[0][2], build_check.ROOT)
        self.assertEqual(calls[1][calls[1].index('--target') + 1], 'cod2_linux')


    def test_client_x64_checks_only_the_client(self):
        """It builds cod2_linux alone: cod2_lnxded's objects do not exist."""
        with tempfile.TemporaryDirectory() as tmp:
            for target in ('cod2_lnxded', 'cod2_linux'):
                os.makedirs(os.path.join(tmp, 'CMakeFiles', target + '.dir'))
            calls = []
            with mock.patch.object(build_check.subprocess, 'run',
                                   side_effect=lambda cmd, **kw: calls.append(cmd) or subprocess.CompletedProcess(cmd, 0, '', '')):
                build_check.duplicates('linux-client-x64', tmp)
        self.assertTrue(calls)
        self.assertFalse(any('cod2_lnxded' in cmd for cmd in calls), calls)


    def test_mingw_client_x64_skipped_without_its_sdl2(self):
        with mock.patch.object(build_check, 'SDL2_MINGW64', '/no/such/file'):
            code, out = self.run_main('mingw-client-x64', '--allow-skip')
        self.assertEqual(code, 0, out)
        self.assertIn('skip mingw-client-x64', out)

    def test_mingw_client_x64_uses_the_x64_toolchain(self):
        flags = build_check.VARIANTS['mingw-client-x64']
        self.assertIn('-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw64.cmake', flags)
        self.assertIn('-DCOD2_X64=ON', flags)
        self.assertIn('-DCOD2_WIN32_CLIENT=ON', flags)

    def test_mingw_client_x64_symbols_have_no_underscore(self):
        """PE x64 decorates no C name: nm and the prefix are x86_64's."""
        with tempfile.TemporaryDirectory() as tmp:
            os.makedirs(os.path.join(tmp, 'CMakeFiles', 'cod2_win32.dir'))
            calls = []
            with mock.patch.object(build_check.subprocess, 'run',
                                   side_effect=lambda cmd, **kw: calls.append(cmd) or subprocess.CompletedProcess(cmd, 0, '', '')):
                build_check.duplicates('mingw-client-x64', tmp)
        nms = {cmd[cmd.index('--nm') + 1] for cmd in calls if '--nm' in cmd}
        prefixes = {cmd[cmd.index('--prefix') + 1] for cmd in calls if '--prefix' in cmd}
        self.assertTrue(nms and all(nm.endswith('x86_64-w64-mingw32-nm') for nm in nms), nms)
        self.assertEqual(prefixes, {''})


    def test_msvc_client_skipped_without_windows(self):
        """msvc-client builds through cmd.exe: off Windows (no interop) it is a skip."""
        with mock.patch.object(build_check.shutil, 'which', return_value=None):
            code, out = self.run_main('msvc-client', '--allow-skip')
        self.assertEqual(code, 0, out)
        self.assertIn('skip msvc-client', out)

    def test_msvc_client_builds_through_msvc_build(self):
        """The variant is the Windows branch with the clang MSVC-ABI toolchain, driven by msvc_build.py."""
        flags = build_check.VARIANTS['msvc-client']
        self.assertIn('-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-msvc-clang.cmake', flags)
        self.assertIn('-DCOD2_WIN32_CLIENT=ON', flags)
        calls = []
        with mock.patch.object(build_check.subprocess, 'run',
                               side_effect=lambda cmd, **kw: calls.append(cmd) or subprocess.CompletedProcess(cmd, 0, '', '')):
            build_check.build('msvc-client', [])
        self.assertTrue(calls[0][1].endswith('msvc_build.py'), calls)

    def test_web_skipped_without_emcc(self):
        with mock.patch.object(build_check, 'EMSDK_ENV', '/no/such/emsdk_env.sh'):
            code, out = self.run_main('web', '--allow-skip')
        self.assertEqual(code, 0, out)
        self.assertIn('skip web', out)

    def test_web_undefined_symbols_may_only_shrink(self):
        """The web link tolerates undefined symbols (as upstream); a new one is a failure."""
        log = 'warning: undefined symbol: CG_Init (referenced by root reference)\nwarning: undefined symbol: Brand_New (x)\n'
        self.assertEqual(build_check.web_undefined_new(log, {'CG_Init', 'Old_Gone'}), ['Brand_New'])

    def test_stock_13_variant(self):
        """linux-13 is the 1.3 build without the Plus protocol."""
        self.assertEqual(build_check.VARIANTS['linux-13'], ['-DCOD2_PLUS_PROTOCOL=OFF'])
        self.assertFalse(build_check.is_x64('linux-13'))


if __name__ == '__main__':
    unittest.main()
