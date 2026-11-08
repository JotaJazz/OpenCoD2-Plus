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


if __name__ == '__main__':
    unittest.main()
