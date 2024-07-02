# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(bitcoin_ipc STATIC EXCLUDE_FROM_ALL
  capnp/protocol.cpp
  interfaces.cpp
  process.cpp
)

target_capnp_sources(bitcoin_ipc ${CMAKE_SOURCE_DIR}
  capnp/echo.capnp capnp/init.capnp
)

target_link_libraries(bitcoin_ipc
  PRIVATE
    core_interface
)
