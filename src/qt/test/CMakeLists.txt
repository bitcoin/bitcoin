# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

set(CMAKE_AUTOMOC_MOC_OPTIONS "-p${CMAKE_CURRENT_SOURCE_DIR}")

add_library(test_bitcoinqt STATIC EXCLUDE_FROM_ALL
  apptests.cpp
  optiontests.cpp
  rpcnestedtests.cpp
  uritests.cpp
  util.cpp
)

target_compile_definitions(test_bitcoinqt
  PUBLIC
    QT_NO_KEYWORDS
    QT_USE_QSTRINGBUILDER
)

target_link_libraries(test_bitcoinqt
  core_interface
  Qt6::Test
  Qt6::Widgets
  bitcoin_common
  univalue
)

if(ENABLE_WALLET)
  target_sources(test_bitcoinqt
    PRIVATE
      addressbooktests.cpp
      wallettests.cpp
      ../../wallet/test/wallet_test_fixture.cpp
  )
  target_link_libraries(test_bitcoinqt
    Boost::headers
  )
endif()

add_executable(test_bitcoin-qt
  test_main.cpp
  ../../init/bitcoin-qt.cpp
)

target_link_libraries(test_bitcoin-qt
  core_interface
  test_bitcoinqt
  bitcoinqt
  test_util
  bitcoin_node
  Qt6::Test
)

import_plugins(test_bitcoin-qt)

add_test(NAME test_bitcoin-qt
  COMMAND test_bitcoin-qt
)
if(WIN32 AND VCPKG_TARGET_TRIPLET)
  # On Windows, vcpkg configures Qt with `-opengl dynamic`, which makes
  # the "minimal" platform plugin unusable due to internal Qt bugs.
  set_tests_properties(test_bitcoin-qt PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=windows"
  )
endif()

install_binary_component(test_bitcoin-qt)
