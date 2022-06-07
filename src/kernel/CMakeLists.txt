# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# TODO: libbitcoinkernel is a work in progress consensus engine library, as more
#       and more modules are decoupled from the consensus engine, this list will
#       shrink to only those which are absolutely necessary. For example, things
#       like index/*.cpp will be removed.

add_library(bitcoinkernel SHARED)
target_sources(bitcoinkernel
  PRIVATE
    bitcoinkernel.cpp
    checks.cpp
    coinstats.cpp
    context.cpp
    ../arith_uint256.cpp
    ../chain.cpp
    ../chainparamsbase.cpp
    ../chainparams.cpp
    ../clientversion.cpp
    ../coins.cpp
    ../compressor.cpp
    ../consensus/merkle.cpp
    ../consensus/tx_check.cpp
    ../consensus/tx_verify.cpp
    ../core_read.cpp
    ../dbwrapper.cpp
    ../deploymentinfo.cpp
    ../deploymentstatus.cpp
    ../flatfile.cpp
    ../fs.cpp
    ../hash.cpp
    ../key.cpp
    ../logging.cpp
    ../node/blockstorage.cpp
    ../node/chainstate.cpp
    ../node/ui_interface.cpp
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
    ../sync.cpp
    ../threadinterrupt.cpp
    ../txdb.cpp
    ../txmempool.cpp
    ../uint256.cpp
    ../util/bytevectorhash.cpp
    ../util/check.cpp
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
if(HAVE_GMTIME_R)
  target_compile_definitions(bitcoinkernel PRIVATE HAVE_GMTIME_R)
endif()
target_link_libraries(bitcoinkernel
  PRIVATE
    bitcoin_crypto
    leveldb
    univalue
    secp256k1
    Boost::headers
)
if(WIN32)
  target_link_libraries(bitcoinkernel PRIVATE ws2_32)
endif()
