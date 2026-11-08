"""The MSVC-ABI shims (src/win32/shims-msvc, win32_compat.h), built with clang for i686-pc-windows-msvc and run on Windows."""
import os
import shutil
import subprocess
import tempfile
import time
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLANG = '/mnt/c/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang.exe'
VCVARS = r'C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat'


def remove_when_unlocked(path):
    """Windows holds a just-exited exe for a moment: retry, so no folder is left behind."""
    for _ in range(20):
        shutil.rmtree(path, ignore_errors=True)
        if not os.path.exists(path):
            return
        time.sleep(0.5)


def win(path):
    return subprocess.run(['wslpath', '-w', path], check=True, capture_output=True, text=True).stdout.strip()


@unittest.skipUnless(shutil.which('cmd.exe') and os.path.exists(CLANG), 'needs Windows (cmd.exe) and VS clang')
class PthreadMsvc(unittest.TestCase):
    def build_and_run(self, sources, flags=''):
        tmp = tempfile.mkdtemp(dir='/mnt/c/Users/Public')
        self.addCleanup(remove_when_unlocked, tmp)
        exe = os.path.join(tmp, 'selftest.exe')
        driver = win(CLANG.replace('clang.exe', 'clang++.exe' if sources[0].endswith('.cpp') else 'clang.exe'))
        cmd = (f'call "{VCVARS}" >nul && "{driver}" --target=i686-pc-windows-msvc -fuse-ld=lld -O2 {flags} '
               f'-I"{win(os.path.join(ROOT, "src", "win32", "shims-msvc"))}" '
               + ' '.join(f'"{win(os.path.join(ROOT, s))}"' for s in sources) + f' -o "{win(exe)}"')
        bat = os.path.join(tmp, 'build.bat')
        with open(bat, 'w', newline='') as f:
            f.write('@echo off\r\n' + cmd + '\r\n')
        build = subprocess.run(['cmd.exe', '/c', win(bat)], cwd=tmp, capture_output=True, text=True)
        self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
        run = subprocess.run([exe], capture_output=True, text=True, timeout=60)
        self.assertEqual(run.stdout.strip(), 'ok', run.stdout + run.stderr)

    def test_selftest(self):
        self.build_and_run(['test/pthread_msvc_selftest.c', 'src/win32/shims-msvc/pthread_msvc.c'])

    def test_constructor_runs(self):
        """COD2_CONSTRUCTOR's function runs before main, also when the optimizer may drop unused statics."""
        self.build_and_run(['test/msvc_constructor_selftest.cpp'],
                           f'-include "{win(os.path.join(ROOT, "src", "headers", "win32_compat.h"))}"')

if __name__ == '__main__':
    unittest.main()
