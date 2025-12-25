# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# This file is part of the transition from Autotools to CMake. Once CMake
# support has been merged we should switch to using the upstream CMake
# buildsystem.

include(CheckCXXSourceCompiles)
include(CheckSourceCompilesWithFlags)

# Check for __builtin_prefetch support in the compiler.
check_cxx_source_compiles("
  int main() {
    char data = 0;
    const char* address = &data;
    __builtin_prefetch(address, 0, 0);
    return 0;
  }
  " HAVE_BUILTIN_PREFETCH
)

# Check for _mm_prefetch support in the compiler.
check_cxx_source_compiles("
  #if defined(_MSC_VER)
  #include <intrin.h>
  #else
  #include <xmmintrin.h>
  #endif

  int main() {
    char data = 0;
    const char* address = &data;
    _mm_prefetch(address, _MM_HINT_NTA);
    return 0;
  }
  " HAVE_MM_PREFETCH
)

# Check for SSE4.2 support in the compiler.
if(MSVC)
  set(SSE42_CXXFLAGS /arch:AVX)
else()
  set(SSE42_CXXFLAGS -msse4.2)
endif()
check_cxx_source_compiles_with_flags("
  #include <cstdint>
  #if defined(_MSC_VER)
  #include <intrin.h>
  #elif defined(__GNUC__) && defined(__SSE4_2__)
  #include <nmmintrin.h>
  #endif

  int main() {
    uint64_t l = 0;
    l = _mm_crc32_u8(l, 0);
    l = _mm_crc32_u32(l, 0);
    l = _mm_crc32_u64(l, 0);
    return l;
  }
  " HAVE_SSE42
  CXXFLAGS ${SSE42_CXXFLAGS}
)

# Check for ARMv8 w/ CRC and CRYPTO extensions support in the compiler.
set(ARM64_CRC_CXXFLAGS -march=armv8-a+crc+crypto)
check_cxx_source_compiles_with_flags("
  #include <arm_acle.h>
  #include <arm_neon.h>

  int main() {
  #ifdef __aarch64__
    __crc32cb(0, 0); __crc32ch(0, 0); __crc32cw(0, 0); __crc32cd(0, 0);
    vmull_p64(0, 0);
  #else
  #error crc32c library does not support hardware acceleration on 32-bit ARM
  #endif
    return 0;
  }
  " HAVE_ARM64_CRC32C
  CXXFLAGS ${ARM64_CRC_CXXFLAGS}
)

add_library(crc32c STATIC EXCLUDE_FROM_ALL
  ${PROJECT_SOURCE_DIR}/src/crc32c/src/crc32c.cc
  ${PROJECT_SOURCE_DIR}/src/crc32c/src/crc32c_portable.cc
)
target_compile_definitions(crc32c PRIVATE
  HAVE_BUILTIN_PREFETCH=$<BOOL:${HAVE_BUILTIN_PREFETCH}>
  HAVE_MM_PREFETCH=$<BOOL:${HAVE_MM_PREFETCH}>
  HAVE_STRONG_GETAUXVAL=$<BOOL:${HAVE_STRONG_GETAUXVAL}>
  BYTE_ORDER_BIG_ENDIAN=$<STREQUAL:${CMAKE_CXX_BYTE_ORDER},BIG_ENDIAN>
  HAVE_SSE42=$<BOOL:${HAVE_SSE42}>
  HAVE_ARM64_CRC32C=$<BOOL:${HAVE_ARM64_CRC32C}>
)
target_include_directories(crc32c
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/crc32c/include>
)
target_link_libraries(crc32c PRIVATE core_interface)
set_target_properties(crc32c PROPERTIES EXPORT_COMPILE_COMMANDS OFF)

if(HAVE_SSE42)
  set(_crc32_src ${PROJECT_SOURCE_DIR}/src/crc32c/src/crc32c_sse42.cc)
  target_sources(crc32c PRIVATE ${_crc32_src})
  set_property(SOURCE ${_crc32_src} PROPERTY COMPILE_OPTIONS ${SSE42_CXXFLAGS})
endif()

if(HAVE_ARM64_CRC32C)
  set(_crc32_src ${PROJECT_SOURCE_DIR}/src/crc32c/src/crc32c_arm64.cc)
  target_sources(crc32c PRIVATE ${_crc32_src})
  set_property(SOURCE ${_crc32_src} PROPERTY COMPILE_OPTIONS ${ARM64_CRC_CXXFLAGS})
endif()

unset(_crc32_src)
