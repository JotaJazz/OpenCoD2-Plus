import os, subprocess, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

CASES = [
    ('connect 94.199.215.103:10405; password abc', 'connect **.***.***.***:*****; password abc'),
    ('^789.39.161.172:28970', '^7**.**.***.***:*****'),       # a color code is not a digit of it
    ('^1Chile ^7(186.53.71.177)', '^1Chile ^7(***.**.**.***)'),
    ('connect 94.199.', 'connect **.***.'),                    # typed: hidden from the second dot
    ('connect 94.199', 'connect 94.199'),
    ('CoD2 MP 1.3 build', 'CoD2 MP 1.3 build'),
    ('sensitivity 2.5', 'sensitivity 2.5'),
    ('1234.5.6.7', '1234.*.*.*'),                              # a group of 4 digits is not one, the rest is
    ('Connecting...94.199.215.103:28960', 'Connecting...**.***.***.***:*****'),
    ('.199.215.103:28960', '.***.***.***:*****'),              # an edit field scrolled into one
    ('5^71.2.3.4', '5^7*.*.*.*'),                              # a color code ends the number before it
    ('a 10.0.0.1 b 192.168.1.20:28960', 'a **.*.*.* b ***.***.*.**:*****'),
    ('ping 1.2.3:80', 'ping *.*.*:**'),
    ('', ''),
    ('^', '^'),
]

PROGRAM = r'''
#include <stdio.h>
#include "r_hide_address.h"
int main(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; i++) {
        R_HideAddresses(argv[i]);
        printf("%s\n", argv[i]);
    }
    return 0;
}
'''


class HideAddresses(unittest.TestCase):
    """cl_hideServerIp's mask (R_HideAddresses), applied to every text the
    renderer draws: an address's digits become '*' and the length stays."""

    def test_cases(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, 't.c')
            exe = os.path.join(tmp, 't')
            with open(src, 'w') as f:
                f.write(PROGRAM)
            subprocess.run(['gcc', '-Wall', '-Werror', '-I', os.path.join(ROOT, 'src', 'PC', 'gfx_d3d'), src, '-o', exe],
                           check=True)
            out = subprocess.run([exe] + [c[0] for c in CASES], capture_output=True, text=True, check=True).stdout
        self.assertEqual(out.split('\n')[:-1], [c[1] for c in CASES])


if __name__ == '__main__':
    unittest.main()
