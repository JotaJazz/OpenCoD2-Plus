import os, sys, unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'scripts'))

import check_c_decls as c


class Differs(unittest.TestCase):
    def test_void_called_as_float(self):
        # the caller pops an x87 register the callee never pushed (UI_FillRect)
        self.assertIn('returns void', c.differs(('void', ('float4',), False), ('float4', ('float4',), False)))

    def test_bool_read_as_int(self):
        # al is set, the rest of eax is not
        self.assertIsNotNone(c.differs(('bool1', (), False), ('int4', (), False)))

    def test_int_read_as_narrower_is_fine(self):
        self.assertIsNone(c.differs(('int4', (), False), ('bool1', (), False)))

    def test_ignored_int_result_is_fine(self):
        self.assertIsNone(c.differs(('int4', (), False), ('void', (), False)))

    def test_struct_return(self):
        # the callee expects a hidden pointer and pops it
        self.assertIsNotNone(c.differs(('struct8', (), False), ('int8', (), False)))

    def test_missing_parameter(self):
        # XAnimSetCompleteGoalWeightKnobAll without its rootIndex
        self.assertIn('8 parameters, called with 7', c.differs(
            ('int4', ('ptr', 'int4', 'int4', 'float4', 'float4', 'float4', 'int4', 'int4'), False),
            ('void', ('ptr', 'int4', 'float4', 'float4', 'float4', 'int4', 'int4'), False)))

    def test_float_for_int_parameter(self):
        self.assertIsNotNone(c.differs(('void', ('float4', 'ptr'), False), ('void', ('int4', 'ptr'), False)))

    def test_pointer_for_int_is_fine(self):
        self.assertIsNone(c.differs(('void', ('ptr', 'int4'), False), ('void', ('int4', 'ptr'), False)))

    def test_stub_without_parameters(self):
        self.assertIsNone(c.differs(('int4', (), False), ('void', ('int4', 'int4'), False)))

    def test_empty_declaration_is_no_parameters(self):
        # f() is f(void) in the engine's C++: FX_AddScheduledEffects
        self.assertIsNotNone(c.differs(('void', ('ptr', 'ptr'), False), ('void', (), False)))

    def test_variadic_fixed_part(self):
        self.assertIsNotNone(c.differs(('void', ('ptr', 'float4'), False), ('void', ('ptr', 'int4'), True)))
        self.assertIsNone(c.differs(('void', ('ptr', 'ptr', 'ptr'), False), ('void', ('ptr',), True)))

    def test_extra_arguments(self):
        self.assertIsNone(c.differs(('void', ('ptr',), False), ('void', ('ptr', 'int4'), False)))

    def test_swapped_pointers(self):
        # R_AddCmdDrawFullScreenColoredQuad(..., color, material) declared
        # (..., material, color): the renderer took colorWhite as a material
        self.assertIn('parameter 6 points to struct', c.differs(
            ('void', ('float4',) * 4 + ('ptr:float', 'ptr:struct'), False),
            ('void', ('float4',) * 4 + ('ptr', 'ptr:float'), False)))

    def test_void_pointer_matches_any(self):
        self.assertIsNone(c.differs(('void', ('ptr:struct', 'ptr'), False), ('void', ('ptr', 'ptr:float'), False)))


if __name__ == '__main__':
    unittest.main()
