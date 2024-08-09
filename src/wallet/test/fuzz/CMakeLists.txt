# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

target_sources(fuzz
  PRIVATE
    coincontrol.cpp
    coinselection.cpp
    fees.cpp
    $<$<BOOL:${USE_SQLITE}>:${CMAKE_CURRENT_LIST_DIR}/notifications.cpp>
    parse_iso8601.cpp
    $<$<BOOL:${USE_SQLITE}>:${CMAKE_CURRENT_LIST_DIR}/scriptpubkeyman.cpp>
    wallet_bdb_parser.cpp
)
target_link_libraries(fuzz bitcoin_wallet)
