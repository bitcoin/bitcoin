# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# TODO: libbitcoinkernel is a work in progress consensus engine library, as more
#       and more modules are decoupled from the consensus engine, this list will
#       shrink to only those which are absolutely necessary. For example, things
#       like index/*.cpp will be removed.
set(bitcoinkernel_src
  $<$<CONFIG:Debug>:../sync.cpp>
  $<TARGET_OBJECTS:bitcoin_crypto>
  bitcoinkernel.cpp
  chain.cpp
  checks.cpp
  chainparams.cpp
  coinstats.cpp
  context.cpp
  cs_main.cpp
  mempool_persist.cpp
  ../arith_uint256.cpp
  ../chain.cpp
  ../chainparamsbase.cpp
  ../chainparams.cpp
  ../clientversion.cpp
  ../coins.cpp
  ../common/args.cpp
  ../common/config.cpp
  ../compressor.cpp
  ../consensus/merkle.cpp
  ../consensus/tx_check.cpp
  ../consensus/tx_verify.cpp
  ../core_read.cpp
  ../dbwrapper.cpp
  ../deploymentinfo.cpp
  ../deploymentstatus.cpp
  ../flatfile.cpp
  ../hash.cpp
  ../key.cpp
  ../logging.cpp
  ../node/blockstorage.cpp
  ../node/chainstate.cpp
  ../node/interface_ui.cpp
  ../node/utxo_snapshot.cpp
  ../policy/feerate.cpp
  ../policy/fees.cpp
  ../policy/packages.cpp
  ../policy/policy.cpp
  ../policy/rbf.cpp
  ../policy/settings.cpp
  ../pow.cpp
  ../primitives/block.cpp
  ../primitives/transaction.cpp
  ../pubkey.cpp
  ../random.cpp
  ../randomenv.cpp
  ../scheduler.cpp
  ../script/interpreter.cpp
  ../script/script.cpp
  ../script/script_error.cpp
  ../script/sigcache.cpp
  ../script/standard.cpp
  ../shutdown.cpp
  ../signet.cpp
  ../support/cleanse.cpp
  ../support/lockedpool.cpp
  ../txdb.cpp
  ../txmempool.cpp
  ../uint256.cpp
  ../util/chaintype.cpp
  ../util/check.cpp
  ../util/exception.cpp
  ../util/fs.cpp
  ../util/fs_helpers.cpp
  ../util/getuniquepath.cpp
  ../util/hasher.cpp
  ../util/moneystr.cpp
  ../util/rbf.cpp
  ../util/serfloat.cpp
  ../util/settings.cpp
  ../util/strencodings.cpp
  ../util/string.cpp
  ../util/syscall_sandbox.cpp
  ../util/syserror.cpp
  ../util/system.cpp
  ../util/thread.cpp
  ../util/threadnames.cpp
  ../util/time.cpp
  ../util/tokenpipe.cpp
  ../validation.cpp
  ../validationinterface.cpp
  ../versionbits.cpp
  ../warnings.cpp
)

add_library(bitcoinkernel_static STATIC ${bitcoinkernel_src})
target_include_directories(bitcoinkernel_static
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)
target_link_libraries(bitcoinkernel_static
  PRIVATE
    leveldb
    univalue
    secp256k1
    Boost::headers
    $<$<PLATFORM_ID:Windows>:ws2_32>
)
if(NOT MSVC)
  set_target_properties(bitcoinkernel_static PROPERTIES
    OUTPUT_NAME bitcoinkernel
  )
endif()
if(NOT BUILD_STATIC)
  set_target_properties(bitcoinkernel_static PROPERTIES
    EXCLUDE_FROM_ALL TRUE
  )
endif()

if(BUILD_SHARED)
  add_library(bitcoinkernel SHARED ${bitcoinkernel_src})
  target_link_libraries(bitcoinkernel
    PRIVATE
      leveldb
      univalue
      secp256k1
      Boost::headers
      $<$<PLATFORM_ID:Windows>:ws2_32>
  )
  set_target_properties(bitcoinkernel PROPERTIES
    SOVERSION 0
    VERSION 0.0.0
  )
endif()
