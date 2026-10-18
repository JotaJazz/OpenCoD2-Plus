import os, subprocess, sys, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT = os.path.join(ROOT, 'scripts', 'check_single_impl.py')


class CheckSingleImpl(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name
        self.interface = os.path.join(self.dir, 'interface.txt')
        with open(self.interface, 'w') as f:
            f.write('# exports\nSys_Milliseconds\n')

    def tearDown(self):
        self.tmp.cleanup()

    def link(self, code):
        obj_dir = os.path.join(self.dir, 'CMakeFiles', 'cod2_objs.dir')
        os.makedirs(obj_dir, exist_ok=True)
        src = os.path.join(self.dir, 'engine.c')
        with open(src, 'w') as f:
            f.write(code)
        subprocess.run(['gcc', '-m32', '-c', src, '-o', os.path.join(obj_dir, 'engine.o')], check=True)
        os.makedirs(os.path.join(self.dir, 'CMakeFiles', 'ded.dir'), exist_ok=True)
        with open(os.path.join(self.dir, 'CMakeFiles', 'ded.dir', 'link.txt'), 'w') as f:
            f.write('c++ CMakeFiles/cod2_objs.dir/engine.o -o ded\n')
        return subprocess.run([sys.executable, SCRIPT, self.dir, 'ded', '--interface', self.interface],
                              capture_output=True, text=True)

    def test_engine_keeps_an_exported_function(self):
        # two clocks: the engine's Sys_Milliseconds and CoD2rev-Plus's
        r = self.link('int Sys_Milliseconds(void) { return 0; }\n')
        self.assertEqual(r.returncode, 1)
        self.assertIn('Sys_Milliseconds', r.stdout)

    def test_engine_without_it_clean(self):
        r = self.link('int Sys_Other(void) { return 0; }\n')
        self.assertEqual(r.returncode, 0, r.stdout)
