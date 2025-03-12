# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xc77d03df6a41b505;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Common = import "common.capnp";
using Proxy = import "/mp/proxy.capnp";
$Proxy.include("interfaces/mining.h");
$Proxy.includeTypes("ipc/capnp/mining-types.h");

interface Mining $Proxy.wrap("interfaces::Mining") {
    isTestChain @0 (context :Proxy.Context) -> (result: Bool);
    isInitialBlockDownload @1 (context :Proxy.Context) -> (result: Bool);
    getTip @2 (context :Proxy.Context) -> (result: Common.BlockRef, hasResult: Bool);
    waitTipChanged @3 (context :Proxy.Context, currentTip: Data, timeout: Float64) -> (result: Common.BlockRef);
    createNewBlock @4 (options: BlockCreateOptions) -> (result: BlockTemplate);
}

interface BlockTemplate $Proxy.wrap("interfaces::BlockTemplate") {
    destroy @0 (context :Proxy.Context) -> ();
    getBlockHeader @1 (context: Proxy.Context) -> (result: Data);
    getBlock @2 (context: Proxy.Context) -> (result: Data);
    getTxFees @3 (context: Proxy.Context) -> (result: List(Int64));
    getTxSigops @4 (context: Proxy.Context) -> (result: List(Int64));
    getCoinbaseTx @5 (context: Proxy.Context) -> (result: Data);
    getCoinbaseCommitment @6 (context: Proxy.Context) -> (result: Data);
    getWitnessCommitmentIndex @7 (context: Proxy.Context) -> (result: Int32);
    getCoinbaseMerklePath @8 (context: Proxy.Context) -> (result: List(Data));
    submitSolution @9 (context: Proxy.Context, version: UInt32, timestamp: UInt32, nonce: UInt32, coinbase :Data) -> (result: Bool);
    waitNext @10 (context: Proxy.Context, options: BlockWaitOptions) -> (result: BlockTemplate);
}

struct BlockCreateOptions $Proxy.wrap("node::BlockCreateOptions") {
    useMempool @0 :Bool $Proxy.name("use_mempool");
    blockReservedWeight @1 :UInt64 $Proxy.name("block_reserved_weight");
    coinbaseOutputMaxAdditionalSigops @2 :UInt64 $Proxy.name("coinbase_output_max_additional_sigops");
}

struct BlockWaitOptions $Proxy.wrap("node::BlockWaitOptions") {
    timeout @0 : Float64 $Proxy.name("timeout");
    feeThreshold @1 : Int64 $Proxy.name("fee_threshold");
}

# Note: serialization of the BlockValidationState C++ type is somewhat fragile
# and using the struct can be awkward. It would be good if testBlockValidity
# method were changed to return validity information in a simpler format.
struct BlockValidationState {
    mode @0 :Int32;
    result @1 :Int32;
    rejectReason @2 :Text;
    debugMessage @3 :Text;
}
