"""r_jpeg.c pokes libjpeg structs at fixed offsets; check them against jpeg-6 per ABI."""
import os
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
JPEG = os.path.join(ROOT, "src", "PC", "jpeg-6")
SRC = os.path.join(ROOT, "src", "PC", "gfx_d3d", "r_jpeg.c")
MINGW64 = os.path.join(ROOT, "third_party", "mingw64", "usr", "bin", "x86_64-w64-mingw32-gcc-posix")

# r_jpeg.c macro -> (struct, field); None means sizeof
FIELDS = {
    "JPEG_COMPRESS_STRUCT_SIZE": ("c", None),
    "JPEG_C_DEST": ("c", "dest"),
    "JPEG_C_IMAGE_WIDTH": ("c", "image_width"),
    "JPEG_C_IMAGE_HEIGHT": ("c", "image_height"),
    "JPEG_C_INPUT_COMPONENTS": ("c", "input_components"),
    "JPEG_C_IN_COLOR_SPACE": ("c", "in_color_space"),
    "JPEG_C_NEXT_SCANLINE": ("c", "next_scanline"),
    "JPEG_D_STRUCT_SIZE": ("d", None),
    "JPEG_D_OUTPUT_WIDTH": ("d", "output_width"),
    "JPEG_D_OUTPUT_HEIGHT": ("d", "output_height"),
    "JPEG_D_OUT_COLOR_COMPS": ("d", "out_color_components"),
    "JPEG_D_OUTPUT_SCANLINE": ("d", "output_scanline"),
}
STRUCTS = {"c": "struct jpeg_compress_struct", "d": "struct jpeg_decompress_struct"}


def measure(cc, tmp):
    """Each value becomes the size of a common symbol, read back with nm (works for cross compilers)."""
    lines = ["#include <stddef.h>", "#include <stdio.h>", '#include "jinclude.h"', '#include "jpeglib.h"']
    for name, (kind, field) in FIELDS.items():
        expr = f"sizeof({STRUCTS[kind]})" if field is None else f"offsetof({STRUCTS[kind]}, {field})"
        lines.append(f"char v_{name}[{expr}];")
    src = os.path.join(tmp, "probe.c")
    obj = os.path.join(tmp, "probe.o")
    with open(src, "w") as f:
        f.write("\n".join(lines) + "\n")
    subprocess.run(cc + ["-fcommon", "-I", JPEG, "-c", src, "-o", obj], check=True)
    nm = shutil.which("nm")
    out = subprocess.run([nm, "-S", obj], check=True, capture_output=True, text=True).stdout
    return {m.group(2): int(m.group(1), 16) for m in re.finditer(r"^(?:\S+ )?([0-9a-f]+) C _?v_(\w+)$", out, re.M)}


def defines(predefined):
    """Macro values r_jpeg.c sees under the given compiler macros (preprocessor only)."""
    gcc = shutil.which("gcc")
    head = open(SRC).read().split("typedef struct jpeg_memory_destination_s")[0]
    head = "\n".join(l for l in head.splitlines() if not l.startswith("#include"))
    args = [gcc, "-E", "-dM", "-undef", "-x", "c", "-"] + [f"-D{d}" for d in predefined]
    out = subprocess.run(args, input=head, check=True, capture_output=True, text=True).stdout
    vals = dict(re.findall(r"^#define (JPEG_\w+) (\S+)$", out, re.M))
    return {k: int(v, 0) for k, v in vals.items() if k in FIELDS}


ABIS = [
    ("linux-i386", ["gcc", "-m32"], ["__i386__", "__linux__"]),
    ("linux-x64", ["gcc", "-m64"], ["__x86_64__", "__linux__"]),
    ("win32", ["i686-w64-mingw32-gcc"], ["_WIN32"]),
    ("win64", [MINGW64], ["_WIN32", "_WIN64"]),
]


class JpegLayout(unittest.TestCase):
    def test_offsets_match_libjpeg(self):
        for name, cc, predefined in ABIS:
            with self.subTest(abi=name):
                if not (shutil.which(cc[0]) or os.path.exists(cc[0])):
                    self.skipTest(f"{cc[0]} missing")
                with tempfile.TemporaryDirectory() as tmp:
                    try:
                        real = measure(cc, tmp)
                    except subprocess.CalledProcessError:
                        self.skipTest(f"{name} toolchain cannot compile")
                self.assertEqual(defines(predefined), real)


if __name__ == "__main__":
    unittest.main()
