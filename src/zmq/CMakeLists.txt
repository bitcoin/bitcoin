# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(bitcoin_zmq STATIC EXCLUDE_FROM_ALL
  zmqabstractnotifier.cpp
  zmqnotificationinterface.cpp
  zmqpublishnotifier.cpp
  zmqrpc.cpp
  zmqutil.cpp
)
target_compile_definitions(bitcoin_zmq
  INTERFACE
    ENABLE_ZMQ=1
  PRIVATE
    $<$<AND:$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:GNU>>:ZMQ_STATIC>
)
target_link_libraries(bitcoin_zmq
  PRIVATE
    core_interface
    univalue
    $<TARGET_NAME_IF_EXISTS:libzmq>
    $<TARGET_NAME_IF_EXISTS:PkgConfig::libzmq>
)
