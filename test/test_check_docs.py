import os, sys, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
import check_docs


class CheckDocs(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name

    def tearDown(self):
        self.tmp.cleanup()

    def write(self, path, text):
        full = os.path.join(self.dir, path)
        os.makedirs(os.path.dirname(full), exist_ok=True)
        with open(full, 'w') as f:
            f.write(text)
        return path

    def problems(self, path, text):
        return check_docs.find_problems(self.dir, [self.write(path, text)])

    def test_broken_link(self):
        p = self.problems('docs/a.md', 'see [x](missing.md)\n')
        self.assertEqual(len(p), 1)
        self.assertIn('missing.md', p[0])
        self.assertEqual(self.problems('docs/a.md', '[x](a.md#sec) [y](https://e.com)\n'), [])

    def test_missing_repo_path(self):
        self.write('scripts/real.py', '')
        self.assertEqual(len(self.problems('docs/a.md', 'run `scripts/nope.py`\n')), 1)
        self.assertEqual(self.problems('docs/a.md', 'run `scripts/real.py` and `cod2_linux`\n'), [])
        self.assertEqual(self.problems('docs/a.md', '`test/smoke_expected_failures_*.txt` `src/<module>`\n'), [])

    def test_paths_inside_commands(self):
        self.write('scripts/real.py', '')
        self.write('CMakeLists.txt', '')
        self.assertEqual(len(self.problems('docs/a.md', '`python3 scripts/nope.py --x`\n')), 1)
        self.assertEqual(len(self.problems('docs/a.md', '`-DCMAKE_TOOLCHAIN_FILE=cmake/nope.cmake`\n')), 1)
        self.assertEqual(len(self.problems('docs/a.md', "`git am --include='src/nope/x.c'`\n")), 1)
        self.assertEqual(self.problems('docs/a.md', '`python3 scripts/real.py` `CMakeLists.txt` `docs/a.md#sec`\n'), [])

    def test_personal_unix_paths_and_keys(self):
        p = self.problems('docs/a.md', '/home/someone/x\n/mnt/e/games\nE:/games\nABCD-EF12-GH34-IJ56-KL78\n')
        self.assertEqual(len(p), 4)
        self.assertEqual(self.problems('docs/a.md', 'version 1.3.0.1 and /home/<user> and ~/.cod2/preferences\n'), [])

    def test_sensitive(self):
        p = self.problems('docs/a.md', 'Z:\\some\\dir\n203.0.113.7\nthe codkey\n')
        self.assertEqual(len(p), 3)
        self.assertEqual(self.problems('docs/a.md', '127.0.0.1 and 0.0.0.0\n'), [])
        self.assertEqual(self.problems('docs/a.md', 'fs_basepath "C:\\path\\to\\CoD2"\n'), [])

    def test_memory_bullets(self):
        stage = '# Memory\n\n## 2026-10-21 — x\n\n' + ''.join('- b%d\n  - sub\n' % i for i in range(4))
        self.assertEqual(len(self.problems('docs/ai/PROJECT_MEMORY.md', stage)), 1)
        stage3 = '# Memory\n\n## 2026-10-21 — x\n\n- a\n- b\n- c\n'
        self.assertEqual(self.problems('docs/ai/PROJECT_MEMORY.md', stage3), [])

    def test_md_files_skip_superpowers(self):
        self.write('docs/superpowers/plans/p.md', '[x](nope.md)\n')
        self.write('docs/a.md', 'ok\n')
        self.write('AGENTS.md', 'ok\n')
        files = check_docs.md_files(self.dir)
        self.assertIn('docs/a.md', files)
        self.assertIn('AGENTS.md', files)
        self.assertNotIn('docs/superpowers/plans/p.md', files)


if __name__ == '__main__':
    unittest.main()
