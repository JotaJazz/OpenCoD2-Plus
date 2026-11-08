"""scripts/msvc_run_test.py's check of a client's console."""
import os, sys, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import msvc_run_test


class MsvcRunTest(unittest.TestCase):
    def test_menu_only_is_not_a_game(self):
        """The main menu draws frames, but no map loaded and no server ran."""
        log = '[rbdiag] q=960 disp=960 count=720 draw=720 nomat=0\n'
        self.assertEqual(msvc_run_test.missing(log)[:2], ['the local server starts the map', 'the map loads'])
        self.assertNotIn('frames are drawn', msvc_run_test.missing(log))

    def test_game_with_no_frames(self):
        log = '\n'.join(line for _, line in msvc_run_test.MARKERS) + '\n[rbdiag] q=0 count=0 draw=0\n'
        self.assertEqual(msvc_run_test.missing(log), ['frames are drawn'])

    def test_full_game(self):
        log = '\n'.join(line for _, line in msvc_run_test.MARKERS) + '\n[rbdiag] count=476 draw=476 nomat=0\n'
        self.assertEqual(msvc_run_test.missing(log), [])


if __name__ == '__main__':
    unittest.main()
