import os, re, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA_S = os.path.join(ROOT, 'src', 'blobs', 'data.S')
DATA_WIN_S = os.path.join(ROOT, 'build', 'win_gen', 'data.win.S')
DATA32_C = os.path.join(ROOT, 'build', 'native_gen', 'data32.c')
SHELLSHOCK_C = os.path.join(ROOT, 'src', 'PC', 'cgame_mp', 'cg_shellshock.c')
COMMON_C = os.path.join(ROOT, 'src', 'PC', 'qcommon', 'common.c')

# the 1.3 binary's table at 0x5d89c8, the 29 dvars a .shock file sets
SHOCK_DVARS_13 = [
    'cg_shock_screenBlendTime', 'cg_shock_screenBlendFadeTime', 'cg_shock_viewKickPeriod',
    'cg_shock_viewKickRadius', 'cg_shock_sound', 'cg_shock_soundFadeInTime', 'cg_shock_soundFadeOutTime',
    'cg_shock_soundLoopFadeTime', 'cg_shock_soundLoopEndDelay', 'cg_shock_soundRoomType',
    'cg_shock_soundDryLevel', 'cg_shock_soundWetLevel', 'cg_shock_soundModEndDelay', 'cg_shock_volume_auto',
    'cg_shock_volume_auto2d', 'cg_shock_volume_menu', 'cg_shock_volume_weapon', 'cg_shock_volume_voice',
    'cg_shock_volume_item', 'cg_shock_volume_body', 'cg_shock_volume_local', 'cg_shock_volume_music',
    'cg_shock_volume_announcer', 'cg_shock_volume_shellshock', 'cg_shock_mouse', 'cg_shock_mouse_maxpitchspeed',
    'cg_shock_mouse_maxyawspeed', 'cg_shock_mouse_sensitivityscale', 'cg_shock_mouse_fadeTime',
]


def read(path):
    with open(path, encoding='latin-1') as f:
        return f.read()


def data_s_object(name, path=DATA_S):
    """The lines of one object in a .S blob, from its label to the next .globl."""
    text = read(path)
    start = text.index(f'\n{name}:\n')
    end = text.find('\n.globl ', start)
    return re.sub(r'/\*.*?\*/', '', text[start:end], flags=re.S)


class ShockDvars(unittest.TestCase):
    """A .shock file must set the same dvars as in 1.3: a mod's file without
    cg_shock_viewKickFadeTime (which 1.3 does not read) used to fail to load,
    and the client dropped with "couldn't register shell shock"."""

    def test_c_table(self):
        body = re.search(r'cg_shock_dvar_names\[30\] = \{(.*?)\};', read(SHELLSHOCK_C), re.S).group(1)
        self.assertEqual(re.findall(r'"(\w+)"', body), SHOCK_DVARS_13)

    def test_data_s_table(self):
        strings = dict(re.findall(r'(str_\w+)\[\] = "(\w+)"', read(COMMON_C)))
        refs = re.findall(r'\.long (str_\w+)', data_s_object('cg_shock_dvar_names'))
        self.assertEqual([strings.get(r, r) for r in refs], SHOCK_DVARS_13)


class DownloadLabel(unittest.TestCase):
    """The download box's label is the localized EXE_DOWNLOADING (0x5ddc44);
    the blob had its last four bytes taken for a pointer."""

    def test_dl_text(self):
        # data.S (Linux), its PE copy (MinGW)
        for path, label in ((DATA_S, 'dlText'), (DATA_WIN_S, '_dlText')):
            body = data_s_object(label, path)
            self.assertNotIn('.long', body, path)
            data = bytes(int(b, 16) for b in re.findall(r'0x([0-9a-f]{2})', body))
            self.assertEqual(data.split(b'\0')[0], b'EXE_DOWNLOADING', path)

    def test_dl_text_native(self):
        # the native blob keeps the 4 bytes as a pointer member: "ING\0"
        body = re.search(r'\bdlText = \{(.*?)\n\};', read(DATA32_C), re.S).group(1)
        self.assertIn('(void *)0x00474e49', body)


if __name__ == '__main__':
    unittest.main()
