# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

@0x94f21a4864bd2c65;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("interfaces/chain.h");
$Proxy.include("rpc/server.h");
$Proxy.includeTypes("ipc/capnp/chain-types.h");

using Common = import "common.capnp";
using Handler = import "handler.capnp";

interface Chain $Proxy.wrap("interfaces::Chain") {
    destroy @0 (context :Proxy.Context) -> ();
    getHeight @1 (context :Proxy.Context) -> (result :Int32, hasResult :Bool);
    getBlockHash @2 (context :Proxy.Context, height :Int32) -> (result :Data);
    haveBlockOnDisk @3 (context :Proxy.Context, height :Int32) -> (result :Bool);
    findLocatorFork @4 (context :Proxy.Context, locator :Data) -> (result :Int32, hasResult :Bool);
    hasBlockFilterIndex @5 (context :Proxy.Context, filterType :UInt8) -> (result :Bool);
    blockFilterMatchesAny @6 (context :Proxy.Context, filterType :UInt8, blockHash :Data, filterSet :List(Data)) -> (result :Bool, hasResult :Bool);
    findBlock @7 (context :Proxy.Context, hash :Data, block :FoundBlockParam) -> (block :FoundBlockResult, result :Bool);
    findFirstBlockWithTimeAndHeight @8 (context :Proxy.Context, minTime :Int64, minHeight :Int32, block :FoundBlockParam) -> (block :FoundBlockResult, result :Bool);
    findAncestorByHeight @9 (context :Proxy.Context, blockHash :Data, ancestorHeight :Int32, ancestor :FoundBlockParam) -> (ancestor :FoundBlockResult, result :Bool);
    findAncestorByHash @10 (context :Proxy.Context, blockHash :Data, ancestorHash :Data, ancestor :FoundBlockParam) -> (ancestor :FoundBlockResult, result :Bool);
    findCommonAncestor @11 (context :Proxy.Context, blockHash1 :Data, blockHash2 :Data, ancestor :FoundBlockParam, block1 :FoundBlockParam, block2 :FoundBlockParam) -> (ancestor :FoundBlockResult, block1 :FoundBlockResult, block2 :FoundBlockResult, result :Bool);
    findCoins @12 (context :Proxy.Context, coins :List(Common.Pair(Data, Data))) -> (coins :List(Common.Pair(Data, Data)));
    guessVerificationProgress @13 (context :Proxy.Context, blockHash :Data) -> (result :Float64);
    hasBlocks @14 (context :Proxy.Context, blockHash :Data, minHeight :Int32, maxHeight: Int32, hasMaxHeight :Bool) -> (result :Bool);
    isRBFOptIn @15 (context :Proxy.Context, tx :Data) -> (result :Int32);
    isInMempool @16 (context :Proxy.Context, txid :Data) -> (result :Bool);
    hasDescendantsInMempool @17 (context :Proxy.Context, txid :Data) -> (result :Bool);
    broadcastTransaction @18 (context :Proxy.Context, tx: Data, maxTxFee :Int64, broadcastMethod :Int32) -> (error: Text, result :Bool);
    getTransactionAncestry @19 (context :Proxy.Context, txid :Data) -> (ancestors :UInt64, descendants :UInt64, ancestorsize :UInt64, ancestorfees :Int64);
    calculateIndividualBumpFees @20 (context :Proxy.Context, outpoints :List(Data), targetFeerate :Data) -> (result: List(Common.PairInt64(Data)));
    calculateCombinedBumpFee @21 (context :Proxy.Context, outpoints :List(Data), targetFeerate :Data) -> (result :Int64, hasResult :Bool);
    getPackageLimits @22 (context :Proxy.Context) -> (ancestors :UInt32, descendants :UInt32);
    checkChainLimits @23 (context :Proxy.Context, tx :Data) -> (result :Common.ResultVoid);
    estimateSmartFee @24 (context :Proxy.Context, numBlocks :Int32, conservative :Bool, wantCalc :Bool) -> (calc :FeeCalculation, result :Data);
    estimateMaxBlocks @25 (context :Proxy.Context) -> (result :UInt32);
    mempoolMinFee @26 (context :Proxy.Context) -> (result :Data);
    relayMinFee @27 (context :Proxy.Context) -> (result :Data);
    relayIncrementalFee @28 (context :Proxy.Context) -> (result :Data);
    relayDustFee @29 (context :Proxy.Context) -> (result :Data);
    havePruned @30 (context :Proxy.Context) -> (result :Bool);
    getPruneHeight @31 (context :Proxy.Context) -> (result: Int32, hasResult: Bool);
    isReadyToBroadcast @32 (context :Proxy.Context) -> (result :Bool);
    isInitialBlockDownload @33 (context :Proxy.Context) -> (result :Bool);
    shutdownRequested @34 (context :Proxy.Context) -> (result :Bool);
    initMessage @35 (context :Proxy.Context, message :Text) -> ();
    initWarning @36 (context :Proxy.Context, message :Common.BilingualStr) -> ();
    initError @37 (context :Proxy.Context, message :Common.BilingualStr) -> ();
    showProgress @38 (context :Proxy.Context, title :Text, progress :Int32, resumePossible :Bool) -> ();
    handleNotifications @39 (context :Proxy.Context, notifications :ChainNotifications) -> (result :Handler.Handler);
    waitForNotificationsIfTipChanged @40 (context :Proxy.Context, oldTip :Data) -> ();
    handleRpc @41 (context :Proxy.Context, command :RPCCommand) -> (result :Handler.Handler);
    rpcEnableDeprecated @42 (context :Proxy.Context, method :Text) -> (result :Bool);
    getSetting @43 (context :Proxy.Context, name :Text) -> (result :Text);
    getSettingsList @44 (context :Proxy.Context, name :Text) -> (result :List(Text));
    getRwSetting @45 (context :Proxy.Context, name :Text) -> (result :Text);
    updateRwSetting @46 (context :Proxy.Context, name :Text, update: SettingsUpdateCallback) -> (result :Bool);
    overwriteRwSetting @47 (context :Proxy.Context, name :Text, value :Text, action :Int32) -> (result :Bool);
    deleteRwSettings @48 (context :Proxy.Context, name :Text, action: Int32) -> (result :Bool);
    requestMempoolTransactions @49 (context :Proxy.Context, notifications :ChainNotifications) -> ();
    hasAssumedValidChain @50 (context :Proxy.Context) -> (result :Bool);
}

interface ChainNotifications $Proxy.wrap("interfaces::Chain::Notifications") {
    destroy @0 (context :Proxy.Context) -> ();
    transactionAddedToMempool @1 (context :Proxy.Context, tx :Data) -> ();
    transactionRemovedFromMempool @2 (context :Proxy.Context, tx :Data, reason :Int32) -> ();
    blockConnected @3 (context :Proxy.Context, role: ChainstateRole, block :BlockInfo) -> ();
    blockDisconnected @4 (context :Proxy.Context, block :BlockInfo) -> ();
    updatedBlockTip @5 (context :Proxy.Context) -> ();
    chainStateFlushed @6 (context :Proxy.Context, role: ChainstateRole, locator :Data) -> ();
}

interface ChainClient $Proxy.wrap("interfaces::ChainClient") {
    destroy @0 (context :Proxy.Context) -> ();
    registerRpcs @1 (context :Proxy.Context) -> ();
    verify @2 (context :Proxy.Context) -> (result :Bool);
    load @3 (context :Proxy.Context) -> (result :Bool);
    start @4 (context :Proxy.Context, scheduler :Void) -> ();
    stop @5 (context :Proxy.Context) -> ();
    setMockTime @6 (context :Proxy.Context, time :Int64) -> ();
    schedulerMockForward @7 (context :Proxy.Context, deltaSeconds :Int64) -> ();
}

struct FeeCalculation $Proxy.wrap("FeeCalculation") {
    est @0 :EstimationResult;
    reason @1 :Int32;
    desiredTarget @2 :Int32;
    returnedTarget @3 :Int32;
    bestHeight @4 :UInt32  $Proxy.name("best_height");
}

struct EstimationResult $Proxy.wrap("EstimationResult")
{
    pass @0 :EstimatorBucket;
    fail @1 :EstimatorBucket;
    decay @2 :Float64;
    scale @3 :UInt32;
}

struct EstimatorBucket $Proxy.wrap("EstimatorBucket")
{
    start @0 :Float64;
    end @1 :Float64;
    withinTarget @2 :Float64;
    totalConfirmed @3 :Float64;
    inMempool @4 :Float64;
    leftMempool @5 :Float64;
}

struct RPCCommand $Proxy.wrap("CRPCCommand") {
   category @0 :Text;
   name @1 :Text;
   actor @2 :ActorCallback;
   argNames @3 :List(RPCArg);
   uniqueId @4 :Int64 $Proxy.name("unique_id");
}

# Representation of std::pair<std::string, bool> in CRPCCommand
struct RPCArg {
   name @0 :Text;
   namedOnly @1: Bool;
}

interface ActorCallback $Proxy.wrap("ProxyCallback<CRPCCommand::Actor>") {
    call @0 (context :Proxy.Context, request :JSONRPCRequest, response :Text, lastCallback :Bool) -> (error :Text $Proxy.exception("std::exception"), rpcError :Text $Proxy.exception("UniValue"), typeError :Text $Proxy.exception("UniValue::type_error"), helpResult :HelpResult $Proxy.exception("HelpResult"), response :Text, result: Bool);
}

struct HelpResult {
    message @0 :Text;
}

struct JSONRPCRequest $Proxy.wrap("JSONRPCRequest") {
    id @0 :Text;
    method @1 :Text $Proxy.name("strMethod");
    params @2 :Text;
    mode @3 :Int32;
    uri @4 :Text $Proxy.name("URI");
    authUser @5 :Text;
    peerAddr @6 :Text;
    version @7: Int32 $Proxy.name("m_json_version");
}

struct FoundBlockParam {
    wantHash @0 :Bool;
    wantHeight @1 :Bool;
    wantTime @2 :Bool;
    wantMaxTime @3 :Bool;
    wantMtpTime @4 :Bool;
    wantInActiveChain @5 :Bool;
    wantLocator @6 :Bool;
    nextBlock @7: FoundBlockParam;
    wantData @8 :Bool;
}

struct FoundBlockResult {
    hash @0 :Data;
    height @1 :Int32;
    time @2 :Int64;
    maxTime @3 :Int64;
    mtpTime @4 :Int64;
    inActiveChain @5 :Bool;
    locator @6 :Data;
    nextBlock @7: FoundBlockResult;
    data @8 :Data;
    found @9 :Bool;
}

struct BlockInfo $Proxy.wrap("interfaces::BlockInfo") {
    # Fields skipped below with Proxy.skip are pointer fields manually handled
    # by CustomBuildMessage / CustomPassMessage overloads.
    hash @0 :Data $Proxy.skip;
    prevHash @1 :Data $Proxy.skip;
    height @2 :Int32 = -1;
    fileNumber @3 :Int32 = -1 $Proxy.name("file_number");
    dataPos @4 :UInt32 = 0 $Proxy.name("data_pos");
    data @5 :Data $Proxy.skip;
    undoData @6 :Data $Proxy.skip;
    chainTimeMax @7 :UInt32 = 0 $Proxy.name("chain_time_max");
}

struct ChainstateRole $Proxy.wrap("kernel::ChainstateRole") {
    validated @0 :Bool;
    historical @1 :Bool;
}

interface SettingsUpdateCallback $Proxy.wrap("ProxyCallback<interfaces::SettingsUpdate>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, value :Text) -> (value :Text, result: Int32, hasResult: Bool);
}
