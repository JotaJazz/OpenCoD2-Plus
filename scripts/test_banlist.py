#!/usr/bin/env python3
"""Tests for banlist.py, the maintainers' tool for shared ban lists:

    python3 -m unittest discover -s scripts -p 'test_*.py'

Needs the openssl CLI.
"""
import base64, json, os, subprocess, sys, tempfile, unittest

HERE = os.path.dirname(os.path.abspath(__file__))
TOOL = os.path.join(HERE, 'banlist.py')
ID = 'P-ABCD-EFGH-IJKL-MNOP'


def read(path, mode='r'):
    with open(path, mode) as f:
        return f.read()


def entry(**extra):
    e = {'id': ID, 'reason': 'aimbot', 'date': '2026-11-01', 'expires': None, 'by': 'admin', 'evidence': 'https://example.com/1'}
    e.update(extra)
    return e


def listing(entries=None, **extra):
    doc = {'format': 1, 'name': 'Test list', 'appeal': 'https://example.com/appeal', 'updated': '2026-11-08T12:00:00Z',
           'entries': [entry()] if entries is None else entries}
    doc.update(extra)
    return doc


class BanlistTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name

    def tearDown(self):
        self.tmp.cleanup()

    def run_tool(self, *args):
        return subprocess.run([sys.executable, TOOL] + list(args), capture_output=True, text=True)

    def write(self, doc, name='list.json'):
        path = os.path.join(self.dir, name)
        with open(path, 'w') as f:
            json.dump(doc, f)
        return path

    def keygen(self):
        out = os.path.join(self.dir, 'maint')
        self.assertEqual(self.run_tool('keygen', out).returncode, 0)
        return out + '.key', read(out + '.pub').strip()

    def test_keygen_writes_compressed_hex(self):
        key, pub = self.keygen()
        self.assertTrue(os.path.exists(key))
        self.assertEqual(len(pub), 66)
        self.assertIn(pub[:2], ('02', '03'))

    def test_sign_verify_round_trip(self):
        key, pub = self.keygen()
        path = self.write(listing())
        self.assertEqual(self.run_tool('sign', path, key).returncode, 0)
        self.assertEqual(len(base64.b64decode(read(path + '.sig'))), 64)
        self.assertEqual(self.run_tool('verify', path, pub).returncode, 0)

    def test_changed_byte_fails_verify(self):
        key, pub = self.keygen()
        path = self.write(listing())
        self.run_tool('sign', path, key)
        data = bytearray(read(path, 'rb'))
        data[10] ^= 1
        with open(path, 'wb') as f:
            f.write(data)
        self.assertEqual(self.run_tool('verify', path, pub).returncode, 1)

    def test_other_key_fails_verify(self):
        key, _ = self.keygen()
        path = self.write(listing())
        self.run_tool('sign', path, key)
        os.rename(os.path.join(self.dir, 'maint.pub'), os.path.join(self.dir, 'old.pub'))
        _, pub2 = self.keygen()
        self.assertEqual(self.run_tool('verify', path, pub2).returncode, 1)

    def test_check_accepts_a_good_list(self):
        self.assertEqual(self.run_tool('check', self.write(listing())).returncode, 0)

    def test_check_accepts_dates_after_2038(self):
        doc = listing([entry(expires='2099-01-01T00:00:00Z'), entry(date='2040-01-01', expires=None)])
        r = self.run_tool('check', self.write(doc))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)

    def test_check_rejects(self):
        bad = {'format 2': listing(format=2),
               'bad id': listing([entry(id='P-abcd-EFGH-IJKL-MNOP')]),
               'guid': listing([entry(guid=1234)]),
               'ip': listing([entry(ip='203.0.113.5')]),
               'bad date': listing([entry(date='2026-02-31')]),
               'bad updated': listing(updated='yesterday'),
               # a typo that would freeze the list: servers refuse anything older than it
               'updated in the future': listing(updated='2099-01-01T00:00:00Z'),
               'expires before date': listing([entry(expires='2026-10-01')]),
               'expires on date': listing([entry(expires='2026-11-01')]),
               'expires 1970': listing([entry(date=None, expires='1970-01-01')]),
               'too many': listing([entry()] * 100001)}
        for what, doc in bad.items():
            with self.subTest(what):
                r = self.run_tool('check', self.write(doc))
                self.assertEqual(r.returncode, 1, r.stdout + r.stderr)


if __name__ == '__main__':
    unittest.main()
