# The 64-bit Linux dedicated server: CoD2rev-Plus's sources alone, with the
# flags of its Makefile. The engine stays out, its layouts and data blobs are
# 32-bit.
#
#   cmake -S cmake/lnxded64 -B build/check-linux-x64 [-DCOD2_PROTOCOL=115]
#   cmake --build build/check-linux-x64
cmake_minimum_required(VERSION 3.16)
project(cod2_lnxded64 LANGUAGES C CXX)

set(COD2_PROTOCOL 118 CACHE STRING "Game version: 115 (1.0) or 118 (1.3)")
get_filename_component(SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../src ABSOLUTE)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "this project builds 64-bit only")
endif()

set(common -fno-pie -O1 -fno-omit-frame-pointer -msse2 -mfpmath=sse -m64 -Wformat -Wno-unused-result
  -fstack-protector-strong -fno-strict-aliasing -fwrapv)
add_compile_options(${common} "$<$<COMPILE_LANGUAGE:CXX>:-include;${SRC}/headers/glibc_compat.h>")
add_compile_definitions(NDEBUG PROTOCOL_VERSION=${COD2_PROTOCOL} _FORTIFY_SOURCE=2 _FILE_OFFSET_BITS=64)
option(COD2_PLUS_PROTOCOL "1.3 builds the 1.3 Plus server (OFF: stock 1.3)" ON)
if(COD2_PROTOCOL EQUAL 118 AND COD2_PLUS_PROTOCOL)
  add_compile_definitions(COD2_PLUS)  # OpenCoD2-Plus Server's make PLUS=1: the 1.3 Plus server
endif()

set(libcod_defs LIBCOD LIBCOD_COMPILE_BOTS=1 LIBCOD_COMPILE_ENTITY=1 LIBCOD_COMPILE_EXEC=1
  LIBCOD_COMPILE_GRAPH=1 LIBCOD_COMPILE_HTTP=1 LIBCOD_COMPILE_JSON=1 LIBCOD_COMPILE_LEVEL=1
  LIBCOD_COMPILE_MEMORY=1 LIBCOD_COMPILE_PLAYER=1 LIBCOD_COMPILE_RATELIMITER=1
  LIBCOD_COMPILE_SQLITE=1 LIBCOD_COMPILE_SPEEX=1 LIBCOD_COMPILE_UTILS=1 LIBCOD_COMPILE_WEAPONS=1)
set(mbedtls_def MBEDTLS_CONFIG_FILE="cod2rev_mbedtls_config.h")
set(mbedtls_dirs ${SRC}/libcod/mbedtls ${SRC}/libcod/mbedtls/include)

file(GLOB game CONFIGURE_DEPENDS ${SRC}/bgame/*.cpp ${SRC}/game/*.cpp ${SRC}/qcommon/*.cpp ${SRC}/script/*.cpp
  ${SRC}/server/*.cpp ${SRC}/stringed/*.cpp ${SRC}/universal/*.cpp ${SRC}/xanim/*.cpp ${SRC}/unix/*.cpp)
file(GLOB libcod CONFIGURE_DEPENDS ${SRC}/libcod/*.cpp)
file(GLOB zlib CONFIGURE_DEPENDS ${SRC}/zlib/*.c)
file(GLOB speex CONFIGURE_DEPENDS ${SRC}/libcod/speex/libspeex/*.c)
file(GLOB mbedtls CONFIGURE_DEPENDS ${SRC}/libcod/mbedtls/library/*.c)

add_executable(cod2_lnxded ${game} ${libcod} ${zlib} ${speex} ${mbedtls}
  ${SRC}/libcod/yyjson/yyjson.c ${SRC}/libcod/sqlite/sqlite3.c)
set_source_files_properties(${game} PROPERTIES COMPILE_DEFINITIONS "${libcod_defs}"
  INCLUDE_DIRECTORIES "${SRC}/libcod/speex/include")
set_source_files_properties(${libcod} PROPERTIES COMPILE_DEFINITIONS "${libcod_defs};${mbedtls_def}"
  INCLUDE_DIRECTORIES "${SRC}/libcod/speex/include;${mbedtls_dirs}")
set_source_files_properties(${speex} PROPERTIES COMPILE_DEFINITIONS HAVE_CONFIG_H
  INCLUDE_DIRECTORIES "${SRC}/libcod/speex;${SRC}/libcod/speex/include")
# Mbed TLS only builds AES-NI on x86-64 with these; it still checks the CPU at run time
set_source_files_properties(${mbedtls} PROPERTIES COMPILE_DEFINITIONS "${mbedtls_def}"
  COMPILE_OPTIONS "-maes;-mpclmul" INCLUDE_DIRECTORIES "${mbedtls_dirs}")

target_link_options(cod2_lnxded PRIVATE -no-pie -m64 -Wl,-z,relro,-z,now)
# zlib: src/zlib holds only CoD2rev-Plus's unzip, as the 32-bit builds
target_link_libraries(cod2_lnxded PRIVATE m pthread stdc++ dl z)
