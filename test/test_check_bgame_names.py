import json, os, subprocess, sys, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT = os.path.join(ROOT, 'scripts', 'check_bgame_names.py')
sys.path.insert(0, os.path.join(ROOT, 'scripts'))

import check_bgame_names


class Scan(unittest.TestCase):
    def test_comments_and_literals_name_nothing(self):
        text = '/* friction */\n// friction\nDvar_RegisterFloat("friction", 5.5f);\nchar c = \'f\';\n'
        self.assertEqual(check_bgame_names.named(text, ['friction']), set())

    def test_identifiers_only_whole(self):
        text = 'x = friction->current.value + g_friction + friction2;\n'
        self.assertEqual(check_bgame_names.named(text, ['friction', 'pmoveHandlers']), {'friction'})

    def test_plain_uses_follow_line_markers(self):
        out = ('# 1 "a.c"\nint x;\n# 1 "h.h" 1\nextern int eng_friction;\n# 3 "a.c" 2\n'
               'int y = friction;\nconst char *s = "friction";\n')
        self.assertEqual(check_bgame_names.plain_uses(out, ['friction']), [('a.c', 3, 'friction')])

    def test_preprocess_args_write_no_object(self):
        entry = {'command': 'cc -DX -MD -MF a.d -o a.o -c /src/a.c', 'directory': '/b'}
        self.assertEqual(check_bgame_names.preprocess_args(entry), ['cc', '-DX', '/src/a.c', '-E'])


class CheckBgameNames(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name
        with open(os.path.join(self.dir, 'names.h'), 'w') as f:
            f.write('#define friction eng_friction\n')
        with open(os.path.join(self.dir, 'names.txt'), 'w') as f:
            f.write('# renamed\nfriction\n')
        with open(os.path.join(self.dir, 'CMakeCache.txt'), 'w') as f:
            f.write(f'CMAKE_HOME_DIRECTORY:INTERNAL={self.dir}\n')

    def tearDown(self):
        self.tmp.cleanup()

    def run_on(self, code):
        src = os.path.join(self.dir, 'engine.c')
        with open(src, 'w') as f:
            f.write(code)
        with open(os.path.join(self.dir, 'compile_commands.json'), 'w') as f:
            json.dump([{'directory': self.dir, 'file': src,
                        'command': f'cc -I{self.dir} -o CMakeFiles/cod2_objs.dir/engine.c.o -c {src}'}], f)
        return subprocess.run([sys.executable, SCRIPT, self.dir, '--names', os.path.join(self.dir, 'names.txt')],
                              capture_output=True, text=True)

    def test_source_without_the_header(self):
        # it would link to CoD2rev-Plus's friction
        r = self.run_on('extern float friction;\nfloat f(void) { return friction; }\n')
        self.assertEqual(r.returncode, 1, r.stdout)
        self.assertIn('engine.c:2: friction', r.stdout)

    def test_source_with_the_header(self):
        r = self.run_on('#include "names.h"\nextern float friction;\nfloat f(void) { return friction; }\n')
        self.assertEqual(r.returncode, 0, r.stdout)

    def test_header_included_too_late(self):
        r = self.run_on('extern float friction;\n#include "names.h"\nfloat f(void) { return friction; }\n')
        self.assertEqual(r.returncode, 1, r.stdout)
        self.assertIn('engine.c:1: friction', r.stdout)


if __name__ == '__main__':
    unittest.main()
