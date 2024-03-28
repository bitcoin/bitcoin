// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/wallet.h>

#include <amount.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <policy/fees.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <script/standard.h>
#include <support/allocators/secure.h>
#include <sync.h>
#include <uint256.h>
#include <util/check.h>
#include <util/ref.h>
#include <util/system.h>
#include <util/ui_change_type.h>
#include <wallet/context.h>
#include <wallet/feebumper.h>
#include <wallet/fees.h>
#include <wallet/ismine.h>
#include <wallet/load.h>
#include <wallet/reserve.h>
#include <wallet/rpcwallet.h>
#include <wallet/txlist.h>
#include <wallet/wallet.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace interfaces {
namespace {

//! Construct wallet TxOut struct.
WalletTxOut MakeWalletTxOut(CWallet& wallet,
    const CWalletTx& wtx,
    const CTxOutput& output) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    WalletTxOut result;
    result.txout = output;
    result.output_index = output.GetIndex();
    result.time = wtx.GetTxTime();
    result.depth_in_main_chain = wtx.GetDepthInMainChain();
    result.is_spent = wallet.IsSpent(output.GetIndex());

    if (output.IsMWEB()) {
        mw::Coin coin;
        if (wallet.GetCoin(output.ToMWEB(), coin)) {
            StealthAddress addr;
            if (wallet.GetMWWallet()->GetStealthAddress(coin, addr)) {
                result.address = addr;
            }

            result.nValue = coin.amount;
        }
    } else {
        result.address = output.GetTxOut().scriptPubKey;
        result.nValue = output.GetTxOut().nValue;
    }

    return result;
}

WalletTxOut MakeWalletTxOut(CWallet& wallet, const COutputCoin& coin)
{
    WalletTxOut result;
    if (coin.IsMWEB()) {
        result.txout = CTxOutput(boost::get<mw::Hash>(coin.GetIndex()));
    } else {
        assert(!coin.GetAddress().IsMWEB());
        result.txout = CTxOutput(coin.GetIndex(), CTxOut(coin.GetValue(), coin.GetAddress().GetScript()));
    }
    result.address = coin.GetAddress();
    result.output_index = coin.GetIndex();
    result.nValue = coin.GetValue();
    result.time = coin.GetWalletTx()->GetTxTime();
    result.depth_in_main_chain = coin.GetDepth();
    result.is_spent = wallet.IsSpent(coin.GetIndex());
    return result;
}

//! Construct wallet tx struct.
WalletTx MakeWalletTx(CWallet& wallet, const CWalletTx& wtx)
{
    LOCK(wallet.cs_wallet);

    WalletTx result;
    result.tx = wtx.tx;
    result.credit = wtx.GetCredit(ISMINE_ALL);
    result.debit = wtx.GetDebit(ISMINE_ALL);
    result.change = wtx.GetChange();
    result.fee = wtx.GetFee(ISMINE_ALL);
    result.time = wtx.GetTxTime();
    result.value_map = wtx.mapValue;
    result.is_coinbase = wtx.IsCoinBase();
    result.is_hogex = wtx.IsHogEx();
    result.wtx_hash = wtx.GetHash();

    //
    // Inputs
    //
    result.inputs = wtx.GetInputs();
    result.txin_is_mine.reserve(result.inputs.size());

    for (const auto& txin : result.inputs) {
        result.txin_is_mine.emplace_back(wallet.IsMine(txin));
    }

    //
    // Outputs
    //
    auto is_address_mine = [&wallet](const CTxOutput& txout) -> isminetype {
        CTxDestination dest;
        return wallet.ExtractOutputDestination(txout, dest) ? wallet.IsMine(dest) : ISMINE_NO;
    };

    std::vector<CTxOutput> outputs = wtx.GetOutputs();
    result.txout_is_mine.reserve(outputs.size());
    result.txout_address_is_mine.reserve(outputs.size());
    result.outputs.reserve(outputs.size());

    for (const auto& txout : outputs) {
        // MWEB Peg-in output pubkey scripts are anyone-can-spend, so don't really have an owner.
        // In order for peg-ins to your own address to show up as SendToSelf txs, we need to filter these out.
        if (!txout.IsMWEB() && txout.GetScriptPubKey().IsMWEBPegin()) {
            continue;
        }

        result.txout_is_mine.emplace_back(wallet.IsMine(txout));
        result.txout_address_is_mine.emplace_back(is_address_mine(txout));
        result.outputs.push_back(MakeWalletTxOut(wallet, wtx, txout));
    }

    result.pegouts = wtx.tx->mweb_tx.GetPegOuts();
    for (const PegOutCoin& pegout : result.pegouts) {
        result.pegout_is_mine.emplace_back(wallet.IsMine(DestinationAddr(pegout.GetScriptPubKey())));
    }

    return result;
}

//! Construct wallet tx status struct.
WalletTxStatus MakeWalletTxStatus(CWallet& wallet, const CWalletTx& wtx)
{
    WalletTxStatus result;
    result.block_height = wtx.m_confirm.block_height > 0 ? wtx.m_confirm.block_height : std::numeric_limits<int>::max();
    result.blocks_to_maturity = wtx.GetBlocksToMaturity();
    result.depth_in_main_chain = wtx.GetDepthInMainChain();
    result.time_received = wtx.nTimeReceived;
    result.lock_time = wtx.tx->nLockTime;
    result.is_final = wallet.chain().checkFinalTx(*wtx.tx);
    result.is_trusted = wtx.IsTrusted();
    result.is_abandoned = wtx.isAbandoned();
    result.is_coinbase = wtx.IsCoinBase();
    result.is_hogex = wtx.IsHogEx();
    result.is_in_main_chain = wtx.IsInMainChain();
    return result;
}

class WalletImpl : public Wallet
{
public:
    explicit WalletImpl(const std::shared_ptr<CWallet>& wallet) : m_wallet(wallet) {}

    bool encryptWallet(const SecureString& wallet_passphrase) override
    {
        return m_wallet->EncryptWallet(wallet_passphrase);
    }
    bool isCrypted() override { return m_wallet->IsCrypted(); }
    bool lock() override { return m_wallet->Lock(); }
    bool unlock(const SecureString& wallet_passphrase) override { return m_wallet->Unlock(wallet_passphrase); }
    bool isLocked() override { return m_wallet->IsLocked(); }
    bool changeWalletPassphrase(const SecureString& old_wallet_passphrase,
        const SecureString& new_wallet_passphrase) override
    {
        return m_wallet->ChangeWalletPassphrase(old_wallet_passphrase, new_wallet_passphrase);
    }
    void abortRescan() override { m_wallet->AbortRescan(); }
    bool backupWallet(const std::string& filename) override { return m_wallet->BackupWallet(filename); }
    std::string getWalletName() override { return m_wallet->GetName(); }
    bool getNewDestination(const OutputType type, const std::string label, CTxDestination& dest) override
    {
        LOCK(m_wallet->cs_wallet);
        std::string error;
        return m_wallet->GetNewDestination(type, label, dest, error);
    }
    std::shared_ptr<ReserveDestination> reserveNewDestination(CTxDestination& dest) override
    {
        LOCK(m_wallet->cs_wallet);

        auto output_type = OutputType::BECH32;
        auto reserve_dest = std::make_shared<ReserveDestination>(m_wallet.get(), output_type);
        if (reserve_dest->GetReservedDestination(dest, false)) {
            return reserve_dest;
        }

        return nullptr;
    }
    bool getPubKey(const CScript& script, const CKeyID& address, CPubKey& pub_key) override
    {
        std::unique_ptr<SigningProvider> provider = m_wallet->GetSolvingProvider(script);
        if (provider) {
            return provider->GetPubKey(address, pub_key);
        }
        return false;
    }
    SigningResult signMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) override
    {
        return m_wallet->SignMessage(message, pkhash, str_sig);
    }
    bool isSpendable(const CTxDestination& dest) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->IsMine(dest) & ISMINE_SPENDABLE;
    }
    bool haveWatchOnly() override
    {
        auto spk_man = m_wallet->GetLegacyScriptPubKeyMan();
        if (spk_man) {
            return spk_man->HaveWatchOnly();
        }
        return false;
    };
    bool setAddressBook(const CTxDestination& dest, const std::string& name, const std::string& purpose) override
    {
        return m_wallet->SetAddressBook(dest, name, purpose);
    }
    bool delAddressBook(const CTxDestination& dest) override
    {
        return m_wallet->DelAddressBook(dest);
    }
    bool getAddress(const CTxDestination& dest,
        std::string* name,
        isminetype* is_mine,
        std::string* purpose) override
    {
        LOCK(m_wallet->cs_wallet);
        auto it = m_wallet->m_address_book.find(dest);
        if (it == m_wallet->m_address_book.end() || it->second.IsChange()) {
            return false;
        }
        if (name) {
            *name = it->second.GetLabel();
        }
        if (is_mine) {
            *is_mine = m_wallet->IsMine(dest);
        }
        if (purpose) {
            *purpose = it->second.purpose;
        }
        return true;
    }
    std::vector<WalletAddress> getAddresses() override
    {
        LOCK(m_wallet->cs_wallet);
        std::vector<WalletAddress> result;
        for (const auto& item : m_wallet->m_address_book) {
            if (item.second.IsChange()) continue;
            result.emplace_back(item.first, m_wallet->IsMine(item.first), item.second.GetLabel(), item.second.purpose);
        }
        return result;
    }
    bool extractOutputDestination(const CTxOutput& output, CTxDestination& dest) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->ExtractOutputDestination(output, dest);
    }
    bool addDestData(const CTxDestination& dest, const std::string& key, const std::string& value) override
    {
        LOCK(m_wallet->cs_wallet);
        WalletBatch batch{m_wallet->GetDatabase()};
        return m_wallet->AddDestData(batch, dest, key, value);
    }
    bool eraseDestData(const CTxDestination& dest, const std::string& key) override
    {
        LOCK(m_wallet->cs_wallet);
        WalletBatch batch{m_wallet->GetDatabase()};
        return m_wallet->EraseDestData(batch, dest, key);
    }
    std::vector<std::string> getDestValues(const std::string& prefix) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->GetDestValues(prefix);
    }
    bool findCoin(const mw::Hash& output_id, mw::Coin& coin) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->GetCoin(output_id, coin);
    }
    void lockCoin(const OutputIndex& output) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->LockCoin(output);
    }
    void unlockCoin(const OutputIndex& output) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->UnlockCoin(output);
    }
    bool isLockedCoin(const OutputIndex& output) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->IsLockedCoin(output);
    }
    void listLockedCoins(std::vector<OutputIndex>& outputs) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->ListLockedCoins(outputs);
    }
    CTransactionRef createTransaction(const std::vector<CRecipient>& recipients,
        const CCoinControl& coin_control,
        bool sign,
        int& change_pos,
        CAmount& fee,
        bilingual_str& fail_reason) override
    {
        LOCK(m_wallet->cs_wallet);
        CTransactionRef tx;
        FeeCalculation fee_calc_out;
        if (!m_wallet->CreateTransaction(recipients, tx, fee, change_pos,
                fail_reason, coin_control, fee_calc_out, sign)) {
            return {};
        }
        return tx;
    }
    void commitTransaction(CTransactionRef tx,
        WalletValueMap value_map,
        WalletOrderForm order_form,
        const std::vector<ReserveDestination*>& reserved_keys) override
    {
        LOCK(m_wallet->cs_wallet);
        for (ReserveDestination* reserved : reserved_keys) {
            reserved->KeepDestination();
        }

        m_wallet->CommitTransaction(std::move(tx), std::move(value_map), std::move(order_form));
    }
    bool transactionCanBeAbandoned(const uint256& txid) override { return m_wallet->TransactionCanBeAbandoned(txid); }
    bool abandonTransaction(const uint256& txid) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->AbandonTransaction(txid);
    }
    bool transactionCanBeBumped(const uint256& txid) override
    {
        return feebumper::TransactionCanBeBumped(*m_wallet.get(), txid);
    }
    bool createBumpTransaction(const uint256& txid,
        const CCoinControl& coin_control,
        std::vector<bilingual_str>& errors,
        CAmount& old_fee,
        CAmount& new_fee,
        CMutableTransaction& mtx) override
    {
        return feebumper::CreateRateBumpTransaction(*m_wallet.get(), txid, coin_control, errors, old_fee, new_fee, mtx) == feebumper::Result::OK;
    }
    bool signBumpTransaction(CMutableTransaction& mtx) override { return feebumper::SignTransaction(*m_wallet.get(), mtx); }
    bool commitBumpTransaction(const uint256& txid,
        CMutableTransaction&& mtx,
        std::vector<bilingual_str>& errors,
        uint256& bumped_txid) override
    {
        return feebumper::CommitTransaction(*m_wallet.get(), txid, std::move(mtx), errors, bumped_txid) ==
               feebumper::Result::OK;
    }
    bool transactionCanBeRebroadcast(const uint256& txid) override { return m_wallet->TransactionCanBeRebroadcast(txid); }
    bool rebroadcastTransaction(const uint256& txid) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->RebroadcastTransaction(txid);
    }
    CTransactionRef getTx(const uint256& txid) override
    {
        LOCK(m_wallet->cs_wallet);
        auto mi = m_wallet->mapWallet.find(txid);
        if (mi != m_wallet->mapWallet.end()) {
            return mi->second.tx;
        }
        return {};
    }
    std::vector<WalletTxRecord> getWalletTx(const uint256& txid) override
    {
        LOCK(m_wallet->cs_wallet);
        auto mi = m_wallet->mapWallet.find(txid);
        if (mi != m_wallet->mapWallet.end()) {
            return TxList(*m_wallet).List(mi->second, ISMINE_ALL, boost::none, boost::none);
        }
        return {};
    }
    std::vector<WalletTxRecord> getWalletTxs() override
    {
        LOCK(m_wallet->cs_wallet);
        return TxList(*m_wallet).ListAll(ISMINE_ALL);
    }
    WalletTx getWalletTxDetails(const uint256& txid,
        WalletTxStatus& tx_status,
        WalletOrderForm& order_form,
        bool& in_mempool,
        int& num_blocks) override
    {
        LOCK(m_wallet->cs_wallet);
        auto mi = m_wallet->mapWallet.find(txid);
        if (mi != m_wallet->mapWallet.end()) {
            num_blocks = m_wallet->GetLastBlockHeight();
            in_mempool = mi->second.InMempool();
            order_form = mi->second.vOrderForm;
            tx_status = MakeWalletTxStatus(*m_wallet, mi->second);
            return MakeWalletTx(*m_wallet, mi->second);
        }
        return {};
    }
    TransactionError fillPSBT(int sighash_type,
        bool sign,
        bool bip32derivs,
        PartiallySignedTransaction& psbtx,
        bool& complete,
        size_t* n_signed) override
    {
        return m_wallet->FillPSBT(psbtx, complete, sighash_type, sign, bip32derivs, n_signed);
    }
    WalletBalances getBalances() override
    {
        const auto bal = m_wallet->GetBalance();
        WalletBalances result;
        result.balance = bal.m_mine_trusted;
        result.unconfirmed_balance = bal.m_mine_untrusted_pending;
        result.immature_balance = bal.m_mine_immature;
        result.have_watch_only = haveWatchOnly();
        if (result.have_watch_only) {
            result.watch_only_balance = bal.m_watchonly_trusted;
            result.unconfirmed_watch_only_balance = bal.m_watchonly_untrusted_pending;
            result.immature_watch_only_balance = bal.m_watchonly_immature;
        }
        return result;
    }
    bool tryGetBalances(WalletBalances& balances, uint256& block_hash) override
    {
        TRY_LOCK(m_wallet->cs_wallet, locked_wallet);
        if (!locked_wallet) {
            return false;
        }
        block_hash = m_wallet->GetLastBlockHash();
        balances = getBalances();
        return true;
    }
    CAmount getBalance() override { return m_wallet->GetBalance().m_mine_trusted; }
    CAmount getAvailableBalance(const CCoinControl& coin_control) override
    {
        return m_wallet->GetAvailableBalance(&coin_control);
    }
    isminetype txinIsMine(const CTxInput& txin) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->IsMine(txin);
    }
    isminetype txoutIsMine(const CTxOutput& txout) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->IsMine(txout);
    }
    CAmount getValue(const CTxOutput& txout) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->GetValue(txout);
    }
    CAmount getDebit(const CTxInput& txin, isminefilter filter) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->GetDebit(txin, filter);
    }
    CAmount getCredit(const CTxOutput& txout, isminefilter filter) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->GetCredit(txout, filter);
    }
    bool isChange(const CTxOutput& txout) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->IsChange(txout);
    }
    CoinsList listCoins() override
    {
        LOCK(m_wallet->cs_wallet);
        CoinsList result;
        for (const auto& entry : m_wallet->ListCoins()) {
            auto& group = result[entry.first];
            for (const auto& coin : entry.second) {
                group.emplace_back(coin.GetIndex(), MakeWalletTxOut(*m_wallet, coin));
            }
        }
        return result;
    }
    std::vector<WalletTxOut> getCoins(const std::vector<OutputIndex>& outputs) override
    {
        LOCK(m_wallet->cs_wallet);
        std::vector<WalletTxOut> result;
        result.reserve(outputs.size());
        for (const OutputIndex& output_idx : outputs) {
            result.emplace_back();
            auto wtx = m_wallet->FindWalletTx(output_idx);
            if (wtx != nullptr) {
                int depth = wtx->GetDepthInMainChain();
                if (depth >= 0) {
                    result.back() = MakeWalletTxOut(*m_wallet, *wtx, wtx->tx->GetOutput(output_idx));
                }
            }
        }
        return result;
    }
    CAmount getRequiredFee(unsigned int tx_bytes, uint64_t mweb_weight) override { return GetRequiredFee(*m_wallet, tx_bytes, mweb_weight); }
    CAmount getMinimumFee(unsigned int tx_bytes,
        uint64_t mweb_weight,
        const CCoinControl& coin_control,
        int* returned_target,
        FeeReason* reason) override
    {
        FeeCalculation fee_calc;
        CAmount result;
        result = GetMinimumFee(*m_wallet, tx_bytes, mweb_weight, coin_control, &fee_calc);
        if (returned_target) *returned_target = fee_calc.returnedTarget;
        if (reason) *reason = fee_calc.reason;
        return result;
    }
    unsigned int getConfirmTarget() override { return m_wallet->m_confirm_target; }
    bool hdEnabled() override { return m_wallet->IsHDEnabled(); }
    bool canGetAddresses() override { return m_wallet->CanGetAddresses(); }
    bool privateKeysDisabled() override { return m_wallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS); }
    OutputType getDefaultAddressType() override { return m_wallet->m_default_address_type; }
    CAmount getDefaultMaxTxFee() override { return m_wallet->m_default_max_tx_fee; }
    void remove() override
    {
        RemoveWallet(m_wallet, false /* load_on_start */);
    }
    bool isLegacy() override { return m_wallet->IsLegacy(); }
    bool getPeginAddress(StealthAddress& pegin_address) override
    {
        return m_wallet->GetMWWallet()->GetStealthAddress(mw::PEGIN_INDEX, pegin_address);
    }
    std::unique_ptr<Handler> handleUnload(UnloadFn fn) override
    {
        return MakeHandler(m_wallet->NotifyUnload.connect(fn));
    }
    std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) override
    {
        return MakeHandler(m_wallet->ShowProgress.connect(fn));
    }
    std::unique_ptr<Handler> handleStatusChanged(StatusChangedFn fn) override
    {
        return MakeHandler(m_wallet->NotifyStatusChanged.connect([fn](CWallet*) { fn(); }));
    }
    std::unique_ptr<Handler> handleAddressBookChanged(AddressBookChangedFn fn) override
    {
        return MakeHandler(m_wallet->NotifyAddressBookChanged.connect(
            [fn](CWallet*, const CTxDestination& address, const std::string& label, bool is_mine,
                const std::string& purpose, ChangeType status) { fn(address, label, is_mine, purpose, status); }));
    }
    std::unique_ptr<Handler> handleTransactionChanged(TransactionChangedFn fn) override
    {
        return MakeHandler(m_wallet->NotifyTransactionChanged.connect(
            [fn](CWallet*, const uint256& txid, ChangeType status) { fn(txid, status); }));
    }
    std::unique_ptr<Handler> handleWatchOnlyChanged(WatchOnlyChangedFn fn) override
    {
        return MakeHandler(m_wallet->NotifyWatchonlyChanged.connect(fn));
    }
    std::unique_ptr<Handler> handleCanGetAddressesChanged(CanGetAddressesChangedFn fn) override
    {
        return MakeHandler(m_wallet->NotifyCanGetAddressesChanged.connect(fn));
    }
    CWallet* wallet() override { return m_wallet.get(); }

    std::shared_ptr<CWallet> m_wallet;
};

class WalletClientImpl : public WalletClient
{
public:
    WalletClientImpl(Chain& chain, ArgsManager& args)
    {
        m_context.chain = &chain;
        m_context.args = &args;
    }
    ~WalletClientImpl() override { UnloadWallets(); }

    //! ChainClient methods
    void registerRpcs() override
    {
        for (const CRPCCommand& command : GetWalletRPCCommands()) {
            m_rpc_commands.emplace_back(command.category, command.name, [this, &command](const JSONRPCRequest& request, UniValue& result, bool last_handler) {
                return command.actor({request, m_context}, result, last_handler);
            }, command.argNames, command.unique_id);
            m_rpc_handlers.emplace_back(m_context.chain->handleRpc(m_rpc_commands.back()));
        }
    }
    bool verify() override { return VerifyWallets(*m_context.chain); }
    bool load() override { return LoadWallets(*m_context.chain); }
    void start(CScheduler& scheduler) override { return StartWallets(scheduler, *Assert(m_context.args)); }
    void flush() override { return FlushWallets(); }
    void stop() override { return StopWallets(); }
    void setMockTime(int64_t time) override { return SetMockTime(time); }

    //! WalletClient methods
    std::unique_ptr<Wallet> createWallet(const std::string& name, const SecureString& passphrase, uint64_t wallet_creation_flags, bilingual_str& error, std::vector<bilingual_str>& warnings) override
    {
        std::shared_ptr<CWallet> wallet;
        DatabaseOptions options;
        DatabaseStatus status;
        options.require_create = true;
        options.create_flags = wallet_creation_flags;
        options.create_passphrase = passphrase;
        return MakeWallet(CreateWallet(*m_context.chain, name, true /* load_on_start */, options, status, error, warnings));
    }
    std::unique_ptr<Wallet> loadWallet(const std::string& name, bilingual_str& error, std::vector<bilingual_str>& warnings) override
    {
        DatabaseOptions options;
        DatabaseStatus status;
        options.require_existing = true;
        return MakeWallet(LoadWallet(*m_context.chain, name, true /* load_on_start */, options, status, error, warnings));
    }
    std::string getWalletDir() override
    {
        return GetWalletDir().string();
    }
    std::vector<std::string> listWalletDir() override
    {
        std::vector<std::string> paths;
        for (auto& path : ListWalletDir()) {
            paths.push_back(path.string());
        }
        return paths;
    }
    std::vector<std::unique_ptr<Wallet>> getWallets() override
    {
        std::vector<std::unique_ptr<Wallet>> wallets;
        for (const auto& wallet : GetWallets()) {
            wallets.emplace_back(MakeWallet(wallet));
        }
        return wallets;
    }
    std::unique_ptr<Handler> handleLoadWallet(LoadWalletFn fn) override
    {
        return HandleLoadWallet(std::move(fn));
    }

    WalletContext m_context;
    const std::vector<std::string> m_wallet_filenames;
    std::vector<std::unique_ptr<Handler>> m_rpc_handlers;
    std::list<CRPCCommand> m_rpc_commands;
};

} // namespace

std::unique_ptr<Wallet> MakeWallet(const std::shared_ptr<CWallet>& wallet) { return wallet ? MakeUnique<WalletImpl>(wallet) : nullptr; }

std::unique_ptr<WalletClient> MakeWalletClient(Chain& chain, ArgsManager& args)
{
    return MakeUnique<WalletClientImpl>(chain, args);
}

} // namespace interfaces
