import os, subprocess, sys, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT = os.path.join(ROOT, 'scripts', 'layout_parity.py')


class LayoutParity(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name

    def tearDown(self):
        self.tmp.cleanup()

    def write(self, name, text):
        path = os.path.join(self.dir, name)
        with open(path, 'w') as f:
            f.write(text)
        return path

    def run_parity(self, a, b, types):
        t = self.write('types.txt', types)
        return subprocess.run([sys.executable, SCRIPT, '--headers-a', a, '--headers-b', b, '--types', t,
                               '--flags=-m32'], capture_output=True, text=True)

    def test_identical(self):
        a = self.write('a.h', 'struct S { int x; char c; int y; };\n')
        b = self.write('b.h', 'struct S { int x; char c; int y; };\n')
        r = self.run_parity(a, b, 'S:x,c,y\n')
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertIn('ok   S', r.stdout)

    def test_moved_field(self):
        a = self.write('a.h', 'struct S { int x; int y; };\n')
        b = self.write('b.h', 'struct S { int x; char pad[4]; int y; };\n')
        r = self.run_parity(a, b, 'S:x,y\n')
        self.assertEqual(r.returncode, 1)
        self.assertIn('DIFF S: y 4 != 8', r.stdout)

    def test_size_only(self):
        a = self.write('a.h', 'struct S { int x; };\n')
        b = self.write('b.h', 'struct S { int x; int y; };\n')
        r = self.run_parity(a, b, '# comment\nS\n')
        self.assertEqual(r.returncode, 1)
        self.assertIn('DIFF S: sizeof 4 != 8', r.stdout)

    def test_fields_only(self):
        a = self.write('a.h', 'struct S { int x; int y; };\n')
        b = self.write('b.h', 'struct S { int x; int y; int tail[8]; };\n')
        r = self.run_parity(a, b, 'S~:x,y\n')
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)


if __name__ == '__main__':
    unittest.main()
