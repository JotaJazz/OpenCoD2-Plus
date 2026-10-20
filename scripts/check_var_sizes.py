#!/usr/bin/env python3
"""Lists the engine's globals that are used as larger than they are defined.

    python3 scripts/check_var_sizes.py <build_dir> [--binary NAME] [--readelf READELF] [-v]

Much of the engine's data is defined in src/blobs and src/stubs as byte
arrays the size of the original binary's, and declared where it is used with
its type. When the type is larger, the code reads and writes past the array,
over whatever the linker put next: the 1.0 client's cls was 162 KB short of a
clientStatic_t, and its debug lists lay over the renderer's entity lighting.
This reads the DWARF of the build's objects (built with -g) for every
declaration of a global with its type's size, and the binary's symbol table
(nm) for the size each global has, and lists the globals some object declares
larger. Exits with 0 when there are none.
"""
import argparse, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
from check_c_decls import ATTR, DIE, objects


def declared(readelf, path):
    """[(name, size)] of the globals the object declares with a sized type."""
    cmd = [readelf, '--dwarf=info', '-W', path] if 'objdump' in os.path.basename(readelf) \
        else [readelf, '--debug-dump=info', '-W', path]
    run = subprocess.run(cmd, capture_output=True, text=True, errors='replace')
    if run.returncode != 0:
        raise RuntimeError(f'{readelf} failed on {path}: {run.stderr.strip()[:200]}')
    dies = {}
    cur = None
    for line in run.stdout.splitlines():
        m = DIE.match(line)
        if m:
            cur = {'tag': m.group(3), 'depth': int(m.group(1)), 'attrs': {}}
            dies[int(m.group(2), 16)] = cur
            continue
        m = ATTR.match(line)
        if m and cur is not None:
            cur['attrs'][m.group(1)] = m.group(2).strip()

    def ref(value):
        m = re.search(r'<0x([0-9a-f]+)>', value)
        return int(m.group(1), 16) if m else None

    def size(off, seen=0):
        d = dies.get(off)
        if d is None or seen > 30:
            return None
        a = d['attrs']
        if 'DW_AT_byte_size' in a:
            # readelf prints the larger ones in hex
            m = re.search(r'(0x[0-9a-f]+|\d+)\s*$', a['DW_AT_byte_size'])
            return int(m.group(1), 0) if m else None
        if d['tag'] in ('DW_TAG_typedef', 'DW_TAG_const_type', 'DW_TAG_volatile_type') and 'DW_AT_type' in a:
            return size(ref(a['DW_AT_type']), seen + 1)
        return None  # arrays of unknown bound, functions

    found = []
    for d in dies.values():
        a = d['attrs']
        if d['tag'] != 'DW_TAG_variable' or d['depth'] != 1 or 'DW_AT_declaration' not in a:
            continue
        name = re.sub(r'^\([^)]*\)\s*', '', a.get('DW_AT_name', '').split('):')[-1].strip())
        n = size(ref(a.get('DW_AT_type', '')))
        if name and n:
            found.append((name, n))
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('build_dir')
    ap.add_argument('--binary', default='cod2_linux', help='the linked binary whose symbols give the sizes')
    ap.add_argument('--readelf', default='readelf')
    ap.add_argument('--nm', default='nm')
    ap.add_argument('-v', action='store_true', help='every object that declares one larger')
    args = ap.parse_args()

    binary = os.path.join(args.build_dir, args.binary)
    run = subprocess.run([args.nm, '-S', binary], capture_output=True, text=True)
    if run.returncode != 0:
        print(f'{args.nm} failed on {binary}: {run.stderr.strip()[:200]}')
        return 1
    defined = {}
    for line in run.stdout.splitlines():
        f = line.split()
        if len(f) == 4 and f[2] in 'bBdDrRgGsS':
            defined[f[3]] = int(f[1], 16)
    objs = list(objects(args.build_dir))
    if not objs:
        print('no objects in compile_commands.json')
        return 1
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        results = list(pool.map(lambda o: (o, declared(args.readelf, o)), objs))
    if not any(found for obj, found in results):
        print('no DWARF in the objects (built without -g?)')
        return 1
    bad = {}
    for obj, found in results:
        for name, n in found:
            if name in defined and n > defined[name]:
                bad.setdefault(name, []).append((n, os.path.relpath(obj, args.build_dir)))
    for name in sorted(bad):
        for n, obj in bad[name][:None if args.v else 1]:
            print(f'{name}: defined with {defined[name]} bytes, declared with {n} ({obj})')
    print(f'{len(bad)} globals declared larger than they are defined ({len(objs)} objects)')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
