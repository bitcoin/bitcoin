# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xe234cce74feea00c;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("ipc/capnp/wallet.h");
$Proxy.includeTypes("ipc/capnp/wallet-types.h");

using Chain = import "chain.capnp";
using Common = import "common.capnp";
using Handler = import "handler.capnp";

interface Wallet $Proxy.wrap("interfaces::Wallet") {
    destroy @0 (context :Proxy.Context) -> ();
    encryptWallet @1 (context :Proxy.Context, walletPassphrase :Data) -> (result :Bool);
    isCrypted @2 (context :Proxy.Context) -> (result :Bool);
    lock @3 (context :Proxy.Context) -> (result :Bool);
    unlock @4 (context :Proxy.Context, walletPassphrase :Data) -> (result :Bool);
    isLocked @5 (context :Proxy.Context) -> (result :Bool);
    changeWalletPassphrase @6 (context :Proxy.Context, oldWalletPassphrase :Data, newWalletPassphrase :Data) -> (result :Bool);
    abortRescan @7 (context :Proxy.Context) -> ();
    backupWallet @8 (context :Proxy.Context, filename :Text) -> (result :Bool);
    getWalletName @9 (context :Proxy.Context) -> (result :Text);
    getNewDestination @10 (context :Proxy.Context, outputType :Int32, label :Text) -> (dest :TxDestination, result :Bool);
    getPubKey @11 (context :Proxy.Context, script :Data, address :Data) -> (pubKey :Data, result :Bool);
    signMessage @12 (context :Proxy.Context, message :Text, pkhash :Data) -> (signature :Text, result :Int32);
    isSpendable @13 (context :Proxy.Context, dest :TxDestination) -> (result :Bool);
    haveWatchOnly @14 (context :Proxy.Context) -> (result :Bool);
    setAddressBook @15 (context :Proxy.Context, dest :TxDestination, name :Text, purpose :Text) -> (result :Bool);
    delAddressBook @16 (context :Proxy.Context, dest :TxDestination) -> (result :Bool);
    getAddress @17 (context :Proxy.Context, dest :TxDestination, wantName :Bool, wantIsMine :Bool, wantPurpose :Bool) -> (name :Text, isMine :Int32, purpose :Text, result :Bool);
    getAddresses @18 (context :Proxy.Context) -> (result :List(WalletAddress));
    getAddressReceiveRequests @19 (context :Proxy.Context) -> (result :List(Data));
    setAddressReceiveRequest @20 (context :Proxy.Context, dest :TxDestination, id :Data, value :Data) -> (result :Bool);
    displayAddress @21 (context :Proxy.Context, dest :TxDestination) -> (result :Bool);
    lockCoin @22 (context :Proxy.Context, output :Data) -> ();
    unlockCoin @23 (context :Proxy.Context, output :Data) -> ();
    isLockedCoin @24 (context :Proxy.Context, output :Data) -> (result :Bool);
    listLockedCoins @25 (context :Proxy.Context) -> (outputs :List(Data));
    createTransaction @26 (context :Proxy.Context, recipients :List(Recipient), coinControl :CoinControl, sign :Bool, changePos :Int32) -> (changePos :Int32, fee :Int64, failReason :Common.BilingualStr, result :Data);
    commitTransaction @27 (context :Proxy.Context, tx :Data, valueMap :List(Common.Pair(Text, Text)), orderForm :List(Common.Pair(Text, Text))) -> ();
    transactionCanBeAbandoned @28 (context :Proxy.Context, txid :Data) -> (result :Bool);
    abandonTransaction @29 (context :Proxy.Context, txid :Data) -> (result :Bool);
    transactionCanBeBumped @30 (context :Proxy.Context, txid :Data) -> (result :Bool);
    createBumpTransaction @31 (context :Proxy.Context, txid :Data, coinControl :CoinControl) -> (errors :List(Common.BilingualStr), oldFee :Int64, newFee :Int64, mtx :Data, result :Bool);
    signBumpTransaction @32 (context :Proxy.Context, mtx :Data) -> (mtx :Data, result :Bool);
    commitBumpTransaction @33 (context :Proxy.Context, txid :Data, mtx :Data) -> (errors :List(Common.BilingualStr), bumpedTxid :Data, result :Bool);
    getTx @34 (context :Proxy.Context, txid :Data) -> (result :Data);
    getWalletTx @35 (context :Proxy.Context, txid :Data) -> (result :WalletTx);
    getWalletTxs @36 (context :Proxy.Context) -> (result :List(WalletTx));
    tryGetTxStatus @37 (context :Proxy.Context, txid :Data) -> (txStatus :WalletTxStatus, numBlocks :Int32, blockTime :Int64, result :Bool);
    getWalletTxDetails @38 (context :Proxy.Context, txid :Data) -> (txStatus :WalletTxStatus, orderForm :List(Common.Pair(Text, Text)), inMempool :Bool, numBlocks :Int32, result :WalletTx);
    getBalances @39 (context :Proxy.Context) -> (result :WalletBalances);
    fillPSBT @40 (context :Proxy.Context, sighashType :Int32, sign :Bool, bip32derivs :Bool, wantNSigned :Bool) -> (nSigned: UInt64, psbt :Data, complete :Bool, result :Int32);
    tryGetBalances @41 (context :Proxy.Context) -> (balances :WalletBalances, blockHash :Data, result :Bool);
    getBalance @42 (context :Proxy.Context) -> (result :Int64);
    getAvailableBalance @43 (context :Proxy.Context, coinControl :CoinControl) -> (result :Int64);
    txinIsMine @44 (context :Proxy.Context, txin :Data) -> (result :Int32);
    txoutIsMine @45 (context :Proxy.Context, txout :Data) -> (result :Int32);
    getDebit @46 (context :Proxy.Context, txin :Data, filter :Int32) -> (result :Int64);
    getCredit @47 (context :Proxy.Context, txout :Data, filter :Int32) -> (result :Int64);
    listCoins @48 (context :Proxy.Context) -> (result :List(Common.Pair(TxDestination, List(Common.Pair(Data, WalletTxOut)))));
    getCoins @49 (context :Proxy.Context, outputs :List(Data)) -> (result :List(WalletTxOut));
    getRequiredFee @50 (context :Proxy.Context, txBytes :UInt32) -> (result :Int64);
    getMinimumFee @51 (context :Proxy.Context, txBytes :UInt32, coinControl :CoinControl, wantReturnedTarget :Bool, wantReason :Bool) -> (returnedTarget :Int32, reason :Int32, result :Int64);
    getConfirmTarget @52 (context :Proxy.Context) -> (result :UInt32);
    hdEnabled @53 (context :Proxy.Context) -> (result :Bool);
    canGetAddresses @54 (context :Proxy.Context) -> (result :Bool);
    privateKeysDisabled @55 (context :Proxy.Context) -> (result :Bool);
    hasExternalSigner @56 (context :Proxy.Context) -> (result :Bool);
    getDefaultAddressType @57 (context :Proxy.Context) -> (result :Int32);
    getDefaultMaxTxFee @58 (context :Proxy.Context) -> (result :Int64);
    remove @59 (context :Proxy.Context) -> ();
    isLegacy @60 (context :Proxy.Context) -> (result :Bool);
    handleUnload @61 (context :Proxy.Context, callback :UnloadWalletCallback) -> (result :Handler.Handler);
    handleShowProgress @62 (context :Proxy.Context, callback :ShowWalletProgressCallback) -> (result :Handler.Handler);
    handleStatusChanged @63 (context :Proxy.Context, callback :StatusChangedCallback) -> (result :Handler.Handler);
    handleAddressBookChanged @64 (context :Proxy.Context, callback :AddressBookChangedCallback) -> (result :Handler.Handler);
    handleTransactionChanged @65 (context :Proxy.Context, callback :TransactionChangedCallback) -> (result :Handler.Handler);
    handleWatchOnlyChanged @66 (context :Proxy.Context, callback :WatchOnlyChangedCallback) -> (result :Handler.Handler);
    handleCanGetAddressesChanged @67 (context :Proxy.Context, callback :CanGetAddressesChangedCallback) -> (result :Handler.Handler);
}

interface UnloadWalletCallback $Proxy.wrap("ProxyCallback<interfaces::Wallet::UnloadFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> ();
}

interface ShowWalletProgressCallback $Proxy.wrap("ProxyCallback<interfaces::Wallet::ShowProgressFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, title :Text, progress :Int32) -> ();
}

interface StatusChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Wallet::StatusChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> ();
}

interface AddressBookChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Wallet::AddressBookChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, address :TxDestination, label :Text, isMine :Bool, purpose :Text, status :Int32) -> ();
}

interface TransactionChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Wallet::TransactionChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, txid :Data, status :Int32) -> ();
}

interface WatchOnlyChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Wallet::WatchOnlyChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, haveWatchOnly :Bool) -> ();
}

interface CanGetAddressesChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Wallet::CanGetAddressesChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> ();
}

interface ExternalSigner $Proxy.wrap("interfaces::ExternalSigner") {
    destroy @0 (context :Proxy.Context) -> ();
    getName @1 (context :Proxy.Context) -> (result :Text);
}

interface WalletClient extends(Chain.ChainClient) $Proxy.wrap("interfaces::WalletClient") {
    createWallet @0 (context :Proxy.Context, name :Text, passphrase :Text, flags :UInt64) -> (error :Common.BilingualStr, warning :List(Common.BilingualStr), result :Wallet);
    loadWallet @1 (context :Proxy.Context, name :Text) -> (error :Common.BilingualStr, warning :List(Common.BilingualStr), result :Wallet);
    getWalletDir @2 (context :Proxy.Context) -> (result :Text);
    listWalletDir @3 (context :Proxy.Context) -> (result :List(Text));
    getWallets @4 (context :Proxy.Context) -> (result :List(Wallet));
    listExternalSigners @5 (context :Proxy.Context) -> (result :List(ExternalSigner));
    handleLoadWallet @6 (context :Proxy.Context, callback :LoadWalletCallback) -> (result :Handler.Handler);
}

interface LoadWalletCallback $Proxy.wrap("ProxyCallback<interfaces::WalletClient::LoadWalletFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, wallet :Wallet) -> ();
}

struct Key {
    secret @0 :Data;
    isCompressed @1 :Bool;
}

struct TxDestination {
    pkHash @0 :Data;
    scriptHash @1 :Data;
    witnessV0ScriptHash @2 :Data;
    witnessV0KeyHash @3 :Data;
    witnessUnknown @4 :WitnessUnknown;
}

struct WitnessUnknown $Proxy.wrap("WitnessUnknown")
{
    version @0 :UInt32;
    length @1 :UInt32;
    program @2 :Data;
}

struct WalletAddress $Proxy.wrap("interfaces::WalletAddress") {
    dest @0 :TxDestination;
    isMine @1 :Int32 $Proxy.name("is_mine");
    name @2 :Text;
    purpose @3 :Text;
}

struct Recipient $Proxy.wrap("CRecipient") {
    scriptPubKey @0 :Data;
    amount @1 :Int64 $Proxy.name("nAmount");
    subtractFeeFromAmount @2 :Bool $Proxy.name("fSubtractFeeFromAmount");
}

struct CoinControl {
    destChange @0 :TxDestination;
    hasChangeType @1 :Bool;
    changeType @2 :Int32;
    allowOtherInputs @3 :Bool;
    allowWatchOnly @4 :Bool;
    overrideFeeRate @5 :Bool;
    hasFeeRate @6 :Bool;
    feeRate @7 :Data;
    hasConfirmTarget @8 :Bool;
    confirmTarget @9 :Int32;
    hasSignalRbf @10 :Bool;
    signalRbf @11 :Bool;
    feeMode @12 :Int32;
    minDepth @13 :Int32;
    setSelected @14 :List(Data);
}

struct WalletTx $Proxy.wrap("interfaces::WalletTx") {
    tx @0 :Data;
    txinIsMine @1 :List(Int32) $Proxy.name("txin_is_mine");
    txoutIsMine @2 :List(Int32) $Proxy.name("txout_is_mine");
    txoutAddress @3 :List(TxDestination) $Proxy.name("txout_address");
    txoutAddressIsMine @4 :List(Int32) $Proxy.name("txout_address_is_mine");
    credit @5 :Int64;
    debit @6 :Int64;
    change @7 :Int64;
    time @8 :Int64;
    valueMap @9 :List(Common.Pair(Text, Text)) $Proxy.name("value_map");
    isCoinbase @10 :Bool $Proxy.name("is_coinbase");
}

struct WalletTxOut $Proxy.wrap("interfaces::WalletTxOut") {
    txout @0 :Data;
    time @1 :Int64;
    depthInMainChain @2 :Int32 $Proxy.name("depth_in_main_chain");
    isSpent @3 :Bool $Proxy.name("is_spent");
}

struct WalletTxStatus $Proxy.wrap("interfaces::WalletTxStatus") {
    blockHeight @0 :Int32 $Proxy.name("block_height");
    blocksToMaturity @1 :Int32 $Proxy.name("blocks_to_maturity");
    depthInMainChain @2 :Int32 $Proxy.name("depth_in_main_chain");
    timeReceived @3 :UInt32 $Proxy.name("time_received");
    lockTime @4 :UInt32 $Proxy.name("lock_time");
    isFinal @5 :Bool $Proxy.name("is_final");
    isTrusted @6 :Bool $Proxy.name("is_trusted");
    isAbandoned @7 :Bool $Proxy.name("is_abandoned");
    isCoinbase @8 :Bool $Proxy.name("is_coinbase");
    isInMainChain @9 :Bool $Proxy.name("is_in_main_chain");
}

struct WalletBalances $Proxy.wrap("interfaces::WalletBalances") {
    balance @0 :Int64;
    unconfirmedBalance @1 :Int64 $Proxy.name("unconfirmed_balance");
    immatureBalance @2 :Int64 $Proxy.name("immature_balance");
    haveWatchOnly @3 :Bool $Proxy.name("have_watch_only");
    watchOnlyBalance @4 :Int64 $Proxy.name("watch_only_balance");
    unconfirmedWatchOnlyBalance @5 :Int64 $Proxy.name("unconfirmed_watch_only_balance");
    immatureWatchOnlyBalance @6 :Int64 $Proxy.name("immature_watch_only_balance");
}
