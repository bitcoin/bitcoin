# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0x92546c47dc734b2e;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("ipc/capnp/node.h");
$Proxy.includeTypes("ipc/capnp/node-types.h");

using Common = import "common.capnp";
using Handler = import "handler.capnp";
using Wallet = import "wallet.capnp";

interface Node $Proxy.wrap("interfaces::Node") {
    destroy @0 (context :Proxy.Context) -> ();
    initLogging @1 (context :Proxy.Context) -> ();
    initParameterInteraction @2 (context :Proxy.Context) -> ();
    getWarnings @3 (context :Proxy.Context) -> (result :Common.BilingualStr);
    getLogCategories @4 (context :Proxy.Context) -> (result :UInt32);
    baseInitialize @5 (context :Proxy.Context, globalArgs :Common.GlobalArgs) -> (error :Text $Proxy.exception("std::exception"), result :Bool);
    appInitMain @6 (context :Proxy.Context) -> (tipInfo :BlockAndHeaderTipInfo, error :Text $Proxy.exception("std::exception"), result :Bool);
    appShutdown @7 (context :Proxy.Context) -> ();
    startShutdown @8 (context :Proxy.Context) -> ();
    shutdownRequested @9 (context :Proxy.Context) -> (result :Bool);
    mapPort @10 (context :Proxy.Context, useUPnP :Bool, useNatPnP :Bool) -> ();
    getProxy @11 (context :Proxy.Context, net :Int32) -> (proxyInfo :ProxyType, result :Bool);
    getNodeCount @12 (context :Proxy.Context, flags :Int32) -> (result :UInt64);
    getNodesStats @13 (context :Proxy.Context) -> (stats :List(NodeStats), result :Bool);
    getBanned @14 (context :Proxy.Context) -> (banmap :List(Common.Pair(Data, Data)), result :Bool);
    ban @15 (context :Proxy.Context, netAddr :Data, banTimeOffset :Int64) -> (result :Bool);
    unban @16 (context :Proxy.Context, ip :Data) -> (result :Bool);
    disconnectByAddress @17 (context :Proxy.Context, address :Data) -> (result :Bool);
    disconnectById @18 (context :Proxy.Context, id :Int64) -> (result :Bool);
    getTotalBytesRecv @19 (context :Proxy.Context) -> (result :Int64);
    getTotalBytesSent @20 (context :Proxy.Context) -> (result :Int64);
    getMempoolSize @21 (context :Proxy.Context) -> (result :UInt64);
    getMempoolDynamicUsage @22 (context :Proxy.Context) -> (result :UInt64);
    getHeaderTip @23 (context :Proxy.Context) -> (height :Int32, blockTime :Int64, result :Bool);
    getNumBlocks @24 (context :Proxy.Context) -> (result :Int32);
    getBestBlockHash @25 (context :Proxy.Context) -> (result :Data);
    getLastBlockTime @26 (context :Proxy.Context) -> (result :Int64);
    getVerificationProgress @27 (context :Proxy.Context) -> (result :Float64);
    isInitialBlockDownload @28 (context :Proxy.Context) -> (result :Bool);
    getReindex @29 (context :Proxy.Context) -> (result :Bool);
    getImporting @30 (context :Proxy.Context) -> (result :Bool);
    setNetworkActive @31 (context :Proxy.Context, active :Bool) -> ();
    getNetworkActive @32 (context :Proxy.Context) -> (result :Bool);
    getDustRelayFee @33 (context :Proxy.Context) -> (result :Data);
    executeRpc @34 (context :Proxy.Context, command :Text, params :Common.UniValue, uri :Text) -> (error :Text $Proxy.exception("std::exception"), rpcError :Common.UniValue $Proxy.exception("UniValue"), result :Common.UniValue);
    listRpcCommands @35 (context :Proxy.Context) -> (result :List(Text));
    rpcSetTimerInterfaceIfUnset @36 (context :Proxy.Context, iface :Void) -> ();
    rpcUnsetTimerInterface @37 (context :Proxy.Context, iface :Void) -> ();
    getUnspentOutput @38 (context :Proxy.Context, output :Data) -> (coin :Data, result :Bool);
    customWalletClient @39 (context :Proxy.Context) -> (result :Wallet.WalletClient) $Proxy.name("walletClient");
    handleInitMessage @40 (context :Proxy.Context, callback :InitMessageCallback) -> (result :Handler.Handler);
    handleMessageBox @41 (context :Proxy.Context, callback :MessageBoxCallback) -> (result :Handler.Handler);
    handleQuestion @42 (context :Proxy.Context, callback :QuestionCallback) -> (result :Handler.Handler);
    handleShowProgress @43 (context :Proxy.Context, callback :ShowNodeProgressCallback) -> (result :Handler.Handler);
    handleNotifyNumConnectionsChanged @44 (context :Proxy.Context, callback :NotifyNumConnectionsChangedCallback) -> (result :Handler.Handler);
    handleNotifyNetworkActiveChanged @45 (context :Proxy.Context, callback :NotifyNetworkActiveChangedCallback) -> (result :Handler.Handler);
    handleNotifyAlertChanged @46 (context :Proxy.Context, callback :NotifyAlertChangedCallback) -> (result :Handler.Handler);
    handleBannedListChanged @47 (context :Proxy.Context, callback :BannedListChangedCallback) -> (result :Handler.Handler);
    handleNotifyBlockTip @48 (context :Proxy.Context, callback :NotifyBlockTipCallback) -> (result :Handler.Handler);
    handleNotifyHeaderTip @49 (context :Proxy.Context, callback :NotifyHeaderTipCallback) -> (result :Handler.Handler);
}

interface InitMessageCallback $Proxy.wrap("ProxyCallback<interfaces::Node::InitMessageFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, message :Text) -> ();
}

interface MessageBoxCallback $Proxy.wrap("ProxyCallback<interfaces::Node::MessageBoxFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, message :Common.BilingualStr, caption :Text, style :UInt32) -> (result :Bool);
}

interface QuestionCallback $Proxy.wrap("ProxyCallback<interfaces::Node::QuestionFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, message :Common.BilingualStr, nonInteractiveMessage :Text, caption :Text, style :UInt32) -> (result :Bool);
}

interface ShowNodeProgressCallback $Proxy.wrap("ProxyCallback<interfaces::Node::ShowProgressFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, title :Text, progress :Int32, resumePossible :Bool) -> ();
}

interface NotifyNumConnectionsChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Node::NotifyNumConnectionsChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, newNumConnections :Int32) -> ();
}

interface NotifyNetworkActiveChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Node::NotifyNetworkActiveChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, networkActive :Bool) -> ();
}

interface NotifyAlertChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Node::NotifyAlertChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> ();
}

interface BannedListChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Node::BannedListChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> ();
}

interface NotifyBlockTipCallback $Proxy.wrap("ProxyCallback<interfaces::Node::NotifyBlockTipFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, syncState: Int32, tip: BlockTip, verificationProgress :Float64) -> ();
}

interface NotifyHeaderTipCallback $Proxy.wrap("ProxyCallback<interfaces::Node::NotifyHeaderTipFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, syncState: Int32, tip: BlockTip, verificationProgress :Float64) -> ();
}

struct ProxyType $Proxy.wrap("proxyType") {
    proxy @0 :Data;
    randomizeCredentials @1 :Bool $Proxy.name("randomize_credentials");
}

struct NodeStats $Proxy.wrap("CNodeStats") {
    nodeid @0 :Int64 $Proxy.name("nodeid");
    services @1 :UInt64 $Proxy.name("nServices");
    relayTxes @2 :Bool $Proxy.name("fRelayTxes");
    lastSend @3 :Int64 $Proxy.name("nLastSend");
    lastRecv @4 :Int64 $Proxy.name("nLastRecv");
    lastTXTime @5 :Int64 $Proxy.name("nLastTXTime");
    lastBlockTime @6 :Int64 $Proxy.name("nLastBlockTime");
    timeConnected @7 :Int64 $Proxy.name("nTimeConnected");
    timeOffset @8 :Int64 $Proxy.name("nTimeOffset");
    addrName @9 :Text $Proxy.name("addrName");
    version @10 :Int32 $Proxy.name("nVersion");
    cleanSubVer @11 :Text $Proxy.name("cleanSubVer");
    inbound @12 :Bool $Proxy.name("fInbound");
    bip152HighbandwidthTo @13 :Bool $Proxy.name("m_bip152_highbandwidth_to");
    bip152HighbandwidthFrom @14 :Bool $Proxy.name("m_bip152_highbandwidth_from");
    startingHeight @15 :Int32 $Proxy.name("m_starting_height");
    sendBytes @16 :UInt64 $Proxy.name("nSendBytes");
    sendBytesPerMsgCmd @17 :List(Common.PairStr64) $Proxy.name("mapSendBytesPerMsgCmd");
    recvBytes @18 :UInt64 $Proxy.name("nRecvBytes");
    recvBytesPerMsgCmd @19 :List(Common.PairStr64) $Proxy.name("mapRecvBytesPerMsgCmd");
    permissionFlags @20 :Int32 $Proxy.name("m_permissionFlags");
    pingTime @21 :Int64 $Proxy.name("m_last_ping_time");
    minPingTime @22 :Int64 $Proxy.name("m_min_ping_time");
    minFeeFilter @23 :Int64 $Proxy.name("minFeeFilter");
    addrLocal @24 :Text $Proxy.name("addrLocal");
    addr @25 :Data $Proxy.name("addr");
    addrBind @26 :Data $Proxy.name("addrBind");
    network @27 :Int32 $Proxy.name("m_network");
    mappedAs @28 :UInt32 $Proxy.name("m_mapped_as");
    connType @29 :Int32 $Proxy.name("m_conn_type");
    stateStats @30 :NodeStateStats $Proxy.skip;
}

struct NodeStateStats $Proxy.wrap("CNodeStateStats") {
    syncHeight @0 :Int32 $Proxy.name("nSyncHeight");
    commonHeight @1 :Int32 $Proxy.name("nCommonHeight");
    startingHeight @2 :Int32 $Proxy.name("m_starting_height");
    pingWait @3 :Int64 $Proxy.name("m_ping_wait");
    heightInFlight @4 :List(Int32) $Proxy.name("vHeightInFlight");
}

struct BlockTip $Proxy.wrap("interfaces::BlockTip") {
    blockHeight @0 :Int32 $Proxy.name("block_height");
    blockTime @1 :Int64 $Proxy.name("block_time");
    blockHash @2 :Data $Proxy.name("block_hash");
}

struct BlockAndHeaderTipInfo $Proxy.wrap("interfaces::BlockAndHeaderTipInfo") {
    blockHeight @0 :Int32 $Proxy.name("block_height");
    blockTime @1 :Int64 $Proxy.name("block_time");
    headerHeight @2 :Int32 $Proxy.name("header_height");
    headerTime @3 :Int64 $Proxy.name("header_time");
    verificationProgress @4 :Float64 $Proxy.name("verification_progress");
}
