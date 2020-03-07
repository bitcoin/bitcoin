add_definitions(-DLEVELDB_PLATFORM_POSIX=1)

add_subdirectory(leveldb)
add_subdirectory(json)

add_library(crown_crypto
  crypto/sha1.cpp
  crypto/sha256.cpp
  crypto/sha512.cpp
  crypto/hmac_sha256.cpp
  crypto/hmac_sha512.cpp
  crypto/ripemd160.cpp
  crypto/common.h
  crypto/sha256.h
  crypto/sha512.h
  crypto/hmac_sha256.h
  crypto/hmac_sha512.h
  crypto/sha1.h
  crypto/ripemd160.h
)
target_include_directories(crown_crypto PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(crown_core INTERFACE
#[[
  activemasternode.h 
  activesystemnode.h 
  addrman.h 
  alert.h 
  amount.h 
  auxpow.h 
  arith_uint256.h 
  base58.h 
  bloom.h 
  chain.h 
  chainparamsbase.h 
  chainparams.h 
  chainparamsseeds.h 
  checkpoints.h 
  checkqueue.h 
  clientversion.h 
  coincontrol.h 
  coins.h 
  compat.h 
  compressor.h 
  core_io.h 
  core_memusage.h 
  crypter.h 
  legacysigner.h 
  db.h 
  hash.h 
  init.h 
  instantx.h 
  key.h 
  keystore.h 
  leveldbwrapper.h 
  limitedmap.h 
  main.h 
  memusage.h 
  merkleblock.h 
  miner.h 
  mruset.h 
  netbase.h 
  net.h 
  noui.h 
  pow.h 
  prevector.h 
  primitives/block.h 
  primitives/pureheader.h 
  primitives/transaction.h 
  protocol.h 
  pubkey.h 
  random.h 
  rpcclient.h 
  rpcprotocol.h 
  rpcserver.h 
  script/interpreter.h 
  script/script_error.h 
  script/script.h 
  script/sigcache.h 
  script/sign.h 
  script/standard.h 
  serialize.h 
  spork.h 
  streams.h 
  support/allocators/secure.h 
  support/allocators/zeroafterfree.h 
  support/pagelocker.h 
  support/cleanse.h 
  sync.h 
  masternode.h 
  masternode-payments.h 
  masternode-budget.h 
  masternode-sync.h 
  masternodeman.h 
  masternodeconfig.h
  nodeconfig.h
  systemnode.h 
  systemnode-payments.h 
  systemnode-sync.h 
  systemnodeman.h 
  systemnodeconfig.h 
  threadsafety.h 
  timedata.h 
  tinyformat.h 
  txdb.h 
  txmempool.h 
  ui_interface.h 
  uint256.h 
  undo.h 
  util.h 
  utilmoneystr.h 
  utilstrencodings.h 
  utiltime.h 
  version.h 
  walletdb.h 
  wallet.h 
  wallet_ismine.h 
  compat/byteswap.h 
  compat/endian.h 
  compat/sanity.h
]]
)
target_include_directories(crown_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


# LIBCROWN-SERVER

add_library(crown_server 
  addrman.cpp 
  alert.cpp 
  bloom.cpp 
  chain.cpp 
  checkpoints.cpp 
  init.cpp 
  leveldbwrapper.cpp 
  main.cpp 
  merkleblock.cpp 
  miner.cpp 
  net.cpp 
  noui.cpp 
  pow.cpp 
  rest.cpp 
  rpcblockchain.cpp 
  rpcservice.cpp 
  rpcmasternode.cpp 
  rpcmasternode-budget.cpp 
  rpcsystemnode.cpp 
  rpcmining.cpp 
  rpcmisc.cpp 
  rpcnet.cpp
  rpcupdate.cpp
  rpcrawtransaction.cpp 
  rpcserver.cpp 
  script/sigcache.cpp 
  timedata.cpp 
  txdb.cpp 
  txmempool.cpp
  dbdetails.cpp
  dbdetails.h
  dbmanager.h
)

target_include_directories(crown_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE leveldb/helpers/memenv)

target_link_libraries(crown_server
        PUBLIC leveldb
        PRIVATE crown_core
        PUBLIC json
        PRIVATE crown_util
        PRIVATE crown_common
        PRIVATE crown_wallet
        PRIVATE OpenSSL::SSL
        PRIVATE leveldb
        PRIVATE memenv
        PRIVATE crown_platform
        )


#LIBCROWN-COMMON

add_library(crown_common
  arith_uint256.cpp 
  amount.cpp 
  auxpow.cpp 
  base58.cpp 
  chainparams.cpp 
  coins.cpp 
  compressor.cpp 
  primitives/block.cpp 
  primitives/pureheader.cpp 
  primitives/transaction.cpp 
  core_read.cpp 
  core_write.cpp 
  hash.cpp 
  key.cpp 
  keystore.cpp 
  netbase.cpp 
  protocol.cpp 
  pubkey.cpp 
  script/interpreter.cpp 
  script/script.cpp 
  script/sign.cpp 
  script/standard.cpp 
  script/script_error.cpp 
  spork.cpp 

  activemasternode.h 
  activesystemnode.h 
  addrman.h 
  alert.h 
  amount.h 
  auxpow.h 
  arith_uint256.h 
  base58.h 
  bloom.h 
  chain.h 
  chainparamsbase.h 
  chainparams.h 
  chainparamsseeds.h 
  checkpoints.h 
  checkqueue.h 
  clientversion.h 
  coincontrol.h 
  coins.h 
  compat.h 
  compressor.h 
  core_io.h 
  core_memusage.h 
  crypter.h 
  legacysigner.h 
  db.h 
  hash.h 
  init.h 
  instantx.h 
  key.h 
  keystore.h 
  leveldbwrapper.h 
  limitedmap.h 
  main.h 
  memusage.h 
  merkleblock.h 
  miner.h 
  mruset.h 
  netbase.h 
  net.h 
  noui.h 
  pow.h 
  prevector.h 
  primitives/block.h 
  primitives/pureheader.h 
  primitives/transaction.h 
  protocol.h 
  pubkey.h 
  random.h 
  rpcclient.h 
  rpcprotocol.h 
  rpcserver.h 
  script/interpreter.h 
  script/script_error.h 
  script/script.h 
  script/sigcache.h 
  script/sign.h 
  script/standard.h 
  serialize.h 
  spork.h 
  streams.h 
  support/allocators/secure.h 
  support/allocators/zeroafterfree.h 
  support/pagelocker.h 
  support/cleanse.h 
  sync.h 
  masternode.h 
  masternode-payments.h 
  masternode-budget.h 
  masternode-sync.h 
  masternodeman.h 
  masternodeconfig.h
  nodeconfig.h
  systemnode.h 
  systemnode-payments.h 
  systemnode-sync.h 
  systemnodeman.h 
  systemnodeconfig.h 
  threadsafety.h 
  timedata.h 
  tinyformat.h 
  txdb.h 
  txmempool.h 
  ui_interface.h 
  uint256.h 
  undo.h 
  util.h 
  utilmoneystr.h 
  utilstrencodings.h 
  utiltime.h 
  version.h 
  walletdb.h 
  wallet.h 
  wallet_ismine.h 
  compat/byteswap.h 
  compat/endian.h 
  compat/sanity.h)

target_include_directories(crown_common PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE leveldb/helpers/memenv PRIVATE secp256k1/include)
target_link_libraries(crown_common PUBLIC crown_core PUBLIC crown_crypto PRIVATE secp256k1 PRIVATE crown_util PRIVATE crown_univalue)

# LIBCROWN-CLIENT

add_library(crown_client
  rpcclient.cpp)

# LIBCROWN-UTIL

add_library(crown_util
  support/pagelocker.cpp 
  chainparamsbase.cpp 
  clientversion.cpp 
  compat/glibc_sanity.cpp 
  compat/glibcxx_sanity.cpp 
  compat/strnlen.cpp 
  random.cpp 
  rpcprotocol.cpp 
  support/cleanse.cpp 
  sync.cpp 
  uint256.cpp 
  util.cpp 
  utilmoneystr.cpp 
  utilstrencodings.cpp 
  utiltime.cpp 
)

target_include_directories(crown_util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(crown_util PUBLIC crown_core PRIVATE Boost::program_options)

add_library(crown_wallet
  activemasternode.cpp 
  activesystemnode.cpp 
  legacysigner.cpp 
  db.cpp 
  crypter.cpp 
  instantx.cpp 
  masternode.cpp 
  masternode-budget.cpp 
  masternode-payments.cpp 
  masternode-sync.cpp 
  masternodeconfig.cpp
  nodeconfig.cpp
  masternodeman.cpp
  systemnode-sync.cpp 
  systemnode.cpp 
  systemnode-payments.cpp 
  systemnodeconfig.cpp 
  systemnodeman.cpp 
  rpcdump.cpp
  rpcwallet.cpp 
  wallet.cpp 
  wallet_ismine.cpp 
  walletdb.cpp
  updater.cpp
)
target_include_directories(crown_wallet
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(crown_wallet
  PUBLIC Boost::thread
  PUBLIC crown_util
  PUBLIC crown_common
  PUBLIC crown_server
  PUBLIC crown_pos
  PUBLIC BDB::bdb
  PUBLIC OpenSSL::Crypto
  PUBLIC OpenSSL::SSL
  PUBLIC CURL::libcurl
  )

#LIBCROWN-PLATFORM

add_library(crown_platform
  platform/specialtx-common.h
  platform/specialtx.h
  platform/specialtx.cpp
  platform/governance-vote.h
  platform/governance-vote.cpp
  platform/governance-approval-voting.h
  platform/governance-approval-voting.cpp
  platform/governance.cpp
  platform/agent.h
  platform/agent.cpp
  platform/platform-utils.h
  platform/platform-db.h
  platform/platform-db.cpp

  platform/rpc/specialtx-rpc-utils.h
  platform/rpc/specialtx-rpc-utils.cpp
  platform/rpc/rpcagents.h
  platform/rpc/rpcagents.cpp
  platform/rpc/rpc-nf-token.h
  platform/rpc/rpc-nf-token.cpp
  platform/rpc/rpc-nft-proto.h
  platform/rpc/rpc-nft-proto.cpp

  platform/nf-token/nf-token.h
  platform/nf-token/nf-token-index.h
  platform/nf-token/nf-token-multiindex-utils.h
  platform/nf-token/nf-token-multiindex-utils.cpp
  platform/nf-token/nf-token-protocol.h
  platform/nf-token/nf-token-protocol.cpp
  platform/nf-token/nf-token-protocol-index.h
  platform/nf-token/nf-token-protocol-reg-tx.h
  platform/nf-token/nf-token-protocol-reg-tx.cpp
  platform/nf-token/nf-token-protocol-tx-mem-pool-handler.h
  platform/nf-token/nf-token-protocol-tx-mem-pool-handler.cpp
  platform/nf-token/nf-token-reg-tx.h
  platform/nf-token/nf-token-reg-tx.cpp
  platform/nf-token/nf-token-reg-tx-builder.h
  platform/nf-token/nf-token-tx-mem-pool-handler.h
  platform/nf-token/nf-token-tx-mem-pool-handler.cpp
  platform/nf-token/nft-protocols-manager.h
  platform/nf-token/nft-protocols-manager.cpp
  platform/nf-token/nf-tokens-manager.cpp





  )

target_include_directories(crown_platform
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE leveldb/include
)

# LIBCROWN-POS

add_library(crown_pos
        mn-pos/blockwitness.h
        mn-pos/kernel.h
        mn-pos/kernel.cpp
        mn-pos/prooftracker.h
        mn-pos/prooftracker.cpp
        mn-pos/stakeminer.h
        mn-pos/stakeminer.cpp
        mn-pos/stakepointer.h
        mn-pos/stakepointer.cpp
        mn-pos/stakevalidation.h
        mn-pos/stakevalidation.cpp
        )

target_include_directories(crown_pos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )


add_executable(crownd
  crownd.cpp
)
target_include_directories(crownd
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(crownd
  crown_core
  crown_server
  crown_common
  crown_wallet
  crown_univalue
  crown_util
  crown_crypto
  crown_platform
  leveldb
  memenv
  secp256k1
  Boost::boost
  Boost::thread
  Boost::program_options
  OpenSSL::Crypto
  OpenSSL::SSL
  BDB::bdb
)

add_executable(crown-cli
  crown-cli.cpp
)

target_include_directories(crown-cli
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_link_libraries(crown-cli
  crown_client
  crown_util
  Boost::thread
  Boost::filesystem
  OpenSSL::SSL
)