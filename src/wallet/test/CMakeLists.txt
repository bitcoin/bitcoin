# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# Do not use generator expressions in test sources because the
# SOURCES property is processed to gather test suite macros.
target_sources(test_bitcoin
  PRIVATE
    init_test_fixture.cpp
    wallet_test_fixture.cpp
    coinselector_tests.cpp
    feebumper_tests.cpp
    group_outputs_tests.cpp
    init_tests.cpp
    ismine_tests.cpp
    psbt_wallet_tests.cpp
    rpc_util_tests.cpp
    scriptpubkeyman_tests.cpp
    spend_tests.cpp
    wallet_crypto_tests.cpp
    wallet_tests.cpp
    wallet_transaction_tests.cpp
    walletdb_tests.cpp
    walletload_tests.cpp
)
if(USE_BDB)
  target_sources(test_bitcoin
    PRIVATE
      db_tests.cpp
  )
endif()
target_link_libraries(test_bitcoin bitcoin_wallet)
