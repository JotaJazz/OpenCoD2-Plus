import os, re, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BGAME = os.path.join(ROOT, 'src', 'PC', 'bgame')


class WalkNormal(unittest.TestCase):
    """The engine's bgame (the client's prediction) compares walkable normals
    with MIN_WALK_NORMAL, the float 0.7 of 1.3 (0x5c427c) and of CoD2rev-Plus's
    server, not with the double 0.69999999: a normal of exactly 0.7f would
    predict a forced crouch or a failed mantle the server allows."""

    def test_no_double_walk_normal(self):
        hits = []
        for name in sorted(os.listdir(BGAME)):
            if not name.endswith('.c'):
                continue
            with open(os.path.join(BGAME, name), encoding='latin-1') as f:
                for n, line in enumerate(f, 1):
                    if re.search(r'normal\[2\]\s*[<>]=?\s*0\.69999999\b', line):
                        hits.append(f'{name}:{n}: {line.strip()}')
        self.assertEqual(hits, [])


if __name__ == '__main__':
    unittest.main()
