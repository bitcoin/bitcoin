# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

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
    getNewDestination @10 (context :Proxy.Context, outputType :Int32, label :Text) -> (result :Common.Result(TxDestination));
    getPubKey @11 (context :Proxy.Context, script :Data, address :Data) -> (pubKey :Data, result :Bool);
    signMessage @12 (context :Proxy.Context, message :Text, pkhash :Data) -> (signature :Text, result :Int32);
    isSpendable @13 (context :Proxy.Context, dest :TxDestination) -> (result :Bool);
    setAddressBook @14 (context :Proxy.Context, dest :TxDestination, name :Text, purpose :Int32) -> (result :Bool);
    delAddressBook @15 (context :Proxy.Context, dest :TxDestination) -> (result :Bool);
    getAddress @16 (context :Proxy.Context, dest :TxDestination, wantName :Bool, wantPurpose :Bool) -> (name :Text, purpose :Int32, result :Bool);
    getAddresses @17 (context :Proxy.Context) -> (result :List(WalletAddress));
    getAddressReceiveRequests @18 (context :Proxy.Context) -> (result :List(Data));
    setAddressReceiveRequest @19 (context :Proxy.Context, dest :TxDestination, id :Data, value :Data) -> (result :Bool);
    displayAddress @20 (context :Proxy.Context, dest :TxDestination) -> (result :Common.ResultVoid);
    lockCoin @21 (context :Proxy.Context, output :Data, writeToDb :Bool) -> (result :Bool);
    unlockCoin @22 (context :Proxy.Context, output :Data) -> (result :Bool);
    isLockedCoin @23 (context :Proxy.Context, output :Data) -> (result :Bool);
    listLockedCoins @24 (context :Proxy.Context) -> (outputs :List(Data));
    createTransaction @25 (context :Proxy.Context, recipients :List(Recipient), coinControl :CoinControl, sign :Bool, changePos :Int32) -> (changePos :Int32, fee :Int64, result :Common.Result(Data));
    commitTransaction @26 (context :Proxy.Context, tx :Data, valueMap :List(Common.Pair(Text, Text)), orderForm :List(Common.Pair(Text, Text))) -> ();
    transactionCanBeAbandoned @27 (context :Proxy.Context, txid :Data) -> (result :Bool);
    abandonTransaction @28 (context :Proxy.Context, txid :Data) -> (result :Bool);
    transactionCanBeBumped @29 (context :Proxy.Context, txid :Data) -> (result :Bool);
    createBumpTransaction @30 (context :Proxy.Context, txid :Data, coinControl :CoinControl) -> (errors :List(Common.BilingualStr), oldFee :Int64, newFee :Int64, mtx :Data, result :Bool);
    signBumpTransaction @31 (context :Proxy.Context, mtx :Data) -> (mtx :Data, result :Bool);
    commitBumpTransaction @32 (context :Proxy.Context, txid :Data, mtx :Data) -> (errors :List(Common.BilingualStr), bumpedTxid :Data, result :Bool);
    getTx @33 (context :Proxy.Context, txid :Data) -> (result :Data);
    getWalletTx @34 (context :Proxy.Context, txid :Data) -> (result :WalletTx);
    getWalletTxs @35 (context :Proxy.Context) -> (result :List(WalletTx));
    tryGetTxStatus @36 (context :Proxy.Context, txid :Data) -> (txStatus :WalletTxStatus, numBlocks :Int32, blockTime :Int64, result :Bool);
    getWalletTxDetails @37 (context :Proxy.Context, txid :Data) -> (txStatus :WalletTxStatus, orderForm :List(Common.Pair(Text, Text)), inMempool :Bool, numBlocks :Int32, result :WalletTx);
    getBalances @38 (context :Proxy.Context) -> (result :WalletBalances);
    fillPSBT @39 (context :Proxy.Context, sighashType :Int32, sign :Bool, bip32derivs :Bool, wantNSigned :Bool) -> (nSigned: UInt64, psbt :Data, complete :Bool, result :Int32);
    tryGetBalances @40 (context :Proxy.Context) -> (balances :WalletBalances, blockHash :Data, result :Bool);
    getBalance @41 (context :Proxy.Context) -> (result :Int64);
    getAvailableBalance @42 (context :Proxy.Context, coinControl :CoinControl) -> (result :Int64);
    txinIsMine @43 (context :Proxy.Context, txin :Data) -> (result :Bool);
    txoutIsMine @44 (context :Proxy.Context, txout :Data) -> (result :Bool);
    getDebit @45 (context :Proxy.Context, txin :Data) -> (result :Int64);
    getCredit @46 (context :Proxy.Context, txout :Data) -> (result :Int64);
    listCoins @47 (context :Proxy.Context) -> (result :List(Common.Pair(TxDestination, List(Common.Pair(Data, WalletTxOut)))));
    getCoins @48 (context :Proxy.Context, outputs :List(Data)) -> (result :List(WalletTxOut));
    getRequiredFee @49 (context :Proxy.Context, txBytes :UInt32) -> (result :Int64);
    getMinimumFee @50 (context :Proxy.Context, txBytes :UInt32, coinControl :CoinControl, wantReturnedTarget :Bool, wantReason :Bool) -> (returnedTarget :Int32, reason :Int32, result :Int64);
    getConfirmTarget @51 (context :Proxy.Context) -> (result :UInt32);
    hdEnabled @52 (context :Proxy.Context) -> (result :Bool);
    canGetAddresses @53 (context :Proxy.Context) -> (result :Bool);
    privateKeysDisabled @54 (context :Proxy.Context) -> (result :Bool);
    taprootEnabled @55 (context :Proxy.Context) -> (result :Bool);
    hasExternalSigner @56 (context :Proxy.Context) -> (result :Bool);
    getDefaultAddressType @57 (context :Proxy.Context) -> (result :Int32);
    getDefaultMaxTxFee @58 (context :Proxy.Context) -> (result :Int64);
    remove @59 (context :Proxy.Context) -> ();
    handleUnload @60 (context :Proxy.Context, callback :UnloadWalletCallback) -> (result :Handler.Handler);
    handleShowProgress @61 (context :Proxy.Context, callback :ShowWalletProgressCallback) -> (result :Handler.Handler);
    handleStatusChanged @62 (context :Proxy.Context, callback :StatusChangedCallback) -> (result :Handler.Handler);
    handleAddressBookChanged @63 (context :Proxy.Context, callback :AddressBookChangedCallback) -> (result :Handler.Handler);
    handleTransactionChanged @64 (context :Proxy.Context, callback :TransactionChangedCallback) -> (result :Handler.Handler);
    handleCanGetAddressesChanged @65 (context :Proxy.Context, callback :CanGetAddressesChangedCallback) -> (result :Handler.Handler);
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
    call @1 (context :Proxy.Context, address :TxDestination, label :Text, isMine :Bool, purpose :Int32, status :Int32) -> ();
}

interface TransactionChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Wallet::TransactionChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, txid :Data, status :Int32) -> ();
}

interface CanGetAddressesChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Wallet::CanGetAddressesChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> ();
}

interface WalletLoader extends(Chain.ChainClient) $Proxy.wrap("interfaces::WalletLoader") {
    createWallet @0 (context :Proxy.Context, name :Text, passphrase :Text, flags :UInt64) -> (warning :List(Common.BilingualStr), result :Common.Result(Wallet));
    loadWallet @1 (context :Proxy.Context, name :Text) -> (warning :List(Common.BilingualStr), result :Common.Result(Wallet));
    getWalletDir @2 (context :Proxy.Context) -> (result :Text);
    restoreWallet @3 (context :Proxy.Context, backupFile :Data, name :Text, loadAfterRestore :Bool) -> (warning :List(Common.BilingualStr), result :Common.Result(Wallet));
    migrateWallet @4 (context :Proxy.Context, name :Text, passphrase :Text) -> (result :Common.Result(WalletMigrationResult));
    isEncrypted @5 (context :Proxy.Context, name :Text) -> (result : Bool);
    listWalletDir @6 (context :Proxy.Context) -> (result :List(Common.Pair(Text, Text)));
    getWallets @7 (context :Proxy.Context) -> (result :List(Wallet));
    handleLoadWallet @8 (context :Proxy.Context, callback :LoadWalletCallback) -> (result :Handler.Handler);
}

interface LoadWalletCallback $Proxy.wrap("ProxyCallback<interfaces::WalletLoader::LoadWalletFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, wallet :Wallet) -> ();
}

struct Key {
    secret @0 :Data;
    isCompressed @1 :Bool;
}

struct TxDestination {
    pubKey @0 :Data;
    pkHash @1 :Data;
    scriptHash @2 :Data;
    witnessV0ScriptHash @3 :Data;
    witnessV0KeyHash @4 :Data;
    witnessV1Taproot @5 :Data;
    witnessUnknown @6 :WitnessUnknown;
}

struct WitnessUnknown
{
    version @0 :UInt32;
    program @1 :Data;
}

struct WalletAddress $Proxy.wrap("interfaces::WalletAddress") {
    dest @0 :TxDestination;
    isMine @1 :Bool $Proxy.name("is_mine");
    name @2 :Text;
    purpose @3 :Int32;
}

struct Recipient $Proxy.wrap("wallet::CRecipient") {
    dest @0 :TxDestination;
    amount @1 :Int64 $Proxy.name("nAmount");
    subtractFeeFromAmount @2 :Bool $Proxy.name("fSubtractFeeFromAmount");
}

struct CoinControl {
    destChange @0 :TxDestination;
    hasChangeType @1 :Bool;
    changeType @2 :Int32;
    includeUnsafeInputs @3 :Bool;
    allowOtherInputs @4 :Bool;
    allowWatchOnly @5 :Bool;
    overrideFeeRate @6 :Bool;
    hasFeeRate @7 :Bool;
    feeRate @8 :Data;
    hasConfirmTarget @9 :Bool;
    confirmTarget @10 :Int32;
    hasSignalRbf @11 :Bool;
    signalRbf @12 :Bool;
    avoidPartialSpends @13 :Bool;
    avoidAddressReuse @14 :Bool;
    feeMode @15 :Int32;
    minDepth @16 :Int32;
    maxDepth @17 :Int32;
    # Note: The corresponding CCoinControl class has a FlatSigningProvider
    # m_external_provider member here, but it is intentionally not included in
    # this capnp struct because it is not needed by the GUI. Probably the
    # CCoinControl class should be split up to separate options which are
    # intended to be set by GUI and RPC interfaces from members containing
    # internal wallet data that just happens to be stored in the CCoinControl
    # object because it is convenient to pass around.
    hasLockTime @18 :UInt32;
    lockTime @19 :UInt32;
    hasVersion @20 :UInt32;
    version @21 :UInt32;
    hasMaxTxWeight @22 :UInt32;
    maxTxWeight @23 :UInt32;
    setSelected @24 :List(Data);
}

struct WalletTx $Proxy.wrap("interfaces::WalletTx") {
    tx @0 :Data;
    txinIsMine @1 :List(Bool) $Proxy.name("txin_is_mine");
    txoutIsMine @2 :List(Bool) $Proxy.name("txout_is_mine");
    txoutIsChange @3 :List(Bool) $Proxy.name("txout_is_change");
    txoutAddress @4 :List(TxDestination) $Proxy.name("txout_address");
    txoutAddressIsMine @5 :List(Bool) $Proxy.name("txout_address_is_mine");
    credit @6 :Int64;
    debit @7 :Int64;
    change @8 :Int64;
    time @9 :Int64;
    valueMap @10 :List(Common.Pair(Text, Text)) $Proxy.name("value_map");
    isCoinbase @11 :Bool $Proxy.name("is_coinbase");
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
    isTrusted @5 :Bool $Proxy.name("is_trusted");
    isAbandoned @6 :Bool $Proxy.name("is_abandoned");
    isCoinbase @7 :Bool $Proxy.name("is_coinbase");
    isInMainChain @8 :Bool $Proxy.name("is_in_main_chain");
}

struct WalletBalances $Proxy.wrap("interfaces::WalletBalances") {
    balance @0 :Int64;
    unconfirmedBalance @1 :Int64 $Proxy.name("unconfirmed_balance");
    immatureBalance @2 :Int64 $Proxy.name("immature_balance");
}

struct WalletMigrationResult $Proxy.wrap("interfaces::WalletMigrationResult") {
    wallet @0 :Wallet;
    watchonlyWalletName @1 :Text $Proxy.name("watchonly_wallet_name");
    solvablesWalletName @2 :Text $Proxy.name("solvables_wallet_name");
    backupPath @3 :Text $Proxy.name("backup_path");
}
