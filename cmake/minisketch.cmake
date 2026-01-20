# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(CheckSourceCompilesWithFlags)

# Check for clmul instructions support.
if(MSVC)
  set(CLMUL_CXXFLAGS "")
else()
  set(CLMUL_CXXFLAGS -mpclmul)
endif()
check_cxx_source_compiles_with_flags("
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
  CXXFLAGS ${CLMUL_CXXFLAGS}
)

add_library(minisketch_common INTERFACE)
if(MSVC)
  target_compile_options(minisketch_common INTERFACE
    /wd4060
    /wd4065
    /wd4146
    /wd4244
    /wd4267
  )
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

target_compile_definitions(minisketch
  PRIVATE
    DISABLE_DEFAULT_FIELDS
    ENABLE_FIELD_32
)

target_include_directories(minisketch
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/minisketch/include>
)

target_link_libraries(minisketch
  PRIVATE
    core_interface
    minisketch_common
)

set_target_properties(minisketch PROPERTIES
  EXPORT_COMPILE_COMMANDS OFF
)

if(HAVE_CLMUL)
  set(_minisketch_clmul_src
      ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_1byte.cpp
      ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_2bytes.cpp
      ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_3bytes.cpp
      ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_4bytes.cpp
      ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_5bytes.cpp
      ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_6bytes.cpp
      ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_7bytes.cpp
      ${PROJECT_SOURCE_DIR}/src/minisketch/src/fields/clmul_8bytes.cpp
  )
  target_sources(minisketch PRIVATE ${_minisketch_clmul_src})
  set_property(SOURCE ${_minisketch_clmul_src} PROPERTY COMPILE_OPTIONS ${CLMUL_CXXFLAGS})
  target_compile_definitions(minisketch PRIVATE HAVE_CLMUL)
  unset(_minisketch_clmul_src)
endif()
