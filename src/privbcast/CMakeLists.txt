# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# Private broadcast jobs (doc/design/private-broadcast-tool.md).
add_library(bitcoin_privbcast STATIC EXCLUDE_FROM_ALL
  assign.cpp
  plan.cpp
)
target_link_libraries(bitcoin_privbcast
  PUBLIC
    bitcoin_common
  PRIVATE
    core_interface
    bitcoin_util
    univalue
)
