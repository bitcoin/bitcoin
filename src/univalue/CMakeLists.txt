# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

add_library(univalue STATIC EXCLUDE_FROM_ALL
  lib/univalue.cpp
  lib/univalue_get.cpp
  lib/univalue_read.cpp
  lib/univalue_write.cpp
)

target_include_directories(univalue
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
