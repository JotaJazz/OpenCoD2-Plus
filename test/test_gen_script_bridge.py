import os, sys, unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'scripts'))

import gen_script_bridge as g


class SplitParams(unittest.TestCase):
    def test_function_pointer(self):
        self.assertEqual(g.split_params('char const*, void* (*)(int)'), ['char const*', 'void* (*)(int)'])

    def test_variadic(self):
        self.assertEqual(g.split_params('char const*, ...'), ['char const*', '...'])

    def test_empty(self):
        self.assertEqual(g.split_params(''), [])


class Thunks(unittest.TestCase):
    def test_plain(self):
        code = g.export_thunk('Scr_GetInt', ['unsigned int'])
        self.assertIn('extern "C" decltype(Scr_GetInt(std::declval<unsigned int>())) '
                      'cod2x_Scr_GetInt(unsigned int a0) __asm__(COD2_SYM("Scr_GetInt"));', code)
        self.assertIn('{ return Scr_GetInt(a0); }', code)

    def test_function_pointer_param(self):
        code = g.export_thunk('XAnimPrecache', ['char const*', 'void* (*)(int)'])
        self.assertIn('cod2x_XAnimPrecache(char const* a0, void* (*a1)(int))', code)

    def test_variadic(self):
        code = g.import_thunk('Com_Printf', ['char const*', '...'])
        self.assertIn('cod2c_Com_Printf(char const* a0, ...) __asm__(COD2_SYM("Com_Printf"));', code)
        self.assertIn('vsnprintf(buf, sizeof(buf), a0, ap);', code)
        self.assertIn('return cod2c_Com_Printf("%s", buf);', code)

    def test_overload_choice(self):
        symbols = {'AddRefToValue': ['AddRefToValue(int, VariableUnion)', 'AddRefToValue(VariableValue*)']}
        self.assertEqual(g.choose_overload('AddRefToValue', symbols['AddRefToValue']), ['int', 'VariableUnion'])

    def test_extra_engine_argument(self):
        code = g.import_thunk('FS_ListFiles', ['char const*', 'char const*', 'FsListBehavior', 'int*'])
        self.assertIn('return cod2c_FS_ListFiles(a0, a1, a2, a3, 10);', code)
        self.assertIn('cod2c_FS_ListFiles(char const* a0, char const* a1, FsListBehavior a2, int* a3, int a4)', code)

    def test_renamed_engine_symbol(self):
        code = g.import_thunk('Hunk_ClearTempMemoryHighInternal', [])
        self.assertIn('__asm__(COD2_SYM("Hunk_ClearTempMemoryHigh"))', code)

    def test_provided_by_hand(self):
        self.assertIn('Q_rint', g.PROVIDED)

    def test_custom_export(self):
        self.assertIn('void cod2x_Scr_FreeScripts(int sys)', g.EXPORT_CUSTOM['Scr_FreeScripts'])
        self.assertIn('Scr_ShutdownSystem(bComplete);', g.EXPORT_CUSTOM['Scr_ShutdownSystem'])


if __name__ == '__main__':
    unittest.main()
