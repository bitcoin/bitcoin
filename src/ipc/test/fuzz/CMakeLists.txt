# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

target_sources(fuzz PRIVATE ${PROJECT_SOURCE_DIR}/src/ipc/test/fuzz/ipc.cpp)
target_link_libraries(fuzz bitcoin_ipc_fuzz Libmultiprocess::multiprocess)
