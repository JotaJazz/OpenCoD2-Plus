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

    def test_scalar_return_guard(self):
        code = g.export_thunk('Scr_GetInt', ['unsigned int'])
        self.assertIn('static_assert(cod2_bridge_returns_scalar<decltype(Scr_GetInt(std::declval<unsigned int>()))>::value', code)

    def test_struct_return_allowed(self):
        code = g.export_thunk('Scr_GetEntityRef', ['unsigned long'])
        self.assertNotIn('static_assert', code)

    def test_get_anim_custom(self):
        self.assertIn('unsigned int cod2x_Scr_GetAnim(unsigned int index, int tree)', g.EXPORT_CUSTOM['Scr_GetAnim'])



import engine_abi


class Abi(unittest.TestCase):
    def test_parse(self):
        abi = engine_abi.parse('# c\nMSG_WriteByte v0 4,4\nCom_Printf v0 4 ...\nSV_Gone missing\nNET_Port i2 -\n')
        self.assertEqual(abi['MSG_WriteByte'], ('v', 0, [4, 4], False))
        self.assertEqual(abi['Com_Printf'], ('v', 0, [4], True))
        self.assertIsNone(abi['SV_Gone'])
        self.assertEqual(abi['NET_Port'], ('i', 2, [], False))

    def test_import_asserts(self):
        code = g.import_thunk('MSG_WriteByte', ['msg_t*', 'int'], abi=('v', 0, [4, 4], False))
        self.assertIn("cod2_abi_class<decltype(MSG_WriteByte(std::declval<msg_t*>(), std::declval<int>()))>() == 'v'", code)
        self.assertIn('static_assert(cod2_abi_slot<int>() == 4, "MSG_WriteByte: parameter 1 differs from the engine");', code)

    def test_parameter_count(self):
        code = g.import_thunk('Cmd_Argv', ['int'], abi=('i', 4, [4, 4], False))
        self.assertIn('"Cmd_Argv: 1 parameters, the engine has 2"', code)

    def test_widened_return(self):
        code = g.import_thunk('SV_Loaded', [], abi=('i', 4, [], False))
        self.assertIn('extern "C" cod2_abi_int<decltype(SV_Loaded()), 4> cod2c_SV_Loaded()', code)
        self.assertIn('return (decltype(SV_Loaded()))cod2c_SV_Loaded();', code)

    def test_export_widened(self):
        code = g.export_thunk('SV_IsLocalClient', ['int'], abi=('i', 4, [4], False))
        self.assertIn('extern "C" cod2_abi_int<decltype(SV_IsLocalClient(std::declval<int>())), 4> cod2x_SV_IsLocalClient(int a0)', code)


class CustomImports(unittest.TestCase):
    def test_com_sprintf_formats_into_dest(self):
        code = g.import_thunk('Com_sprintf', ['char*', 'unsigned int', 'char const*', '...'])
        self.assertIn('vsnprintf(dest, size, format, ap);', code)
        self.assertNotIn('char buf[4096]', code)

    def test_com_error_never_returns(self):
        code = g.import_thunk('Com_Error', ['errorParm_t', 'char const*', '...'])
        self.assertIn('cod2c_Com_Error(ERR_DROP, "%s", buf);', code)
        self.assertIn('__builtin_unreachable();', code)

    def test_server_read_bits_compress_is_bounded(self):
        code = g.import_thunk('MSG_ReadBitsCompress', ['unsigned char const*', 'int', 'unsigned char*', 'int'],
                              cfg=g.MODULES['server'])
        self.assertIn('if ( count > toSizeBytes )', code)


class Modules(unittest.TestCase):
    def test_server_module_includes_game(self):
        # the game links with the server, so the server's bridge covers it
        objects = g.MODULES['server']['objects']
        self.assertTrue(any(o.endswith('src/game/*.o') for o in objects), objects)


if __name__ == '__main__':
    unittest.main()
