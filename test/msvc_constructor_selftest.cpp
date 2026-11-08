// win32_compat.h's COD2_CONSTRUCTOR on the MSVC ABI, as the engine's C compiled as C++ uses it.
#include <stdio.h>
extern "C" {
static int ran;
COD2_CONSTRUCTOR(set_ran) { ran = 1; }
}
int main(void) { puts(ran ? "ok" : "FAIL: constructor did not run"); return !ran; }
