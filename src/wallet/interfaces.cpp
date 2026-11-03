// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/wallet.h>

#include <chain.h>
#include <chainparams.h>
#include <coinjoin/client.h>
#include <consensus/amount.h>
#include <interfaces/chain.h>
#include <interfaces/coinjoin.h>
#include <interfaces/handler.h>
#include <policy/fees.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <script/standard.h>
#include <support/allocators/secure.h>
#include <sync.h>
#include <uint256.h>
#include <util/check.h>
#include <util/system.h>
#include <util/translation.h>
#include <util/ui_change_type.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/coinjoin.h>
#include <wallet/context.h>
#include <wallet/fees.h>
#include <wallet/types.h>
#include <wallet/load.h>
#include <wallet/receive.h>
#include <wallet/rpc/wallet.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>
#include <wallet/hdchain.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/walletutil.h>
#include <evo/deterministicmns.h>
#include <masternode/sync.h>
#include <txdb.h>
#include <node/context.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using interfaces::Chain;
using interfaces::CoinLockResult;
using interfaces::FoundBlock;
using interfaces::Handler;
using interfaces::MakeSignalHandler;
using interfaces::Wallet;
using interfaces::WalletAddress;
using interfaces::WalletBalances;
using interfaces::WalletLoader;
using interfaces::WalletMigrationResult;
using interfaces::WalletOrderForm;
using interfaces::WalletTx;
using interfaces::WalletTxOut;
using interfaces::WalletTxSignResult;
using interfaces::WalletTxStatus;
using interfaces::WalletValueMap;
using node::NodeContext;

namespace wallet {
// All members of the classes in this namespace are intentionally public, as the
// classes themselves are private.
namespace {
//! Construct wallet tx struct.
WalletTx MakeWalletTx(CWallet& wallet, const CWalletTx& wtx)
{
    LOCK(wallet.cs_wallet);
    WalletTx result;
    bool fInputDenomFound{false}, fOutputDenomFound{false};
    result.tx = wtx.tx;
    result.txin_is_mine.reserve(wtx.tx->vin.size());
    for (const auto& txin : wtx.tx->vin) {
        result.txin_is_mine.emplace_back(InputIsMine(wallet, txin));
        if (!fInputDenomFound && result.txin_is_mine.back() && wallet.IsDenominated(txin.prevout)) {
            fInputDenomFound = true;
        }
    }
    result.txout_is_mine.reserve(wtx.tx->vout.size());
    result.txout_address.reserve(wtx.tx->vout.size());
    result.txout_address_is_mine.reserve(wtx.tx->vout.size());
    for (const auto& txout : wtx.tx->vout) {
        result.txout_is_mine.emplace_back(wallet.IsMine(txout));
        result.txout_address.emplace_back();
        result.txout_address_is_mine.emplace_back(ExtractDestination(txout.scriptPubKey, result.txout_address.back()) ?
                                                      wallet.IsMine(result.txout_address.back()) :
                                                      ISMINE_NO);
        if (!fOutputDenomFound && result.txout_address_is_mine.back() && CoinJoin::IsDenominatedAmount(txout.nValue)) {
            fOutputDenomFound = true;
        }
    }
    result.credit = CachedTxGetCredit(wallet, wtx, ISMINE_ALL);
    result.debit = CachedTxGetDebit(wallet, wtx, ISMINE_ALL);
    result.change = CachedTxGetChange(wallet, wtx);
    result.time = wtx.GetTxTime();
    result.value_map = wtx.mapValue;
    result.is_coinbase = wtx.IsCoinBase();
    result.is_platform_transfer = wtx.IsPlatformTransfer();
    // The determination of is_denominate is based on simplified checks here because in this part of the code
    // we only want to know about mixing transactions belonging to this specific wallet.
    // Post-V24 a session may contain promotion (10 inputs to 1 output) and demotion (1 to 10)
    // entries, so the input and output counts of a mixing transaction no longer have to match.
    // Every output being a denominated amount is what still sets one apart from an ordinary
    // payment, which would have a non-denominated payment or change output.
    const bool fAllOutputsDenominated = std::ranges::all_of(
        wtx.tx->vout, [](const CTxOut& txout) { return CoinJoin::IsDenominatedAmount(txout.nValue); });
    result.is_denominate = fAllOutputsDenominated &&
                           (result.credit - result.debit) == 0 && // Transaction pays no tx fee
                           fInputDenomFound && fOutputDenomFound; // At least 1 input and 1 output are denominated belonging to the provided wallet
    return result;
}

//! Construct wallet tx status struct.
WalletTxStatus MakeWalletTxStatus(const CWallet& wallet, const CWalletTx& wtx)
    EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    AssertLockHeld(wallet.cs_wallet);

    WalletTxStatus result;
    result.block_height =
        wtx.state<TxStateConfirmed>() ? wtx.state<TxStateConfirmed>()->confirmed_block_height :
        wtx.state<TxStateConflicted>() ? wtx.state<TxStateConflicted>()->conflicting_block_height :
        std::numeric_limits<int>::max();
    result.blocks_to_maturity = wallet.GetTxBlocksToMaturity(wtx);
    result.depth_in_main_chain = wallet.GetTxDepthInMainChain(wtx);
    result.time_received = wtx.nTimeReceived;
    result.lock_time = wtx.tx->nLockTime;
    result.is_trusted = CachedTxIsTrusted(wallet, wtx);
    result.is_abandoned = wtx.isAbandoned();
    result.is_coinbase = wtx.IsCoinBase();
    result.is_in_main_chain = wallet.IsTxInMainChain(wtx);
    result.is_chainlocked = wallet.IsTxChainLocked(wtx);
    result.is_islocked = wallet.IsTxLockedByInstantSend(wtx);
    return result;
}

//! Construct wallet TxOut struct.
WalletTxOut MakeWalletTxOut(const CWallet& wallet,
    const CWalletTx& wtx,
    int n,
    int depth) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    WalletTxOut result;
    result.txout = wtx.tx->vout[n];
    result.time = wtx.GetTxTime();
    result.depth_in_main_chain = depth;
    result.is_spent = wallet.IsSpent(COutPoint(wtx.GetHash(), n));
    return result;
}

WalletTxOut MakeWalletTxOut(const CWallet& wallet,
    const COutput& output) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    WalletTxOut result;
    result.txout = output.txout;
    result.time = output.time;
    result.depth_in_main_chain = output.depth;
    result.is_spent = wallet.IsSpent(output.outpoint);
    return result;
}

class WalletImpl : public Wallet
{
public:
    explicit WalletImpl(WalletContext& context, const std::shared_ptr<CWallet>& wallet) : m_context(context), m_wallet(wallet) {}

    void markDirty() override
    {
        m_wallet->MarkDirty();
    }
    bool encryptWallet(const SecureString& wallet_passphrase) override
    {
        return m_wallet->EncryptWallet(wallet_passphrase);
    }
    bool isCrypted() override { return m_wallet->IsCrypted(); }
    bool lock(bool fAllowMixing) override { return m_wallet->Lock(fAllowMixing); }
    bool unlock(const SecureString& wallet_passphrase, bool fAllowMixing) override { return m_wallet->Unlock(wallet_passphrase, fAllowMixing); }
    bool isLocked(bool fForMixing) override { return m_wallet->IsLocked(fForMixing); }
    bool changeWalletPassphrase(const SecureString& old_wallet_passphrase,
        const SecureString& new_wallet_passphrase) override
    {
        return m_wallet->ChangeWalletPassphrase(old_wallet_passphrase, new_wallet_passphrase);
    }
    wallet::RescanStatus startRescan(bool from_genesis) override
    {
        int rescan_height{0};
        if (!from_genesis) {
            std::optional<int64_t> time_first_key;
            for (auto spk_man : m_wallet->GetAllScriptPubKeyMans()) {
                int64_t time = spk_man->GetTimeFirstKey();
                if (!time_first_key || time < *time_first_key) time_first_key = time;
            }
            if (time_first_key) {
                m_wallet->chain().findFirstBlockWithTimeAndHeight(*time_first_key - TIMESTAMP_WINDOW, rescan_height,
                                                                  FoundBlock().height(rescan_height));
            }
        }

        WalletRescanReserver reserver(*m_wallet);
        if (!reserver.reserve()) {
            return wallet::RescanStatus::BUSY;
        }
        switch (m_wallet->ScanForWalletTransactions(m_wallet->chain().getBlockHash(rescan_height), rescan_height, /*max_height=*/std::nullopt,
                                                    reserver, /*fUpdate=*/true, /*save_progress=*/false).status) {
        case CWallet::ScanResult::FAILURE:
            return wallet::RescanStatus::FAILURE;
        case CWallet::ScanResult::SUCCESS:
            return wallet::RescanStatus::SUCCESS;
        case CWallet::ScanResult::USER_ABORT:
            return wallet::RescanStatus::USER_ABORT;
        }
        Assume(false); // unreachable
        return wallet::RescanStatus::FAILURE; // fallback for release builds
    }
    void abortRescan() override { m_wallet->AbortRescan(); }
    void autoLockMasternodeCollaterals() override { m_wallet->AutoLockMasternodeCollaterals(); }
    bool backupWallet(const std::string& filename) override { return m_wallet->BackupWallet(filename); }
    bool autoBackupWallet(const fs::path& wallet_path, bilingual_str& error_string, std::vector<bilingual_str>& warnings) override
    {
        return m_wallet->AutoBackupWallet(wallet_path, error_string, warnings);
    }
    int64_t getKeysLeftSinceAutoBackup() override { return m_wallet->nKeysLeftSinceAutoBackup; }
    std::string getWalletName() override { return m_wallet->GetName(); }
    util::Result<CTxDestination> getNewDestination(const std::string& label) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->GetNewDestination(label);
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
    bool signSpecialTxPayload(const uint256& hash, const CKeyID& keyid, std::vector<unsigned char>& vchSig) override
    {
        return m_wallet->SignSpecialTxPayload(hash, keyid, vchSig);
    }
    wallet::PlatformKeyResult<CPubKey> getPlatformPubKey(const wallet::PlatformKeyRequest& request) override
    {
        return m_wallet->GetPlatformPubKey(request);
    }
    wallet::PlatformKeyResult<std::vector<unsigned char>> signPlatformDigest(const wallet::PlatformKeyRequest& request,
                                                                             const uint256& digest) override
    {
        return m_wallet->SignPlatformDigest(request, digest);
    }
    wallet::PlatformKeyResult<SecureVector> platformECDHSecret(const wallet::IdentityAuthKey& key,
                                                               const CPubKey& counterparty) override
    {
        return m_wallet->PlatformECDHSecret(key, counterparty);
    }
    wallet::PlatformKeyResult<wallet::FriendshipXpub> ensureFriendshipReceivingKeychain(
        const wallet::FriendshipKeychainRequest& request) override
    {
        return m_wallet->EnsureFriendshipReceivingKeychain(request);
    }
    bool isSpendable(const CScript& script) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->IsMine(script) & ISMINE_SPENDABLE;
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
    bool setAddressBook(const CTxDestination& dest, const std::string& name, const std::optional<AddressPurpose>& purpose) override
    {
        return m_wallet->SetAddressBook(dest, name, purpose);
    }
    bool delAddressBook(const CTxDestination& dest) override
    {
        return m_wallet->DelAddressBook(dest);
    }
    bool getAddress(const CTxDestination& dest,
        std::string* name,
        wallet::isminetype* is_mine,
        wallet::AddressPurpose* purpose) override
    {
        LOCK(m_wallet->cs_wallet);
        const auto& entry = m_wallet->FindAddressBookEntry(dest, /*allow_change=*/false);
        if (!entry) return false; // addr not found
        if (name) {
            *name = entry->GetLabel();
        }
        std::optional<isminetype> dest_is_mine;
        if (is_mine || purpose) {
            dest_is_mine = m_wallet->IsMine(dest);
        }
        if (is_mine) {
            *is_mine = *dest_is_mine;
        }
        if (purpose) {
            // In very old wallets, address purpose may not be recorded so we derive it from IsMine
            *purpose = entry->purpose.value_or(*dest_is_mine ? AddressPurpose::RECEIVE : AddressPurpose::SEND);
        }
        return true;
    }
    std::vector<WalletAddress> getAddresses() const override
    {
        LOCK(m_wallet->cs_wallet);
        std::vector<WalletAddress> result;
        m_wallet->ForEachAddrBookEntry([&](const CTxDestination& dest, const std::string& label, bool is_change, const std::optional<AddressPurpose>& purpose) EXCLUSIVE_LOCKS_REQUIRED(m_wallet->cs_wallet) {
            if (is_change) return;
            isminetype is_mine = m_wallet->IsMine(dest);
            // In very old wallets, address purpose may not be recorded so we derive it from IsMine
            result.emplace_back(dest, is_mine, purpose.value_or(is_mine ? AddressPurpose::RECEIVE : AddressPurpose::SEND), label);
        });
        return result;
    }
    std::vector<std::string> getAddressReceiveRequests() override {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->GetAddressReceiveRequests();
    }
    bool setAddressReceiveRequest(const CTxDestination& dest, const std::string& id, const std::string& value) override {
        // Note: The setAddressReceiveRequest interface used by the GUI to store
        // receive requests is a little awkward and could be improved in the
        // future:
        //
        // - The same method is used to save requests and erase them, but
        //   having separate methods could be clearer and prevent bugs.
        //
        // - Request ids are passed as strings even though they are generated as
        //   integers.
        //
        // - Multiple requests can be stored for the same address, but it might
        //   be better to only allow one request or only keep the current one.
        LOCK(m_wallet->cs_wallet);
        WalletBatch batch{m_wallet->GetDatabase()};
        return value.empty() ? m_wallet->EraseAddressReceiveRequest(batch, dest, id)
                             : m_wallet->SetAddressReceiveRequest(batch, dest, id, value);
    }
    bool writePlatformData(const std::string& key, const std::vector<unsigned char>& value) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->WritePlatformData(key, value);
    }
    std::map<std::string, std::vector<unsigned char>> getPlatformData(const std::string& prefix) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->GetPlatformData(prefix);
    }
    bool displayAddress(const CTxDestination& dest) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->DisplayAddress(dest);
    }
    bool lockCoin(const COutPoint& output, const bool write_to_db) override
    {
        LOCK(m_wallet->cs_wallet);
        std::unique_ptr<WalletBatch> batch = write_to_db ? std::make_unique<WalletBatch>(m_wallet->GetDatabase()) : nullptr;
        return m_wallet->LockCoin(output, batch.get());
    }
    CoinLockResult acquireCoinLock(const COutPoint& output, bool write_to_db) override
    {
        LOCK(m_wallet->cs_wallet);
        if (m_wallet->IsLockedCoin(output)) return CoinLockResult::ALREADY_LOCKED;
        std::unique_ptr<WalletBatch> batch = write_to_db ? std::make_unique<WalletBatch>(m_wallet->GetDatabase()) : nullptr;
        if (!m_wallet->LockCoin(output, batch.get())) {
            // LockCoin inserts into memory before persisting; the coin was not
            // locked on entry, so undo the insertion (and erase any partially
            // persisted record) to keep FAILED meaning "no lock acquired".
            m_wallet->UnlockCoin(output, batch.get());
            return CoinLockResult::FAILED;
        }
        return CoinLockResult::ACQUIRED;
    }
    bool unlockCoin(const COutPoint& output) override
    {
        LOCK(m_wallet->cs_wallet);
        std::unique_ptr<WalletBatch> batch = std::make_unique<WalletBatch>(m_wallet->GetDatabase());
        return m_wallet->UnlockCoin(output, batch.get());
    }
    bool isLockedCoin(const COutPoint& output) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->IsLockedCoin(output);
    }
    std::set<COutPoint> listLockedCoins() override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->ListLockedCoins();
    }
    bool lockCoins(const std::vector<COutPoint>& outputs) override
    {
        LOCK(m_wallet->cs_wallet);
        WalletBatch batch(m_wallet->GetDatabase());
        for (const auto& output : outputs) {
            if (!m_wallet->LockCoin(output, &batch)) return false;
        }
        return true;
    }
    bool unlockCoins(const std::vector<COutPoint>& outputs) override
    {
        LOCK(m_wallet->cs_wallet);
        WalletBatch batch(m_wallet->GetDatabase());
        for (const auto& output : outputs) {
            if (!m_wallet->UnlockCoin(output, &batch)) return false;
        }
        return true;
    }
    void setDustProtectionThreshold(CAmount threshold) override
    {
        LOCK(m_wallet->cs_wallet);
        m_wallet->m_dust_protection_threshold = std::clamp(threshold, CAmount{0}, MAX_DUST_PROTECTION_THRESHOLD);
    }
    void lockExistingDustOutputs() override
    {
        LOCK(m_wallet->cs_wallet);
        m_wallet->LockExistingDustOutputs();
    }
    std::vector<COutPoint> listProTxCoins() override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->ListProTxCoins();
    }
    util::Result<CTransactionRef> createTransaction(const std::vector<CRecipient>& recipients,
        const CCoinControl& coin_control,
        bool sign,
        int& change_pos,
        CAmount& fee) override
    {
        LOCK(m_wallet->cs_wallet);
        auto res = CreateTransaction(*m_wallet, recipients, change_pos, coin_control, sign);
        if (!res) return util::Error{util::ErrorString(res)};
        const auto& txr = *res;
        fee = txr.fee;
        change_pos = txr.change_pos;

        return txr.tx;
    }
    util::Result<CTransactionRef> fundTransaction(const CMutableTransaction& tx_template,
                                                  const CTxDestination& fund_destination) override
    {
        // Callers must not hold cs_main: the wallet library cannot reference
        // node locks, and syncing below would deadlock if it were held.
        m_wallet->BlockUntilSyncedToCurrentChain();

        LOCK(m_wallet->cs_wallet);
        if (std::holds_alternative<CNoDestination>(fund_destination)) {
            return util::Error{Untranslated("No source of funds specified")};
        }

        CMutableTransaction tx{tx_template};
        static const CTxOut dummy_output{0, CScript() << OP_RETURN};
        const bool add_dummy_output{tx.vout.empty()};
        if (add_dummy_output) tx.vout.emplace_back(dummy_output);

        std::vector<CRecipient> recipients;
        recipients.reserve(tx.vout.size());
        for (const auto& output : tx.vout) {
            recipients.push_back({output.scriptPubKey, output.nValue, /*fSubtractFeeFromAmount=*/false});
        }

        CCoinControl coin_control;
        coin_control.destChange = fund_destination;
        // Spend only from fund_destination
        coin_control.m_allow_other_inputs = false;
        std::vector<COutput> address_coins;
        for (const auto& output : AvailableCoinsListUnspent(*m_wallet).All()) {
            CTxDestination destination;
            if (ExtractDestination(output.txout.scriptPubKey, destination) && destination == fund_destination) {
                address_coins.push_back(output);
            }
        }
        if (address_coins.empty()) {
            return util::Error{
                strprintf(Untranslated("No funds at specified address %s"), EncodeDestination(fund_destination))};
        }

        // Every selected coin is force-spent, so select as few as possible: largest
        // first, adding another one only while they don't cover amount plus fees
        std::sort(address_coins.begin(), address_coins.end(),
                  [](const COutput& a, const COutput& b) { return a.txout.nValue > b.txout.nValue; });
        util::Result<CreatedTransactionResult> result{util::Error{}};
        for (const COutput& coin : address_coins) {
            coin_control.Select(coin.outpoint);
            result = CreateTransaction(*m_wallet, recipients, RANDOM_CHANGE_POSITION, coin_control,
                                       /*sign=*/true, tx.vExtraPayload.size());
            if (result) break;
        }
        if (!result) return util::Error{util::ErrorString(result)};

        tx.vin = result->tx->vin;
        tx.vout = result->tx->vout;
        if (add_dummy_output && tx.vout.size() > 1) {
            const auto it{std::find(tx.vout.begin(), tx.vout.end(), dummy_output)};
            CHECK_NONFATAL(it != tx.vout.end());
            tx.vout.erase(it);
        }
        return MakeTransactionRef(std::move(tx));
    }
    util::Result<WalletTxSignResult> signTransaction(const CMutableTransaction& tx) override
    {
        // Callers must not hold cs_main (see fundTransaction above).
        std::map<COutPoint, Coin> coins;
        for (const auto& input : tx.vin)
            coins.try_emplace(input.prevout);
        m_wallet->chain().findCoins(coins);

        LOCK(m_wallet->cs_wallet);
        if (m_wallet->IsLocked()) {
            return util::Error{_("Please enter the wallet passphrase with walletpassphrase first.")};
        }

        CMutableTransaction signed_tx{tx};
        std::map<int, bilingual_str> input_errors;
        const bool complete{m_wallet->SignTransaction(signed_tx, coins, SIGHASH_ALL, input_errors)};
        std::vector<bilingual_str> errors;
        errors.reserve(input_errors.size());
        for (auto& [index, error] : input_errors)
            errors.push_back(std::move(error));
        return WalletTxSignResult{MakeTransactionRef(std::move(signed_tx)), complete, std::move(errors)};
    }
    void commitTransaction(CTransactionRef tx,
        WalletValueMap value_map,
        WalletOrderForm order_form) override
    {
        LOCK(m_wallet->cs_wallet);
        m_wallet->CommitTransaction(std::move(tx), std::move(value_map), std::move(order_form));
    }
    bool transactionCanBeAbandoned(const uint256& txid) override { return m_wallet->TransactionCanBeAbandoned(txid); }
    bool transactionCanBeResent(const uint256& txid) override { return m_wallet->TransactionCanBeResent(txid); }
    bool abandonTransaction(const uint256& txid) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->AbandonTransaction(txid);
    }
    bool resendTransaction(const uint256& txid) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->ResendTransaction(txid);
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
    WalletTx getWalletTx(const uint256& txid) override
    {
        LOCK(m_wallet->cs_wallet);
        auto mi = m_wallet->mapWallet.find(txid);
        if (mi != m_wallet->mapWallet.end()) {
            return MakeWalletTx(*m_wallet, mi->second);
        }
        return {};
    }
    std::set<WalletTx> getWalletTxs() override
    {
        LOCK(m_wallet->cs_wallet);
        std::set<WalletTx> result;
        for (const auto& entry : m_wallet->mapWallet) {
            result.emplace(MakeWalletTx(*m_wallet, entry.second));
        }
        return result;
    }
    bool tryGetTxStatus(const uint256& txid,
        interfaces::WalletTxStatus& tx_status,
        int& num_blocks,
        int64_t& block_time) override
    {
        TRY_LOCK(m_wallet->cs_wallet, locked_wallet);
        if (!locked_wallet) {
            return false;
        }
        auto mi = m_wallet->mapWallet.find(txid);
        if (mi == m_wallet->mapWallet.end()) {
            return false;
        }
        num_blocks = m_wallet->GetLastBlockHeight();
        block_time = -1;
        CHECK_NONFATAL(m_wallet->chain().findBlock(m_wallet->GetLastBlockHash(), FoundBlock().time(block_time)));
        tx_status = MakeWalletTxStatus(*m_wallet, mi->second);
        return true;
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
    int getRealOutpointCoinJoinRounds(const COutPoint& outpoint) override { return m_wallet->GetRealOutpointCoinJoinRounds(outpoint); }
    bool isFullyMixed(const COutPoint& outpoint) override { return m_wallet->IsFullyMixed(outpoint); }

    TransactionError fillPSBT(int sighash_type,
        bool sign,
        bool bip32derivs,
        size_t* n_signed,
        PartiallySignedTransaction& psbtx,
        bool& complete) override
    {
        return m_wallet->FillPSBT(psbtx, complete, sighash_type, sign, bip32derivs, n_signed);
    }
    WalletBalances getBalances() override
    {
        const auto bal = GetBalance(*m_wallet);
        WalletBalances result;
        result.balance = bal.m_mine_trusted;
        result.unconfirmed_balance = bal.m_mine_untrusted_pending;
        result.immature_balance = bal.m_mine_immature;
        result.locked_balance = bal.m_mine_trusted_locked;
        result.anonymized_balance = bal.m_anonymized;
        result.have_watch_only = haveWatchOnly();
        if (result.have_watch_only) {
            result.watch_only_balance = bal.m_watchonly_trusted;
            result.unconfirmed_watch_only_balance = bal.m_watchonly_untrusted_pending;
            result.immature_watch_only_balance = bal.m_watchonly_immature;
            result.locked_watch_only_balance = bal.m_watchonly_trusted_locked;
        }
        result.denominated_untrusted_pending = bal.m_denominated_untrusted_pending;
        result.denominated_trusted = bal.m_denominated_trusted;
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
    CAmount getBalance() override { return GetBalance(*m_wallet).m_mine_trusted; }
    CAmount getAnonymizableBalance(bool fSkipDenominated, bool fSkipUnconfirmed) override
    {
        return m_wallet->GetAnonymizableBalance(fSkipDenominated, fSkipUnconfirmed);
    }
    CAmount getNormalizedAnonymizedBalance() override
    {
        return m_wallet->GetNormalizedAnonymizedBalance();
    }
    CAmount getAverageAnonymizedRounds() override
    {
        return m_wallet->GetAverageAnonymizedRounds();
    }
    CAmount getAvailableBalance(const CCoinControl& coin_control) override
    {
        if (coin_control.IsUsingCoinJoin()) {
            return GetBalanceAnonymized(*m_wallet, coin_control);
        }
        LOCK(m_wallet->cs_wallet);
        CAmount total_amount = 0;
        // Fetch selected coins total amount
        if (coin_control.HasSelected()) {
            FastRandomContext rng{};
            CoinSelectionParams params(rng);
            // Note: for now, swallow any error.
            if (auto res = FetchSelectedInputs(*m_wallet, coin_control, params)) {
                total_amount += res->total_amount;
            }
        }

        // And fetch the wallet available coins
        if (coin_control.m_allow_other_inputs) {
            total_amount += AvailableCoins(*m_wallet, &coin_control).GetTotalAmount();
        }

        return total_amount;
    }
    wallet::isminetype txinIsMine(const CTxIn& txin) override
    {
        LOCK(m_wallet->cs_wallet);
        return InputIsMine(*m_wallet, txin);
    }
    wallet::isminetype txoutIsMine(const CTxOut& txout) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->IsMine(txout);
    }
    CAmount getDebit(const CTxIn& txin, wallet::isminefilter filter) override
    {
        LOCK(m_wallet->cs_wallet);
        return m_wallet->GetDebit(txin, filter);
    }
    CAmount getCredit(const CTxOut& txout, wallet::isminefilter filter) override
    {
        LOCK(m_wallet->cs_wallet);
        return OutputGetCredit(*m_wallet, txout, filter);
    }
    CoinsList listCoins() override
    {
        LOCK(m_wallet->cs_wallet);
        CoinsList result;
        for (const auto& entry : ListCoins(*m_wallet)) {
            auto& group = result[entry.first];
            for (const auto& coin : entry.second) {
                group.emplace_back(coin.outpoint,
                    MakeWalletTxOut(*m_wallet, coin));
            }
        }
        return result;
    }
    std::vector<WalletTxOut> getCoins(const std::vector<COutPoint>& outputs) override
    {
        LOCK(m_wallet->cs_wallet);
        std::vector<WalletTxOut> result;
        result.reserve(outputs.size());
        for (const auto& output : outputs) {
            result.emplace_back();
            auto it = m_wallet->mapWallet.find(output.hash);
            // The outpoint may come from user input; an index past the end is
            // simply not a coin of this wallet
            if (it != m_wallet->mapWallet.end() && output.n < it->second.tx->vout.size()) {
                int depth = m_wallet->GetTxDepthInMainChain(it->second);
                if (depth >= 0) {
                    result.back() = MakeWalletTxOut(*m_wallet, it->second, output.n, depth);
                }
            }
        }
        return result;
    }
    CAmount getRequiredFee(unsigned int tx_bytes) override { return GetRequiredFee(*m_wallet, tx_bytes); }
    CAmount getMinimumFee(unsigned int tx_bytes,
        const CCoinControl& coin_control,
        int* returned_target,
        FeeReason* reason) override
    {
        FeeCalculation fee_calc;
        CAmount result;
        result = GetMinimumFee(*m_wallet, tx_bytes, coin_control, &fee_calc);
        if (returned_target) *returned_target = fee_calc.returnedTarget;
        if (reason) *reason = fee_calc.reason;
        return result;
    }
    unsigned int getConfirmTarget() override { return m_wallet->m_confirm_target; }
    bool hdEnabled() override { return m_wallet->IsHDEnabled(); }
    bool canGetAddresses() override { return m_wallet->CanGetAddresses(); }
    bool hasExternalSigner() override { return m_wallet->IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER); }
    bool privateKeysDisabled() override { return m_wallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS); }
    CAmount getDefaultMaxTxFee() override { return m_wallet->m_default_max_tx_fee; }
    void remove() override
    {
        RemoveWallet(m_context, m_wallet, /*load_on_start=*/false);
    }
    bool isLegacy() override { return m_wallet->IsLegacy(); }
    bool getMnemonic(SecureString& mnemonic_out, SecureString& mnemonic_passphrase_out) override
    {
        LOCK(m_wallet->cs_wallet);

        mnemonic_out.clear();
        mnemonic_passphrase_out.clear();

        if (m_wallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
            return false;
        }

        if (m_wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
            // Descriptor wallet
            for (auto spk_man : m_wallet->GetActiveScriptPubKeyMans()) {
                if (auto desc_spk_man = dynamic_cast<DescriptorScriptPubKeyMan*>(spk_man)) {
                    if (desc_spk_man->GetMnemonicString(mnemonic_out, mnemonic_passphrase_out)) {
                        return true;
                    }
                }
            }
            return false;
        } else {
            // Legacy wallet
            auto spk_man = m_wallet->GetLegacyScriptPubKeyMan();
            if (!spk_man) {
                return false;
            }

            CHDChain hdChainCurrent;
            if (!spk_man->GetHDChain(hdChainCurrent)) {
                return false;
            }

            // Get decrypted HD chain if wallet is encrypted
            if (m_wallet->IsCrypted()) {
                if (!spk_man->GetDecryptedHDChain(hdChainCurrent)) {
                    return false;
                }
            }

            return hdChainCurrent.GetMnemonic(mnemonic_out, mnemonic_passphrase_out);
        }
    }
    std::unique_ptr<Handler> handleUnload(UnloadFn fn) override
    {
        return MakeSignalHandler(m_wallet->NotifyUnload.connect(fn));
    }
    std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) override
    {
        return MakeSignalHandler(m_wallet->ShowProgress.connect(fn));
    }
    std::unique_ptr<Handler> handleStatusChanged(StatusChangedFn fn) override
    {
        return MakeSignalHandler(m_wallet->NotifyStatusChanged.connect([fn](CWallet*) { fn(); }));
    }
    std::unique_ptr<Handler> handleAddressBookChanged(AddressBookChangedFn fn) override
    {
        return MakeSignalHandler(m_wallet->NotifyAddressBookChanged.connect(
            [fn](const CTxDestination& address, const std::string& label, bool is_mine,
                 AddressPurpose purpose, ChangeType status) { fn(address, label, is_mine, purpose, status); }));
    }
    std::unique_ptr<Handler> handleTransactionChanged(TransactionChangedFn fn) override
    {
        return MakeSignalHandler(m_wallet->NotifyTransactionChanged.connect(
            [fn](const uint256& txid, ChangeType status) { fn(txid, status); }));
    }
    std::unique_ptr<Handler> handleInstantLockReceived(InstantLockReceivedFn fn) override
    {
        return MakeSignalHandler(m_wallet->NotifyISLockReceived.connect(
            [fn]() { fn(); }));
    }
    std::unique_ptr<Handler> handleChainLockReceived(ChainLockReceivedFn fn) override
    {
        return MakeSignalHandler(m_wallet->NotifyChainLockReceived.connect(
            [fn](int chainLockHeight) { fn(chainLockHeight); }));
    }
    std::unique_ptr<Handler> handleWatchOnlyChanged(WatchOnlyChangedFn fn) override
    {
        return MakeSignalHandler(m_wallet->NotifyWatchonlyChanged.connect(fn));
    }
    std::unique_ptr<Handler> handleCanGetAddressesChanged(CanGetAddressesChangedFn fn) override
    {
        return MakeSignalHandler(m_wallet->NotifyCanGetAddressesChanged.connect(fn));
    }
    std::unique_ptr<Handler> handleLockedCoinsChanged(LockedCoinsChangedFn fn) override
    {
        return MakeSignalHandler(m_wallet->NotifyLockedCoinsChanged.connect(fn));
    }
    std::vector<Governance::Object> getGovernanceObjects() override
    {
        LOCK(m_wallet->cs_wallet);
        std::vector<Governance::Object> result;
        for (const auto* obj : m_wallet->GetGovernanceObjects()) {
            result.push_back(*obj);
        }
        return result;
    }
    bool prepareProposal(const uint256& govobj_hash, CAmount fee, int32_t revision, int64_t created_time,
                         const std::string& data_hex, const COutPoint& outpoint,
                         std::string& out_fee_txid, std::string& error) override
    {
        LOCK(m_wallet->cs_wallet);
        CTransactionRef tx;
        if (!GenBudgetSystemCollateralTx(*m_wallet, tx, govobj_hash, fee, outpoint)) {
            error = "Error making collateral transaction for governance object.";
            return false;
        }
        if (!m_wallet->WriteGovernanceObject(Governance::Object{uint256{}, revision, created_time, tx->GetHash(), data_hex})) {
            error = "WriteGovernanceObject failed";
            return false;
        }
        m_wallet->CommitTransaction(tx, {}, {});
        out_fee_txid = tx->GetHash().ToString();
        return true;
    }
    bool signGovernanceVote(const CKeyID& keyID, CGovernanceVote& vote) override
    {
        return m_wallet->SignGovernanceVote(keyID, vote);
    }
    CWallet* wallet() override { return m_wallet.get(); }

    WalletContext& m_context;
    std::shared_ptr<CWallet> m_wallet;
};

class WalletLoaderImpl : public WalletLoader
{
private:
    void RegisterRPCs(const Span<const CRPCCommand>& commands)
    {
        for (const CRPCCommand& command : commands) {
            m_rpc_commands.emplace_back(command.category, command.name, [this, &command](const JSONRPCRequest& request, UniValue& result, bool last_handler) {
                JSONRPCRequest wallet_request = request;
                wallet_request.context = m_context;
                return command.actor(wallet_request, result, last_handler);
            }, command.argNames, command.unique_id);
            m_rpc_handlers.emplace_back(m_context.chain->handleRpc(m_rpc_commands.back()));
        }
    }

public:
    WalletLoaderImpl(Chain& chain, ArgsManager& args, NodeContext& node_context,
                     interfaces::CoinJoin::Loader& coinjoin_loader)
    {
        m_context.chain = &chain;
        m_context.args = &args;
        m_context.node_context = &node_context;
        m_context.coinjoin_loader = &coinjoin_loader;
    }
    ~WalletLoaderImpl() override { stop(); }

    //! ChainClient methods
    void registerRpcs() override
    {
        RegisterRPCs(GetWalletRPCCommands());
    }
    bool verify() override { return VerifyWallets(m_context); }
    bool load() override { return LoadWallets(m_context); }
    void start(CScheduler& scheduler) override { return StartWallets(m_context, scheduler); }
    void flush() override { return FlushWallets(m_context); }
    void stop() override { return UnloadWallets(m_context); }
    void setMockTime(int64_t time) override { return SetMockTime(time); }

    //! WalletLoader methods
    void registerOtherRpcs(const Span<const CRPCCommand>& commands) override
    {
        return RegisterRPCs(commands);
    }
    util::Result<std::unique_ptr<Wallet>> createWallet(const std::string& name, const SecureString& passphrase, uint64_t wallet_creation_flags, std::vector<bilingual_str>& warnings) override
    {
        DatabaseOptions options;
        DatabaseStatus status;
        ReadDatabaseArgs(*m_context.args, options);
        options.require_create = true;
        options.create_flags = wallet_creation_flags;
        options.create_passphrase = passphrase;
        bilingual_str error;
        std::unique_ptr<Wallet> wallet{MakeWallet(m_context, CreateWallet(m_context, name, /*load_on_start=*/true, options, status, error, warnings))};
        if (wallet) {
            return {std::move(wallet)};
        } else {
            return util::Error{error};
        }
    }
    util::Result<std::unique_ptr<Wallet>> loadWallet(const std::string& name, std::vector<bilingual_str>& warnings) override
    {
        DatabaseOptions options;
        DatabaseStatus status;
        ReadDatabaseArgs(*m_context.args, options);
        options.require_existing = true;
        bilingual_str error;
        std::unique_ptr<Wallet> wallet{MakeWallet(m_context, LoadWallet(m_context, name, /*load_on_start=*/true, options, status, error, warnings))};
        if (wallet) {
            return {std::move(wallet)};
        } else {
            return util::Error{error};
        }
    }
    util::Result<std::unique_ptr<Wallet>> restoreWallet(const fs::path& backup_file, const std::string& wallet_name, std::vector<bilingual_str>& warnings) override
    {
        DatabaseStatus status;
        bilingual_str error;
        std::unique_ptr<Wallet> wallet{MakeWallet(m_context, RestoreWallet(m_context, backup_file, wallet_name, /*load_on_start=*/true, status, error, warnings))};
        if (wallet) {
            return {std::move(wallet)};
        } else {
            return util::Error{error};
        }
    }
    util::Result<WalletMigrationResult> migrateWallet(const std::string& name, const SecureString& passphrase) override
    {
        auto res = wallet::MigrateLegacyToDescriptor(name, passphrase, m_context);
        if (!res) return util::Error{util::ErrorString(res)};
        WalletMigrationResult out{
            .wallet = MakeWallet(m_context, res->wallet),
            .watchonly_wallet_name = res->watchonly_wallet ? std::make_optional(res->watchonly_wallet->GetName()) : std::nullopt,
            .solvables_wallet_name = res->solvables_wallet ? std::make_optional(res->solvables_wallet->GetName()) : std::nullopt,
            .backup_path = res->backup_path,
        };
        return {std::move(out)}; // std::move to work around clang bug
    }
    std::string getWalletDir() override
    {
        return fs::PathToString(GetWalletDir());
    }
    std::vector<std::string> listWalletDir() override
    {
        std::vector<std::string> paths;
        for (auto& path : ListDatabases(GetWalletDir())) {
            paths.push_back(fs::PathToString(path));
        }
        return paths;
    }
    std::vector<std::unique_ptr<Wallet>> getWallets() override
    {
        std::vector<std::unique_ptr<Wallet>> wallets;
        for (const auto& wallet : GetWallets(m_context)) {
            wallets.emplace_back(MakeWallet(m_context, wallet));
        }
        return wallets;
    }
    std::unique_ptr<Handler> handleLoadWallet(LoadWalletFn fn) override
    {
        return HandleLoadWallet(m_context, std::move(fn));
    }
    std::unique_ptr<Handler> handleLoadWalletLoading(LoadWalletFn fn) override
    {
        return HandleLoadWalletLoading(m_context, std::move(fn));
    }
    WalletContext* context() override  { return &m_context; }

    WalletContext m_context;
    const std::vector<std::string> m_wallet_filenames;
    std::vector<std::unique_ptr<Handler>> m_rpc_handlers;
    std::list<CRPCCommand> m_rpc_commands;
};
} // namespace
} // namespace wallet

namespace interfaces {
std::unique_ptr<Wallet> MakeWallet(wallet::WalletContext& context, const std::shared_ptr<wallet::CWallet>& wallet) { return wallet ? std::make_unique<wallet::WalletImpl>(context, wallet) : nullptr; }
std::unique_ptr<WalletLoader> MakeWalletLoader(Chain& chain, ArgsManager& args, NodeContext& node_context,
                                               interfaces::CoinJoin::Loader& coinjoin_loader)
{
    return std::make_unique<wallet::WalletLoaderImpl>(chain, args, node_context, coinjoin_loader);
}
} // namespace interfaces
