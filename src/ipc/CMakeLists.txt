# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(bitcoin_ipc STATIC EXCLUDE_FROM_ALL
  capnp/chain.cpp
  capnp/common.cpp
  capnp/mining.cpp
  capnp/protocol.cpp
  interfaces.cpp
  process.cpp
)

target_capnp_sources(bitcoin_ipc ${PROJECT_SOURCE_DIR}
  capnp/chain.capnp
  capnp/common.capnp
  capnp/echo.capnp
  capnp/handler.capnp
  capnp/init.capnp
  capnp/mining.capnp
)

if (ENABLE_WALLET)
  target_sources(bitcoin_ipc PRIVATE capnp/wallet.cpp)
  target_capnp_sources(bitcoin_ipc ${PROJECT_SOURCE_DIR} capnp/wallet.capnp)
else()
  target_capnp_sources(bitcoin_ipc ${PROJECT_SOURCE_DIR} ONLY_CAPNP capnp/wallet.capnp)
endif()

target_link_libraries(bitcoin_ipc
  PRIVATE
    core_interface
    univalue
)

configure_file(.clang-tidy.in .clang-tidy USE_SOURCE_PERMISSIONS COPYONLY)
