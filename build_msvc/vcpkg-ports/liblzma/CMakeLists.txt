cmake_minimum_required(VERSION 3.0)
project(LibLZMA)

set(INSTALL_BIN_DIR      "bin"                CACHE PATH   "Path where exe and dll will be installed")
set(INSTALL_LIB_DIR      "lib"                CACHE PATH   "Path where lib will be installed")
set(INSTALL_INCLUDE_DIR  "include"            CACHE PATH   "Path where headers will be installed")
set(INSTALL_CMAKE_DIR    "share/liblzma"      CACHE PATH   "Path where cmake configs will be installed")
set(CMAKE_DEBUG_POSTFIX  "d"                  CACHE STRING "Append a suffix to debug libraries")

if(NOT WIN32)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.h
    "
#define TUKLIB_SYMBOL_PREFIX lzma_
#define ASSUME_RAM 128
#define HAVE_CHECK_CRC32 1
#define HAVE_CHECK_CRC64 1
#define HAVE_CHECK_SHA256 1
#define HAVE_DECODERS 1
#define HAVE_DECODER_ARM 1
#define HAVE_DECODER_ARMTHUMB 1
#define HAVE_DECODER_DELTA 1
#define HAVE_DECODER_IA64 1
#define HAVE_DECODER_LZMA1 1
#define HAVE_DECODER_LZMA2 1
#define HAVE_DECODER_POWERPC 1
#define HAVE_DECODER_SPARC 1
#define HAVE_DECODER_X86 1
#define HAVE_ENCODERS 1
#define HAVE_ENCODER_ARM 1
#define HAVE_ENCODER_ARMTHUMB 1
#define HAVE_ENCODER_DELTA 1
#define HAVE_ENCODER_IA64 1
#define HAVE_ENCODER_LZMA1 1
#define HAVE_ENCODER_LZMA2 1
#define HAVE_ENCODER_POWERPC 1
#define HAVE_ENCODER_SPARC 1
#define HAVE_ENCODER_X86 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MF_BT2 1
#define HAVE_MF_BT3 1
#define HAVE_MF_BT4 1
#define HAVE_MF_HC3 1
#define HAVE_MF_HC4 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_VISIBILITY 1
#define MYTHREAD_POSIX 1
#define PACKAGE_NAME \"XZ Utils\"
#define PACKAGE_URL \"http://tukaani.org/xz/\"
#define SIZEOF_SIZE_T (sizeof(size_t))
")
endif()

add_library(LibLZMA
    src/common/tuklib_cpucores.c
    src/common/tuklib_physmem.c
    src/liblzma/check/check.c
    src/liblzma/check/crc32_fast.c
    src/liblzma/check/crc32_table.c
    src/liblzma/check/crc64_fast.c
    src/liblzma/check/crc64_table.c
    src/liblzma/check/sha256.c
    src/liblzma/common/alone_decoder.c
    src/liblzma/common/alone_encoder.c
    src/liblzma/common/auto_decoder.c
    src/liblzma/common/block_buffer_decoder.c
    src/liblzma/common/block_buffer_encoder.c
    src/liblzma/common/block_decoder.c
    src/liblzma/common/block_encoder.c
    src/liblzma/common/block_header_decoder.c
    src/liblzma/common/block_header_encoder.c
    src/liblzma/common/block_util.c
    src/liblzma/common/common.c
    src/liblzma/common/easy_buffer_encoder.c
    src/liblzma/common/easy_decoder_memusage.c
    src/liblzma/common/easy_encoder.c
    src/liblzma/common/easy_encoder_memusage.c
    src/liblzma/common/easy_preset.c
    src/liblzma/common/filter_buffer_decoder.c
    src/liblzma/common/filter_buffer_encoder.c
    src/liblzma/common/filter_common.c
    src/liblzma/common/filter_decoder.c
    src/liblzma/common/filter_encoder.c
    src/liblzma/common/filter_flags_decoder.c
    src/liblzma/common/filter_flags_encoder.c
    src/liblzma/common/hardware_cputhreads.c
    src/liblzma/common/hardware_physmem.c
    src/liblzma/common/index.c
    src/liblzma/common/index_decoder.c
    src/liblzma/common/index_encoder.c
    src/liblzma/common/index_hash.c
    src/liblzma/common/outqueue.c
    src/liblzma/common/stream_buffer_decoder.c
    src/liblzma/common/stream_buffer_encoder.c
    src/liblzma/common/stream_decoder.c
    src/liblzma/common/stream_encoder.c
    src/liblzma/common/stream_encoder_mt.c
    src/liblzma/common/stream_flags_common.c
    src/liblzma/common/stream_flags_decoder.c
    src/liblzma/common/stream_flags_encoder.c
    src/liblzma/common/vli_decoder.c
    src/liblzma/common/vli_encoder.c
    src/liblzma/common/vli_size.c
    src/liblzma/delta/delta_common.c
    src/liblzma/delta/delta_decoder.c
    src/liblzma/delta/delta_encoder.c
    src/liblzma/lzma/fastpos_table.c
    src/liblzma/lzma/lzma2_decoder.c
    src/liblzma/lzma/lzma2_encoder.c
    src/liblzma/lzma/lzma_decoder.c
    src/liblzma/lzma/lzma_encoder.c
    src/liblzma/lzma/lzma_encoder_optimum_fast.c
    src/liblzma/lzma/lzma_encoder_optimum_normal.c
    src/liblzma/lzma/lzma_encoder_presets.c
    src/liblzma/lz/lz_decoder.c
    src/liblzma/lz/lz_encoder.c
    src/liblzma/lz/lz_encoder_mf.c
    src/liblzma/rangecoder/price_table.c
    src/liblzma/simple/arm.c
    src/liblzma/simple/armthumb.c
    src/liblzma/simple/ia64.c
    src/liblzma/simple/powerpc.c
    src/liblzma/simple/simple_coder.c
    src/liblzma/simple/simple_decoder.c
    src/liblzma/simple/simple_encoder.c
    src/liblzma/simple/sparc.c
    src/liblzma/simple/x86.c
)

if(MSVC)
    target_include_directories(LibLZMA PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/windows/vs2017>)
else()
    target_compile_definitions(LibLZMA PRIVATE -DHAVE_VISIBILITY=1)
endif()

if(BUILD_SHARED_LIBS)
    target_compile_definitions(LibLZMA PRIVATE -DDLL_EXPORT)
    target_compile_definitions(LibLZMA PUBLIC -DLIBLZMADLL_EXPORTS)
else()
    target_compile_definitions(LibLZMA PUBLIC -DLZMA_API_STATIC)
endif()
target_compile_definitions(LibLZMA PRIVATE -DHAVE_CONFIG_H)

target_include_directories(LibLZMA PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/liblzma/common>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/common>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/liblzma/check>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/liblzma/delta>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/liblzma/lz>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/liblzma/rangecoder>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/liblzma/simple>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/liblzma/lzma>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/liblzma/api>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
)

set_target_properties(LibLZMA PROPERTIES OUTPUT_NAME "lzma")

set_target_properties(LibLZMA PROPERTIES PUBLIC_HEADER
"${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/base.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/bcj.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/block.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/check.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/container.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/delta.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/filter.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/hardware.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/index.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/index_hash.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/lzma12.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/stream_flags.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/version.h;${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma/vli.h"
)

install(TARGETS LibLZMA EXPORT LibLZMATargets
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
    ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
    PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/lzma"
)

install(FILES "${PROJECT_SOURCE_DIR}/src/liblzma/api/lzma.h"
    DESTINATION "${INSTALL_INCLUDE_DIR}"
)

install(EXPORT LibLZMATargets
    FILE LibLZMAConfig.cmake
    NAMESPACE LibLZMA::
    DESTINATION "${INSTALL_CMAKE_DIR}"
)
