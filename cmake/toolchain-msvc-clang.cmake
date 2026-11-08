# The Windows client on the MSVC ABI: clang's GNU-style driver targeting
# i686-pc-windows-msvc, lld-link, the MSVC C runtime and the Windows SDK.
# Run from an environment vcvars32.bat has set up (INCLUDE, LIB); clang comes
# from VS's "C++ Clang tools" (COD2_CLANG_DIR overrides where).
#   cmake -G Ninja -S . -B build/msvc -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-msvc-clang.cmake -DCOD2_WIN32_CLIENT=ON
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

if(NOT COD2_CLANG_DIR)
  set(COD2_CLANG_DIR "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin")
endif()
set(CMAKE_C_COMPILER   "${COD2_CLANG_DIR}/clang.exe")
set(CMAKE_CXX_COMPILER "${COD2_CLANG_DIR}/clang++.exe")
set(CMAKE_ASM_COMPILER "${COD2_CLANG_DIR}/clang.exe")
set(CMAKE_RC_COMPILER  "${COD2_CLANG_DIR}/llvm-rc.exe")
set(CMAKE_LINKER       "${COD2_CLANG_DIR}/lld-link.exe")
set(CMAKE_AR           "${COD2_CLANG_DIR}/llvm-lib.exe")
foreach(lang C CXX ASM)
  set(CMAKE_${lang}_COMPILER_TARGET i686-pc-windows-msvc)
endforeach()
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")

set(COD2_MSVC_ABI ON CACHE BOOL "the Windows branch on the MSVC ABI (clang + lld-link)" FORCE)
