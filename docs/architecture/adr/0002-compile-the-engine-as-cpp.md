# 0002. Compile the engine as C++

- Status: Accepted
- Date: 2026-10-18

## Context

CoD2rev-Plus is C++ and the bridge calls it by its C++ names; the engine's C let mismatched declarations, missing returns and implicit conversions pass unnoticed.

## Decision

Build every engine C source as C++ (gnu++17) through generated `extern "C"` wrappers (`COD2_CXX_ALL`, ON), keeping C names for the blobs and aliases. Blobs and third-party libraries stay C.

## Consequences

- The C++ front end checks every header and layout pin (`test/cxx/header_check.cpp`).
- Functions that could end without a return must return; glibc 2.38+ C23 redirects are turned off (`src/headers/glibc_compat.h`).
- `COD2_CXX_ALL=OFF` still builds C.
