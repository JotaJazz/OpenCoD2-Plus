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


if __name__ == '__main__':
    unittest.main()
