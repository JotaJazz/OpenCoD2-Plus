"""cmake/msvc_data_aliases.cmake: the /alternatename pairs that join C and C++ globals on the MSVC ABI."""
import os
import subprocess
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT = os.path.join(ROOT, 'cmake', 'msvc_data_aliases.cmake')


def aliases(undefined, defined):
    with tempfile.TemporaryDirectory() as tmp:
        files = {}
        for name, lines in (('undefined', undefined), ('defined', defined)):
            files[name] = os.path.join(tmp, name + '.txt')
            with open(files[name], 'w') as f:
                f.write('\n'.join(lines) + '\n')
        out = os.path.join(tmp, 'aliases.rsp')
        subprocess.run(['cmake', f'-DUNDEFINED_FILE={files["undefined"]}', f'-DDEFINED_FILE={files["defined"]}',
                        f'-DOUT={out}', '-P', SCRIPT], check=True, capture_output=True)
        with open(out) as f:
            return sorted(line.strip() for line in f if line.strip())


class MsvcDataAliases(unittest.TestCase):
    def test_cxx_reference_to_c_global(self):
        """C++ code's extern for an engine (C) global: the mangled name resolves to the C one."""
        self.assertEqual(aliases(['?sv_paused@@3PAUdvar_s@@A'], ['_sv_paused']),
                         ['/alternatename:?sv_paused@@3PAUdvar_s@@A=_sv_paused'])

    def test_c_reference_to_cxx_global(self):
        """The engine's C reference to a global OpenCoD2-Plus Server defines in C++."""
        self.assertEqual(aliases(['_scr_const'], ['?scr_const@@3Uscr_const_t@@A']),
                         ['/alternatename:_scr_const=?scr_const@@3Uscr_const_t@@A'])

    def test_no_alias_without_a_definition(self):
        """A symbol defined nowhere stays an undefined-symbol error, as it should."""
        self.assertEqual(aliases(['_missing', '?gone@@3HA'], ['_other']), [])

    def test_functions_and_members_are_left_alone(self):
        """Functions cross through the bridge's thunks and class members are not globals."""
        self.assertEqual(aliases(['?Com_Printf@@YAXPBDZZ', '?g@UI_Component@@2PAEA'],
                                 ['_Com_Printf', '_g']), [])

    def test_import_thunks_are_left_alone(self):
        self.assertEqual(aliases(['__imp__CreateFileA@28'], ['?_imp__CreateFileA@28@@3HA']), [])


if __name__ == '__main__':
    unittest.main()
