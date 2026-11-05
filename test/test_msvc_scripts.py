"""Safety of the WSL-to-Windows scripts: msvc_build.py's mirror and msvc_run_test.py's kill."""
import os, sys, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import msvc_build
import msvc_run_test


class Mirror(unittest.TestCase):
    def test_refuses_a_folder_it_did_not_make(self):
        """A mistyped --src (a game folder, a drive) must never be pruned."""
        with tempfile.TemporaryDirectory() as tmp:
            keep = os.path.join(tmp, 'jugar.bat')
            open(keep, 'w').close()
            with self.assertRaises(SystemExit):
                msvc_build.claim_mirror(tmp)
            self.assertTrue(os.path.exists(keep))

    def test_claims_an_empty_or_new_folder(self):
        with tempfile.TemporaryDirectory() as tmp:
            new = os.path.join(tmp, 'copy')
            msvc_build.claim_mirror(new)
            self.assertTrue(os.path.exists(os.path.join(new, msvc_build.MARKER)))
            msvc_build.claim_mirror(new)   # its own copy again: fine


class Verdict(unittest.TestCase):
    def test_crash_report_named_with_missing_markers(self):
        """A crash that kept the map from loading must be in the message, not only the missing lines."""
        self.assertEqual(msvc_run_test.verdict(['the map loads'], ['cod2_crash_7.txt']),
                         ['the map loads', 'no crash report (cod2_crash_7.txt)'])
        self.assertEqual(msvc_run_test.verdict([], []), [])


class Kill(unittest.TestCase):
    def test_kills_only_the_tested_copy(self):
        """Another cod2_win32.exe (the user's game) must survive the test."""
        cmd = msvc_run_test.kill_command(r'E:\opencod2-msvc-test\check-msvc-client\cod2_win32.exe')
        self.assertNotIn('/IM', cmd)
        self.assertIn(r"E:\opencod2-msvc-test\check-msvc-client\cod2_win32.exe", ' '.join(cmd))


if __name__ == '__main__':
    unittest.main()
