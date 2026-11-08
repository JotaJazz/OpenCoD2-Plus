# =============================================================================
# unified CMake build for all targets.
#
#   Native Linux (default; the client on DXVK needs scripts/build_linux_vk_deps.sh,
#   -DCOD2_LINUX_DXVK=OFF builds it on the GL device instead):
#     cmake -S . -B build/native && cmake --build build/native -j
#       -> build/native/cod2_linux   build/native/cod2_lnxded
#
#   Web (WebAssembly, requires the Emscripten SDK):
#     emcmake cmake -S . -B build/web && cmake --build build/web -j
#       -> build/web/cod2.html (+ cod2.js / cod2.wasm / cod2_fs.js)
#
#   Windows (MinGW cross-compile):
#     cmake -S . -B build/win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
#     cmake --build build/win32 -j
#       -> build/win32/cod2_win32_ded.exe          (headless, default)
#     add -DCOD2_WIN32_CLIENT=ON for the SDL2/GL client (needs static SDL2 libs).
#
# compile_commands.json is emitted for IDE/clangd use.
# =============================================================================
cmake_minimum_required(VERSION 3.16)

# x86_64 (LP64) target. OFF (default) = the original ILP32/x86 reconstruction
# (4-byte pointers, the committed 32-bit data blobs). ON is an IN-PROGRESS port:
# it suppresses the -m32 seed and the 32-bit-only asserts/lib paths so an x64
# toolchain can configure and COMPILE the source set (to measure the x64 error
# surface). It does NOT link/run yet — the typed-data migration (Stage 2) is the
# prerequisite. Additive: every 32-bit path below is preserved as the default.
option(COD2_X64 "Target x86_64/LP64 instead of the original ILP32/x86 (port in progress)" OFF)

# -m32 must be in effect when the native compiler is probed (multilib link
# tests). Seed it ONLY for the plain native 32-bit build — the MinGW (i686) and
# Emscripten toolchains are 32-bit/wasm already and must not see -m32, and the
# x64 port wants the native 64-bit ABI.
if(NOT CMAKE_TOOLCHAIN_FILE AND NOT COD2_X64)
  set(CMAKE_C_FLAGS_INIT   "-m32")
  set(CMAKE_CXX_FLAGS_INIT "-m32")
  set(CMAKE_ASM_FLAGS_INIT "-m32")
endif()

project(cod2 LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)   # gnu++17: the C sources rely on GNU extensions

# git short hash for the crash-reporter build stamp.
execute_process(COMMAND git -C ${CMAKE_SOURCE_DIR} rev-parse --short HEAD
                OUTPUT_VARIABLE COD2_GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)
if(NOT COD2_GIT_HASH)
  set(COD2_GIT_HASH "unknown")
endif()

# Common warning-demotions used on every toolchain (reconstructed C trips these).
set(COD2_WNO
  -Wno-error=incompatible-pointer-types -Wno-error=implicit-function-declaration
  -Wno-error=implicit-int -Wno-error=int-conversion)
# GCC 14 made a return's type mismatch an error and named it; GCC 13 (Ubuntu's
# MinGW) only warns and rejects -Wno-error= of a warning it does not know.
include(CheckCCompilerFlag)
check_c_compiler_flag(-Wreturn-mismatch COD2_HAS_WRETURN_MISMATCH)
if(COD2_HAS_WRETURN_MISMATCH)
  list(APPEND COD2_WNO -Wno-error=return-mismatch)
endif()

set(COD2_SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# C++ conversion. A C source picked here is compiled as C++ through a generated
# wrapper, extern "C" { #include "x.c" }, so everything it defines or declares
# (block-scope externs included) keeps its C name for the data blobs, the .S
# files and the --defsym aliases. The data blobs and the third-party libraries
# stay C.
set(COD2_CXX_DIRS "" CACHE STRING "With COD2_CXX_ALL=OFF: src/ subdirectories or files still compiled as C++, e.g. PC/qcommon;PC/snd.c")
# Linux and MinGW compile the engine as C++ (Phase 0 of the unification plan);
# OFF goes back to C, where COD2_CXX_DIRS can still pick single modules.
option(COD2_CXX_ALL "Compile every engine C source as C++" ON)
set(COD2_CXX_NEVER "^(blobs|PC/speex|PC/jpeg-6|PC/zlib|win32/sdl2)/")

# OpenCoD2-Plus Server's script VM, server, game, bgame and collision in every
# build (the engine's own were removed, ADR 0015); the sources and the data
# blobs test these macros. The client's local games run on them too, and cgame
# keeps the engine's bgame, whose globals have their eng_ names
# (scripts/bgame_names.py). PROTOCOL_VERSION follows the game version.
add_compile_definitions(COD2_PLUS_SCRIPT=1)   # the data blobs leave the VM globals to src/script
set(COD2_PLUS_CLIENT_SERVER ON)
if(WIN32 AND NOT COD2_WIN32_CLIENT)
  set(COD2_PLUS_CLIENT_SERVER OFF)   # a MinGW configure builds either the client or the dedicated server
endif()
if(COD2_PLUS_CLIENT_SERVER)
  add_compile_definitions(COD2_PLUS_CLIENT_SERVER=1)  # cgame no longer shares the bgame with a local server
endif()
# CoD2rev-Plus's libcod (src/libcod): its script functions, bots,
# sv_master1..5, HTTP, SQLite and the rest, with the modules CoD2rev-Plus's
# Makefile turns on (all but MySQL), in the dedicated server and, when it runs
# CoD2rev-Plus's server (COD2_PLUS_CLIENT_SERVER), in the client's local
# games. Their LIBCOD blocks in the server, game, bgame, core and script VM
# are compiled in with it.
option(COD2_PLUS_LIBCOD "CoD2rev-Plus's libcod in the dedicated server and the client's local games" ON)
if((WIN32 AND COD2_X64) OR EMSCRIPTEN)
  # libcod keeps pointers in long (gsc_memory.cpp), 4 bytes on Win64; the web
  # build has none of its threads, sockets and SQLite
  set(COD2_PLUS_LIBCOD OFF)
endif()
# the client too: then the script VM, which both share, has libcod's blocks;
# otherwise the dedicated server builds its own (cod2_plus_script_ded)
set(COD2_LIBCOD_CLIENT OFF)
if(COD2_PLUS_LIBCOD AND COD2_PLUS_CLIENT_SERVER)
  set(COD2_LIBCOD_CLIENT ON)
endif()
set(COD2_LIBCOD_DEFS "")
if(COD2_PLUS_LIBCOD)
  set(COD2_LIBCOD_DEFS LIBCOD LIBCOD_COMPILE_BOTS=1 LIBCOD_COMPILE_ENTITY=1 LIBCOD_COMPILE_EXEC=1
    LIBCOD_COMPILE_GRAPH=1 LIBCOD_COMPILE_HTTP=1 LIBCOD_COMPILE_JSON=1 LIBCOD_COMPILE_LEVEL=1
    LIBCOD_COMPILE_MEMORY=1 LIBCOD_COMPILE_PLAYER=1 LIBCOD_COMPILE_RATELIMITER=1 LIBCOD_COMPILE_SQLITE=1
    LIBCOD_COMPILE_SPEEX=1 LIBCOD_COMPILE_UTILS=1 LIBCOD_COMPILE_WEAPONS=1)
endif()
if(COD2_FEATURE_CFLAGS MATCHES "COD2_PATCH_10")
  set(COD2_PROTOCOL 115)
else()
  set(COD2_PROTOCOL 118)
endif()
# 1.3 Plus or stock 1.3: with it (OpenCoD2-Plus Server's make PLUS=1) the
# servers take only the Plus client and the client announces itself as one;
# without it everything speaks stock 1.3, and every other improvement stays
option(COD2_PLUS_PROTOCOL "1.3 builds speak CoD2 1.3 Plus (OFF: stock 1.3)" ON)
if(COD2_PROTOCOL EQUAL 118 AND COD2_PLUS_PROTOCOL)
  add_compile_definitions(COD2_PLUS)
endif()
if(COD2_FEATURE_CFLAGS AND EMSCRIPTEN)
  message(FATAL_ERROR "COD2_FEATURE_CFLAGS is not applied on the web build")
endif()

# cod2_plus_script(): the OBJECT library with CoD2rev-Plus's script sources and
# the generated bridge in src/compat (scripts/gen_script_bridge.py), and the
# bridge's calls into src/PC for the executables without one of CoD2rev-Plus's
# groups: cod2_plus_script_game when CoD2rev-Plus's server, game and bgame are
# not linked, cod2_plus_script_core when its core (universal, qcommon, xanim)
# is not.
function(cod2_plus_script)
  file(GLOB src CONFIGURE_DEPENDS ${COD2_SRC_DIR}/script/*.cpp)
  add_library(cod2_plus_script OBJECT ${src} ${COD2_SRC_DIR}/compat/script_exports.cpp
    ${COD2_SRC_DIR}/compat/script_imports.cpp ${COD2_SRC_DIR}/compat/script_missing.cpp)
  target_compile_definitions(cod2_plus_script PRIVATE PROTOCOL_VERSION=${COD2_PROTOCOL} NDEBUG)
  if(COD2_LIBCOD_CLIENT)
    target_compile_definitions(cod2_plus_script PRIVATE ${COD2_LIBCOD_DEFS})
    target_include_directories(cod2_plus_script PRIVATE ${COD2_SRC_DIR}/libcod/speex/include)
  endif()
  add_library(cod2_plus_script_game OBJECT ${COD2_SRC_DIR}/compat/script_game_imports.cpp)
  target_compile_definitions(cod2_plus_script_game PRIVATE PROTOCOL_VERSION=${COD2_PROTOCOL} NDEBUG)
  add_library(cod2_plus_script_core OBJECT ${COD2_SRC_DIR}/compat/script_core_imports.cpp
    ${COD2_SRC_DIR}/compat/script_core_missing.cpp)
  target_compile_definitions(cod2_plus_script_core PRIVATE PROTOCOL_VERSION=${COD2_PROTOCOL} NDEBUG)
  if(COD2_PLUS_LIBCOD AND NOT COD2_LIBCOD_CLIENT)
    # the dedicated server's VM, with its LIBCOD blocks
    add_library(cod2_plus_script_ded OBJECT ${src} ${COD2_SRC_DIR}/compat/script_exports.cpp
      ${COD2_SRC_DIR}/compat/script_imports.cpp ${COD2_SRC_DIR}/compat/script_missing.cpp)
    target_compile_definitions(cod2_plus_script_ded PRIVATE PROTOCOL_VERSION=${COD2_PROTOCOL} NDEBUG
      ${COD2_LIBCOD_DEFS})
    target_include_directories(cod2_plus_script_ded PRIVATE ${COD2_SRC_DIR}/libcod/speex/include)
  endif()
endfunction()

# cod2_plus_mbedtls(): the OBJECT library with Mbed TLS, in every build that
# has a client or a dedicated server (the Plus ID's signatures need it, and
# Win64, which has no libcod, links it too), made once and linked by each.
function(cod2_plus_mbedtls)
  if(TARGET cod2_plus_mbedtls)
    return()
  endif()
  set(dir ${COD2_SRC_DIR}/libcod)
  file(GLOB mbedtls CONFIGURE_DEPENDS ${dir}/mbedtls/library/*.c)
  add_library(cod2_plus_mbedtls OBJECT ${mbedtls})
  target_compile_definitions(cod2_plus_mbedtls PRIVATE MBEDTLS_CONFIG_FILE="cod2rev_mbedtls_config.h" NDEBUG)
  target_include_directories(cod2_plus_mbedtls PRIVATE ${dir}/mbedtls ${dir}/mbedtls/include)
  if(COD2_X64)
    # as CoD2rev-Plus's Makefile on x64: Mbed TLS builds AES-NI only with these (it checks the CPU at run time)
    target_compile_options(cod2_plus_mbedtls PRIVATE -maes -mpclmul)
  endif()
endfunction()

# cod2_plus_libcod(): the OBJECT library with libcod and the libraries it
# brings (yyjson, SQLite; Mbed TLS is cod2_plus_mbedtls), each with the flags
# CoD2rev-Plus's Makefile gives it. Its Speex is left out: the engine links its own
# (src/PC/speex, for the client's voice chat), whose API, SpeexBits and
# narrowband mode are the ones libcod_sound.cpp uses, and whose bitstream the
# clients decode.
function(cod2_plus_libcod)
  set(dir ${COD2_SRC_DIR}/libcod)
  file(GLOB libcod CONFIGURE_DEPENDS ${dir}/*.cpp)
  add_library(cod2_plus_libcod OBJECT ${libcod} ${dir}/yyjson/yyjson.c ${dir}/sqlite/sqlite3.c)
  target_compile_definitions(cod2_plus_libcod PRIVATE PROTOCOL_VERSION=${COD2_PROTOCOL} NDEBUG)
  set_source_files_properties(${libcod} PROPERTIES
    COMPILE_DEFINITIONS "${COD2_LIBCOD_DEFS};MBEDTLS_CONFIG_FILE=\"cod2rev_mbedtls_config.h\""
    INCLUDE_DIRECTORIES "${dir}/speex/include;${dir}/mbedtls;${dir}/mbedtls/include")
endfunction()

# cod2_plus_server(): the OBJECT library with CoD2rev-Plus's server, game,
# bgame, universal, qcommon, stringed, xanim, zlib's unzip and platform
# (src/unix, or src/win32 on Windows)
# sources and their bridge to the engine (src/compat/server_*.cpp).
function(cod2_plus_server)
  if(WIN32)
    set(platform ${COD2_SRC_DIR}/win32/win_*.cpp)
  else()
    set(platform ${COD2_SRC_DIR}/unix/*.cpp)
  endif()
  file(GLOB src CONFIGURE_DEPENDS ${COD2_SRC_DIR}/server/*.cpp ${COD2_SRC_DIR}/game/*.cpp
    ${COD2_SRC_DIR}/bgame/*.cpp ${COD2_SRC_DIR}/universal/*.cpp ${COD2_SRC_DIR}/qcommon/*.cpp
    ${COD2_SRC_DIR}/stringed/*.cpp ${COD2_SRC_DIR}/xanim/*.cpp ${COD2_SRC_DIR}/zlib/unzip.c ${COD2_SRC_DIR}/zlib/ioapi.c ${platform}
    ${COD2_SRC_DIR}/compat/server_*.cpp)
  add_library(cod2_plus_server OBJECT ${src})
  target_compile_definitions(cod2_plus_server PRIVATE PROTOCOL_VERSION=${COD2_PROTOCOL} NDEBUG ${COD2_LIBCOD_DEFS})
  if(COD2_PLUS_LIBCOD)
    target_include_directories(cod2_plus_server PRIVATE ${COD2_SRC_DIR}/libcod/speex/include)
    if(NOT TARGET cod2_plus_libcod)  # the client's local server may have made it
      cod2_plus_libcod()
    endif()
  endif()
endfunction()

# cod2_plus_server_client(): the OBJECT library with CoD2rev-Plus's server,
# game and bgame for the client's local games, and its collision (cm_*) for
# all the client, on the engine's other core and xanim, and its bridge
# (src/compat/client_server_*.cpp).
function(cod2_plus_server_client)
  file(GLOB src CONFIGURE_DEPENDS ${COD2_SRC_DIR}/server/*.cpp ${COD2_SRC_DIR}/game/*.cpp
    ${COD2_SRC_DIR}/bgame/*.cpp ${COD2_SRC_DIR}/qcommon/cm_*.cpp ${COD2_SRC_DIR}/compat/client_server_*.cpp)
  add_library(cod2_plus_server_client OBJECT ${src})
  # LISTEN_SERVER: CoD2rev-Plus's code for a server inside a client (its
  # #ifndef DEDICATED blocks)
  target_compile_definitions(cod2_plus_server_client PRIVATE PROTOCOL_VERSION=${COD2_PROTOCOL} NDEBUG LISTEN_SERVER)
  if(COD2_LIBCOD_CLIENT)
    target_compile_definitions(cod2_plus_server_client PRIVATE ${COD2_LIBCOD_DEFS})
    target_include_directories(cod2_plus_server_client PRIVATE ${COD2_SRC_DIR}/libcod/speex/include)
    if(NOT TARGET cod2_plus_libcod)  # or the dedicated server's
      cod2_plus_libcod()
    endif()
  endif()
endfunction()

# cod2_plus_replaced(<list_var> <out_var>): moves to <out_var> the sources of
# <list_var> that test/plus_replaces.txt names (a line ending in / names a
# directory): the dedicated server leaves them to
# CoD2rev-Plus's code, and only the client links them.
function(cod2_plus_replaced list_var out_var)
  file(STRINGS ${CMAKE_SOURCE_DIR}/test/plus_replaces.txt entries REGEX "^[^#]")
  set(keep "")
  set(moved "")
  foreach(src ${${list_var}})
    set(hit FALSE)
    foreach(entry ${entries})
      string(FIND "${src}" "${entry}" pos)
      if(pos EQUAL 0 AND (entry MATCHES "/$" OR src STREQUAL entry))
        set(hit TRUE)
      endif()
    endforeach()
    if(hit)
      list(APPEND moved ${src})
    else()
      list(APPEND keep ${src})
    endif()
  endforeach()
  set(${list_var} ${keep} PARENT_SCOPE)
  set(${out_var} ${moved} PARENT_SCOPE)
endfunction()

# cod2_cxx_sources(<out_var> <sources...>): the sources, with the picked C files
# replaced by their wrappers under ${CMAKE_BINARY_DIR}/cxx_wrap/.
function(cod2_cxx_sources out)
  set(result "")
  foreach(src ${ARGN})
    file(RELATIVE_PATH rel ${COD2_SRC_DIR} ${CMAKE_SOURCE_DIR}/${src})
    set(pick FALSE)
    if(src MATCHES "\\.c$" AND NOT rel MATCHES "^\\.\\." AND NOT rel MATCHES "${COD2_CXX_NEVER}")
      if(COD2_CXX_ALL)
        set(pick TRUE)
      else()
        foreach(dir ${COD2_CXX_DIRS})
          if(rel MATCHES "^${dir}(/|$)")
            set(pick TRUE)
          endif()
        endforeach()
      endif()
    endif()
    if(pick)
      set(wrapper ${CMAKE_BINARY_DIR}/cxx_wrap/${rel}.cpp)
      set(content "extern \"C\" {\n#include \"${CMAKE_SOURCE_DIR}/${src}\"\n}\n")
      set(old "")
      if(EXISTS ${wrapper})
        file(READ ${wrapper} old)
      endif()
      if(NOT old STREQUAL content)
        file(WRITE ${wrapper} "${content}")
      endif()
      get_source_file_property(defs ${src} COMPILE_DEFINITIONS)
      if(defs)
        set_source_files_properties(${wrapper} PROPERTIES COMPILE_DEFINITIONS "${defs}")
      endif()
      list(APPEND result ${wrapper})
    else()
      list(APPEND result ${src})
    endif()
  endforeach()
  set(${out} ${result} PARENT_SCOPE)
endfunction()

# x64 port: expose COD2_X64 to the source so the ILP32 layout pins
# (COD2_ASSERT_FIELD/SIZE in cod2_defs.h) and other arch-specific code can adapt.
# Global so every target/branch below inherits it. No-op for the default 32-bit
# build. Placed before any target is created.
if(COD2_X64)
  add_compile_definitions(COD2_X64=1)
  # add_compile_definitions(COD2_GUARDHEAP=1)  # DIAGNOSTIC: guard-page Z_Malloc to catch heap overruns
endif()

# =============================================================================
if(EMSCRIPTEN)
# ----------------------------------------------------------------------------
# WEB (WebAssembly via Emscripten)
# ----------------------------------------------------------------------------
option(COD2_WEB_WASMFS  "OPFS-backed WASMFS variant (-pthread; needs COOP/COEP)" OFF)
option(COD2_WEB_WORKERFS "True zero-copy WORKERFS variant (-pthread; needs COOP/COEP)" OFF)

set(WEB_GEN ${CMAKE_SOURCE_DIR}/build/web_gen)

set(WEB_GEN_C
  ${WEB_GEN}/wasm_asm_replacements.c ${WEB_GEN}/bss.c
  ${WEB_GEN}/data.c ${WEB_GEN}/link_stubs.c
  ${WEB_GEN}/missing_c_syms.c ${WEB_GEN}/import_pointers.c)

# --- web source set (mirrors WEB_SRCS) ---------------------------------------
file(GLOB_RECURSE WEB_PC_C    RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/PC/*.c")
file(GLOB_RECURSE WEB_MAC_C   RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/Mac/*.c")
file(GLOB_RECURSE WEB_STUBS_C RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/stubs/*.c")
file(GLOB         WEB_ROOT_C  RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/*.c")
set(WEB_C ${WEB_PC_C} ${WEB_MAC_C} ${WEB_STUBS_C} ${WEB_ROOT_C})
# the stubs the generated web versions replace.
list(FILTER WEB_C EXCLUDE REGEX "^src/stubs/(link_stubs|missing_c_syms)\\.c$")
# the Linux platform layer + bundled zlib + web glue are added explicitly.
list(APPEND WEB_C
  src/unix/linux_common.c src/unix/linux_input.c src/unix/linux_net.c src/unix/sysdiff_statehash.c
  src/PC/zlib/adler32.c src/PC/zlib/infblock.c src/PC/zlib/infcodes.c src/PC/zlib/inffast.c
  src/PC/zlib/inflate.c src/PC/zlib/inftrees.c src/PC/zlib/infutil.c src/PC/zlib/zutil.c
  src/web/webgl2_compat.c src/web/wasm_aliases.c src/web/cod2_wasmfs_mount.c)

# the local games on OpenCoD2-Plus Server's VM, server, game, bgame and
# collision, through the bridge, as the other clients
add_compile_definitions(COD2_PLUS_GAME=1 COD2_PLUS_CM=1
  "MAC_STATIC=")   # OpenCoD2-Plus Server's q_shared.h defines it per platform, none of them wasm
cod2_plus_script()
cod2_plus_server_client()
add_executable(cod2 ${WEB_C} ${WEB_GEN_C} $<TARGET_OBJECTS:cod2_plus_script>
  $<TARGET_OBJECTS:cod2_plus_script_core> $<TARGET_OBJECTS:cod2_plus_server_client>)
target_compile_definitions(cod2 PRIVATE WEB_BUILD=1 SDL_DISABLE_IMMINTRIN_H)
target_include_directories(cod2 PRIVATE
  ${COD2_SRC_DIR}/PC/speex ${COD2_SRC_DIR}/PC/zlib ${COD2_SRC_DIR}
  ${COD2_SRC_DIR}/headers ${CMAKE_SOURCE_DIR})
target_compile_options(cod2 PRIVATE -w -O0 -gsource-map -flto -sUSE_SDL=2 ${COD2_WNO})   # the SDL2 port's headers
set_source_files_properties(src/PC/qcommon/crash_handler.c PROPERTIES
  COMPILE_DEFINITIONS "COD2_GIT_HASH=\"${COD2_GIT_HASH}\"")

set(WEB_LINK
  -flto -sUSE_SDL=2 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3=1
  -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=256MB -sSTACK_SIZE=5242880
  -sFORCE_FILESYSTEM=1 -sASSERTIONS=1 -sEXIT_RUNTIME=0 -sERROR_ON_UNDEFINED_SYMBOLS=0
  -sEXPORTED_RUNTIME_METHODS=callMain,FS,ccall
  --shell-file ${COD2_SRC_DIR}/web/shell.html
  -Wl,--allow-multiple-definition)
if(COD2_WEB_WASMFS)
  target_compile_options(cod2 PRIVATE -DWEB_WASMFS -pthread)
  list(APPEND WEB_LINK -sWASMFS -pthread -sPTHREAD_POOL_SIZE=2
       -sEXPORTED_FUNCTIONS=_main,_cod2_mount_opfs)
elseif(COD2_WEB_WORKERFS)
  target_compile_options(cod2 PRIVATE -pthread)
  list(APPEND WEB_LINK -pthread -sPTHREAD_POOL_SIZE=4 -sPROXY_TO_PTHREAD=1
       -sOFFSCREENCANVAS_SUPPORT=1 "-sOFFSCREENCANVASES_TO_PTHREAD=#canvas" -lworkerfs.js)
else()
  list(APPEND WEB_LINK -lidbfs.js)   # legacy MEMFS default (matches `make web`)
endif()
target_link_options(cod2 PRIVATE ${WEB_LINK})
set_target_properties(cod2 PROPERTIES SUFFIX ".html"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# the browser-side loader sits next to cod2.html (shell.html loads it as a sibling).
add_custom_command(TARGET cod2 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${COD2_SRC_DIR}/web/cod2_fs.js ${CMAKE_BINARY_DIR}/cod2_fs.js)

# =============================================================================
elseif(WIN32)
# ----------------------------------------------------------------------------
# WINDOWS (MinGW cross-compile: cod2_win32_ded.exe / cod2_win32.exe)
# ----------------------------------------------------------------------------
option(COD2_WIN32_CLIENT "SDL2/GL client (needs static SDL2 libs) instead of the headless server" OFF)
# the client draws with the system's d3d9 and compiles the game's HLSL with its
# d3dx9, as the original renderer; OFF keeps the Mac port's GL device
option(COD2_WIN32_REAL_D3D9 "Windows client: real Direct3D 9 and d3dx9 shaders instead of the GL device" ON)
# feature gates as on Linux (-DCOD2_PATCH_10 builds 1.0): CoD2rev-Plus's
# PROTOCOL_VERSION already follows them, so the engine has to see them too
set(COD2_FEATURE_CFLAGS "" CACHE STRING "Pass-through feature gates, e.g. -DCOD2_PATCH_10")
separate_arguments(COD2_FEATURE_LIST UNIX_COMMAND "${COD2_FEATURE_CFLAGS}")
add_compile_options(${COD2_FEATURE_LIST})

# win32 C flags (mirror Makefile.win32 WCFLAGS); -fcommon is essential (tentative
# BSS defs in both bss.c and the reconstructed .c rely on the pre-GCC-10 merge).
add_compile_options(
  "$<$<COMPILE_LANGUAGE:C>:-w;-g;-msse2;-mfpmath=sse;-fcommon;-DSDL_DISABLE_IMMINTRIN_H>"
  "$<$<COMPILE_LANGUAGE:C>:-include;${COD2_SRC_DIR}/headers/win32_compat.h>"
  "$<$<COMPILE_LANGUAGE:C>:-D_GNU_SOURCE=1;-DWIN32;-D_WIN32>"
  "$<$<COMPILE_LANGUAGE:C>:${COD2_WNO}>"
  "$<$<COMPILE_LANGUAGE:CXX>:-w;-g;-msse2;-mfpmath=sse;-DSDL_DISABLE_IMMINTRIN_H>"
  "$<$<COMPILE_LANGUAGE:CXX>:-include;${COD2_SRC_DIR}/headers/win32_compat.h>"
  "$<$<COMPILE_LANGUAGE:CXX>:-D_GNU_SOURCE=1;-DWIN32;-D_WIN32>"
  "$<$<COMPILE_LANGUAGE:ASM>:$<$<NOT:$<BOOL:${COD2_X64}>>:-m32>>")
include_directories(${COD2_SRC_DIR}/PC/speex ${COD2_SRC_DIR} ${COD2_SRC_DIR}/headers ${COD2_SRC_DIR}/win32/shims ${CMAKE_SOURCE_DIR})
if(COD2_MSVC_ABI)
  # the MSVC runtime has no POSIX headers (unistd.h, dirent.h, pthread.h): the shims, first
  include_directories(BEFORE ${COD2_SRC_DIR}/win32/shims-msvc)
  add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS
                          _WINSOCK_DEPRECATED_NO_WARNINGS)
endif()

# PE link: same seam aliases as Linux; PE32 decorates every C symbol with a
# leading '_', PE x64 does not
if(COD2_X64)
  set(COD2_PE_C "")
else()
  set(COD2_PE_C "_")
endif()
set(COD2_WIN_LINK
  -static -static-libgcc -static-libstdc++
  -Wl,--allow-multiple-definition -Wl,--wrap,R_Error
  -Wl,--defsym,${COD2_PE_C}__mh_execute_header=0x1000
  -Wl,--defsym,${COD2_PE_C}level_ptr=${COD2_PE_C}level
  -Wl,--defsym,${COD2_PE_C}g_entities_ptr=${COD2_PE_C}g_entities
  -Wl,--defsym,${COD2_PE_C}scr_const_ptr=${COD2_PE_C}scr_const
  -Wl,--defsym,${COD2_PE_C}playerCorpseInfo_ptr=${COD2_PE_C}g_scr_data
  -Wl,--defsym,${COD2_PE_C}g_renderer_ptr=${COD2_PE_C}re
  -Wl,--defsym,${COD2_PE_C}scrAnimPub_ptr=${COD2_PE_C}scrAnimPub
  -Wl,--defsym,${COD2_PE_C}scrCompPub_ptr=${COD2_PE_C}scrCompilePub
  -Wl,--defsym,${COD2_PE_C}scrParserPub_ptr=${COD2_PE_C}scrParserPub
  -Wl,--defsym,${COD2_PE_C}r_frontEndData_ptr=${COD2_PE_C}rg
  -Wl,--defsym,${COD2_PE_C}r_sys_ptr=${COD2_PE_C}ri
  -Wl,--defsym,${COD2_PE_C}r_limits_ptr=${COD2_PE_C}vidConfig
  -Wl,--defsym,${COD2_PE_C}sv_ptr=${COD2_PE_C}sv
  -Wl,--defsym,${COD2_PE_C}svs_ptr=${COD2_PE_C}svs
  -Wl,--defsym,${COD2_PE_C}cg_globUI=${COD2_PE_C}legacyHacks
  -Wl,--defsym,${COD2_PE_C}g_time=${COD2_PE_C}imp_level_bgs
  -Wl,--defsym,${COD2_PE_C}g_time_ptr=${COD2_PE_C}imp_bgs)
set(COD2_WIN_LIBS ws2_32 winmm z stdc++ winpthread dbghelp)
if(COD2_MSVC_ABI)
  # lld-link spells GNU ld's options its own way; the MSVC runtime brings the C++
  # library, and zlib is compiled from src/PC/zlib below
  list(FILTER COD2_WIN_LINK EXCLUDE REGEX "__mh_execute_header")   # absolute: src/stubs/link_stubs.c
  string(REPLACE "-Wl,--defsym," "-Wl,/alternatename:" COD2_WIN_LINK "${COD2_WIN_LINK}")
  list(REMOVE_ITEM COD2_WIN_LINK -static -static-libgcc -static-libstdc++)
  list(TRANSFORM COD2_WIN_LINK REPLACE "^-Wl,--allow-multiple-definition$" "-Wl,/FORCE:MULTIPLE")
  list(TRANSFORM COD2_WIN_LINK REPLACE "^-Wl,--wrap,(.*)$" "-Wl,/wrap:\\1")
  set(COD2_WIN_LIBS ws2_32 winmm dbghelp)
endif()

if(COD2_WIN32_CLIENT)
  if(NOT COD2_X64)
    if(COD2_MSVC_ABI)
      list(APPEND COD2_WIN_LINK -Wl,/LARGEADDRESSAWARE)
    else()
      list(APPEND COD2_WIN_LINK -Wl,--large-address-aware)  # DXVK maps its staging memory into the process
    endif()
  endif()
  add_compile_definitions(W32_CLIENT)
  if(COD2_X64)
    # the x86_64 static SDL2 of SDL's MinGW package (scripts/mingw64_deps.sh)
    set(COD2_SDL2_WIN64 ${CMAKE_SOURCE_DIR}/third_party/mingw64-sdl2/SDL2-2.30.9/x86_64-w64-mingw32)
    include_directories(${COD2_SDL2_WIN64}/include ${COD2_SDL2_WIN64}/include/SDL2)
    list(APPEND COD2_WIN_LINK -L${COD2_SDL2_WIN64}/lib)
  elseif(COD2_MSVC_ABI)
    # SDL's VC development package (SDL2.lib, SDL2.dll), supplied in third_party/
    file(GLOB COD2_SDL2_VC ${CMAKE_SOURCE_DIR}/third_party/SDL2-*/lib/x86)
    if(NOT COD2_SDL2_VC)
      message(FATAL_ERROR "no SDL2 VC package under third_party/SDL2-*/ (SDL2-devel-<version>-VC.zip)")
    endif()
    list(GET COD2_SDL2_VC 0 COD2_SDL2_VC)
    # the package keeps its headers in include/, the engine includes <SDL2/SDL.h>
    file(COPY ${COD2_SDL2_VC}/../../include/ DESTINATION ${CMAKE_BINARY_DIR}/sdl2/SDL2)
    include_directories(${CMAKE_BINARY_DIR}/sdl2 ${CMAKE_BINARY_DIR}/sdl2/SDL2)
    list(APPEND COD2_WIN_LINK -L${COD2_SDL2_VC})
  else()
    include_directories(${COD2_SRC_DIR}/win32/sdl2/include)
    list(APPEND COD2_WIN_LINK -L${COD2_SRC_DIR}/win32/sdl2/lib)
  endif()
  list(APPEND COD2_WIN_LIBS SDL2 opengl32 imm32 ole32 oleaut32 version uuid wininet
       setupapi shell32 dinput8 gdi32 advapi32 user32 kernel32 winmm ws2_32)
  set(W32_EXE cod2_win32)
  # one executable per configure: the blobs leave the game's and the
  # collision's globals to CoD2rev-Plus's code, as cod2_linux's do
  add_compile_definitions(COD2_PLUS_GAME=1 COD2_PLUS_CM=1)
else()
  add_compile_definitions(DEDICATED)
  set(W32_EXE cod2_win32_ded)
  add_compile_definitions(COD2_PLUS_SERVER=1 COD2_PLUS_GAME=1)  # the blobs leave their globals to CoD2rev-Plus
endif()
if(COD2_WIN32_CLIENT AND COD2_WIN32_REAL_D3D9)
  add_compile_definitions(GFX_REAL_D3D9)
endif()

# source set (mirror WIN_C_SRCS): src/PC, src/Mac, src/stubs, src/win32, and
# src-root files, minus the native-asm
# data .c, cpp_trampoline, agl_stubs, and the bundled-zlib inflate split (use -lz).
file(GLOB_RECURSE W_PC_C    CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/PC/*.c")
file(GLOB_RECURSE W_MAC_C   CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/Mac/*.c")
file(GLOB_RECURSE W_STUBS_C CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/stubs/*.c")
file(GLOB_RECURSE W_WIN_C   CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/win32/*.c")
file(GLOB         W_ROOT_C  CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/*.c")
set(WIN_C ${W_PC_C} ${W_MAC_C} ${W_STUBS_C} ${W_WIN_C} ${W_ROOT_C})
list(FILTER WIN_C EXCLUDE REGEX "^src/stubs/(cpp_trampoline|agl_stubs)\\.c$")
# the MSVC-ABI build's POSIX shims (its own dirent, pthreads): with MinGW
# their readdir replaced the C runtime's, whose struct dirent differs, so
# every file name came 8 bytes short and no .iwd was found
list(FILTER WIN_C EXCLUDE REGEX "^src/win32/shims-msvc/")
if(COD2_MSVC_ABI)
  # the MSVC runtime has no readdir nor pthreads
  list(APPEND WIN_C src/win32/shims-msvc/dirent_msvc.c src/win32/shims-msvc/pthread_msvc.c)
  # zlib allocates through the engine (src/stubs/zlib_alloc.c), as on MinGW, where it wins over -lz's
  set_source_files_properties(src/PC/zlib/zutil.c PROPERTIES COMPILE_DEFINITIONS MY_ZCALLOC)
else()
  list(FILTER WIN_C EXCLUDE REGEX "^src/PC/zlib/(inflate|infblock|infcodes|inffast|inftrees|infutil|adler32|zutil)\\.c$")
endif()
list(APPEND WIN_C src/blobs/bss.c src/unix/sysdiff_statehash.c)
if(COD2_WIN32_CLIENT)
  list(APPEND WIN_C src/unix/linux_input.c)   # pure-SDL2 event pump, works on win32
  # src/snd_sdl instead of the Mac port's engine
  list(REMOVE_ITEM WIN_C src/Mac/Tools/MacMSS.c)
else()
  list(REMOVE_ITEM WIN_C src/win32/gl_loader.c)  # client-only (real GL loader)
endif()

# ELF .S data blobs -> PE/COFF .S, then assemble (data/literals/import_pointers/vtables).
set(W32GEN ${CMAKE_SOURCE_DIR}/build/win_gen)
if(COD2_X64)
  # x64: the portable C blobs (pointer slots as void *), as the 64-bit Linux
  # client and MSVC; PE x64 names need no '_' aliases for their C++ symbols
  set(WIN_ASM_GEN ${CMAKE_SOURCE_DIR}/build/native_gen/data32.c ${CMAKE_SOURCE_DIR}/build/native_gen/literals32.c
      ${CMAKE_SOURCE_DIR}/build/native_gen/import_pointers_native.c)
else()
  set(WIN_ASM_GEN ${W32GEN}/data.win.S ${W32GEN}/literals.win.S ${W32GEN}/import_pointers.win.S)
  set_source_files_properties(${WIN_ASM_GEN} PROPERTIES LANGUAGE ASM)
endif()

# PE link aliases for C++ symbols stored in the (C-compiled) data blobs: the C
# compiler prefixes them to '___ZN...' (common syms, no address until link), so
# we point the '__ZN...' references at them with --defsym (an asm .set can't
# bind a common symbol). Generated at configure time from the blob sources.
file(STRINGS ${W32GEN}/cxx_defsyms_c.txt COD2_CXX_DEFSYMS_C)
file(STRINGS ${W32GEN}/cxx_defsyms_asm.txt COD2_CXX_DEFSYMS_ASM)
if(NOT COD2_X64)
  if(COD2_MSVC_ABI)
    set(COD2_DEFSYM -Wl,/alternatename:)
  else()
    set(COD2_DEFSYM -Wl,--defsym,)
  endif()
  foreach(pair ${COD2_CXX_DEFSYMS_C} ${COD2_CXX_DEFSYMS_ASM})
    list(APPEND COD2_WIN_LINK ${COD2_DEFSYM}${pair})
  endforeach()
endif()

set_source_files_properties(src/PC/qcommon/crash_handler.c PROPERTIES
  COMPILE_DEFINITIONS "COD2_GIT_HASH=\"${COD2_GIT_HASH}\"")

cod2_plus_script()
set(COD2_SCRIPT_OBJS $<TARGET_OBJECTS:cod2_plus_script>)
if(NOT COD2_WIN32_CLIENT)
  cod2_plus_replaced(WIN_C replaced)
  cod2_plus_server()
  set(COD2_W32_SERVER_OBJS $<TARGET_OBJECTS:cod2_plus_server>)
  if(COD2_PLUS_LIBCOD)
    if(NOT COD2_LIBCOD_CLIENT)
      set(COD2_SCRIPT_OBJS $<TARGET_OBJECTS:cod2_plus_script_ded>)
    endif()
    list(APPEND COD2_W32_SERVER_OBJS $<TARGET_OBJECTS:cod2_plus_libcod>)
  endif()
else()
  # the client runs CoD2rev-Plus's server, game, bgame and collision, as cod2_linux does
  cod2_plus_server_client()
  set(COD2_W32_SERVER_OBJS $<TARGET_OBJECTS:cod2_plus_script_core> $<TARGET_OBJECTS:cod2_plus_server_client>)
  if(COD2_LIBCOD_CLIENT)
    list(APPEND COD2_W32_SERVER_OBJS $<TARGET_OBJECTS:cod2_plus_libcod>)
  endif()
endif()
cod2_plus_mbedtls()
list(APPEND COD2_W32_SERVER_OBJS $<TARGET_OBJECTS:cod2_plus_mbedtls>)
cod2_cxx_sources(WIN_C ${WIN_C})
if(COD2_WIN32_CLIENT)
  # The sound engine: C as on Linux, and optimized, as this client's other code
  # is not and a mixer has a deadline. SDL's threads enter with a 4-byte stack.
  file(GLOB COD2_SND_SDL_C CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/snd_sdl/*.c")
  set_source_files_properties(${COD2_SND_SDL_C} PROPERTIES COMPILE_OPTIONS
    "-O2;-fno-strict-aliasing;-fwrapv;$<$<NOT:$<OR:$<BOOL:${COD2_X64}>,$<BOOL:${COD2_MSVC_ABI}>>>:-mincoming-stack-boundary=2>")
  list(APPEND WIN_C ${COD2_SND_SDL_C})
endif()
enable_language(RC)
add_executable(${W32_EXE} ${WIN_C} ${WIN_ASM_GEN} ${COD2_SCRIPT_OBJS} ${COD2_W32_SERVER_OBJS}
  ${COD2_SRC_DIR}/win32/opencod2.rc)
if(COD2_LIBCOD_CLIENT)
  # the engine's Com_Frame and Com_EventLoop (common.c) work for libcod
  target_compile_definitions(${W32_EXE} PRIVATE COD2_PLUS_LIBCOD=1)
endif()
# Mbed TLS: BCryptGenRandom for its entropy, the system store for its CAs
target_link_libraries(${W32_EXE} PRIVATE bcrypt crypt32)

# The shared headers compiled as C++ (their layout pins and C linkage).
add_library(cod2_cxx_header_check OBJECT test/cxx/header_check.cpp)
target_link_options(${W32_EXE} PRIVATE ${COD2_WIN_LINK})
if(COD2_MSVC_ABI)
  # C and C++ globals meet by name only on the Itanium ABI: alias them (cmake/msvc_data_aliases.cmake)
  get_filename_component(COD2_LLVM_BIN ${CMAKE_CXX_COMPILER} DIRECTORY)
  add_custom_command(TARGET ${W32_EXE} PRE_LINK
    COMMAND ${CMAKE_COMMAND} -DNM=${COD2_LLVM_BIN}/llvm-nm.exe -DOBJECT_DIR=${CMAKE_BINARY_DIR}/CMakeFiles
            -DOUT=${CMAKE_BINARY_DIR}/msvc_data_aliases.rsp -P ${CMAKE_SOURCE_DIR}/cmake/msvc_data_aliases.cmake
    VERBATIM)
  # a console program entering at WinMain, as MinGW's runtime does
  target_link_options(${W32_EXE} PRIVATE -Wl,@${CMAKE_BINARY_DIR}/msvc_data_aliases.rsp -Wl,/MAP
                      -Wl,/ENTRY:WinMainCRTStartup -Wl,/SAFESEH:NO   # the assembled blobs carry no SEH tables
                      -Wl,/STACK:0x200000   # GNU ld's 2 MB, the stack every map and script ran with on MinGW
                      -Wl,/DEBUG)   # a .pdb, so the crash reporter names functions
endif()
target_link_libraries(${W32_EXE} PRIVATE ${COD2_WIN_LIBS})
set_target_properties(${W32_EXE} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} SUFFIX ".exe")


# =============================================================================
else()
# ----------------------------------------------------------------------------
# NATIVE Linux (cod2_linux + cod2_lnxded)
# ----------------------------------------------------------------------------
option(COD2_DEBUG "-O0, no hardening (gdb / canary-UB work; mirrors DEBUG=1)" OFF)
option(COD2_LINUX_DXVK "Linux client on real Direct3D 9 (DXVK native, vkd3d-shader); OFF: the GL device" ON)
# the 64-bit client loads 64-bit builds of them: scripts/build_linux_vk_deps.sh x64
if(COD2_X64)
  set(COD2_LINUX_VK_DIR ${CMAKE_SOURCE_DIR}/third_party/linux-vk64)
  set(COD2_LINUX_VK_SCRIPT "scripts/build_linux_vk_deps.sh x64")
else()
  set(COD2_LINUX_VK_DIR ${CMAKE_SOURCE_DIR}/third_party/linux-vk)
  set(COD2_LINUX_VK_SCRIPT "scripts/build_linux_vk_deps.sh")
endif()
if(COD2_LINUX_DXVK)
  if(NOT EXISTS ${COD2_LINUX_VK_DIR}/lib/libdxvk_d3d9.so.0 OR NOT EXISTS ${COD2_LINUX_VK_DIR}/lib/libvkd3d-shader.so.1
     OR NOT EXISTS ${COD2_LINUX_VK_DIR}/include/vkd3d_shader.h)
    message(FATAL_ERROR "The Linux client draws on DXVK: run ${COD2_LINUX_VK_SCRIPT} first, "
                        "or configure with -DCOD2_LINUX_DXVK=OFF for the GL device")
  endif()
  add_compile_definitions(GFX_REAL_D3D9)
  include_directories(${COD2_LINUX_VK_DIR}/include)
  message(STATUS "cod2_linux: DXVK (Vulkan)")
else()
  # a build directory configured before DXVK became the default keeps OFF cached
  message(STATUS "cod2_linux: GL device (COD2_LINUX_DXVK=OFF)")
endif()
set(COD2_FEATURE_CFLAGS "" CACHE STRING "Pass-through feature gates, e.g. -DCOD2_PATCH_13=1")
set(COD2_WWWDL_LIBS "-l:libcurl.so.4" CACHE STRING "1.3 redirected-download backend lib")

# Architecture seam (COD2_X64): the original build is ILP32/x86 (-m32, /usr/lib32,
# 4-byte-stack-boundary). The x64 port uses the native LP64 ABI instead. Every
# value below defaults to the original 32-bit one so existing builds are byte-for-
# byte unchanged when COD2_X64=OFF.
if(COD2_X64)
  set(COD2_ARCH_CFLAGS "")                                 # native x86_64
  set(COD2_ARCH_LDFLAGS "")
  set(COD2_LIBSTDCPP "stdc++" CACHE STRING "C++ runtime")  # native libstdc++
  set(COD2_ARCH_LIBDIRS "")
else()
  set(COD2_ARCH_CFLAGS -m32 -mincoming-stack-boundary=2)
  set(COD2_ARCH_LDFLAGS -m32)
  set(COD2_LIBSTDCPP "/usr/lib32/libstdc++.so.6" CACHE STRING "32-bit libstdc++")
  set(COD2_ARCH_LIBDIRS -L/usr/lib32 -L/usr/lib/i386-linux-gnu)
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(SDL2 sdl2)
endif()
if(NOT SDL2_FOUND)
  set(SDL2_INCLUDE_DIRS /usr/include/SDL2)
  set(SDL2_LIBRARIES SDL2)
endif()
# the 64-bit client: SDL2's amd64 headers from scripts/linux_x64_deps.sh (a host set up
# for the 32-bit builds has only libsdl2-dev:i386), and the system's amd64 runtime
set(COD2_LINUX_X64_DIR ${CMAKE_SOURCE_DIR}/third_party/linux-x64)
if(COD2_X64 AND NOT CMAKE_TOOLCHAIN_FILE AND EXISTS ${COD2_LINUX_X64_DIR}/usr/include/SDL2/SDL.h)
  set(SDL2_INCLUDE_DIRS ${COD2_LINUX_X64_DIR}/usr/include/x86_64-linux-gnu ${COD2_LINUX_X64_DIR}/usr/include
      ${COD2_LINUX_X64_DIR}/usr/include/x86_64-linux-gnu/SDL2 ${COD2_LINUX_X64_DIR}/usr/include/SDL2)
  set(SDL2_CFLAGS_OTHER -D_REENTRANT)
  set(SDL2_LIBRARIES -l:libSDL2-2.0.so.0)
endif()

if(COD2_DEBUG)
  set(COD2_OPT_FLAGS -O0)
  set(COD2_HARDEN_CFLAGS "")
  set(COD2_HARDEN_LDFLAGS "")
else()
  set(COD2_OPT_FLAGS
      -O2 -fno-strict-aliasing -fwrapv -fno-strict-overflow
      -fno-delete-null-pointer-checks -D_FORTIFY_SOURCE=2)
  set(COD2_HARDEN_CFLAGS -fstack-protector-strong -fstack-clash-protection)
  set(COD2_HARDEN_LDFLAGS -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack)
endif()

separate_arguments(COD2_FEATURE_LIST UNIX_COMMAND "${COD2_FEATURE_CFLAGS}")

add_compile_options(
  "$<$<COMPILE_LANGUAGE:C>:${COD2_ARCH_CFLAGS};-fno-pie;-no-pie;-w;-g;-msse2;-mfpmath=sse;-fno-omit-frame-pointer>"
  "$<$<COMPILE_LANGUAGE:C>:-include;${COD2_SRC_DIR}/headers/glibc_compat.h>"
  "$<$<COMPILE_LANGUAGE:C>:-DSDL_DISABLE_IMMINTRIN_H>"
  "$<$<COMPILE_LANGUAGE:C>:${SDL2_CFLAGS_OTHER}>"
  "$<$<COMPILE_LANGUAGE:C>:${COD2_OPT_FLAGS}>"
  "$<$<COMPILE_LANGUAGE:C>:${COD2_WNO}>"
  "$<$<COMPILE_LANGUAGE:C>:${COD2_HARDEN_CFLAGS}>"
  "$<$<COMPILE_LANGUAGE:C>:${COD2_FEATURE_LIST}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${COD2_ARCH_CFLAGS};-fno-pie;-no-pie;-w;-g;-msse2;-mfpmath=sse;-fno-omit-frame-pointer>"
  "$<$<COMPILE_LANGUAGE:CXX>:-include;${COD2_SRC_DIR}/headers/glibc_compat.h>"
  "$<$<COMPILE_LANGUAGE:CXX>:-DSDL_DISABLE_IMMINTRIN_H>"
  "$<$<COMPILE_LANGUAGE:CXX>:${SDL2_CFLAGS_OTHER}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${COD2_OPT_FLAGS}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${COD2_HARDEN_CFLAGS}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${COD2_FEATURE_LIST}>"
)
include_directories(${COD2_SRC_DIR}/PC/speex ${COD2_SRC_DIR} ${COD2_SRC_DIR}/headers ${CMAKE_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})

add_link_options(${COD2_ARCH_LDFLAGS} -no-pie ${COD2_ARCH_LIBDIRS} -rdynamic ${COD2_HARDEN_LDFLAGS})

set(COD2_LINK_EXTRA
  -Wl,--wrap,R_Error
  -Wl,--defsym,__mh_execute_header=0x1000
  -Wl,--defsym,level_ptr=level
  -Wl,--defsym,g_entities_ptr=g_entities
  -Wl,--defsym,scr_const_ptr=scr_const
  -Wl,--defsym,playerCorpseInfo_ptr=g_scr_data
  -Wl,--defsym,g_renderer_ptr=re
  -Wl,--defsym,scrAnimPub_ptr=scrAnimPub
  -Wl,--defsym,scrCompPub_ptr=scrCompilePub
  -Wl,--defsym,scrParserPub_ptr=scrParserPub
  -Wl,--defsym,r_frontEndData_ptr=rg
  -Wl,--defsym,r_sys_ptr=ri
  -Wl,--defsym,r_limits_ptr=vidConfig
  -Wl,--defsym,sv_ptr=sv
  -Wl,--defsym,svs_ptr=svs
  -Wl,--defsym,cg_globUI=legacyHacks
  -Wl,--defsym,g_time=imp_level_bgs
  -Wl,--defsym,g_time_ptr=imp_bgs)

# The dedicated server gets src/unix/linux_ded_stubs.c instead of the
# client's libraries (COD2_CLIENT_LIBS: GL, X11, SDL2 and curl), so it runs on
# a machine without them.
set(COD2_DED_LIBS m pthread dl z ${COD2_LIBSTDCPP})
set(COD2_CLIENT_LIBS GL X11 ${SDL2_LIBRARIES})
if(COD2_WWWDL_LIBS)
  separate_arguments(COD2_WWWDL_LIST UNIX_COMMAND "${COD2_WWWDL_LIBS}")
  list(APPEND COD2_CLIENT_LIBS ${COD2_WWWDL_LIST})
endif()
set(COD2_LIBS ${COD2_CLIENT_LIBS} ${COD2_DED_LIBS})

file(GLOB_RECURSE COD2_PC_C    CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/PC/*.c")
file(GLOB_RECURSE COD2_MAC_C   CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/Mac/*.c")
file(GLOB_RECURSE COD2_STUBS_C CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/stubs/*.c")
file(GLOB         COD2_ROOT_C  CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/*.c")
set(ALL_C ${COD2_PC_C} ${COD2_MAC_C} ${COD2_STUBS_C} ${COD2_ROOT_C})
# Sound: the clients have the SDL engine, and the dedicated server the Mac port's stubs of Miles.
# src/stubs/audio_stubs.c stays in both: the voice code (groupvoice, mac_record_dsound.c) links to it.
file(GLOB COD2_SND_DED_C CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/Mac/Tools/MacMSS*.c")
list(REMOVE_ITEM ALL_C ${COD2_SND_DED_C})
file(GLOB COD2_SND_SDL_C CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/snd_sdl/*.c")
file(GLOB_RECURSE COD2_PC_ASM    CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/PC/*.S")
file(GLOB_RECURSE COD2_STUBS_ASM CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/stubs/*.S")
file(GLOB         COD2_ROOT_ASM  CONFIGURE_DEPENDS RELATIVE ${CMAKE_SOURCE_DIR} "${COD2_SRC_DIR}/*.S")
set(ALL_ASM ${COD2_PC_ASM} ${COD2_STUBS_ASM} ${COD2_ROOT_ASM})

list(FILTER ALL_C EXCLUDE REGEX "(^|/)(inflate|infblock|infcodes|inffast|inftrees|infutil|adler32|zutil)\\.c$")

list(APPEND ALL_C
  src/blobs/bss.c src/unix/linux_common.c src/unix/linux_input.c src/unix/linux_net.c src/unix/sysdiff_statehash.c)

set(GEN_DIR ${CMAKE_SOURCE_DIR}/build/native_gen)
if(NOT EXISTS ${GEN_DIR}/import_pointers_native.c)
  message(FATAL_ERROR "missing ${GEN_DIR}/import_pointers_native.c (a prebuilt DATA_PORTABLE artifact)")
endif()
set(GEN_C ${GEN_DIR}/data32.c ${GEN_DIR}/literals32.c ${GEN_DIR}/import_pointers_native.c)

set_source_files_properties(src/PC/qcommon/crash_handler.c PROPERTIES
  COMPILE_DEFINITIONS "COD2_GIT_HASH=\"${COD2_GIT_HASH}\"")

set(COD2_DED_GUARDED src/Mac/Main/mac_main.c src/PC/qcommon/common.c)
list(REMOVE_ITEM ALL_C ${COD2_DED_GUARDED})
# The dedicated server links CoD2rev-Plus's code instead of the sources in
# test/plus_replaces.txt, which only the client links, and the blobs that
# define their globals are compiled for each executable, leaving them to
# CoD2rev-Plus's code in the dedicated.
cod2_plus_replaced(ALL_C COD2_SERVER_MP_C)
cod2_plus_replaced(COD2_DED_GUARDED replaced_guarded)
list(APPEND COD2_SERVER_MP_C ${replaced_guarded})
list(REMOVE_ITEM ALL_C src/blobs/bss.c src/PC/universal/com_math.c)
list(REMOVE_ITEM GEN_C ${GEN_DIR}/data32.c ${GEN_DIR}/literals32.c)
list(APPEND COD2_DED_GUARDED src/blobs/bss.c ${GEN_DIR}/data32.c ${GEN_DIR}/literals32.c
  src/PC/universal/com_math.c)

# 64-bit file offsets and inode numbers: without them a 32-bit readdir/stat
# fails with EOVERFLOW on file systems with large inodes (Windows drives under
# WSL), and no .iwd is found.
add_compile_definitions(_FILE_OFFSET_BITS=64)

cod2_plus_script()
set(COD2_SCRIPT_OBJS $<TARGET_OBJECTS:cod2_plus_script>)
cod2_cxx_sources(ALL_C ${ALL_C})
cod2_cxx_sources(COD2_DED_GUARDED ${COD2_DED_GUARDED})
cod2_cxx_sources(COD2_SND_DED_C ${COD2_SND_DED_C})
add_library(cod2_objs OBJECT ${ALL_C} ${ALL_ASM} ${GEN_C})
add_library(cod2_snd_ded OBJECT ${COD2_SND_DED_C})
add_library(cod2_snd_sdl OBJECT ${COD2_SND_SDL_C})
# the client runs CoD2rev-Plus's server, game and bgame
cod2_plus_server_client()
set(COD2_CLIENT_SERVER_OBJS $<TARGET_OBJECTS:cod2_plus_script_core> $<TARGET_OBJECTS:cod2_plus_server_client>)
if(COD2_LIBCOD_CLIENT)
  list(APPEND COD2_CLIENT_SERVER_OBJS $<TARGET_OBJECTS:cod2_plus_libcod>)
endif()
cod2_cxx_sources(COD2_SERVER_MP_C ${COD2_SERVER_MP_C})
add_library(cod2_server_mp OBJECT ${COD2_SERVER_MP_C})
if(COD2_LIBCOD_CLIENT)
  # the engine's Com_Frame and Com_EventLoop (common.c, here) work for libcod
  target_compile_definitions(cod2_server_mp PRIVATE COD2_PLUS_LIBCOD=1)
endif()
cod2_plus_server()
cod2_plus_mbedtls()
list(APPEND COD2_CLIENT_SERVER_OBJS $<TARGET_OBJECTS:cod2_server_mp> $<TARGET_OBJECTS:cod2_plus_mbedtls>)
set(COD2_DED_SERVER_OBJS $<TARGET_OBJECTS:cod2_plus_server> $<TARGET_OBJECTS:cod2_plus_mbedtls>)
if(COD2_PLUS_LIBCOD)
  list(APPEND COD2_DED_SERVER_OBJS $<TARGET_OBJECTS:cod2_plus_libcod>)
endif()
set(COD2_DED_SCRIPT_OBJS ${COD2_SCRIPT_OBJS})
if(COD2_PLUS_LIBCOD AND NOT COD2_LIBCOD_CLIENT)
  set(COD2_DED_SCRIPT_OBJS $<TARGET_OBJECTS:cod2_plus_script_ded>)
endif()

# The shared headers compiled as C++ (their layout pins and C linkage).
add_library(cod2_cxx_header_check OBJECT test/cxx/header_check.cpp)

add_executable(cod2_linux $<TARGET_OBJECTS:cod2_objs> $<TARGET_OBJECTS:cod2_snd_sdl> ${COD2_SCRIPT_OBJS} ${COD2_CLIENT_SERVER_OBJS} ${COD2_DED_GUARDED})
target_link_libraries(cod2_linux PRIVATE ${COD2_LIBS})
target_link_options(cod2_linux PRIVATE ${COD2_LINK_EXTRA})

add_executable(cod2_lnxded $<TARGET_OBJECTS:cod2_objs> $<TARGET_OBJECTS:cod2_snd_ded> ${COD2_DED_SCRIPT_OBJS} ${COD2_DED_SERVER_OBJS} ${COD2_DED_GUARDED}
  src/unix/linux_ded_stubs.c)
target_compile_definitions(cod2_lnxded PRIVATE DEDICATED COD2_PLUS_SERVER=1 COD2_PLUS_GAME=1)
target_compile_definitions(cod2_linux PRIVATE $<$<BOOL:${COD2_PLUS_CLIENT_SERVER}>:COD2_PLUS_GAME=1>
  $<$<BOOL:${COD2_PLUS_CLIENT_SERVER}>:COD2_PLUS_CM=1>)
target_link_libraries(cod2_lnxded PRIVATE ${COD2_DED_LIBS})
target_link_options(cod2_lnxded PRIVATE ${COD2_LINK_EXTRA})

set_target_properties(cod2_linux cod2_lnxded PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# COD2_LINUX_DXVK: the client renders through the real Direct3D 9 path
# (GFX_REAL_D3D9) as the Windows client, on DXVK native, and compiles the
# game's shaders with vkd3d-shader; both are loaded at run time from next to
# the executable (scripts/build_linux_vk_deps.sh builds them). The whole build
# directory is compiled that way.
if(COD2_LINUX_DXVK)
  target_link_options(cod2_linux PRIVATE "-Wl,-rpath,$ORIGIN")
  foreach(lib libdxvk_d3d9.so.0 libvkd3d-shader.so.1)
    add_custom_command(TARGET cod2_linux POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COD2_LINUX_VK_DIR}/lib/${lib} ${CMAKE_BINARY_DIR}/${lib})
  endforeach()
endif()

endif()
