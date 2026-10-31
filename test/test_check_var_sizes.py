import json, os, subprocess, sys, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def build(tmp, definition, declaration):
    """A program with a global defined in one file and declared in another,
    and the compile_commands.json the script reads."""
    sources = {'def.c': definition, 'use.c': declaration + '\nint main(void) { return (int)x[0]; }\n'}
    commands = []
    for name, text in sources.items():
        path = os.path.join(tmp, name)
        with open(path, 'w') as f:
            f.write(text)
        output = path[:-2] + '.o'
        subprocess.run(['gcc', '-m32', '-g', '-c', path, '-o', output], check=True)
        commands.append({'directory': tmp, 'file': path, 'output': output,
                         'command': f'gcc -c {path} -o {output}'})
    with open(os.path.join(tmp, 'compile_commands.json'), 'w') as f:
        json.dump(commands, f)
    subprocess.run(['gcc', '-m32', os.path.join(tmp, 'def.o'), os.path.join(tmp, 'use.o'),
                    '-o', os.path.join(tmp, 'prog')], check=True)


def check(tmp):
    return subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'check_var_sizes.py'), tmp,
                           '--binary', 'prog'], capture_output=True, text=True)


class ArrayGlobals(unittest.TestCase):
    """A global declared as an array larger than its definition: the code
    reads and writes past it, as a scalar or a struct declared larger does."""

    def test_array_too_small(self):
        with tempfile.TemporaryDirectory() as tmp:
            build(tmp, 'char x[10] = {1};\n', 'extern int x[10];')
            run = check(tmp)
        self.assertEqual(run.returncode, 1, run.stdout)
        self.assertIn('x: defined with 10 bytes, declared with 40', run.stdout)

    def test_two_dimensions(self):
        with tempfile.TemporaryDirectory() as tmp:
            build(tmp, 'char x[10] = {1};\n', 'extern char x[4][8];')
            run = check(tmp)
        self.assertEqual(run.returncode, 1, run.stdout)
        self.assertIn('declared with 32', run.stdout)

    def test_array_ok(self):
        with tempfile.TemporaryDirectory() as tmp:
            build(tmp, 'char x[40] = {1};\n', 'extern int x[10];')
            run = check(tmp)
        self.assertEqual(run.returncode, 0, run.stdout)

    def test_unknown_bound_ok(self):
        with tempfile.TemporaryDirectory() as tmp:
            build(tmp, 'char x[4] = {1};\nint y = 1;\n', 'extern int x[];\nextern int y;\nint *p = &y;')
            run = check(tmp)
        self.assertEqual(run.returncode, 0, run.stdout)


if __name__ == '__main__':
    unittest.main()
