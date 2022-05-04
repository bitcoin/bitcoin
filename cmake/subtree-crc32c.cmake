# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Check for __builtin_prefetch support in the compiler.
check_cxx_source_compiles("
  int main() {
    char data = 0;
    const char* address = &data;
    __builtin_prefetch(address, 0, 0);
    return 0;
  }"
  HAVE_BUILTIN_PREFETCH
)

# Check for _mm_prefetch support in the compiler.
check_cxx_source_compiles("
  #include <xmmintrin.h>

  int main() {
    char data = 0;
    const char* address = &data;
    _mm_prefetch(address, _MM_HINT_NTA);
    return 0;
  }"
  HAVE_MM_PREFETCH
)

# Check for strong getauxval() support in the system headers.
check_cxx_source_compiles("
  #include <sys/auxv.h>

  int main() {
    getauxval(AT_HWCAP);
    return 0;
  }"
  HAVE_STRONG_GETAUXVAL
)

# Check for SSE4.2 support in the compiler.
set(TEMP_CMAKE_REQURED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(SSE42_CXXFLAGS "-msse4.2")
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${SSE42_CXXFLAGS}")
check_cxx_source_compiles("
  #include <stdint.h>
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
  }"
  HAVE_SSE42
)
set(CMAKE_REQUIRED_FLAGS ${TEMP_CMAKE_REQURED_FLAGS})

# Check for ARMv8 w/ CRC and CRYPTO extensions support in the compiler.
set(TEMP_CMAKE_REQURED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(ARM_CRC_CXXFLAGS "-march=armv8-a+crc+crypto")
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${ARM_CRC_CXXFLAGS}")
check_cxx_source_compiles("
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
  }"
  HAVE_ARM64_CRC32C
)
set(CMAKE_REQUIRED_FLAGS ${TEMP_CMAKE_REQURED_FLAGS})

# A home for a few CRC32C implementations under an umbrella that dispatches
# to a suitable implementation based on the host computer's hardware capabilities.
# Subtree at https://github.com/bitcoin-core/crc32c-subtree
# Upstream at https://github.com/google/crc32c
add_library(crc32c STATIC EXCLUDE_FROM_ALL)
target_sources(crc32c
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/crc32c/src/crc32c.cc
    ${CMAKE_SOURCE_DIR}/src/crc32c/src/crc32c_portable.cc
)
target_include_directories(crc32c
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/crc32c/include>
)

target_compile_definitions(crc32c
  PRIVATE
    BYTE_ORDER_BIG_ENDIAN=${WORDS_BIGENDIAN}
)

if(HAVE_BUILTIN_PREFETCH)
  target_compile_definitions(crc32c PRIVATE HAVE_BUILTIN_PREFETCH)
endif()
if(HAVE_MM_PREFETCH)
  target_compile_definitions(crc32c PRIVATE HAVE_MM_PREFETCH)
endif()
if(HAVE_STRONG_GETAUXVAL)
  target_compile_definitions(crc32c PRIVATE HAVE_STRONG_GETAUXVAL)
endif()

if(HAVE_SSE42)
  # SSE4.2 code is built separately, so we don't accidentally compile unsupported
  # instructions into code that gets run without SSE4.2 support.
  add_library(crc32c_sse42 OBJECT EXCLUDE_FROM_ALL)
  target_sources(crc32c_sse42 PRIVATE ${CMAKE_SOURCE_DIR}/src/crc32c/src/crc32c_sse42.cc)
  target_compile_definitions(crc32c_sse42 PRIVATE HAVE_SSE42)
  target_compile_options(crc32c_sse42 PRIVATE ${SSE42_CXXFLAGS})
  target_sources(crc32c PRIVATE $<TARGET_OBJECTS:crc32c_sse42>)
  target_compile_definitions(crc32c PRIVATE HAVE_SSE42)
endif()

if(HAVE_ARM64_CRC32C)
  # ARM64 CRC32C code is built separately, so we don't accidentally compile
  # unsupported instructions into code that gets run without ARM32 support.
  add_library(crc32c_arm64 OBJECT EXCLUDE_FROM_ALL)
  target_sources(crc32c_arm64 PRIVATE ${CMAKE_SOURCE_DIR}/src/crc32c/src/crc32c_arm64.cc)
  target_compile_definitions(crc32c_arm64 PRIVATE HAVE_ARM64_CRC32C)
  target_compile_options(crc32c_arm64 PRIVATE ${ARM_CRC_CXXFLAGS})
  target_sources(crc32c PRIVATE $<TARGET_OBJECTS:crc32c_arm64>)
  target_compile_definitions(crc32c PRIVATE HAVE_ARM64_CRC32C)
endif()
