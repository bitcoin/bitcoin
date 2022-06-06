# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

include(ExternalProject)

# Optimized C library for ECDSA signatures and secret/public key operations on curve secp256k1.
# See https://github.com/bitcoin-core/secp256k1
add_library(secp256k1 STATIC IMPORTED GLOBAL)

target_include_directories(secp256k1
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/secp256k1/include>
)

set(libsecp256k1_configure_cmd "${CMAKE_SOURCE_DIR}/src/secp256k1/configure")
list(APPEND libsecp256k1_configure_cmd "--disable-shared")
list(APPEND libsecp256k1_configure_cmd "--with-pic")
list(APPEND libsecp256k1_configure_cmd "--enable-benchmark=no")
list(APPEND libsecp256k1_configure_cmd "--enable-module-recovery")
list(APPEND libsecp256k1_configure_cmd "--enable-module-schnorrsig")
ExternalProject_Add(libsecp256k1
  PREFIX src/secp256k1
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/secp256k1
  BINARY_DIR ${CMAKE_BINARY_DIR}/src/secp256k1
  CONFIGURE_COMMAND cd ${CMAKE_SOURCE_DIR}/src/secp256k1 && ./autogen.sh && cd ${CMAKE_BINARY_DIR}/src/secp256k1 && ${libsecp256k1_configure_cmd}
  BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} libsecp256k1.la
  INSTALL_COMMAND true
  EXCLUDE_FROM_ALL ON
)

add_dependencies(secp256k1 libsecp256k1)
set_target_properties(secp256k1 PROPERTIES
  IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/src/secp256k1/.libs/libsecp256k1.a
)
