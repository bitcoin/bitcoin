# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

function(generate_setup_nsi)
  set(abs_top_srcdir ${PROJECT_SOURCE_DIR})
  set(abs_top_builddir ${PROJECT_BINARY_DIR})
  set(PACKAGE_VERSION ${PROJECT_VERSION})
  set(CLIENT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(CLIENT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(CLIENT_VERSION_BUILD ${PROJECT_VERSION_PATCH})
  set(PACKAGE_URL ${PROJECT_HOMEPAGE_URL})
  set(PACKAGE_TARNAME "bitcoin")
  set(BITCOIN_GUI_NAME "bitcoin-qt")
  set(BITCOIN_DAEMON_NAME "bitcoind")
  set(BITCOIN_CLI_NAME "bitcoin-cli")
  set(BITCOIN_TX_NAME "bitcoin-tx")
  set(BITCOIN_WALLET_TOOL_NAME "bitcoin-wallet")
  set(BITCOIN_TEST_NAME "test_bitcoin")
  set(EXEEXT ${CMAKE_EXECUTABLE_SUFFIX})
  configure_file(${PROJECT_SOURCE_DIR}/share/setup.nsi.in ${PROJECT_BINARY_DIR}/share/setup.nsi @ONLY)
endfunction()
