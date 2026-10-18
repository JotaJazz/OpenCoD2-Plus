#!/usr/bin/env python3
"""Lists the C names an executable gets both from CoD2rev-Plus and from the engine.

    python3 scripts/check_duplicates.py <build_dir> <target> [--nm NM]

The objects of cod2_plus_script, cod2_plus_script_client and cod2_plus_server
(CoD2rev-Plus's code and its bridges) define globals by their C names, and the
data blobs must leave those to them. MinGW links with
--allow-multiple-definition, so a blob that still defines one wins without an
error. Reads the objects the target links (CMakeFiles/<target>.dir/objects*.rsp,
or the .o/.obj files in its link.txt) and exits with 0 when no unmangled global
is defined both by a CoD2rev-Plus object and by another one.
"""
import argparse, glob, os, re, subprocess, sys

PLUS_TARGETS = ('cod2_plus_script.dir', 'cod2_plus_script_client.dir', 'cod2_plus_server.dir')
MANGLED = re.compile(r'^_{1,2}Z')
STRONG = set('BDRT')  # not weak (W, V), unique (u) or common (C)


def link_objects(build_dir, target):
    target_dir = os.path.join(build_dir, 'CMakeFiles', target + '.dir')
    files = sorted(glob.glob(os.path.join(target_dir, 'objects*.rsp'))) or [os.path.join(target_dir, 'link.txt')]
    objects = []
    for path in files:
        with open(path) as f:
            objects += [t for t in f.read().split() if t.endswith(('.o', '.obj'))]
    return [o if os.path.isabs(o) else os.path.join(build_dir, o) for o in objects]


def strong_globals(nm, path):
    out = subprocess.run([nm, '-g', '--defined-only', path], capture_output=True, text=True).stdout
    return {p[-1] for p in (line.split() for line in out.splitlines())
            if len(p) >= 3 and p[-2] in STRONG and not MANGLED.match(p[-1])}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('build_dir')
    ap.add_argument('target')
    ap.add_argument('--nm', default='nm')
    args = ap.parse_args()

    plus, engine = {}, {}
    for path in link_objects(args.build_dir, args.target):
        side = plus if any(t in path for t in PLUS_TARGETS) else engine
        for name in strong_globals(args.nm, path):
            side.setdefault(name, path)
    both = sorted(set(plus) & set(engine))
    for name in both:
        print(f'{name}: {os.path.relpath(plus[name], args.build_dir)} and {os.path.relpath(engine[name], args.build_dir)}')
    print(f'{len(both)} names defined twice')
    return 1 if both else 0


if __name__ == '__main__':
    sys.exit(main())
