# The /alternatename pairs that join C and C++ globals on the MSVC ABI.
#
# On MinGW (Itanium C++ ABI) a namespace-scope C++ variable has its plain name,
# so OpenCoD2-Plus Server's C++ and the engine's C share globals by name. The
# MSVC ABI mangles variables (?sv_paused@@3PAUdvar_s@@A), so each undefined
# global gets an alias to the other language's definition:
#   ?name@@3...  (C++ extern) -> _name           (defined in C)
#   _name        (C extern)   -> ?name@@3...     (defined in C++)
# Functions are not touched: they cross through the bridge's thunks, and only
# a data definition (llvm-nm's B, D, R, C) is ever an alias target.
#
#   cmake -DNM=llvm-nm -DOBJECT_DIR=<dir with .obj> -DOUT=<rsp> -P msvc_data_aliases.cmake
#   cmake -DUNDEFINED_FILE=<names> -DDEFINED_FILE=<"name type" lines> -DOUT=<rsp> -P msvc_data_aliases.cmake
# The rsp has one /alternatename option per line, for lld-link's @file.

if(OBJECT_DIR)
  file(GLOB_RECURSE objs "${OBJECT_DIR}/*.obj")
  # the executable's own objects and its OpenCoD2-Plus Server object libraries
  list(FILTER objs INCLUDE REGEX "/(cod2_win32|cod2_plus_[a-z_]+)\\.dir/")
  string(REPLACE ";" "\n" obj_lines "${objs}")
  file(WRITE "${OUT}.objs" "${obj_lines}\n")
  execute_process(COMMAND "${NM}" -j -g --undefined-only "@${OUT}.objs" OUTPUT_VARIABLE undefined_text RESULT_VARIABLE rc1)
  execute_process(COMMAND "${NM}" -g --defined-only --format=posix --no-sort "@${OUT}.objs"
                  OUTPUT_VARIABLE defined_text RESULT_VARIABLE rc2)
  if(rc1 OR rc2)
    message(FATAL_ERROR "${NM} failed on ${OBJECT_DIR}")
  endif()
else()
  file(READ "${UNDEFINED_FILE}" undefined_text)
  file(READ "${DEFINED_FILE}" defined_text)
endif()

string(REGEX REPLACE "[\r]" "" undefined_text "${undefined_text}")
string(REGEX REPLACE "[\r]" "" defined_text "${defined_text}")
# one symbol per list item; [ ] and ; never occur in COFF symbol names
string(REPLACE "\n" ";" undefined "${undefined_text}")
string(REPLACE "\n" ";" defined "${defined_text}")

foreach(line IN LISTS defined)
  # "name type ..." (llvm-nm --format=posix); a data type only
  if(NOT line MATCHES "^([^ ]+) [BDRC]( |$)")
    continue()
  endif()
  set(sym "${CMAKE_MATCH_1}")
  if(sym MATCHES "^\\?([A-Za-z_][A-Za-z0-9_]*)@@3")
    set("cxx_${CMAKE_MATCH_1}" "${sym}")
  elseif(sym MATCHES "^_([A-Za-z_][A-Za-z0-9_]*)$")
    set("c_${CMAKE_MATCH_1}" 1)
  endif()
endforeach()

set(out "")
foreach(sym IN LISTS undefined)
  if(sym MATCHES "^\\?([A-Za-z_][A-Za-z0-9_]*)@@3")
    if(DEFINED "c_${CMAKE_MATCH_1}")
      string(APPEND out "/alternatename:${sym}=_${CMAKE_MATCH_1}\n")
    endif()
  elseif(sym MATCHES "^__imp_")
    # an import thunk's pointer: the DLL's, never a global of ours
  elseif(sym MATCHES "^_([A-Za-z_][A-Za-z0-9_]*)$")
    if(DEFINED "cxx_${CMAKE_MATCH_1}")
      string(APPEND out "/alternatename:${sym}=${cxx_${CMAKE_MATCH_1}}\n")
    endif()
  endif()
endforeach()
# the same reference in several objects: one option each
string(REPLACE "\n" ";" out_lines "${out}")
list(REMOVE_DUPLICATES out_lines)
list(REMOVE_ITEM out_lines "")
string(REPLACE ";" "\n" out "${out_lines}")
file(WRITE "${OUT}" "${out}\n")
