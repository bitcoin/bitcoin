# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# This file is part of the transition from Autotools to CMake. Once CMake
# support has been merged we should switch to using the upstream CMake
# buildsystem.

enable_language(C)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_EXTENSIONS OFF)

if(ASM)
  include(CheckCSourceCompiles)
  check_c_source_compiles("
    #include <stdint.h>

    int main()
    {
      uint64_t a = 11, tmp;
      __asm__ __volatile__(\"movq $0x100000000,%1; mulq %%rsi\" : \"+a\"(a) : \"S\"(tmp) : \"cc\", \"%rdx\");
    }
    " HAVE_64BIT_ASM
  )
endif()

add_library(secp256k1 STATIC EXCLUDE_FROM_ALL
  ${PROJECT_SOURCE_DIR}/src/secp256k1/src/secp256k1.c
  ${PROJECT_SOURCE_DIR}/src/secp256k1/src/precomputed_ecmult.c
  ${PROJECT_SOURCE_DIR}/src/secp256k1/src/precomputed_ecmult_gen.c
)

target_compile_definitions(secp256k1
  PRIVATE
    ECMULT_GEN_PREC_BITS=4
    ECMULT_WINDOW_SIZE=15
    ENABLE_MODULE_RECOVERY
    ENABLE_MODULE_SCHNORRSIG
    ENABLE_MODULE_EXTRAKEYS
    ENABLE_MODULE_ELLSWIFT
    $<$<BOOL:${HAVE_64BIT_ASM}>:USE_ASM_X86_64=1>
)

target_include_directories(secp256k1
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/secp256k1/include>
)

target_compile_options(secp256k1
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/wd4146 /wd4334>
)
