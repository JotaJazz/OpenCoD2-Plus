import os, subprocess, sys, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT = os.path.join(ROOT, 'scripts', 'check_duplicates.py')


class CheckDuplicates(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name

    def tearDown(self):
        self.tmp.cleanup()

    def compile(self, target_dir, name, code):
        obj_dir = os.path.join(self.dir, 'CMakeFiles', target_dir)
        os.makedirs(obj_dir, exist_ok=True)
        src = os.path.join(self.dir, name + '.c')
        with open(src, 'w') as f:
            f.write(code)
        obj = os.path.join('CMakeFiles', target_dir, name + '.o')
        subprocess.run(['gcc', '-m32', '-c', src, '-o', os.path.join(self.dir, obj)], check=True)
        return obj

    def link(self, *objects):
        os.makedirs(os.path.join(self.dir, 'CMakeFiles', 'ded.dir'), exist_ok=True)
        with open(os.path.join(self.dir, 'CMakeFiles', 'ded.dir', 'link.txt'), 'w') as f:
            f.write('c++ -m32 ' + ' '.join(objects) + ' -o ded\n')
        return subprocess.run([sys.executable, SCRIPT, self.dir, 'ded'], capture_output=True, text=True)

    def test_blob_and_plus_define_the_same_global(self):
        # a blob's placeholder left unguarded wins silently with --allow-multiple-definition
        r = self.link(self.compile('cod2_plus_server.dir', 'game', 'int bg_numItems = 131;\n'),
                      self.compile('cod2_objs.dir', 'literals', 'int bg_numItems = 0;\n'))
        self.assertEqual(r.returncode, 1)
        self.assertIn('bg_numItems', r.stdout)

    def test_distinct_globals_clean(self):
        r = self.link(self.compile('cod2_plus_server.dir', 'game', 'int bg_numItems = 131;\n'),
                      self.compile('cod2_objs.dir', 'literals', 'int faceAxis = 0;\n'))
        self.assertEqual(r.returncode, 0, r.stdout)

    def test_two_plus_objects_not_compared(self):
        r = self.link(self.compile('cod2_plus_server.dir', 'a', 'int x = 1;\n'),
                      self.compile('cod2_plus_script.dir', 'b', 'int y = 1;\n'))
        self.assertEqual(r.returncode, 0, r.stdout)
