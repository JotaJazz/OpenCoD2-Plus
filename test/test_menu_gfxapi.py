import os, subprocess, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PROGRAM = r'''
#include <stdio.h>
#include "l_menu_gfxapi.h"
int main(int argc, char **argv)
{
    Script_PatchGraphicsMenu(argv[1]);
    printf("%s", argv[1]);
    return 0;
}
'''
STOCK = ('type ITEM_TYPE_MULTI\ndvar "ui_r_rendererpreference"\n'
         'dvarStrList { "@MENU_AUTO", "none", "@MENU_DIRECTX_7", "dx7", "@MENU_DIRECTX_9", "dx9" }\nrect 0 75')


class MenuGfxApi(unittest.TestCase):
    """The stock graphics menu's rendering method item picks r_gfxApi, and any
    other menu stays as it is."""

    def patched(self, text):
        with tempfile.TemporaryDirectory() as tmp:
            source = os.path.join(tmp, 'p.c')
            binary = os.path.join(tmp, 'p')
            with open(source, 'w') as f:
                f.write(PROGRAM)
            subprocess.run(['gcc', '-I', os.path.join(ROOT, 'src', 'PC', 'botlib'), source, '-o', binary], check=True)
            return subprocess.run([binary, text], capture_output=True, text=True, check=True).stdout

    def test_stock_menu(self):
        out = self.patched(STOCK)
        self.assertEqual(len(out), len(STOCK))
        self.assertIn('dvar "r_gfxApi"', out)
        self.assertIn('{ "Direct3D 9", "d3d9", "Vulkan", "vulkan" }', out)
        self.assertNotIn('rendererpreference', out)
        self.assertNotIn('MENU_DIRECTX', out)
        self.assertTrue(out.endswith('rect 0 75'))

    def test_a_list_too_short_stays(self):
        text = 'dvar "ui_r_rendererpreference"\ndvarStrList { "a", "b" }\n'
        self.assertEqual(self.patched(text), text)

    def test_the_next_items_list_stays(self):
        text = ('dvar "ui_r_rendererpreference"\ndvarFloatList { "a" 0 }\n}\nitemDef\n{\n'
                'dvarStrList { "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb" }\n')
        self.assertEqual(self.patched(text), text)

    def test_another_menu_stays(self):
        text = 'dvar "ui_r_mode"\ndvarStrList { "a", "b" }\n'
        self.assertEqual(self.patched(text), text)


if __name__ == '__main__':
    unittest.main()
