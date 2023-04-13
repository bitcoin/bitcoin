# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

add_library(bitcoin_zmq STATIC
  zmqabstractnotifier.cpp
  zmqnotificationinterface.cpp
  zmqpublishnotifier.cpp
  zmqrpc.cpp
  zmqutil.cpp
)
target_compile_definitions(bitcoin_zmq
  INTERFACE
    ENABLE_ZMQ=1
)
target_link_libraries(bitcoin_zmq
  PRIVATE
    leveldb
    univalue
    $<TARGET_NAME_IF_EXISTS:libzmq>
    $<TARGET_NAME_IF_EXISTS:PkgConfig::libzmq>
)
