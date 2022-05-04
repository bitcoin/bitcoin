# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Check for clmul instructions support.
set(TEMP_CMAKE_REQURED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CLMUL_CXXFLAGS "-mpclmul")
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CLMUL_CXXFLAGS}")
check_cxx_source_compiles("
  #include <stdint.h>
  #include <x86intrin.h>

  int main() {
    __m128i a = _mm_cvtsi64_si128((uint64_t)7);
    __m128i b = _mm_clmulepi64_si128(a, a, 37);
    __m128i c = _mm_srli_epi64(b, 41);
    __m128i d = _mm_xor_si128(b, c);
    uint64_t e = _mm_cvtsi128_si64(d);
    return e == 0;
  }"
  HAVE_CLMUL
)
set(CMAKE_REQUIRED_FLAGS ${TEMP_CMAKE_REQURED_FLAGS})

# Check for working clz builtins.
check_cxx_source_compiles("
  int main() {
    unsigned a = __builtin_clz(1);
    unsigned long b = __builtin_clzl(1);
    unsigned long long c = __builtin_clzll(1);
    return 0;
  }"
  HAVE_CLZ
)

# A library for BCH-based set reconciliation.
# Upstream at https://github.com/sipa/minisketch
add_library(minisketch STATIC EXCLUDE_FROM_ALL)
target_sources(minisketch
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/minisketch/src/minisketch.cpp
    ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/generic_common_impl.h
    ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/generic_1byte.cpp
    ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/generic_2bytes.cpp
    ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/generic_3bytes.cpp
    ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/generic_4bytes.cpp
    ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/generic_5bytes.cpp
    ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/generic_6bytes.cpp
    ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/generic_7bytes.cpp
    ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/generic_8bytes.cpp
)
target_include_directories(minisketch
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/minisketch/include>
)
target_compile_definitions(minisketch
  PRIVATE
    DISABLE_DEFAULT_FIELDS
    ENABLE_FIELD_32
)

if(HAVE_CLMUL)
  target_sources(minisketch
    PRIVATE
      ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/clmul_1byte.cpp
      ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/clmul_2bytes.cpp
      ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/clmul_3bytes.cpp
      ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/clmul_4bytes.cpp
      ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/clmul_5bytes.cpp
      ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/clmul_6bytes.cpp
      ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/clmul_7bytes.cpp
      ${CMAKE_SOURCE_DIR}/src/minisketch/src/fields/clmul_8bytes.cpp
  )
  target_compile_definitions(minisketch PRIVATE HAVE_CLMUL)
  target_compile_options(minisketch PRIVATE ${CLMUL_CXXFLAGS})
endif()

if(HAVE_CLZ)
  target_compile_definitions(minisketch PRIVATE HAVE_CLZ)
endif()
