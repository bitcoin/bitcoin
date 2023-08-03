// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <crypto/common.h>
#include <fs.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <key.h>
#include <key_io.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <txmempool.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/error.h>
#include <util/fees.h>
#include <util/moneystr.h>
#include <util/string.h>
#include <util/translation.h>
#ifdef USE_BDB
#include <wallet/bdb.h>
#endif
#include <wallet/coincontrol.h>
#include <wallet/coinselection.h>
#include <wallet/fees.h>
#include <walletinitinterface.h>
#include <warnings.h>

#include <coinjoin/client.h>
#include <coinjoin/options.h>
#include <governance/governance.h>
#include <evo/deterministicmns.h>
#include <masternode/sync.h>

#include <univalue.h>

#include <evo/providertx.h>

#include <llmq/instantsend.h>
#include <llmq/chainlocks.h>
#include <assert.h>

using interfaces::FoundBlock;

const std::map<uint64_t,std::string> WALLET_FLAG_CAVEATS{
    {WALLET_FLAG_AVOID_REUSE,
        "You need to rescan the blockchain in order to correctly mark used "
        "destinations in the past. Until this is done, some destinations may "
        "be considered unused, even if the opposite is the case."
    },
};

static const size_t OUTPUT_GROUP_MAX_ENTRIES = 10;

RecursiveMutex cs_wallets;
static std::vector<std::shared_ptr<CWallet>> vpwallets GUARDED_BY(cs_wallets);
static std::list<LoadWalletFn> g_load_wallet_fns GUARDED_BY(cs_wallets);

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

bool AddWallet(const std::shared_ptr<CWallet>& wallet)
{
    {
        LOCK(cs_wallets);
        assert(wallet);
        std::vector<std::shared_ptr<CWallet>>::const_iterator i = std::find(vpwallets.begin(), vpwallets.end(), wallet);
        if (i != vpwallets.end()) return false;
        vpwallets.push_back(wallet);
    }
    wallet->ConnectScriptPubKeyManNotifiers();
    assert(::masternodeSync != nullptr);
    coinJoinClientManagers.emplace(std::make_pair(wallet->GetName(), std::make_shared<CCoinJoinClientManager>(*wallet, *::masternodeSync)));
    g_wallet_init_interface.InitCoinJoinSettings();
    return true;
}

bool RemoveWallet(const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start, std::vector<bilingual_str>& warnings)
{
    assert(wallet);

    interfaces::Chain& chain = wallet->chain();
    std::string name = wallet->GetName();

    // Unregister with the validation interface which also drops shared ponters.
    wallet->m_chain_notifications_handler.reset();
    {
        LOCK(cs_wallets);
        std::vector<std::shared_ptr<CWallet>>::iterator i = std::find(vpwallets.begin(), vpwallets.end(), wallet);
        if (i == vpwallets.end()) return false;
        vpwallets.erase(i);
    }
    auto it = coinJoinClientManagers.find(wallet->GetName());
    coinJoinClientManagers.erase(it);
    g_wallet_init_interface.InitCoinJoinSettings();

    // Write the wallet setting
    UpdateWalletSetting(chain, name, load_on_start, warnings);

    return true;
}

bool RemoveWallet(const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start)
{
    std::vector<bilingual_str> warnings;
    return RemoveWallet(wallet, load_on_start, warnings);
}

std::vector<std::shared_ptr<CWallet>> GetWallets()
{
    LOCK(cs_wallets);
    return vpwallets;
}

std::shared_ptr<CWallet> GetWallet(const std::string& name)
{
    LOCK(cs_wallets);
    for (const std::shared_ptr<CWallet>& wallet : vpwallets) {
        if (wallet->GetName() == name) return wallet;
    }
    return nullptr;
}

std::unique_ptr<interfaces::Handler> HandleLoadWallet(LoadWalletFn load_wallet)
{
    LOCK(cs_wallets);
    auto it = g_load_wallet_fns.emplace(g_load_wallet_fns.end(), std::move(load_wallet));
    return interfaces::MakeHandler([it] { LOCK(cs_wallets); g_load_wallet_fns.erase(it); });
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
std::shared_ptr<CWallet> LoadWalletInternal(interfaces::Chain& chain, const std::string& name, std::optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    try {
        std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(name, options, status, error);
        if (!database) {
            error = Untranslated("Wallet file verification failed.") + Untranslated(" ") + error;
            return nullptr;
        }

        std::shared_ptr<CWallet> wallet = CWallet::Create(chain, name, std::move(database), options.create_flags, error, warnings);
        if (!wallet) {
            error = Untranslated("Wallet loading failed.") + Untranslated(" ") + error;
            status = DatabaseStatus::FAILED_LOAD;
            return nullptr;
        }
        AddWallet(wallet);
        wallet->postInitProcess();

        // Write the wallet setting
        UpdateWalletSetting(chain, name, load_on_start, warnings);

        return wallet;
    } catch (const std::runtime_error& e) {
        error = Untranslated(e.what());
        status = DatabaseStatus::FAILED_LOAD;
        return nullptr;
    }
}
} // namespace

std::shared_ptr<CWallet> LoadWallet(interfaces::Chain& chain, const std::string& name, std::optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    auto result = WITH_LOCK(g_loading_wallet_mutex, return g_loading_wallet_set.insert(name));
    if (!result.second) {
        error = Untranslated("Wallet already loading.");
        status = DatabaseStatus::FAILED_LOAD;
        return nullptr;
    }
    auto wallet = LoadWalletInternal(chain, name, load_on_start, options, status, error, warnings);
    WITH_LOCK(g_loading_wallet_mutex, g_loading_wallet_set.erase(result.first));
    return wallet;
}

std::shared_ptr<CWallet> CreateWallet(interfaces::Chain& chain, const std::string& name, std::optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    uint64_t wallet_creation_flags = options.create_flags;
    const SecureString& passphrase = options.create_passphrase;

    // Indicate that the wallet is actually supposed to be blank and not just blank to make it encrypted
    bool create_blank = (wallet_creation_flags & WALLET_FLAG_BLANK_WALLET);

    // Born encrypted wallets need to be created blank first.
    if (!passphrase.empty()) {
        wallet_creation_flags |= WALLET_FLAG_BLANK_WALLET;
    }

    // Wallet::Verify will check if we're trying to create a wallet with a duplicate name.
    std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(name, options, status, error);
    if (!database) {
        error = Untranslated("Wallet file verification failed.") + Untranslated(" ") + error;
        status = DatabaseStatus::FAILED_VERIFY;
        return nullptr;
    }

    // Do not allow a passphrase when private keys are disabled
    if (!passphrase.empty() && (wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        error = Untranslated("Passphrase provided but private keys are disabled. A passphrase is only used to encrypt private keys, so cannot be used for wallets with private keys disabled.");
        status = DatabaseStatus::FAILED_CREATE;
        return nullptr;
    }

    // Make the wallet
    std::shared_ptr<CWallet> wallet = CWallet::Create(chain, name, std::move(database), wallet_creation_flags, error, warnings);
    if (!wallet) {
        error = Untranslated("Wallet creation failed.") + Untranslated(" ") + error;
        status = DatabaseStatus::FAILED_CREATE;
        return nullptr;
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

            // Set a HD chain for the wallet
            // TODO: re-enable this and `keypoolsize_hd_internal` check in `wallet_createwallet.py`
            // when HD is the default mode (make sure this actually works!)...
            // if (auto spk_man = wallet->GetLegacyScriptPubKeyMan() {
            //   if (!spk_man->GenerateNewHDChainEncrypted("", "", passphrase)) {
            //     throw JSONRPCError(RPC_WALLET_ERROR, "Failed to generate encrypted HD wallet");
            //   }
            // }
            // ... and drop this
            LOCK(wallet->cs_wallet);
            wallet->UnsetWalletFlag(WALLET_FLAG_BLANK_WALLET);
            if (auto spk_man = wallet->GetLegacyScriptPubKeyMan()) {
                spk_man->NewKeyPool();
            }
            // end TODO

            // backup the wallet we just encrypted
            if (!wallet->AutoBackupWallet("", error, warnings) && !error.original.empty()) {
                status = DatabaseStatus::FAILED_ENCRYPT;
                return nullptr;
            }

            // Relock the wallet
            wallet->Lock();
        }
    }
    AddWallet(wallet);
    wallet->postInitProcess();

    // Write the wallet settings
    UpdateWalletSetting(chain, name, load_on_start, warnings);

    status = DatabaseStatus::SUCCESS;
    return wallet;
}

/** @defgroup mapWallet
 *
 * @{
 */

std::string COutput::ToString() const
{
    return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString(), i, nDepth, FormatMoney(tx->tx->vout[i].nValue));
}

const CWalletTx* CWallet::GetWalletTx(const uint256& hash) const
{
    LOCK(cs_wallet);
    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(hash);
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
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = static_cast<unsigned int>(pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime))));

                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + static_cast<unsigned int>(pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime)))) / 2;

                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;

                WalletLogPrintf("Wallet passphrase changed to an nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(_vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;
                WalletBatch(*database).WriteMasterKey(pMasterKey.first, pMasterKey.second);
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
    WalletBatch batch(*database);
    batch.WriteBestBlock(loc);
}

void CWallet::SetMinVersion(enum WalletFeature nVersion, WalletBatch* batch_in, bool fExplicit)
{
    LOCK(cs_wallet);
    if (nWalletVersion >= nVersion)
        return;

    // when doing an explicit upgrade, if we pass the max version permitted, upgrade all the way
    if (fExplicit && nVersion > nWalletMaxVersion)
            nVersion = FEATURE_LATEST;

    nWalletVersion = nVersion;

    if (nVersion > nWalletMaxVersion)
        nWalletMaxVersion = nVersion;

    {
        WalletBatch* batch = batch_in ? batch_in : new WalletBatch(*database);
        if (nWalletVersion > 40000)
            batch->WriteMinVersion(nWalletVersion);
        if (!batch_in)
            delete batch;
    }
}

bool CWallet::SetMaxVersion(int nVersion)
{
    LOCK(cs_wallet);
    // cannot downgrade below current version
    if (nWalletVersion > nVersion)
        return false;

    nWalletMaxVersion = nVersion;

    return true;
}

std::set<uint256> CWallet::GetConflicts(const uint256& txid) const
{
    std::set<uint256> result;
    AssertLockHeld(cs_wallet);

    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(txid);
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

void CWallet::Flush()
{
    database->Flush();
}

void CWallet::Close()
{
    database->Close();
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
bool CWallet::IsSpent(const uint256& hash, unsigned int n) const
{
    const COutPoint outpoint(hash, n);
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    range = mapTxSpends.equal_range(outpoint);

    for (TxSpends::const_iterator it = range.first; it != range.second; ++it)
    {
        const uint256& wtxid = it->second;
        std::map<uint256, CWalletTx>::const_iterator mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end()) {
            int depth = mit->second.GetDepthInMainChain();
            if (depth > 0  || (depth == 0 && !mit->second.isAbandoned()))
                return true; // Spent
        }
    }
    return false;
}

void CWallet::AddToSpends(const COutPoint& outpoint, const uint256& wtxid)
{
    mapTxSpends.insert(std::make_pair(outpoint, wtxid));
    setWalletUTXO.erase(outpoint);

    setLockedCoins.erase(outpoint);

    std::pair<TxSpends::iterator, TxSpends::iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    SyncMetaData(range);
}


void CWallet::AddToSpends(const uint256& wtxid)
{
    auto it = mapWallet.find(wtxid);
    assert(it != mapWallet.end());
    CWalletTx& thisTx = it->second;
    if (thisTx.IsCoinBase()) // Coinbases don't spend anything!
        return;

    for (const CTxIn& txin : thisTx.tx->vin)
        AddToSpends(txin.prevout, wtxid);
}

bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
        return false;

    CKeyingMaterial _vMasterKey;

    _vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetStrongRandBytes(&_vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey;

    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetStrongRandBytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = static_cast<unsigned int>(2500000 / ((double)(GetTimeMillis() - nStartTime)));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + static_cast<unsigned int>(kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime)))) / 2;

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
        WalletBatch* encrypted_batch = new WalletBatch(*database);
        if (!encrypted_batch->TxnBegin()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            return false;
        }
        encrypted_batch->WriteMasterKey(nMasterKeyMaxID, kMasterKey);

        // must get current HD chain before EncryptKeys
        CHDChain hdChainCurrent;

        // GetHDChain exist only at legacy.... how to validate it? just do static cast? we don't have any other type yet so may be ok temporary!
        if (auto spk_man = GetLegacyScriptPubKeyMan()) {
            spk_man->GetHDChain(hdChainCurrent);
            if (!spk_man->Encrypt(_vMasterKey, encrypted_batch)) {
                encrypted_batch->TxnAbort();
                delete encrypted_batch;
                encrypted_batch = nullptr;
                // We now probably have half of our keys encrypted in memory, and half not...
                // die and let the user reload the unencrypted wallet.
                assert(false);
            }
            if (!hdChainCurrent.IsNull()) {
                assert(spk_man->EncryptHDChain(_vMasterKey));

                CHDChain hdChainCrypted;
                assert(spk_man->GetHDChain(hdChainCrypted));

                DBG(
                    tfm::format(std::cout, "EncryptWallet -- current seed: '%s'\n", HexStr(hdChainCurrent.GetSeed()));
                    tfm::format(std::cout, "EncryptWallet -- crypted seed: '%s'\n", HexStr(hdChainCrypted.GetSeed()));
                );

                // ids should match, seed hashes should not
                assert(hdChainCurrent.GetID() == hdChainCrypted.GetID());
                assert(hdChainCurrent.GetSeedHash() != hdChainCrypted.GetSeedHash());

                assert(spk_man->SetCryptedHDChain(*encrypted_batch, hdChainCrypted, false));
            }
        }


        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, encrypted_batch, true);

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

        if (auto spk_man = GetLegacyScriptPubKeyMan()) {
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
        database->Rewrite();

        // BDB seems to have a bad habit of writing old data into
        // slack space in .dat files; that is bad if the old data is
        // unencrypted private keys. So:
        database->ReloadDbEnv();

    }
    NotifyStatusChanged(this);

    return true;
}

DBErrors CWallet::ReorderTransactions()
{
    LOCK(cs_wallet);
    WalletBatch batch(*database);

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
        WalletBatch(*database).WriteOrderPosNext(nOrderPosNext);
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
            if (used && !GetDestData(dst, "used", nullptr)) {
                if (AddDestData(batch, dst, "used", "p")) { // p for "present", opposite of absent (null)
                    tx_destinations.insert(dst);
                }
            } else if (!used && GetDestData(dst, "used", nullptr)) {
                EraseDestData(batch, dst, "used");
            }
        }
    }
}

bool CWallet::IsSpentKey(const uint256& hash, unsigned int n) const
{
    AssertLockHeld(cs_wallet);
    const CWalletTx* srctx = GetWalletTx(hash);
    if (srctx) {
        assert(srctx->tx->vout.size() > n);
        LegacyScriptPubKeyMan* spk_man = GetLegacyScriptPubKeyMan();
        // When descriptor wallets arrive, these additional checks are
        // likely superfluous and can be optimized out
        assert(spk_man != nullptr);
        for (const auto& keyid : GetAffectedKeys(srctx->tx->vout[n].scriptPubKey, *spk_man)) {
            if (GetDestData(PKHash(keyid), "used", nullptr)) {
                return true;
            }
        }
    }
    return false;
}

bool CWallet::AddToWallet(const CWalletTx& wtxIn, bool fFlushOnClose)
{
    LOCK(cs_wallet);

    WalletBatch batch(*database, fFlushOnClose);

    uint256 hash = wtxIn.GetHash();

    if (IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE)) {
        // Mark used destinations
        std::set<CTxDestination> tx_destinations;

        for (const CTxIn& txin : wtxIn.tx->vin) {
            const COutPoint& op = txin.prevout;
            SetSpentKeyState(batch, op.hash, op.n, true, tx_destinations);
        }

        MarkDestinationsDirty(tx_destinations);
    }

    // Inserts only if not already there, returns tx inserted or tx found
    std::pair<std::map<uint256, CWalletTx>::iterator, bool> ret = mapWallet.insert(std::make_pair(hash, wtxIn));
    CWalletTx& wtx = (*ret.first).second;
    wtx.BindWallet(this);
    bool fInsertedNew = ret.second;
    if (fInsertedNew) {
        wtx.nTimeReceived = GetTime();
        wtx.nOrderPos = IncOrderPosNext(&batch);
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
        wtx.nTimeSmart = ComputeTimeSmart(wtx);
        AddToSpends(hash);

        auto mnList = deterministicMNManager->GetListAtChainTip();
        for(unsigned int i = 0; i < wtx.tx->vout.size(); ++i) {
            if (IsMine(wtx.tx->vout[i]) && !IsSpent(hash, i)) {
                setWalletUTXO.insert(COutPoint(hash, i));
                if (deterministicMNManager->IsProTxWithCollateral(wtx.tx, i) || mnList.HasMNByCollateral(COutPoint(hash, i))) {
                    LockCoin(COutPoint(hash, i));
                }
            }
        }
    }

    bool fUpdated = false;
    if (!fInsertedNew)
    {
        if (wtxIn.m_confirm.status != wtx.m_confirm.status) {
            wtx.m_confirm.status = wtxIn.m_confirm.status;
            wtx.m_confirm.nIndex = wtxIn.m_confirm.nIndex;
            wtx.m_confirm.hashBlock = wtxIn.m_confirm.hashBlock;
            wtx.m_confirm.block_height = wtxIn.m_confirm.block_height;
            fUpdated = true;
        } else {
            assert(wtx.m_confirm.nIndex == wtxIn.m_confirm.nIndex);
            assert(wtx.m_confirm.hashBlock == wtxIn.m_confirm.hashBlock);
            assert(wtx.m_confirm.block_height == wtxIn.m_confirm.block_height);
        }
        if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
        {
            wtx.fFromMe = wtxIn.fFromMe;
            fUpdated = true;
        }

        auto mnList = deterministicMNManager->GetListAtChainTip();
        for (unsigned int i = 0; i < wtx.tx->vout.size(); ++i) {
            if (IsMine(wtx.tx->vout[i]) && !IsSpent(hash, i)) {
                bool new_utxo = setWalletUTXO.insert(COutPoint(hash, i)).second;
                if (new_utxo && (deterministicMNManager->IsProTxWithCollateral(wtx.tx, i) || mnList.HasMNByCollateral(COutPoint(hash, i)))) {
                    LockCoin(COutPoint(hash, i));
                }
                fUpdated |= new_utxo;
            }
        }
    }

    //// debug print
    WalletLogPrintf("AddToWallet %s  %s%s\n", wtxIn.GetHash().ToString(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

    // Write to disk
    if (fInsertedNew || fUpdated)
        if (!batch.WriteTx(wtx))
            return false;

    // Break debit/credit balance caches:
    wtx.MarkDirty();

    // Notify UI of new or updated transaction
    NotifyTransactionChanged(this, hash, fInsertedNew ? CT_NEW : CT_UPDATED);

#if HAVE_SYSTEM
    // notify an external script when a wallet transaction comes in or is updated
    std::string strCmd = gArgs.GetArg("-walletnotify", "");

    if (!strCmd.empty())
    {
        ReplaceAll(strCmd, "%s", wtxIn.GetHash().GetHex());
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

    return true;
}

void CWallet::LoadToWallet(CWalletTx& wtxIn)
{
    // If wallet doesn't have a chain (e.g dash-wallet), don't bother to update txn.
    if (HaveChain()) {
        bool active;
        int height;
        if (chain().findBlock(wtxIn.m_confirm.hashBlock, FoundBlock().inActiveChain(active).height(height)) && active) {
            // Update cached block height variable since it not stored in the
            // serialized transaction.
            wtxIn.m_confirm.block_height = height;
        } else if (wtxIn.isConflicted() || wtxIn.isConfirmed()) {
            // If tx block (or conflicting block) was reorged out of chain
            // while the wallet was shutdown, change tx status to UNCONFIRMED
            // and reset block height, hash, and index. ABANDONED tx don't have
            // associated blocks and don't need to be updated. The case where a
            // transaction was reorged out while online and then reconfirmed
            // while offline is covered by the rescan logic.
            wtxIn.setUnconfirmed();
            wtxIn.m_confirm.hashBlock = uint256();
            wtxIn.m_confirm.block_height = 0;
            wtxIn.m_confirm.nIndex = 0;
        }
    }
    uint256 hash = wtxIn.GetHash();
    const auto& ins = mapWallet.emplace(hash, wtxIn);
    CWalletTx& wtx = ins.first->second;
    wtx.BindWallet(this);
    if (/* insertion took place */ ins.second) {
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
    }
    AddToSpends(hash);
    for (const CTxIn& txin : wtx.tx->vin) {
        auto it = mapWallet.find(txin.prevout.hash);
        if (it != mapWallet.end()) {
            CWalletTx& prevtx = it->second;
            if (prevtx.isConflicted()) {
                MarkConflicted(prevtx.m_confirm.hashBlock, prevtx.m_confirm.block_height, wtx.GetHash());
            }
        }
    }
}

bool CWallet::AddToWalletIfInvolvingMe(const CTransactionRef& ptx, CWalletTx::Confirmation confirm, WalletBatch& batch, bool fUpdate)
{
    const CTransaction& tx = *ptx;
    {
        AssertLockHeld(cs_wallet);

        if (!confirm.hashBlock.IsNull()) {
            for (const CTxIn& txin : tx.vin) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(txin.prevout);
                while (range.first != range.second) {
                    if (range.first->second != tx.GetHash()) {
                        WalletLogPrintf("Transaction %s (in block %s) conflicts with wallet transaction %s (both spend %s:%i)\n", tx.GetHash().ToString(), confirm.hashBlock.ToString(), range.first->second.ToString(), range.first->first.hash.ToString(), range.first->first.n);
                        MarkConflicted(confirm.hashBlock, confirm.block_height, range.first->second);
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
            if (!confirm.hashBlock.IsNull()) {
                int64_t block_time_tmp;
                bool found_block = chain().findBlock(confirm.hashBlock, FoundBlock().maxTime(block_time_tmp));
                assert(found_block);
                block_time = block_time_tmp;
            }
            // loop though all outputs
            for (const CTxOut& txout: tx.vout) {
                for (const auto& spk_man_pair : m_spk_managers) {
                    spk_man_pair.second->MarkUnusedAddresses(batch, txout.scriptPubKey, block_time);
                }
            }

            CWalletTx wtx(this, ptx);

            // Block disconnection override an abandoned tx as unconfirmed
            // which means user may have to call abandontransaction again
            wtx.m_confirm = confirm;

            return AddToWallet(wtx, false);
        }
    }
    return false;
}

bool CWallet::TransactionCanBeAbandoned(const uint256& hashTx) const
{
    LOCK(cs_wallet);
    const CWalletTx* wtx = GetWalletTx(hashTx);
    return wtx && !wtx->isAbandoned() && wtx->GetDepthInMainChain() == 0 && !wtx->InMempool();
}

bool CWallet::TransactionCanBeResent(const uint256& hashTx) const
{
    LOCK(cs_wallet);
    const CWalletTx* wtx = GetWalletTx(hashTx);
    return wtx && wtx->CanBeResent();
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

    WalletBatch batch(*database);

    std::set<uint256> todo;
    std::set<uint256> done;

    // Can't mark abandoned if confirmed or in mempool
    auto it = mapWallet.find(hashTx);
    assert(it != mapWallet.end());
    CWalletTx& origtx = it->second;
    if (origtx.GetDepthInMainChain() != 0 || origtx.InMempool() || origtx.IsLockedByInstantSend()) {
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
        int currentconfirm = wtx.GetDepthInMainChain();
        // If the orig tx was not in block, none of its spends can be
        assert(currentconfirm <= 0);
        // if (currentconfirm < 0) {Tx and spends are already conflicted, no need to abandon}
        if (currentconfirm == 0 && !wtx.isAbandoned()) {
            // If the orig tx was not in block/mempool, none of its spends can be in mempool
            assert(!wtx.InMempool());
            wtx.setAbandoned();
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            NotifyTransactionChanged(this, wtx.GetHash(), CT_UPDATED);
            // Iterate over all its outputs, and mark transactions in the wallet that spend them abandoned too
            TxSpends::const_iterator iter = mapTxSpends.lower_bound(COutPoint(now, 0));
            while (iter != mapTxSpends.end() && iter->first.hash == now) {
                if (!done.count(iter->second)) {
                    todo.insert(iter->second);
                }
                iter++;
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

    std::string unused_err_string;
    return wtx.SubmitMemoryPoolAndRelay(unused_err_string, true);
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
    WalletBatch batch(*database, false);

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
        int currentconfirm = wtx.GetDepthInMainChain();
        if (conflictconfirms < currentconfirm) {
            // Block is 'more conflicted' than current confirm; update.
            // Mark transaction as conflicted with this block.
            wtx.m_confirm.nIndex = 0;
            wtx.m_confirm.hashBlock = hashBlock;
            wtx.m_confirm.block_height = conflicting_height;
            wtx.setConflicted();
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            // Iterate over all its outputs, and mark transactions in the wallet that spend them conflicted too
            TxSpends::const_iterator iter = mapTxSpends.lower_bound(COutPoint(now, 0));
            while (iter != mapTxSpends.end() && iter->first.hash == now) {
                 if (!done.count(iter->second)) {
                     todo.insert(iter->second);
                 }
                 iter++;
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            MarkInputsDirty(wtx.tx);
        }
    }

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::SyncTransaction(const CTransactionRef& ptx, CWalletTx::Confirmation confirm, WalletBatch& batch, bool update_tx)
{
    if (!AddToWalletIfInvolvingMe(ptx, confirm, batch, update_tx))
        return; // Not one of ours

    // If a transaction changes 'conflicted' state, that changes the balance
    // available of the outputs it spends. So force those to be
    // recomputed, also:
    MarkInputsDirty(ptx);

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::transactionAddedToMempool(const CTransactionRef& ptx, int64_t nAcceptTime) {
    LOCK(cs_wallet);
    CWalletTx::Confirmation confirm(CWalletTx::Status::UNCONFIRMED, /* block_height */ 0, {}, /* nIndex */ 0);
    WalletBatch batch(*database);
    SyncTransaction(ptx, confirm, batch);

    auto it = mapWallet.find(ptx->GetHash());
    if (it != mapWallet.end()) {
        it->second.fInMempool = true;
    }
}

void CWallet::transactionRemovedFromMempool(const CTransactionRef &ptx, MemPoolRemovalReason reason) {
    if (reason != MemPoolRemovalReason::CONFLICT) {
        LOCK(cs_wallet);
        auto it = mapWallet.find(ptx->GetHash());
        if (it != mapWallet.end()) {
            it->second.fInMempool = false;
        }
    }
}

void CWallet::blockConnected(const CBlock& block, int height)
{
    const uint256& block_hash = block.GetHash();
    LOCK(cs_wallet);

    m_last_block_processed_height = height;
    m_last_block_processed = block_hash;
    WalletBatch batch(*database);
    for (size_t index = 0; index < block.vtx.size(); index++) {
        CWalletTx::Confirmation confirm(CWalletTx::Status::CONFIRMED, height, block_hash, index);
        SyncTransaction(block.vtx[index], confirm, batch);
        transactionRemovedFromMempool(block.vtx[index], MemPoolRemovalReason::MANUAL);
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
    WalletBatch batch(*database);
    for (const CTransactionRef& ptx : block.vtx) {
        CWalletTx::Confirmation confirm(CWalletTx::Status::UNCONFIRMED, /* block_height */ 0, {}, /* nIndex */ 0);
        SyncTransaction(ptx, confirm, batch);
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
    // chainActive.Tip(), otherwise put a callback in the validation interface queue and wait
    // for the queue to drain enough to execute it (indicating we are caught up
    // at least with the time we entered this function).
    uint256 last_block_hash = WITH_LOCK(cs_wallet, return m_last_block_processed);
    chain().waitForNotificationsIfTipChanged(last_block_hash);
}


isminetype CWallet::IsMine(const CTxIn &txin) const
{
    {
        LOCK(cs_wallet);
        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.tx->vout.size())
                return IsMine(prev.tx->vout[txin.prevout.n]);
        }
    }
    return ISMINE_NO;
}

// Note that this function doesn't distinguish between a 0-valued input,
// and a not-"is mine" (according to the filter) input.
CAmount CWallet::GetDebit(const CTxIn &txin, const isminefilter& filter) const
{
    {
        LOCK(cs_wallet);
        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
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

// Recursively determine the rounds of a given input (How deep is the CoinJoin chain for a given input)
int CWallet::GetRealOutpointCoinJoinRounds(const COutPoint& outpoint, int nRounds) const
{
    LOCK(cs_wallet);

    const int nRoundsMax = MAX_COINJOIN_ROUNDS + CCoinJoinClientOptions::GetRandomRounds();

    if (nRounds >= nRoundsMax) {
        // there can only be nRoundsMax rounds max
        return nRoundsMax - 1;
    }

    auto pair = mapOutpointRoundsCache.emplace(outpoint, -10);
    auto nRoundsRef = &pair.first->second;
    if (!pair.second) {
        // we already processed it, just return what we have
        return *nRoundsRef;
    }

    // TODO wtx should refer to a CWalletTx object, not a pointer, based on surrounding code
    const CWalletTx* wtx = GetWalletTx(outpoint.hash);

    if (wtx == nullptr || wtx->tx == nullptr) {
        // no such tx in this wallet
        *nRoundsRef = -1;
        LogPrint(BCLog::COINJOIN, "%s FAILED    %-70s %3d\n", __func__, outpoint.ToStringShort(), -1);
        return *nRoundsRef;
    }

    // bounds check
    if (outpoint.n >= wtx->tx->vout.size()) {
        // should never actually hit this
        *nRoundsRef = -4;
        LogPrint(BCLog::COINJOIN, "%s FAILED    %-70s %3d\n", __func__, outpoint.ToStringShort(), -4);
        return *nRoundsRef;
    }

    auto txOutRef = &wtx->tx->vout[outpoint.n];

    if (CCoinJoin::IsCollateralAmount(txOutRef->nValue)) {
        *nRoundsRef = -3;
        LogPrint(BCLog::COINJOIN, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
        return *nRoundsRef;
    }

    // make sure the final output is non-denominate
    if (!CCoinJoin::IsDenominatedAmount(txOutRef->nValue)) { //NOT DENOM
        *nRoundsRef = -2;
        LogPrint(BCLog::COINJOIN, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
        return *nRoundsRef;
    }

    for (const auto& out : wtx->tx->vout) {
        if (!CCoinJoin::IsDenominatedAmount(out.nValue)) {
            // this one is denominated but there is another non-denominated output found in the same tx
            *nRoundsRef = 0;
            LogPrint(BCLog::COINJOIN, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
            return *nRoundsRef;
        }
    }

    int nShortest = -10; // an initial value, should be no way to get this by calculations
    bool fDenomFound = false;
    // only denoms here so let's look up
    for (const auto& txinNext : wtx->tx->vin) {
        if (IsMine(txinNext)) {
            int n = GetRealOutpointCoinJoinRounds(txinNext.prevout, nRounds + 1);
            // denom found, find the shortest chain or initially assign nShortest with the first found value
            if(n >= 0 && (n < nShortest || nShortest == -10)) {
                nShortest = n;
                fDenomFound = true;
            }
        }
    }
    *nRoundsRef = fDenomFound
            ? (nShortest >= nRoundsMax - 1 ? nRoundsMax : nShortest + 1) // good, we a +1 to the shortest one but only nRoundsMax rounds max allowed
            : 0;            // too bad, we are the fist one in that chain
    LogPrint(BCLog::COINJOIN, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
    return *nRoundsRef;
}

// respect current settings
int CWallet::GetCappedOutpointCoinJoinRounds(const COutPoint& outpoint) const
{
    LOCK(cs_wallet);
    int realCoinJoinRounds = GetRealOutpointCoinJoinRounds(outpoint);
    return realCoinJoinRounds > CCoinJoinClientOptions::GetRounds() ? CCoinJoinClientOptions::GetRounds() : realCoinJoinRounds;
}

bool CWallet::IsDenominated(const COutPoint& outpoint) const
{
    LOCK(cs_wallet);

    const auto it = mapWallet.find(outpoint.hash);
    if (it == mapWallet.end()) {
        return false;
    }

    if (outpoint.n >= it->second.tx->vout.size()) {
        return false;
    }

    return CCoinJoin::IsDenominatedAmount(it->second.tx->vout[outpoint.n].nValue);
}

bool CWallet::IsFullyMixed(const COutPoint& outpoint) const
{
    int nRounds = GetRealOutpointCoinJoinRounds(outpoint);
    // Mix again if we don't have N rounds yet
    if (nRounds < CCoinJoinClientOptions::GetRounds()) return false;

    // Try to mix a "random" number of rounds more than minimum.
    // If we have already mixed N + MaxOffset rounds, don't mix again.
    // Otherwise, we should mix again 50% of the time, this results in an exponential decay
    // N rounds 50% N+1 25% N+2 12.5%... until we reach N + GetRandomRounds() rounds where we stop.
    if (nRounds < CCoinJoinClientOptions::GetRounds() + CCoinJoinClientOptions::GetRandomRounds()) {
        CDataStream ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << outpoint << nCoinJoinSalt;
        uint256 nHash;
        CSHA256().Write((const unsigned char*)ss.data(), ss.size()).Finalize(nHash.begin());
        if (ReadLE64(nHash.begin()) % 2 == 0) {
            return false;
        }
    }

    return true;
}

isminetype CWallet::IsMine(const CTxOut& txout) const
{
    return IsMine(txout.scriptPubKey);
}

isminetype CWallet::IsMine(const CTxDestination& dest) const
{
    return IsMine(GetScriptForDestination(dest));
}

isminetype CWallet::IsMine(const CScript& script) const
{
    isminetype result = ISMINE_NO;
    for (const auto& spk_man_pair : m_spk_managers) {
        result = std::max(result, spk_man_pair.second->IsMine(script));
    }
    return result;
}

CAmount CWallet::GetCredit(const CTxOut& txout, const isminefilter& filter) const
{
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    return ((IsMine(txout) & filter) ? txout.nValue : 0);
}

bool CWallet::IsChange(const CTxOut& txout) const
{
    return IsChange(txout.scriptPubKey);
}

bool CWallet::IsChange(const CScript& script) const
{
    // TODO: fix handling of 'change' outputs. The assumption is that any
    // payment to a script that is ours, but is not in the address book
    // is change. That assumption is likely to break when we implement multisignature
    // wallets that return change back into a multi-signature-protected address;
    // a better way of identifying which outputs are 'the send' and which are
    // 'the change' will need to be implemented (maybe extend CWalletTx to remember
    // which output, if any, was change).
    if (IsMine(script))
    {
        CTxDestination address;
        if (!ExtractDestination(script, address))
            return true;

        LOCK(cs_wallet);
        if (!FindAddressBookEntry(address)) {
            return true;
        }
    }
    return false;
}

CAmount CWallet::GetChange(const CTxOut& txout) const
{
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    return (IsChange(txout) ? txout.nValue : 0);
}

bool CWallet::IsMine(const CTransaction& tx) const
{
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

bool CWallet::IsAllFromMe(const CTransaction& tx, const isminefilter& filter) const
{
    LOCK(cs_wallet);

    for (const CTxIn& txin : tx.vin)
    {
        auto mi = mapWallet.find(txin.prevout.hash);
        if (mi == mapWallet.end())
            return false; // any unknown inputs can't be from us

        const CWalletTx& prev = (*mi).second;

        if (txin.prevout.n >= prev.tx->vout.size())
            return false; // invalid input!

        if (!(IsMine(prev.tx->vout[txin.prevout.n]) & filter))
            return false;
    }
    return true;
}

CAmount CWallet::GetCredit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nCredit = 0;
    for (const CTxOut& txout : tx.vout)
    {
        nCredit += GetCredit(txout, filter);
        if (!MoneyRange(nCredit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nCredit;
}

CAmount CWallet::GetChange(const CTransaction& tx) const
{
    CAmount nChange = 0;
    for (const CTxOut& txout : tx.vout)
    {
        nChange += GetChange(txout);
        if (!MoneyRange(nChange))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nChange;
}

bool CWallet::IsHDEnabled() const
{
    bool result = true;
    for (const auto& spk_man_pair : m_spk_managers) {
        result &= spk_man_pair.second->IsHDEnabled();
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
    if (!WalletBatch(*database).WriteWalletFlags(m_wallet_flags))
        throw std::runtime_error(std::string(__func__) + ": writing wallet flags failed");
}

void CWallet::UnsetWalletFlag(uint64_t flag)
{
    LOCK(cs_wallet);
    WalletBatch batch(*database);
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
    auto it = coinJoinClientManagers.find(GetName());
    if (it != coinJoinClientManagers.end()) {
        it->second->StopMixing();
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

bool CWallet::SetWalletFlags(uint64_t overwriteFlags, bool memonly)
{
    LOCK(cs_wallet);
    m_wallet_flags = overwriteFlags;
    if (((overwriteFlags & KNOWN_WALLET_FLAGS) >> 32) ^ (overwriteFlags >> 32)) {
        // contains unknown non-tolerable wallet flags
        return false;
    }
    if (!memonly && !WalletBatch(*database).WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) + ": writing wallet flags failed");
    }

    return true;
}

int64_t CWalletTx::GetTxTime() const
{
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

// Helper for producing a max-sized low-S low-R signature (eg 71 bytes)
// or a max-sized low-S signature (e.g. 72 bytes) if use_max_sig is true
bool CWallet::DummySignInput(CTxIn &tx_in, const CTxOut &txout, bool use_max_sig) const
{
    // Fill in dummy signatures for fee calculation.
    const CScript& scriptPubKey = txout.scriptPubKey;
    SignatureData sigdata;

    std::unique_ptr<SigningProvider> provider = GetSolvingProvider(scriptPubKey);
    if (!provider) {
        // We don't know about this scriptpbuKey;
        return false;
    }

    if (!ProduceSignature(*provider, use_max_sig ? DUMMY_MAXIMUM_SIGNATURE_CREATOR : DUMMY_SIGNATURE_CREATOR, scriptPubKey, sigdata)) {
        return false;
    }
    UpdateInput(tx_in, sigdata);
    return true;
}

// Helper for producing a bunch of max-sized low-S low-R signatures (eg 71 bytes)
bool CWallet::DummySignTx(CMutableTransaction &txNew, const std::vector<CTxOut> &txouts, bool use_max_sig) const
{
    // Fill in dummy signatures for fee calculation.
    int nIn = 0;
    for (const auto& txout : txouts)
    {
        if (!DummySignInput(txNew.vin[nIn], txout, use_max_sig)) {
            return false;
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
        WalletBatch batch(*database);
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

int64_t CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, bool use_max_sig)
{
    std::vector<CTxOut> txouts;
    for (const CTxIn& input : tx.vin) {
        const auto mi = wallet->mapWallet.find(input.prevout.hash);
        // Can not estimate size without knowing the input details
        if (mi == wallet->mapWallet.end()) {
            return -1;
        }
        assert(input.prevout.n < mi->second.tx->vout.size());
        txouts.emplace_back(mi->second.tx->vout[input.prevout.n]);
    }
    return CalculateMaximumSignedTxSize(tx, wallet, txouts, use_max_sig);
}

// txouts needs to be in the order of tx.vin
int64_t CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, const std::vector<CTxOut>& txouts, bool use_max_sig)
{
    CMutableTransaction txNew(tx);
    if (!wallet->DummySignTx(txNew, txouts, use_max_sig)) {
        return -1;
    }
    return ::GetSerializeSize(txNew, PROTOCOL_VERSION);
}

int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* wallet, bool use_max_sig)
{
    CMutableTransaction txn;
    txn.vin.push_back(CTxIn(COutPoint()));
    if (!wallet->DummySignInput(txn.vin[0], txout, use_max_sig)) {
        return -1;
    }
    return ::GetSerializeSize(txn.vin[0], PROTOCOL_VERSION);
}

void CWalletTx::GetAmounts(std::list<COutputEntry>& listReceived,
                           std::list<COutputEntry>& listSent, CAmount& nFee, const isminefilter& filter) const
{
    nFee = 0;
    listReceived.clear();
    listSent.clear();

    // Compute fee:
    CAmount nDebit = GetDebit(filter);
    if (nDebit > 0) // debit>0 means we signed/sent this transaction
    {
        CAmount nValueOut = tx->GetValueOut();
        nFee = nDebit - nValueOut;
    }

    // Sent/received.
    for (unsigned int i = 0; i < tx->vout.size(); ++i)
    {
        const CTxOut& txout = tx->vout[i];
        isminetype fIsMine = pwallet->IsMine(txout);
        // Only need to handle txouts if AT LEAST one of these is true:
        //   1) they debit from us (sent)
        //   2) the output is to us (received)
        if (nDebit > 0)
        {
            // Don't report 'change' txouts
            if (pwallet->IsChange(txout))
                continue;
        }
        else if (!(fIsMine & filter))
            continue;

        // In either case, we need to get the destination address
        CTxDestination address;

        if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
        {
            pwallet->WalletLogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
                                    this->GetHash().ToString());
            address = CNoDestination();
        }

        COutputEntry output = {address, txout.nValue, (int)i};

        // If we are debited by the transaction, add the output as a "sent" entry
        if (nDebit > 0)
            listSent.push_back(output);

        // If we are receiving the output, add it as a "received" entry
        if (fIsMine & filter)
            listReceived.push_back(output);
    }

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
        ScanResult result = ScanForWalletTransactions(start_block, start_height, {} /* max_height */, reserver, update);
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
CWallet::ScanResult CWallet::ScanForWalletTransactions(const uint256& start_block, int start_height, std::optional<int> max_height, const WalletRescanReserver& reserver, bool fUpdate)
{
    int64_t nNow = GetTime();
    int64_t start_time = GetTimeMillis();

    assert(reserver.isReserved());

    uint256 block_hash = start_block;
    ScanResult result;

    WalletLogPrintf("Rescan started from block %s...\n", start_block.ToString());

    ShowProgress(strprintf("%s " + _("Rescanning...").translated, GetDisplayName()), 0); // show rescan progress in GUI as dialog or on splashscreen, if -rescan on startup
    uint256 tip_hash = WITH_LOCK(cs_wallet, return GetLastBlockHash());
    uint256 end_hash = tip_hash;
    if (max_height) chain().findAncestorByHeight(tip_hash, *max_height, FoundBlock().hash(end_hash));
    double progress_begin = chain().guessVerificationProgress(block_hash);
    double progress_end = chain().guessVerificationProgress(end_hash);
    double progress_current = progress_begin;
    int block_height = start_height;
    WalletBatch batch(*database);
    while (!fAbortRescan && !chain().shutdownRequested()) {
        if (progress_end - progress_begin > 0.0) {
            m_scanning_progress = (progress_current - progress_begin) / (progress_end - progress_begin);
        } else { // avoid divide-by-zero for single block scan range (i.e. start and stop hashes are equal)
            m_scanning_progress = 0;
        }
        if (block_height % 100 == 0 && progress_end - progress_begin > 0.0) {
            ShowProgress(strprintf("%s " + _("Rescanning...").translated, GetDisplayName()), std::max(1, std::min(99, (int)(m_scanning_progress * 100))));
        }
        if (GetTime() >= nNow + 60) {
            nNow = GetTime();
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
                CWalletTx::Confirmation confirm(CWalletTx::Status::CONFIRMED, block_height, block_hash, posInBlock);
                SyncTransaction(block.vtx[posInBlock], confirm, batch, fUpdate);
            }
            // scan succeeded, record block as most recent successfully scanned
            result.last_scanned_block = block_hash;
            result.last_scanned_height = block_height;
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
    ShowProgress(strprintf("%s " + _("Rescanning...").translated, GetDisplayName()), 100); // hide progress dialog in GUI
    if (block_height && fAbortRescan) {
        WalletLogPrintf("Rescan aborted at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else if (block_height && chain().shutdownRequested()) {
        WalletLogPrintf("Rescan interrupted by shutdown request at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else {
        WalletLogPrintf("Rescan completed in %15dms\n", GetTimeMillis() - start_time);
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

        int nDepth = wtx.GetDepthInMainChain();

        if (!wtx.IsCoinBase() && (nDepth == 0 && !wtx.IsLockedByInstantSend() && !wtx.isAbandoned())) {
            mapSorted.insert(std::make_pair(wtx.nOrderPos, &wtx));
        }
    }

    // Try to add wallet transactions to memory pool
    for (const std::pair<const int64_t, CWalletTx*>& item : mapSorted) {
        CWalletTx& wtx = *(item.second);
        std::string unused_err_string;
        wtx.SubmitMemoryPoolAndRelay(unused_err_string, false);
    }
}

bool CWalletTx::CanBeResent() const
{
    return
        // Can't relay if wallet is not broadcasting
        pwallet->GetBroadcastTransactions() &&
        // Don't relay abandoned transactions
        !isAbandoned() &&
        // Don't try to submit coinbase transactions. These would fail anyway but would
        // cause log spam.
        !IsCoinBase() &&
        // Don't try to submit conflicted or confirmed transactions.
        GetDepthInMainChain() == 0 &&
        // Don't try to submit transactions locked via InstantSend.
        !IsLockedByInstantSend();
}

bool CWalletTx::SubmitMemoryPoolAndRelay(std::string& err_string, bool relay)
{
    if (!CanBeResent()) return false;

    // Submit transaction to mempool for relay
    pwallet->WalletLogPrintf("Submitting wtx %s to mempool for relay\n", GetHash().ToString());
    // We must set fInMempool here - while it will be re-set to true by the
    // entered-mempool callback, if we did not there would be a race where a
    // user could call sendmoney in a loop and hit spurious out of funds errors
    // because we think that this newly generated transaction's change is
    // unavailable as we're not yet aware that it is in the mempool.
    //
    // Irrespective of the failure reason, un-marking fInMempool
    // out-of-order is incorrect - it should be unmarked when
    // TransactionRemovedFromMempool fires.
    bool ret = pwallet->chain().broadcastTransaction(tx, pwallet->m_default_max_tx_fee, relay, err_string);
    fInMempool |= ret;
    return ret;
}

std::set<uint256> CWalletTx::GetConflicts() const
{
    std::set<uint256> result;
    if (pwallet != nullptr)
    {
        AssertLockHeld(pwallet->cs_wallet);
        uint256 myHash = GetHash();
        result = pwallet->GetConflicts(myHash);
        result.erase(myHash);
    }
    return result;
}

CAmount CWalletTx::GetCachableAmount(AmountType type, const isminefilter& filter, bool recalculate) const
{
    auto& amount = m_amounts[type];
    if (recalculate || !amount.m_cached[filter]) {
        amount.Set(filter, type == DEBIT ? pwallet->GetDebit(*tx, filter) : pwallet->GetCredit(*tx, filter));
        m_is_cache_empty = false;
    }
    return amount.m_value[filter];
}

CAmount CWalletTx::GetDebit(const isminefilter& filter) const
{
    if (tx->vin.empty())
        return 0;

    CAmount debit = 0;
    if (filter & ISMINE_SPENDABLE) {
        debit += GetCachableAmount(DEBIT, ISMINE_SPENDABLE);
    }
    if (filter & ISMINE_WATCH_ONLY) {
        debit += GetCachableAmount(DEBIT, ISMINE_WATCH_ONLY);
    }
    return debit;
}

CAmount CWalletTx::GetCredit(const isminefilter& filter) const
{
    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsImmatureCoinBase())
        return 0;

    CAmount credit = 0;
    if (filter & ISMINE_SPENDABLE) {
        // GetBalance can assume transactions in mapWallet won't change
        credit += GetCachableAmount(CREDIT, ISMINE_SPENDABLE);
    }
    if (filter & ISMINE_WATCH_ONLY) {
        credit += GetCachableAmount(CREDIT, ISMINE_WATCH_ONLY);
    }
    return credit;
}

CAmount CWalletTx::GetImmatureCredit(bool fUseCache) const
{
    if (IsImmatureCoinBase() && IsInMainChain()) {
        return GetCachableAmount(IMMATURE_CREDIT, ISMINE_SPENDABLE, !fUseCache);
    }

    return 0;
}

CAmount CWalletTx::GetAvailableCredit(bool fUseCache, const isminefilter& filter) const
{
    if (pwallet == nullptr)
        return 0;

    // Avoid caching ismine for NO or ALL cases (could remove this check and simplify in the future).
    bool allow_cache = (filter & ISMINE_ALL) && (filter & ISMINE_ALL) != ISMINE_ALL;

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsImmatureCoinBase())
        return 0;

    if (fUseCache && allow_cache && m_amounts[AVAILABLE_CREDIT].m_cached[filter]) {
        return m_amounts[AVAILABLE_CREDIT].m_value[filter];
    }

    bool allow_used_addresses = (filter & ISMINE_USED) || !pwallet->IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE);
    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < tx->vout.size(); i++)
    {
        if (!pwallet->IsSpent(hashTx, i) && (allow_used_addresses || !pwallet->IsSpentKey(hashTx, i)))
        {
            const CTxOut &txout = tx->vout[i];
            nCredit += pwallet->GetCredit(txout, filter);
            if (!MoneyRange(nCredit))
                throw std::runtime_error(std::string(__func__) + ": value out of range");
        }
    }

    if (allow_cache) {
        m_amounts[AVAILABLE_CREDIT].Set(filter, nCredit);
        m_is_cache_empty = false;
    }

    return nCredit;
}

CAmount CWalletTx::GetImmatureWatchOnlyCredit(const bool fUseCache) const
{
    if (IsImmatureCoinBase() && IsInMainChain()) {
        return GetCachableAmount(IMMATURE_CREDIT, ISMINE_WATCH_ONLY, !fUseCache);
    }

    return 0;
}

CAmount CWalletTx::GetAnonymizedCredit(const CCoinControl* coinControl) const
{
    if (!pwallet)
        return 0;

    AssertLockHeld(pwallet->cs_wallet);

    // Exclude coinbase and conflicted txes
    if (IsCoinBase() || GetDepthInMainChain() < 0)
        return 0;

    if (coinControl == nullptr && m_amounts[ANON_CREDIT].m_cached[ISMINE_SPENDABLE])
        return m_amounts[ANON_CREDIT].m_value[ISMINE_SPENDABLE];

    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < tx->vout.size(); i++)
    {
        const CTxOut &txout = tx->vout[i];
        const COutPoint outpoint = COutPoint(hashTx, i);

        if (coinControl != nullptr && coinControl->HasSelected() && !coinControl->IsSelected(outpoint)) {
            continue;
        }

        if (pwallet->IsSpent(hashTx, i) || !CCoinJoin::IsDenominatedAmount(txout.nValue)) continue;

        if (pwallet->IsFullyMixed(outpoint)) {
            nCredit += pwallet->GetCredit(txout, ISMINE_SPENDABLE);
            if (!MoneyRange(nCredit))
                throw std::runtime_error(std::string(__func__) + ": value out of range");
        }
    }

    if (coinControl == nullptr) {
        m_amounts[ANON_CREDIT].Set(ISMINE_SPENDABLE, nCredit);
    }

    return nCredit;
}

CAmount CWalletTx::GetDenominatedCredit(bool unconfirmed, bool fUseCache) const
{
    if (pwallet == nullptr)
        return 0;

    AssertLockHeld(pwallet->cs_wallet);

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsCoinBase() && GetBlocksToMaturity() > 0)
        return 0;

    int nDepth = GetDepthInMainChain();
    if (nDepth < 0) return 0;

    bool isUnconfirmed = IsTrusted() && nDepth == 0;
    if (unconfirmed != isUnconfirmed) return 0;

    if (fUseCache) {
        if(unconfirmed && m_amounts[DENOM_UCREDIT].m_cached[ISMINE_SPENDABLE]) {
            return m_amounts[DENOM_UCREDIT].m_value[ISMINE_SPENDABLE];
        } else if (!unconfirmed && m_amounts[DENOM_CREDIT].m_cached[ISMINE_SPENDABLE]) {
            return m_amounts[DENOM_CREDIT].m_value[ISMINE_SPENDABLE];
        }
    }

    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < tx->vout.size(); i++)
    {
        const CTxOut &txout = tx->vout[i];

        if (pwallet->IsSpent(hashTx, i) || !CCoinJoin::IsDenominatedAmount(txout.nValue)) continue;

        nCredit += pwallet->GetCredit(txout, ISMINE_SPENDABLE);
        if (!MoneyRange(nCredit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }

    if (unconfirmed) {
        m_amounts[DENOM_UCREDIT].Set(ISMINE_SPENDABLE, nCredit);
    } else {
        m_amounts[DENOM_CREDIT].Set(ISMINE_SPENDABLE, nCredit);
    }
    return nCredit;
}

CAmount CWalletTx::GetChange() const
{
    if (fChangeCached)
        return nChangeCached;
    nChangeCached = pwallet->GetChange(*tx);
    fChangeCached = true;
    return nChangeCached;
}

bool CWalletTx::InMempool() const
{
    return fInMempool;
}

bool CWalletTx::IsTrusted() const
{
    std::set<uint256> s;
    return IsTrusted(s);
}

bool CWalletTx::IsTrusted(std::set<uint256>& trusted_parents) const
{
    // Quick answer in most cases
    if (!pwallet->chain().checkFinalTx(*tx)) return false;
    int nDepth = GetDepthInMainChain();
    if (nDepth >= 1) return true;
    if (nDepth < 0) return false;
    if (IsLockedByInstantSend()) return true;
    // using wtx's cached debit
    if (!pwallet->m_spend_zero_conf_change || !IsFromMe(ISMINE_ALL)) return false;

    // Don't trust unconfirmed transactions from us unless they are in the mempool.
    if (!InMempool()) return false;

    // Trusted if all inputs are from us and are in the mempool:
    for (const CTxIn& txin : tx->vin)
    {
        // Transactions not sent by us: not trusted
        const CWalletTx* parent = pwallet->GetWalletTx(txin.prevout.hash);
        if (parent == nullptr) return false;
        const CTxOut& parentOut = parent->tx->vout[txin.prevout.n];
        // Check that this specific input being spent is trusted
        if (pwallet->IsMine(parentOut) != ISMINE_SPENDABLE) return false;
        // If we've already trusted this parent, continue
        if (trusted_parents.count(parent->GetHash())) continue;
        // Recurse to check that the parent is also trusted
        if (!parent->IsTrusted(trusted_parents)) return false;
        trusted_parents.insert(parent->GetHash());
    }
    return true;
}

bool CWalletTx::IsEquivalentTo(const CWalletTx& _tx) const
{
        CMutableTransaction tx1 {*this->tx};
        CMutableTransaction tx2 {*_tx.tx};
        for (auto& txin : tx1.vin) txin.scriptSig = CScript();
        for (auto& txin : tx2.vin) txin.scriptSig = CScript();
        return CTransaction(tx1) == CTransaction(tx2);
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
            // the last block. SubmitMemoryPoolAndRelay() will not rebroadcast
            // any confirmed or conflicting txs.
            if (wtx.nTimeReceived > m_best_block_time - 5 * 60) continue;
            std::string unused_err_string;
            if (wtx.SubmitMemoryPoolAndRelay(unused_err_string, true)) ++submitted_tx_count;
        }
    } // cs_wallet

    if (submitted_tx_count > 0) {
        WalletLogPrintf("%s: resubmit %u unconfirmed transactions\n", __func__, submitted_tx_count);
    }
}

/** @} */ // end of mapWallet

void MaybeResendWalletTxs()
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
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

CWallet::Balance CWallet::GetBalance(const int min_depth, const bool avoid_reuse, const bool fAddLocked, const CCoinControl* coinControl) const
{
    Balance ret;
    isminefilter reuse_filter = avoid_reuse ? 0 : ISMINE_USED;
    {
        LOCK(cs_wallet);
        std::set<uint256> trusted_parents;
        for (auto pcoin : GetSpendableTXs()) {
            const bool is_trusted{pcoin->IsTrusted(trusted_parents)};
            const int tx_depth{pcoin->GetDepthInMainChain()};
            const CAmount tx_credit_mine{pcoin->GetAvailableCredit(/* fUseCache */ true, ISMINE_SPENDABLE | reuse_filter)};
            const CAmount tx_credit_watchonly{pcoin->GetAvailableCredit(/* fUseCache */ true, ISMINE_WATCH_ONLY | reuse_filter)};
            if (is_trusted && ((tx_depth >= min_depth) || (fAddLocked && pcoin->IsLockedByInstantSend()))) {
                ret.m_mine_trusted += tx_credit_mine;
                ret.m_watchonly_trusted += tx_credit_watchonly;
            }
            if (!is_trusted && tx_depth == 0 && pcoin->InMempool()) {
                ret.m_mine_untrusted_pending += tx_credit_mine;
                ret.m_watchonly_untrusted_pending += tx_credit_watchonly;
            }
            ret.m_mine_immature += pcoin->GetImmatureCredit();
            ret.m_watchonly_immature += pcoin->GetImmatureWatchOnlyCredit();
            if (CCoinJoinClientOptions::IsEnabled()) {
                ret.m_anonymized += pcoin->GetAnonymizedCredit(coinControl);
                ret.m_denominated_trusted += pcoin->GetDenominatedCredit(false);
                ret.m_denominated_untrusted_pending += pcoin->GetDenominatedCredit(true);
            }
        }
    }
    return ret;
}

CAmount CWallet::GetAnonymizableBalance(bool fSkipDenominated, bool fSkipUnconfirmed) const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    std::vector<CompactTallyItem> vecTally = SelectCoinsGroupedByAddresses(fSkipDenominated, true, fSkipUnconfirmed);
    if (vecTally.empty()) return 0;

    CAmount nTotal = 0;

    const CAmount nSmallestDenom = CCoinJoin::GetSmallestDenomination();
    const CAmount nMixingCollateral = CCoinJoin::GetCollateralAmount();
    for (const auto& item : vecTally) {
        bool fIsDenominated = CCoinJoin::IsDenominatedAmount(item.nAmount);
        if(fSkipDenominated && fIsDenominated) continue;
        // assume that the fee to create denoms should be mixing collateral at max
        if(item.nAmount >= nSmallestDenom + (fIsDenominated ? 0 : nMixingCollateral))
            nTotal += item.nAmount;
    }

    return nTotal;
}

// Note: calculated including unconfirmed,
// that's ok as long as we use it for informational purposes only
float CWallet::GetAverageAnonymizedRounds() const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    int nTotal = 0;
    int nCount = 0;

    LOCK(cs_wallet);
    for (const auto& outpoint : setWalletUTXO) {
        if(!IsDenominated(outpoint)) continue;

        nTotal += GetCappedOutpointCoinJoinRounds(outpoint);
        nCount++;
    }

    if(nCount == 0) return 0;

    return (float)nTotal/nCount;
}

// Note: calculated including unconfirmed,
// that's ok as long as we use it for informational purposes only
CAmount CWallet::GetNormalizedAnonymizedBalance() const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    CAmount nTotal = 0;

    LOCK(cs_wallet);
    for (const auto& outpoint : setWalletUTXO) {
        const auto it = mapWallet.find(outpoint.hash);
        if (it == mapWallet.end()) continue;

        CAmount nValue = it->second.tx->vout[outpoint.n].nValue;
        if (!CCoinJoin::IsDenominatedAmount(nValue)) continue;
        if (it->second.GetDepthInMainChain() < 0) continue;

        int nRounds = GetCappedOutpointCoinJoinRounds(outpoint);
        nTotal += nValue * nRounds / CCoinJoinClientOptions::GetRounds();
    }

    return nTotal;
}

CAmount CWallet::GetAvailableBalance(const CCoinControl* coinControl) const
{
    LOCK(cs_wallet);

    CAmount balance = 0;
    std::vector<COutput> vCoins;
    AvailableCoins(vCoins, true, coinControl);
    for (const COutput& out : vCoins) {
        if (out.fSpendable) {
            balance += out.tx->tx->vout[out.i].nValue;
        }
    }
    return balance;
}

void CWallet::AvailableCoins(std::vector<COutput> &vCoins, bool fOnlySafe, const CCoinControl* coinControl, const CAmount& nMinimumAmount, const CAmount& nMaximumAmount, const CAmount &nMinimumSumAmount, const uint64_t nMaximumCount) const
{
    AssertLockHeld(cs_wallet);

    vCoins.clear();
    CoinType nCoinType = coinControl ? coinControl->nCoinType : CoinType::ALL_COINS;

    CAmount nTotal = 0;
    // Either the WALLET_FLAG_AVOID_REUSE flag is not set (in which case we always allow), or we default to avoiding, and only in the case where
    // a coin control object is provided, and has the avoid address reuse flag set to false, do we allow already used addresses
    bool allow_used_addresses = !IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE) || (coinControl && !coinControl->m_avoid_address_reuse);
    const int min_depth = {coinControl ? coinControl->m_min_depth : DEFAULT_MIN_DEPTH};
    const int max_depth = {coinControl ? coinControl->m_max_depth : DEFAULT_MAX_DEPTH};

    std::set<uint256> trusted_parents;
    for (auto pcoin : GetSpendableTXs()) {
        const uint256& wtxid = pcoin->GetHash();

        if (!chain().checkFinalTx(*pcoin->tx))
            continue;

        if (pcoin->IsImmatureCoinBase())
            continue;

        int nDepth = pcoin->GetDepthInMainChain();

        // We should not consider coins which aren't at least in our mempool
        // It's possible for these to be conflicted via ancestors which we may never be able to detect
        if (nDepth == 0 && !pcoin->InMempool())
            continue;

        bool safeTx = pcoin->IsTrusted(trusted_parents);

        if (fOnlySafe && !safeTx) {
            continue;
        }

        if (nDepth < min_depth || nDepth > max_depth) {
            continue;
        }

        for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++) {
            bool found = false;
            if (nCoinType == CoinType::ONLY_FULLY_MIXED) {
                if (!CCoinJoin::IsDenominatedAmount(pcoin->tx->vout[i].nValue)) continue;
                found = IsFullyMixed(COutPoint(wtxid, i));
            } else if(nCoinType == CoinType::ONLY_READY_TO_MIX) {
                if (!CCoinJoin::IsDenominatedAmount(pcoin->tx->vout[i].nValue)) continue;
                found = !IsFullyMixed(COutPoint(wtxid, i));
            } else if(nCoinType == CoinType::ONLY_NONDENOMINATED) {
                if (CCoinJoin::IsCollateralAmount(pcoin->tx->vout[i].nValue)) continue; // do not use collateral amounts
                found = !CCoinJoin::IsDenominatedAmount(pcoin->tx->vout[i].nValue);
            } else if(nCoinType == CoinType::ONLY_MASTERNODE_COLLATERAL) {
                found = dmn_types::IsCollateralAmount(pcoin->tx->vout[i].nValue);
            } else if(nCoinType == CoinType::ONLY_COINJOIN_COLLATERAL) {
                found = CCoinJoin::IsCollateralAmount(pcoin->tx->vout[i].nValue);
            } else {
                found = true;
            }
            if(!found) continue;

            if (pcoin->tx->vout[i].nValue < nMinimumAmount || pcoin->tx->vout[i].nValue > nMaximumAmount)
                continue;

            if (coinControl && coinControl->HasSelected() && !coinControl->fAllowOtherInputs && !coinControl->IsSelected(COutPoint(wtxid, i)))
                continue;

            if (IsLockedCoin(wtxid, i) && nCoinType != CoinType::ONLY_MASTERNODE_COLLATERAL)
                continue;

            if (IsSpent(wtxid, i))
                continue;

            isminetype mine = IsMine(pcoin->tx->vout[i]);

            if (mine == ISMINE_NO) {
                continue;
            }

            if (!allow_used_addresses && IsSpentKey(wtxid, i)) {
                continue;
            }

            std::unique_ptr<SigningProvider> provider = GetSolvingProvider(pcoin->tx->vout[i].scriptPubKey);

            bool solvable = provider ? IsSolvable(*provider, pcoin->tx->vout[i].scriptPubKey) : false;
            bool spendable = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) || (((mine & ISMINE_WATCH_ONLY) != ISMINE_NO) && (coinControl && coinControl->fAllowWatchOnly && solvable));

            vCoins.push_back(COutput(pcoin, i, nDepth, spendable, solvable, safeTx, (coinControl && coinControl->fAllowWatchOnly)));

            // Checks the sum amount of all UTXO's.
            if (nMinimumSumAmount != MAX_MONEY) {
                nTotal += pcoin->tx->vout[i].nValue;

                if (nTotal >= nMinimumSumAmount) {
                    return;
                }
            }

            // Checks the maximum number of UTXO's.
            if (nMaximumCount > 0 && vCoins.size() >= nMaximumCount) {
                return;
            }
        }
    }
}

std::map<CTxDestination, std::vector<COutput>> CWallet::ListCoins() const
{
    AssertLockHeld(cs_wallet);

    std::map<CTxDestination, std::vector<COutput>> result;
    std::vector<COutput> availableCoins;

    AvailableCoins(availableCoins);

    for (const COutput& coin : availableCoins) {
        CTxDestination address;
        if ((coin.fSpendable || (IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && coin.fSolvable)) &&
            ExtractDestination(FindNonChangeParentOutput(*coin.tx->tx, coin.i).scriptPubKey, address)) {
            result[address].emplace_back(std::move(coin));
        }
    }

    std::vector<COutPoint> lockedCoins;
    ListLockedCoins(lockedCoins);
    // Include watch-only for LegacyScriptPubKeyMan wallets without private keys
    const bool include_watch_only = GetLegacyScriptPubKeyMan() && IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    const isminetype is_mine_filter = include_watch_only ? ISMINE_WATCH_ONLY : ISMINE_SPENDABLE;
    for (const COutPoint& output : lockedCoins) {
        auto it = mapWallet.find(output.hash);
        if (it != mapWallet.end()) {
            int depth = it->second.GetDepthInMainChain();
            if (depth >= 0 && output.n < it->second.tx->vout.size() &&
                IsMine(it->second.tx->vout[output.n]) == is_mine_filter
            ) {
                CTxDestination address;
                if (ExtractDestination(FindNonChangeParentOutput(*it->second.tx, output.n).scriptPubKey, address)) {
                    result[address].emplace_back(
                        &it->second, output.n, depth, true /* spendable */, true /* solvable */, false /* safe */);
                }
            }
        }
    }

    return result;
}

const CTxOut& CWallet::FindNonChangeParentOutput(const CTransaction& tx, int output) const
{
    const CTransaction* ptx = &tx;
    int n = output;
    while (IsChange(ptx->vout[n]) && ptx->vin.size() > 0) {
        const COutPoint& prevout = ptx->vin[0].prevout;
        auto it = mapWallet.find(prevout.hash);
        if (it == mapWallet.end() || it->second.tx->vout.size() <= prevout.n ||
            !IsMine(it->second.tx->vout[prevout.n])) {
            break;
        }
        ptx = it->second.tx.get();
        n = prevout.n;
    }
    return ptx->vout[n];
}

void CWallet::InitCoinJoinSalt()
{
    // Avoid fetching it multiple times
    assert(nCoinJoinSalt.IsNull());

    WalletBatch batch(*database);
    if (!batch.ReadCoinJoinSalt(nCoinJoinSalt) && batch.ReadCoinJoinSalt(nCoinJoinSalt, true)) {
        batch.WriteCoinJoinSalt(nCoinJoinSalt);
    }

    while (nCoinJoinSalt.IsNull()) {
        // We never generated/saved it
        nCoinJoinSalt = GetRandHash();
        batch.WriteCoinJoinSalt(nCoinJoinSalt);
    }
}

struct CompareByPriority
{
    bool operator()(const COutput& t1,
                    const COutput& t2) const
    {
        return CCoinJoin::CalculateAmountPriority(t1.GetInputCoin().effective_value) > CCoinJoin::CalculateAmountPriority(t2.GetInputCoin().effective_value);
    }
};

bool CWallet::SelectCoinsMinConf(const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, std::vector<OutputGroup> groups,
                                 std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CoinSelectionParams& coin_selection_params, bool& bnb_used, CoinType nCoinType) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    std::vector<OutputGroup> utxo_pool;
    if (coin_selection_params.use_bnb) {
        // Get long term estimate
        FeeCalculation feeCalc;
        CCoinControl temp;
        temp.m_confirm_target = 1008;
        CFeeRate long_term_feerate = GetMinimumFeeRate(*this, temp, &feeCalc);

        // Calculate cost of change
        CAmount cost_of_change = GetDiscardRate(*this).GetFee(coin_selection_params.change_spend_size) + coin_selection_params.effective_fee.GetFee(coin_selection_params.change_output_size);

        // Filter by the min conf specs and add to utxo_pool and calculate effective value
        for (OutputGroup& group : groups) {
            if (!group.EligibleForSpending(eligibility_filter)) continue;

            group.fee = 0;
            group.long_term_fee = 0;
            group.effective_value = 0;
            for (auto it = group.m_outputs.begin(); it != group.m_outputs.end(); ) {
                const CInputCoin& coin = *it;
                CAmount effective_value = coin.txout.nValue - (coin.m_input_bytes < 0 ? 0 : coin_selection_params.effective_fee.GetFee(coin.m_input_bytes));
                // Only include outputs that are positive effective value (i.e. not dust)
                if (effective_value > 0) {
                    group.fee += coin.m_input_bytes < 0 ? 0 : coin_selection_params.effective_fee.GetFee(coin.m_input_bytes);
                    group.long_term_fee += coin.m_input_bytes < 0 ? 0 : long_term_feerate.GetFee(coin.m_input_bytes);
                    if (coin_selection_params.m_subtract_fee_outputs) {
                        group.effective_value += coin.txout.nValue;
                    } else {
                        group.effective_value += effective_value;
                    }
                    ++it;
                } else {
                    it = group.Discard(coin);
                }
            }
            if (group.effective_value > 0) utxo_pool.push_back(group);
        }
        // Calculate the fees for things that aren't inputs
        CAmount not_input_fees = coin_selection_params.effective_fee.GetFee(coin_selection_params.tx_noinputs_size);
        bnb_used = true;
        return SelectCoinsBnB(utxo_pool, nTargetValue, cost_of_change, setCoinsRet, nValueRet, not_input_fees);
    } else {
        // Filter by the min conf specs and add to utxo_pool
        for (const OutputGroup& group : groups) {
            if (!group.EligibleForSpending(eligibility_filter)) continue;
            utxo_pool.push_back(group);
        }
        bnb_used = false;
        return KnapsackSolver(nTargetValue, utxo_pool, setCoinsRet, nValueRet, nCoinType == CoinType::ONLY_FULLY_MIXED, m_default_max_tx_fee);
    }
}

bool CWallet::SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CCoinControl& coin_control, CoinSelectionParams& coin_selection_params, bool& bnb_used) const
{
    // Note: this function should never be used for "always free" tx types like dstx

    std::vector<COutput> vCoins(vAvailableCoins);
    CoinType nCoinType = coin_control.nCoinType;
    CAmount value_to_select = nTargetValue;

    // Default to bnb was not used. If we use it, we set it later
    bnb_used = false;

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coin_control.HasSelected() && !coin_control.fAllowOtherInputs)
    {
        for (const COutput& out : vCoins)
        {
            if(!out.fSpendable)
                continue;

            nValueRet += out.tx->tx->vout[out.i].nValue;
            setCoinsRet.insert(out.GetInputCoin());

            if (!coin_control.fRequireAllInputs && nValueRet >= nTargetValue) {
                // stop when we added at least one input and enough inputs to have at least nTargetValue funds
                return true;
            }
        }

        return (nValueRet >= nTargetValue);
    }

    // calculate value from preset inputs and store them
    std::set<CInputCoin> setPresetCoins;
    CAmount nValueFromPresetInputs = 0;

    std::vector<COutPoint> vPresetInputs;
    coin_control.ListSelected(vPresetInputs);
    for (const COutPoint& outpoint : vPresetInputs)
    {
        std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it != mapWallet.end())
        {
            const CWalletTx* pcoin = &it->second;
            // Clearly invalid input, fail
            if (pcoin->tx->vout.size() <= outpoint.n) {
                return false;
            }
            if (nCoinType == CoinType::ONLY_FULLY_MIXED) {
                // Make sure to include mixed preset inputs only,
                // even if some non-mixed inputs were manually selected via CoinControl
                if (!IsFullyMixed(outpoint)) continue;
            }
            // Just to calculate the marginal byte size
            CInputCoin coin(pcoin->tx, outpoint.n, pcoin->GetSpendSize(outpoint.n, false));
            nValueFromPresetInputs += coin.txout.nValue;
            if (coin.m_input_bytes <= 0) {
                return false; // Not solvable, can't estimate size for fee
            }
            coin.effective_value = coin.txout.nValue - coin_selection_params.effective_fee.GetFee(coin.m_input_bytes);
            if (coin_selection_params.use_bnb) {
                value_to_select -= coin.effective_value;
            } else {
                value_to_select -= coin.txout.nValue;
            }
            setPresetCoins.insert(coin);
        } else {
            return false; // TODO: Allow non-wallet inputs
        }
    }

    // remove preset inputs from vCoins
    for (std::vector<COutput>::iterator it = vCoins.begin(); it != vCoins.end() && coin_control.HasSelected();)
    {
        if (setPresetCoins.count(it->GetInputCoin()))
            it = vCoins.erase(it);
        else
            ++it;
    }

    unsigned int limit_ancestor_count = 0;
    unsigned int limit_descendant_count = 0;
    chain().getPackageLimits(limit_ancestor_count, limit_descendant_count);
    size_t max_ancestors = (size_t)std::max<int64_t>(1, limit_ancestor_count);
    size_t max_descendants = (size_t)std::max<int64_t>(1, limit_descendant_count);
    bool fRejectLongChains = gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    // form groups from remaining coins; note that preset coins will not
    // automatically have their associated (same address) coins included
    if (coin_control.m_avoid_partial_spends && vCoins.size() > OUTPUT_GROUP_MAX_ENTRIES) {
        // Cases where we have 11+ outputs all pointing to the same destination may result in
        // privacy leaks as they will potentially be deterministically sorted. We solve that by
        // explicitly shuffling the outputs before processing
        Shuffle(vCoins.begin(), vCoins.end(), FastRandomContext());
    }
    std::vector<OutputGroup> groups = GroupOutputs(vCoins, !coin_control.m_avoid_partial_spends, max_ancestors);

    bool res = value_to_select <= 0 ||
        SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(1, 6, 0), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType) ||
        SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(1, 1, 0), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, 2), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, std::min((size_t)4, max_ancestors/3), std::min((size_t)4, max_descendants/3)), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, max_ancestors/2, max_descendants/2), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, max_ancestors-1, max_descendants-1), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) ||
        (m_spend_zero_conf_change && !fRejectLongChains && SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, std::numeric_limits<uint64_t>::max()), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType));

    // because SelectCoinsMinConf clears the setCoinsRet, we now add the possible inputs to the coinset
    util::insert(setCoinsRet, setPresetCoins);

    // add preset inputs to the total value selected
    nValueRet += nValueFromPresetInputs;

    return res;
}

bool CWallet::SignTransaction(CMutableTransaction& tx) const
{
    AssertLockHeld(cs_wallet);

    // Build coins map
    std::map<COutPoint, Coin> coins;
    for (auto& input : tx.vin) {
        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(input.prevout.hash);
        if(mi == mapWallet.end() || input.prevout.n >= mi->second.tx->vout.size()) {
            return false;
        }
        const CWalletTx& wtx = mi->second;
        coins[input.prevout] = Coin(wtx.tx->vout[input.prevout.n], wtx.m_confirm.block_height, wtx.IsCoinBase());
    }
    std::map<int, std::string> input_errors;
    return SignTransaction(tx, coins, SIGHASH_ALL, input_errors);
}

bool CWallet::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, std::string>& input_errors) const
{
    // Sign the tx with ScriptPubKeyMans
    // Because each ScriptPubKeyMan can sign more than one input, we need to keep track of each ScriptPubKeyMan that has signed this transaction.
    // Each iteration, we may sign more txins than the txin that is specified in that iteration.
    // We assume that each input is signed by only one ScriptPubKeyMan.
    std::set<uint256> visited_spk_mans;
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        // Get the prevout
        CTxIn& txin = tx.vin[i];
        auto coin = coins.find(txin.prevout);
        if (coin == coins.end() || coin->second.IsSpent()) {
            input_errors[i] = "Input not found or already spent";
            continue;
        }

        // Check if this input is complete
        SignatureData sigdata = DataFromTransaction(tx, i, coin->second.out);
        if (sigdata.complete) {
            continue;
        }

        // Input needs to be signed, find the right ScriptPubKeyMan
        std::set<ScriptPubKeyMan*> spk_mans = GetScriptPubKeyMans(coin->second.out.scriptPubKey, sigdata);
        if (spk_mans.size() == 0) {
            input_errors[i] = "Unable to sign input, missing keys";
            continue;
        }

        for (auto& spk_man : spk_mans) {
            // If we've already been signed by this spk_man, skip it
            if (visited_spk_mans.count(spk_man->GetID()) > 0) {
                continue;
            }

            // Sign the tx.
            // spk_man->SignTransaction will return true if the transaction is complete,
            // so we can exit early and return true if that happens.
            if (spk_man->SignTransaction(tx, coins, sighash, input_errors)) {
                return true;
            }

            // Add this spk_man to visited_spk_mans so we can skip it later
            visited_spk_mans.insert(spk_man->GetID());
        }
    }
    return false;
}

TransactionError CWallet::FillPSBT(PartiallySignedTransaction& psbtx, bool& complete, int sighash_type, bool sign, bool bip32derivs) const
{
    LOCK(cs_wallet);
    // Get all of the previous transactions
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // Verify input looks sane. This will check that we have at most one uxto, witness or non-witness.
        if (!input.IsSane()) {
            return TransactionError::INVALID_PSBT;
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
    // Because each ScriptPubKeyMan may be able to fill more than one input, we need to keep track of each ScriptPubKeyMan that has filled this psbt.
    // Each iteration, we may fill more inputs than the input that is specified in that iteration.
    // We assume that each input is filled by only one ScriptPubKeyMan
    std::set<uint256> visited_spk_mans;
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // Get the scriptPubKey to know which SigningProvider to use
        CScript script;
        if (input.non_witness_utxo) {
            if (txin.prevout.n >= input.non_witness_utxo->vout.size()) {
                return TransactionError::MISSING_INPUTS;
            }
            script = input.non_witness_utxo->vout[txin.prevout.n].scriptPubKey;
        } else {
            // There's no UTXO so we can just skip this now
            complete = false;
            continue;
        }
        SignatureData sigdata;
        input.FillSignatureData(sigdata);
        std::set<ScriptPubKeyMan*> spk_mans = GetScriptPubKeyMans(script, sigdata);
        if (spk_mans.size() == 0) {
            continue;
        }

        for (auto& spk_man : spk_mans) {
            // If we've already been signed by this spk_man, skip it
            if (visited_spk_mans.count(spk_man->GetID()) > 0) {
                continue;
            }

            // Fill in the information from the spk_man
            TransactionError res = spk_man->FillPSBT(psbtx, sighash_type, sign, bip32derivs);
            if (res != TransactionError::OK) {
                return res;
            }

            // Add this spk_man to visited_spk_mans so we can skip it later
            visited_spk_mans.insert(spk_man->GetID());
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
            return spk_man_pair.second->SignMessage(message, pkhash, str_sig);
        }
    }
    return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
}

bool CWallet::FundTransaction(CMutableTransaction& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, CCoinControl coinControl)
{
    std::vector<CRecipient> vecSend;

    // Turn the txout set into a CRecipient vector.
    for (size_t idx = 0; idx < tx.vout.size(); idx++) {
        const CTxOut& txOut = tx.vout[idx];
        CRecipient recipient = {txOut.scriptPubKey, txOut.nValue, setSubtractFeeFromOutputs.count(idx) == 1};
        vecSend.push_back(recipient);
    }

    coinControl.fAllowOtherInputs = true;

    for (const CTxIn& txin : tx.vin) {
        coinControl.Select(txin.prevout);
    }

    // Acquire the locks to prevent races to the new locked unspents between the
    // CreateTransaction call and LockCoin calls (when lockUnspents is true).
    LOCK(cs_wallet);

    int nExtraPayloadSize = 0;
    if (tx.nVersion == 3 && tx.nType != TRANSACTION_NORMAL)
        nExtraPayloadSize = (int)tx.vExtraPayload.size();

    CTransactionRef tx_new;
    if (!CreateTransaction(vecSend, tx_new, nFeeRet, nChangePosInOut, error, coinControl, false, nExtraPayloadSize)) {
        return false;
    }

    if (nChangePosInOut != -1) {
        tx.vout.insert(tx.vout.begin() + nChangePosInOut, tx_new->vout[nChangePosInOut]);
    }

    // Copy output sizes from new transaction; they may have had the fee
    // subtracted from them.
    for (unsigned int idx = 0; idx < tx.vout.size(); idx++) {
        tx.vout[idx].nValue = tx_new->vout[idx].nValue;
    }

    // Add new txins while keeping original txin scriptSig/order.
    for (const CTxIn& txin : tx_new->vin) {
        if (!coinControl.IsSelected(txin.prevout)) {
            tx.vin.push_back(txin);

            if (lockUnspents) {
                LockCoin(txin.prevout);
            }
        }
    }

    return true;
}

bool CWallet::SelectTxDSInsByDenomination(int nDenom, CAmount nValueMax, std::vector<CTxDSIn>& vecTxDSInRet)
{
    LOCK(cs_wallet);

    CAmount nValueTotal{0};

    std::set<uint256> setRecentTxIds;
    std::vector<COutput> vCoins;

    vecTxDSInRet.clear();

    if (!CCoinJoin::IsValidDenomination(nDenom)) {
        return false;
    }
    CAmount nDenomAmount = CCoinJoin::DenominationToAmount(nDenom);

    CCoinControl coin_control;
    coin_control.nCoinType = CoinType::ONLY_READY_TO_MIX;
    AvailableCoins(vCoins, true, &coin_control);
    LogPrint(BCLog::COINJOIN, "CWallet::%s -- vCoins.size(): %d\n", __func__, vCoins.size());

    Shuffle(vCoins.rbegin(), vCoins.rend(), FastRandomContext());

    for (const auto& out : vCoins) {
        uint256 txHash = out.tx->GetHash();
        CAmount nValue = out.tx->tx->vout[out.i].nValue;
        if (setRecentTxIds.find(txHash) != setRecentTxIds.end()) continue; // no duplicate txids
        if (nValueTotal + nValue > nValueMax) continue;
        if (nValue != nDenomAmount) continue;

        CTxIn txin = CTxIn(txHash, out.i);
        CScript scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
        int nRounds = GetRealOutpointCoinJoinRounds(txin.prevout);

        nValueTotal += nValue;
        vecTxDSInRet.emplace_back(CTxDSIn(txin, scriptPubKey, nRounds));
        setRecentTxIds.emplace(txHash);
        LogPrint(BCLog::COINJOIN, "CWallet::%s -- hash: %s, nValue: %d.%08d\n",
                        __func__, txHash.ToString(), nValue / COIN, nValue % COIN);
    }

    LogPrint(BCLog::COINJOIN, "CWallet::%s -- setRecentTxIds.size(): %d\n", __func__, setRecentTxIds.size());

    return nValueTotal > 0;
}

static bool IsCurrentForAntiFeeSniping(interfaces::Chain& chain, const uint256& block_hash)
{
    if (chain.isInitialBlockDownload()) {
        return false;
    }
    constexpr int64_t MAX_ANTI_FEE_SNIPING_TIP_AGE = 8 * 60 * 60; // in seconds
    int64_t block_time;
    CHECK_NONFATAL(chain.findBlock(block_hash, FoundBlock().time(block_time)));
    if (block_time < (GetTime() - MAX_ANTI_FEE_SNIPING_TIP_AGE)) {
        return false;
    }
    return true;
}

/**
 * Return a height-based locktime for new transactions (uses the height of the
 * current chain tip unless we are not synced with the current chain
 */
static uint32_t GetLocktimeForNewTransaction(interfaces::Chain& chain, const uint256& block_hash, int block_height)
{
    uint32_t locktime;
    // Discourage fee sniping.
    //
    // For a large miner the value of the transactions in the best block and
    // the mempool can exceed the cost of deliberately attempting to mine two
    // blocks to orphan the current best block. By setting nLockTime such that
    // only the next block can include the transaction, we discourage this
    // practice as the height restricted and limited blocksize gives miners
    // considering fee sniping fewer options for pulling off this attack.
    //
    // A simple way to think about this is from the wallet's point of view we
    // always want the blockchain to move forward. By setting nLockTime this
    // way we're basically making the statement that we only want this
    // transaction to appear in the next block; we don't want to potentially
    // encourage reorgs by allowing transactions to appear at lower heights
    // than the next block in forks of the best chain.
    //
    // Of course, the subsidy is high enough, and transaction volume low
    // enough, that fee sniping isn't a problem yet, but by implementing a fix
    // now we ensure code won't be written that makes assumptions about
    // nLockTime that preclude a fix later.
    if (IsCurrentForAntiFeeSniping(chain, block_hash)) {
        locktime = block_height;

        // Secondly occasionally randomly pick a nLockTime even further back, so
        // that transactions that are delayed after signing for whatever reason,
        // e.g. high-latency mix networks and some CoinJoin implementations, have
        // better privacy.
        if (GetRandInt(10) == 0)
            locktime = std::max(0, (int)locktime - GetRandInt(100));
    } else {
        // If our chain is lagging behind, we can't discourage fee sniping nor help
        // the privacy of high-latency transactions. To avoid leaking a potentially
        // unique "nLockTime fingerprint", set nLockTime to a constant.
        locktime = 0;
    }
    assert(locktime < LOCKTIME_THRESHOLD);
    return locktime;
}

std::vector<CompactTallyItem> CWallet::SelectCoinsGroupedByAddresses(bool fSkipDenominated, bool fAnonymizable, bool fSkipUnconfirmed, int nMaxOupointsPerAddress) const
{
    LOCK(cs_wallet);

    isminefilter filter = ISMINE_SPENDABLE;

    // Try using the cache for already confirmed mixable inputs.
    // This should only be used if nMaxOupointsPerAddress was NOT specified.
    if(nMaxOupointsPerAddress == -1 && fAnonymizable && fSkipUnconfirmed) {
        if(fSkipDenominated && fAnonymizableTallyCachedNonDenom) {
            LogPrint(BCLog::SELECTCOINS, "SelectCoinsGroupedByAddresses - using cache for non-denom inputs %d\n", vecAnonymizableTallyCachedNonDenom.size());
            return vecAnonymizableTallyCachedNonDenom;
        }
        if(!fSkipDenominated && fAnonymizableTallyCached) {
            LogPrint(BCLog::SELECTCOINS, "SelectCoinsGroupedByAddresses - using cache for all inputs %d\n", vecAnonymizableTallyCached.size());
            return vecAnonymizableTallyCached;
        }
    }

    CAmount nSmallestDenom = CCoinJoin::GetSmallestDenomination();

    // Tally
    std::map<CTxDestination, CompactTallyItem> mapTally;
    std::set<uint256> setWalletTxesCounted;
    for (const auto& outpoint : setWalletUTXO) {

        if (!setWalletTxesCounted.emplace(outpoint.hash).second) continue;

        std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it == mapWallet.end()) continue;

        const CWalletTx& wtx = (*it).second;

        if(wtx.IsCoinBase() && wtx.GetBlocksToMaturity() > 0) continue;
        if(fSkipUnconfirmed && !wtx.IsTrusted()) continue;
        if (wtx.GetDepthInMainChain() < 0) continue;

        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
            CTxDestination txdest;
            if (!ExtractDestination(wtx.tx->vout[i].scriptPubKey, txdest)) continue;

            isminefilter mine = IsMine(txdest);
            if(!(mine & filter)) continue;

            auto itTallyItem = mapTally.find(txdest);
            if (nMaxOupointsPerAddress != -1 && itTallyItem != mapTally.end() && int64_t(itTallyItem->second.vecInputCoins.size()) >= nMaxOupointsPerAddress) continue;

            if(IsSpent(outpoint.hash, i) || IsLockedCoin(outpoint.hash, i)) continue;

            if(fSkipDenominated && CCoinJoin::IsDenominatedAmount(wtx.tx->vout[i].nValue)) continue;

            if(fAnonymizable) {
                // ignore collaterals
                if(CCoinJoin::IsCollateralAmount(wtx.tx->vout[i].nValue)) continue;
                if (fMasternodeMode && dmn_types::IsCollateralAmount(wtx.tx->vout[i].nValue)) continue;
                // ignore outputs that are 10 times smaller then the smallest denomination
                // otherwise they will just lead to higher fee / lower priority
                if(wtx.tx->vout[i].nValue <= nSmallestDenom/10) continue;
                // ignore mixed
                if (IsFullyMixed(COutPoint(outpoint.hash, i))) continue;
            }

            if (itTallyItem == mapTally.end()) {
                itTallyItem = mapTally.emplace(txdest, CompactTallyItem()).first;
                itTallyItem->second.txdest = txdest;
            }
            itTallyItem->second.nAmount += wtx.tx->vout[i].nValue;
            itTallyItem->second.vecInputCoins.emplace_back(wtx.tx, i);
        }
    }

    // construct resulting vector
    // NOTE: vecTallyRet is "sorted" by txdest (i.e. address), just like mapTally
    std::vector<CompactTallyItem> vecTallyRet;
    for (const auto& item : mapTally) {
        if(fAnonymizable && item.second.nAmount < nSmallestDenom) continue;
        vecTallyRet.push_back(item.second);
    }

    // Cache already confirmed mixable entries for later use.
    // This should only be used if nMaxOupointsPerAddress was NOT specified.
    if(nMaxOupointsPerAddress == -1 && fAnonymizable && fSkipUnconfirmed) {
        if(fSkipDenominated) {
            vecAnonymizableTallyCachedNonDenom = vecTallyRet;
            fAnonymizableTallyCachedNonDenom = true;
        } else {
            vecAnonymizableTallyCached = vecTallyRet;
            fAnonymizableTallyCached = true;
        }
    }

    // debug
    if (LogAcceptCategory(BCLog::SELECTCOINS)) {
        std::string strMessage = "SelectCoinsGroupedByAddresses - vecTallyRet:\n";
        for (const auto& item : vecTallyRet)
            strMessage += strprintf("  %s %f\n", EncodeDestination(item.txdest), float(item.nAmount)/COIN);
        LogPrint(BCLog::SELECTCOINS, "%s", strMessage); /* Continued */
    }

    return vecTallyRet;
}

bool CWallet::SelectDenominatedAmounts(CAmount nValueMax, std::set<CAmount>& setAmountsRet) const
{
    LOCK(cs_wallet);

    CAmount nValueTotal{0};
    setAmountsRet.clear();

    std::vector<COutput> vCoins;
    CCoinControl coin_control;
    coin_control.nCoinType = CoinType::ONLY_READY_TO_MIX;
    AvailableCoins(vCoins, true, &coin_control);
    // larger denoms first
    std::sort(vCoins.rbegin(), vCoins.rend(), CompareByPriority());

    for (const auto& out : vCoins) {
        CAmount nValue = out.tx->tx->vout[out.i].nValue;
        if (nValueTotal + nValue <= nValueMax) {
            nValueTotal += nValue;
            setAmountsRet.emplace(nValue);
        }
    }

    return nValueTotal >= CCoinJoin::GetSmallestDenomination();
}

int CWallet::CountInputsWithAmount(CAmount nInputAmount) const
{
    CAmount nTotal = 0;

    LOCK(cs_wallet);

    for (const auto& outpoint : setWalletUTXO) {
        const auto it = mapWallet.find(outpoint.hash);
        if (it == mapWallet.end()) continue;
        if (it->second.tx->vout[outpoint.n].nValue != nInputAmount) continue;
        if (it->second.GetDepthInMainChain() < 0) continue;

        nTotal++;
    }

    return nTotal;
}

bool CWallet::HasCollateralInputs(bool fOnlyConfirmed) const
{
    LOCK(cs_wallet);

    std::vector<COutput> vCoins;
    CCoinControl coin_control;
    coin_control.nCoinType = CoinType::ONLY_COINJOIN_COLLATERAL;
    AvailableCoins(vCoins, fOnlyConfirmed, &coin_control);

    return !vCoins.empty();
}

bool CWallet::GetBudgetSystemCollateralTX(CTransactionRef& tx, uint256 hash, CAmount amount, const COutPoint& outpoint)
{
    CScript scriptChange;
    scriptChange << OP_RETURN << ToByteVector(hash);

    CAmount nFeeRet = 0;
    int nChangePosRet = -1;
    bilingual_str error;
    std::vector< CRecipient > vecSend;
    vecSend.push_back((CRecipient){scriptChange, amount, false});

    CCoinControl coinControl;
    if (!outpoint.IsNull()) {
        coinControl.Select(outpoint);
    }
    bool success = CreateTransaction(vecSend, tx, nFeeRet, nChangePosRet, error, coinControl);
    if(!success){
        WalletLogPrintf("CWallet::GetBudgetSystemCollateralTX -- Error: %s\n", error.original);
        return false;
    }

    return true;
}

bool CWallet::CreateTransaction(const std::vector<CRecipient>& vecSend, CTransactionRef& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, const CCoinControl& coin_control, bool sign, int nExtraPayloadSize)
{
    CAmount nValue = 0;
    ReserveDestination reservedest(this);
    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    for (const auto& recipient : vecSend)
    {
        if (nValue < 0 || recipient.nAmount < 0)
        {
            error = _("Transaction amounts must not be negative");
            return false;
        }
        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;
    }
    if (vecSend.empty())
    {
        error = _("Transaction must have at least one recipient");
        return false;
    }

    CMutableTransaction txNew;
    FeeCalculation feeCalc;
    CFeeRate discard_rate = coin_control.m_discard_feerate ? *coin_control.m_discard_feerate : GetDiscardRate(*this);
    int nBytes{0};
    {
        std::vector<CInputCoin> vecCoins;
        LOCK(cs_wallet);
        txNew.nLockTime = GetLocktimeForNewTransaction(chain(), GetLastBlockHash(), GetLastBlockHeight());
        {
            CAmount nAmountAvailable{0};
            std::vector<COutput> vAvailableCoins;
            AvailableCoins(vAvailableCoins, true, &coin_control, 1, MAX_MONEY, MAX_MONEY, 0);
            CoinSelectionParams coin_selection_params; // Parameters for coin selection, init with dummy
            coin_selection_params.use_bnb = false; // never use BnB

            for (auto out : vAvailableCoins) {
                if (out.fSpendable) {
                    nAmountAvailable += out.tx->tx->vout[out.i].nValue;
                }
            }

            // Create change script that will be used if we need change
            // TODO: pass in scriptChange instead of reservedest so
            // change transaction isn't always pay-to-bitcoin-address
            CScript scriptChange;

            // coin control: send change to custom address
            if (!std::get_if<CNoDestination>(&coin_control.destChange)) {
                scriptChange = GetScriptForDestination(coin_control.destChange);
            } else { // no coin control: send change to newly generated address
                // Note: We use a new key here to keep it from being obvious which side is the change.
                //  The drawback is that by not reusing a previous key, the change may be lost if a
                //  backup is restored, if the backup doesn't have the new private key for the change.
                //  If we reused the old key, it would be possible to add code to look for and
                //  rediscover unknown transactions that were written with keys of ours to recover
                //  post-backup change.

                // Reserve a new key pair from key pool
                if (!CanGetAddresses(true)) {
                    error = _("Can't generate a change-address key. No keys in the internal keypool and can't generate any keys.");
                    return false;
                }
                CTxDestination dest;
                bool ret = reservedest.GetReservedDestination(dest, true);
                if (!ret)
                {
                    error = _("Keypool ran out, please call keypoolrefill first");
                    return false;
                }

                scriptChange = GetScriptForDestination(dest);
            }

            nFeeRet = 0;
            bool pick_new_inputs = true;
            CAmount nValueIn = 0;
            CAmount nAmountToSelectAdditional{0};
            // Start with nAmountToSelectAdditional=0 and loop until there is enough to cover the request + fees, try it 500 times.
            int nMaxTries = 500;
            while (--nMaxTries > 0)
            {
                nChangePosInOut = std::numeric_limits<int>::max();
                txNew.vin.clear();
                txNew.vout.clear();
                bool fFirst = true;

                CAmount nValueToSelect = nValue;
                if (nSubtractFeeFromAmount == 0) {
                    assert(nAmountToSelectAdditional >= 0);
                    nValueToSelect += nAmountToSelectAdditional;
                }
                // vouts to the payees
                for (const auto& recipient : vecSend)
                {
                    CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

                    if (recipient.fSubtractFeeFromAmount)
                    {
                        assert(nSubtractFeeFromAmount != 0);
                        txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

                        if (fFirst) // first receiver pays the remainder not divisible by output count
                        {
                            fFirst = false;
                            txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
                        }
                    }

                    if (IsDust(txout, chain().relayDustFee()))
                    {
                        if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
                        {
                            if (txout.nValue < 0)
                                error = _("The transaction amount is too small to pay the fee");
                            else
                                error = _("The transaction amount is too small to send after the fee has been deducted");
                        }
                        else
                            error = _("Transaction amount too small");
                        return false;
                    }
                    txNew.vout.push_back(txout);
                }

                // Choose coins to use
                bool bnb_used = false;
                if (pick_new_inputs) {
                    nValueIn = 0;
                    std::set<CInputCoin> setCoinsTmp;
                    if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoinsTmp, nValueIn, coin_control, coin_selection_params, bnb_used)) {
                        if (coin_control.nCoinType == CoinType::ONLY_NONDENOMINATED) {
                            error = _("Unable to locate enough non-denominated funds for this transaction.");
                        } else if (coin_control.nCoinType == CoinType::ONLY_FULLY_MIXED) {
                            error = _("Unable to locate enough mixed funds for this transaction.");
                            error = error + Untranslated(" ") + strprintf(_("%s uses exact denominated amounts to send funds, you might simply need to mix some more coins."), gCoinJoinName);
                        } else if (nValueIn < nValueToSelect) {
                            error = _("Insufficient funds.");
                        }
                        return false;
                    }
                    vecCoins.assign(setCoinsTmp.begin(), setCoinsTmp.end());
                }

                // Fill vin
                //
                // Note how the sequence number is set to max()-1 so that the
                // nLockTime set above actually works.
                txNew.vin.clear();
                for (const auto& coin : vecCoins) {
                    txNew.vin.emplace_back(coin.outpoint, CScript(), CTxIn::SEQUENCE_FINAL - 1);
                }

                auto calculateFee = [&](CAmount& nFee) -> bool {
                    AssertLockHeld(cs_wallet);
                    nBytes = CalculateMaximumSignedTxSize(CTransaction(txNew), this, coin_control.fAllowWatchOnly);
                    if (nBytes < 0) {
                        error = _("Signing transaction failed");
                        return false;
                    }

                    if (nExtraPayloadSize != 0) {
                        // account for extra payload in fee calculation
                        nBytes += GetSizeOfCompactSize(nExtraPayloadSize) + nExtraPayloadSize;
                    }

                    if (nBytes > MAX_STANDARD_TX_SIZE) {
                        // Do not create oversized transactions (bad-txns-oversize).
                        error = _("Transaction too large");
                        return false;
                    }

                    // Remove scriptSigs to eliminate the fee calculation dummy signatures
                    for (auto& txin : txNew.vin) {
                        txin.scriptSig = CScript();
                    }

                    nFee = GetMinimumFee(*this, nBytes, coin_control, &feeCalc);

                    return true;
                };

                if (!calculateFee(nFeeRet)) {
                    return false;
                }

                CTxOut newTxOut;
                const CAmount nAmountLeft = nValueIn - nValue;
                auto getChange = [&]() {
                    if (nSubtractFeeFromAmount > 0) {
                        return nAmountLeft;
                    } else {
                        return nAmountLeft - nFeeRet;
                    }
                };

                if (getChange() > 0)
                {
                    //over pay for denominated transactions
                    if (coin_control.nCoinType == CoinType::ONLY_FULLY_MIXED) {
                        nChangePosInOut = -1;
                        nFeeRet += getChange();
                    } else {
                        // Fill a vout to ourself with zero amount until we know the correct change
                        newTxOut = CTxOut(0, scriptChange);
                        txNew.vout.push_back(newTxOut);

                        // Calculate the fee with the change output added, store the
                        // current fee to reset it in case the remainder is dust and we
                        // don't need to fee with change output added.
                        CAmount nFeePrev = nFeeRet;
                        if (!calculateFee(nFeeRet)) {
                            return false;
                        }

                        // Remove the change output again, it will be added later again if required
                        txNew.vout.pop_back();

                        // Set the change amount properly
                        newTxOut.nValue = getChange();

                        // Never create dust outputs; if we would, just
                        // add the dust to the fee.
                        if (IsDust(newTxOut, discard_rate))
                        {
                            nFeeRet = nFeePrev;
                            nChangePosInOut = -1;
                            nFeeRet += getChange();
                        }
                        else
                        {
                            if (nChangePosRequest == -1)
                            {
                                // Insert change txn at random position:
                                nChangePosInOut = GetRandInt(txNew.vout.size()+1);
                            }
                            else if ((unsigned int)nChangePosRequest > txNew.vout.size())
                            {
                                error = _("Change index out of range");
                                return false;
                            } else {
                                nChangePosInOut = nChangePosRequest;
                            }

                            std::vector<CTxOut>::iterator position = txNew.vout.begin()+nChangePosInOut;
                            txNew.vout.insert(position, newTxOut);
                        }
                    }
                } else {
                    nChangePosInOut = -1;
                }

                if (getChange() < 0) {
                    if (nSubtractFeeFromAmount == 0) {
                        // nValueIn is not enough to cover nValue + nFeeRet. Add the missing amount abs(nChange) to the fee
                        // and try to select other inputs in the next loop step to cover the full required amount.
                        nAmountToSelectAdditional += abs(getChange());
                    } else if (nAmountToSelectAdditional > 0 && nValueToSelect == nAmountAvailable) {
                        // We tried selecting more and failed. We have no extra funds left,
                        // so just add 1 duff to fail in the next loop step with a correct reason
                        nAmountToSelectAdditional += 1;
                    }
                    continue;
                }

                // If no specific change position was requested, apply BIP69
                if (nChangePosRequest == -1) {
                    std::sort(vecCoins.begin(), vecCoins.end(), CompareInputCoinBIP69());
                    std::sort(txNew.vin.begin(), txNew.vin.end(), CompareInputBIP69());
                    std::sort(txNew.vout.begin(), txNew.vout.end(), CompareOutputBIP69());

                    // If there was a change output added before, we must update its position now
                    if (nChangePosInOut != -1) {
                        int i = 0;
                        for (const CTxOut& txOut : txNew.vout)
                        {
                            if (txOut == newTxOut)
                            {
                                nChangePosInOut = i;
                                break;
                            }
                            i++;
                        }
                    }
                }

                if (feeCalc.reason == FeeReason::FALLBACK && !m_allow_fallback_fee) {
                    // eventually allow a fallback fee
                    error = _("Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable -fallbackfee.");
                    return false;
                }

                if (nAmountLeft == nFeeRet) {
                    // We either added the change amount to nFeeRet because the change amount was considered
                    // to be dust or the input exactly matches output + fee.
                    // Either way, we used the total amount of the inputs we picked and the transaction is ready.
                    break;
                }

                // We have a change output and we don't need to subtruct fees, which means the transaction is ready.
                if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                    break;
                }

                // If subtracting fee from recipients, we now know what fee we
                // need to subtract, we have no reason to reselect inputs
                if (nSubtractFeeFromAmount > 0) {
                    // If we are in here the second time it means we already subtracted the fee from the
                    // output(s) and there weren't any issues while doing that. So the transaction is ready now
                    // and we can break.
                    if (!pick_new_inputs) {
                        break;
                    }
                    pick_new_inputs = false;
                }
            }

            if (nMaxTries == 0) {
                error = _("Exceeded max tries.");
                return false;
            }
        }

        // Make sure change position was updated one way or another
        assert(nChangePosInOut != std::numeric_limits<int>::max());

        if (sign && !SignTransaction(txNew)) {
            error = _("Signing transaction failed");
            return false;
        }

        // Return the constructed transaction data.
        tx = MakeTransactionRef(std::move(txNew));
    }

    if (nFeeRet > m_default_max_tx_fee) {
        error = TransactionErrorString(TransactionError::MAX_FEE_EXCEEDED);
        return false;
    }

    if (gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        if (!chain().checkChainLimits(tx)) {
            error = _("Transaction has too long of a mempool chain");
            return false;
        }
    }

    // Before we return success, we assume any change key will be used to prevent
    // accidental re-use.
    reservedest.KeepDestination();

    WalletLogPrintf("Fee Calculation: Fee:%d Bytes:%u Tgt:%d (requested %d) Reason:\"%s\" Decay %.5f: Estimation: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out) Fail: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out)\n",
              nFeeRet, nBytes, feeCalc.returnedTarget, feeCalc.desiredTarget, StringForFeeReason(feeCalc.reason), feeCalc.est.decay,
              feeCalc.est.pass.start, feeCalc.est.pass.end,
              (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) > 0.0 ? 100 * feeCalc.est.pass.withinTarget / (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) : 0.0,
              feeCalc.est.pass.withinTarget, feeCalc.est.pass.totalConfirmed, feeCalc.est.pass.inMempool, feeCalc.est.pass.leftMempool,
              feeCalc.est.fail.start, feeCalc.est.fail.end,
              (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) > 0.0 ? 100 * feeCalc.est.fail.withinTarget / (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) : 0.0,
              feeCalc.est.fail.withinTarget, feeCalc.est.fail.totalConfirmed, feeCalc.est.fail.inMempool, feeCalc.est.fail.leftMempool);
    return true;
}

void CWallet::CommitTransaction(CTransactionRef tx, mapValue_t mapValue, std::vector<std::pair<std::string, std::string>> orderForm)
{
    LOCK(cs_wallet);

    CWalletTx wtxNew(this, std::move(tx));
    wtxNew.mapValue = std::move(mapValue);
    wtxNew.vOrderForm = std::move(orderForm);
    wtxNew.fTimeReceivedIsTxTime = true;
    wtxNew.fFromMe = true;

    WalletLogPrintf("CommitTransaction:\n%s", wtxNew.tx->ToString()); /* Continued */
    // Add tx to wallet, because if it has change it's also ours,
    // otherwise just for transaction history.
    AddToWallet(wtxNew);

    // Notify that old coins are spent
    std::set<uint256> updated_hahes;
    for (const CTxIn& txin : wtxNew.tx->vin){
        // notify only once
        if(updated_hahes.find(txin.prevout.hash) != updated_hahes.end()) continue;

        CWalletTx &coin = mapWallet.at(txin.prevout.hash);
        coin.BindWallet(this);
        NotifyTransactionChanged(this, txin.prevout.hash, CT_UPDATED);
        updated_hahes.insert(txin.prevout.hash);
    }
    // Get the inserted-CWalletTx from mapWallet so that the
    // fInMempool flag is cached properly
    CWalletTx& wtx = mapWallet.at(wtxNew.GetHash());

    if (!fBroadcastTransactions) {
        // Don't submit tx to the mempool
        return;
    }

    std::string err_string;
    if (!wtx.SubmitMemoryPoolAndRelay(err_string, true)) {
        WalletLogPrintf("CommitTransaction(): Transaction cannot be broadcast immediately, %s\n", err_string);
        // TODO: if we expect the failure to be long term or permanent, instead delete wtx from the wallet and return failure.
    }
}

DBErrors CWallet::LoadWallet(bool& fFirstRunRet)
{
    LOCK(cs_wallet);

    fFirstRunRet = false;
    DBErrors nLoadWalletRet = WalletBatch(*database).LoadWallet(this);
    if (nLoadWalletRet == DBErrors::NEED_REWRITE)
    {
        if (database->Rewrite("\x04pool"))
        {
            for (const auto& spk_man_pair : m_spk_managers) {
                spk_man_pair.second->RewriteDB();
            }
            nKeysLeftSinceAutoBackup = 0;
        }
    }

    // This wallet is in its first run if there are no ScriptPubKeyMans and it isn't blank or no privkeys
    fFirstRunRet = m_spk_managers.empty() && !IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && !IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET);
    if (fFirstRunRet) {
        assert(m_external_spk_managers == nullptr);
        assert(m_internal_spk_managers == nullptr);
    }

    if (HaveChain()) {
        const std::optional<int> tip_height = chain().getHeight();
        if (tip_height) {
            SetLastBlockProcessed(*tip_height, chain().getBlockHash(*tip_height));
            for (auto& pair : mapWallet) {
                for(unsigned int i = 0; i < pair.second.tx->vout.size(); ++i) {
                    if (IsMine(pair.second.tx->vout[i]) && !IsSpent(pair.first, i)) {
                        setWalletUTXO.insert(COutPoint(pair.first, i));
                    }
                }
            }
        }
    }

    InitCoinJoinSalt();

    if (nLoadWalletRet != DBErrors::LOAD_OK)
        return nLoadWalletRet;

    return DBErrors::LOAD_OK;
}

// Goes through all wallet transactions and checks if they are masternode collaterals, in which case these are locked
// This avoids accidental spending of collaterals. They can still be unlocked manually if a spend is really intended.
void CWallet::AutoLockMasternodeCollaterals()
{
    auto mnList = deterministicMNManager->GetListAtChainTip();

    LOCK(cs_wallet);
    for (const auto& pair : mapWallet) {
        for (unsigned int i = 0; i < pair.second.tx->vout.size(); ++i) {
            if (IsMine(pair.second.tx->vout[i]) && !IsSpent(pair.first, i)) {
                if (deterministicMNManager->IsProTxWithCollateral(pair.second.tx, i) || mnList.HasMNByCollateral(COutPoint(pair.first, i))) {
                    LockCoin(COutPoint(pair.first, i));
                }
            }
        }
    }
}

DBErrors CWallet::ZapSelectTx(std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut)
{
    AssertLockHeld(cs_wallet);

    WalletLogPrintf("ZapSelectTx started for %d transactions...\n", vHashIn.size());

    DBErrors nZapSelectTxRet = WalletBatch(*database).ZapSelectTx(vHashIn, vHashOut);
    for (uint256 hash : vHashOut) {
        const auto& it = mapWallet.find(hash);
        wtxOrdered.erase(it->second.m_it_wtxOrdered);
        mapWallet.erase(it);
        NotifyTransactionChanged(this, hash, CT_DELETED);
    }

    if (nZapSelectTxRet == DBErrors::NEED_REWRITE)
    {
        if (database->Rewrite("\x04pool"))
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
    {
        LOCK(cs_wallet);
        std::map<CTxDestination, CAddressBookData>::iterator mi = m_address_book.find(address);
        fUpdated = (mi != m_address_book.end() && !mi->second.IsChange());
        m_address_book[address].SetLabel(strName);
        if (!strPurpose.empty()) /* update purpose only if requested */
            m_address_book[address].purpose = strPurpose;
    }
    NotifyAddressBookChanged(this, address, strName, IsMine(address) != ISMINE_NO,
                             strPurpose, (fUpdated ? CT_UPDATED : CT_NEW) );
    if (!strPurpose.empty() && !batch.WritePurpose(EncodeDestination(address), strPurpose))
        return false;
    return batch.WriteName(EncodeDestination(address), strName);
}

bool CWallet::SetAddressBook(const CTxDestination& address, const std::string& strName, const std::string& strPurpose)
{
    WalletBatch batch(*database);
    return SetAddressBookWithDB(batch, address, strName, strPurpose);
}

bool CWallet::DelAddressBook(const CTxDestination& address)
{
    // If we want to delete receiving addresses, we need to take care that DestData "used" (and possibly newer DestData) gets preserved (and the "deleted" address transformed into a change entry instead of actually being deleted)
    // NOTE: This isn't a problem for sending addresses because they never have any DestData yet!
    // When adding new DestData, it should be considered here whether to retain or delete it (or move it?).
    if (IsMine(address)) {
        WalletLogPrintf("%s called with IsMine address, NOT SUPPORTED. Please report this bug! %s\n", __func__, PACKAGE_BUGREPORT);
        return false;
    }

    {
        LOCK(cs_wallet);

        // Delete destdata tuples associated with address
        std::string strAddress = EncodeDestination(address);
        for (const std::pair<const std::string, std::string> &item : m_address_book[address].destdata)
        {
            WalletBatch(*database).EraseDestData(strAddress, item.first);
        }
        m_address_book.erase(address);
    }

    NotifyAddressBookChanged(this, address, "", IsMine(address) != ISMINE_NO, "", CT_DELETED);

    WalletBatch(*database).ErasePurpose(EncodeDestination(address));
    return WalletBatch(*database).EraseName(EncodeDestination(address));
}

size_t CWallet::KeypoolCountExternalKeys() const
{
    AssertLockHeld(cs_wallet);

    unsigned int count = 0;
    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        count += spk_man->KeypoolCountExternalKeys();
    }

    return count;
}

size_t CWallet::KeypoolCountInternalKeys() const
{
    AssertLockHeld(cs_wallet);

    unsigned int count = 0;
    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        count += spk_man->KeypoolCountInternalKeys();
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

bool CWallet::GetNewDestination(const std::string label, CTxDestination& dest, std::string& error)
{
    error.clear();
    bool result = false;

    LOCK(cs_wallet);
    auto spk_man = GetScriptPubKeyMan(false /* internal */);
    if (spk_man) {
        spk_man->TopUp();
        result = spk_man->GetNewDestination(dest, error);
    }
    if (result) {
        SetAddressBook(dest, label, "receive");
    }

    return result;
}

bool CWallet::GetNewChangeDestination(CTxDestination& dest, std::string& error)
{
    LOCK(cs_wallet);
    error.clear();

    ReserveDestination reservedest(this);
    if (!reservedest.GetReservedDestination(dest, true)) {
        error = "Error: Keypool ran out, please call keypoolrefill first";
        return false;
    }

    reservedest.KeepDestination();
    return true;
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

std::map<CTxDestination, CAmount> CWallet::GetAddressBalances() const
{
    std::map<CTxDestination, CAmount> balances;

    {
        LOCK(cs_wallet);
        std::set<uint256> trusted_parents;
        for (const auto& walletEntry : mapWallet)
        {
            const CWalletTx *pcoin = &walletEntry.second;

            if (!pcoin->IsTrusted(trusted_parents))
                continue;

            if (pcoin->IsImmatureCoinBase())
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if ((nDepth < (pcoin->IsFromMe(ISMINE_ALL) ? 0 : 1)) && !pcoin->IsLockedByInstantSend())
                continue;

            for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++)
            {
                CTxDestination addr;
                if (!IsMine(pcoin->tx->vout[i]))
                    continue;
                if(!ExtractDestination(pcoin->tx->vout[i].scriptPubKey, addr))
                    continue;

                CAmount n = IsSpent(walletEntry.first, i) ? 0 : pcoin->tx->vout[i].nValue;

                balances[addr] += n;
            }
        }
    }

    return balances;
}

std::set< std::set<CTxDestination> > CWallet::GetAddressGroupings() const
{
    AssertLockHeld(cs_wallet);
    std::set< std::set<CTxDestination> > groupings;
    std::set<CTxDestination> grouping;

    for (const auto& walletEntry : mapWallet)
    {
        const CWalletTx *pcoin = &walletEntry.second;

        if (pcoin->tx->vin.size() > 0)
        {
            bool any_mine = false;
            // group all input addresses with each other
            for (const CTxIn& txin : pcoin->tx->vin)
            {
                CTxDestination address;
                if(!IsMine(txin)) /* If this input isn't mine, ignore it */
                    continue;
                if(!ExtractDestination(mapWallet.at(txin.prevout.hash).tx->vout[txin.prevout.n].scriptPubKey, address))
                    continue;
                grouping.insert(address);
                any_mine = true;
            }

            // group change with input addresses
            if (any_mine)
            {
               for (const CTxOut& txout : pcoin->tx->vout)
                   if (IsChange(txout))
                   {
                       CTxDestination txoutAddr;
                       if(!ExtractDestination(txout.scriptPubKey, txoutAddr))
                           continue;
                       grouping.insert(txoutAddr);
                   }
            }
            if (grouping.size() > 0)
            {
                groupings.insert(grouping);
                grouping.clear();
            }
        }

        // group lone addrs by themselves
        for (const auto& txout : pcoin->tx->vout)
            if (IsMine(txout))
            {
                CTxDestination address;
                if(!ExtractDestination(txout.scriptPubKey, address))
                    continue;
                grouping.insert(address);
                groupings.insert(grouping);
                grouping.clear();
            }
    }

    std::set< std::set<CTxDestination>* > uniqueGroupings; // a set of pointers to groups of addresses
    std::map< CTxDestination, std::set<CTxDestination>* > setmap;  // map addresses to the unique group containing it
    for (std::set<CTxDestination> _grouping : groupings)
    {
        // make a set of all the groups hit by this new group
        std::set< std::set<CTxDestination>* > hits;
        std::map< CTxDestination, std::set<CTxDestination>* >::iterator it;
        for (const CTxDestination& address : _grouping)
            if ((it = setmap.find(address)) != setmap.end())
                hits.insert((*it).second);

        // merge all hit groups into a new single group and delete old groups
        std::set<CTxDestination>* merged = new std::set<CTxDestination>(_grouping);
        for (std::set<CTxDestination>* hit : hits)
        {
            merged->insert(hit->begin(), hit->end());
            uniqueGroupings.erase(hit);
            delete hit;
        }
        uniqueGroupings.insert(merged);

        // update setmap
        for (const CTxDestination& element : *merged)
            setmap[element] = merged;
    }

    std::set< std::set<CTxDestination> > ret;
    for (const std::set<CTxDestination>* uniqueGrouping : uniqueGroupings)
    {
        ret.insert(*uniqueGrouping);
        delete uniqueGrouping;
    }

    return ret;
}

std::set<CTxDestination> CWallet::GetLabelAddresses(const std::string& label) const
{
    LOCK(cs_wallet);
    std::set<CTxDestination> result;
    for (const std::pair<const CTxDestination, CAddressBookData>& item : m_address_book)
    {
        if (item.second.IsChange()) continue;
        const CTxDestination& address = item.first;
        const std::string& strName = item.second.GetLabel();
        if (strName == label)
            result.insert(address);
    }
    return result;
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
        if (!m_spk_man->GetReservedDestination(fInternalIn, address, nIndex, keypool)) {
            return false;
        }
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

void CWallet::LockCoin(const COutPoint& output)
{
    AssertLockHeld(cs_wallet);
    setLockedCoins.insert(output);
    std::map<uint256, CWalletTx>::iterator it = mapWallet.find(output.hash);
    if (it != mapWallet.end()) it->second.MarkDirty(); // recalculate all credits for this tx

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::UnlockCoin(const COutPoint& output)
{
    AssertLockHeld(cs_wallet);
    setLockedCoins.erase(output);
    std::map<uint256, CWalletTx>::iterator it = mapWallet.find(output.hash);
    if (it != mapWallet.end()) it->second.MarkDirty(); // recalculate all credits for this tx

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::UnlockAllCoins()
{
    AssertLockHeld(cs_wallet);
    setLockedCoins.clear();
}

bool CWallet::IsLockedCoin(uint256 hash, unsigned int n) const
{
    AssertLockHeld(cs_wallet);
    COutPoint outpt(hash, n);

    return (setLockedCoins.count(outpt) > 0);
}

void CWallet::ListLockedCoins(std::vector<COutPoint>& vOutpts) const
{
    AssertLockHeld(cs_wallet);
    for (std::set<COutPoint>::iterator it = setLockedCoins.begin();
         it != setLockedCoins.end(); it++) {
        COutPoint outpt = (*it);
        vOutpts.push_back(outpt);
    }
}

void CWallet::ListProTxCoins(std::vector<COutPoint>& vOutpts) const
{
    auto mnList = deterministicMNManager->GetListAtChainTip();

    AssertLockHeld(cs_wallet);
    for (const auto &o : setWalletUTXO) {
        auto it = mapWallet.find(o.hash);
        if (it != mapWallet.end()) {
            const auto &p = it->second;
            if (deterministicMNManager->IsProTxWithCollateral(p.tx, o.n) || mnList.HasMNByCollateral(o)) {
                vOutpts.emplace_back(o);
            }
        }
    }
}

/** @} */ // end of Actions

void CWallet::GetKeyBirthTimes(std::map<CKeyID, int64_t>& mapKeyBirth) const {
    AssertLockHeld(cs_wallet);
    mapKeyBirth.clear();

    LegacyScriptPubKeyMan* spk_man = GetLegacyScriptPubKeyMan();
    assert(spk_man != nullptr);
    LOCK(spk_man->cs_KeyStore);

    // get birth times for keys with metadata
    for (const auto& entry : spk_man->mapKeyMetadata) {
        if (entry.second.nCreateTime) {
            mapKeyBirth[entry.first] = entry.second.nCreateTime;
        }
    }

    // map in which we'll infer heights of other keys
    std::map<CKeyID, const CWalletTx::Confirmation*> mapKeyFirstBlock;
    CWalletTx::Confirmation max_confirm;
    max_confirm.block_height = GetLastBlockHeight() > 144 ? GetLastBlockHeight() - 144 : 0; // the tip can be reorganized; use a 144-block safety margin
    CHECK_NONFATAL(chain().findAncestorByHeight(GetLastBlockHash(), max_confirm.block_height, FoundBlock().hash(max_confirm.hashBlock)));
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
        if (wtx.m_confirm.status == CWalletTx::CONFIRMED) {
            // ... which are already in a block
            for (const CTxOut &txout : wtx.tx->vout) {
                // iterate over all their outputs
                for (const auto &keyid : GetAffectedKeys(txout.scriptPubKey, *spk_man)) {
                    // ... and all their affected keys
                    auto rit = mapKeyFirstBlock.find(keyid);
                    if (rit != mapKeyFirstBlock.end() && wtx.m_confirm.block_height < rit->second->block_height) {
                        rit->second = &wtx.m_confirm;
                    }
                }
            }
        }
    }

    // Extract block timestamps for those keys
    for (const auto& entry : mapKeyFirstBlock) {
        int64_t block_time;
        CHECK_NONFATAL(chain().findBlock(entry.second->hashBlock, FoundBlock().time(block_time)));
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
unsigned int CWallet::ComputeTimeSmart(const CWalletTx& wtx) const
{
    unsigned int nTimeSmart = wtx.nTimeReceived;
    if (!wtx.isUnconfirmed() && !wtx.isAbandoned()) {
        int64_t blocktime;
        if (chain().findBlock(wtx.m_confirm.hashBlock, FoundBlock().time(blocktime))) {
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
        } else {
            WalletLogPrintf("%s: found %s in block %s not in index\n", __func__, wtx.GetHash().ToString(), wtx.m_confirm.hashBlock.ToString());
        }
    }
    return nTimeSmart;
}

bool CWallet::AddDestData(WalletBatch& batch, const CTxDestination &dest, const std::string &key, const std::string &value)
{
    if (std::get_if<CNoDestination>(&dest))
        return false;

    m_address_book[dest].destdata.insert(std::make_pair(key, value));
    return batch.WriteDestData(EncodeDestination(dest), key, value);
}

bool CWallet::EraseDestData(WalletBatch& batch, const CTxDestination &dest, const std::string &key)
{
    if (!m_address_book[dest].destdata.erase(key))
        return false;
    return batch.EraseDestData(EncodeDestination(dest), key);
}

void CWallet::LoadDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    m_address_book[dest].destdata.insert(std::make_pair(key, value));
}

bool CWallet::GetDestData(const CTxDestination &dest, const std::string &key, std::string *value) const
{
    std::map<CTxDestination, CAddressBookData>::const_iterator i = m_address_book.find(dest);
    if(i != m_address_book.end())
    {
        CAddressBookData::StringMap::const_iterator j = i->second.destdata.find(key);
        if(j != i->second.destdata.end())
        {
            if(value)
                *value = j->second;
            return true;
        }
    }
    return false;
}

std::vector<std::string> CWallet::GetDestValues(const std::string& prefix) const
{
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

std::unique_ptr<WalletDatabase> MakeWalletDatabase(const std::string& name, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error_string)
{
    // Do some checking on wallet path. It should be either a:
    //
    // 1. Path where a directory can be created.
    // 2. Path to an existing directory.
    // 3. Path to a symlink to a directory.
    // 4. For backwards compatibility, the name of a data file in -walletdir.
    const fs::path& wallet_path = fs::absolute(name, GetWalletDir());
    fs::file_type path_type = fs::symlink_status(wallet_path).type();
    if (!(path_type == fs::file_not_found || path_type == fs::directory_file ||
          (path_type == fs::symlink_file && fs::is_directory(wallet_path)) ||
          (path_type == fs::regular_file && fs::path(name).filename() == name))) {
        error_string = Untranslated(strprintf(
              "Invalid -wallet path '%s'. -wallet path should point to a directory where wallet.dat and "
              "database/log.?????????? files can be stored, a location where such a directory could be created, "
              "or (for backwards compatibility) the name of an existing data file in -walletdir (%s)",
              name, GetWalletDir()));
        status = DatabaseStatus::FAILED_BAD_PATH;
        return nullptr;
    }

    return MakeDatabase(wallet_path, options, status, error_string);
}

std::shared_ptr<CWallet> CWallet::Create(interfaces::Chain& chain, const std::string& name, std::unique_ptr<WalletDatabase> database, uint64_t wallet_creation_flags, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    const std::string& walletFile = database->Filename();

    chain.initMessage(_("Loading wallet...").translated);

    int64_t nStart = GetTimeMillis();
    bool fFirstRun = true;
    // TODO: Can't use std::make_shared because we need a custom deleter but
    // should be possible to use std::allocate_shared.
    std::shared_ptr<CWallet> walletInstance(new CWallet(&chain, name, std::move(database)), ReleaseWallet);
    if (!walletInstance->AutoBackupWallet(walletFile, error, warnings) && !error.original.empty()) {
        return nullptr;
    }
    DBErrors nLoadWalletRet = walletInstance->LoadWallet(fFirstRun);
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

    if (fFirstRun)
    {
        walletInstance->SetMaxVersion(FEATURE_LATEST);
        walletInstance->SetWalletFlags(wallet_creation_flags, false);

        // Always create LegacyScriptPubKeyMan for now
        walletInstance->SetupLegacyScriptPubKeyMan();

        if (!(wallet_creation_flags & (WALLET_FLAG_DISABLE_PRIVATE_KEYS | WALLET_FLAG_BLANK_WALLET))) {
            // Create new HD chain
            if (gArgs.GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET) && !walletInstance->IsHDEnabled()) {
                std::string strSeed = gArgs.GetArg("-hdseed", "not hex");

                if (gArgs.IsArgSet("-hdseed") && IsHex(strSeed)) {
                    CHDChain newHdChain;
                    std::vector<unsigned char> vchSeed = ParseHex(strSeed);
                    if (!newHdChain.SetSeed(SecureVector(vchSeed.begin(), vchSeed.end()), true)) {
                        error = strprintf(_("%s failed"), "SetSeed");
                        return nullptr;
                    }
                    LOCK(walletInstance->cs_wallet);
                    if (auto spk_man = walletInstance->GetLegacyScriptPubKeyMan()) {
                        if (!spk_man->SetHDChainSingle(newHdChain, false)) {
                            error = strprintf(_("%s failed"), "SetHDChainSingle");
                            return nullptr;
                        }
                    }
                    // add default account
                    newHdChain.AddAccount();
                    newHdChain.Debug(__func__);
                } else {
                    if (gArgs.IsArgSet("-hdseed") && !IsHex(strSeed)) {
                        walletInstance->WalletLogPrintf("%s -- Incorrect seed, generating a random mnemonic instead\n", __func__);
                    }
                    SecureString secureMnemonic = gArgs.GetArg("-mnemonic", "").c_str();
                    SecureString secureMnemonicPassphrase = gArgs.GetArg("-mnemonicpassphrase", "").c_str();
                    LOCK(walletInstance->cs_wallet);
                    if (auto spk_man = walletInstance->GetLegacyScriptPubKeyMan()) {
                        spk_man->GenerateNewHDChain(secureMnemonic, secureMnemonicPassphrase);
                    }
                }

                // ensure this wallet.dat can only be opened by clients supporting HD
                walletInstance->WalletLogPrintf("Upgrading wallet to HD\n");
                walletInstance->SetMinVersion(FEATURE_HD);

                // clean up
                gArgs.ForceRemoveArg("hdseed");
                gArgs.ForceRemoveArg("mnemonic");
                gArgs.ForceRemoveArg("mnemonicpassphrase");
            }
        } // Otherwise, do not create a new HD chain

        // Top up the keypool
        {
            LOCK(walletInstance->cs_wallet);
            if (auto spk_man = walletInstance->GetLegacyScriptPubKeyMan()) {
                if (spk_man->CanGenerateKeys() && !spk_man->TopUp()) {
                    error = _("Unable to generate initial keys");
                    return nullptr;
                }
            }
        }

        walletInstance->chainStateFlushed(chain.getTipLocator());

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
        return NULL;
    } else if (walletInstance->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        for (auto spk_man : walletInstance->GetActiveScriptPubKeyMans()) {
            if (spk_man->HavePrivateKeys()) {
                warnings.push_back(strprintf(_("Warning: Private keys detected in wallet {%s} with disabled private keys"), walletFile));
            }
        }
    }
    else if (gArgs.IsArgSet("-usehd")) {
        bool useHD = gArgs.GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET);
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
        SetMiscWarning(_("Make sure to encrypt your wallet and delete all non-encrypted backups after you have verified that the wallet works!").translated);
    }

    if (gArgs.IsArgSet("-mintxfee")) {
        std::optional<CAmount> min_tx_fee = ParseMoney(gArgs.GetArg("-mintxfee", ""));
        if (!min_tx_fee || min_tx_fee.value() == 0) {
            error = AmountErrMsg("mintxfee", gArgs.GetArg("-mintxfee", ""));
            return nullptr;
        } else if (min_tx_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-mintxfee") + Untranslated(" ") +
                              _("This is the minimum transaction fee you pay on every transaction."));
        }

        walletInstance->m_min_fee = CFeeRate{min_tx_fee.value()};
    }

    if (gArgs.IsArgSet("-fallbackfee")) {
        std::optional<CAmount> fallback_fee = ParseMoney(gArgs.GetArg("-fallbackfee", ""));
        if (!fallback_fee) {
            error = strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'"), gArgs.GetArg("-fallbackfee", ""));
            return nullptr;
        } else if (fallback_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-fallbackfee") + Untranslated(" ") +
                              _("This is the transaction fee you may pay when fee estimates are not available."));
        }
        walletInstance->m_fallback_fee = CFeeRate{fallback_fee.value()};
    }
    // Disable fallback fee in case value was set to 0, enable if non-null value
    walletInstance->m_allow_fallback_fee = walletInstance->m_fallback_fee.GetFeePerK() != 0;

    if (gArgs.IsArgSet("-discardfee")) {
        std::optional<CAmount> discard_fee = ParseMoney(gArgs.GetArg("-discardfee", ""));
        if (!discard_fee) {
            error = strprintf(_("Invalid amount for -discardfee=<amount>: '%s'"), gArgs.GetArg("-discardfee", ""));
            return nullptr;
        } else if (discard_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-discardfee") + Untranslated(" ") +
                              _("This is the transaction fee you may discard if change is smaller than dust at this level"));
        }
        walletInstance->m_discard_rate = CFeeRate{discard_fee.value()};
    }

    if (gArgs.IsArgSet("-paytxfee")) {
        std::optional<CAmount> pay_tx_fee = ParseMoney(gArgs.GetArg("-paytxfee", ""));
        if (!pay_tx_fee) {
            error = AmountErrMsg("paytxfee", gArgs.GetArg("-paytxfee", ""));
            return nullptr;
        } else if (pay_tx_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-paytxfee") + Untranslated(" ") +
                              _("This is the transaction fee you will pay if you send a transaction."));
        }
        walletInstance->m_pay_tx_fee = CFeeRate{pay_tx_fee.value(), 1000};
        if (walletInstance->m_pay_tx_fee < chain.relayMinFee()) {
            error = strprintf(_("Invalid amount for -paytxfee=<amount>: '%s' (must be at least %s)"),
                gArgs.GetArg("-paytxfee", ""), chain.relayMinFee().ToString());
            return nullptr;
        }
    }

    if (gArgs.IsArgSet("-maxtxfee")) {
        std::optional<CAmount> max_fee = ParseMoney(gArgs.GetArg("-maxtxfee", ""));
        if (!max_fee) {
            error = AmountErrMsg("maxtxfee", gArgs.GetArg("-maxtxfee", ""));
            return nullptr;
        } else if (max_fee.value() > HIGH_MAX_TX_FEE) {
            warnings.push_back(_("-maxtxfee is set very high! Fees this large could be paid on a single transaction."));
        }
        if (CFeeRate{max_fee.value(), 1000} < chain.relayMinFee()) {
            error = strprintf(_("Invalid amount for -maxtxfee=<amount>: '%s' (must be at least the minrelay fee of %s to prevent stuck transactions)"),
                gArgs.GetArg("-maxtxfee", ""), chain.relayMinFee().ToString());
            return nullptr;
        }

        walletInstance->m_default_max_tx_fee = max_fee.value();
    }

    if (chain.relayMinFee().GetFeePerK() > HIGH_TX_FEE_PER_KB)
        warnings.push_back(AmountHighWarn("-minrelaytxfee") + Untranslated(" ") +
                    _("The wallet will avoid paying less than the minimum relay fee."));

    walletInstance->m_confirm_target = gArgs.GetArg("-txconfirmtarget", DEFAULT_TX_CONFIRM_TARGET);
    walletInstance->m_spend_zero_conf_change = gArgs.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);

    walletInstance->WalletLogPrintf("Wallet completed loading in %15dms\n", GetTimeMillis() - nStart);

    // Try to top up keypool. No-op if the wallet is locked.
    walletInstance->TopUpKeyPool();

    LOCK(walletInstance->cs_wallet);

    // Register wallet with validationinterface. It's done before rescan to avoid
    // missing block connections between end of rescan and validation subscribing.
    // Because of wallet lock being hold, block connection notifications are going to
    // be pending on the validation-side until lock release. It's likely to have
    // block processing duplicata (if rescan block range overlaps with notification one)
    // but we guarantee at least than wallet state is correct after notifications delivery.
    // This is temporary until rescan and notifications delivery are unified under same
    // interface.
    walletInstance->m_chain_notifications_handler = walletInstance->chain().handleNotifications(walletInstance);

    int rescan_height = 0;
    if (!gArgs.GetBoolArg("-rescan", false))
    {
        WalletBatch batch(*walletInstance->database);
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
        // We can't rescan beyond non-pruned blocks, stop and throw an error.
        // This might happen if a user uses an old wallet within a pruned node
        // or if they ran -disablewallet for a longer time, then decided to re-enable
        if (chain.havePruned()) {
            // Exit early and print an error.
            // If a block is pruned after this check, we will load the wallet,
            // but fail the rescan with a generic error.
            int block_height = *tip_height;
            while (block_height > 0 && chain.haveBlockOnDisk(block_height - 1) && rescan_height != block_height) {
                --block_height;
            }

            if (rescan_height != block_height) {
                error = _("Prune: last wallet synchronisation goes beyond pruned data. You need to -reindex (download the whole blockchain again in case of pruned node)");
                return nullptr;
            }
        }

        chain.initMessage(_("Rescanning...").translated);
        walletInstance->WalletLogPrintf("Rescanning last %i blocks (from block %i)...\n", *tip_height - rescan_height, rescan_height);

        // No need to read and scan block if block was created before
        // our wallet birthday (as adjusted for block time variability)
        // unless a full rescan was requested
        if (gArgs.GetArg("-rescan", 0) != 2) {
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
            if (!reserver.reserve() || (ScanResult::SUCCESS != walletInstance->ScanForWalletTransactions(chain.getBlockHash(rescan_height), rescan_height, {} /* max height */, reserver, true /* update */).status)) {
                error = _("Failed to rescan the wallet during initialization");
                return nullptr;
            }
        }
        walletInstance->chainStateFlushed(chain.getTipLocator());
        walletInstance->database->IncrementUpdateCounter();
    }

    assert(::masternodeSync != nullptr);
    coinJoinClientManagers.emplace(std::make_pair(walletInstance->GetName(), std::make_shared<CCoinJoinClientManager>(*walletInstance, *::masternodeSync)));

    {
        LOCK(cs_wallets);
        for (auto& load_wallet : g_load_wallet_fns) {
            load_wallet(interfaces::MakeWallet(walletInstance));
        }
    }

    walletInstance->SetBroadcastTransactions(gArgs.GetBoolArg("-walletbroadcast", DEFAULT_WALLETBROADCAST));

    {
        walletInstance->WalletLogPrintf("setExternalKeyPool.size() = %u\n",   walletInstance->KeypoolCountExternalKeys());
        walletInstance->WalletLogPrintf("setInternalKeyPool.size() = %u\n",   walletInstance->KeypoolCountInternalKeys());
        walletInstance->WalletLogPrintf("mapWallet.size() = %u\n",            walletInstance->mapWallet.size());
        walletInstance->WalletLogPrintf("m_address_book.size() = %u\n",  walletInstance->m_address_book.size());
        for (auto spk_man : walletInstance->GetAllScriptPubKeyMans()) {
            walletInstance->WalletLogPrintf("nTimeFirstKey = %u\n", spk_man->GetTimeFirstKey());
        }
    }

    return walletInstance;
}

bool CWallet::UpgradeWallet(int version, bilingual_str& error)
{
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
        error = Untranslated("Cannot downgrade wallet");
        return false;
    }

    SetMaxVersion(nMaxVersion);
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

bool CWallet::InitAutoBackup()
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET))
        return true;

    nWalletBackups = gArgs.GetArg("-createwalletbackups", 10);
    nWalletBackups = std::max(0, std::min(10, nWalletBackups));

    return true;
}

bool CWallet::BackupWallet(const std::string& strDest) const
{
    return database->Backup(strDest);
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

    fs::path backupsDir = GetBackupsDir();
    backupsDir.make_preferred();

    if (!fs::exists(backupsDir))
    {
        // Always create backup folder to not confuse the operating system's file browser
        WalletLogPrintf("Creating backup folder %s\n", backupsDir.string());
        if(!fs::create_directories(backupsDir)) {
            // something is wrong, we shouldn't continue until it's resolved
            error_string = strprintf(_("Wasn't able to create wallet backup folder %s!"), backupsDir.string());
            WalletLogPrintf("%s\n", error_string.translated);
            nWalletBackups = -1;
            return false;
        }
    } else if (!fs::is_directory(backupsDir)) {
        // something is wrong, we shouldn't continue until it's resolved
        error_string = strprintf(_("%s is not a valid backup folder!"), backupsDir.string());
        WalletLogPrintf("%s\n", error_string.translated);
        nWalletBackups = -1;
        return false;
    }

    // Create backup of the ...
    struct tm ts;
    time_t time_val = GetTime();
#ifdef HAVE_GMTIME_R
    gmtime_r(&time_val, &ts);
#else
    gmtime_s(&ts, &time_val);
#endif
    std::string dateTimeStr = strprintf(".%04i-%02i-%02i-%02i-%02i",
            ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min);

    if (wallet_path.empty()) {
        // ... opened wallet
        LOCK(cs_wallet);
        fs::path backupFile = backupsDir / (strWalletName + dateTimeStr);
        backupFile.make_preferred();
        if (!BackupWallet(backupFile.string())) {
            warnings.push_back(strprintf(_("Failed to create backup %s!"), backupFile.string()));
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
        std::shared_ptr<BerkeleyEnvironment> env = GetBerkeleyEnv(strSourceFile.parent_path());
        fs::path sourceFile = env->Directory() / strSourceFile.filename().string();
        fs::path backupFile = backupsDir / (strWalletName + dateTimeStr);
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
                fs::copy_file(sourceFile, backupFile);
                WalletLogPrintf("Creating backup of %s -> %s\n", sourceFile.string(), backupFile.string());
            } catch(fs::filesystem_error &error) {
                warnings.push_back(strprintf(_("Failed to create backup, error: %s"), fsbridge::get_filesystem_error_message(error)));
                WalletLogPrintf("%s\n", Join(warnings, Untranslated("\n")).original);
                nWalletBackups = -1;
                return false;
            }
        }
    }

    // Keep only the last 10 backups, including the new one of course
    typedef std::multimap<std::time_t, fs::path> folder_set_t;
    folder_set_t folder_set;
    fs::directory_iterator end_iter;
    // Build map of backup files for current(!) wallet sorted by last write time
    fs::path currentFile;
    for (fs::directory_iterator dir_iter(backupsDir); dir_iter != end_iter; ++dir_iter)
    {
        // Only check regular files
        if ( fs::is_regular_file(dir_iter->status()))
        {
            currentFile = dir_iter->path().filename();
            // Only add the backups for the current wallet, e.g. wallet.dat.*
            if (dir_iter->path().stem().string() == strWalletName) {
                folder_set.insert(folder_set_t::value_type(fs::last_write_time(dir_iter->path()), *dir_iter));
            }
        }
    }

    // Loop backward through backup files and keep the N newest ones (1 <= N <= 10)
    int counter = 0;
    for(auto it = folder_set.rbegin(); it != folder_set.rend(); ++it) {
        std::pair<const std::time_t, fs::path> file = *it;
        counter++;
        if (counter > nWalletBackups)
        {
            // More than nWalletBackups backups: delete oldest one(s)
            try {
                fs::remove(file.second);
                WalletLogPrintf("Old backup deleted: %s\n", file.second);
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

void CWallet::notifyTransactionLock(const CTransactionRef &tx, const std::shared_ptr<const llmq::CInstantSendLock>& islock)
{
    LOCK(cs_wallet);
    // Only notify UI if this transaction is in this wallet
    uint256 txHash = tx->GetHash();
    std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txHash);
    if (mi != mapWallet.end()){
        NotifyTransactionChanged(this, txHash, CT_UPDATED);
        NotifyISLockReceived();
#if HAVE_SYSTEM
        // notify an external script
        std::string strCmd = gArgs.GetArg("-instantsendnotify", "");
        if (!strCmd.empty()) {
            ReplaceAll(strCmd, "%s", txHash.GetHex());
            std::thread t(runCommand, strCmd);
            t.detach(); // thread runs free
        }
#endif
    }
}

void CWallet::notifyChainLock(const CBlockIndex* pindexChainLock, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    NotifyChainLockReceived(pindexChainLock->nHeight);
}

bool CWallet::LoadGovernanceObject(const CGovernanceObject& obj)
{
    AssertLockHeld(cs_wallet);
    return m_gobjects.emplace(obj.GetHash(), obj).second;
}

bool CWallet::WriteGovernanceObject(const CGovernanceObject& obj)
{
    AssertLockHeld(cs_wallet);
    WalletBatch batch(*database);
    return batch.WriteGovernanceObject(obj) && LoadGovernanceObject(obj);
}

std::vector<const CGovernanceObject*> CWallet::GetGovernanceObjects()
{
    AssertLockHeld(cs_wallet);
    std::vector<const CGovernanceObject*> vecObjects;
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

int CWalletTx::GetDepthInMainChain() const
{
    assert(pwallet != nullptr);
    AssertLockHeld(pwallet->cs_wallet);
    if (isUnconfirmed() || isAbandoned()) return 0;

    return (pwallet->GetLastBlockHeight() - m_confirm.block_height + 1) * (isConflicted() ? -1 : 1);
}

bool CWalletTx::IsLockedByInstantSend() const
{
    if (fIsChainlocked) {
        fIsInstantSendLocked = false;
    } else if (!fIsInstantSendLocked) {
        fIsInstantSendLocked = llmq::quorumInstantSendManager->IsLocked(GetHash());
    }
    return fIsInstantSendLocked;
}

bool CWalletTx::IsChainLocked() const
{
    if (!fIsChainlocked) {
        assert(pwallet != nullptr);
        AssertLockHeld(pwallet->cs_wallet);
        bool active;
        int height;
        if (pwallet->chain().findBlock(m_confirm.hashBlock, FoundBlock().inActiveChain(active).height(height)) && active) {
            fIsChainlocked = llmq::chainLocksHandler->HasChainLock(height, m_confirm.hashBlock);
        }
    }
    return fIsChainlocked;
}

int CWalletTx::GetBlocksToMaturity() const
{
    if (!IsCoinBase())
        return 0;
    int chain_depth = GetDepthInMainChain();
    assert(chain_depth >= 0); // coinbase tx should not be conflicted
    return std::max(0, (COINBASE_MATURITY+1) - chain_depth);
}

bool CWalletTx::IsImmatureCoinBase() const
{
    // note GetBlocksToMaturity is 0 for non-coinbase tx
    return GetBlocksToMaturity() > 0;
}

std::vector<OutputGroup> CWallet::GroupOutputs(const std::vector<COutput>& outputs, bool single_coin, const size_t max_ancestors) const {
    std::vector<OutputGroup> groups;
    std::map<CTxDestination, OutputGroup> gmap;
    std::set<CTxDestination> full_groups;

    for (const auto& output : outputs) {
        if (output.fSpendable) {
            CTxDestination dst;
            CInputCoin input_coin = output.GetInputCoin();

            size_t ancestors, descendants;
            chain().getTransactionAncestry(output.tx->GetHash(), ancestors, descendants);
            if (!single_coin && ExtractDestination(output.tx->tx->vout[output.i].scriptPubKey, dst)) {
                auto it = gmap.find(dst);
                if (it != gmap.end()) {
                    // Limit output groups to no more than OUTPUT_GROUP_MAX_ENTRIES
                    // number of entries, to protect against inadvertently creating
                    // a too-large transaction when using -avoidpartialspends to
                    // prevent breaking consensus or surprising users with a very
                    // high amount of fees.
                    if (it->second.m_outputs.size() >= OUTPUT_GROUP_MAX_ENTRIES) {
                        groups.push_back(it->second);
                        it->second = OutputGroup{};
                        full_groups.insert(dst);
                    }
                    it->second.Insert(input_coin, output.nDepth, output.tx->IsFromMe(ISMINE_ALL), ancestors, descendants);
                } else {
                    gmap[dst].Insert(input_coin, output.nDepth, output.tx->IsFromMe(ISMINE_ALL), ancestors, descendants);
                }
            } else {
                groups.emplace_back(input_coin, output.nDepth, output.tx->IsFromMe(ISMINE_ALL), ancestors, descendants);
            }
        }
    }
    if (!single_coin) {
        for (auto& it : gmap) {
            auto& group = it.second;
            if (full_groups.count(it.first) > 0) {
                // Make this unattractive as we want coin selection to avoid it if possible
                group.m_ancestors = max_ancestors - 1;
            }
            groups.push_back(group);
        }
    }
    return groups;
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
    bool result;
    {
        LOCK(cs_wallet);
        result = vMasterKey.empty();
    }
    // fForMixing   fOnlyMixingAllowed  return
    // ---------------------------------------
    // true         true                result
    // true         false               result
    // false        true                true
    // false        false               result

    if(!fForMixing && fOnlyMixingAllowed) return true;

    return result;
}

bool CWallet::Lock(bool fAllowMixing)
{
    if (!IsCrypted())
        return false;

    if(!fAllowMixing) {
        LOCK(cs_wallet);
        vMasterKey.clear();
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
                if(nWalletBackups == -2) {
                    TopUpKeyPool();
                    WalletLogPrintf("Keypool replenished, re-initializing automatic backups.\n");
                    nWalletBackups = gArgs.GetArg("-createwalletbackups", 10);
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
        WalletLogPrintf("%s scriptPubKey Manager does not exist\n", internal ? "Internal" : "External");
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
    if (m_internal_spk_managers || m_external_spk_managers || !m_spk_managers.empty()) {
        return;
    }

    auto spk_manager = std::make_unique<LegacyScriptPubKeyMan>(*this);
    m_internal_spk_managers = spk_manager.get();
    m_external_spk_managers = spk_manager.get();
    m_spk_managers[spk_manager->GetID()] = std::move(spk_manager);
}

const CKeyingMaterial& CWallet::GetEncryptionKey() const
{
    return vMasterKey;
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

bool CWallet::GenerateNewHDChainEncrypted(const SecureString& secureMnemonic, const SecureString& secureMnemonicPassphrase, const SecureString& secureWalletPassphrase)
{
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        throw std::runtime_error(strprintf("%s: spk_man is not available", __func__));
    }

    assert(!IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));

    if (!HasEncryptionKeys()) {
        return false;
    }

    CCrypter crypter;
    CKeyingMaterial vMasterKey;
    CHDChain hdChainTmp;

    // NOTE: an empty mnemonic means "generate a new one for me"
    // NOTE: default mnemonic passphrase is an empty string
    if (!hdChainTmp.SetMnemonic(secureMnemonic, secureMnemonicPassphrase, true)) {
        throw std::runtime_error(std::string(__func__) + ": SetMnemonic failed");
    }

    // add default account
    hdChainTmp.AddAccount();
    hdChainTmp.Debug(__func__);

    LOCK(cs_wallet);
    for (const CWallet::MasterKeyMap::value_type& pMasterKey : mapMasterKeys) {
        if (!crypter.SetKeyFromPassphrase(secureWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod)) {
            return false;
        }
        // get vMasterKey to encrypt new hdChain
        if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey)) {
            continue; // try another master key
        }

        bool res = spk_man->EncryptHDChain(vMasterKey, hdChainTmp);
        assert(res);

        CHDChain hdChainCrypted;
        res = spk_man->GetHDChain(hdChainCrypted);
        assert(res);

        DBG(
            tfm::format(std::cout, "GenerateNewHDChainEncrypted -- current seed: '%s'\n", HexStr(hdChainTmp.GetSeed()));
            tfm::format(std::cout, "GenerateNewHDChainEncrypted -- crypted seed: '%s'\n", HexStr(hdChainCrypted.GetSeed()));
        );

        // ids should match, seed hashes should not
        assert(hdChainTmp.GetID() == hdChainCrypted.GetID());
        assert(hdChainTmp.GetSeedHash() != hdChainCrypted.GetSeedHash());

        hdChainCrypted.Debug(__func__);

        if (spk_man->SetCryptedHDChainSingle(hdChainCrypted, false)) {
            Lock();
            if (!Unlock(secureWalletPassphrase)) {
                // this should never happen
                throw std::runtime_error(std::string(__func__) + ": Unlock failed");
            }
            if (!spk_man->NewKeyPool()) {
                throw std::runtime_error(std::string(__func__) + ": NewKeyPool failed");
            }
            Lock();
            return true;
        }
    }

    return false;
}
