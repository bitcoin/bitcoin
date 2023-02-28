# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Check for clmul instructions support.
if(MSVC)
  set(CLMUL_CXXFLAGS)
else()
  set(CLMUL_CXXFLAGS -mpclmul)
endif()
check_cxx_source_compiles_with_flags("${CLMUL_CXXFLAGS}" "
  #include <immintrin.h>
  #include <cstdint>

  int main()
  {
    __m128i a = _mm_cvtsi64_si128((uint64_t)7);
    __m128i b = _mm_clmulepi64_si128(a, a, 37);
    __m128i c = _mm_srli_epi64(b, 41);
    __m128i d = _mm_xor_si128(b, c);
    uint64_t e = _mm_cvtsi128_si64(d);
    return e == 0;
  }
  " HAVE_CLMUL
)

add_library(minisketch_defs INTERFACE)
target_compile_definitions(minisketch_defs INTERFACE
  DISABLE_DEFAULT_FIELDS
  ENABLE_FIELD_32
  $<$<AND:$<BOOL:${HAVE_BUILTIN_CLZL}>,$<BOOL:${HAVE_BUILTIN_CLZLL}>>:HAVE_CLZ>
)

if(HAVE_CLMUL)
  add_library(minisketch_clmul OBJECT EXCLUDE_FROM_ALL
    ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_1byte.cpp
    ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_2bytes.cpp
    ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_3bytes.cpp
    ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_4bytes.cpp
    ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_5bytes.cpp
    ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_6bytes.cpp
    ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_7bytes.cpp
    ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_8bytes.cpp
  )
  target_compile_definitions(minisketch_clmul PUBLIC HAVE_CLMUL)
  target_compile_options(minisketch_clmul PRIVATE ${CLMUL_CXXFLAGS})
  target_link_libraries(minisketch_clmul PRIVATE minisketch_defs)
endif()

add_library(minisketch STATIC EXCLUDE_FROM_ALL
  ${PROJECT_SOURCE_DIR}/src/minisketch/src/minisketch.cpp
  ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/generic_1byte.cpp
  ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/generic_2bytes.cpp
  ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/generic_3bytes.cpp
  ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/generic_4bytes.cpp
  ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/generic_5bytes.cpp
  ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/generic_6bytes.cpp
  ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/generic_7bytes.cpp
  ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/generic_8bytes.cpp
)

target_include_directories(minisketch
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/minisketch/include>
)

target_link_libraries(minisketch
  PRIVATE
    minisketch_defs
    $<TARGET_NAME_IF_EXISTS:minisketch_clmul>
)
