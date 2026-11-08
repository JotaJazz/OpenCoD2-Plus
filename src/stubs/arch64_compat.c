/* x86_64 (LP64) compatibility shims. On the GCC/Linux x64 path the data blob is
 * leaner and does not provide these symbols, so define them here. The MSVC x64
 * build links the full reconstructed blob (data32.c/literals32.c) + win32_stubs.c
 * which already define all of them, so this TU stays inactive there (guarded on
 * __x86_64__ only, NOT _M_X64) to avoid LNK2005 duplicates. (x64 port Stage 4.) */
#if defined(__x86_64__)

#    include "common_types.h"

/* weak: the Linux x64 build's generated blob (build/native_gen/data32.c) now defines some of these */
#    define ARCH64_VTABLE(sym) unsigned char sym[256] __attribute__((aligned(16), weak)) = { 0 }
/* Effect vtables are implemented with real function pointers in FxPrimitives.c:
ARCH64_VTABLE(__ZTV6Effect);
ARCH64_VTABLE(__ZTV16OrientedParticle);
ARCH64_VTABLE(__ZTV4Line);
ARCH64_VTABLE(__ZTV4Tail);
ARCH64_VTABLE(__ZTV5Cloud);
ARCH64_VTABLE(__ZTV7Emitter);
ARCH64_VTABLE(__ZTV8Cylinder);
*/
ARCH64_VTABLE(__ZTV12IncludeClass);
#    undef ARCH64_VTABLE

void *__ZTIl = 0;

__attribute__((weak)) void *TheStringPackage = 0;
__attribute__((weak)) void *__ZN12CSoundObject13sReadCallbackE = 0;
__attribute__((weak)) void *__ZN12CSoundObject13sSeekCallbackE = 0;
__attribute__((weak)) void *__ZN12CSoundObject14sCloseCallbackE = 0;
__attribute__((weak)) void *__ZN12CSoundObject13sOpenCallbackE = 0;

#endif
