# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xc77d03df6a41b505;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Common = import "common.capnp";
using Proxy = import "/mp/proxy.capnp";
$Proxy.include("interfaces/mining.h");
$Proxy.includeTypes("ipc/capnp/mining-types.h");

const maxMoney :Int64 = 2100000000000000;
const maxDouble :Float64 = 1.7976931348623157e308;
const defaultBlockReservedWeight :UInt32 = 8000;
const defaultCoinbaseOutputMaxAdditionalSigops :UInt32 = 400;

interface Mining $Proxy.wrap("interfaces::Mining") {
    isTestChain @0 (context :Proxy.Context) -> (result: Bool);
    isInitialBlockDownload @1 (context :Proxy.Context) -> (result: Bool);
    getTip @2 (context :Proxy.Context) -> (result: Common.BlockRef, hasResult: Bool);
    waitTipChanged @3 (context :Proxy.Context, currentTip: Data, timeout: Float64 = .maxDouble) -> (result: Common.BlockRef);
    createNewBlock @4 (context :Proxy.Context, options: BlockCreateOptions, cooldown: Bool = true) -> (result: BlockTemplate);
    checkBlock @5 (context :Proxy.Context, block: Data, options: BlockCheckOptions) -> (reason: Text, debug: Text, result: Bool);
    interrupt @6 () -> ();
}

interface BlockTemplate $Proxy.wrap("interfaces::BlockTemplate") {
    destroy @0 (context :Proxy.Context) -> ();
    getBlockHeader @1 (context: Proxy.Context) -> (result: Data);
    getBlock @2 (context: Proxy.Context) -> (result: Data);
    getTxFees @3 (context: Proxy.Context) -> (result: List(Int64));
    getTxSigops @4 (context: Proxy.Context) -> (result: List(Int64));
    getCoinbaseTx @5 (context: Proxy.Context) -> (result: CoinbaseTx);
    getCoinbaseMerklePath @6 (context: Proxy.Context) -> (result: List(Data));
    submitSolution @7 (context: Proxy.Context, version: UInt32, timestamp: UInt32, nonce: UInt32, coinbase :Data) -> (result: Bool);
    waitNext @8 (context: Proxy.Context, options: BlockWaitOptions) -> (result: BlockTemplate);
    interruptWait @9() -> ();
}

struct BlockCreateOptions $Proxy.wrap("node::BlockCreateOptions") {
    useMempool @0 :Bool = true $Proxy.name("use_mempool");
    blockReservedWeight @1 :UInt64 = .defaultBlockReservedWeight $Proxy.name("block_reserved_weight");
    coinbaseOutputMaxAdditionalSigops @2 :UInt64 = .defaultCoinbaseOutputMaxAdditionalSigops $Proxy.name("coinbase_output_max_additional_sigops");
}

struct BlockWaitOptions $Proxy.wrap("node::BlockWaitOptions") {
    timeout @0 : Float64 = .maxDouble $Proxy.name("timeout");
    feeThreshold @1 : Int64 = .maxMoney $Proxy.name("fee_threshold");
}

struct BlockCheckOptions $Proxy.wrap("node::BlockCheckOptions") {
    checkMerkleRoot @0 :Bool = true $Proxy.name("check_merkle_root");
    checkPow @1 :Bool = true $Proxy.name("check_pow");
}

struct CoinbaseTx $Proxy.wrap("node::CoinbaseTx") {
    version @0 :UInt32 $Proxy.name("version");
    sequence @1 :UInt32 $Proxy.name("sequence");
    scriptSigPrefix @2 :Data $Proxy.name("script_sig_prefix");
    witness @3 :Data $Proxy.name("witness");
    blockRewardRemaining @4 :Int64 $Proxy.name("block_reward_remaining");
    requiredOutputs @5 :List(Data) $Proxy.name("required_outputs");
    lockTime @6 :UInt32 $Proxy.name("lock_time");
}
