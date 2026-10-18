import os, sys, tempfile, unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'scripts'))

import opencod2_smoke


class ParseResults(unittest.TestCase):
    def test_parse_ok_and_fail(self):
        text = 'ok   getinfo reply\nFAIL rcon reply (timeout)\n     protocol 118\n'
        self.assertEqual(opencod2_smoke.parse_results(text), {'getinfo reply': True, 'rcon reply': False})


class Compare(unittest.TestCase):
    def test_new_failure(self):
        self.assertEqual(opencod2_smoke.compare({'a': False, 'b': True}, set()), ({'a'}, set()))

    def test_fixed_failure(self):
        self.assertEqual(opencod2_smoke.compare({'a': True}, {'a'}), (set(), {'a'}))

    def test_expected_failure_still_failing(self):
        self.assertEqual(opencod2_smoke.compare({'a': False}, {'a'}), (set(), set()))

    def test_missing_check_counts_as_fixed(self):
        self.assertEqual(opencod2_smoke.compare({'b': True}, {'a'}), (set(), {'a'}))


class LoadExpected(unittest.TestCase):
    def test_comments_ignored(self):
        with tempfile.NamedTemporaryFile('w', suffix='.txt', delete=False) as f:
            f.write('# comentario\n\nrcon reply\n  crash report written  \n')
        try:
            self.assertEqual(opencod2_smoke.load_expected(f.name), {'rcon reply', 'crash report written'})
        finally:
            os.unlink(f.name)


if __name__ == '__main__':
    unittest.main()
