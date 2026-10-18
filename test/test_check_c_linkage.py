import os, subprocess, sys, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT = os.path.join(ROOT, 'scripts', 'check_c_linkage.py')


class CheckCLinkage(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name

    def tearDown(self):
        self.tmp.cleanup()

    def compile(self, compiler, ext, code):
        src = os.path.join(self.dir, 'foo' + ext)
        with open(src, 'w') as f:
            f.write(code)
        subprocess.run([compiler, '-m32', '-c', src, '-o', os.path.join(self.dir, 'foo.o')], check=True)

    def run_check(self, *extra):
        return subprocess.run([sys.executable, SCRIPT, self.dir] + list(extra), capture_output=True, text=True)

    def test_c_object_clean(self):
        self.compile('gcc', '.c', 'int foo(void){return 0;}\n')
        self.assertEqual(self.run_check().returncode, 0)

    def test_cxx_object_flagged(self):
        self.compile('g++', '.cpp', 'int foo(void){return 0;}\n')
        r = self.run_check()
        self.assertEqual(r.returncode, 1)
        self.assertIn('_Z3foov', r.stdout)

    def test_extern_c_clean(self):
        self.compile('g++', '.cpp', 'extern "C" int foo(void){return 0;}\n')
        self.assertEqual(self.run_check().returncode, 0)

    def test_allowlist(self):
        self.compile('g++', '.cpp', 'int foo(void){return 0;}\n')
        allow = os.path.join(self.dir, 'allow.txt')
        with open(allow, 'w') as f:
            f.write('# a real C++ symbol\n_Z3foov\n')
        self.assertEqual(self.run_check('--allowlist', allow).returncode, 0)

    def test_allowlist_regex(self):
        self.compile('g++', '.cpp', 'int foo(void){return 0;}\n')
        allow = os.path.join(self.dir, 'allow.txt')
        with open(allow, 'w') as f:
            f.write('re:^_Z3fo+v$\n')
        self.assertEqual(self.run_check('--allowlist', allow).returncode, 0)


if __name__ == '__main__':
    unittest.main()
