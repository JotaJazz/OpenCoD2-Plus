import os, subprocess, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PROGRAM = r'''
#include <stdio.h>
#include "in_mouse.h"
int main(void)
{
    /* has a client, focused, key catchers, full screen, in_mouse */
    printf("%d", IN_WantMouse(1, 1, 0, 1, 1));  /* playing: captured */
    printf("%d", IN_WantMouse(1, 1, 0, 0, 1));  /* in a window too */
    printf("%d", IN_WantMouse(1, 1, 2, 0, 1));  /* in a menu too */
    printf("%d", IN_WantMouse(1, 1, 1, 1, 1));  /* console, full screen */
    printf("%d", IN_WantMouse(1, 1, 1, 0, 1));  /* console in a window: let go */
    printf("%d", IN_WantMouse(1, 0, 0, 1, 1));  /* no focus: let go */
    printf("%d", IN_WantMouse(0, 1, 0, 1, 1));  /* no client yet */
    printf("%d", IN_WantMouse(1, 1, 0, 1, 0));  /* in_mouse 0: never */
    printf(" %d%d", IN_MouseButtons(1), IN_MouseButtons(0));  /* buttons only with in_mouse */
    return 0;
}
'''


class InMouse(unittest.TestCase):
    """When the client captures the mouse, as 1.3's IN_Frame (0x464b30):
    with the window focused, unless the console is open in a window, and
    never with in_mouse 0, whose buttons do nothing either (IN_MouseEvent
    0x464940 returns at once without mouseInitialized)."""

    def test_want_mouse(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, 't.c')
            exe = os.path.join(tmp, 't')
            with open(src, 'w') as f:
                f.write(PROGRAM)
            subprocess.run(['gcc', '-I', os.path.join(ROOT, 'src', 'unix'), src, '-o', exe], check=True)
            out = subprocess.run([exe], capture_output=True, text=True, check=True).stdout
        self.assertEqual(out, '11110000 10')


if __name__ == '__main__':
    unittest.main()
