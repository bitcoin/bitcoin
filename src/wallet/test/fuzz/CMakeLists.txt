# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

target_sources(fuzz
  PRIVATE
    bip39.cpp
    coincontrol.cpp
    coinselection.cpp
    crypter.cpp
    fees.cpp
    scriptpubkeyman.cpp
    spend.cpp
    wallet_bdb_parser.cpp
)
target_link_libraries(fuzz bitcoin_wallet)
