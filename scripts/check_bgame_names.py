#!/usr/bin/env python3
"""Lists the engine sources that name a renamed bgame global under its plain name.

    python3 scripts/check_bgame_names.py <build_dir> [--names FILE]

In the client the engine's bgame globals that CoD2rev-Plus's code also defines
are renamed to eng_<name> by src/headers/engine_bgame_names.h, which
common_types.h includes (scripts/bgame_names.py). An engine source that names
one of them without that header in effect would link to CoD2rev-Plus's global
without an error. For each translation unit in <build_dir>/compile_commands.json
that is not CoD2rev-Plus's (the cod2_plus_* targets and test/vendored_files.txt)
and whose text names one of test/engine_bgame_names.txt, this runs the
preprocessor and looks for the plain name left in its output. Exits with 0
when there is none.
"""
import argparse, json, os, re, shlex, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_duplicates import PLUS_TARGETS

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# comments, then string and character literals (whose text is no identifier)
LEXEMES = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)
LINE_MARKER = re.compile(r'^# (\d+) "((?:\\.|[^"\\])*)"')


def load(path):
    with open(path) as f:
        return [l.strip() for l in f if l.strip() and not l.startswith('#')]


def strip_lexemes(text):
    """text without comments and literals; newlines kept, so lines still count."""
    return LEXEMES.sub(lambda m: '\n' * m.group(0).count('\n') if m.group(0).startswith('/') else '""', text)


def named(text, names):
    """The names text uses as identifiers (not in comments or literals)."""
    words = set(re.findall(r'[A-Za-z_]\w*', strip_lexemes(text)))
    return words & set(names)


def plain_uses(preprocessed, names):
    """(file, line, name) for each plain name left in preprocessor output."""
    pattern = re.compile(r'(?<![\w$])(' + '|'.join(map(re.escape, sorted(names))) + r')(?![\w$])')
    found = []
    path, line = '?', 0
    for raw in preprocessed.splitlines():
        marker = LINE_MARKER.match(raw)
        if marker:
            path, line = marker.group(2), int(marker.group(1))
            continue
        if not raw.startswith('#'):
            for m in pattern.finditer(strip_lexemes(raw)):
                found.append((path, line, m.group(1)))
        line += 1
    return found


def source_text(path):
    """path's text and that of the files it includes by a quoted name next to
    it or by an absolute one (as the C++ wrappers under cxx_wrap/ include
    their C source), one level deep."""
    with open(path, errors='replace') as f:
        text = f.read()
    parts = [text]
    for inc in re.findall(r'^\s*#\s*include\s*"([^"]+)"', text, re.M):
        inc = os.path.join(os.path.dirname(path), inc)
        if os.path.isfile(inc):
            with open(inc, errors='replace') as f:
                parts.append(f.read())
    return '\n'.join(parts)


def preprocess_args(entry):
    """The compile command of entry, made to write its preprocessed output."""
    args = entry['arguments'] if 'arguments' in entry else shlex.split(entry['command'])
    out, skip = [], False
    for a in args:
        if skip:
            skip = False
        elif a in ('-o', '-MF', '-MT', '-MQ'):
            skip = True
        elif a not in ('-c', '-MD', '-MMD'):
            out.append(a)
    return out + ['-E']


def source_root(build_dir):
    with open(os.path.join(build_dir, 'CMakeCache.txt')) as f:
        for line in f:
            if line.startswith('CMAKE_HOME_DIRECTORY:'):
                return line.split('=', 1)[1].strip()
    return ROOT


def engine_entries(build_dir):
    with open(os.path.join(build_dir, 'compile_commands.json')) as f:
        entries = json.load(f)
    src_root = source_root(build_dir)
    vendored = set(load(os.path.join(ROOT, 'test', 'vendored_files.txt')))
    seen = set()
    for e in entries:
        path = os.path.normpath(os.path.join(e['directory'], e['file']))
        output = e.get('output') or ' '.join(preprocess_args(e))
        if any(t in output for t in PLUS_TARGETS) or os.path.relpath(path, src_root) in vendored or path in seen:
            continue
        seen.add(path)
        yield path, e


def check(path, entry, names):
    run = subprocess.run(preprocess_args(entry), cwd=entry['directory'], capture_output=True, text=True,
                         errors='replace')
    if run.returncode != 0:
        return [(path, 0, 'preprocessor failed: ' + (run.stderr.strip().splitlines() or ['?'])[0])]
    return plain_uses(run.stdout, names)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('build_dir')
    ap.add_argument('--names', default=os.path.join(ROOT, 'test', 'engine_bgame_names.txt'))
    args = ap.parse_args()

    names = load(args.names)
    todo, missing = [], 0
    for path, entry in engine_entries(args.build_dir):
        if not os.path.exists(path):
            missing += 1  # generated by the build, not yet there
            continue
        if named(source_text(path), names):
            todo.append((path, entry))
    with ThreadPoolExecutor(os.cpu_count() or 4) as pool:
        results = list(pool.map(lambda t: check(t[0], t[1], names), todo))
    found = sorted({u for r in results for u in r})
    for path, line, what in found:
        print(f'{path}:{line}: {what}')
    note = f', {missing} not generated yet' if missing else ''
    print(f'{len(found)} plain names in {len(todo)} engine sources that name one{note}')
    return 1 if found else 0


if __name__ == '__main__':
    sys.exit(main())
