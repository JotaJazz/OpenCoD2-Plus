import os, subprocess, sys, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT = os.path.join(ROOT, 'scripts', 'check_bridge_symbols.py')

BRIDGE = '''extern "C" int cod2c_Code(int a0) __asm__(COD2_SYM("Code"));
extern "C" int cod2c_Data(int a0) __asm__(COD2_SYM("Data"));
extern "C" int cod2c_Absent(int a0) __asm__(COD2_SYM("Absent"));
'''


class CheckBridgeSymbols(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name
        with open(os.path.join(self.dir, 'bridge.cpp'), 'w') as f:
            f.write(BRIDGE)

    def tearDown(self):
        self.tmp.cleanup()

    def build(self, source):
        src = os.path.join(self.dir, 'p.c')
        exe = os.path.join(self.dir, 'p')
        with open(src, 'w') as f:
            f.write(source)
        subprocess.run(['gcc', '-o', exe, src], check=True)
        return exe

    def run_check(self, exe):
        return subprocess.run([sys.executable, SCRIPT, exe, os.path.join(self.dir, 'bridge.cpp')],
                              capture_output=True, text=True)

    def test_code_passes(self):
        exe = self.build('int Code(int x) { return x; }\nint Data(int x) { return x; }\nint main(void) { return 0; }\n')
        r = self.run_check(exe)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)

    def test_data_fails(self):
        exe = self.build('int Code(int x) { return x; }\nint (*Data)(int) = 0;\nint main(void) { return 0; }\n')
        r = self.run_check(exe)
        self.assertEqual(r.returncode, 1)
        self.assertIn('Data: ', r.stdout)
        self.assertNotIn('Code', r.stdout)
        self.assertNotIn('Absent', r.stdout)


if __name__ == '__main__':
    unittest.main()
