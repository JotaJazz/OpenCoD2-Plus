#!/usr/bin/env python3
"""Records the ABI of the engine functions a bridge module crosses.

    python3 scripts/engine_abi.py --module server [--binary build/check-linux/cod2_linux] [--gdb gdb]

For each import and export of the module (scripts/gen_script_bridge.py
--names), reads with gdb the debug information of a binary built with the
engine's own side (src/PC/server_mp for the server: a client configured with
-DCOD2_PLUS_CLIENT_SERVER=OFF, since the bridge's C++ thunks of a client with
CoD2rev-Plus's server shadow the engine's functions) and writes
test/<module>_abi.txt, one line per function:

    name <class><size> <slot>,<slot>,... [...]

class is v (void), i (integer or pointer, returned in eax), f (floating, in
st0) or s (struct or union, through a hidden pointer); size is the return's
sizeof; each slot is the stack size of a parameter, rounded up to 4 bytes;
'...' marks a variadic function. A function the binary does not define is
written as 'name missing'. gen_script_bridge.py turns each line into
static_asserts against CoD2rev-Plus's declarations.
"""
import argparse, os, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))

# Functions the binary defines but whose debug information gdb does not read
# back (it reports '<unknown return type> (void)'), from their definitions.
BY_HAND = {
    'AngleNormalize360': 'f4 4',                # src/PC/universal/com_math.c
    'Com_AbortDObj': 'v0 -',                    # src/PC/qcommon/dobj_management.c
    'Hunk_ClearLow': 'v0 4',                    # Hunk_ClearToMarkLow, src/PC/universal/com_memory.c
    'MatrixTransformVector': 'v0 4,4,4',        # src/PC/universal/com_math.c
}

GDB_SCRIPT = r'''
import gdb
def cls(t):
    t = t.strip_typedefs()
    if t.code == gdb.TYPE_CODE_VOID:
        return 'v0'
    if t.code == gdb.TYPE_CODE_FLT:
        return 'f%d' % t.sizeof
    if t.code in (gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION):
        return 's%d' % t.sizeof
    return 'i%d' % t.sizeof
out = open(OUT, 'w')
for name in open(NAMES).read().split():
    sym = gdb.lookup_global_symbol(name) or gdb.lookup_static_symbol(name)
    if sym is None or sym.type.strip_typedefs().code != gdb.TYPE_CODE_FUNC:
        out.write('%s missing\n' % name)
        continue
    t = sym.type.strip_typedefs()
    slots = ','.join(str((f.type.strip_typedefs().sizeof + 3) // 4 * 4) for f in t.fields())
    variadic = ' ...' if str(t).endswith('...)') else ''
    out.write('%s %s %s%s\n' % (name, cls(t.target()), slots or '-', variadic))
out.close()
'''


def parse(text):
    """{name: (class, size, [slots], variadic)} from the lines of an ABI file; missing ones are None."""
    abi = {}
    for line in text.splitlines():
        line = line.split('#')[0].strip()
        if not line:
            continue
        parts = line.split()
        if parts[1] == 'missing':
            abi[parts[0]] = None
            continue
        slots = [] if parts[2] == '-' else [int(x) for x in parts[2].split(',')]
        abi[parts[0]] = (parts[1][0], int(parts[1][1:]), slots, len(parts) > 3 and parts[3] == '...')
    return abi


def load(module):
    path = os.path.join(ROOT, 'test', f'{module}_abi.txt')
    if not os.path.exists(path):
        return {}
    with open(path) as f:
        return parse(f.read())


def main():
    import gen_script_bridge as bridge
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--module', required=True, choices=list(bridge.MODULES))
    ap.add_argument('--build', default=os.path.join(ROOT, 'build', 'check-linux'))
    ap.add_argument('--binary', help='default: cod2_linux of the build')
    ap.add_argument('--gdb', default='gdb')
    args = ap.parse_args()
    binary = args.binary or os.path.join(args.build, 'cod2_linux')

    builds = [args.build]
    mingw = os.path.join(ROOT, 'build', 'check-mingw')
    if os.path.isdir(os.path.join(mingw, 'CMakeFiles')):
        builds.append(mingw)
    imports, exports = bridge.crossing_names(args.module, builds)
    names = sorted(set(imports) | set(exports))
    path = os.path.join(ROOT, 'test', f'{args.module}_abi.txt')
    with tempfile.TemporaryDirectory(prefix='engine_abi_') as tmp:
        names_file = os.path.join(tmp, 'names')
        out_file = os.path.join(tmp, 'abi')
        script = os.path.join(tmp, 'abi.py')
        with open(names_file, 'w') as f:
            f.write('\n'.join(names) + '\n')
        with open(script, 'w') as f:
            f.write(f'NAMES = {names_file!r}\nOUT = {out_file!r}\n' + GDB_SCRIPT)
        run = subprocess.run([args.gdb, '-batch', '-nx', '-ex', f'source {script}', binary],
                             capture_output=True, text=True)
        if not os.path.exists(out_file):
            sys.exit('gdb wrote nothing:\n' + run.stderr[-2000:])
        lines = ''.join(f'{l.split()[0]} {BY_HAND[l.split()[0]]}\n' if l.endswith(' missing') and l.split()[0] in BY_HAND
                        else l + '\n' for l in open(out_file).read().splitlines())
    with open(path, 'w') as f:
        f.write(f'# Written by scripts/engine_abi.py --module {args.module} from {os.path.relpath(binary, ROOT)}; '
                'see its docstring for the format.\n' + lines)
    abi = parse(lines)
    missing = sorted(n for n, v in abi.items() if v is None)
    print(f'{len(abi)} functions, {len(missing)} missing' + (': ' + ' '.join(missing) if missing else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
