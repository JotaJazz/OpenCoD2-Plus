import os, re, unittest

SRC = os.environ.get('SND_DRIVER_SRC') or os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'src', 'PC', 'win32', 'snd_driver.c')


class ReverbCond(unittest.TestCase):
    """CoD2MP_s.exe 1.3 0x4b7900 (2D), 0x4b7970 (3D), 0x4b79f0 (stream): wet only with no alias,
    or snd_enableReverb on and flag 0x10 clear; anything else gets 0."""

    def level(self, fn, alias, enabled, flags):
        text = open(SRC).read()
        start = text.index('void %s(int index)\n{' % fn)
        body = text[start:text.index('\n}\n', start)]  # only this function
        m = re.search(r'if \((.*?)\) \{\s+reverbLevel = (\S+);\s+\} else \{\s+reverbLevel = (\S+);', body, re.S)
        self.assertIsNotNone(m, fn)
        cond0 = m.group(1)
        cond = ' '.join(cond0.split()).replace('pAlias == NULL', 'alias is None').replace('pAlias != NULL', 'alias is not None')
        cond = re.sub(r'\(\*\(const dvar_t \*\*\)imp_snd_enableReverb\)->current\.enabled|\(\*\(dvar_t \*\*\)imp_snd_enableReverb\)->current\.enabled',
                      'enabled', cond).replace('pAlias->flags', 'flags').replace('&&', ' and ').replace('||', ' or ').replace('!', ' not ')
        cond = cond.replace(' not =', '!=')
        z = lambda v: 0 if v in ('0', '0.0f') else 'wet'
        return z(m.group(2)) if eval(cond) else z(m.group(3))

    def test_truth_table(self):
        for fn in ('SND_Update2DChannelReverb', 'SND_Update3DChannelReverb', 'SND_UpdateStreamChannelReverb'):
            self.assertEqual(self.level(fn, None, 0, 0), 'wet', fn)
            self.assertEqual(self.level(fn, 1, 1, 0), 'wet', fn)
            self.assertEqual(self.level(fn, 1, 1, 0x10), 0, fn)
            self.assertEqual(self.level(fn, 1, 0, 0), 0, fn)
            self.assertEqual(self.level(fn, 1, 0, 0x10), 0, fn)


if __name__ == '__main__':
    unittest.main()
