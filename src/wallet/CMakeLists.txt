# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

option(USE_SQLITE "Enable SQLite wallet support" ON)
option(USE_BDB "Enable Berkeley DB wallet support" OFF)

if(USE_BDB)
  message(WARNING "Berkeley DB wallet support has not been implemented yet, thus it will be disabled.")
  set(USE_BDB OFF)
endif()

if(NOT USE_SQLITE AND NOT USE_BDB)
  message(FATAL_ERROR "None of USE_SQLITE and USE_BDB are enabled")
endif()

# Wallet functionality used by bitcoind and bitcoin-wallet executables.
add_library(bitcoin_wallet STATIC EXCLUDE_FROM_ALL)
target_sources(bitcoin_wallet
  PRIVATE
    coincontrol.cpp
    coinselection.cpp
    context.cpp
    crypter.cpp
    db.cpp
    dump.cpp
    external_signer_scriptpubkeyman.cpp
    feebumper.cpp
    fees.cpp
    interfaces.cpp
    load.cpp
    receive.cpp
    rpc/addresses.cpp
    rpc/backup.cpp
    rpc/coins.cpp
    rpc/encrypt.cpp
    rpc/spend.cpp
    rpc/signmessage.cpp
    rpc/transactions.cpp
    rpc/util.cpp
    rpc/wallet.cpp
    scriptpubkeyman.cpp
    spend.cpp
    transaction.cpp
    wallet.cpp
    walletdb.cpp
    walletutil.cpp
)
target_include_directories(bitcoin_wallet
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>/src
)
target_link_libraries(bitcoin_wallet
  PRIVATE
    univalue
    Boost::headers
)
if(USE_SQLITE)
  pkg_check_modules(sqlite REQUIRED sqlite3>=3.7.17 IMPORTED_TARGET)
  target_sources(bitcoin_wallet PRIVATE sqlite.cpp)
  target_compile_definitions(bitcoin_wallet PRIVATE USE_SQLITE)
  target_link_libraries(bitcoin_wallet PRIVATE PkgConfig::sqlite)
endif()
