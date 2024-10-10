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
    createNewBlock @4 (scriptPubKey: Data, options: BlockCreateOptions) -> (result: BlockTemplate);
    processNewBlock @5 (context :Proxy.Context, block: Data) -> (newBlock: Bool, result: Bool);
    getTransactionsUpdated @6 (context :Proxy.Context) -> (result: UInt32);
    testBlockValidity @7 (context :Proxy.Context, block: Data, checkMerkleRoot: Bool) -> (state: BlockValidationState, result: Bool);
}

interface BlockTemplate $Proxy.wrap("interfaces::BlockTemplate") {
    getBlockHeader @0 (context: Proxy.Context) -> (result: Data);
    getBlock @1 (context: Proxy.Context) -> (result: Data);
    getTxFees @2 (context: Proxy.Context) -> (result: List(Int64));
    getTxSigops @3 (context: Proxy.Context) -> (result: List(Int64));
    getCoinbaseTx @4 (context: Proxy.Context) -> (result: Data);
    getCoinbaseCommitment @5 (context: Proxy.Context) -> (result: Data);
    getWitnessCommitmentIndex @6 (context: Proxy.Context) -> (result: Int32);
    getCoinbaseMerklePath @7 (context: Proxy.Context) -> (result: List(Data));
    submitSolution@8 (context: Proxy.Context, version: UInt32, timestamp: UInt32, nonce: UInt32, coinbase :Data) -> (result: Bool);
}

struct BlockCreateOptions $Proxy.wrap("node::BlockCreateOptions") {
    useMempool @0 :Bool $Proxy.name("use_mempool");
    coinbaseMaxAdditionalWeight @1 :UInt64 $Proxy.name("coinbase_max_additional_weight");
    coinbaseOutputMaxAdditionalSigops @2 :UInt64 $Proxy.name("coinbase_output_max_additional_sigops");
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
