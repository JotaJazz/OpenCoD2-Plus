"""The engine side's ABI of the functions a bridge module crosses, as test/<module>_abi*.txt keeps it.

One line per function:

    name <class><size> <slot>,<slot>,... [...]

class is v (void), i (integer or pointer, returned in eax), f (floating, in
st0) or s (struct or union, through a hidden pointer); size is the return's
sizeof; each slot is the stack size of a parameter, rounded up to 4 bytes;
'-' means no parameters; '...' marks a variadic function; 'name missing' a
function the engine does not define. test/<module>_abi_x64.txt holds the
values a COD2_X64 build checks (pointers take 8 bytes).

The engine's side is 1.3's, which does not change: the files were recorded
with gdb from builds that still compiled the engine's own server (removed
since) and are kept by hand now. scripts/gen_script_bridge.py turns each line
into static_asserts against OpenCoD2-Plus Server's declarations and refuses a
crossing that has no line, so a new crossing gets its line from the engine's
declaration (and the 1.3 function it reconstructs).
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


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


def abi_path(module, arch='x86'):
    return os.path.join(ROOT, 'test', f'{module}_abi.txt' if arch == 'x86' else f'{module}_abi_{arch}.txt')


def load(module, arch='x86'):
    path = abi_path(module, arch)
    if not os.path.exists(path):
        return {}
    with open(path) as f:
        return parse(f.read())
