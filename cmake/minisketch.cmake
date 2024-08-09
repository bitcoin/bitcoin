# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

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

add_library(minisketch_common INTERFACE)
target_compile_definitions(minisketch_common INTERFACE
  DISABLE_DEFAULT_FIELDS
  ENABLE_FIELD_32
)
if(MSVC)
  target_compile_options(minisketch_common INTERFACE
    /wd4060
    /wd4065
    /wd4146
    /wd4244
    /wd4267
  )
endif()

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
  target_link_libraries(minisketch_clmul
    PRIVATE
      core_interface
      minisketch_common
  )
  set_target_properties(minisketch_clmul PROPERTIES
    EXPORT_COMPILE_COMMANDS OFF
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

target_include_directories(minisketch
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/minisketch/include>
)

target_link_libraries(minisketch
  PRIVATE
    core_interface
    minisketch_common
    $<TARGET_NAME_IF_EXISTS:minisketch_clmul>
)

set_target_properties(minisketch PROPERTIES
  EXPORT_COMPILE_COMMANDS OFF
)
