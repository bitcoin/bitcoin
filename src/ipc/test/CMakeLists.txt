# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

target_link_libraries(test_bitcoin bitcoin_ipc_test bitcoin_ipc)

add_boost_test(${CMAKE_CURRENT_SOURCE_DIR}/ipc_tests.cpp)
