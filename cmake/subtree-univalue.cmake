# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# A universal value class, with JSON encoding and decoding.
# Subtree at https://github.com/bitcoin-core/univalue-subtree
# Deviates from upstream https://github.com/jgarzik/univalue
add_library(univalue STATIC EXCLUDE_FROM_ALL)

target_sources(univalue
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/univalue/lib/univalue.cpp
    ${CMAKE_SOURCE_DIR}/src/univalue/lib/univalue_get.cpp
    ${CMAKE_SOURCE_DIR}/src/univalue/lib/univalue_read.cpp
    ${CMAKE_SOURCE_DIR}/src/univalue/lib/univalue_write.cpp
)

target_include_directories(univalue
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/univalue/include>
)
