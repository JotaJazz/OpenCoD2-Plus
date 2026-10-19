import os, sys, tempfile, unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'scripts'))

import bgame_names


class BgameNames(unittest.TestCase):
    def test_header_renames_each_name(self):
        header = bgame_names.header(['player_breath_hold_time', 'pmoveHandlers'])
        self.assertIn('#define player_breath_hold_time eng_player_breath_hold_time', header)
        self.assertIn('#define pmoveHandlers eng_pmoveHandlers', header)

    def test_placeholders_define_the_new_names(self):
        # the dedicated server leaves the engine's bgame out, but the imp_
        # pointers still take these addresses
        code = bgame_names.placeholders(['pmoveHandlers'])
        self.assertIn('char eng_pmoveHandlers[4];', code)
        self.assertIn('extern "C"', code)

    def test_rename_asm_keeps_other_labels(self):
        text = '.globl _pmoveHandlers\n_pmoveHandlers:\n    .long _pmoveHandlers2\n    .long _PM_Foo\n'
        out = bgame_names.rename_asm(text, ['pmoveHandlers'])
        self.assertEqual(out, '.globl _eng_pmoveHandlers\n_eng_pmoveHandlers:\n    .long _pmoveHandlers2\n    .long _PM_Foo\n')

    def test_names_file(self):
        with tempfile.NamedTemporaryFile('w', suffix='.txt', delete=False) as f:
            f.write('# comment\nplayer_turnAnims\n\nfriction\n')
        self.assertEqual(bgame_names.load(f.name), ['player_turnAnims', 'friction'])
        os.unlink(f.name)


if __name__ == '__main__':
    unittest.main()
