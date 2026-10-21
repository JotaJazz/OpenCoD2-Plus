#!/usr/bin/env python3
"""Checks the documentation against the repository.

    python3 scripts/check_docs.py [ROOT]

Reads AGENTS.md, the README files and docs/**/*.md (not docs/superpowers,
which is local) and prints one line per problem, as path:line: message:
a relative link to a file that does not exist; a repository path (src/,
scripts/, test/, docs/, cmake/, mods/, third_party/) anywhere in backticks
that does not exist; personal data (a Windows drive path but the
C:\\path\\to\\ placeholder, /home/<name> or /mnt/<drive>, an IPv4 address
but 127.0.0.1 and 0.0.0.0, a CD key or the word codkey); a stage
of docs/ai/PROJECT_MEMORY.md with more than 3 bullets. Exits with 0 when
there are none.
"""
import os, re, sys

LINK = re.compile(r'\[[^\]]*\]\(([^)\s]+)\)')
CODE = re.compile(r'`([^`\n]+)`')
REPO_DIRS = ('src/', 'scripts/', 'test/', 'docs/', 'cmake/', 'mods/', 'third_party/')
SENSITIVE = [
    (re.compile(r'\b[A-Za-z]:[\\/](?!path[\\/]to[\\/])'), 'a Windows drive path'),
    (re.compile(r'/home/\w|/mnt/[a-z]/'), 'a personal Unix path'),
    (re.compile(r'(?<!version )(?<![\w.])(?!127\.0\.0\.1\b)(?!0\.0\.0\.0\b)\d{1,3}(?:\.\d{1,3}){3}\b'), 'an IP address'),
    (re.compile(r'codkey', re.I), 'a CD key'),
    (re.compile(r'\b[A-Z0-9]{4}(?:-[A-Z0-9]{4}){4}\b'), 'a CD key'),
]
MEMORY = 'docs/ai/PROJECT_MEMORY.md'
MAX_BULLETS = 3


def md_files(root):
    files = [f for f in os.listdir(root) if f == 'AGENTS.md' or (f.startswith('README') and f.endswith('.md'))]
    docs = os.path.join(root, 'docs')
    for dirpath, dirnames, filenames in os.walk(docs):
        rel = os.path.relpath(dirpath, root)
        if rel.split(os.sep)[:2] == ['docs', 'superpowers']:
            dirnames[:] = []
            continue
        files += [os.path.join(rel, f) for f in filenames if f.endswith('.md')]
    return sorted(f.replace(os.sep, '/') for f in files)


def check_file(root, path):
    problems = []
    with open(os.path.join(root, path), encoding='utf-8') as f:
        lines = f.read().split('\n')
    here = os.path.dirname(os.path.join(root, path))
    fenced = False
    bullets = None
    for n, line in enumerate(lines, 1):
        where = '%s:%d: ' % (path, n)
        for pattern, what in SENSITIVE:
            if pattern.search(line):
                problems.append(where + 'personal data (%s)' % what)
        if line.startswith('```'):
            fenced = not fenced
            continue
        if path == MEMORY:
            if line.startswith('## '):
                bullets = 0
            elif bullets is not None and line.startswith('- '):
                bullets += 1
                if bullets == MAX_BULLETS + 1:
                    problems.append(where + 'stage with more than %d bullets' % MAX_BULLETS)
        if fenced:
            continue
        for target in LINK.findall(line):
            if re.match(r'[a-z]+:', target) or target.startswith('#'):
                continue
            target = target.split('#')[0]
            if not os.path.exists(os.path.normpath(os.path.join(here, target))):
                problems.append(where + 'broken link %s' % target)
        for code in CODE.findall(line):
            for token in code.split():
                token = token.strip('\'"').split('=')[-1].strip('\'"').split('#')[0].rstrip('.,:;)')
                if not token.startswith(REPO_DIRS) or any(c in token for c in '*<>{}$|'):
                    continue
                if not os.path.exists(os.path.join(root, token)):
                    problems.append(where + 'no such path %s' % token)
    return problems


def find_problems(root, files):
    problems = []
    for path in files:
        problems += check_file(root, path)
    return problems


def main():
    root = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    problems = find_problems(root, md_files(root))
    for p in problems:
        print(p)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
