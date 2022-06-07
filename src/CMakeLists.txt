# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

add_subdirectory(crypto)
add_subdirectory(util)
if(ENABLE_WALLET)
  add_subdirectory(wallet)
endif()

# Home for common functionality shared by different executables and libraries.
# Similar to `bitcoin_util` library, but higher-level.
add_library(bitcoin_common STATIC EXCLUDE_FROM_ALL)
target_sources(bitcoin_common
  PRIVATE
    base58.cpp
    bech32.cpp
    chainparams.cpp
    coins.cpp
    common/bloom.cpp
    compressor.cpp
    core_read.cpp
    core_write.cpp
    deploymentinfo.cpp
    external_signer.cpp
    init/common.cpp
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
target_include_directories(bitcoin_common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(bitcoin_common
  PRIVATE
    univalue
    secp256k1
)


# Stable, backwards-compatible consensus functionality
# also exposed as a shared library.
add_library(bitcoin_consensus OBJECT EXCLUDE_FROM_ALL)
target_sources(bitcoin_consensus
  PRIVATE
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
target_include_directories(bitcoin_consensus
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(bitcoin_consensus
  PRIVATE
    secp256k1
)


# P2P and RPC server functionality used by `bitcoind` and `bitcoin-qt` executables.
add_library(bitcoin_node STATIC EXCLUDE_FROM_ALL)

target_sources(bitcoin_node
  PRIVATE
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
    httprpc.cpp
    httpserver.cpp
    i2p.cpp
    index/base.cpp
    index/blockfilterindex.cpp
    index/coinstatsindex.cpp
    index/txindex.cpp
    init.cpp
    kernel/checks.cpp
    kernel/coinstats.cpp
    kernel/context.cpp
    mapport.cpp
    net.cpp
    netgroup.cpp
    net_processing.cpp
    node/blockstorage.cpp
    node/caches.cpp
    node/chainstate.cpp
    node/coin.cpp
    node/context.cpp
    node/interfaces.cpp
    node/miner.cpp
    node/minisketchwrapper.cpp
    node/psbt.cpp
    node/transaction.cpp
    node/ui_interface.cpp
    noui.cpp
    policy/fees.cpp
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
)
if(ENABLE_WALLET)
  target_sources(bitcoin_node PRIVATE wallet/init.cpp)
else()
  target_sources(bitcoin_node PRIVATE dummywallet.cpp)
endif()

target_include_directories(bitcoin_node
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(bitcoin_node
  PRIVATE
    minisketch
    leveldb
    univalue
    Boost::headers
)


if(BUILD_DAEMON)
  # Bitcoin Core bitcoind
  add_executable(bitcoind)

  target_sources(bitcoind
    PRIVATE
      bitcoind.cpp
      init/bitcoind.cpp
  )

  if(ENABLE_WALLET)
    target_link_libraries(bitcoind PRIVATE bitcoin_wallet)
  endif()
  target_link_libraries(bitcoind
    PRIVATE
      bitcoin_node
      bitcoin_consensus
      bitcoin_common
      bitcoin_util
      bitcoin_crypto
      PkgConfig::libevent
  )
  if(WIN32)
    target_link_options(bitcoind PRIVATE -static)
  else()
    target_link_libraries(bitcoind PRIVATE PkgConfig::libevent_pthreads)
  endif()
endif()


if(BUILD_CLI)
  # Bitcoin Core RPC client
  add_executable(bitcoin-cli)

  target_sources(bitcoin-cli
    PRIVATE
      bitcoin-cli.cpp
      compat/stdin.cpp
      rpc/client.cpp
  )

  target_link_libraries(bitcoin-cli
    PRIVATE
      bitcoin_util
      bitcoin_crypto
      univalue
      PkgConfig::libevent
  )

  if(WIN32)
    target_link_options(bitcoin-cli PRIVATE -static)
  endif()
endif()
