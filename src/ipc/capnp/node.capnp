# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

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
    getLogCategories @4 (context :Proxy.Context) -> (result :UInt64);
    getExitStatus @5 (context :Proxy.Context) -> (result :Int32);
    baseInitialize @6 (context :Proxy.Context, globalArgs :Common.GlobalArgs) -> (error :Text $Proxy.exception("std::exception"), result :Bool);
    appInitMain @7 (context :Proxy.Context) -> (tipInfo :BlockAndHeaderTipInfo, error :Text $Proxy.exception("std::exception"), result :Bool);
    appShutdown @8 (context :Proxy.Context) -> ();
    startShutdown @9 (context :Proxy.Context) -> ();
    shutdownRequested @10 (context :Proxy.Context) -> (result :Bool);
    isSettingIgnored @11 (name :Text) -> (result: Bool);
    getPersistentSetting @12 (name :Text) -> (result: Text);
    updateRwSetting @13 (name :Text, value :Text) -> ();
    forceSetting @14 (name :Text, value :Text) -> ();
    resetSettings @15 () -> ();
    mapPort @16 (context :Proxy.Context, useNatPnP :Bool) -> ();
    getProxy @17 (context :Proxy.Context, net :Int32) -> (proxyInfo :ProxyInfo, result :Bool);
    getNodeCount @18 (context :Proxy.Context, flags :Int32) -> (result :UInt64);
    getNodesStats @19 (context :Proxy.Context) -> (stats :List(NodeStats), result :Bool);
    getBanned @20 (context :Proxy.Context) -> (banmap :Banmap, result :Bool);
    ban @21 (context :Proxy.Context, netAddr :Data, banTimeOffset :Int64) -> (result :Bool);
    unban @22 (context :Proxy.Context, ip :Data) -> (result :Bool);
    disconnectByAddress @23 (context :Proxy.Context, address :Data) -> (result :Bool);
    disconnectById @24 (context :Proxy.Context, id :Int64) -> (result :Bool);
    listExternalSigners @25 (context :Proxy.Context) -> (result :List(ExternalSigner));
    getTotalBytesRecv @26 (context :Proxy.Context) -> (result :Int64);
    getTotalBytesSent @27 (context :Proxy.Context) -> (result :Int64);
    getMempoolSize @28 (context :Proxy.Context) -> (result :UInt64);
    getMempoolDynamicUsage @29 (context :Proxy.Context) -> (result :UInt64);
    getMempoolMaxUsage @30 (context :Proxy.Context) -> (result :UInt64);
    getHeaderTip @31 (context :Proxy.Context) -> (height :Int32, blockTime :Int64, result :Bool);
    getNumBlocks @32 (context :Proxy.Context) -> (result :Int32);
    getNetLocalAddresses @33 (context :Proxy.Context) -> (result :List(Common.Pair(Data, LocalServiceInfo)));
    getBestBlockHash @34 (context :Proxy.Context) -> (result :Data);
    getLastBlockTime @35 (context :Proxy.Context) -> (result :Int64);
    getVerificationProgress @36 (context :Proxy.Context) -> (result :Float64);
    isInitialBlockDownload @37 (context :Proxy.Context) -> (result :Bool);
    isLoadingBlocks @38 (context :Proxy.Context) -> (result :Bool);
    setNetworkActive @39 (context :Proxy.Context, active :Bool) -> ();
    getNetworkActive @40 (context :Proxy.Context) -> (result :Bool);
    getDustRelayFee @41 (context :Proxy.Context) -> (result :Data);
    executeRpc @42 (context :Proxy.Context, command :Text, params :Text, uri :Text) -> (error :Text $Proxy.exception("std::exception"), rpcError :Text $Proxy.exception("UniValue"), result :Text);
    listRpcCommands @43 (context :Proxy.Context) -> (result :List(Text));
    getUnspentOutput @44 (context :Proxy.Context, output :Data) -> (result :Data);
    broadcastTransaction @45 (context :Proxy.Context, tx: Data, maxTxFee :Int64) -> (error: Text, result :Int32);
    customWalletLoader @46 (context :Proxy.Context) -> (result :Wallet.WalletLoader) $Proxy.name("walletLoader");
    handleInitMessage @47 (context :Proxy.Context, callback :InitMessageCallback) -> (result :Handler.Handler);
    handleMessageBox @48 (context :Proxy.Context, callback :MessageBoxCallback) -> (result :Handler.Handler);
    handleQuestion @49 (context :Proxy.Context, callback :QuestionCallback) -> (result :Handler.Handler);
    handleShowProgress @50 (context :Proxy.Context, callback :ShowNodeProgressCallback) -> (result :Handler.Handler);
    handleInitWallet @51 (context :Proxy.Context, callback :InitWalletCallback) -> (result :Handler.Handler);
    handleNotifyNumConnectionsChanged @52 (context :Proxy.Context, callback :NotifyNumConnectionsChangedCallback) -> (result :Handler.Handler);
    handleNotifyNetworkActiveChanged @53 (context :Proxy.Context, callback :NotifyNetworkActiveChangedCallback) -> (result :Handler.Handler);
    handleNotifyAlertChanged @54 (context :Proxy.Context, callback :NotifyAlertChangedCallback) -> (result :Handler.Handler);
    handleBannedListChanged @55 (context :Proxy.Context, callback :BannedListChangedCallback) -> (result :Handler.Handler);
    handleNotifyBlockTip @56 (context :Proxy.Context, callback :NotifyBlockTipCallback) -> (result :Handler.Handler);
    handleNotifyHeaderTip @57 (context :Proxy.Context, callback :NotifyHeaderTipCallback) -> (result :Handler.Handler);
}

interface ExternalSigner $Proxy.wrap("interfaces::ExternalSigner") {
    destroy @0 (context :Proxy.Context) -> ();
    getName @1 (context :Proxy.Context) -> (result :Text);
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

interface InitWalletCallback $Proxy.wrap("ProxyCallback<interfaces::Node::InitWalletFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> ();
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

struct ProxyInfo $Proxy.wrap("::Proxy") {
    proxy @0 :Data;
    torStreamIsolation @1 :Bool $Proxy.name("m_tor_stream_isolation");
}

struct NodeStats $Proxy.wrap("CNodeStats") {
    nodeid @0 :Int64 $Proxy.name("nodeid");
    lastSend @1 :Int64 $Proxy.name("m_last_send");
    lastRecv @2 :Int64 $Proxy.name("m_last_recv");
    lastTXTime @3 :Int64 $Proxy.name("m_last_tx_time");
    lastBlockTime @4 :Int64 $Proxy.name("m_last_block_time");
    timeConnected @5 :Int64 $Proxy.name("m_connected");
    addrName @6 :Text $Proxy.name("m_addr_name");
    version @7 :Int32 $Proxy.name("nVersion");
    cleanSubVer @8 :Text $Proxy.name("cleanSubVer");
    inbound @9 :Bool $Proxy.name("fInbound");
    bip152HighbandwidthTo @10 :Bool $Proxy.name("m_bip152_highbandwidth_to");
    bip152HighbandwidthFrom @11 :Bool $Proxy.name("m_bip152_highbandwidth_from");
    startingHeight @12 :Int32 $Proxy.name("m_starting_height");
    sendBytes @13 :UInt64 $Proxy.name("nSendBytes");
    sendBytesPerMsgType @14 :List(Common.PairUInt64(Text)) $Proxy.name("mapSendBytesPerMsgType");
    recvBytes @15 :UInt64 $Proxy.name("nRecvBytes");
    recvBytesPerMsgType @16 :List(Common.PairUInt64(Text)) $Proxy.name("mapRecvBytesPerMsgType");
    permissionFlags @17 :Int32 $Proxy.name("m_permission_flags");
    pingTime @18 :Int64 $Proxy.name("m_last_ping_time");
    minPingTime @19 :Int64 $Proxy.name("m_min_ping_time");
    addrLocal @20 :Text $Proxy.name("addrLocal");
    addr @21 :Data $Proxy.name("addr");
    addrBind @22 :Data $Proxy.name("addrBind");
    network @23 :Int32 $Proxy.name("m_network");
    mappedAs @24 :UInt32 $Proxy.name("m_mapped_as");
    connType @25 :Int32 $Proxy.name("m_conn_type");
    transportType @26 :UInt8 $Proxy.name("m_transport_type;");
    sessionId @27 :Text $Proxy.name("m_session_id");
    stateStats @28 :NodeStateStats $Proxy.skip;
}

struct NodeStateStats $Proxy.wrap("CNodeStateStats") {
    syncHeight @0 :Int32 $Proxy.name("nSyncHeight");
    commonHeight @1 :Int32 $Proxy.name("nCommonHeight");
    startingHeight @2 :Int32 $Proxy.name("m_starting_height");
    pingWait @3 :Int64 $Proxy.name("m_ping_wait");
    heightInFlight @4 :List(Int32) $Proxy.name("vHeightInFlight");
    addressesProcessed @5 :UInt64 $Proxy.name("m_addr_processed");
    addressesRateLimited @6 :UInt64 $Proxy.name("m_addr_rate_limited");
    addressRelayEnabled @7 :Bool $Proxy.name("m_addr_relay_enabled");
    theirServices @8 :UInt64 $Proxy.name("their_services");
    presyncHeight @9 :Int64 $Proxy.name("presync_height");
    timeOffset @10 :Int64 $Proxy.name("time_offset");
}

struct Banmap {
    json @0 :Text;
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

struct LocalServiceInfo $Proxy.wrap("LocalServiceInfo") {
    score @0 :Int32 $Proxy.name("nScore");
    port @1 :UInt16 $Proxy.name("nPort");
}
