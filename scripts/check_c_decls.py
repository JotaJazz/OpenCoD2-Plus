#!/usr/bin/env python3
"""Lists the engine functions called with another type than they are defined.

    python3 scripts/check_c_decls.py <build_dir> [--readelf READELF] [--allowlist FILE] [-v]
    (MinGW: --readelf i686-w64-mingw32-objdump)

The engine's sources have C linkage and declare what they call with their own
`extern` lines, so nothing checks a declaration against the definition in
another file. On i386 a wrong return type breaks the call: a function that
returns nothing but is declared float leaves the caller popping an empty x87
stack, a float declared int leaves one on it, a bool (al) read as an int takes
the rest of eax. This reads the DWARF of the build's objects (they are built
with -g): the declarations a compiler records are those of the functions an
object calls. For each function defined with C linkage it compares the return
type and the parameters' sizes and kinds with every declaration of it in
another object, and lists the differences. Exits with 0 when every difference
is in the allowlist (one name per line, # comments; default
test/c_decls_allowlist.txt), and lists the allowlist's names that no longer
differ, so the list only shrinks.
"""
import argparse, json, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CXX_TARGETS = ('cod2_plus_script.dir', 'cod2_plus_script_game.dir', 'cod2_plus_script_core.dir',
                'cod2_plus_server.dir', 'cod2_plus_server_client.dir')
DIE = re.compile(r'^\s*<(\d+)><([0-9a-f]+)>: Abbrev Number: \d+ \((DW_TAG_\w+)\)')
ATTR = re.compile(r'^\s*<[0-9a-f]+>\s+(DW_AT_\w+)\s*:\s*(.*)$')


def objects(build_dir):
    """The engine's objects of compile_commands.json (stale ones from another
    configuration are left out), without CoD2rev-Plus's C++ targets."""
    with open(os.path.join(build_dir, 'compile_commands.json')) as f:
        entries = json.load(f)
    for e in entries:
        m = re.search(r'-o (\S+)', e.get('command', ''))
        # import_pointers and surfaceflags.c only take addresses
        if not m or any(t in m.group(1) for t in CXX_TARGETS) or 'import_pointers' in m.group(1) \
                or 'surfaceflags' in m.group(1):
            continue
        path = os.path.join(e['directory'], m.group(1))
        if os.path.exists(path):
            yield path


def parse(readelf, path):
    """[(name, is_declaration, signature)] of the object's C functions."""
    # readelf for ELF; objdump prints the same DWARF dump for MinGW's COFF
    cmd = [readelf, '--dwarf=info', '-W', path] if 'objdump' in os.path.basename(readelf) \
        else [readelf, '--debug-dump=info', '-W', path]
    run = subprocess.run(cmd, capture_output=True, text=True, errors='replace')
    if run.returncode != 0:
        raise RuntimeError(f'{readelf} failed on {path}: {run.stderr.strip()[:200]}')
    out = run.stdout
    dies = {}
    order = []
    cur = None
    for line in out.splitlines():
        m = DIE.match(line)
        if m:
            depth, off, tag = int(m.group(1)), int(m.group(2), 16), m.group(3)
            cur = {'tag': tag, 'depth': depth, 'attrs': {}, 'children': []}
            dies[off] = cur
            order.append((off, depth))
            continue
        m = ATTR.match(line)
        if m and cur is not None:
            cur['attrs'][m.group(1)] = m.group(2).strip()
    # parents
    stack = []
    for off, depth in order:
        while stack and stack[-1][1] >= depth:
            stack.pop()
        if stack:
            dies[stack[-1][0]]['children'].append(off)
        stack.append((off, depth))

    def ref(value):
        m = re.search(r'<0x([0-9a-f]+)>', value)
        return int(m.group(1), 16) if m else None

    def kind(off, seen=0):
        """'void', 'float4', 'int4', 'bool1', 'ptr', 'struct<size>'..."""
        if off is None or seen > 20:
            return 'void'
        d = dies.get(off)
        if d is None:
            return '?'
        a = d['attrs']
        tag = d['tag']
        if tag in ('DW_TAG_typedef', 'DW_TAG_const_type', 'DW_TAG_volatile_type', 'DW_TAG_restrict_type'):
            return kind(ref(a.get('DW_AT_type', '')), seen + 1)
        if tag in ('DW_TAG_pointer_type', 'DW_TAG_reference_type', 'DW_TAG_rvalue_reference_type',
                   'DW_TAG_array_type', 'DW_TAG_subroutine_type'):
            return 'ptr'
        # readelf prints the larger sizes in hex
        size = re.search(r'(0x[0-9a-f]+|\d+)\s*$', a.get('DW_AT_byte_size', ''))
        size = str(int(size.group(1), 0)) if size else '?'
        if tag == 'DW_TAG_base_type':
            enc = a.get('DW_AT_encoding', '')
            if 'float' in enc:
                return 'float' + size
            if 'boolean' in enc:
                return 'bool' + size
            return 'int' + size
        if tag == 'DW_TAG_enumeration_type':
            return 'int' + size
        if tag in ('DW_TAG_structure_type', 'DW_TAG_union_type', 'DW_TAG_class_type'):
            return 'struct' + size
        return tag

    funcs = []
    for off, d in dies.items():
        if d['tag'] != 'DW_TAG_subprogram':
            continue
        a = d['attrs']
        if 'DW_AT_linkage_name' in a or 'DW_AT_specification' in a or 'DW_AT_abstract_origin' in a:
            continue
        name = a.get('DW_AT_name', '')
        name = re.sub(r'^\([^)]*\)\s*', '', name.split('):')[-1].strip())
        if not name or 'DW_AT_external' not in a:
            continue
        ret = kind(ref(a.get('DW_AT_type', ''))) if 'DW_AT_type' in a else 'void'
        params = []
        variadic = False
        for c in d['children']:
            cd = dies[c]
            if cd['tag'] == 'DW_TAG_formal_parameter':
                params.append(kind(ref(cd['attrs'].get('DW_AT_type', ''))))
            elif cd['tag'] == 'DW_TAG_unspecified_parameters':
                variadic = True
        funcs.append((name, 'DW_AT_declaration' in a, (ret, tuple(params), variadic)))
    return funcs


def ret_class(k):
    """Where i386 returns a value of kind k."""
    if k == 'void':
        return 'void'
    if k.startswith('float'):
        return 'x87'
    if k.startswith('struct'):
        return 'sret'
    if k == 'ptr':
        return 'eax4'
    m = re.match(r'(int|bool)(\d+)', k)
    if m:
        return 'edx:eax' if m.group(2) == '8' else 'eax' + m.group(2)
    return k


def param_class(k):
    """How a parameter of kind k sits on the stack."""
    if k == 'ptr':
        return 'int4'
    m = re.match(r'(int|bool)(\d+)', k)
    if m:
        return 'int8' if m.group(2) == '8' else 'int4'
    return k


def differs(defn, decl):
    """Why a call through decl breaks with the definition, or None."""
    dret, dparams, dvar = defn
    cret, cparams, cvar = decl
    d, c = ret_class(dret), ret_class(cret)
    if d != c:
        harmless = (c == 'void' and d.startswith('eax')) or \
                   (d.startswith('eax') and c.startswith('eax') and int(d[3:]) >= int(c[3:]))
        if not harmless:
            return f'returns {dret}, called as {cret}'
    if not dparams and not dvar:
        return None  # a stub without parameters: the arguments go unread
    dp = [param_class(k) for k in dparams]
    cp = [param_class(k) for k in cparams]
    if dvar or cvar:
        # variadic: the fixed parameters still have to agree
        n = min(len(dp), len(cp))
        if dp[:n] != cp[:n]:
            return 'the fixed parameters differ'
        return None  # a K&R import declared (...) passes the call's arguments as they are
    if len(dp) != len(cp):
        if dp[:min(len(dp), len(cp))] == cp[:min(len(dp), len(cp))] and len(cp) > len(dp):
            return None  # extra arguments the callee never reads (cdecl)
        return f'{len(dp)} parameters, called with {len(cp)}'
    for n, (a, b) in enumerate(zip(dp, cp)):
        if a != b:
            return f'parameter {n + 1} is {dparams[n]}, called as {cparams[n]}'
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('build_dir')
    ap.add_argument('--readelf', default='readelf')
    ap.add_argument('--allowlist', default=os.path.join(ROOT, 'test', 'c_decls_allowlist.txt'))
    ap.add_argument('-v', action='store_true', help='every declaration that differs, with its object')
    args = ap.parse_args()

    objs = list(objects(args.build_dir))
    if not objs:
        print('no objects in compile_commands.json')
        return 1
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        results = list(pool.map(lambda o: (o, parse(args.readelf, o)), objs))
    if not any(funcs for obj, funcs in results):
        print('no DWARF in the objects (built without -g?)')
        return 1
    defs = {}
    decls = {}
    for obj, funcs in results:
        for name, is_decl, sig in funcs:
            (decls.setdefault(name, []) if is_decl else defs.setdefault(name, [])).append((obj, sig))
    bad = {}
    for name, where in defs.items():
        defn = where[0][1]
        for obj, sig in decls.get(name, ()):
            why = differs(defn, sig)
            if why:
                bad.setdefault(name, []).append((why, os.path.relpath(obj, args.build_dir)))
    allowed = set()
    if os.path.exists(args.allowlist):
        with open(args.allowlist) as f:
            allowed = {l.split('#')[0].strip() for l in f} - {''}
    new = sorted(n for n in bad if n not in allowed)
    gone = sorted(allowed - set(bad))
    for name in sorted(bad) if args.v else new:
        for why, obj in bad[name][:3 if not args.v else None]:
            print(f'{name}: {why} ({obj})')
    for name in gone:
        print(f'{name}: listed in the allowlist but no longer differs')
    print(f'{len(new)} functions called with another type, {len(gone)} stale allowlist entries '
          f'({len(objs)} objects)')
    return 1 if new or gone else 0


if __name__ == '__main__':
    sys.exit(main())
