# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

add_library(bitcoinconsensus SHARED)
target_sources(bitcoinconsensus
  PRIVATE
    bitcoinconsensus.cpp
    ../support/cleanse.cpp
)
target_compile_definitions(bitcoinconsensus
  PRIVATE
    BUILD_BITCOIN_INTERNAL HAVE_DEFAULT_VISIBILITY_ATTRIBUTE
)
target_link_libraries(bitcoinconsensus
  PRIVATE
    bitcoin_crypto
    bitcoin_consensus
)
