import os, subprocess, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PROGRAM = r'''
#include <stdio.h>
#include <string.h>
#include <strings.h>
extern "C" {
#include "PC/qcommon/net_hardening.c"
void Com_Printf(const char *fmt, ...) {}
void Com_DPrintf(const char *fmt, ...) {}
int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
int I_strnicmp(const char *a, const char *b, size_t n) { return strncasecmp(a, b, n); }
const dvar_t *Dvar_FindVar(const char *name)
{
    static dvar_t d;
    static const struct { const char *name; unsigned flags; } vars[] = {
        {"fs_game", 0x101c}, {"sv_pure", 0x100c}, {"sv_serverid", 0x1048},
        {"com_maxfps", 0x1}, {"r_gamma", 0x1}, {"ro_test", 0x40}, {"cheat_test", 0x80},
        {"init_test", 0x10}, {"jump_height", 0x1180}, {"player_moveThreshhold", 0x11c0}, {"cg_fovscale", 0x80},
    };
    for (unsigned i = 0; i < sizeof(vars) / sizeof(vars[0]); i++) {
        if (!strcasecmp(vars[i].name, name)) {
            d.flags = vars[i].flags;
            return &d;
        }
    }
    return 0;
}
}
int main(int argc, char **argv)
{
    /* cvar NAME | name NAME EXT | url TEXT */
    if (!strcmp(argv[1], "cvar"))
        printf("%d", NetHardening_ServerMaySetCvar(argv[2]));
    else if (!strcmp(argv[1], "name"))
        printf("%d", NetHardening_ValidDownloadName(argv[2], argv[3][0] == '1'));
    else
        printf("%d", NetHardening_ValidUrl(argv[2]));
    return 0;
}
'''


class NetHardening(unittest.TestCase):
    """What a client lets a server decide: which dvars it sets, where a
    download lands and which URLs it opens."""

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.exe = os.path.join(cls.tmp.name, 't')
        src = os.path.join(cls.tmp.name, 't.cpp')
        with open(src, 'w') as f:
            f.write(PROGRAM)
        includes = ['-I' + os.path.join(ROOT, 'src'), '-I' + os.path.join(ROOT, 'src', 'headers')]
        subprocess.run(['g++', '-x', 'c++', '-m32', '-w', '-fpermissive', '-DCOD2_FEATURE_NET_HARDENING=1']
                       + includes + [src, '-o', cls.exe], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def ask(self, *args):
        return subprocess.run([self.exe] + list(args), capture_output=True, text=True, check=True).stdout

    def assert_cvars(self, names, expected):
        self.assertEqual({n: self.ask('cvar', n) for n in names}, {n: expected for n in names})

    def test_commands_blocked(self):
        self.assert_cvars(['activeAction', 'ACTIVEACTION', 'nextdemo'], '0')

    def test_protected_blocked(self):
        self.assert_cvars(['fs_basepath', 'FS_BASEPATH', 'rcon_password', 'RCON_PASSWORD', 'cl_allowDownload',
                           'CL_ALLOWDOWNLOAD', 'cl_wwwDownload', 'net_ip', 'in_mouse', ''], '0')

    def test_systeminfo_dvars_pass(self):
        self.assert_cvars(['fs_game', 'FS_GAME', 'sv_pure', 'sv_serverid'], '1')

    def test_mod_dvars_pass(self):
        self.assert_cvars(['r_gamma', 'cg_foo', 'ui_bar', 'scr_baz', 'unknown_new'], '1')

    def test_readonly_blocked(self):
        self.assert_cvars(['ro_test', 'init_test'], '0')

    def test_game_dvars_pass(self):
        # the server sends these for the client's prediction (flags CHEAT and
        # CODINFO); without them the client predicts other physics
        self.assert_cvars(['jump_height', 'player_moveThreshhold', 'cheat_test', 'cg_fovscale'], '1')

    def test_valid_download_names(self):
        for name in ['main/a.iwd', 'mods/x/y.iwd', 'main/a.0012abcd.iwd']:
            self.assertEqual(self.ask('name', name, '1'), '1', name)

    def test_bad_download_names(self):
        for name in ['../a.iwd', '/a.iwd', '\\a.iwd', 'c:/a.iwd', 'a.txt', 'a.dll', 'a/../../b.iwd', '']:
            self.assertEqual(self.ask('name', name, '1'), '0', name)

    def test_urls(self):
        for url in ['http://a', 'https://a']:
            self.assertEqual(self.ask('url', url), '1', url)
        for url in ['file:///x', '\\\\h\\s', 'calc.exe', '']:
            self.assertEqual(self.ask('url', url), '0', url)


if __name__ == '__main__':
    unittest.main()
