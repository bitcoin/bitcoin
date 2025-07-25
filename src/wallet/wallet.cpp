// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <crypto/common.h>
#include <fs.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <key.h>
#include <key_io.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <support/cleanse.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/error.h>
#include <util/moneystr.h>
#include <util/string.h>
#include <util/translation.h>
#ifdef USE_BDB
#include <wallet/bdb.h>
#endif
#include <wallet/bip39.h> // TODO(refactor): move dependency it to scriptpubkeyman.cpp
#include <wallet/coincontrol.h>
#include <wallet/context.h>
#include <warnings.h>

#include <coinjoin/options.h>
#include <evo/providertx.h>
#include <governance/vote.h>

#include <univalue.h>

#include <algorithm>
#include <cassert>
#include <ranges>

using interfaces::FoundBlock;

namespace wallet {
const std::map<uint64_t,std::string> WALLET_FLAG_CAVEATS{
    {WALLET_FLAG_AVOID_REUSE,
        "You need to rescan the blockchain in order to correctly mark used "
        "destinations in the past. Until this is done, some destinations may "
        "be considered unused, even if the opposite is the case."
    },
};

bool AddWalletSetting(interfaces::Chain& chain, const std::string& wallet_name)
{
    util::SettingsValue setting_value = chain.getRwSetting("wallet");
    if (!setting_value.isArray()) setting_value.setArray();
    for (const util::SettingsValue& value : setting_value.getValues()) {
        if (value.isStr() && value.get_str() == wallet_name) return true;
    }
    setting_value.push_back(wallet_name);
    return chain.updateRwSetting("wallet", setting_value);
}

bool RemoveWalletSetting(interfaces::Chain& chain, const std::string& wallet_name)
{
    util::SettingsValue setting_value = chain.getRwSetting("wallet");
    if (!setting_value.isArray()) return true;
    util::SettingsValue new_value(util::SettingsValue::VARR);
    for (const util::SettingsValue& value : setting_value.getValues()) {
        if (!value.isStr() || value.get_str() != wallet_name) new_value.push_back(value);
    }
    if (new_value.size() == setting_value.size()) return true;
    return chain.updateRwSetting("wallet", new_value);
}

static void UpdateWalletSetting(interfaces::Chain& chain,
                                const std::string& wallet_name,
                                std::optional<bool> load_on_startup,
                                std::vector<bilingual_str>& warnings)
{
    if (load_on_startup == std::nullopt) return;
    if (load_on_startup.value() && !AddWalletSetting(chain, wallet_name)) {
        warnings.emplace_back(Untranslated("Wallet load on startup setting could not be updated, so wallet may not be loaded next node startup."));
    } else if (!load_on_startup.value() && !RemoveWalletSetting(chain, wallet_name)) {
        warnings.emplace_back(Untranslated("Wallet load on startup setting could not be updated, so wallet may still be loaded next node startup."));
    }
}

/**
 * Refresh mempool status so the wallet is in an internally consistent state and
 * immediately knows the transaction's status: Whether it can be considered
 * trusted and is eligible to be abandoned ...
 */
static void RefreshMempoolStatus(CWalletTx& tx, interfaces::Chain& chain)
{
    if (chain.isInMempool(tx.GetHash())) {
        tx.m_state = TxStateInMempool();
    } else if (tx.state<TxStateInMempool>()) {
        tx.m_state = TxStateInactive();
    }
}

bool AddWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet)
{
    {
        LOCK(context.wallets_mutex);
        assert(wallet);
        std::vector<std::shared_ptr<CWallet>>::const_iterator i = std::find(context.wallets.begin(), context.wallets.end(), wallet);
        if (i != context.wallets.end()) return false;
        context.wallets.push_back(wallet);
    }
    wallet->ConnectScriptPubKeyManNotifiers();
    wallet->AutoLockMasternodeCollaterals();
    if (wallet->coinjoin_available()) {
        wallet->coinjoin_loader().AddWallet(wallet);
    }
    wallet->NotifyCanGetAddressesChanged();
    return true;
}

bool RemoveWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start, std::vector<bilingual_str>& warnings)
{
    assert(wallet);

    interfaces::Chain& chain = wallet->chain();
    std::string name = wallet->GetName();

    // Unregister with the validation interface which also drops shared pointers.
    wallet->m_chain_notifications_handler.reset();
    {
        LOCK(context.wallets_mutex);
        std::vector<std::shared_ptr<CWallet>>::iterator i = std::find(context.wallets.begin(), context.wallets.end(), wallet);
        if (i == context.wallets.end()) return false;
        context.wallets.erase(i);
    }

    if (wallet->coinjoin_available()) {
        wallet->coinjoin_loader().RemoveWallet(name);
    }

    // Write the wallet setting
    UpdateWalletSetting(chain, name, load_on_start, warnings);

    return true;
}

bool RemoveWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start)
{
    std::vector<bilingual_str> warnings;
    return RemoveWallet(context, wallet, load_on_start, warnings);
}

std::vector<std::shared_ptr<CWallet>> GetWallets(WalletContext& context)
{
    LOCK(context.wallets_mutex);
    return context.wallets;
}

std::shared_ptr<CWallet> GetWallet(WalletContext& context, const std::string& name)
{
    LOCK(context.wallets_mutex);
    for (const std::shared_ptr<CWallet>& wallet : context.wallets) {
        if (wallet->GetName() == name) return wallet;
    }
    return nullptr;
}

std::unique_ptr<interfaces::Handler> HandleLoadWallet(WalletContext& context, LoadWalletFn load_wallet)
{
    LOCK(context.wallets_mutex);
    auto it = context.wallet_load_fns.emplace(context.wallet_load_fns.end(), std::move(load_wallet));
    return interfaces::MakeHandler([&context, it] { LOCK(context.wallets_mutex); context.wallet_load_fns.erase(it); });
}

void NotifyWalletLoaded(WalletContext& context, const std::shared_ptr<CWallet>& wallet)
{
    LOCK(context.wallets_mutex);
    for (auto& load_wallet : context.wallet_load_fns) {
        load_wallet(interfaces::MakeWallet(context, wallet));
    }
}

static Mutex g_loading_wallet_mutex;
static Mutex g_wallet_release_mutex;
static std::condition_variable g_wallet_release_cv;
static std::set<std::string> g_loading_wallet_set GUARDED_BY(g_loading_wallet_mutex);
static std::set<std::string> g_unloading_wallet_set GUARDED_BY(g_wallet_release_mutex);

// Custom deleter for shared_ptr<CWallet>.
static void ReleaseWallet(CWallet* wallet)
{
    const std::string name = wallet->GetName();
    wallet->WalletLogPrintf("Releasing wallet\n");
    wallet->Flush();
    delete wallet;
    // Wallet is now released, notify UnloadWallet, if any.
    {
        LOCK(g_wallet_release_mutex);
        if (g_unloading_wallet_set.erase(name) == 0) {
            // UnloadWallet was not called for this wallet, all done.
            return;
        }
    }
    g_wallet_release_cv.notify_all();
}

void UnloadWallet(std::shared_ptr<CWallet>&& wallet)
{
    // Mark wallet for unloading.
    const std::string name = wallet->GetName();
    {
        LOCK(g_wallet_release_mutex);
        auto it = g_unloading_wallet_set.insert(name);
        assert(it.second);
    }
    // The wallet can be in use so it's not possible to explicitly unload here.
    // Notify the unload intent so that all remaining shared pointers are
    // released.
    wallet->NotifyUnload();

    // Time to ditch our shared_ptr and wait for ReleaseWallet call.
    wallet.reset();
    {
        WAIT_LOCK(g_wallet_release_mutex, lock);
        while (g_unloading_wallet_set.count(name) == 1) {
            g_wallet_release_cv.wait(lock);
        }
    }
}

namespace {
std::shared_ptr<CWallet> LoadWalletInternal(WalletContext& context, const std::string& name, std::optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    try {
        std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(name, options, status, error);
        if (!database) {
            error = Untranslated("Wallet file verification failed.") + Untranslated(" ") + error;
            return nullptr;
        }

        context.chain->initMessage(_("Loading wallet…").translated);
        const std::shared_ptr<CWallet> wallet = CWallet::Create(context, name, std::move(database), options.create_flags, error, warnings);
        if (!wallet) {
            error = Untranslated("Wallet loading failed.") + Untranslated(" ") + error;
            status = DatabaseStatus::FAILED_LOAD;
            return nullptr;
        }

        NotifyWalletLoaded(context, wallet);
        AddWallet(context, wallet);
        wallet->postInitProcess();

        // Write the wallet setting
        UpdateWalletSetting(*context.chain, name, load_on_start, warnings);

        return wallet;
    } catch (const std::runtime_error& e) {
        error = Untranslated(e.what());
        status = DatabaseStatus::FAILED_LOAD;
        return nullptr;
    }
}
} // namespace

std::shared_ptr<CWallet> LoadWallet(WalletContext& context, const std::string& name, std::optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    auto result = WITH_LOCK(g_loading_wallet_mutex, return g_loading_wallet_set.insert(name));
    if (!result.second) {
        error = Untranslated("Wallet already loading.");
        status = DatabaseStatus::FAILED_LOAD;
        return nullptr;
    }
    auto wallet = LoadWalletInternal(context, name, load_on_start, options, status, error, warnings);
    WITH_LOCK(g_loading_wallet_mutex, g_loading_wallet_set.erase(result.first));
    return wallet;
}

std::shared_ptr<CWallet> CreateWallet(WalletContext& context, const std::string& name, std::optional<bool> load_on_start, DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    ArgsManager& args = *Assert(context.args);

    uint64_t wallet_creation_flags = options.create_flags;
    const SecureString& passphrase = options.create_passphrase;

    if (wallet_creation_flags & WALLET_FLAG_DESCRIPTORS) options.require_format = DatabaseFormat::SQLITE;

    // Indicate that the wallet is actually supposed to be blank and not just blank to make it encrypted
    bool create_blank = (wallet_creation_flags & WALLET_FLAG_BLANK_WALLET);

    // Born encrypted wallets need to be created blank first.
    if (!passphrase.empty()) {
        wallet_creation_flags |= WALLET_FLAG_BLANK_WALLET;
    }

    // Do not allow a passphrase when private keys are disabled
    if (!passphrase.empty() && (wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        error = Untranslated("Passphrase provided but private keys are disabled. A passphrase is only used to encrypt private keys, so cannot be used for wallets with private keys disabled.");
        status = DatabaseStatus::FAILED_CREATE;
        return nullptr;
    }

    // Wallet::Verify will check if we're trying to create a wallet with a duplicate name.
    std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(name, options, status, error);
    if (!database) {
        error = Untranslated("Wallet file verification failed.") + Untranslated(" ") + error;
        status = DatabaseStatus::FAILED_VERIFY;
        return nullptr;
    }

    // Make the wallet
    context.chain->initMessage(_("Loading wallet…").translated);
    const std::shared_ptr<CWallet> wallet = CWallet::Create(context, name, std::move(database), wallet_creation_flags, error, warnings);
    if (!wallet) {
        error = Untranslated("Wallet creation failed.") + Untranslated(" ") + error;
        status = DatabaseStatus::FAILED_CREATE;
        return nullptr;
    }
    if (args.GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET)) {
        wallet->WalletLogPrintf("Set HD by default\n");
        wallet->SetMinVersion(FEATURE_HD);
    }

    // Encrypt the wallet
    if (!passphrase.empty() && !(wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        if (!wallet->EncryptWallet(passphrase)) {
            error = Untranslated("Error: Wallet created but failed to encrypt.");
            status = DatabaseStatus::FAILED_ENCRYPT;
            return nullptr;
        }
        if (!create_blank) {
            // Unlock the wallet
            if (!wallet->Unlock(passphrase)) {
                error = Untranslated("Error: Wallet was encrypted but could not be unlocked");
                status = DatabaseStatus::FAILED_ENCRYPT;
                return nullptr;
            }

            // Set a seed for the wallet
            if (wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
                LOCK(wallet->cs_wallet);
                wallet->SetupDescriptorScriptPubKeyMans("", "");
            } else {
                // TODO: drop this condition after removing option to create non-HD wallets
                // related backport bitcoin#11250
                if (wallet->GetVersion() >= FEATURE_HD) {
                    if (!wallet->GenerateNewHDChain(/*secureMnemonic=*/"", /*secureMnemonicPassphrase=*/"", passphrase)) {
                       error = Untranslated("Error: Failed to generate encrypted HD wallet");
                       status = DatabaseStatus::FAILED_CREATE;
                       return nullptr;
                    }
                }
            }

            // backup the wallet we just encrypted
            if (!wallet->AutoBackupWallet("", error, warnings) && !error.original.empty()) {
                status = DatabaseStatus::FAILED_ENCRYPT;
                return nullptr;
            }

            // Relock the wallet
            wallet->Lock();
        }
    }

    NotifyWalletLoaded(context, wallet);
    AddWallet(context, wallet);
    wallet->postInitProcess();

    // Write the wallet settings
    UpdateWalletSetting(*context.chain, name, load_on_start, warnings);

    status = DatabaseStatus::SUCCESS;
    return wallet;
}

std::shared_ptr<CWallet> RestoreWallet(WalletContext& context, const fs::path& backup_file, const std::string& wallet_name, std::optional<bool> load_on_start, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    DatabaseOptions options;
    ReadDatabaseArgs(*context.args, options);
    options.require_existing = true;

    const fs::path wallet_path = fsbridge::AbsPathJoin(GetWalletDir(), fs::u8path(wallet_name));
    auto wallet_file = wallet_path / "wallet.dat";
    std::shared_ptr<CWallet> wallet;

    try {
        if (!fs::exists(backup_file)) {
            error = Untranslated("Backup file does not exist");
            status = DatabaseStatus::FAILED_INVALID_BACKUP_FILE;
            return nullptr;
        }

        if (fs::exists(wallet_path) || !TryCreateDirectories(wallet_path)) {
            error = Untranslated(strprintf("Failed to create database path '%s'. Database already exists.", fs::PathToString(wallet_path)));
            status = DatabaseStatus::FAILED_ALREADY_EXISTS;
            return nullptr;
        }

        fs::copy_file(backup_file, wallet_file, fs::copy_options::none);

        wallet = LoadWallet(context, wallet_name, load_on_start, options, status, error, warnings);
    } catch (const std::exception& e) {
        assert(!wallet);
        if (!error.empty()) error += Untranslated("\n");
        error += strprintf(Untranslated("Unexpected exception: %s"), e.what());
    }
    if (!wallet) {
        fs::remove(wallet_file);
        fs::remove(wallet_path);
    }

    return wallet;
}

/** @defgroup mapWallet
 *
 * @{
 */

const CWalletTx* CWallet::GetWalletTx(const uint256& hash) const
{
    AssertLockHeld(cs_wallet);
    const auto it = mapWallet.find(hash);
    if (it == mapWallet.end())
        return nullptr;
    return &(it->second);
}

void CWallet::UpgradeKeyMetadata()
{
    if (IsLocked() || IsWalletFlagSet(WALLET_FLAG_KEY_ORIGIN_METADATA)) {
        return;
    }

    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return;
    }

    spk_man->UpgradeKeyMetadata();
    SetWalletFlag(WALLET_FLAG_KEY_ORIGIN_METADATA);
}

void CWallet::UpgradeDescriptorCache()
{
    if (!IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS) || IsLocked() || IsWalletFlagSet(WALLET_FLAG_LAST_HARDENED_XPUB_CACHED)) {
        return;
    }

    for (ScriptPubKeyMan* spkm : GetAllScriptPubKeyMans()) {
        DescriptorScriptPubKeyMan* desc_spkm = dynamic_cast<DescriptorScriptPubKeyMan*>(spkm);
        desc_spkm->UpgradeDescriptorCache();
    }
    SetWalletFlag(WALLET_FLAG_LAST_HARDENED_XPUB_CACHED);
}

bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
{
    bool fWasLocked = IsLocked(true);

    {
        LOCK(cs_wallet);
        Lock();

        CCrypter crypter;
        CKeyingMaterial _vMasterKey;
        for (MasterKeyMap::value_type& pMasterKey : mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, _vMasterKey))
                return false;
            if (Unlock(_vMasterKey))
            {
                int64_t nStartTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = static_cast<unsigned int>(pMasterKey.second.nDeriveIterations * (100 / ((double)(TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - nStartTime))));

                nStartTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + static_cast<unsigned int>(pMasterKey.second.nDeriveIterations * 100 / ((double)(TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - nStartTime)))) / 2;

                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;

                WalletLogPrintf("Wallet passphrase changed to an nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(_vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;
                WalletBatch(GetDatabase()).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked)
                    Lock();

                return true;
            }
        }
    }

    return false;
}

void CWallet::chainStateFlushed(const CBlockLocator& loc)
{
    // Don't update the best block until the chain is attached so that in case of a shutdown,
    // the rescan will be restarted at next startup.
    if (m_attaching_chain) {
        return;
    }
    WalletBatch batch(GetDatabase());
    batch.WriteBestBlock(loc);
}

void CWallet::SetMinVersion(enum WalletFeature nVersion, WalletBatch* batch_in)
{
    LOCK(cs_wallet);
    if (nWalletVersion >= nVersion)
        return;
    WalletLogPrintf("Setting minversion to %d\n", nVersion);
    nWalletVersion = nVersion;

    {
        WalletBatch* batch = batch_in ? batch_in : new WalletBatch(GetDatabase());
        if (nWalletVersion > 40000)
            batch->WriteMinVersion(nWalletVersion);
        if (!batch_in)
            delete batch;
    }
}

std::set<uint256> CWallet::GetConflicts(const uint256& txid) const
{
    std::set<uint256> result;
    AssertLockHeld(cs_wallet);

    const auto it = mapWallet.find(txid);
    if (it == mapWallet.end())
        return result;
    const CWalletTx& wtx = it->second;

    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;

    for (const CTxIn& txin : wtx.tx->vin)
    {
        if (mapTxSpends.count(txin.prevout) <= 1)
            continue;  // No conflict if zero or one spends
        range = mapTxSpends.equal_range(txin.prevout);
        for (TxSpends::const_iterator _it = range.first; _it != range.second; ++_it)
            result.insert(_it->second);
    }
    return result;
}

bool CWallet::HasWalletSpend(const CTransactionRef& tx) const
{
    AssertLockHeld(cs_wallet);
    const uint256& txid = tx->GetHash();
    for (unsigned int i = 0; i < tx->vout.size(); ++i) {
        auto iter = mapTxSpends.find(COutPoint(txid, i));
        if (iter != mapTxSpends.end()) {
            return true;
        }
    }
    return false;
}

void CWallet::Flush()
{
    GetDatabase().Flush();
}

void CWallet::Close()
{
    GetDatabase().Close();
}

void CWallet::SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator> range)
{
    // We want all the wallet transactions in range to have the same metadata as
    // the oldest (smallest nOrderPos).
    // So: find smallest nOrderPos:

    int nMinOrderPos = std::numeric_limits<int>::max();
    const CWalletTx* copyFrom = nullptr;
    for (TxSpends::iterator it = range.first; it != range.second; ++it) {
        const CWalletTx* wtx = &mapWallet.at(it->second);
        if (wtx->nOrderPos < nMinOrderPos) {
            nMinOrderPos = wtx->nOrderPos;
            copyFrom = wtx;
        }
    }

    if (!copyFrom) {
        return;
    }

    // Now copy data from copyFrom to rest:
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        CWalletTx* copyTo = &mapWallet.at(hash);
        if (copyFrom == copyTo) continue;
        assert(copyFrom && "Oldest wallet transaction in range assumed to have been found.");
        if (!copyFrom->IsEquivalentTo(*copyTo)) continue;
        copyTo->mapValue = copyFrom->mapValue;
        copyTo->vOrderForm = copyFrom->vOrderForm;
        // fTimeReceivedIsTxTime not copied on purpose
        // nTimeReceived not copied on purpose
        copyTo->nTimeSmart = copyFrom->nTimeSmart;
        copyTo->fFromMe = copyFrom->fFromMe;
        // nOrderPos not copied on purpose
        // cached members not copied on purpose
    }
}

/**
 * Outpoint is spent if any non-conflicted transaction
 * spends it:
 */
bool CWallet::IsSpent(const COutPoint& outpoint) const
{
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    range = mapTxSpends.equal_range(outpoint);

    for (TxSpends::const_iterator it = range.first; it != range.second; ++it) {
        const uint256& wtxid = it->second;
        const auto mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end()) {
            int depth = GetTxDepthInMainChain(mit->second);
            if (depth > 0  || (depth == 0 && !mit->second.isAbandoned()))
                return true; // Spent
        }
    }
    return false;
}

void CWallet::AddToSpends(const COutPoint& outpoint, const uint256& wtxid, WalletBatch* batch)
{
    mapTxSpends.insert(std::make_pair(outpoint, wtxid));
    setWalletUTXO.erase(outpoint);

    if (batch) {
        UnlockCoin(outpoint, batch);
    } else {
        WalletBatch temp_batch(GetDatabase());
        UnlockCoin(outpoint, &temp_batch);
    }

    std::pair<TxSpends::iterator, TxSpends::iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    SyncMetaData(range);
}


void CWallet::AddToSpends(const CWalletTx& wtx, WalletBatch* batch)
{
    if (wtx.IsCoinBase()) // Coinbases don't spend anything!
        return;

    for (const CTxIn& txin : wtx.tx->vin)
        AddToSpends(txin.prevout, wtx.GetHash(), batch);
}

bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
        return false;

    CKeyingMaterial _vMasterKey;

    _vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetStrongRandBytes(_vMasterKey);

    CMasterKey kMasterKey;

    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetStrongRandBytes(kMasterKey.vchSalt);

    CCrypter crypter;
    int64_t nStartTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = static_cast<unsigned int>(2500000 / ((double)(TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - nStartTime)));

    nStartTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + static_cast<unsigned int>(kMasterKey.nDeriveIterations * 100 / ((double)(TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - nStartTime)))) / 2;

    if (kMasterKey.nDeriveIterations < 25000)
        kMasterKey.nDeriveIterations = 25000;

    WalletLogPrintf("Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(_vMasterKey, kMasterKey.vchCryptedKey))
        return false;

    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        WalletBatch* encrypted_batch = new WalletBatch(GetDatabase());
        if (!encrypted_batch->TxnBegin()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            return false;
        }
        encrypted_batch->WriteMasterKey(nMasterKeyMaxID, kMasterKey);

        for (const auto& spk_man_pair : m_spk_managers) {
            auto spk_man = spk_man_pair.second.get();

            if (!spk_man->Encrypt(_vMasterKey, encrypted_batch)) {
                encrypted_batch->TxnAbort();
                delete encrypted_batch;
                encrypted_batch = nullptr;
                // We now probably have half of our keys encrypted in memory, and half not...
                // die and let the user reload the unencrypted wallet.
                assert(false);
            }
        }


        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, encrypted_batch);

        if (!encrypted_batch->TxnCommit()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            // We now have keys encrypted in memory, but not on disk...
            // die to avoid confusion and let the user reload the unencrypted wallet.
            assert(false);
        }

        delete encrypted_batch;
        encrypted_batch = nullptr;

        Lock();
        Unlock(strWalletPassphrase);

        // If we are using descriptors, make new descriptors with a new seed
        if (IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS) && !IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET)) {
            // Do nothing for descriptor wallets (keep old seed / mnemonic)
        } else if (auto spk_man = GetLegacyScriptPubKeyMan()) {
            // if we are not using HD, generate new keypool
            if (spk_man->IsHDEnabled()) {
                if (!spk_man->TopUp()) {
                    return false;
                }
            }
            else {
                spk_man->NewKeyPool();
            }
        }

        Lock();

        // Need to completely rewrite the wallet file; if we don't, bdb might keep
        // bits of the unencrypted private key in slack space in the database file.
        GetDatabase().Rewrite();

        // BDB seems to have a bad habit of writing old data into
        // slack space in .dat files; that is bad if the old data is
        // unencrypted private keys. So:
        GetDatabase().ReloadDbEnv();

    }
    NotifyStatusChanged(this);

    return true;
}

DBErrors CWallet::ReorderTransactions()
{
    LOCK(cs_wallet);
    WalletBatch batch(GetDatabase());

    // Old wallets didn't have any defined order for transactions
    // Probably a bad idea to change the output of this

    // First: get all CWalletTx into a sorted-by-time multimap.
    typedef std::multimap<int64_t, CWalletTx*> TxItems;
    TxItems txByTime;

    for (auto& entry : mapWallet)
    {
        CWalletTx* wtx = &entry.second;
        txByTime.insert(std::make_pair(wtx->nTimeReceived, wtx));
    }

    nOrderPosNext = 0;
    std::vector<int64_t> nOrderPosOffsets;
    for (TxItems::iterator it = txByTime.begin(); it != txByTime.end(); ++it)
    {
        CWalletTx *const pwtx = (*it).second;
        int64_t& nOrderPos = pwtx->nOrderPos;

        if (nOrderPos == -1)
        {
            nOrderPos = nOrderPosNext++;
            nOrderPosOffsets.push_back(nOrderPos);

            if (!batch.WriteTx(*pwtx))
                return DBErrors::LOAD_FAIL;
        }
        else
        {
            int64_t nOrderPosOff = 0;
            for (const int64_t& nOffsetStart : nOrderPosOffsets)
            {
                if (nOrderPos >= nOffsetStart)
                    ++nOrderPosOff;
            }
            nOrderPos += nOrderPosOff;
            nOrderPosNext = std::max(nOrderPosNext, nOrderPos + 1);

            if (!nOrderPosOff)
                continue;

            // Since we're changing the order, write it back
            if (!batch.WriteTx(*pwtx))
                return DBErrors::LOAD_FAIL;
        }
    }
    batch.WriteOrderPosNext(nOrderPosNext);

    return DBErrors::LOAD_OK;
}

int64_t CWallet::IncOrderPosNext(WalletBatch* batch)
{
    AssertLockHeld(cs_wallet);
    int64_t nRet = nOrderPosNext++;
    if (batch) {
        batch->WriteOrderPosNext(nOrderPosNext);
    } else {
        WalletBatch(GetDatabase()).WriteOrderPosNext(nOrderPosNext);
    }
    return nRet;
}

void CWallet::MarkDirty()
{
    {
        LOCK(cs_wallet);
        for (std::pair<const uint256, CWalletTx>& item : mapWallet)
            item.second.MarkDirty();
    }

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::SetSpentKeyState(WalletBatch& batch, const uint256& hash, unsigned int n, bool used, std::set<CTxDestination>& tx_destinations)
{
    AssertLockHeld(cs_wallet);
    const CWalletTx* srctx = GetWalletTx(hash);
    if (!srctx) return;

    CTxDestination dst;
    if (ExtractDestination(srctx->tx->vout[n].scriptPubKey, dst)) {
        if (IsMine(dst)) {
            if (used != IsAddressUsed(dst)) {
                if (used) {
                    tx_destinations.insert(dst);
                }
                SetAddressUsed(batch, dst, used);
            }
        }
    }
}

bool CWallet::IsSpentKey(const CScript& scriptPubKey) const
{
    AssertLockHeld(cs_wallet);
    CTxDestination dest;
    if (!ExtractDestination(scriptPubKey, dest)) {
        return false;
    }
    if (IsAddressUsed(dest)) {
        return true;
    }
    if (IsLegacy()) {
        LegacyScriptPubKeyMan* spk_man = GetLegacyScriptPubKeyMan();
        assert(spk_man != nullptr);
        for (const auto& keyid : GetAffectedKeys(scriptPubKey, *spk_man)) {
            if (IsAddressUsed(PKHash(keyid))) {
                return true;
            }
        }
    }
    return false;
}

CWalletTx* CWallet::AddToWallet(CTransactionRef tx, const TxState& state, const UpdateWalletTxFn& update_wtx, bool fFlushOnClose, bool rescanning_old_block)
{
    LOCK(cs_wallet);

    WalletBatch batch(GetDatabase(), fFlushOnClose);

    uint256 hash = tx->GetHash();

    if (IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE)) {
        // Mark used destinations
        std::set<CTxDestination> tx_destinations;

        for (const CTxIn& txin : tx->vin) {
            const COutPoint& op = txin.prevout;
            SetSpentKeyState(batch, op.hash, op.n, true, tx_destinations);
        }

        MarkDestinationsDirty(tx_destinations);
    }

    // Inserts only if not already there, returns tx inserted or tx found
    auto ret = mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(tx, state));
    CWalletTx& wtx = (*ret.first).second;
    bool fInsertedNew = ret.second;
    bool fUpdated = update_wtx && update_wtx(wtx, fInsertedNew);
    std::set<COutPoint> candidates;
    if (fInsertedNew) {
        wtx.nTimeReceived = GetTime();
        wtx.nOrderPos = IncOrderPosNext(&batch);
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
        wtx.nTimeSmart = ComputeTimeSmart(wtx, rescanning_old_block);
        AddToSpends(wtx, &batch);
        candidates = AddWalletUTXOs(wtx.tx, /*ret_dups=*/true);
    }

    if (!fInsertedNew)
    {
        if (state.index() != wtx.m_state.index()) {
            wtx.m_state = state;
            fUpdated = true;
        } else {
            assert(TxStateSerializedIndex(wtx.m_state) == TxStateSerializedIndex(state));
            assert(TxStateSerializedBlockHash(wtx.m_state) == TxStateSerializedBlockHash(state));
        }
        candidates = AddWalletUTXOs(wtx.tx, /*ret_dups=*/false);
        if (!candidates.empty()) fUpdated = true;
    }

    LockProTxCoins(candidates, &batch);

    //// debug print
    WalletLogPrintf("AddToWallet %s  %s%s\n", hash.ToString(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

    // Write to disk
    if (fInsertedNew || fUpdated)
        if (!batch.WriteTx(wtx))
            return nullptr;

    // Break debit/credit balance caches:
    wtx.MarkDirty();

    // Notify UI of new or updated transaction
    NotifyTransactionChanged(hash, fInsertedNew ? CT_NEW : CT_UPDATED);

#if HAVE_SYSTEM
    // notify an external script when a wallet transaction comes in or is updated
    std::string strCmd = m_args.GetArg("-walletnotify", "");

    if (!strCmd.empty())
    {
        ReplaceAll(strCmd, "%s", hash.GetHex());
        if (auto* conf = wtx.state<TxStateConfirmed>())
        {
            ReplaceAll(strCmd, "%b", conf->confirmed_block_hash.GetHex());
            ReplaceAll(strCmd, "%h", ToString(conf->confirmed_block_height));
        } else {
            ReplaceAll(strCmd, "%b", "unconfirmed");
            ReplaceAll(strCmd, "%h", "-1");
        }
#ifndef WIN32
        // Substituting the wallet name isn't currently supported on windows
        // because windows shell escaping has not been implemented yet:
        // https://github.com/bitcoin/bitcoin/pull/13339#issuecomment-537384875
        // A few ways it could be implemented in the future are described in:
        // https://github.com/bitcoin/bitcoin/pull/13339#issuecomment-461288094
        ReplaceAll(strCmd, "%w", ShellEscape(GetName()));
#endif
        std::thread t(runCommand, strCmd);
        t.detach(); // thread runs free
    }
#endif

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;

    return &wtx;
}

bool CWallet::LoadToWallet(const uint256& hash, const UpdateWalletTxFn& fill_wtx)
{
    const auto& ins = mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(nullptr, TxStateInactive{}));
    CWalletTx& wtx = ins.first->second;
    if (!fill_wtx(wtx, ins.second)) {
        return false;
    }
    // If wallet doesn't have a chain (e.g when using dash-wallet tool),
    // don't bother to update txn.
    if (HaveChain()) {
        bool active;
        auto lookup_block = [&](const uint256& hash, int& height, TxState& state) {
            // If tx block (or conflicting block) was reorged out of chain
            // while the wallet was shutdown, change tx status to UNCONFIRMED
            // and reset block height, hash, and index. ABANDONED tx don't have
            // associated blocks and don't need to be updated. The case where a
            // transaction was reorged out while online and then reconfirmed
            // while offline is covered by the rescan logic.
            if (!chain().findBlock(hash, FoundBlock().inActiveChain(active).height(height)) || !active) {
                state = TxStateInactive{};
            }
        };
        if (auto* conf = wtx.state<TxStateConfirmed>()) {
            lookup_block(conf->confirmed_block_hash, conf->confirmed_block_height, wtx.m_state);
        } else if (auto* conf = wtx.state<TxStateConflicted>()) {
            lookup_block(conf->conflicting_block_hash, conf->conflicting_block_height, wtx.m_state);
        }
    }
    if (/* insertion took place */ ins.second) {
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
    }
    AddToSpends(wtx);
    for (const CTxIn& txin : wtx.tx->vin) {
        auto it = mapWallet.find(txin.prevout.hash);
        if (it != mapWallet.end()) {
            CWalletTx& prevtx = it->second;
            if (auto* prev = prevtx.state<TxStateConflicted>()) {
                MarkConflicted(prev->conflicting_block_hash, prev->conflicting_block_height, wtx.GetHash());
            }
        }
    }
    return true;
}

std::set<COutPoint> CWallet::AddWalletUTXOs(CTransactionRef tx, bool ret_dups)
{
    AssertLockHeld(cs_wallet);
    std::set<COutPoint> ret;
    uint256 hash{tx->GetHash()};
    for (size_t idx = 0; idx < tx->vout.size(); ++idx) {
        COutPoint outpoint(hash, idx);
        if (IsMine(tx->vout[idx]) && !IsSpent(outpoint)) {
            if (auto [_, inserted] = setWalletUTXO.emplace(outpoint); inserted || ret_dups) {
                ret.emplace(outpoint);
            }
        }
    }
    return ret;
}

bool CWallet::AddToWalletIfInvolvingMe(const CTransactionRef& ptx, const SyncTxState& state, WalletBatch& batch, bool fUpdate, bool rescanning_old_block)
{
    const CTransaction& tx = *ptx;
    {
        AssertLockHeld(cs_wallet);

        if (auto* conf = std::get_if<TxStateConfirmed>(&state)) {
            for (const CTxIn& txin : tx.vin) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(txin.prevout);
                while (range.first != range.second) {
                    if (range.first->second != tx.GetHash()) {
                        WalletLogPrintf("Transaction %s (in block %s) conflicts with wallet transaction %s (both spend %s:%i)\n", tx.GetHash().ToString(), conf->confirmed_block_hash.ToString(), range.first->second.ToString(), range.first->first.hash.ToString(), range.first->first.n);
                        MarkConflicted(conf->confirmed_block_hash, conf->confirmed_block_height, range.first->second);
                    }
                    range.first++;
                }
            }
        }

        bool fExisted = mapWallet.count(tx.GetHash()) != 0;
        if (fExisted && !fUpdate) return false;
        if (fExisted || IsMine(tx) || IsFromMe(tx))
        {
            /* Check if any keys in the wallet keypool that were supposed to be unused
             * have appeared in a new transaction. If so, remove those keys from the keypool.
             * This can happen when restoring an old wallet backup that does not contain
             * the mostly recently created transactions from newer versions of the wallet.
             */

            std::optional<int64_t> block_time;
            if (auto* conf = std::get_if<TxStateConfirmed>(&state)) {
                int64_t block_time_tmp;
                bool found_block = chain().findBlock(conf->confirmed_block_hash, FoundBlock().maxTime(block_time_tmp));
                assert(found_block);
                block_time = block_time_tmp;
            }
            // loop though all outputs
            for (const CTxOut& txout: tx.vout) {
                for (const auto& spk_man_pair : m_spk_managers) {
                    spk_man_pair.second->MarkUnusedAddresses(batch, txout.scriptPubKey, block_time);
                }
            }

            // Block disconnection override an abandoned tx as unconfirmed
            // which means user may have to call abandontransaction again
            TxState tx_state = std::visit([](auto&& s) -> TxState { return s; }, state);
            return AddToWallet(MakeTransactionRef(tx), tx_state, /*update_wtx=*/nullptr, /*fFlushOnClose=*/false, rescanning_old_block);
        }
    }
    return false;
}

bool CWallet::TransactionCanBeAbandoned(const uint256& hashTx) const
{
    LOCK(cs_wallet);
    const CWalletTx* wtx = GetWalletTx(hashTx);
    return wtx && !wtx->isAbandoned() && GetTxDepthInMainChain(*wtx) == 0 && !wtx->InMempool();
}

bool CWallet::TransactionCanBeResent(const uint256& hashTx) const
{
    LOCK(cs_wallet);
    const CWalletTx* wtx = GetWalletTx(hashTx);
    return wtx && CanTxBeResent(*wtx);
}

void CWallet::MarkInputsDirty(const CTransactionRef& tx)
{
    for (const CTxIn& txin : tx->vin) {
        auto it = mapWallet.find(txin.prevout.hash);
        if (it != mapWallet.end()) {
            it->second.MarkDirty();
        }
    }
}

bool CWallet::AbandonTransaction(const uint256& hashTx)
{
    LOCK(cs_wallet);

    WalletBatch batch(GetDatabase());

    std::set<uint256> todo;
    std::set<uint256> done;

    // Can't mark abandoned if confirmed or in mempool
    auto it = mapWallet.find(hashTx);
    assert(it != mapWallet.end());
    const CWalletTx& origtx = it->second;
    if (GetTxDepthInMainChain(origtx) != 0 || origtx.InMempool() || IsTxLockedByInstantSend(origtx)) {
        return false;
    }

    todo.insert(hashTx);

    while (!todo.empty()) {
        uint256 now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        auto it = mapWallet.find(now);
        assert(it != mapWallet.end());
        CWalletTx& wtx = it->second;
        int currentconfirm = GetTxDepthInMainChain(wtx);
        // If the orig tx was not in block, none of its spends can be
        assert(currentconfirm <= 0);
        // if (currentconfirm < 0) {Tx and spends are already conflicted, no need to abandon}
        if (currentconfirm == 0 && !wtx.isAbandoned()) {
            // If the orig tx was not in block/mempool, none of its spends can be in mempool
            assert(!wtx.InMempool());
            wtx.m_state = TxStateInactive{/*abandoned=*/true};
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            NotifyTransactionChanged(wtx.GetHash(), CT_UPDATED);
            // Iterate over all its outputs, and mark transactions in the wallet that spend them abandoned too
            for (unsigned int i = 0; i < wtx.tx->vout.size(); ++i) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(COutPoint(now, i));
                for (TxSpends::const_iterator iter = range.first; iter != range.second; ++iter) {
                    if (!done.count(iter->second)) {
                        todo.insert(iter->second);
                    }
                }
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            MarkInputsDirty(wtx.tx);
        }
    }

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;

    return true;
}

bool CWallet::ResendTransaction(const uint256& hashTx)
{
    LOCK(cs_wallet);

    auto it = mapWallet.find(hashTx);
    assert(it != mapWallet.end());
    CWalletTx& wtx = it->second;

    bilingual_str unused_err_string;
    return SubmitTxMemoryPoolAndRelay(wtx, unused_err_string, true);
}

void CWallet::MarkConflicted(const uint256& hashBlock, int conflicting_height, const uint256& hashTx)
{
    LOCK(cs_wallet);

    int conflictconfirms = (m_last_block_processed_height - conflicting_height + 1) * -1;
    // If number of conflict confirms cannot be determined, this means
    // that the block is still unknown or not yet part of the main chain,
    // for example when loading the wallet during a reindex. Do nothing in that
    // case.
    if (conflictconfirms >= 0)
        return;

    // Do not flush the wallet here for performance reasons
    WalletBatch batch(GetDatabase(), false);

    std::set<uint256> todo;
    std::set<uint256> done;

    todo.insert(hashTx);

    while (!todo.empty()) {
        uint256 now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        auto it = mapWallet.find(now);
        assert(it != mapWallet.end());
        CWalletTx& wtx = it->second;
        int currentconfirm = GetTxDepthInMainChain(wtx);
        if (conflictconfirms < currentconfirm) {
            // Block is 'more conflicted' than current confirm; update.
            // Mark transaction as conflicted with this block.
            wtx.m_state = TxStateConflicted{hashBlock, conflicting_height};
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            // Iterate over all its outputs, and mark transactions in the wallet that spend them conflicted too
            for (unsigned int i = 0; i < wtx.tx->vout.size(); ++i) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(COutPoint(now, i));
                for (TxSpends::const_iterator iter = range.first; iter != range.second; ++iter) {
                    if (!done.count(iter->second)) {
                        todo.insert(iter->second);
                    }
                }
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            MarkInputsDirty(wtx.tx);
        }
    }

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::SyncTransaction(const CTransactionRef& ptx, const SyncTxState& state, WalletBatch& batch, bool update_tx, bool rescanning_old_block)
{
    if (!AddToWalletIfInvolvingMe(ptx, state, batch, update_tx, rescanning_old_block))
        return; // Not one of ours

    // If a transaction changes 'conflicted' state, that changes the balance
    // available of the outputs it spends. So force those to be
    // recomputed, also:
    MarkInputsDirty(ptx);

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::transactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime, uint64_t mempool_sequence) {
    LOCK(cs_wallet);
    WalletBatch batch(GetDatabase());
    SyncTransaction(tx, TxStateInMempool{}, batch);

    auto it = mapWallet.find(tx->GetHash());
    if (it != mapWallet.end()) {
        RefreshMempoolStatus(it->second, chain());
    }
}

void CWallet::transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) {
    if (reason != MemPoolRemovalReason::CONFLICT) {
        LOCK(cs_wallet);
        auto it = mapWallet.find(tx->GetHash());
        if (it != mapWallet.end()) {
            RefreshMempoolStatus(it->second, chain());
        }
    }
    // Handle transactions that were removed from the mempool because they
    // conflict with transactions in a newly connected block.
    if (reason == MemPoolRemovalReason::CONFLICT) {
        // Trigger external -walletnotify notifications for these transactions.
        // Set Status::UNCONFIRMED instead of Status::CONFLICTED for a few reasons:
        //
        // 1. The transactionRemovedFromMempool callback does not currently
        //    provide the conflicting block's hash and height, and for backwards
        //    compatibility reasons it may not be not safe to store conflicted
        //    wallet transactions with a null block hash. See
        //    https://github.com/bitcoin/bitcoin/pull/18600#discussion_r420195993.
        // 2. For most of these transactions, the wallet's internal conflict
        //    detection in the blockConnected handler will subsequently call
        //    MarkConflicted and update them with CONFLICTED status anyway. This
        //    applies to any wallet transaction that has inputs spent in the
        //    block, or that has ancestors in the wallet with inputs spent by
        //    the block.
        // 3. Longstanding behavior since the sync implementation in
        //    https://github.com/bitcoin/bitcoin/pull/9371 and the prior sync
        //    implementation before that was to mark these transactions
        //    unconfirmed rather than conflicted.
        //
        // Nothing described above should be seen as an unchangeable requirement
        // when improving this code in the future. The wallet's heuristics for
        // distinguishing between conflicted and unconfirmed transactions are
        // imperfect, and could be improved in general, see
        // https://github.com/bitcoin-core/bitcoin-devwiki/wiki/Wallet-Transaction-Conflict-Tracking
        LOCK(cs_wallet);
        WalletBatch batch(GetDatabase());
        SyncTransaction(tx, TxStateInactive{}, batch);
    }
}

void CWallet::blockConnected(const CBlock& block, int height)
{
    const uint256& block_hash = block.GetHash();
    LOCK(cs_wallet);

    m_last_block_processed_height = height;
    m_last_block_processed = block_hash;
    WalletBatch batch(GetDatabase());
    for (size_t index = 0; index < block.vtx.size(); index++) {
        SyncTransaction(block.vtx[index], TxStateConfirmed{block_hash, height, static_cast<int>(index)}, batch);
        transactionRemovedFromMempool(block.vtx[index], MemPoolRemovalReason::BLOCK, 0 /* mempool_sequence */);
    }

    // reset cache to make sure no longer immature coins are included
    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::blockDisconnected(const CBlock& block, int height)
{
    LOCK(cs_wallet);

    // At block disconnection, this will change an abandoned transaction to
    // be unconfirmed, whether or not the transaction is added back to the mempool.
    // User may have to call abandontransaction again. It may be addressed in the
    // future with a stickier abandoned state or even removing abandontransaction call.
    m_last_block_processed_height = height - 1;
    m_last_block_processed = block.hashPrevBlock;
    WalletBatch batch(GetDatabase());
    for (const CTransactionRef& ptx : block.vtx) {
        SyncTransaction(ptx, TxStateInactive{}, batch);
    }

    // reset cache to make sure no longer mature coins are excluded
    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::updatedBlockTip()
{
    m_best_block_time = GetTime();
}

void CWallet::BlockUntilSyncedToCurrentChain() const {
    AssertLockNotHeld(cs_wallet);
    // Skip the queue-draining stuff if we know we're caught up with
    // chain().Tip(), otherwise put a callback in the validation interface queue and wait
    // for the queue to drain enough to execute it (indicating we are caught up
    // at least with the time we entered this function).
    uint256 last_block_hash = WITH_LOCK(cs_wallet, return m_last_block_processed);
    chain().waitForNotificationsIfTipChanged(last_block_hash);
}

// Note that this function doesn't distinguish between a 0-valued input,
// and a not-"is mine" (according to the filter) input.
CAmount CWallet::GetDebit(const CTxIn &txin, const isminefilter& filter) const
{
    {
        LOCK(cs_wallet);
        const auto mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.tx->vout.size())
                if (IsMine(prev.tx->vout[txin.prevout.n]) & filter)
                    return prev.tx->vout[txin.prevout.n].nValue;
        }
    }
    return 0;
}

isminetype CWallet::IsMine(const CTxOut& txout) const
{
    AssertLockHeld(cs_wallet);
    return IsMine(txout.scriptPubKey);
}

isminetype CWallet::IsMine(const CTxDestination& dest) const
{
    AssertLockHeld(cs_wallet);
    return IsMine(GetScriptForDestination(dest));
}

isminetype CWallet::IsMine(const CScript& script) const
{
    AssertLockHeld(cs_wallet);
    isminetype result = ISMINE_NO;
    for (const auto& spk_man_pair : m_spk_managers) {
        result = std::max(result, spk_man_pair.second->IsMine(script));
    }
    return result;
}

bool CWallet::IsMine(const CTransaction& tx) const
{
    AssertLockHeld(cs_wallet);
    for (const CTxOut& txout : tx.vout)
        if (IsMine(txout))
            return true;
    return false;
}

bool CWallet::IsFromMe(const CTransaction& tx) const
{
    return (GetDebit(tx, ISMINE_ALL) > 0);
}

CAmount CWallet::GetDebit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nDebit = 0;
    for (const CTxIn& txin : tx.vin)
    {
        nDebit += GetDebit(txin, filter);
        if (!MoneyRange(nDebit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nDebit;
}

bool CWallet::IsHDEnabled() const
{
    // All Active ScriptPubKeyMans must be HD for this to be true
    bool result = false;
    for (const auto& spk_man : GetActiveScriptPubKeyMans()) {
        if (!spk_man->IsHDEnabled()) return false;
        result = true;
    }
    return result;
}

bool CWallet::CanGetAddresses(bool internal) const
{
    LOCK(cs_wallet);
    if (m_spk_managers.empty()) return false;
    auto spk_man = GetScriptPubKeyMan(internal);
    if (spk_man && spk_man->CanGetAddresses(internal)) {
        return true;
    }
    return false;
}

void CWallet::SetWalletFlag(uint64_t flags)
{
    LOCK(cs_wallet);
    m_wallet_flags |= flags;
    if (!WalletBatch(GetDatabase()).WriteWalletFlags(m_wallet_flags))
        throw std::runtime_error(std::string(__func__) + ": writing wallet flags failed");
}

void CWallet::UnsetWalletFlag(uint64_t flag)
{
    LOCK(cs_wallet);
    WalletBatch batch(GetDatabase());
    UnsetWalletFlagWithDB(batch, flag);
}

void CWallet::UnsetWalletFlagWithDB(WalletBatch& batch, uint64_t flag)
{
    LOCK(cs_wallet);
    m_wallet_flags &= ~flag;
    if (!batch.WriteWalletFlags(m_wallet_flags))
        throw std::runtime_error(std::string(__func__) + ": writing wallet flags failed");
}

void CWallet::UnsetBlankWalletFlag(WalletBatch& batch)
{
    UnsetWalletFlagWithDB(batch, WALLET_FLAG_BLANK_WALLET);
}

void CWallet::NewKeyPoolCallback()
{
    // Note: GetClient(*this) can return nullptr when this wallet is in the middle of its creation.
    // Skipping stopMixing() is fine in this case.
    if (std::unique_ptr<interfaces::CoinJoin::Client> coinjoin_client = coinjoin_available() ? coinjoin_loader().GetClient(GetName()) : nullptr) {
        coinjoin_client->stopMixing();
    }
    nKeysLeftSinceAutoBackup = 0;
}

void CWallet::KeepDestinationCallback(bool erased)
{
    if (erased) --nKeysLeftSinceAutoBackup;
    if (!nWalletBackups) nKeysLeftSinceAutoBackup = 0;
}

bool CWallet::IsWalletFlagSet(uint64_t flag) const
{
    return (m_wallet_flags & flag);
}

bool CWallet::LoadWalletFlags(uint64_t flags)
{
    LOCK(cs_wallet);
    if (((flags & KNOWN_WALLET_FLAGS) >> 32) ^ (flags >> 32)) {
        // contains unknown non-tolerable wallet flags
        return false;
    }
    m_wallet_flags = flags;

    return true;
}

void CWallet::InitWalletFlags(uint64_t flags)
{
    LOCK(cs_wallet);

    // We should never be writing unknown non-tolerable wallet flags
    assert(((flags & KNOWN_WALLET_FLAGS) >> 32) == (flags >> 32));
    // This should only be used once, when creating a new wallet - so current flags are expected to be blank
    assert(m_wallet_flags == 0);

    if (!WalletBatch(GetDatabase()).WriteWalletFlags(flags)) {
        throw std::runtime_error(std::string(__func__) + ": writing wallet flags failed");
    }

    if (!LoadWalletFlags(flags)) assert(false);
}

// Helper for producing a max-sized low-S low-R signature (eg 71 bytes)
// or a max-sized low-S signature (e.g. 72 bytes) depending on coin_control
bool DummySignInput(const SigningProvider& provider, CTxIn &tx_in, const CTxOut &txout, const CCoinControl* coin_control)
{
    // Fill in dummy signatures for fee calculation.
    const CScript& scriptPubKey = txout.scriptPubKey;
    SignatureData sigdata;

    // Use max sig if watch only inputs were used or if this particular input is an external input
    // to ensure a sufficient fee is attained for the requested feerate.
    const bool use_max_sig = coin_control && (coin_control->fAllowWatchOnly || coin_control->IsExternalSelected(tx_in.prevout));
    if (!ProduceSignature(provider, use_max_sig ? DUMMY_MAXIMUM_SIGNATURE_CREATOR : DUMMY_SIGNATURE_CREATOR, scriptPubKey, sigdata)) {
        return false;
    }
    UpdateInput(tx_in, sigdata);
    return true;
}

// Helper for producing a bunch of max-sized low-S low-R signatures (eg 71 bytes)
bool CWallet::DummySignTx(CMutableTransaction &txNew, const std::vector<CTxOut> &txouts, const CCoinControl* coin_control) const
{
    // Fill in dummy signatures for fee calculation.
    int nIn = 0;
    for (const auto& txout : txouts)
    {
        CTxIn& txin = txNew.vin[nIn];
        const std::unique_ptr<SigningProvider> provider = GetSolvingProvider(txout.scriptPubKey);
        if (!provider || !DummySignInput(*provider, txin, txout, coin_control)) {
            if (!coin_control || !DummySignInput(coin_control->m_external_provider, txin, txout, coin_control)) {
                return false;
            }
        }

        nIn++;
    }
    return true;
}

bool CWallet::ImportScripts(const std::set<CScript> scripts, int64_t timestamp)
{
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    return spk_man->ImportScripts(scripts, timestamp);
}

bool CWallet::ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp)
{
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    return spk_man->ImportPrivKeys(privkey_map, timestamp);
}

bool CWallet::ImportPubKeys(const std::vector<CKeyID>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const bool internal, const int64_t timestamp)
{
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    return spk_man->ImportPubKeys(ordered_pubkeys, pubkey_map, key_origins, add_keypool, internal, timestamp);
}

bool CWallet::ImportScriptPubKeys(const std::string& label, const std::set<CScript>& script_pub_keys, const bool have_solving_data, const bool apply_label, const int64_t timestamp)
{
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    if (!spk_man->ImportScriptPubKeys(script_pub_keys, have_solving_data, timestamp)) {
        return false;
    }
    if (apply_label) {
        WalletBatch batch(GetDatabase());
        for (const CScript& script : script_pub_keys) {
            CTxDestination dest;
            ExtractDestination(script, dest);
            if (IsValidDestination(dest)) {
                SetAddressBookWithDB(batch, dest, label, "receive");
            }
        }
    }
    return true;
}

/**
 * Scan active chain for relevant transactions after importing keys. This should
 * be called whenever new keys are added to the wallet, with the oldest key
 * creation time.
 *
 * @return Earliest timestamp that could be successfully scanned from. Timestamp
 * returned will be higher than startTime if relevant blocks could not be read.
 */
int64_t CWallet::RescanFromTime(int64_t startTime, const WalletRescanReserver& reserver, bool update)
{
    // Find starting block. May be null if nCreateTime is greater than the
    // highest blockchain timestamp, in which case there is nothing that needs
    // to be scanned.
    int start_height = 0;
    uint256 start_block;
    bool start = chain().findFirstBlockWithTimeAndHeight(startTime - TIMESTAMP_WINDOW, 0, FoundBlock().hash(start_block).height(start_height));
    WalletLogPrintf("%s: Rescanning last %i blocks\n", __func__, start ? WITH_LOCK(cs_wallet, return GetLastBlockHeight()) - start_height + 1 : 0);

    if (start) {
        // TODO: this should take into account failure by ScanResult::USER_ABORT
        ScanResult result = ScanForWalletTransactions(start_block, start_height, /*max_height=*/{}, reserver, /*fUpdate=*/update, /*save_progress=*/false);
        if (result.status == ScanResult::FAILURE) {
            int64_t time_max;
            CHECK_NONFATAL(chain().findBlock(result.last_failed_block, FoundBlock().maxTime(time_max)));
            return time_max + TIMESTAMP_WINDOW + 1;
        }
    }
    return startTime;
}

/**
 * Scan the block chain (starting in start_block) for transactions
 * from or to us. If fUpdate is true, found transactions that already
 * exist in the wallet will be updated.
 *
 * @param[in] start_block Scan starting block. If block is not on the active
 *                        chain, the scan will return SUCCESS immediately.
 * @param[in] start_height Height of start_block
 * @param[in] max_height  Optional max scanning height. If unset there is
 *                        no maximum and scanning can continue to the tip
 *
 * @return ScanResult returning scan information and indicating success or
 *         failure. Return status will be set to SUCCESS if scan was
 *         successful. FAILURE if a complete rescan was not possible (due to
 *         pruning or corruption). USER_ABORT if the rescan was aborted before
 *         it could complete.
 *
 * @pre Caller needs to make sure start_block (and the optional stop_block) are on
 * the main chain after to the addition of any new keys you want to detect
 * transactions for.
 */
CWallet::ScanResult CWallet::ScanForWalletTransactions(const uint256& start_block, int start_height, std::optional<int> max_height, const WalletRescanReserver& reserver, bool fUpdate, const bool save_progress)
{
    constexpr auto INTERVAL_TIME{60s};
    auto current_time{reserver.now()};
    auto start_time{reserver.now()};

    assert(reserver.isReserved());

    uint256 block_hash = start_block;
    ScanResult result;

    WalletLogPrintf("Rescan started from block %s...\n", start_block.ToString());

    ShowProgress(strprintf("%s " + _("Rescanning…").translated, GetDisplayName()), 0); // show rescan progress in GUI as dialog or on splashscreen, if -rescan on startup
    uint256 tip_hash = WITH_LOCK(cs_wallet, return GetLastBlockHash());
    uint256 end_hash = tip_hash;
    if (max_height) chain().findAncestorByHeight(tip_hash, *max_height, FoundBlock().hash(end_hash));
    double progress_begin = chain().guessVerificationProgress(block_hash);
    double progress_end = chain().guessVerificationProgress(end_hash);
    double progress_current = progress_begin;
    int block_height = start_height;
    WalletBatch batch(GetDatabase());
    while (!fAbortRescan && !chain().shutdownRequested()) {
        if (progress_end - progress_begin > 0.0) {
            m_scanning_progress = (progress_current - progress_begin) / (progress_end - progress_begin);
        } else { // avoid divide-by-zero for single block scan range (i.e. start and stop hashes are equal)
            m_scanning_progress = 0;
        }
        if (block_height % 100 == 0 && progress_end - progress_begin > 0.0) {
            ShowProgress(strprintf("%s " + _("Rescanning…").translated, GetDisplayName()), std::max(1, std::min(99, (int)(m_scanning_progress * 100))));
        }

        bool next_interval = reserver.now() >= current_time + INTERVAL_TIME;
        if (next_interval) {
            current_time = reserver.now();
            WalletLogPrintf("Still rescanning. At block %d. Progress=%f\n", block_height, progress_current);
        }

        // Read block data
        CBlock block;
        chain().findBlock(block_hash, FoundBlock().data(block));

        // Find next block separately from reading data above, because reading
        // is slow and there might be a reorg while it is read.
        bool block_still_active = false;
        bool next_block = false;
        uint256 next_block_hash;
        chain().findBlock(block_hash, FoundBlock().inActiveChain(block_still_active).nextBlock(FoundBlock().inActiveChain(next_block).hash(next_block_hash)));

        if (!block.IsNull()) {
            LOCK(cs_wallet);
            if (!block_still_active) {
                // Abort scan if current block is no longer active, to prevent
                // marking transactions as coming from the wrong block.
                result.last_failed_block = block_hash;
                result.status = ScanResult::FAILURE;
                break;
            }
            for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
                SyncTransaction(block.vtx[posInBlock], TxStateConfirmed{block_hash, block_height, static_cast<int>(posInBlock)}, batch, fUpdate, /*rescanning_old_block=*/true);
            }
            // scan succeeded, record block as most recent successfully scanned
            result.last_scanned_block = block_hash;
            result.last_scanned_height = block_height;

            if (save_progress && next_interval) {
                CBlockLocator loc = m_chain->getActiveChainLocator(block_hash);

                if (!loc.IsNull()) {
                    WalletLogPrintf("Saving scan progress %d.\n", block_height);
                    WalletBatch batch(GetDatabase());
                    batch.WriteBestBlock(loc);
                }
            }
        } else {
            // could not scan block, keep scanning but record this block as the most recent failure
            result.last_failed_block = block_hash;
            result.status = ScanResult::FAILURE;
        }
        if (max_height && block_height >= *max_height) {
            break;
        }
        {
            if (!next_block) {
                // break successfully when rescan has reached the tip, or
                // previous block is no longer on the chain due to a reorg
                break;
            }

            // increment block and verification progress
            block_hash = next_block_hash;
            ++block_height;
            progress_current = chain().guessVerificationProgress(block_hash);

            // handle updated tip hash
            const uint256 prev_tip_hash = tip_hash;
            tip_hash = WITH_LOCK(cs_wallet, return GetLastBlockHash());
            if (!max_height && prev_tip_hash != tip_hash) {
                // in case the tip has changed, update progress max
                progress_end = chain().guessVerificationProgress(tip_hash);
            }
        }
    }
    ShowProgress(strprintf("%s " + _("Rescanning…").translated, GetDisplayName()), 100); // hide progress dialog in GUI
    if (block_height && fAbortRescan) {
        WalletLogPrintf("Rescan aborted at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else if (block_height && chain().shutdownRequested()) {
        WalletLogPrintf("Rescan interrupted by shutdown request at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else {
        WalletLogPrintf("Rescan completed in %15dms\n", Ticks<std::chrono::milliseconds>(reserver.now() - start_time));
    }
    return result;
}

void CWallet::ReacceptWalletTransactions()
{
    // If transactions aren't being broadcasted, don't let them into local mempool either
    if (!fBroadcastTransactions)
        return;
    std::map<int64_t, CWalletTx*> mapSorted;

    // Sort pending wallet transactions based on their initial wallet insertion order
    for (std::pair<const uint256, CWalletTx>& item : mapWallet) {
        const uint256& wtxid = item.first;
        CWalletTx& wtx = item.second;
        assert(wtx.GetHash() == wtxid);

        int nDepth = GetTxDepthInMainChain(wtx);

        if (!wtx.IsCoinBase() && (nDepth == 0 && !IsTxLockedByInstantSend(wtx) && !wtx.isAbandoned())) {
            mapSorted.insert(std::make_pair(wtx.nOrderPos, &wtx));
        }
    }

    // Try to add wallet transactions to memory pool
    for (const std::pair<const int64_t, CWalletTx*>& item : mapSorted) {
        CWalletTx& wtx = *(item.second);
        bilingual_str unused_err_string;
        SubmitTxMemoryPoolAndRelay(wtx, unused_err_string, false);
    }
}

bool CWallet::CanTxBeResent(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);

    return
        // Can't relay if wallet is not broadcasting
        GetBroadcastTransactions() &&
        // Don't relay abandoned transactions
        !wtx.isAbandoned() &&
        // Don't try to submit coinbase transactions. These would fail anyway but would
        // cause log spam.
        !wtx.IsCoinBase() &&
        // Don't try to submit conflicted or confirmed transactions.
        GetTxDepthInMainChain(wtx) == 0 &&
        // Don't try to submit transactions locked via InstantSend.
        !IsTxLockedByInstantSend(wtx);
}

bool CWallet::SubmitTxMemoryPoolAndRelay(CWalletTx& wtx, bilingual_str& err_string, bool relay) const
{
    AssertLockHeld(cs_wallet);

    if (!CanTxBeResent(wtx)) return false;

    // Submit transaction to mempool for relay
    WalletLogPrintf("Submitting wtx %s to mempool for relay\n", wtx.GetHash().ToString());
    // We must set TxStateInMempool here. Even though it will also be set later by the
    // entered-mempool callback, if we did not there would be a race where a
    // user could call sendmoney in a loop and hit spurious out of funds errors
    // because we think that this newly generated transaction's change is
    // unavailable as we're not yet aware that it is in the mempool.
    //
    // If broadcast fails for any reason, trying to set wtx.m_state here would be incorrect.
    // If transaction was previously in the mempool, it should be updated when
    // TransactionRemovedFromMempool fires.
    bool ret = chain().broadcastTransaction(wtx.tx, m_default_max_tx_fee, relay, err_string);
    if (ret) wtx.m_state = TxStateInMempool{};
    return ret;
}

std::set<uint256> CWallet::GetTxConflicts(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);

    const uint256 myHash{wtx.GetHash()};
    std::set<uint256> result{GetConflicts(myHash)};
    result.erase(myHash);
    return result;
}

// Rebroadcast transactions from the wallet. We do this on a random timer
// to slightly obfuscate which transactions come from our wallet.
//
// Ideally, we'd only resend transactions that we think should have been
// mined in the most recent block. Any transaction that wasn't in the top
// blockweight of transactions in the mempool shouldn't have been mined,
// and so is probably just sitting in the mempool waiting to be confirmed.
// Rebroadcasting does nothing to speed up confirmation and only damages
// privacy.
void CWallet::ResendWalletTransactions()
{
    // During reindex, importing and IBD, old wallet transactions become
    // unconfirmed. Don't resend them as that would spam other nodes.
    if (!chain().isReadyToBroadcast()) return;

    // Do this infrequently and randomly to avoid giving away
    // that these are our transactions.
    if (GetTime() < nNextResend || !fBroadcastTransactions) return;
    bool fFirst = (nNextResend == 0);
    // resend 1-3 hours from now, ~2 hours on average.
    nNextResend = GetTime() + (1 * 60 * 60) + GetRand(2 * 60 * 60);
    if (fFirst) return;

    int submitted_tx_count = 0;

    { // cs_wallet scope
        LOCK(cs_wallet);

        // Relay transactions
        for (std::pair<const uint256, CWalletTx>& item : mapWallet) {
            CWalletTx& wtx = item.second;
            // Attempt to rebroadcast all txes more than 5 minutes older than
            // the last block. SubmitTxMemoryPoolAndRelay() will not rebroadcast
            // any confirmed or conflicting txs.
            if (wtx.nTimeReceived > m_best_block_time - 5 * 60) continue;
            bilingual_str unused_err_string;
            if (SubmitTxMemoryPoolAndRelay(wtx, unused_err_string, true)) ++submitted_tx_count;
        }
    } // cs_wallet

    if (submitted_tx_count > 0) {
        WalletLogPrintf("%s: resubmit %u unconfirmed transactions\n", __func__, submitted_tx_count);
    }
}

/** @} */ // end of mapWallet

void MaybeResendWalletTxs(WalletContext& context)
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets(context)) {
        pwallet->ResendWalletTransactions();
    }
}


/** @defgroup Actions
 *
 * @{
 */

std::unordered_set<const CWalletTx*, WalletTxHasher> CWallet::GetSpendableTXs() const
{
    AssertLockHeld(cs_wallet);

    std::unordered_set<const CWalletTx*, WalletTxHasher> ret;
    for (auto it = setWalletUTXO.begin(); it != setWalletUTXO.end(); ) {
        const auto& outpoint = *it;
        const auto jt = mapWallet.find(outpoint.hash);
        if (jt != mapWallet.end()) {
            ret.emplace(&jt->second);
        }

        // setWalletUTXO is sorted by COutPoint, which means that all UTXOs for the same TX are neighbors
        // skip entries until we encounter a new TX
        while (it != setWalletUTXO.end() && it->hash == outpoint.hash) {
            ++it;
        }
    }
    return ret;
}

bool CWallet::SignTransaction(CMutableTransaction& tx) const
{
    AssertLockHeld(cs_wallet);

    // Build coins map
    std::map<COutPoint, Coin> coins;
    for (auto& input : tx.vin) {
        const auto mi = mapWallet.find(input.prevout.hash);
        if(mi == mapWallet.end() || input.prevout.n >= mi->second.tx->vout.size()) {
            return false;
        }
        const CWalletTx& wtx = mi->second;
        int prev_height = wtx.state<TxStateConfirmed>() ? wtx.state<TxStateConfirmed>()->confirmed_block_height : 0;
        coins[input.prevout] = Coin(wtx.tx->vout[input.prevout.n], prev_height, wtx.IsCoinBase());
    }
    std::map<int, bilingual_str> input_errors;
    return SignTransaction(tx, coins, SIGHASH_ALL, input_errors);
}

bool CWallet::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const
{
    // Try to sign with all ScriptPubKeyMans
    for (ScriptPubKeyMan* spk_man : GetAllScriptPubKeyMans()) {
        // spk_man->SignTransaction will return true if the transaction is complete,
        // so we can exit early and return true if that happens
        if (spk_man->SignTransaction(tx, coins, sighash, input_errors)) {
            return true;
        }
    }

    // At this point, one input was not fully signed otherwise we would have exited already
    return false;
}

TransactionError CWallet::FillPSBT(PartiallySignedTransaction& psbtx, bool& complete, int sighash_type, bool sign, bool bip32derivs, size_t * n_signed, bool finalize) const
{
    if (n_signed) {
        *n_signed = 0;
    }
    const PrecomputedTransactionData txdata = PrecomputePSBTData(psbtx);
    LOCK(cs_wallet);
    // Get all of the previous transactions
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // If we have no utxo, grab it from the wallet.
        if (!input.non_witness_utxo) {
            const uint256& txhash = txin.prevout.hash;
            const auto it = mapWallet.find(txhash);
            if (it != mapWallet.end()) {
                const CWalletTx& wtx = it->second;
                // We only need the non_witness_utxo, which is a superset of the witness_utxo.
                //   The signing code will switch to the smaller witness_utxo if this is ok.
                input.non_witness_utxo = wtx.tx;
            }
        }
    }

    // Fill in information from ScriptPubKeyMans
    for (ScriptPubKeyMan* spk_man : GetAllScriptPubKeyMans()) {
        int n_signed_this_spkm = 0;
        TransactionError res = spk_man->FillPSBT(psbtx, txdata, sighash_type, sign, bip32derivs, &n_signed_this_spkm, finalize);
        if (res != TransactionError::OK) {
            return res;
        }

        if (n_signed) {
            (*n_signed) += n_signed_this_spkm;
        }
    }

    // Complete if every input is now signed
    complete = true;
    for (const auto& input : psbtx.inputs) {
        complete &= PSBTInputSigned(input);
    }

    return TransactionError::OK;
}

SigningResult CWallet::SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const
{
    SignatureData sigdata;
    CScript script_pub_key = GetScriptForDestination(pkhash);
    for (const auto& spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script_pub_key, sigdata)) {
            LOCK(cs_wallet);  // DescriptorScriptPubKeyMan calls IsLocked which can lock cs_wallet in a deadlocking order
            return spk_man_pair.second->SignMessage(message, pkhash, str_sig);
        }
    }
    return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
}

bool CWallet::SignSpecialTxPayload(const uint256& hash, const CKeyID& keyid, std::vector<unsigned char>& vchSig) const
{
    SignatureData sigdata;
    CScript script_pub_key = GetScriptForDestination(PKHash(keyid));
    for (const auto& spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script_pub_key, sigdata)) {
            LOCK(cs_wallet);  // DescriptorScriptPubKeyMan calls IsLocked which can lock cs_wallet in a deadlocking order
            return spk_man_pair.second->SignSpecialTxPayload(hash, keyid, vchSig);
        }
    }
    return false;
}

bool CWallet::SignGovernanceVote(const CKeyID& keyID, CGovernanceVote& vote) const
{
    // Special implementation for testnet (Harden Spork6 that has not been deployed to other networks)
    if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        std::vector<unsigned char> signature;
        if (!SignSpecialTxPayload(vote.GetSignatureHash(), keyID, signature)) {
            LogPrintf("SignGovernanceVote -- SignHash() failed\n");
            return false;
        }
        vote.SetSignature(signature);
        return true;
    } // end of testnet implementation

    std::string strMessage{vote.GetSignatureString()};
    std::string signature;
    SigningResult err = SignMessage(strMessage, PKHash{keyID}, signature);
    if (err != SigningResult::OK) {
        LogPrintf("SignGovernanceVote failed due to: %s\n", SigningResultString(err));
        return false;
    }
    const auto opt_decoded = DecodeBase64(signature);
    CHECK_NONFATAL(opt_decoded.has_value()); // DecodeBase64 should not fail

    vote.SetSignature(std::vector<unsigned char>(opt_decoded->data(), opt_decoded->data() + opt_decoded->size()));
    return true;
}

void CWallet::CommitTransaction(CTransactionRef tx, mapValue_t mapValue, std::vector<std::pair<std::string, std::string>> orderForm)
{
    LOCK(cs_wallet);
    WalletLogPrintf("CommitTransaction:\n%s", tx->ToString()); /* Continued */

    // Add tx to wallet, because if it has change it's also ours,
    // otherwise just for transaction history.
    CWalletTx* wtx = AddToWallet(tx, TxStateInactive{}, [&](CWalletTx& wtx, bool new_tx) {
        CHECK_NONFATAL(wtx.mapValue.empty());
        CHECK_NONFATAL(wtx.vOrderForm.empty());
        wtx.mapValue = std::move(mapValue);
        wtx.vOrderForm = std::move(orderForm);
        wtx.fTimeReceivedIsTxTime = true;
        wtx.fFromMe = true;
        return true;
    });

    // wtx can only be null if the db write failed.
    if (!wtx) {
        throw std::runtime_error(std::string(__func__) + ": Wallet db error, transaction commit failed");
    }

    // Notify that old coins are spent
    std::set<uint256> updated_hahes;
    for (const CTxIn& txin : tx->vin){
        // notify only once
        if(updated_hahes.find(txin.prevout.hash) != updated_hahes.end()) continue;

        CWalletTx &coin = mapWallet.at(txin.prevout.hash);
        coin.MarkDirty();
        NotifyTransactionChanged(txin.prevout.hash, CT_UPDATED);
        updated_hahes.insert(txin.prevout.hash);
    }

    if (!fBroadcastTransactions) {
        // Don't submit tx to the mempool
        return;
    }

    bilingual_str err_string;
    if (!SubmitTxMemoryPoolAndRelay(*wtx, err_string, true)) {
        WalletLogPrintf("CommitTransaction(): Transaction cannot be broadcast immediately, %s\n", err_string.original);
        // TODO: if we expect the failure to be long term or permanent, instead delete wtx from the wallet and return failure.
    }
}

DBErrors CWallet::LoadWallet()
{
    LOCK(cs_wallet);

    DBErrors nLoadWalletRet = WalletBatch(GetDatabase()).LoadWallet(this);
    if (nLoadWalletRet == DBErrors::NEED_REWRITE)
    {
        if (GetDatabase().Rewrite("\x04pool"))
        {
            for (const auto& spk_man_pair : m_spk_managers) {
                spk_man_pair.second->RewriteDB();
            }
            nKeysLeftSinceAutoBackup = 0;
        }
    }

    if (m_spk_managers.empty()) {
        assert(m_external_spk_managers == nullptr);
        assert(m_internal_spk_managers == nullptr);
    }

    if (HaveChain()) {
        const std::optional<int> tip_height = chain().getHeight();
        if (tip_height) {
            SetLastBlockProcessed(*tip_height, chain().getBlockHash(*tip_height));
            for (auto& pair : mapWallet) {
                for(unsigned int i = 0; i < pair.second.tx->vout.size(); ++i) {
                    COutPoint outpoint(pair.first, i);
                    if (IsMine(pair.second.tx->vout[i]) && !IsSpent(outpoint)) {
                        setWalletUTXO.insert(outpoint);
                    }
                }
            }
        }
    }

    if (nLoadWalletRet != DBErrors::LOAD_OK)
        return nLoadWalletRet;

    /* If the CoinJoin salt is not set, try to set a new random hash as the salt */
    if (GetCoinJoinSalt().IsNull() && !SetCoinJoinSalt(GetRandHash())) {
        return DBErrors::LOAD_FAIL;
    }

    return DBErrors::LOAD_OK;
}

// Goes through all wallet transactions and checks if they are masternode collaterals, in which case these are locked
// This avoids accidental spending of collaterals. They can still be unlocked manually if a spend is really intended.
void CWallet::AutoLockMasternodeCollaterals()
{
    std::set<COutPoint> candidates;
    LOCK(cs_wallet);
    for (const auto& [txid, wtx] : mapWallet) {
        auto tx_utxos{AddWalletUTXOs(wtx.tx, /*ret_dups=*/true)};
        candidates.insert(tx_utxos.begin(), tx_utxos.end());
    }
    WalletBatch batch(GetDatabase());
    LockProTxCoins(candidates, &batch);
}

DBErrors CWallet::ZapSelectTx(std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut)
{
    AssertLockHeld(cs_wallet);

    WalletLogPrintf("ZapSelectTx started for %d transactions...\n", vHashIn.size());

    DBErrors nZapSelectTxRet = WalletBatch(GetDatabase()).ZapSelectTx(vHashIn, vHashOut);
    for (const uint256& hash : vHashOut) {
        const auto& it = mapWallet.find(hash);
        wtxOrdered.erase(it->second.m_it_wtxOrdered);
        for (const auto& txin : it->second.tx->vin)
            mapTxSpends.erase(txin.prevout);
        mapWallet.erase(it);
        NotifyTransactionChanged(hash, CT_DELETED);
    }

    if (nZapSelectTxRet == DBErrors::NEED_REWRITE)
    {
        if (GetDatabase().Rewrite("\x04pool"))
        {
            for (const auto& spk_man_pair : m_spk_managers) {
                spk_man_pair.second->RewriteDB();
            }
        }
    }

    if (nZapSelectTxRet != DBErrors::LOAD_OK)
        return nZapSelectTxRet;

    MarkDirty();

    WalletLogPrintf("ZapSelectTx completed for %d transactions.\n", vHashOut.size());
    return DBErrors::LOAD_OK;
}

bool CWallet::SetAddressBookWithDB(WalletBatch& batch, const CTxDestination& address, const std::string& strName, const std::string& strPurpose)
{
    bool fUpdated = false;
    bool is_mine;
    {
        LOCK(cs_wallet);
        std::map<CTxDestination, CAddressBookData>::iterator mi = m_address_book.find(address);
        fUpdated = (mi != m_address_book.end() && !mi->second.IsChange());
        m_address_book[address].SetLabel(strName);
        if (!strPurpose.empty()) /* update purpose only if requested */
            m_address_book[address].purpose = strPurpose;
        is_mine = IsMine(address) != ISMINE_NO;
    }
    NotifyAddressBookChanged(address, strName, is_mine,
                             strPurpose, (fUpdated ? CT_UPDATED : CT_NEW));
    if (!strPurpose.empty() && !batch.WritePurpose(EncodeDestination(address), strPurpose))
        return false;
    return batch.WriteName(EncodeDestination(address), strName);
}

bool CWallet::SetAddressBook(const CTxDestination& address, const std::string& strName, const std::string& strPurpose)
{
    WalletBatch batch(GetDatabase());
    return SetAddressBookWithDB(batch, address, strName, strPurpose);
}

bool CWallet::DelAddressBook(const CTxDestination& address)
{
    bool is_mine;
    WalletBatch batch(GetDatabase());
    {
        LOCK(cs_wallet);
        // If we want to delete receiving addresses, we need to take care that DestData "used" (and possibly newer DestData) gets preserved (and the "deleted" address transformed into a change entry instead of actually being deleted)
        // NOTE: This isn't a problem for sending addresses because they never have any DestData yet!
        // When adding new DestData, it should be considered here whether to retain or delete it (or move it?).
        if (IsMine(address)) {
            WalletLogPrintf("%s called with IsMine address, NOT SUPPORTED. Please report this bug! %s\n", __func__, PACKAGE_BUGREPORT);
            return false;
        }
        // Delete destdata tuples associated with address
        std::string strAddress = EncodeDestination(address);
        for (const std::pair<const std::string, std::string> &item : m_address_book[address].destdata)
        {
            batch.EraseDestData(strAddress, item.first);
        }
        m_address_book.erase(address);
        is_mine = IsMine(address) != ISMINE_NO;
    }

    NotifyAddressBookChanged(address, "", is_mine, "", CT_DELETED);

    batch.ErasePurpose(EncodeDestination(address));
    return batch.EraseName(EncodeDestination(address));
}

size_t CWallet::KeypoolCountExternalKeys() const
{
    AssertLockHeld(cs_wallet);

    auto legacy_spk_man = GetLegacyScriptPubKeyMan();
    if (legacy_spk_man) {
        return legacy_spk_man->KeypoolCountExternalKeys();
    }

    unsigned int count = 0;
    if (m_external_spk_managers) {
        count += m_external_spk_managers->GetKeyPoolSize();
    }

    return count;
}

unsigned int CWallet::GetKeyPoolSize() const
{
    AssertLockHeld(cs_wallet);

    unsigned int count = 0;
    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        count += spk_man->GetKeyPoolSize();
    }
    return count;
}

bool CWallet::TopUpKeyPool(unsigned int kpSize)
{
    LOCK(cs_wallet);
    bool res = true;
    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        res &= spk_man->TopUp(kpSize);
    }
    return res;
}

bool CWallet::GetNewDestination(const std::string label, CTxDestination& dest, bilingual_str& error)
{
    error.clear();
    bool result = false;

    LOCK(cs_wallet);
    auto spk_man = GetScriptPubKeyMan(false /* internal */);
    if (spk_man) {
        spk_man->TopUp();
        result = spk_man->GetNewDestination(dest, error);
    } else {
        error = strprintf(_("Error: No addresses available."));
    }
    if (result) {
        SetAddressBook(dest, label, "receive");
    }

    return result;
}

bool CWallet::GetNewChangeDestination(CTxDestination& dest,  bilingual_str& error)
{
    LOCK(cs_wallet);
    error.clear();

    ReserveDestination reservedest(this);
    if (!reservedest.GetReservedDestination(dest, true)) {
        error = _("Error: Keypool ran out, please call keypoolrefill first");
        return false;
    }

    reservedest.KeepDestination();
    return true;
}

std::optional<int64_t> CWallet::GetOldestKeyPoolTime() const
{
    LOCK(cs_wallet);
    if (m_spk_managers.empty()) {
        return std::nullopt;
    }

    std::optional<int64_t> oldest_key{std::numeric_limits<int64_t>::max()};
    for (const auto& spk_man_pair : m_spk_managers) {
        oldest_key = std::min(oldest_key, spk_man_pair.second->GetOldestKeyPoolTime());
    }
    return oldest_key;
}

void CWallet::MarkDestinationsDirty(const std::set<CTxDestination>& destinations) {
    for (auto& entry : mapWallet) {
        CWalletTx& wtx = entry.second;
        if (wtx.m_is_cache_empty) continue;
        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
            CTxDestination dst;
            if (ExtractDestination(wtx.tx->vout[i].scriptPubKey, dst) && destinations.count(dst)) {
                wtx.MarkDirty();
                break;
            }
        }
    }
}

void CWallet::ForEachAddrBookEntry(const ListAddrBookFunc& func) const
{
    for (const std::pair<const CTxDestination, CAddressBookData>& item : m_address_book) {
        const auto& entry = item.second;
        func(item.first, entry.GetLabel(), entry.purpose, entry.IsChange());
    }
}

std::vector<CTxDestination> CWallet::ListAddrBookAddresses(const std::optional<AddrBookFilter>& _filter) const
{
    AssertLockHeld(cs_wallet);
    std::vector<CTxDestination> result;
    AddrBookFilter filter = _filter ? *_filter : AddrBookFilter();
    ForEachAddrBookEntry([&result, &filter](const CTxDestination& dest, const std::string& label, const std::string& purpose, bool is_change) {
        // Filter by change
        if (filter.ignore_change && is_change) return;
        // Filter by label
        if (filter.m_op_label && *filter.m_op_label != label) return;
        // All good
        result.emplace_back(dest);
    });
    return result;
}

std::set<std::string> CWallet::ListAddrBookLabels(const std::string& purpose) const
{
    AssertLockHeld(cs_wallet);
    std::set<std::string> label_set;
    ForEachAddrBookEntry([&](const CTxDestination& _dest, const std::string& _label,
                             const std::string& _purpose, bool _is_change) {
        if (_is_change) return;
        if (purpose.empty() || _purpose == purpose) {
            label_set.insert(_label);
        }
    });
    return label_set;
}

bool ReserveDestination::GetReservedDestination(CTxDestination& dest, bool fInternalIn)
{
    m_spk_man = pwallet->GetScriptPubKeyMan(fInternalIn);
    if (!m_spk_man) {
        return false;
    }

    if (nIndex == -1)
    {
        m_spk_man->TopUp();

        CKeyPool keypool;
        int64_t index;
        if (!m_spk_man->GetReservedDestination(fInternalIn, address, index, keypool)) {
            return false;
        }
        nIndex = index;
        fInternal = keypool.fInternal;
    }
    dest = address;
    return true;
}

void ReserveDestination::KeepDestination()
{
    if (nIndex != -1) {
        m_spk_man->KeepDestination(nIndex);
    }
    nIndex = -1;
    address = CNoDestination();
}

void ReserveDestination::ReturnDestination()
{
    if (nIndex != -1) {
        m_spk_man->ReturnDestination(nIndex, fInternal, address);
    }
    nIndex = -1;
    address = CNoDestination();
}

bool CWallet::LockCoin(const COutPoint& output, WalletBatch* batch)
{
    AssertLockHeld(cs_wallet);
    setLockedCoins.insert(output);
    RecalculateMixedCredit(output.hash);
    if (batch) {
        return batch->WriteLockedUTXO(output);
    }
    return true;
}

bool CWallet::UnlockCoin(const COutPoint& output, WalletBatch* batch)
{
    AssertLockHeld(cs_wallet);
    bool was_locked = setLockedCoins.erase(output);
    RecalculateMixedCredit(output.hash);
    if (batch && was_locked) {
        return batch->EraseLockedUTXO(output);
    }
    return true;
}

bool CWallet::UnlockAllCoins()
{
    AssertLockHeld(cs_wallet);
    bool success = true;
    WalletBatch batch(GetDatabase());
    for (auto it = setLockedCoins.begin(); it != setLockedCoins.end(); ++it) {
        success &= batch.EraseLockedUTXO(*it);
    }
    setLockedCoins.clear();
    return success;
}

bool CWallet::IsLockedCoin(const COutPoint& output) const
{
    AssertLockHeld(cs_wallet);
    return setLockedCoins.count(output) > 0;
}

std::vector<COutPoint> CWallet::ListLockedCoins() const
{
    AssertLockHeld(cs_wallet);
    return std::vector<COutPoint>(setLockedCoins.begin(), setLockedCoins.end());
}

std::vector<COutPoint> CWallet::ListProTxCoins() const { return ListProTxCoins(setWalletUTXO); }

std::vector<COutPoint> CWallet::ListProTxCoins(const std::set<COutPoint>& utxos) const
{
    AssertLockHeld(cs_wallet);

    if (!m_chain) return std::vector<COutPoint>();

    std::vector<std::pair<const CTransactionRef&, /*index=*/uint32_t>> candidates;
    for (const auto& output : utxos) {
        if (auto it = mapWallet.find(output.hash); it != mapWallet.end()) {
            const auto& [hash, wtx] = *it;
            candidates.emplace_back(wtx.tx, output.n);
        }
    }
    return m_chain->listMNCollaterials(candidates);
}

void CWallet::LockProTxCoins(const std::set<COutPoint>& utxos, WalletBatch* batch)
{
    AssertLockHeld(cs_wallet);
    for (const auto& utxo : ListProTxCoins(utxos)) {
        LockCoin(utxo, batch);
    }
}

/** @} */ // end of Actions

void CWallet::GetKeyBirthTimes(std::map<CKeyID, int64_t>& mapKeyBirth) const {
    AssertLockHeld(cs_wallet);
    mapKeyBirth.clear();

    // map in which we'll infer heights of other keys
    std::map<CKeyID, const TxStateConfirmed*> mapKeyFirstBlock;
    TxStateConfirmed max_confirm{uint256{}, /*height=*/-1, /*index=*/-1};
    max_confirm.confirmed_block_height = GetLastBlockHeight() > 144 ? GetLastBlockHeight() - 144 : 0; // the tip can be reorganized; use a 144-block safety margin
    CHECK_NONFATAL(chain().findAncestorByHeight(GetLastBlockHash(), max_confirm.confirmed_block_height, FoundBlock().hash(max_confirm.confirmed_block_hash)));

    {
        LegacyScriptPubKeyMan* spk_man = GetLegacyScriptPubKeyMan();
        assert(spk_man != nullptr);
        LOCK(spk_man->cs_KeyStore);

        // get birth times for keys with metadata
        for (const auto& entry : spk_man->mapKeyMetadata) {
            if (entry.second.nCreateTime) {
                mapKeyBirth[entry.first] = entry.second.nCreateTime;
            }
        }

        // Prepare to infer birth heights for keys without metadata
        for (const CKeyID &keyid : spk_man->GetKeys()) {
            if (mapKeyBirth.count(keyid) == 0)
                mapKeyFirstBlock[keyid] = &max_confirm;
        }

        // if there are no such keys, we're done
        if (mapKeyFirstBlock.empty())
            return;

        // find first block that affects those keys, if there are any left
        for (const auto& entry : mapWallet) {
            // iterate over all wallet transactions...
            const CWalletTx &wtx = entry.second;
            if (auto* conf = wtx.state<TxStateConfirmed>()) {
                // ... which are already in a block
                for (const CTxOut &txout : wtx.tx->vout) {
                    // iterate over all their outputs
                    for (const auto &keyid : GetAffectedKeys(txout.scriptPubKey, *spk_man)) {
                        // ... and all their affected keys
                        auto rit = mapKeyFirstBlock.find(keyid);
                        if (rit != mapKeyFirstBlock.end() && conf->confirmed_block_height < rit->second->confirmed_block_height) {
                            rit->second = conf;
                        }
                    }
                }
            }
        }
    }

    // Extract block timestamps for those keys
    for (const auto& entry : mapKeyFirstBlock) {
        int64_t block_time;
        CHECK_NONFATAL(chain().findBlock(entry.second->confirmed_block_hash, FoundBlock().time(block_time)));
        mapKeyBirth[entry.first] = block_time - TIMESTAMP_WINDOW; // block times can be 2h off
    }
}

/**
 * Compute smart timestamp for a transaction being added to the wallet.
 *
 * Logic:
 * - If sending a transaction, assign its timestamp to the current time.
 * - If receiving a transaction outside a block, assign its timestamp to the
 *   current time.
 * - If receiving a transaction during a rescanning process, assign all its
 *   (not already known) transactions' timestamps to the block time.
 * - If receiving a block with a future timestamp, assign all its (not already
 *   known) transactions' timestamps to the current time.
 * - If receiving a block with a past timestamp, before the most recent known
 *   transaction (that we care about), assign all its (not already known)
 *   transactions' timestamps to the same timestamp as that most-recent-known
 *   transaction.
 * - If receiving a block with a past timestamp, but after the most recent known
 *   transaction, assign all its (not already known) transactions' timestamps to
 *   the block time.
 *
 * For more information see CWalletTx::nTimeSmart,
 * https://bitcointalk.org/?topic=54527, or
 * https://github.com/bitcoin/bitcoin/pull/1393.
 */
unsigned int CWallet::ComputeTimeSmart(const CWalletTx& wtx, bool rescanning_old_block) const
{
    std::optional<uint256> block_hash;
    if (auto* conf = wtx.state<TxStateConfirmed>()) {
        block_hash = conf->confirmed_block_hash;
    } else if (auto* conf = wtx.state<TxStateConflicted>()) {
        block_hash = conf->conflicting_block_hash;
    }

    unsigned int nTimeSmart = wtx.nTimeReceived;
    if (block_hash) {
        int64_t blocktime;
        int64_t block_max_time;
        if (chain().findBlock(*block_hash, FoundBlock().time(blocktime).maxTime(block_max_time))) {
            if (rescanning_old_block) {
                nTimeSmart = block_max_time;
            } else {
                int64_t latestNow = wtx.nTimeReceived;
                int64_t latestEntry = 0;

                // Tolerate times up to the last timestamp in the wallet not more than 5 minutes into the future
                int64_t latestTolerated = latestNow + 300;
                const TxItems& txOrdered = wtxOrdered;
                for (auto it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
                    CWalletTx* const pwtx = it->second;
                    if (pwtx == &wtx) {
                        continue;
                    }
                    int64_t nSmartTime;
                    nSmartTime = pwtx->nTimeSmart;
                    if (!nSmartTime) {
                        nSmartTime = pwtx->nTimeReceived;
                    }
                    if (nSmartTime <= latestTolerated) {
                        latestEntry = nSmartTime;
                        if (nSmartTime > latestNow) {
                            latestNow = nSmartTime;
                        }
                        break;
                    }
                }

                nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
            }
        } else {
            WalletLogPrintf("%s: found %s in block %s not in index\n", __func__, wtx.GetHash().ToString(), block_hash->ToString());
        }
    }
    return nTimeSmart;
}

bool CWallet::SetAddressUsed(WalletBatch& batch, const CTxDestination& dest, bool used)
{
    const std::string key{"used"};
    if (std::get_if<CNoDestination>(&dest))
        return false;

    if (!used) {
        if (auto* data = util::FindKey(m_address_book, dest)) data->destdata.erase(key);
        return batch.EraseDestData(EncodeDestination(dest), key);
    }

    const std::string value{"1"};
    m_address_book[dest].destdata.insert(std::make_pair(key, value));
    return batch.WriteDestData(EncodeDestination(dest), key, value);
}

void CWallet::LoadDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    m_address_book[dest].destdata.insert(std::make_pair(key, value));
}

bool CWallet::IsAddressUsed(const CTxDestination& dest) const
{
    const std::string key{"used"};
    std::map<CTxDestination, CAddressBookData>::const_iterator i = m_address_book.find(dest);
    if(i != m_address_book.end())
    {
        CAddressBookData::StringMap::const_iterator j = i->second.destdata.find(key);
        if(j != i->second.destdata.end())
        {
            return true;
        }
    }
    return false;
}

std::vector<std::string> CWallet::GetAddressReceiveRequests() const
{
    const std::string prefix{"rr"};
    std::vector<std::string> values;
    for (const auto& address : m_address_book) {
        for (const auto& data : address.second.destdata) {
            if (!data.first.compare(0, prefix.size(), prefix)) {
                values.emplace_back(data.second);
            }
        }
    }
    return values;
}

bool CWallet::SetAddressReceiveRequest(WalletBatch& batch, const CTxDestination& dest, const std::string& id, const std::string& value)
{
    const std::string key{"rr" + id}; // "rr" prefix = "receive request" in destdata
    CAddressBookData& data = m_address_book.at(dest);
    if (value.empty()) {
        if (!batch.EraseDestData(EncodeDestination(dest), key)) return false;
        data.destdata.erase(key);
    } else {
        if (!batch.WriteDestData(EncodeDestination(dest), key, value)) return false;
        data.destdata[key] = value;
    }
    return true;
}

std::unique_ptr<WalletDatabase> MakeWalletDatabase(const std::string& name, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error_string)
{
    // Do some checking on wallet path. It should be either a:
    //
    // 1. Path where a directory can be created.
    // 2. Path to an existing directory.
    // 3. Path to a symlink to a directory.
    // 4. For backwards compatibility, the name of a data file in -walletdir.
    const fs::path wallet_path = fsbridge::AbsPathJoin(GetWalletDir(), fs::PathFromString(name));
    fs::file_type path_type = fs::symlink_status(wallet_path).type();
    if (!(path_type == fs::file_type::not_found || path_type == fs::file_type::directory ||
          (path_type == fs::file_type::symlink && fs::is_directory(wallet_path)) ||
          (path_type == fs::file_type::regular && fs::PathFromString(name).filename() == fs::PathFromString(name)))) {
        error_string = Untranslated(strprintf(
              "Invalid -wallet path '%s'. -wallet path should point to a directory where wallet.dat and "
              "database/log.?????????? files can be stored, a location where such a directory could be created, "
              "or (for backwards compatibility) the name of an existing data file in -walletdir (%s)",
              name, fs::quoted(fs::PathToString(GetWalletDir()))));
        status = DatabaseStatus::FAILED_BAD_PATH;
        return nullptr;
    }

    return MakeDatabase(wallet_path, options, status, error_string);
}

std::shared_ptr<CWallet> CWallet::Create(WalletContext& context, const std::string& name, std::unique_ptr<WalletDatabase> database, uint64_t wallet_creation_flags, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    interfaces::Chain* chain = context.chain;
    interfaces::CoinJoin::Loader* coinjoin_loader = context.coinjoin_loader;
    ArgsManager& args = *Assert(context.args);
    const std::string& walletFile = database->Filename();

    const auto start{SteadyClock::now()};
    // TODO: Can't use std::make_shared because we need a custom deleter but
    // should be possible to use std::allocate_shared.
    const std::shared_ptr<CWallet> walletInstance(new CWallet(chain, coinjoin_loader, name, args, std::move(database)), ReleaseWallet);
    // TODO: refactor this condition: validation of error looks like workaround
    if (!walletInstance->AutoBackupWallet(fs::PathFromString(walletFile), error, warnings) && !error.original.empty()) {
        return nullptr;
    }
    DBErrors nLoadWalletRet = walletInstance->LoadWallet();
    if (nLoadWalletRet != DBErrors::LOAD_OK)
    {
        if (nLoadWalletRet == DBErrors::CORRUPT) {
            error = strprintf(_("Error loading %s: Wallet corrupted"), walletFile);
            return nullptr;
        }
        else if (nLoadWalletRet == DBErrors::NONCRITICAL_ERROR)
        {
            warnings.push_back(strprintf(_("Error reading %s! All keys read correctly, but transaction data"
                                           " or address book entries might be missing or incorrect."),
                walletFile));
        }
        else if (nLoadWalletRet == DBErrors::TOO_NEW) {
            error = strprintf(_("Error loading %s: Wallet requires newer version of %s"), walletFile, PACKAGE_NAME);
            return nullptr;
        }
        else if (nLoadWalletRet == DBErrors::NEED_REWRITE)
        {
            error = strprintf(_("Wallet needed to be rewritten: restart %s to complete"), PACKAGE_NAME);
            return nullptr;
        }
        else {
            error = strprintf(_("Error loading %s"), walletFile);
            return nullptr;
        }
    }

    // This wallet is in its first run if there are no ScriptPubKeyMans and it isn't blank or no privkeys
    const bool fFirstRun = walletInstance->m_spk_managers.empty() &&
                     !walletInstance->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) &&
                     !walletInstance->IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET);
    if (fFirstRun)
    {
        walletInstance->SetMinVersion(FEATURE_LATEST);

        walletInstance->InitWalletFlags(wallet_creation_flags);

        // Only create LegacyScriptPubKeyMan when not descriptor wallet
        if (!walletInstance->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
            walletInstance->SetupLegacyScriptPubKeyMan();
        }

        if (!(wallet_creation_flags & (WALLET_FLAG_DISABLE_PRIVATE_KEYS | WALLET_FLAG_BLANK_WALLET))) {
            // Create new HD chain
            if (args.GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET) && !walletInstance->IsHDEnabled()) {
                std::string strSeed = args.GetArg("-hdseed", "not hex");

                // ensure this wallet.dat can only be opened by clients supporting HD
                walletInstance->WalletLogPrintf("Upgrading wallet to HD\n");
                walletInstance->SetMinVersion(FEATURE_HD);

                if (args.IsArgSet("-hdseed") && IsHex(strSeed)) {
                    CHDChain newHdChain;
                    std::vector<unsigned char> vchSeed = ParseHex(strSeed);
                    if (!newHdChain.SetSeed(SecureVector(vchSeed.begin(), vchSeed.end()), true)) {
                        error = strprintf(_("%s failed"), "SetSeed");
                        return nullptr;
                    }
                    LOCK(walletInstance->cs_wallet);
                    if (auto spk_man = walletInstance->GetLegacyScriptPubKeyMan()) {
                        if (!spk_man->AddHDChainSingle(newHdChain)) {
                            error = strprintf(_("%s failed"), "AddHDChainSingle");
                            return nullptr;
                        }
                    }
                    // add default account
                    newHdChain.AddAccount();
                } else {
                    if (args.IsArgSet("-hdseed") && !IsHex(strSeed)) {
                        error = strprintf(_("%s -- Incorrect seed, it should be a hex string"), __func__);
                        return nullptr;
                    }
                    SecureString secureMnemonic = args.GetArg("-mnemonic", "").c_str();
                    SecureString secureMnemonicPassphrase = args.GetArg("-mnemonicpassphrase", "").c_str();
                    LOCK(walletInstance->cs_wallet);
                    if (auto spk_man = walletInstance->GetLegacyScriptPubKeyMan()) {
                        spk_man->GenerateNewHDChain(secureMnemonic, secureMnemonicPassphrase);
                    }
                }

                // clean up
                args.ForceRemoveArg("hdseed");
                args.ForceRemoveArg("mnemonic");
                args.ForceRemoveArg("mnemonicpassphrase");
            } // Otherwise, do not create a new HD chain

            LOCK(walletInstance->cs_wallet);
            if (walletInstance->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
                SecureString mnemonic = args.GetArg("-mnemonic", "").c_str();
                SecureString mnemonic_passphrase = args.GetArg("-mnemonicpassphrase", "").c_str();
                args.ForceRemoveArg("mnemonic");
                args.ForceRemoveArg("mnemonicpassphrase");
                walletInstance->SetupDescriptorScriptPubKeyMans(mnemonic, mnemonic_passphrase);
                // SetupDescriptorScriptPubKeyMans already calls SetupGeneration for us so we don't need to call SetupGeneration separately
            } else { // Top up the keypool
                // Legacy wallets need SetupGeneration here.
                if (auto spk_man = walletInstance->GetLegacyScriptPubKeyMan()) {
                    if (spk_man->CanGenerateKeys() && !spk_man->TopUp()) {
                        error = _("Unable to generate initial keys");
                        return nullptr;
                    }
                }
            }
        }

        if (chain) {
            walletInstance->chainStateFlushed(chain->getTipLocator());
        }

        // Try to create wallet backup right after new wallet was created
        bilingual_str strBackupError;
        if(!walletInstance->AutoBackupWallet("", strBackupError, warnings)) {
            if (!strBackupError.original.empty()) {
                error = strBackupError;
                return nullptr;
            }
        }
    } else if (wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS) {
        // Make it impossible to disable private keys after creation
        error = strprintf(_("Error loading %s: Private keys can only be disabled during creation"), walletFile);
        return nullptr;
    } else if (walletInstance->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        for (auto spk_man : walletInstance->GetActiveScriptPubKeyMans()) {
            if (spk_man->HavePrivateKeys()) {
                warnings.push_back(strprintf(_("Warning: Private keys detected in wallet {%s} with disabled private keys"), walletFile));
            }
        }
    }
    else if (args.IsArgSet("-usehd")) {
        bool useHD = args.GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET);
        if (walletInstance->IsHDEnabled() && !useHD) {
            error = strprintf(_("Error loading %s: You can't disable HD on an already existing HD wallet"), walletInstance->GetName());
            return nullptr;
        }
        if (!walletInstance->IsHDEnabled() && useHD) {
            error = strprintf(_("Error loading %s: You can't enable HD on an already existing non-HD wallet"), walletInstance->GetName());
            return nullptr;
        }
    }

    // Warn user every time a non-encrypted HD wallet is started
    if (walletInstance->IsHDEnabled() && !walletInstance->IsLocked()) {
        SetMiscWarning(_("Make sure to encrypt your wallet and delete all non-encrypted backups after you have verified that the wallet works!"));
    }

    if (args.IsArgSet("-mintxfee")) {
        std::optional<CAmount> min_tx_fee = ParseMoney(args.GetArg("-mintxfee", ""));
        if (!min_tx_fee) {
            error = AmountErrMsg("mintxfee", args.GetArg("-mintxfee", ""));
            return nullptr;
        } else if (min_tx_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-mintxfee") + Untranslated(" ") +
                              _("This is the minimum transaction fee you pay on every transaction."));
        }

        walletInstance->m_min_fee = CFeeRate{min_tx_fee.value()};
    }

    if (args.IsArgSet("-maxapsfee")) {
        const std::string max_aps_fee{args.GetArg("-maxapsfee", "")};
        if (max_aps_fee == "-1") {
            walletInstance->m_max_aps_fee = -1;
        } else if (std::optional<CAmount> max_fee = ParseMoney(max_aps_fee)) {
            if (max_fee.value() > HIGH_APS_FEE) {
                warnings.push_back(AmountHighWarn("-maxapsfee") + Untranslated(" ") +
                                  _("This is the maximum transaction fee you pay (in addition to the normal fee) to prioritize partial spend avoidance over regular coin selection."));
            }
            walletInstance->m_max_aps_fee = max_fee.value();
        } else {
            error = AmountErrMsg("maxapsfee", max_aps_fee);
            return nullptr;
        }
    }

    if (args.IsArgSet("-fallbackfee")) {
        std::optional<CAmount> fallback_fee = ParseMoney(args.GetArg("-fallbackfee", ""));
        if (!fallback_fee) {
            error = strprintf(_("Invalid amount for %s=<amount>: '%s'"), "-fallbackfee", args.GetArg("-fallbackfee", ""));
            return nullptr;
        } else if (fallback_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-fallbackfee") + Untranslated(" ") +
                              _("This is the transaction fee you may pay when fee estimates are not available."));
        }
        walletInstance->m_fallback_fee = CFeeRate{fallback_fee.value()};
    }
    // Disable fallback fee in case value was set to 0, enable if non-null value
    walletInstance->m_allow_fallback_fee = walletInstance->m_fallback_fee.GetFeePerK() != 0;

    if (args.IsArgSet("-discardfee")) {
        std::optional<CAmount> discard_fee = ParseMoney(args.GetArg("-discardfee", ""));
        if (!discard_fee) {
            error = strprintf(_("Invalid amount for %s=<amount>: '%s'"), "-discardfee", args.GetArg("-discardfee", ""));
            return nullptr;
        } else if (discard_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-discardfee") + Untranslated(" ") +
                              _("This is the transaction fee you may discard if change is smaller than dust at this level"));
        }
        walletInstance->m_discard_rate = CFeeRate{discard_fee.value()};
    }

    if (args.IsArgSet("-paytxfee")) {
        std::optional<CAmount> pay_tx_fee = ParseMoney(args.GetArg("-paytxfee", ""));
        if (!pay_tx_fee) {
            error = AmountErrMsg("paytxfee", args.GetArg("-paytxfee", ""));
            return nullptr;
        } else if (pay_tx_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-paytxfee") + Untranslated(" ") +
                              _("This is the transaction fee you will pay if you send a transaction."));
        }
        walletInstance->m_pay_tx_fee = CFeeRate{pay_tx_fee.value(), 1000};
        if (chain && walletInstance->m_pay_tx_fee < chain->relayMinFee()) {
            error = strprintf(_("Invalid amount for %s=<amount>: '%s' (must be at least %s)"),
                "-paytxfee", args.GetArg("-paytxfee", ""), chain->relayMinFee().ToString());
            return nullptr;
        }
    }

    if (args.IsArgSet("-maxtxfee")) {
        std::optional<CAmount> max_fee = ParseMoney(args.GetArg("-maxtxfee", ""));
        if (!max_fee) {
            error = AmountErrMsg("maxtxfee", args.GetArg("-maxtxfee", ""));
            return nullptr;
        } else if (max_fee.value() > HIGH_MAX_TX_FEE) {
            warnings.push_back(strprintf(_("%s is set very high! Fees this large could be paid on a single transaction."), "-maxtxfee"));
        }
        if (chain && CFeeRate{max_fee.value(), 1000} < chain->relayMinFee()) {
            error = strprintf(_("Invalid amount for %s=<amount>: '%s' (must be at least the minrelay fee of %s to prevent stuck transactions)"),
                "-maxtxfee", args.GetArg("-maxtxfee", ""), chain->relayMinFee().ToString());
            return nullptr;
        }

        walletInstance->m_default_max_tx_fee = max_fee.value();
    }

    if (args.IsArgSet("-consolidatefeerate")) {
        if (std::optional<CAmount> consolidate_feerate = ParseMoney(args.GetArg("-consolidatefeerate", ""))) {
            walletInstance->m_consolidate_feerate = CFeeRate(*consolidate_feerate);
        } else {
            error = AmountErrMsg("consolidatefeerate", args.GetArg("-consolidatefeerate", ""));
            return nullptr;
        }
    }

    if (chain && chain->relayMinFee().GetFeePerK() > HIGH_TX_FEE_PER_KB)
        warnings.push_back(AmountHighWarn("-minrelaytxfee") + Untranslated(" ") +
                    _("The wallet will avoid paying less than the minimum relay fee."));

    walletInstance->m_confirm_target = args.GetIntArg("-txconfirmtarget", DEFAULT_TX_CONFIRM_TARGET);
    walletInstance->m_spend_zero_conf_change = args.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);

    walletInstance->WalletLogPrintf("Wallet completed loading in %15dms\n", Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));

    // Try to top up keypool. No-op if the wallet is locked.
    walletInstance->TopUpKeyPool();

    if (chain && !AttachChain(walletInstance, *chain, error, warnings)) {
        return nullptr;
    }

    if (coinjoin_loader) {
        coinjoin_loader->AddWallet(walletInstance);
    }

    {
        LOCK(walletInstance->cs_wallet);
        walletInstance->SetBroadcastTransactions(args.GetBoolArg("-walletbroadcast", DEFAULT_WALLETBROADCAST));
        walletInstance->WalletLogPrintf("setExternalKeyPool.size() = %u\n",   walletInstance->KeypoolCountExternalKeys());
        walletInstance->WalletLogPrintf("GetKeyPoolSize() = %u\n",   walletInstance->GetKeyPoolSize());
        walletInstance->WalletLogPrintf("mapWallet.size() = %u\n",            walletInstance->mapWallet.size());
        walletInstance->WalletLogPrintf("m_address_book.size() = %u\n",  walletInstance->m_address_book.size());
        for (auto spk_man : walletInstance->GetAllScriptPubKeyMans()) {
            walletInstance->WalletLogPrintf("nTimeFirstKey = %u\n", spk_man->GetTimeFirstKey());
        }
    }

    return walletInstance;
}

bool CWallet::AttachChain(const std::shared_ptr<CWallet>& walletInstance, interfaces::Chain& chain, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    LOCK(walletInstance->cs_wallet);
    // allow setting the chain if it hasn't been set already but prevent changing it
    assert(!walletInstance->m_chain || walletInstance->m_chain == &chain);
    walletInstance->m_chain = &chain;

    // Register wallet with validationinterface. It's done before rescan to avoid
    // missing block connections between end of rescan and validation subscribing.
    // Because of wallet lock being hold, block connection notifications are going to
    // be pending on the validation-side until lock release. It's likely to have
    // block processing duplicata (if rescan block range overlaps with notification one)
    // but we guarantee at least than wallet state is correct after notifications delivery.
    // However, chainStateFlushed notifications are ignored until the rescan is finished
    // so that in case of a shutdown event, the rescan will be repeated at the next start.
    // This is temporary until rescan and notifications delivery are unified under same
    // interface.
    walletInstance->m_attaching_chain = true; //ignores chainStateFlushed notifications
    walletInstance->m_chain_notifications_handler = walletInstance->chain().handleNotifications(walletInstance);

    int rescan_height = 0;
    if (!gArgs.GetBoolArg("-rescan", false))
    {
        WalletBatch batch(walletInstance->GetDatabase());
        CBlockLocator locator;
        if (batch.ReadBestBlock(locator)) {
            if (const std::optional<int> fork_height = chain.findLocatorFork(locator)) {
                rescan_height = *fork_height;
            }
        }
    }

    const std::optional<int> tip_height = chain.getHeight();
    if (tip_height) {
        walletInstance->m_last_block_processed = chain.getBlockHash(*tip_height);
        walletInstance->m_last_block_processed_height = *tip_height;
    } else {
        walletInstance->m_last_block_processed.SetNull();
        walletInstance->m_last_block_processed_height = -1;
    }

    if (tip_height && *tip_height != rescan_height)
    {
        // Technically we could execute the code below in any case, but performing the
        // `while` loop below can make startup very slow, so only check blocks on disk
        // if necessary.
        if (chain.havePruned() || chain.hasAssumedValidChain()) {
            int block_height = *tip_height;
            while (block_height > 0 && chain.haveBlockOnDisk(block_height - 1) && rescan_height != block_height) {
                --block_height;
            }

            if (rescan_height != block_height) {
                // We can't rescan beyond blocks we don't have data for, stop and throw an error.
                // This might happen if a user uses an old wallet within a pruned node
                // or if they ran -disablewallet for a longer time, then decided to re-enable
                // Exit early and print an error.
                // It also may happen if an assumed-valid chain is in use and therefore not
                // all block data is available.
                // If a block is pruned after this check, we will load the wallet,
                // but fail the rescan with a generic error.

                error = chain.havePruned() ?
                     _("Prune: last wallet synchronisation goes beyond pruned data. You need to -reindex (download the whole blockchain again in case of pruned node)") :
                     strprintf(_(
                        "Error loading wallet. Wallet requires blocks to be downloaded, "
                        "and software does not currently support loading wallets while "
                        "blocks are being downloaded out of order when using assumeutxo "
                        "snapshots. Wallet should be able to load successfully after "
                        "node sync reaches height %s"), block_height);
                return false;
            }
        }

        chain.initMessage(_("Rescanning…").translated);
        walletInstance->WalletLogPrintf("Rescanning last %i blocks (from block %i)...\n", *tip_height - rescan_height, rescan_height);

        // No need to read and scan block if block was created before
        // our wallet birthday (as adjusted for block time variability)
        // unless a full rescan was requested
        if (gArgs.GetIntArg("-rescan", 0) != 2) {
            std::optional<int64_t> time_first_key;
            for (auto spk_man : walletInstance->GetAllScriptPubKeyMans()) {
                int64_t time = spk_man->GetTimeFirstKey();
                if (!time_first_key || time < *time_first_key) time_first_key = time;
            }
            if (time_first_key) {
                chain.findFirstBlockWithTimeAndHeight(*time_first_key - TIMESTAMP_WINDOW, rescan_height, FoundBlock().height(rescan_height));
            }
        }

        {
            WalletRescanReserver reserver(*walletInstance);
            if (!reserver.reserve() || (ScanResult::SUCCESS != walletInstance->ScanForWalletTransactions(chain.getBlockHash(rescan_height), rescan_height, /*max_height=*/{}, reserver, /*fUpdate=*/true, /*save_progress=*/true).status)) {
                error = _("Failed to rescan the wallet during initialization");
                return false;
            }
        }
        walletInstance->m_attaching_chain = false;
        walletInstance->chainStateFlushed(chain.getTipLocator());
        walletInstance->GetDatabase().IncrementUpdateCounter();
    }
    walletInstance->m_attaching_chain = false;

    return true;
}

bool CWallet::UpgradeWallet(int version, bilingual_str& error)
{
    int prev_version = GetVersion();
    int nMaxVersion = version;
    auto nMinVersion = DEFAULT_USE_HD_WALLET ? FEATURE_LATEST : FEATURE_COMPRPUBKEY;
    if (nMaxVersion == 0) {
        WalletLogPrintf("Performing wallet upgrade to %i\n", nMinVersion);
        nMaxVersion = FEATURE_LATEST;
        SetMinVersion(nMinVersion); // permanently upgrade the wallet immediately
    } else {
        WalletLogPrintf("Allowing wallet upgrade up to %i\n", nMaxVersion);
    }

    if (nMaxVersion < GetVersion()) {
        error = strprintf(_("Cannot downgrade wallet from version %i to version %i. Wallet version unchanged."), prev_version, version);
        return false;
    }

    // TODO: consider discourage users to skip passphrase for HD wallets for v21
    if (/* DISABLES CODE */ (false) && nMaxVersion >= FEATURE_HD && !IsHDEnabled()) {
        error = Untranslated("You should use upgradetohd RPC to upgrade non-HD wallet to HD");
        error = strprintf(_("Cannot upgrade a non HD wallet from version %i to version %i which is non-HD wallet. Use upgradetohd RPC"), prev_version, version);
        return false;
    }

    SetMinVersion(GetClosestWalletFeature(version));

    return true;
}

bool CWallet::UpgradeToHD(const SecureString& secureMnemonic, const SecureString& secureMnemonicPassphrase, const SecureString& secureWalletPassphrase, bilingual_str& error)
{
    LOCK(cs_wallet);

    // Do not do anything to HD wallets
    if (IsHDEnabled()) {
        error = Untranslated("Cannot upgrade a wallet to HD if it is already upgraded to HD.");
        return false;
    }

    if (IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        error = Untranslated("Private keys are disabled for this wallet");
        return false;
    }

    WalletLogPrintf("Upgrading wallet to HD\n");
    SetMinVersion(FEATURE_HD);

    if (IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        if (IsCrypted()) {
            if (secureWalletPassphrase.empty()) {
                error = Untranslated("Error: Wallet encrypted but supplied empty wallet passphrase");
                return false;
            }

            // Unlock the wallet
            if (!Unlock(secureWalletPassphrase)) {
                error = Untranslated("Error: The wallet passphrase entered was incorrect");
                return false;
            }
        }
        SetupDescriptorScriptPubKeyMans(secureMnemonic, secureMnemonicPassphrase);

        if (IsCrypted()) {
            // Relock the wallet
            Lock();
        }
    } else {
        if (!GenerateNewHDChain(secureMnemonic, secureMnemonicPassphrase, secureWalletPassphrase)) {
            error = Untranslated("Failed to generate HD wallet");
            return false;
        }
    }
    return true;
}

const CAddressBookData* CWallet::FindAddressBookEntry(const CTxDestination& dest, bool allow_change) const
{
    const auto& address_book_it = m_address_book.find(dest);
    if (address_book_it == m_address_book.end()) return nullptr;
    if ((!allow_change) && address_book_it->second.IsChange()) {
        return nullptr;
    }
    return &address_book_it->second;
}

void CWallet::postInitProcess()
{
    LOCK(cs_wallet);

    // Add wallet transactions that aren't already in a block to mempool
    // Do this here as mempool requires genesis block to be loaded
    ReacceptWalletTransactions();

    // Update wallet transactions with current mempool transactions.
    chain().requestMempoolTransactions(*this);
}

void CWallet::InitAutoBackup()
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET))
        return;

    nWalletBackups = gArgs.GetIntArg("-createwalletbackups", 10);
    nWalletBackups = std::max(0, std::min(10, nWalletBackups));
}

bool CWallet::BackupWallet(const std::string& strDest) const
{
    return GetDatabase().Backup(strDest);
}

// This should be called carefully:
// either supply the actual wallet_path to make a raw copy of wallet.dat or "" to backup current instance via BackupWallet()
#ifdef USE_BDB
bool CWallet::AutoBackupWallet(const fs::path& wallet_path, bilingual_str& error_string, std::vector<bilingual_str>& warnings)
{
    std::string strWalletName = GetName();
    if (strWalletName.empty()) {
        strWalletName = "wallet.dat";
    }
    // This condition is required to be sure that wallet.dat won't be re-opened by IsBDBFile
    // Re-opening of database file brokes an exclusive inter-process lock for SQLite
    if (m_database && !m_database->SupportsAutoBackup()) {
        WalletLogPrintf("Automatic wallet backups are not supported!\n");
        return false;
    }
    if (!wallet_path.empty() && !IsBDBFile(BDBDataFile(wallet_path))) {
        WalletLogPrintf("Automatic wallet backups are currently only supported with Berkeley DB!\n");
        return false;
    }

    if (IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET)) {
        WalletLogPrintf("Wallet is blank, won't create new backup for it!\n");
        return false;
    }

    if (nWalletBackups <= 0) {
        WalletLogPrintf("Automatic wallet backups are disabled!\n");
        return false;
    }

    fs::path backupsDir = gArgs.GetBackupsDirPath();
    backupsDir.make_preferred();

    if (!fs::exists(backupsDir))
    {
        // Always create backup folder to not confuse the operating system's file browser
        WalletLogPrintf("Creating backup folder %s\n", fs::PathToString(backupsDir));
        if(!fs::create_directories(backupsDir)) {
            // something is wrong, we shouldn't continue until it's resolved
            error_string = strprintf(_("Wasn't able to create wallet backup folder %s!"), fs::PathToString(backupsDir));
            WalletLogPrintf("%s\n", error_string.translated);
            nWalletBackups = -1;
            return false;
        }
    } else if (!fs::is_directory(backupsDir)) {
        // something is wrong, we shouldn't continue until it's resolved
        error_string = strprintf(_("%s is not a valid backup folder!"), fs::PathToString(backupsDir));
        WalletLogPrintf("%s\n", error_string.translated);
        nWalletBackups = -1;
        return false;
    }

    // Create backup of the ...
    std::string dateTimeStr = [&]() {
        const std::chrono::sys_seconds secs{GetTime<std::chrono::seconds>()};
        const auto days{std::chrono::floor<std::chrono::days>(secs)};
        const std::chrono::year_month_day ymd{days};
        const std::chrono::hh_mm_ss hms{secs - days};
        return strprintf(".%04i-%02u-%02u-%02i-%02i", signed{ymd.year()}, unsigned{ymd.month()}, unsigned{ymd.day()}, hms.hours().count(), hms.minutes().count());
    }();

    if (wallet_path.empty()) {
        // ... opened wallet
        LOCK(cs_wallet);
        fs::path backupFile = backupsDir / fs::u8path(strWalletName + dateTimeStr);
        backupFile.make_preferred();
        if (!BackupWallet(fs::PathToString(backupFile))) {
            warnings.push_back(strprintf(_("Failed to create backup %s!"), fs::PathToString(backupFile)));
            WalletLogPrintf("%s\n", Join(warnings, Untranslated("\n")).original);
            nWalletBackups = -1;
            return false;
        }

        // Update nKeysLeftSinceAutoBackup using current external keypool size
        nKeysLeftSinceAutoBackup = KeypoolCountExternalKeys();
        WalletLogPrintf("nKeysLeftSinceAutoBackup: %d\n", nKeysLeftSinceAutoBackup);
        if (IsLocked(true)) {
            warnings.push_back(_("Wallet is locked, can't replenish keypool! Automatic backups and mixing are disabled, please unlock your wallet to replenish keypool."));
            WalletLogPrintf("%s\n", Join(warnings, Untranslated("\n")).original);
            nWalletBackups = -2;
            return false;
        }
    } else {
        // ... strWalletName file
        fs::path strSourceFile = BDBDataFile(wallet_path);
        std::shared_ptr<BerkeleyEnvironment> env = GetBerkeleyEnv(strSourceFile.parent_path(), /*use_shared_memory=*/true);
        fs::path sourceFile = env->Directory() / strSourceFile;
        fs::path backupFile = backupsDir / fs::u8path(strWalletName + dateTimeStr);
        sourceFile.make_preferred();
        backupFile.make_preferred();
        if (fs::exists(backupFile))
        {
            warnings.push_back(_("Failed to create backup, file already exists! This could happen if you restarted wallet in less than 60 seconds. You can continue if you are ok with this."));
            WalletLogPrintf("%s\n", Join(warnings, Untranslated("\n")).original);
            return false;
        }
        if(fs::exists(sourceFile)) {
            try {
                fs::copy_file(sourceFile, backupFile, fs::copy_options::none);
                WalletLogPrintf("Creating backup of %s -> %s\n", fs::PathToString(sourceFile), fs::PathToString(backupFile));
            } catch(fs::filesystem_error &error) {
                warnings.push_back(strprintf(_("Failed to create backup, error: %s"), fsbridge::get_filesystem_error_message(error)));
                WalletLogPrintf("%s\n", Join(warnings, Untranslated("\n")).original);
                nWalletBackups = -1;
                return false;
            }
        }
    }

    // Keep only the last 10 backups, including the new one of course
    std::multimap<fs::file_time_type, fs::path> folder_set;
    // Build map of backup files for current(!) wallet sorted by last write time
    fs::path currentFile;
    for (const auto& entry : fs::directory_iterator(backupsDir)) {
        // Only check regular files
        if (entry.is_regular_file()) {
            currentFile = entry.path().filename();
            // Only add the backups for the current wallet, e.g. wallet.dat.*
            if (fs::PathToString(entry.path().stem()) == strWalletName) {
                folder_set.insert(decltype(folder_set)::value_type(fs::last_write_time(entry.path()), entry));
            }
        }
    }

    // Loop backward through backup files and keep the N newest ones (1 <= N <= 10)
    int counter{0};
    for (const auto& [entry_time, entry] : folder_set | std::views::reverse) {
        counter++;
        if (counter > nWalletBackups) {
            // More than nWalletBackups backups: delete oldest one(s)
            try {
                fs::remove(entry);
                WalletLogPrintf("Old backup deleted: %s\n", fs::PathToString(entry));
            } catch(fs::filesystem_error &error) {
                warnings.push_back(strprintf(_("Failed to delete backup, error: %s"), fsbridge::get_filesystem_error_message(error)));
                WalletLogPrintf("%s\n", Join(warnings, Untranslated("\n")).original);
                return false;
            }
        }
    }

    return true;
}
#elif defined(USE_SQLITE)
bool CWallet::AutoBackupWallet(const fs::path& wallet_path, bilingual_str& error_string, std::vector<bilingual_str>& warnings)
{
    WalletLogPrintf("Automatic wallet backups are currently only supported with Berkeley DB!\n");
    return false;
}
#endif // USE_BDB

void CWallet::notifyTransactionLock(const CTransactionRef &tx, const std::shared_ptr<const instantsend::InstantSendLock>& islock)
{
    LOCK(cs_wallet);
    // Only notify UI if this transaction is in this wallet
    uint256 txHash = tx->GetHash();
    const auto mi = mapWallet.find(txHash);
    if (mi != mapWallet.end()){
        NotifyTransactionChanged(txHash, CT_UPDATED);
        NotifyISLockReceived();
#if HAVE_SYSTEM
        // notify an external script
        std::string strCmd = m_args.GetArg("-instantsendnotify", "");
        if (!strCmd.empty()) {
            ReplaceAll(strCmd, "%s", txHash.GetHex());
#ifndef WIN32
            // Substituting the wallet name isn't currently supported on windows
            // because windows shell escaping has not been implemented yet:
            // https://github.com/bitcoin/bitcoin/pull/13339#issuecomment-537384875
            // A few ways it could be implemented in the future are described in:
            // https://github.com/bitcoin/bitcoin/pull/13339#issuecomment-461288094
            ReplaceAll(strCmd, "%w", ShellEscape(GetName()));
#endif
            std::thread t(runCommand, strCmd);
            t.detach(); // thread runs free
        }
#endif
    }
}

void CWallet::notifyChainLock(const CBlockIndex* pindexChainLock, const std::shared_ptr<const chainlock::ChainLockSig>& clsig)
{
    NotifyChainLockReceived(pindexChainLock->nHeight);
}

bool CWallet::LoadGovernanceObject(const Governance::Object& obj)
{
    AssertLockHeld(cs_wallet);
    return m_gobjects.emplace(obj.GetHash(), obj).second;
}

bool CWallet::WriteGovernanceObject(const Governance::Object& obj)
{
    AssertLockHeld(cs_wallet);
    WalletBatch batch(GetDatabase());
    return batch.WriteGovernanceObject(obj) && LoadGovernanceObject(obj);
}

std::vector<const Governance::Object*> CWallet::GetGovernanceObjects()
{
    AssertLockHeld(cs_wallet);
    std::vector<const Governance::Object*> vecObjects;
    vecObjects.reserve(m_gobjects.size());
    for (auto& obj : m_gobjects) {
        vecObjects.push_back(&obj.second);
    }
    return vecObjects;
}

CKeyPool::CKeyPool()
{
    nTime = GetTime();
    fInternal = false;
}

CKeyPool::CKeyPool(const CPubKey& vchPubKeyIn, bool fInternalIn)
{
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
    fInternal = fInternalIn;
}

int CWallet::GetTxDepthInMainChain(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);
    if (auto* conf = wtx.state<TxStateConfirmed>()) {
        return GetLastBlockHeight() - conf->confirmed_block_height + 1;
    } else if (auto* conf = wtx.state<TxStateConflicted>()) {
        return -1 * (GetLastBlockHeight() - conf->conflicting_block_height + 1);
    } else {
        return 0;
    }
}

bool CWallet::IsTxLockedByInstantSend(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);
    if (wtx.fIsChainlocked) {
        wtx.fIsInstantSendLocked = false;
    } else if (!wtx.fIsInstantSendLocked) {
        wtx.fIsInstantSendLocked = chain().isInstantSendLockedTx(wtx.GetHash());
    }
    return wtx.fIsInstantSendLocked;
}

bool CWallet::IsTxChainLocked(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);
    if (!wtx.fIsChainlocked) {
        bool active; int height;
        if (auto* conf = wtx.state<TxStateConfirmed>()) {
            if (chain().findBlock(conf->confirmed_block_hash, FoundBlock().inActiveChain(active).height(height)) && active) {
                wtx.fIsChainlocked = chain().hasChainLock(height, conf->confirmed_block_hash);
            }
        }
    }
    return wtx.fIsChainlocked;
}

int CWallet::GetTxBlocksToMaturity(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);

    if (!wtx.IsCoinBase()) {
        return 0;
    }
    int chain_depth = GetTxDepthInMainChain(wtx);
    assert(chain_depth >= 0); // coinbase tx should not be conflicted
    return std::max(0, (COINBASE_MATURITY+1) - chain_depth);
}

bool CWallet::IsTxImmatureCoinBase(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);

    // note GetBlocksToMaturity is 0 for non-coinbase tx
    return GetTxBlocksToMaturity(wtx) > 0;
}

bool CWallet::IsCrypted() const
{
    return HasEncryptionKeys();
}

// This function should be used in a different combinations to determine
// if FillableSigningProvider is fully locked so that no operations requiring access
// to private keys are possible:
//      IsLocked(true)
// or if FillableSigningProvider's private keys are available for mixing only:
//      !IsLocked(true) && IsLocked()
// or if they are available for everything:
//      !IsLocked()
bool CWallet::IsLocked(bool fForMixing) const
{
    if (!IsCrypted())
        return false;

    if(!fForMixing && fOnlyMixingAllowed) return true;

    LOCK(cs_wallet);
    return vMasterKey.empty();
}

bool CWallet::Lock(bool fAllowMixing)
{
    if (!IsCrypted())
        return false;

    if(!fAllowMixing) {
        LOCK(cs_wallet);
        if (!vMasterKey.empty()) {
            memory_cleanse(vMasterKey.data(), vMasterKey.size() * sizeof(decltype(vMasterKey)::value_type));
            vMasterKey.clear();
        }
    }

    fOnlyMixingAllowed = fAllowMixing;
    NotifyStatusChanged(this);
    return true;
}

bool CWallet::Unlock(const SecureString& strWalletPassphrase, bool fForMixingOnly, bool accept_no_keys)
{
    if (!IsLocked()) // was already fully unlocked, not only for mixing
        return true;

    CCrypter crypter;
    CKeyingMaterial _vMasterKey;

    {
        LOCK(cs_wallet);
        for (const MasterKeyMap::value_type& pMasterKey : mapMasterKeys)
        {
            if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, _vMasterKey))
                continue; // try another master key
            if (Unlock(_vMasterKey, fForMixingOnly, accept_no_keys)) {
                // Now that we've unlocked, upgrade the key metadata
                UpgradeKeyMetadata();
                // Now that we've unlocked, upgrade the descriptor cache
                UpgradeDescriptorCache();
                if(nWalletBackups == -2) {
                    TopUpKeyPool();
                    WalletLogPrintf("Keypool replenished, re-initializing automatic backups.\n");
                    nWalletBackups = m_args.GetIntArg("-createwalletbackups", 10);
                }
                return true;
            }
        }
    }
    return false;
}

bool CWallet::Unlock(const CKeyingMaterial& vMasterKeyIn, bool fForMixingOnly, bool accept_no_keys)
{
    {
        LOCK(cs_wallet);
        for (const auto& spk_man_pair : m_spk_managers) {
            if (!spk_man_pair.second->CheckDecryptionKey(vMasterKeyIn, accept_no_keys)) {
                return false;
            }
        }
        vMasterKey = vMasterKeyIn;
        fOnlyMixingAllowed = fForMixingOnly;
    }
    NotifyStatusChanged(this);
    return true;
}

std::set<ScriptPubKeyMan*> CWallet::GetActiveScriptPubKeyMans() const
{
    std::set<ScriptPubKeyMan*> spk_mans;
    for (bool internal : {false, true}) {
        auto spk_man = GetScriptPubKeyMan(internal);
        if (spk_man) {
            spk_mans.insert(spk_man);
        }
    }
    return spk_mans;
}

std::set<ScriptPubKeyMan*> CWallet::GetAllScriptPubKeyMans() const
{
    std::set<ScriptPubKeyMan*> spk_mans;
    for (const auto& spk_man_pair : m_spk_managers) {
        spk_mans.insert(spk_man_pair.second.get());
    }
    return spk_mans;
}

ScriptPubKeyMan* CWallet::GetScriptPubKeyMan(bool internal) const
{
    const auto spk_manager = internal ? m_internal_spk_managers : m_external_spk_managers;
    if (spk_manager == nullptr) {
        return nullptr;
    }
    return spk_manager;
}

std::set<ScriptPubKeyMan*> CWallet::GetScriptPubKeyMans(const CScript& script, SignatureData& sigdata) const
{
    std::set<ScriptPubKeyMan*> spk_mans;
    for (const auto& spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script, sigdata)) {
            spk_mans.insert(spk_man_pair.second.get());
        }
    }
    return spk_mans;
}

ScriptPubKeyMan* CWallet::GetScriptPubKeyMan(const CScript& script) const
{
    SignatureData sigdata;
    for (const auto& spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script, sigdata)) {
            return spk_man_pair.second.get();
        }
    }
    return nullptr;
}

ScriptPubKeyMan* CWallet::GetScriptPubKeyMan(const uint256& id) const
{
    if (m_spk_managers.count(id) > 0) {
        return m_spk_managers.at(id).get();
    }
    return nullptr;
}

std::unique_ptr<SigningProvider> CWallet::GetSolvingProvider(const CScript& script) const
{
    SignatureData sigdata;
    return GetSolvingProvider(script, sigdata);
}

std::unique_ptr<SigningProvider> CWallet::GetSolvingProvider(const CScript& script, SignatureData& sigdata) const
{
    for (const auto& spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script, sigdata)) {
            return spk_man_pair.second->GetSolvingProvider(script);
        }
    }
    return nullptr;
}

LegacyScriptPubKeyMan* CWallet::GetLegacyScriptPubKeyMan() const
{
    if (IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        return nullptr;
    }
    // Legacy wallets only have one ScriptPubKeyMan which is a LegacyScriptPubKeyMan.
    // Everything in m_internal_spk_managers and m_external_spk_managers point to the same legacyScriptPubKeyMan.
    if (m_internal_spk_managers == nullptr) return nullptr;
    return dynamic_cast<LegacyScriptPubKeyMan*>(m_internal_spk_managers);
}

LegacyScriptPubKeyMan* CWallet::GetOrCreateLegacyScriptPubKeyMan()
{
    SetupLegacyScriptPubKeyMan();
    return GetLegacyScriptPubKeyMan();
}

void CWallet::SetupLegacyScriptPubKeyMan()
{
    if (m_internal_spk_managers || m_external_spk_managers || !m_spk_managers.empty() || IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        return;
    }

    auto spk_manager = std::make_unique<LegacyScriptPubKeyMan>(*this);
    m_internal_spk_managers = spk_manager.get();
    m_external_spk_managers = spk_manager.get();
    m_spk_managers[spk_manager->GetID()] = std::move(spk_manager);
}

bool CWallet::WithEncryptionKey(std::function<bool (const CKeyingMaterial&)> cb) const
{
    LOCK(cs_wallet);
    return cb(vMasterKey);
}

bool CWallet::HasEncryptionKeys() const
{
    return !mapMasterKeys.empty();
}

void CWallet::ConnectScriptPubKeyManNotifiers()
{
    for (const auto& spk_man : GetActiveScriptPubKeyMans()) {
        spk_man->NotifyWatchonlyChanged.connect(NotifyWatchonlyChanged);
        spk_man->NotifyCanGetAddressesChanged.connect(NotifyCanGetAddressesChanged);
    }
}

bool CWallet::GenerateNewHDChain(const SecureString& secureMnemonic, const SecureString& secureMnemonicPassphrase, const SecureString& secureWalletPassphrase)
{
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        throw std::runtime_error(strprintf("%s: spk_man is not available", __func__));
    }

    if (IsCrypted()) {
        if (secureWalletPassphrase.empty()) {
            throw std::runtime_error(strprintf("%s: encrypted but supplied empty wallet passphrase", __func__));
        }

        bool is_locked = IsLocked();

        CCrypter crypter;
        CKeyingMaterial vMasterKey;

        // We are intentionally re-locking the wallet so we can validate vMasterKey
        // by verifying if it can unlock the wallet
        Lock();

        LOCK(cs_wallet);
        for (const auto& [_, master_key] : mapMasterKeys) {
            CKeyingMaterial _vMasterKey;
            if (!crypter.SetKeyFromPassphrase(secureWalletPassphrase, master_key.vchSalt, master_key.nDeriveIterations, master_key.nDerivationMethod)) {
                return false;
            }
            // Try another key if it cannot be decrypted or the key is incapable of encrypting
            if (!crypter.Decrypt(master_key.vchCryptedKey, _vMasterKey) || _vMasterKey.size() != WALLET_CRYPTO_KEY_SIZE) {
                continue;
            }
            // The likelihood of the plaintext being gibberish but also of the expected size is low but not zero.
            // If it can unlock the wallet, it's a good key.
            if (Unlock(_vMasterKey)) {
                vMasterKey = _vMasterKey;
                break;
            }
        }

        // We got a gibberish key...
        if (vMasterKey.empty()) {
            // Mimicking the error message of RPC_WALLET_PASSPHRASE_INCORRECT as it's possible
            // that the user may see this error when interacting with the upgradetohd RPC
            throw std::runtime_error("Error: The wallet passphrase entered was incorrect");
        }

        spk_man->GenerateNewHDChain(secureMnemonic, secureMnemonicPassphrase, vMasterKey);

        if (is_locked) {
            Lock();
        }
    } else {
        spk_man->GenerateNewHDChain(secureMnemonic, secureMnemonicPassphrase);
    }

    return true;
}

void CWallet::UpdateProgress(const std::string& title, int nProgress)
{
    ShowProgress(title, nProgress);
}

void CWallet::LoadDescriptorScriptPubKeyMan(uint256 id, WalletDescriptor& desc)
{
    auto spk_manager = std::unique_ptr<ScriptPubKeyMan>(new DescriptorScriptPubKeyMan(*this, desc));
    m_spk_managers[id] = std::move(spk_manager);
}

void CWallet::SetupDescriptorScriptPubKeyMans(const SecureString& mnemonic_arg, const SecureString mnemonic_passphrase)
{
    AssertLockHeld(cs_wallet);

    // Make a seed
    // TODO: remove duplicated code with CHDChain::SetMnemonic
    const SecureString mnemonic = mnemonic_arg.empty() ? CMnemonic::Generate(m_args.GetIntArg("-mnemonicbits", CHDChain::DEFAULT_MNEMONIC_BITS)) : mnemonic_arg;
    if (!CMnemonic::Check(mnemonic)) {
        throw std::runtime_error(std::string(__func__) + ": invalid mnemonic: `" + std::string(mnemonic.c_str()) + "`");
    }
    SecureVector seed_key;
    CMnemonic::ToSeed(mnemonic, mnemonic_passphrase, seed_key);

    // Get the extended key
    CExtKey master_key;
    master_key.SetSeed(MakeByteSpan(seed_key));

    for (bool internal : {false, true}) {
        { // OUTPUT_TYPE is only one: LEGACY
            auto spk_manager = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(*this));
            if (IsCrypted()) {
                if (IsLocked()) {
                    throw std::runtime_error(std::string(__func__) + ": Wallet is locked, cannot setup new descriptors");
                }
                if (!spk_manager->CheckDecryptionKey(vMasterKey) && !spk_manager->Encrypt(vMasterKey, nullptr)) {
                    throw std::runtime_error(std::string(__func__) + ": Could not encrypt new descriptors");
                }
            }
            spk_manager->SetupDescriptorGeneration(master_key, mnemonic, mnemonic_passphrase, internal);
            uint256 id = spk_manager->GetID();
            m_spk_managers[id] = std::move(spk_manager);
            AddActiveScriptPubKeyMan(id, internal);
        }
    }
}

void CWallet::AddActiveScriptPubKeyMan(uint256 id, bool internal)
{
    WalletBatch batch(GetDatabase());
    if (!batch.WriteActiveScriptPubKeyMan(id, internal)) {
        throw std::runtime_error(std::string(__func__) + ": writing active ScriptPubKeyMan id failed");
    }
    LoadActiveScriptPubKeyMan(id, internal);
}

void CWallet::LoadActiveScriptPubKeyMan(uint256 id, bool internal)
{
    // Activating ScriptPubKeyManager for a given output and change type is incompatible with legacy wallets.
    // Legacy wallets have only one ScriptPubKeyManager and it's active for all output and change types.
    Assert(IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));

    WalletLogPrintf("Setting spkMan to active: id = %s, type = %s, internal = %s\n", id.ToString(), FormatOutputType(OutputType::LEGACY), internal ? "true" : "false");
    auto& spk_mans = internal ? m_internal_spk_managers : m_external_spk_managers;
    auto& spk_mans_other = internal ? m_external_spk_managers : m_internal_spk_managers;
    auto spk_man = m_spk_managers.at(id).get();
    spk_mans = spk_man;

    if (spk_mans_other == spk_man) {
        spk_mans_other = nullptr;
    }

    NotifyCanGetAddressesChanged();

}

void CWallet::DeactivateScriptPubKeyMan(uint256 id, bool internal)
{
    auto spk_man = GetScriptPubKeyMan(internal);
    if (spk_man != nullptr && spk_man->GetID() == id) {
        WalletLogPrintf("Deactivate spkMan: id = %s, type = %s, internal = %s\n", id.ToString(), FormatOutputType(OutputType::LEGACY), internal ? "true" : "false");
        WalletBatch batch(GetDatabase());
        if (!batch.EraseActiveScriptPubKeyMan(internal)) {
            throw std::runtime_error(std::string(__func__) + ": erasing active ScriptPubKeyMan id failed");
        }

        auto& spk_mans = internal ? m_internal_spk_managers : m_external_spk_managers;
        spk_mans = nullptr;
    }

    NotifyCanGetAddressesChanged();
}

bool CWallet::IsLegacy() const
{
    if (m_internal_spk_managers == nullptr) return false;
    auto spk_man = dynamic_cast<LegacyScriptPubKeyMan*>(m_internal_spk_managers);
    return spk_man != nullptr;
}

DescriptorScriptPubKeyMan* CWallet::GetDescriptorScriptPubKeyMan(const WalletDescriptor& desc) const
{
    for (auto& spk_man_pair : m_spk_managers) {
        // Try to downcast to DescriptorScriptPubKeyMan then check if the descriptors match
        DescriptorScriptPubKeyMan* spk_manager = dynamic_cast<DescriptorScriptPubKeyMan*>(spk_man_pair.second.get());
        if (spk_manager != nullptr && spk_manager->HasWalletDescriptor(desc)) {
            return spk_manager;
        }
    }

    return nullptr;
}

ScriptPubKeyMan* CWallet::AddWalletDescriptor(WalletDescriptor& desc, const FlatSigningProvider& signing_provider, const std::string& label, bool internal)
{
    AssertLockHeld(cs_wallet);

    if (!IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        WalletLogPrintf("Cannot add WalletDescriptor to a non-descriptor wallet\n");
        return nullptr;
    }

    SecureString mnemonic;
    SecureString mnemonic_passphrase;

    auto spk_man = GetDescriptorScriptPubKeyMan(desc);
    if (spk_man) {
        WalletLogPrintf("Update existing descriptor: %s\n", desc.descriptor->ToString());
        spk_man->UpdateWalletDescriptor(desc);
    } else {
        auto new_spk_man = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(*this, desc));
        spk_man = new_spk_man.get();

        // Save the descriptor to memory
        m_spk_managers[new_spk_man->GetID()] = std::move(new_spk_man);
    }

    // Add the private keys to the descriptor
    for (const auto& entry : signing_provider.keys) {
        const CKey& key = entry.second;
        spk_man->AddDescriptorKey(key, key.GetPubKey());
    }

    // Top up key pool, the manager will generate new scriptPubKeys internally
    if (!spk_man->TopUp()) {
        WalletLogPrintf("Could not top up scriptPubKeys\n");
        return nullptr;
    }

    // Apply the label if necessary
    // Note: we disable labels for ranged descriptors
    if (!desc.descriptor->IsRange()) {
        auto script_pub_keys = spk_man->GetScriptPubKeys();
        if (script_pub_keys.empty()) {
            WalletLogPrintf("Could not generate scriptPubKeys (cache is empty)\n");
            return nullptr;
        }

        CTxDestination dest;
        if (!internal && ExtractDestination(script_pub_keys.at(0), dest)) {
            SetAddressBook(dest, label, "receive");
        }
    }

    // Save the descriptor to DB
    spk_man->WriteDescriptor();

    return spk_man;
}
} // namespace wallet
