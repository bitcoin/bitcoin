# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

configure_file(${CMAKE_SOURCE_DIR}/cmake/bitcoin-config.h.in config/bitcoin-config.h @ONLY)
add_compile_definitions(HAVE_CONFIG_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(crypto)
add_subdirectory(univalue)
add_subdirectory(util)


# Stable, backwards-compatible consensus functionality
# also exposed as a shared library and/or a static one.
add_library(bitcoin_consensus OBJECT EXCLUDE_FROM_ALL
  arith_uint256.cpp
  consensus/merkle.cpp
  consensus/tx_check.cpp
  hash.cpp
  primitives/block.cpp
  primitives/transaction.cpp
  pubkey.cpp
  script/interpreter.cpp
  script/script.cpp
  script/script_error.cpp
  uint256.cpp
  util/strencodings.cpp
)
target_link_libraries(bitcoin_consensus PRIVATE secp256k1)

if(WITH_ZMQ)
  add_subdirectory(zmq EXCLUDE_FROM_ALL)
endif()

# Home for common functionality shared by different executables and libraries.
# Similar to `bitcoin_util` library, but higher-level.
add_library(bitcoin_common STATIC EXCLUDE_FROM_ALL
  base58.cpp
  bech32.cpp
  chainparams.cpp
  coins.cpp
  common/args.cpp
  common/bloom.cpp
  common/config.cpp
  common/init.cpp
  common/interfaces.cpp
  common/run_command.cpp
  common/settings.cpp
  common/system.cpp
  $<$<TARGET_EXISTS:libevent::libevent>:common/url.cpp>
  compressor.cpp
  core_read.cpp
  core_write.cpp
  deploymentinfo.cpp
  external_signer.cpp
  init/common.cpp
  kernel/chainparams.cpp
  key.cpp
  key_io.cpp
  merkleblock.cpp
  net_types.cpp
  netaddress.cpp
  netbase.cpp
  net_permissions.cpp
  outputtype.cpp
  policy/feerate.cpp
  policy/policy.cpp
  protocol.cpp
  psbt.cpp
  rpc/rawtransaction_util.cpp
  rpc/request.cpp
  rpc/external_signer.cpp
  rpc/util.cpp
  scheduler.cpp
  script/descriptor.cpp
  script/miniscript.cpp
  script/sign.cpp
  script/signingprovider.cpp
  script/standard.cpp
  warnings.cpp
)
target_compile_definitions(bitcoin_common
  PRIVATE
    ${THREAD_LOCAL_IF_AVAILABLE}
)
target_link_libraries(bitcoin_common
  PRIVATE
    bitcoin_consensus
    bitcoin_util
    univalue
    secp256k1
    Boost::headers
    $<TARGET_NAME_IF_EXISTS:libevent::libevent>
)


# P2P and RPC server functionality used by `bitcoind` and `bitcoin-qt` executables.
add_library(bitcoin_node STATIC EXCLUDE_FROM_ALL
  addrdb.cpp
  addrman.cpp
  banman.cpp
  blockencodings.cpp
  blockfilter.cpp
  chain.cpp
  consensus/tx_verify.cpp
  dbwrapper.cpp
  deploymentstatus.cpp
  flatfile.cpp
  headerssync.cpp
  httprpc.cpp
  httpserver.cpp
  i2p.cpp
  index/base.cpp
  index/blockfilterindex.cpp
  index/coinstatsindex.cpp
  index/txindex.cpp
  init.cpp
  kernel/chain.cpp
  kernel/checks.cpp
  kernel/coinstats.cpp
  kernel/context.cpp
  kernel/cs_main.cpp
  kernel/mempool_persist.cpp
  mapport.cpp
  net.cpp
  net_processing.cpp
  netgroup.cpp
  node/abort.cpp
  node/blockmanager_args.cpp
  node/blockstorage.cpp
  node/caches.cpp
  node/chainstate.cpp
  node/chainstatemanager_args.cpp
  node/coin.cpp
  node/coins_view_args.cpp
  node/connection_types.cpp
  node/context.cpp
  node/database_args.cpp
  node/eviction.cpp
  node/interface_ui.cpp
  node/interfaces.cpp
  node/kernel_notifications.cpp
  node/mempool_args.cpp
  node/mempool_persist_args.cpp
  node/miner.cpp
  node/mini_miner.cpp
  node/minisketchwrapper.cpp
  node/psbt.cpp
  node/transaction.cpp
  node/txreconciliation.cpp
  node/utxo_snapshot.cpp
  node/validation_cache_args.cpp
  noui.cpp
  policy/fees.cpp
  policy/fees_args.cpp
  policy/packages.cpp
  policy/rbf.cpp
  policy/settings.cpp
  pow.cpp
  rest.cpp
  rpc/blockchain.cpp
  rpc/fees.cpp
  rpc/mempool.cpp
  rpc/mining.cpp
  rpc/net.cpp
  rpc/node.cpp
  rpc/output_script.cpp
  rpc/rawtransaction.cpp
  rpc/server.cpp
  rpc/server_util.cpp
  rpc/signmessage.cpp
  rpc/txoutproof.cpp
  script/sigcache.cpp
  shutdown.cpp
  signet.cpp
  timedata.cpp
  torcontrol.cpp
  txdb.cpp
  txmempool.cpp
  txorphanage.cpp
  txrequest.cpp
  validation.cpp
  validationinterface.cpp
  versionbits.cpp

  dummywallet.cpp
)
target_link_libraries(bitcoin_node
  PRIVATE
    bitcoin_common
    bitcoin_util
    leveldb
    minisketch
    univalue
    Boost::headers
    libevent::libevent
    $<TARGET_NAME_IF_EXISTS:PkgConfig::libevent_pthreads>
    $<TARGET_NAME_IF_EXISTS:NATPMP::NATPMP>
    $<TARGET_NAME_IF_EXISTS:MiniUPnPc::MiniUPnPc>
    $<TARGET_NAME_IF_EXISTS:bitcoin_zmq>
)


# Bitcoin Core bitcoind.
if(BUILD_DAEMON)
  add_executable(bitcoind
    bitcoind.cpp
    init/bitcoind.cpp
  )
  target_link_libraries(bitcoind
    PRIVATE
      bitcoin_node
  )
  target_link_options(bitcoind
    PRIVATE
      $<$<BOOL:${MINGW}>:-static>
  )
endif()


add_library(bitcoin_cli STATIC EXCLUDE_FROM_ALL
  compat/stdin.cpp
  rpc/client.cpp
)
target_link_libraries(bitcoin_cli
  PUBLIC
    univalue
)


# Bitcoin Core RPC client
if(BUILD_CLI)
  add_executable(bitcoin-cli bitcoin-cli.cpp)
  target_link_libraries(bitcoin-cli
    bitcoin_cli
    bitcoin_common
    bitcoin_util
    libevent::libevent
  )
endif()


if(BUILD_TX)
  add_executable(bitcoin-tx bitcoin-tx.cpp)
  target_link_libraries(bitcoin-tx
    bitcoin_common
    bitcoin_util
    univalue
  )
endif()


if(BUILD_UTIL)
  add_executable(bitcoin-util bitcoin-util.cpp)
  target_link_libraries(bitcoin-util
    bitcoin_common
    bitcoin_util
  )
endif()
