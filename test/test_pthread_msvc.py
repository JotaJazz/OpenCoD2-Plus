"""The MSVC-ABI pthread shim (src/win32/shims-msvc), built with clang for i686-pc-windows-msvc and run on Windows."""
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLANG = '/mnt/c/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang.exe'
VCVARS = r'C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat'


def win(path):
    return subprocess.run(['wslpath', '-w', path], check=True, capture_output=True, text=True).stdout.strip()


@unittest.skipUnless(shutil.which('cmd.exe') and os.path.exists(CLANG), 'needs Windows (cmd.exe) and VS clang')
class PthreadMsvc(unittest.TestCase):
    def test_selftest(self):
        with tempfile.TemporaryDirectory(dir='/mnt/c/Users/Public') as tmp:
            exe = os.path.join(tmp, 'pthread_selftest.exe')
            sources = [win(os.path.join(ROOT, 'test', 'pthread_msvc_selftest.c')),
                       win(os.path.join(ROOT, 'src', 'win32', 'shims-msvc', 'pthread_msvc.c'))]
            clang = win(CLANG)
            cmd = (f'call "{VCVARS}" >nul && "{clang}" --target=i686-pc-windows-msvc -fuse-ld=lld -O1 '
                   f'-I"{win(os.path.join(ROOT, "src", "win32", "shims-msvc"))}" '
                   + ' '.join(f'"{s}"' for s in sources) + f' -o "{win(exe)}"')
            bat = os.path.join(tmp, 'build.bat')
            with open(bat, 'w', newline='') as f:
                f.write('@echo off\r\n' + cmd + '\r\n')
            build = subprocess.run(['cmd.exe', '/c', win(bat)], cwd=tmp, capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            run = subprocess.run([exe], capture_output=True, text=True, timeout=60)
            self.assertEqual(run.stdout.strip(), 'ok', run.stdout + run.stderr)


if __name__ == '__main__':
    unittest.main()
