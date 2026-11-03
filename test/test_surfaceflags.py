import os, re, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SURFACEFLAGS_C = os.path.join(ROOT, 'src', 'PC', 'universal', 'surfaceflags.c')

# 1.3's values (CoD2rev-Plus's src/universal/surfaceflags.cpp and its SURF_/CONTENTS_
# defines): the decompiler wrote these as symbol + offset, the Mac binary's
# addresses that happened to equal them, so the engine got its own functions'
# addresses instead; on x64 they did not compile
EXPECTED = {
    'opaqueglass': (0, 0x900000, 0),
    'playerclip': (1, 0, 0x10000),
    'monsterclip': (1, 0, 0x20000),
    'nonsolid': (1, 0x4000, 0),
    'alphashadow': (0, 0x10000, 0),
    'nocastshadow': (0, 0x40000, 0),
    'nodlight': (0, 0x20000, 0),
}


def info_parms():
    """name -> (clearSolid, surfaceFlags, contents) from the table's rows."""
    text = open(SURFACEFLAGS_C, encoding='latin-1').read()
    strings = dict(re.findall(r'const char (str_\w+)\[\] = "([^"]*)";', text))
    rows = {}
    for sym, solid, surf, contents in re.findall(
            r'\{ \(char \*\)&(str_\w+), (\S+), (.+?), (.+?), \S+ \},', text):
        rows[strings[sym]] = (solid, surf, contents)
    return rows


class InfoParms(unittest.TestCase):
    def test_flags_are_1_3_constants(self):
        rows = info_parms()
        for name, (solid, surf, contents) in EXPECTED.items():
            with self.subTest(name=name):
                self.assertEqual(rows[name], (hex(solid), hex(surf), hex(contents)))

    def test_no_value_is_an_address(self):
        for name, row in info_parms().items():
            with self.subTest(name=name):
                self.assertFalse(any('&' in v for v in row), row)


if __name__ == '__main__':
    unittest.main()
