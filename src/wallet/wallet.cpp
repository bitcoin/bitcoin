// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <chain.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <fs.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <key.h>
#include <key_io.h>
#include <optional.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <txmempool.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/error.h>
#include <util/fees.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/string.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/txassembler.h>
#include <wallet/fees.h>
#include <wallet/reserve.h>

#include <univalue.h>

#include <algorithm>
#include <assert.h>

#include <boost/algorithm/string/replace.hpp>

using interfaces::FoundBlock;

const std::map<uint64_t,std::string> WALLET_FLAG_CAVEATS{
    {WALLET_FLAG_AVOID_REUSE,
        "You need to rescan the blockchain in order to correctly mark used "
        "destinations in the past. Until this is done, some destinations may "
        "be considered unused, even if the opposite is the case."
    },
};

static const size_t OUTPUT_GROUP_MAX_ENTRIES = 10;

static RecursiveMutex cs_wallets;
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
                                Optional<bool> load_on_startup,
                                std::vector<bilingual_str>& warnings)
{
    if (load_on_startup == nullopt) return;
    if (load_on_startup.get() && !AddWalletSetting(chain, wallet_name)) {
        warnings.emplace_back(Untranslated("Wallet load on startup setting could not be updated, so wallet may not be loaded next node startup."));
    } else if (!load_on_startup.get() && !RemoveWalletSetting(chain, wallet_name)) {
        warnings.emplace_back(Untranslated("Wallet load on startup setting could not be updated, so wallet may still be loaded next node startup."));
    }
}

bool AddWallet(const std::shared_ptr<CWallet>& wallet)
{
    LOCK(cs_wallets);
    assert(wallet);
    std::vector<std::shared_ptr<CWallet>>::const_iterator i = std::find(vpwallets.begin(), vpwallets.end(), wallet);
    if (i != vpwallets.end()) return false;
    vpwallets.push_back(wallet);
    wallet->ConnectScriptPubKeyManNotifiers();
    wallet->NotifyCanGetAddressesChanged();
    return true;
}

bool RemoveWallet(const std::shared_ptr<CWallet>& wallet, Optional<bool> load_on_start, std::vector<bilingual_str>& warnings)
{
    assert(wallet);

    interfaces::Chain& chain = wallet->chain();
    std::string name = wallet->GetName();

    // Unregister with the validation interface which also drops shared ponters.
    wallet->m_chain_notifications_handler.reset();
    LOCK(cs_wallets);
    std::vector<std::shared_ptr<CWallet>>::iterator i = std::find(vpwallets.begin(), vpwallets.end(), wallet);
    if (i == vpwallets.end()) return false;
    vpwallets.erase(i);

    // Write the wallet setting
    UpdateWalletSetting(chain, name, load_on_start, warnings);

    return true;
}

bool RemoveWallet(const std::shared_ptr<CWallet>& wallet, Optional<bool> load_on_start)
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
std::shared_ptr<CWallet> LoadWalletInternal(interfaces::Chain& chain, const std::string& name, Optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
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

std::shared_ptr<CWallet> LoadWallet(interfaces::Chain& chain, const std::string& name, Optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    auto result = WITH_LOCK(g_loading_wallet_mutex, return g_loading_wallet_set.insert(name));
    if (!result.second) {
        error = Untranslated("Wallet already being loading.");
        status = DatabaseStatus::FAILED_LOAD;
        return nullptr;
    }
    auto wallet = LoadWalletInternal(chain, name, load_on_start, options, status, error, warnings);
    WITH_LOCK(g_loading_wallet_mutex, g_loading_wallet_set.erase(result.first));
    return wallet;
}

std::shared_ptr<CWallet> CreateWallet(interfaces::Chain& chain, const std::string& name, Optional<bool> load_on_start, DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    uint64_t wallet_creation_flags = options.create_flags;
    const SecureString& passphrase = options.create_passphrase;

    if (wallet_creation_flags & WALLET_FLAG_DESCRIPTORS) {
        error = Untranslated("Descriptor wallets not supported.") + Untranslated(" ") + error;
        status = DatabaseStatus::FAILED_CREATE;
        return nullptr;
    }

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

            // Set a seed for the wallet
            {
                LOCK(wallet->cs_wallet);
                if (wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
                    wallet->SetupDescriptorScriptPubKeyMans();
                } else {
                    for (auto spk_man : wallet->GetActiveScriptPubKeyMans()) {
                        if (!spk_man->SetupGeneration()) {
                            error = Untranslated("Unable to generate initial keys");
                            status = DatabaseStatus::FAILED_CREATE;
                            return nullptr;
                        }
                    }
                }
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

CWalletTx* CWallet::GetWalletTx(const uint256& hash)
{
    AssertLockHeld(cs_wallet);
    std::map<uint256, CWalletTx>::iterator it = mapWallet.find(hash);
    if (it == mapWallet.end())
        return nullptr;
    return &(it->second);
}

const CWalletTx* CWallet::GetWalletTx(const uint256& hash) const
{
    AssertLockHeld(cs_wallet);
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

bool CWallet::Unlock(const SecureString& strWalletPassphrase, bool accept_no_keys)
{
    CCrypter crypter;
    CKeyingMaterial _vMasterKey;

    {
        LOCK(cs_wallet);
        for (const MasterKeyMap::value_type& pMasterKey : mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, _vMasterKey))
                continue; // try another master key
            if (Unlock(_vMasterKey, accept_no_keys)) {
                // Now that we've unlocked, upgrade the key metadata
                UpgradeKeyMetadata();

                // MWEB: Load MWEB keychain
                auto mweb_spk_man = GetScriptPubKeyMan(OutputType::MWEB, false);
                if (mweb_spk_man) {
                    mweb_spk_man->LoadMWEBKeychain();
                    mweb_wallet->UpgradeCoins();
                }

                return true;
            }
        }
    }
    return false;
}

bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
{
    bool fWasLocked = IsLocked();

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

void CWallet::SetMinVersion(enum WalletFeature nVersion, WalletBatch* batch_in)
{
    LOCK(cs_wallet);
    if (nWalletVersion >= nVersion)
        return;
    nWalletVersion = nVersion;

    {
        WalletBatch* batch = batch_in ? batch_in : new WalletBatch(*database);
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

    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(txid);
    if (it == mapWallet.end())
        return result;
    const CWalletTx& wtx = it->second;

    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;

    for (const CTxInput& txin : wtx.GetInputs())
    {
        if (mapTxSpends.count(txin.GetIndex()) <= 1)
            continue;  // No conflict if zero or one spends
        range = mapTxSpends.equal_range(txin.GetIndex());
        for (TxSpends::const_iterator _it = range.first; _it != range.second; ++_it)
            result.insert(_it->second);
    }
    return result;
}

bool CWallet::HasWalletSpend(const uint256& txid) const
{
    AssertLockHeld(cs_wallet);
    auto iter = mapTxSpends.lower_bound(COutPoint(txid, 0));
    return (iter != mapTxSpends.end() && iter->first.type() == typeid(COutPoint) && boost::get<COutPoint>(iter->first).hash == txid);
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
bool CWallet::IsSpent(const OutputIndex& idx) const
{
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    range = mapTxSpends.equal_range(idx);

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

void CWallet::AddToSpends(const OutputIndex& idx, const uint256& wtxid)
{
    mapTxSpends.insert(std::make_pair(idx, wtxid));

    setLockedCoins.erase(idx);

    std::pair<TxSpends::iterator, TxSpends::iterator> range;
    range = mapTxSpends.equal_range(idx);
    SyncMetaData(range);
}


void CWallet::AddToSpends(const uint256& wtxid)
{
    auto it = mapWallet.find(wtxid);
    assert(it != mapWallet.end());
    CWalletTx& thisTx = it->second;
    if (thisTx.IsCoinBase()) // Coinbases don't spend anything!
        return;

    for (const CTxInput& input : thisTx.tx->GetInputs()) {
        AddToSpends(input.GetIndex(), wtxid);
    }

    if (!!thisTx.mweb_wtx_info && !!thisTx.mweb_wtx_info->spent_input) {
        AddToSpends(*thisTx.mweb_wtx_info->spent_input, wtxid);
    }
}

void CWallet::AddMWEBOrigins(const CWalletTx& wtx)
{
    for (const mw::Hash& output_id : wtx.tx->mweb_tx.GetOutputIDs()) {
        mapOutputsMWEB.insert(std::make_pair(output_id, wtx.GetHash()));
    }

    if (wtx.mweb_wtx_info && wtx.mweb_wtx_info->received_coin) {
        const mw::Hash& output_id = wtx.mweb_wtx_info->received_coin->output_id;
        mapOutputsMWEB.insert(std::make_pair(output_id, wtx.GetHash()));
    }

    for (const mw::Hash& kernel_id : wtx.tx->mweb_tx.GetKernelIDs()) {
        mapKernelsMWEB.insert(std::make_pair(kernel_id, wtx.GetHash()));
    }
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

        // MWEB: No need to replace HD seed, which would complicate MWEB key management.
        // So for now, we don't generate a new seed.
        // 
        // If we are using descriptors, make new descriptors with a new seed
        //if (IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS) && !IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET)) {
        //    SetupDescriptorScriptPubKeyMans();
        //} else if (auto spk_man = GetLegacyScriptPubKeyMan()) {
        //    // if we are using HD, replace the HD seed with a new one
        //    if (spk_man->IsHDEnabled()) {
        //        if (!spk_man->SetupGeneration(true)) {
        //            return false;
        //        }
        //    }
        //}
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
}

bool CWallet::MarkReplaced(const uint256& originalHash, const uint256& newHash)
{
    LOCK(cs_wallet);

    auto mi = mapWallet.find(originalHash);

    // There is a bug if MarkReplaced is not called on an existing wallet transaction.
    assert(mi != mapWallet.end());

    CWalletTx& wtx = (*mi).second;

    // Ensure for now that we're not overwriting data
    assert(wtx.mapValue.count("replaced_by_txid") == 0);

    wtx.mapValue["replaced_by_txid"] = newHash.ToString();

    WalletBatch batch(*database);

    bool success = true;
    if (!batch.WriteTx(wtx)) {
        WalletLogPrintf("%s: Updating batch tx %s failed\n", __func__, wtx.GetHash().ToString());
        success = false;
    }

    NotifyTransactionChanged(this, originalHash, CT_UPDATED);

    return success;
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

bool CWallet::IsSpentKey(const CTxOutput& output) const
{
    AssertLockHeld(cs_wallet);
    CTxDestination dest;
    if (!ExtractOutputDestination(output, dest)) {
        return false;
    }
    if (GetDestData(dest, "used", nullptr)) {
        return true;
    }

    DestinationAddr dest_addr;
    if (!ExtractDestinationScript(output, dest_addr)) {
        return false;
    }
    if (IsLegacy()) {
        LegacyScriptPubKeyMan* spk_man = GetLegacyScriptPubKeyMan();
        assert(spk_man != nullptr);
        for (const auto& keyid : GetAffectedKeys(dest_addr, *spk_man)) {
            WitnessV0KeyHash wpkh_dest(keyid);
            if (GetDestData(wpkh_dest, "used", nullptr)) {
                return true;
            }
            ScriptHash sh_wpkh_dest(GetScriptForDestination(wpkh_dest));
            if (GetDestData(sh_wpkh_dest, "used", nullptr)) {
                return true;
            }
            PKHash pkh_dest(keyid);
            if (GetDestData(pkh_dest, "used", nullptr)) {
                return true;
            }
        }
    }

    return false;
}

CWalletTx* CWallet::AddToWallet(CTransactionRef tx, const boost::optional<MWEB::WalletTxInfo>& mweb_wtx_info, const CWalletTx::Confirmation& confirm, const UpdateWalletTxFn& update_wtx, bool fFlushOnClose)
{
    LOCK(cs_wallet);

    WalletBatch batch(*database, fFlushOnClose);

    uint256 hash = CWalletTx(this, tx, mweb_wtx_info).GetHash();

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
    auto ret = mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(this, tx, mweb_wtx_info));
    CWalletTx& wtx = (*ret.first).second;
    bool fInsertedNew = ret.second;
    bool fUpdated = update_wtx && update_wtx(wtx, fInsertedNew);
    if (fInsertedNew) {
        wtx.m_confirm = confirm;
        wtx.nTimeReceived = chain().getAdjustedTime();
        wtx.nOrderPos = IncOrderPosNext(&batch);
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
        wtx.nTimeSmart = ComputeTimeSmart(wtx);
        AddToSpends(hash);
        AddMWEBOrigins(wtx);
    }

    if (!fInsertedNew)
    {
        if (confirm != wtx.m_confirm) {
            wtx.m_confirm.status = confirm.status;
            wtx.m_confirm.nIndex = confirm.nIndex;
            wtx.m_confirm.hashBlock = confirm.hashBlock;
            wtx.m_confirm.block_height = confirm.block_height;
            fUpdated = true;
        }

        // If we have a witness-stripped version of this transaction, and we
        // see a new version with a witness, then we must be upgrading a pre-segwit
        // wallet.  Store the new version of the transaction with the witness,
        // as the stripped-version must be invalid.
        // TODO: Store all versions of the transaction, instead of just one.
        if (tx->HasWitness() && !wtx.tx->HasWitness()) {
            wtx.SetTx(tx);
            fUpdated = true;
        }

        // MWEB: If we have only a partial transaction, and tx is not partial, replace it.
        if (wtx.tx->IsNull() && !tx->IsNull()) {
            wtx.SetTx(tx);
            wtx.mweb_wtx_info = mweb_wtx_info;
            fUpdated = true;
        }
    }

    //// debug print
    WalletLogPrintf("AddToWallet %s  %s%s\n", hash.ToString(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

    // Write to disk
    if (fInsertedNew || fUpdated)
        if (!batch.WriteTx(wtx))
            return nullptr;

    // Break debit/credit balance caches:
    wtx.MarkDirty();

    // Notify UI of new or updated transaction
    NotifyTransactionChanged(this, hash, fInsertedNew ? CT_NEW : CT_UPDATED);

#if HAVE_SYSTEM
    // notify an external script when a wallet transaction comes in or is updated
    std::string strCmd = gArgs.GetArg("-walletnotify", "");

    if (!strCmd.empty())
    {
        boost::replace_all(strCmd, "%s", hash.GetHex());
#ifndef WIN32
        // Substituting the wallet name isn't currently supported on windows
        // because windows shell escaping has not been implemented yet:
        // https://github.com/bitcoin/bitcoin/pull/13339#issuecomment-537384875
        // A few ways it could be implemented in the future are described in:
        // https://github.com/bitcoin/bitcoin/pull/13339#issuecomment-461288094
        boost::replace_all(strCmd, "%w", ShellEscape(GetName()));
#endif
        std::thread t(runCommand, strCmd);
        t.detach(); // thread runs free
    }
#endif

    return &wtx;
}

bool CWallet::LoadToWallet(const uint256& hash, const UpdateWalletTxFn& fill_wtx)
{
    CWalletTx wtx_tmp(this, nullptr);
    if (!fill_wtx(wtx_tmp, true)) {
        return false;
    }

    uint256 wtx_hash = wtx_tmp.GetHash();
    if (mapWallet.count(wtx_hash) > 0 && wtx_tmp.tx->IsNull()) {
        WalletLogPrintf("%s already exists\n", wtx_hash.ToString());
        return true;
    }

    const auto& ins = mapWallet.emplace(wtx_hash, std::move(wtx_tmp));
    assert(ins.second);

    CWalletTx& wtx = ins.first->second;

    // If wallet doesn't have a chain (e.g wallet-tool), don't bother to update txn.
    if (HaveChain()) {
        Optional<int> block_height = chain().getBlockHeight(wtx.m_confirm.hashBlock);
        if (block_height) {
            // Update cached block height variable since it not stored in the
            // serialized transaction.
            wtx.m_confirm.block_height = *block_height;
        } else if (wtx.isConflicted() || wtx.isConfirmed()) {
            // If tx block (or conflicting block) was reorged out of chain
            // while the wallet was shutdown, change tx status to UNCONFIRMED
            // and reset block height, hash, and index. ABANDONED tx don't have
            // associated blocks and don't need to be updated. The case where a
            // transaction was reorged out while online and then reconfirmed
            // while offline is covered by the rescan logic.
            wtx.setUnconfirmed();
            wtx.m_confirm.hashBlock = uint256();
            wtx.m_confirm.block_height = 0;
            wtx.m_confirm.nIndex = 0;
        }
    }
    if (/* insertion took place */ ins.second) {
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
    }
    AddToSpends(wtx.GetHash());
    AddMWEBOrigins(wtx);
    for (const CTxInput& txin : wtx.GetInputs()) {
        CWalletTx* prevtx = FindPrevTx(txin);
        if (prevtx != nullptr) {
            if (prevtx->isConflicted()) {
                MarkConflicted(prevtx->m_confirm.hashBlock, prevtx->m_confirm.block_height, wtx.GetHash());
            }
        }
    }
    return true;
}

bool CWallet::AddToWalletIfInvolvingMe(const CTransactionRef& ptx, const boost::optional<MWEB::WalletTxInfo>& mweb_wtx_info, CWalletTx::Confirmation confirm, bool fUpdate)
{
    CWalletTx wtx(this, ptx, mweb_wtx_info);
    uint256 hash = wtx.GetHash();

    const CTransaction& tx = *ptx;
    {
        AssertLockHeld(cs_wallet);

        if (!confirm.hashBlock.IsNull()) {
            for (const CTxInput& txin : wtx.GetInputs()) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(txin.GetIndex());
                while (range.first != range.second) {
                    if (range.first->second != hash) {
                        WalletLogPrintf("Transaction %s (in block %s) conflicts with wallet transaction %s\n", hash.ToString(), confirm.hashBlock.ToString(), range.first->second.ToString());
                        MarkConflicted(confirm.hashBlock, confirm.block_height, range.first->second);
                    }
                    range.first++;
                }
            }
        }

        mweb_wallet->RewindOutputs(tx);

        bool fExisted = mapWallet.count(hash) != 0;
        if (fExisted && !fUpdate) return false;
        if (fExisted || IsMine(tx, mweb_wtx_info) || IsFromMe(tx, mweb_wtx_info))
        {
            /* Check if any keys in the wallet keypool that were supposed to be unused
             * have appeared in a new transaction. If so, remove those keys from the keypool.
             * This can happen when restoring an old wallet backup that does not contain
             * the mostly recently created transactions from newer versions of the wallet.
             */

            // loop though all outputs
            for (const CTxOutput& txout : wtx.GetOutputs()) {
                DestinationAddr dest;
                if (ExtractDestinationScript(txout, dest)) {
                    for (const auto& spk_man_pair : m_spk_managers) {
                        spk_man_pair.second->MarkUnusedAddresses(dest);
                    }
                }
            }

            // Loop through pegout scripts
            for (const PegOutCoin& pegout : tx.mweb_tx.GetPegOuts()) {
                for (const auto& spk_man_pair : m_spk_managers) {
                    spk_man_pair.second->MarkUnusedAddresses(DestinationAddr{pegout.GetScriptPubKey()});
                }
            }

            // Block disconnection override an abandoned tx as unconfirmed
            // which means user may have to call abandontransaction again
            return AddToWallet(MakeTransactionRef(tx), mweb_wtx_info, confirm, /* update_wtx= */ nullptr, /* fFlushOnClose= */ false);
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

void CWallet::MarkInputsDirty(const CWalletTx& wtx)
{
    for (const CTxInput& txin : wtx.GetInputs()) {
        CWalletTx* prev = FindPrevTx(txin);
        if (prev != nullptr) {
            prev->MarkDirty();
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
    if (origtx.GetDepthInMainChain() != 0 || origtx.InMempool()) {
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
            for (const CTxOutput& output : wtx.GetOutputs()) {
                auto iter = mapTxSpends.find(output.GetIndex());
                if (iter != mapTxSpends.end()) {
                    if (!done.count(iter->second)) {
                        todo.insert(iter->second);
                    }
                }
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            MarkInputsDirty(wtx);
        }
    }

    return true;
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
            for (const CTxOutput& output : wtx.GetOutputs()) {
                auto iter = mapTxSpends.find(output.GetIndex());
                if (iter != mapTxSpends.end()) {
                    if (!done.count(iter->second)) {
                        todo.insert(iter->second);
                    }
                }
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            MarkInputsDirty(wtx);
        }
    }
}

void CWallet::SyncTransaction(const CTransactionRef& ptx, const boost::optional<MWEB::WalletTxInfo>& mweb_wtx_info, CWalletTx::Confirmation confirm, bool update_tx)
{
    if (!AddToWalletIfInvolvingMe(ptx, mweb_wtx_info, confirm, update_tx))
        return; // Not one of ours

    // If a transaction changes 'conflicted' state, that changes the balance
    // available of the outputs it spends. So force those to be
    // recomputed, also:
    MarkInputsDirty(CWalletTx(this, ptx, mweb_wtx_info));
}

void CWallet::transactionAddedToMempool(const CTransactionRef& tx, uint64_t mempool_sequence) {
    LOCK(cs_wallet);
    SyncTransaction(tx, boost::none, {CWalletTx::Status::UNCONFIRMED, /* block height */ 0, /* block hash */ {}, /* index */ 0});

    auto it = mapWallet.find(tx->GetHash());
    if (it != mapWallet.end()) {
        it->second.fInMempool = true;
    }
}

void CWallet::transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) {
    LOCK(cs_wallet);
    auto it = mapWallet.find(tx->GetHash());
    if (it != mapWallet.end()) {
        it->second.fInMempool = false;
    }

    for (const mw::Hash& output_id : tx->mweb_tx.GetOutputIDs()) {
        auto out_iter = mapOutputsMWEB.find(output_id);
        if (out_iter != mapOutputsMWEB.end()) {
            auto tx_iter = mapWallet.find(out_iter->second);
            if (tx_iter != mapWallet.end()) {
                tx_iter->second.fInMempool = false;
            }
        }
    }

    // Handle transactions that were removed from the mempool because they
    // conflict with transactions in a newly connected block.
    if (reason == MemPoolRemovalReason::CONFLICT) {
        // Call SyncNotifications, so external -walletnotify notifications will
        // be triggered for these transactions. Set Status::UNCONFIRMED instead
        // of Status::CONFLICTED for a few reasons:
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
        SyncTransaction(tx, boost::none, {CWalletTx::Status::UNCONFIRMED, /* block height */ 0, /* block hash */ {}, /* index */ 0});
    }
}

void CWallet::blockConnected(const CBlock& block, int height)
{
    const uint256& block_hash = block.GetHash();
    LOCK(cs_wallet);

    m_last_block_processed_height = height;
    m_last_block_processed = block_hash;
    for (size_t index = 0; index < block.vtx.size(); index++) {
        SyncTransaction(block.vtx[index], boost::none, {CWalletTx::Status::CONFIRMED, height, block_hash, (int)index});
        transactionRemovedFromMempool(block.vtx[index], MemPoolRemovalReason::BLOCK, 0 /* mempool_sequence */);
    }

    if (!block.mweb_block.IsNull()) {
        mw::Coin coin;
        for (const mw::Hash& spent_id : block.mweb_block.GetSpentIDs()) {
            if (GetCoin(spent_id, coin) && coin.IsMine() && !IsSpent(spent_id)) {
                AddToWallet(
                    MakeTransactionRef(),
                    boost::make_optional<MWEB::WalletTxInfo>(spent_id),
                    {CWalletTx::Status::CONFIRMED, height, block_hash, 0}
                );
            }
        }

        CWalletTx* hogex_wtx = GetWalletTx(block.vtx.back()->GetHash());
        if (hogex_wtx != nullptr) {
            hogex_wtx->pegout_indices.clear();
            hogex_wtx->pegout_indices.push_back({mw::Hash(), 0}); // HogAddr doesn't have a corresponding kernel

            std::multimap<PegOutCoin, std::pair<mw::Hash, size_t>> pegout_kernels;
            for (const Kernel& kernel : block.mweb_block.m_block->GetKernels()) {
                const auto& kernel_pegouts = kernel.GetPegOuts();
                for (size_t pegout_idx = 0; pegout_idx < kernel_pegouts.size(); pegout_idx++) {
                    pegout_kernels.insert({kernel_pegouts[pegout_idx], {kernel.GetKernelID(), pegout_idx}});
                }
            }

            for (size_t i = 1; i < hogex_wtx->tx->vout.size(); i++) {
                const auto& hogex_out = hogex_wtx->tx->vout[i];
                PegOutCoin pegout{hogex_out.nValue, hogex_out.scriptPubKey};
                auto iter = pegout_kernels.find(pegout);
                if (iter != pegout_kernels.end()) {
                    hogex_wtx->pegout_indices.push_back(iter->second);
                    pegout_kernels.erase(iter);
                } else {
                    assert(false);
                }
            }

            assert(hogex_wtx->tx->vout.size() == hogex_wtx->pegout_indices.size());
            WalletBatch(*database).WriteTx(*hogex_wtx);
        }

        for (const Kernel& kernel : block.mweb_block.m_block->GetKernels()) {
            auto wtx = FindWalletTxByKernelId(kernel.GetKernelID());
            if (wtx != nullptr) {
                SyncTransaction(wtx->tx, wtx->mweb_wtx_info, {CWalletTx::Status::CONFIRMED, height, block_hash, wtx->m_confirm.nIndex});
                transactionRemovedFromMempool(wtx->tx, MemPoolRemovalReason::BLOCK, 0 /* mempool_sequence */);
            }
        }

        mw::Coin mweb_coin;
        for (const Output& output : block.mweb_block.m_block->GetOutputs()) {
            if (mweb_wallet->RewindOutput(output, mweb_coin)) {
                auto wtx = FindWalletTx(output.GetOutputID());
                if (wtx != nullptr) {
                    SyncTransaction(wtx->tx, wtx->mweb_wtx_info, {CWalletTx::Status::CONFIRMED, height, block_hash, wtx->m_confirm.nIndex});
                    transactionRemovedFromMempool(wtx->tx, MemPoolRemovalReason::BLOCK, 0 /* mempool_sequence */);
                } else {
                    AddToWallet(
                        MakeTransactionRef(),
                        boost::make_optional<MWEB::WalletTxInfo>(mweb_coin),
                        {CWalletTx::Status::CONFIRMED, height, block_hash, 0}
                    );
                }
            }
        }
    }
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
    for (const CTransactionRef& ptx : block.vtx) {
        SyncTransaction(ptx, boost::none, {CWalletTx::Status::UNCONFIRMED, /* block height */ 0, /* block hash */ {}, /* index */ 0});
    }

    if (!block.mweb_block.IsNull()) {
        mw::Coin coin;
        for (const mw::Hash& spent_id : block.mweb_block.GetSpentIDs()) {
            if (GetCoin(spent_id, coin) && coin.IsMine()) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(spent_id);
                // MWEB: We just choose the first spend. In the future, we may need a better approach for handling conflicted txs
                if (range.first != range.second) {
                    auto tx_iter = mapWallet.find(range.first->second);
                    SyncTransaction(
                        tx_iter->second.tx,
                        tx_iter->second.mweb_wtx_info,
                        {CWalletTx::Status::UNCONFIRMED, /* block height */ 0, /* block hash */ {}, /* index */ 0}
                    );
                }
            }
        }

        for (const mw::Hash& kernel_id : block.mweb_block.GetKernelIDs()) {
            auto wtx = FindWalletTxByKernelId(kernel_id);
            if (wtx != nullptr) {
                SyncTransaction(
                    wtx->tx,
                    wtx->mweb_wtx_info,
                    {CWalletTx::Status::UNCONFIRMED, /* block height */ 0, /* block hash */ {}, /* index */ 0}
                );
            }
        }

        for (const Output& output : block.mweb_block.m_block->GetOutputs()) {
            if (mweb_wallet->RewindOutput(output, coin)) {
                auto wtx = FindWalletTx(output.GetOutputID());
                if (wtx != nullptr) {
                    SyncTransaction(
                        wtx->tx,
                        wtx->mweb_wtx_info,
                        {CWalletTx::Status::UNCONFIRMED, /* block height */ 0, /* block hash */ {}, /* index */ 0}
                    );
                }
            }
        }
    }
}

void CWallet::updatedBlockTip()
{
    m_best_block_time = GetTime();
}


void CWallet::BlockUntilSyncedToCurrentChain() const {
    AssertLockNotHeld(cs_wallet);
    // Skip the queue-draining stuff if we know we're caught up with
    // ::ChainActive().Tip(), otherwise put a callback in the validation interface queue and wait
    // for the queue to drain enough to execute it (indicating we are caught up
    // at least with the time we entered this function).
    uint256 last_block_hash = WITH_LOCK(cs_wallet, return m_last_block_processed);
    chain().waitForNotificationsIfTipChanged(last_block_hash);
}


isminetype CWallet::IsMine(const CTxInput& input) const
{
    AssertLockHeld(cs_wallet);
    if (input.IsMWEB()) {
        mw::Coin coin;
        return GetCoin(input.ToMWEB(), coin) && coin.IsMine() ? ISMINE_SPENDABLE : ISMINE_NO;
    }

    const CWalletTx* prev = FindPrevTx(input);
    if (prev != nullptr)
    {
        const CTxIn& txin = input.GetTxIn();
        if (txin.prevout.n < prev->tx->vout.size())
            return IsMine(prev->tx->GetOutput(txin.prevout.n));
    }
    return ISMINE_NO;
}

// Note that this function doesn't distinguish between a 0-valued input,
// and a not-"is mine" (according to the filter) input.
CAmount CWallet::GetDebit(const CTxInput& input, const isminefilter& filter) const
{
    {
        LOCK(cs_wallet);
        if (input.IsMWEB()) {
            mw::Coin coin;
            if ((filter & ISMINE_SPENDABLE) && GetCoin(input.ToMWEB(), coin) && coin.IsMine()) {
                return coin.amount;
            }

            return 0;
        }

        const CWalletTx* prev = FindPrevTx(input);
        if (prev != nullptr) {
            const CTxIn& txin = input.GetTxIn();
            if (txin.prevout.n < prev->tx->vout.size()) {
                if (IsMine(prev->tx->GetOutput(txin.prevout.n)) & filter) {
                    return prev->tx->vout[txin.prevout.n].nValue;
                }
            }
        }
    }
    return 0;
}

isminetype CWallet::IsMine(const CTxOutput& output) const
{
    AssertLockHeld(cs_wallet);
    if (output.IsMWEB()) {
        mw::Coin coin;
        return GetCoin(output.ToMWEB(), coin) && coin.IsMine() ? ISMINE_SPENDABLE : ISMINE_NO;
    }

    return IsMine(DestinationAddr(output.GetScriptPubKey()));
}

isminetype CWallet::IsMine(const CTxDestination& dest) const
{
    AssertLockHeld(cs_wallet);
    return IsMine(DestinationAddr(dest));
}

isminetype CWallet::IsMine(const DestinationAddr& script) const
{
    AssertLockHeld(cs_wallet);
    isminetype result = ISMINE_NO;
    for (const auto& spk_man_pair : m_spk_managers) {
        result = std::max(result, spk_man_pair.second->IsMine(script));
    }
    return result;
}

CAmount CWallet::GetCredit(const CTxOutput& output, const isminefilter& filter) const
{
    const CAmount amount = GetValue(output);
    if (!MoneyRange(amount))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    LOCK(cs_wallet);
    return ((IsMine(output) & filter) ? amount : 0);
}

bool CWallet::IsChange(const CTxOutput& output) const
{
    if (output.IsMWEB()) {
        mw::Coin coin;
        if (GetCoin(output.ToMWEB(), coin)) {
            return coin.IsChange();
        }

        return false;
    }

    return IsChange(output.GetScriptPubKey());
}

bool CWallet::IsChange(const DestinationAddr& script) const
{
    if (script.IsMWEB()) {
        return GetMWWallet()->IsChange(script.GetMWEBAddress());
    }

    // TODO: fix handling of 'change' outputs. The assumption is that any
    // payment to a script that is ours, but is not in the address book
    // is change. That assumption is likely to break when we implement multisignature
    // wallets that return change back into a multi-signature-protected address;
    // a better way of identifying which outputs are 'the send' and which are
    // 'the change' will need to be implemented (maybe extend CWalletTx to remember
    // which output, if any, was change).
    AssertLockHeld(cs_wallet);
    if (IsMine(script))
    {
        CTxDestination address;
        if (!script.ExtractDestination(address))
            return true;
        if (!FindAddressBookEntry(address)) {
            return true;
        }
    }
    return false;
}

CAmount CWallet::GetChange(const CTxOutput& output) const
{
    AssertLockHeld(cs_wallet);
    const CAmount amount = GetValue(output);
    if (!MoneyRange(amount))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    return (IsChange(output) ? amount : 0);
}

bool CWallet::IsMine(const CTransaction& tx, const boost::optional<MWEB::WalletTxInfo>& mweb_wtx_info) const
{
    AssertLockHeld(cs_wallet);
    for (const CTxOutput& txout : tx.GetOutputs())
        if (IsMine(txout))
            return true;

    for (const PegOutCoin& pegout : tx.mweb_tx.GetPegOuts()) {
        if (IsMine(DestinationAddr{pegout.GetScriptPubKey()})) {
            return true;
        }
    }

    if (mweb_wtx_info && mweb_wtx_info->received_coin) {
        if (mweb_wtx_info->received_coin->IsMine()) {
            return true;
        }
    }

    return false;
}

bool CWallet::IsFromMe(const CTransaction& tx, const boost::optional<MWEB::WalletTxInfo>& mweb_wtx_info) const
{
    return (GetDebit(tx, mweb_wtx_info, ISMINE_ALL) > 0);
}

CAmount CWallet::GetDebit(const CTransaction& tx, const boost::optional<MWEB::WalletTxInfo>& mweb_wtx_info, const isminefilter& filter) const
{
    CAmount nDebit = 0;
    for (const CTxInput& txin : tx.GetInputs())
    {
        nDebit += GetDebit(txin, filter);
        if (!MoneyRange(nDebit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }

    if (mweb_wtx_info && mweb_wtx_info->spent_input) {
        nDebit += GetDebit(CTxInput{*mweb_wtx_info->spent_input}, filter);
        if (!MoneyRange(nDebit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    
    return nDebit;
}

bool CWallet::IsAllFromMe(const CTransaction& tx, const isminefilter& filter) const
{
    LOCK(cs_wallet);

    for (const CTxInput& txin : tx.GetInputs())
    {
        if (!(IsMine(txin) & filter))
            return false;
    }
    return true;
}

CAmount CWallet::GetCredit(const CTransaction& tx, const boost::optional<MWEB::WalletTxInfo>& mweb_wtx_info, const isminefilter& filter) const
{
    CAmount nCredit = 0;
    for (const CTxOutput& txout : tx.GetOutputs())
    {
        nCredit += GetCredit(txout, filter);
        if (!MoneyRange(nCredit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }

    if (mweb_wtx_info && mweb_wtx_info->received_coin) {
        nCredit += GetCredit(CTxOutput{mweb_wtx_info->received_coin->output_id}, filter);
        if (!MoneyRange(nCredit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }

    bool has_my_inputs = false;
    for (const CTxInput& txin : tx.GetInputs()) {
        LOCK(cs_wallet);
        if (IsMine(txin)) {
            has_my_inputs = true;
            break;
        }
    }

    if (!has_my_inputs) {
        for (const PegOutCoin& pegout : tx.mweb_tx.GetPegOuts()) {
            LOCK(cs_wallet);
            if (!(IsMine(DestinationAddr(pegout.GetScriptPubKey())) & filter)) {
                nCredit += pegout.GetAmount();
                if (!MoneyRange(nCredit))
                    throw std::runtime_error(std::string(__func__) + ": value out of range");
            }
        }
    }

    return nCredit;
}

CAmount CWallet::GetChange(const CTransaction& tx, const boost::optional<MWEB::WalletTxInfo>& mweb_wtx_info) const
{
    LOCK(cs_wallet);
    CAmount nChange = 0;
    for (const CTxOutput& txout : tx.GetOutputs())
    {
        nChange += GetChange(txout);
        if (!MoneyRange(nChange))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }

    if (mweb_wtx_info && mweb_wtx_info->received_coin) {
        nChange += GetChange(CTxOutput{mweb_wtx_info->received_coin->output_id});
        if (!MoneyRange(nChange))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }

    for (const PegOutCoin& pegout : tx.mweb_tx.GetPegOuts()) {
        LOCK(cs_wallet);
        if (IsChange(DestinationAddr{pegout.GetScriptPubKey()})) {
            nChange += pegout.GetAmount();
            if (!MoneyRange(nChange))
                throw std::runtime_error(std::string(__func__) + ": value out of range");
        
        }
    }

    return nChange;
}

bool CWallet::IsHDEnabled() const
{
    // All Active ScriptPubKeyMans must be HD for this to be true
    bool result = true;
    for (const auto& spk_man : GetActiveScriptPubKeyMans()) {
        result &= spk_man->IsHDEnabled();
    }
    return result;
}

bool CWallet::CanGetAddresses(bool internal) const
{
    LOCK(cs_wallet);
    if (m_spk_managers.empty()) return false;
    for (OutputType t : OUTPUT_TYPES) {
        auto spk_man = GetScriptPubKeyMan(t, internal);
        if (spk_man && spk_man->CanGetAddresses(t == OutputType::MWEB ? KeyPurpose::MWEB : (internal ? KeyPurpose::INTERNAL : KeyPurpose::EXTERNAL))) {
            return true;
        }
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

bool CWallet::AddWalletFlags(uint64_t flags)
{
    LOCK(cs_wallet);
    // We should never be writing unknown non-tolerable wallet flags
    assert(((flags & KNOWN_WALLET_FLAGS) >> 32) == (flags >> 32));
    if (!WalletBatch(*database).WriteWalletFlags(flags)) {
        throw std::runtime_error(std::string(__func__) + ": writing wallet flags failed");
    }

    return LoadWalletFlags(flags);
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

bool CWallet::ImportScriptPubKeys(const std::string& label, const std::set<DestinationAddr>& script_pub_keys, const bool have_solving_data, const bool apply_label, const int64_t timestamp)
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
        for (const DestinationAddr& script : script_pub_keys) {
            CTxDestination dest;
            script.ExtractDestination(dest);
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
    return GetVirtualTransactionSize(CTransaction(txNew));
}

int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* wallet, bool use_max_sig)
{
    CMutableTransaction txn;
    txn.vin.push_back(CTxIn(COutPoint()));
    if (!wallet->DummySignInput(txn.vin[0], txout, use_max_sig)) {
        return -1;
    }
    return GetVirtualTransactionInputSize(txn.vin[0]);
}

void CWalletTx::GetAmounts(std::list<COutputEntry>& listReceived,
                           std::list<COutputEntry>& listSent, CAmount& nFee, const isminefilter& filter) const
{
    nFee = GetFee(filter);
    listReceived.clear();
    listSent.clear();

    // Compute fee:
    CAmount nDebit = GetDebit(filter);

    LOCK(pwallet->cs_wallet);
    // Sent/received.
    for (const CTxOutput& txout : tx->GetOutputs())
    {
        if (!txout.IsMWEB() && txout.GetScriptPubKey().IsMWEBPegin()) {
            continue;
        }

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

        if (!pwallet->ExtractOutputDestination(txout, address) && (txout.IsMWEB() || !txout.GetScriptPubKey().IsUnspendable()))
        {
            pwallet->WalletLogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
                                    this->GetHash().ToString());
            address = CNoDestination();
        }

        COutputEntry output = {address, pwallet->GetValue(txout), txout.GetIndex()};

        // If we are debited by the transaction, add the output as a "sent" entry
        if (nDebit > 0)
            listSent.push_back(output);

        // If we are receiving the output, add it as a "received" entry
        if (fIsMine & filter)
            listReceived.push_back(output);
    }

    // MWEB: Treat pegouts as outputs
    for (const PegOutCoin& pegout : tx->mweb_tx.GetPegOuts()) {
        DestinationAddr dest_addr{pegout.GetScriptPubKey()};

        isminetype fIsMine = pwallet->IsMine(dest_addr);
        // Only need to handle txouts if AT LEAST one of these is true:
        //   1) they debit from us (sent)
        //   2) the output is to us (received)
        if (nDebit > 0) {
            // Don't report 'change' txouts
            if (pwallet->IsChange(dest_addr))
                continue;
        } else if (!(fIsMine & filter))
            continue;

        CTxDestination address;
        dest_addr.ExtractDestination(address);

        COutputEntry output = {address, pegout.GetAmount(), mw::Hash()};

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
CWallet::ScanResult CWallet::ScanForWalletTransactions(const uint256& start_block, int start_height, Optional<int> max_height, const WalletRescanReserver& reserver, bool fUpdate)
{
    int64_t nNow = GetTime();
    int64_t start_time = GetTimeMillis();

    assert(reserver.isReserved());

    uint256 block_hash = start_block;
    ScanResult result;

    WalletLogPrintf("Rescan started from block %s...\n", start_block.ToString());

    fAbortRescan = false;
    ShowProgress(strprintf("%s " + _("Rescanning...").translated, GetDisplayName()), 0); // show rescan progress in GUI as dialog or on splashscreen, if -rescan on startup
    uint256 tip_hash = WITH_LOCK(cs_wallet, return GetLastBlockHash());
    uint256 end_hash = tip_hash;
    if (max_height) chain().findAncestorByHeight(tip_hash, *max_height, FoundBlock().hash(end_hash));
    double progress_begin = chain().guessVerificationProgress(block_hash);
    double progress_end = chain().guessVerificationProgress(end_hash);
    double progress_current = progress_begin;
    int block_height = start_height;
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

        CBlock block;
        bool next_block;
        uint256 next_block_hash;
        bool reorg = false;
        if (chain().findBlock(block_hash, FoundBlock().data(block)) && !block.IsNull()) {
            LOCK(cs_wallet);
            next_block = chain().findNextBlock(block_hash, block_height, FoundBlock().hash(next_block_hash), &reorg);
            if (reorg) {
                // Abort scan if current block is no longer active, to prevent
                // marking transactions as coming from the wrong block.
                // TODO: This should return success instead of failure, see
                // https://github.com/bitcoin/bitcoin/pull/14711#issuecomment-458342518
                result.last_failed_block = block_hash;
                result.status = ScanResult::FAILURE;
                break;
            }
            for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
                SyncTransaction(block.vtx[posInBlock], boost::none, {CWalletTx::Status::CONFIRMED, block_height, block_hash, (int)posInBlock}, fUpdate);
            }

            if (!block.mweb_block.IsNull()) {
                for (const Kernel& kernel : block.mweb_block.m_block->GetKernels()) {
                    const CWalletTx* wtx = FindWalletTxByKernelId(kernel.GetKernelID());
                    if (wtx) {
                        SyncTransaction(
                            wtx->tx,
                            wtx->mweb_wtx_info,
                            {CWalletTx::Status::CONFIRMED, block_height, block_hash, wtx->m_confirm.nIndex},
                            fUpdate
                        );
                    }
                }

                mw::Coin mweb_coin;
                for (const Output& output : block.mweb_block.m_block->GetOutputs()) {
                    if (mweb_wallet->RewindOutput(output, mweb_coin)) {
                        const CWalletTx* wtx = FindWalletTx(mweb_coin.output_id);
                        if (wtx) {
                            SyncTransaction(
                                wtx->tx,
                                wtx->mweb_wtx_info,
                                {CWalletTx::Status::CONFIRMED, block_height, block_hash, wtx->m_confirm.nIndex},
                                fUpdate
                            );
                        } else {
                            AddToWallet(
                                MakeTransactionRef(),
                                boost::make_optional<MWEB::WalletTxInfo>(mweb_coin),
                                {CWalletTx::Status::CONFIRMED, block_height, block_hash, 0},
                                nullptr,
                                false
                            );
                        }
                    }
                }

                for (const mw::Hash& spent_id : block.mweb_block.GetSpentIDs()) {
                    if (IsMine(CTxInput(spent_id))) {
                        auto spend_iter = mapTxSpends.find(spent_id);
                        if (spend_iter != mapTxSpends.end()) {
                            auto tx_iter = mapWallet.find(spend_iter->second);
                            if (tx_iter != mapWallet.end()) {
                                SyncTransaction(
                                    tx_iter->second.tx,
                                    tx_iter->second.mweb_wtx_info,
                                    {CWalletTx::Status::CONFIRMED, block_height, block_hash, tx_iter->second.m_confirm.nIndex},
                                    fUpdate
                                );
                            }
                        } else {
                            AddToWallet(
                                MakeTransactionRef(),
                                boost::make_optional<MWEB::WalletTxInfo>(spent_id),
                                {CWalletTx::Status::CONFIRMED, block_height, block_hash, 0},
                                nullptr,
                                false
                            );
                        }

                        CWalletTx* prev = FindPrevTx(spent_id);
                        if (prev != nullptr) {
                            prev->MarkDirty();
                        }
                    }
                }
            }

            // scan succeeded, record block as most recent successfully scanned
            result.last_scanned_block = block_hash;
            result.last_scanned_height = block_height;
        } else {
            // could not scan block, keep scanning but record this block as the most recent failure
            result.last_failed_block = block_hash;
            result.status = ScanResult::FAILURE;
            next_block = chain().findNextBlock(block_hash, block_height, FoundBlock().hash(next_block_hash), &reorg);
        }
        if (max_height && block_height >= *max_height) {
            break;
        }
        {
            if (!next_block || reorg) {
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

        if (!wtx.IsCoinBase() && !wtx.IsHogEx() && (nDepth == 0 && !wtx.isAbandoned())) {
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

bool CWalletTx::SubmitMemoryPoolAndRelay(std::string& err_string, bool relay)
{
    // Can't relay if wallet is not broadcasting
    if (!pwallet->GetBroadcastTransactions()) return false;
    // Don't relay abandoned transactions
    if (isAbandoned()) return false;
    // Don't try to submit coinbase transactions. These would fail anyway but would
    // cause log spam.
    if (IsCoinBase()) return false;
    // Don't try to submit conflicted or confirmed transactions.
    if (GetDepthInMainChain() != 0) return false;

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
        amount.Set(filter, (type == DEBIT) ? pwallet->GetDebit(*tx, mweb_wtx_info, filter) : pwallet->GetCredit(*tx, mweb_wtx_info, filter));
        m_is_cache_empty = false;
    }
    return amount.m_value[filter];
}

CAmount CWalletTx::GetDebit(const isminefilter& filter) const
{
    if (GetInputs().empty())
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
    if (IsImmature())
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
    if (IsImmature() && IsInMainChain()) {
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
    if (IsImmature())
        return 0;

    if (fUseCache && allow_cache && m_amounts[AVAILABLE_CREDIT].m_cached[filter]) {
        return m_amounts[AVAILABLE_CREDIT].m_value[filter];
    }

    bool allow_used_addresses = (filter & ISMINE_USED) || !pwallet->IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE);
    CAmount nCredit = 0;
    for (const CTxOutput& output : GetOutputs())
    {
        if (!pwallet->IsSpent(output.GetIndex()) && (allow_used_addresses || !pwallet->IsSpentKey(output))) {
            nCredit += pwallet->GetCredit(output, filter);
            if (!MoneyRange(nCredit))
                throw std::runtime_error(std::string(__func__) + " : value out of range");
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
    if (IsImmature() && IsInMainChain()) {
        return GetCachableAmount(IMMATURE_CREDIT, ISMINE_WATCH_ONLY, !fUseCache);
    }

    return 0;
}

CAmount CWalletTx::GetChange() const
{
    if (fChangeCached)
        return nChangeCached;
    nChangeCached = pwallet->GetChange(*tx, mweb_wtx_info);
    fChangeCached = true;
    return nChangeCached;
}

CAmount CWalletTx::GetFee(const isminefilter& filter) const
{
    CAmount nFee = 0;

    CAmount nDebit = GetDebit(filter);
    // debit>0 means we signed/sent this transaction
    if (nDebit > 0) {
        CAmount nValueOut = 0;
        for (const CTxOutput& output : GetOutputs()) {
            if (!IsPegInOutput(output)) {
                nValueOut += pwallet->GetValue(output);
            }
        }

        for (const PegOutCoin& pegout : tx->mweb_tx.GetPegOuts()) {
            nValueOut += pegout.GetAmount();
        }

        nFee = nDebit - nValueOut;
    }

    return nFee;
}

bool CWalletTx::InMempool() const
{
    return fInMempool;
}

bool CWalletTx::IsTrusted() const
{
    std::set<uint256> trusted_parents;
    LOCK(pwallet->cs_wallet);
    return pwallet->IsTrusted(*this, trusted_parents);
}

bool CWallet::IsTrusted(const CWalletTx& wtx, std::set<uint256>& trusted_parents) const
{
    AssertLockHeld(cs_wallet);
    // Quick answer in most cases
    if (!chain().checkFinalTx(*wtx.tx)) return false;
    int nDepth = wtx.GetDepthInMainChain();
    if (nDepth >= 1) return true;
    if (nDepth < 0) return false;

    // If the HogEx is not in the main chain, then we should assume it has been replaced during a reorg.
    if (wtx.IsHogEx()) return false;

    // using wtx's cached debit
    if (!m_spend_zero_conf_change || !wtx.IsFromMe(ISMINE_ALL)) return false;

    // Don't trust unconfirmed transactions from us unless they are in the mempool.
    if (!wtx.InMempool()) return false;

    // Trusted if all inputs are from us and are in the mempool:
    for (const CTxInput& input : wtx.tx->GetInputs())
    {
        // Transactions not sent by us: not trusted
        const CWalletTx* parent = FindPrevTx(input);
        if (parent == nullptr) return false;
        CTxOutput parentOut = parent->tx->GetOutput(input.GetIndex());
        // Check that this specific input being spent is trusted
        if (IsMine(parentOut) != ISMINE_SPENDABLE) return false;
        // If we've already trusted this parent, continue
        if (trusted_parents.count(parent->GetHash())) continue;
        // Recurse to check that the parent is also trusted
        if (!IsTrusted(*parent, trusted_parents)) return false;
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
    // resend 12-36 hours from now, ~1 day on average.
    nNextResend = GetTime() + (12 * 60 * 60) + GetRand(24 * 60 * 60);
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


CWallet::Balance CWallet::GetBalance(const int min_depth, bool avoid_reuse) const
{
    Balance ret;
    isminefilter reuse_filter = avoid_reuse ? ISMINE_NO : ISMINE_USED;
    {
        LOCK(cs_wallet);
        std::set<uint256> trusted_parents;
        for (const auto& entry : mapWallet)
        {
            const CWalletTx& wtx = entry.second;
            const bool is_trusted{IsTrusted(wtx, trusted_parents)};
            const int tx_depth{wtx.GetDepthInMainChain()};
            const CAmount tx_credit_mine{wtx.GetAvailableCredit(/* fUseCache */ true, ISMINE_SPENDABLE | reuse_filter)};
            const CAmount tx_credit_watchonly{wtx.GetAvailableCredit(/* fUseCache */ true, ISMINE_WATCH_ONLY | reuse_filter)};
            if (is_trusted && tx_depth >= min_depth) {
                ret.m_mine_trusted += tx_credit_mine;
                ret.m_watchonly_trusted += tx_credit_watchonly;
            }
            if (!is_trusted && tx_depth == 0 && wtx.InMempool()) {
                ret.m_mine_untrusted_pending += tx_credit_mine;
                ret.m_watchonly_untrusted_pending += tx_credit_watchonly;
            }
            ret.m_mine_immature += wtx.GetImmatureCredit();
            ret.m_watchonly_immature += wtx.GetImmatureWatchOnlyCredit();
        }
    }
    return ret;
}

CAmount CWallet::GetAvailableBalance(const CCoinControl* coinControl) const
{
    LOCK(cs_wallet);

    CAmount balance = 0;
    std::vector<COutputCoin> vCoins;
    AvailableCoins(vCoins, true, coinControl);
    for (const COutputCoin& output_coin : vCoins) {
        if (output_coin.IsSpendable()) {
            balance += output_coin.GetValue();
        }
    }
    return balance;
}

void CWallet::AvailableCoins(std::vector<COutputCoin>& vCoins, bool fOnlySafe, const CCoinControl* coinControl, const CAmount& nMinimumAmount, const CAmount& nMaximumAmount, const CAmount& nMinimumSumAmount, const uint64_t nMaximumCount) const
{
    AssertLockHeld(cs_wallet);

    vCoins.clear();
    CAmount nTotal = 0;
    // Either the WALLET_FLAG_AVOID_REUSE flag is not set (in which case we always allow), or we default to avoiding, and only in the case where
    // a coin control object is provided, and has the avoid address reuse flag set to false, do we allow already used addresses
    bool allow_used_addresses = !IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE) || (coinControl && !coinControl->m_avoid_address_reuse);
    const int min_depth = {coinControl ? coinControl->m_min_depth : DEFAULT_MIN_DEPTH};
    const int max_depth = {coinControl ? coinControl->m_max_depth : DEFAULT_MAX_DEPTH};

    std::set<uint256> trusted_parents;
    for (const auto& entry : mapWallet)
    {
        const CWalletTx& wtx = entry.second;

        if (!chain().checkFinalTx(*wtx.tx)) {
            continue;
        }

        if (wtx.IsImmature())
            continue;

        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < 0)
            continue;

        // We should not consider coins which aren't at least in our mempool
        // It's possible for these to be conflicted via ancestors which we may never be able to detect
        if (nDepth == 0 && !wtx.InMempool())
            continue;

        bool safeTx = IsTrusted(wtx, trusted_parents);

        // We should not consider coins from transactions that are replacing
        // other transactions.
        //
        // Example: There is a transaction A which is replaced by bumpfee
        // transaction B. In this case, we want to prevent creation of
        // a transaction B' which spends an output of B.
        //
        // Reason: If transaction A were initially confirmed, transactions B
        // and B' would no longer be valid, so the user would have to create
        // a new transaction C to replace B'. However, in the case of a
        // one-block reorg, transactions B' and C might BOTH be accepted,
        // when the user only wanted one of them. Specifically, there could
        // be a 1-block reorg away from the chain where transactions A and C
        // were accepted to another chain where B, B', and C were all
        // accepted.
        if (nDepth == 0 && wtx.mapValue.count("replaces_txid")) {
            safeTx = false;
        }

        // Similarly, we should not consider coins from transactions that
        // have been replaced. In the example above, we would want to prevent
        // creation of a transaction A' spending an output of A, because if
        // transaction B were initially confirmed, conflicting with A and
        // A', we wouldn't want to the user to create a transaction D
        // intending to replace A', but potentially resulting in a scenario
        // where A, A', and D could all be accepted (instead of just B and
        // D, or just A and A' like the user would want).
        if (nDepth == 0 && wtx.mapValue.count("replaced_by_txid")) {
            safeTx = false;
        }

        if (fOnlySafe && !safeTx) {
            continue;
        }

        if (nDepth < min_depth || nDepth > max_depth) {
            continue;
        }

        for (const CTxOutput& output : wtx.GetOutputs()) {
            if (coinControl && ((output.IsMWEB() && coinControl->fPegIn) || (!output.IsMWEB() && coinControl->fPegOut)))
                continue;

            // Only consider selected coins if add_inputs is false
            if (coinControl && !coinControl->m_add_inputs && !coinControl->IsSelected(output.GetIndex())) {
                continue;
            }

            CAmount value = GetValue(output);
            if (value < nMinimumAmount || value > nMaximumAmount)
                continue;

            if (coinControl && coinControl->HasSelected() && !coinControl->fAllowOtherInputs && !coinControl->IsSelected(output.GetIndex()))
                continue;

            if (IsLockedCoin(output.GetIndex()))
                continue;

            if (IsSpent(output.GetIndex()))
                continue;

            isminetype mine = IsMine(output);

            if (mine == ISMINE_NO) {
                continue;
            }

            if (!allow_used_addresses && IsSpentKey(output)) {
                continue;
            }

            std::unique_ptr<SigningProvider> provider = output.IsMWEB() ? nullptr : GetSolvingProvider(output.GetScriptPubKey());

            bool solvable = provider ? IsSolvable(*provider, output.GetScriptPubKey()) : output.IsMWEB();
            bool spendable = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) || (((mine & ISMINE_WATCH_ONLY) != ISMINE_NO) && (coinControl && coinControl->fAllowWatchOnly && solvable));

            if (output.IsMWEB()) {
                mw::Coin coin;
                if (!GetCoin(output.ToMWEB(), coin) || !coin.IsMine()) {
                    continue;
                }

                StealthAddress address;
                if (!mweb_wallet->GetStealthAddress(coin, address)) {
                    continue;
                }

                vCoins.push_back(MWOutput{coin, nDepth, address, &wtx});
            } else {
                size_t i = boost::get<COutPoint>(output.GetIndex()).n;
                vCoins.push_back(COutput(&wtx, i, nDepth, spendable, solvable, safeTx, (coinControl && coinControl->fAllowWatchOnly)));
            }

            // Checks the sum amount of all UTXO's.
            if (nMinimumSumAmount != MAX_MONEY) {
                nTotal += value;

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

std::map<CTxDestination, std::vector<COutputCoin>> CWallet::ListCoins() const
{
    AssertLockHeld(cs_wallet);

    std::map<CTxDestination, std::vector<COutputCoin>> result;
    std::vector<COutputCoin> availableCoins;

    AvailableCoins(availableCoins);

    for (const COutputCoin& coin : availableCoins) {
        CTxDestination address;
        if ((coin.IsSpendable() || (IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && coin.IsSolvable())) &&
            ExtractOutputDestination(FindNonChangeParentOutput(*coin.GetWalletTx()->tx, coin.GetIndex()), address)) {
            result[address].emplace_back(std::move(coin));
        }
    }

    std::vector<OutputIndex> lockedCoins;
    ListLockedCoins(lockedCoins);
    // Include watch-only for LegacyScriptPubKeyMan wallets without private keys
    const bool include_watch_only = GetLegacyScriptPubKeyMan() && IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    const isminetype is_mine_filter = include_watch_only ? ISMINE_WATCH_ONLY : ISMINE_SPENDABLE;
    for (const OutputIndex& output_idx : lockedCoins) {
        const CWalletTx* wtx = FindWalletTx(output_idx);
        if (wtx != nullptr) {
            int depth = wtx->GetDepthInMainChain();
            if (depth >= 0 && IsMine(wtx->tx->GetOutput(output_idx)) == is_mine_filter) {
                CTxDestination address;
                if (ExtractOutputDestination(FindNonChangeParentOutput(*wtx->tx, output_idx), address)) {
                    if (output_idx.type() == typeid(mw::Hash)) {
                        mw::Coin coin;
                        if (GetCoin(boost::get<mw::Hash>(output_idx), coin) && coin.IsMine() && coin.HasSpendKey()) {
                            StealthAddress stealth_address;
                            mweb_wallet->GetStealthAddress(coin, stealth_address);
                            result[address].emplace_back(MWOutput{coin, depth, stealth_address, wtx});
                        }
                    } else {
                        result[address].emplace_back(
                            COutput(wtx, boost::get<COutPoint>(output_idx).n, depth, true /* spendable */, true /* solvable */, false /* safe */));
                    }
                }
            }
        }
    }

    return result;
}

CTxOutput CWallet::FindNonChangeParentOutput(const CTransaction& tx, const OutputIndex& output_idx) const
{
    AssertLockHeld(cs_wallet);
    const CTransaction* ptx = &tx;
    OutputIndex idx = output_idx;
    while (IsChange(ptx->GetOutput(idx)) && ptx->GetInputs().size() > 0) {
        CTxInput input = ptx->GetInputs().front();
        const CWalletTx* wtx = FindPrevTx(input);
        if (wtx == nullptr || !IsMine(wtx->tx->GetOutput(input.GetIndex()))) {
            break;
        }

        ptx = wtx->tx.get();
        idx = input.GetIndex();
    }

    return ptx->GetOutput(idx);
}

bool CWallet::SelectCoinsMinConf(const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, std::vector<OutputGroup> groups,
                                 std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CoinSelectionParams& coin_selection_params, bool& bnb_used) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    std::vector<OutputGroup> utxo_pool;
    if (coin_selection_params.use_bnb) {
        // Calculate cost of change
        size_t mweb_change_spend_weight = 0; // MWEB inputs are weightless
        CAmount cost_of_change = coin_selection_params.m_discard_feerate.GetTotalFee(coin_selection_params.change_spend_size, mweb_change_spend_weight)
            + coin_selection_params.m_effective_feerate.GetTotalFee(coin_selection_params.change_output_size, coin_selection_params.mweb_change_output_weight);

        // Filter by the min conf specs and add to utxo_pool and calculate effective value
        for (OutputGroup& group : groups) {
            if (!group.EligibleForSpending(eligibility_filter, coin_selection_params.input_preference)) continue;

            if (coin_selection_params.m_subtract_fee_outputs) {
                // Set the effective feerate to 0 as we don't want to use the effective value since the fees will be deducted from the output
                group.SetFees(CFeeRate(0) /* effective_feerate */, coin_selection_params.m_long_term_feerate);
            } else {
                group.SetFees(coin_selection_params.m_effective_feerate, coin_selection_params.m_long_term_feerate);
            }

            OutputGroup pos_group = group.GetPositiveOnlyGroup();
            if (pos_group.effective_value > 0) utxo_pool.push_back(pos_group);
        }
        // Calculate the fees for things that aren't inputs
        CAmount not_input_fees = coin_selection_params.m_effective_feerate.GetTotalFee(coin_selection_params.tx_noinputs_size, coin_selection_params.mweb_nochange_weight);
        bnb_used = true;
        return SelectCoinsBnB(utxo_pool, nTargetValue, cost_of_change, setCoinsRet, nValueRet, not_input_fees);
    } else {
        // Filter by the min conf specs and add to utxo_pool
        for (const OutputGroup& group : groups) {
            if (!group.EligibleForSpending(eligibility_filter, coin_selection_params.input_preference)) continue;
            utxo_pool.push_back(group);
        }
        bnb_used = false;
        return KnapsackSolver(nTargetValue, utxo_pool, setCoinsRet, nValueRet);
    }
}

bool CWallet::SelectCoins(const std::vector<COutputCoin>& vAvailableCoins, const CAmount& nTargetValue, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CCoinControl& coin_control, CoinSelectionParams& coin_selection_params, bool& bnb_used) const
{
    std::vector<COutputCoin> vCoins(vAvailableCoins);
    CAmount value_to_select = nTargetValue;

    // Default to bnb was not used. If we use it, we set it later
    bnb_used = false;

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coin_control.HasSelected() && !coin_control.fAllowOtherInputs)
    {
        for (const COutputCoin& out : vCoins)
        {
            if (!out.IsSpendable())
                 continue;
            nValueRet += out.GetValue();
            setCoinsRet.insert(out.GetInputCoin());
        }
        return (nValueRet >= nTargetValue);
    }

    // calculate value from preset inputs and store them
    std::set<CInputCoin> setPresetCoins;
    CAmount nValueFromPresetInputs = 0;

    std::vector<OutputIndex> vPresetInputs;
    coin_control.ListSelected(vPresetInputs);
    for (const OutputIndex& idx : vPresetInputs)
    {
        if (idx.type() == typeid(mw::Hash)) {
            mw::Coin mweb_coin;
            if (!GetCoin(boost::get<mw::Hash>(idx), mweb_coin) || !mweb_coin.IsMine()) {
                return false;
            }

            CInputCoin coin(mweb_coin);
            coin.effective_value = coin.GetAmount() - coin.CalculateFee(coin_selection_params.m_effective_feerate);
            if (coin_selection_params.use_bnb) {
                value_to_select -= coin.effective_value;
            } else {
                value_to_select -= coin.GetAmount();
            }

            nValueFromPresetInputs += coin.GetAmount();
            setPresetCoins.insert(coin);
            continue;
        }

        const COutPoint& outpoint = boost::get<COutPoint>(idx);
        std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it != mapWallet.end())
        {
            const CWalletTx& wtx = it->second;
            // Clearly invalid input, fail
            if (wtx.tx->vout.size() <= outpoint.n) {
                return false;
            }
            // Just to calculate the marginal byte size
            CInputCoin coin(wtx.tx, outpoint.n, wtx.GetSpendSize(outpoint.n, false));
            nValueFromPresetInputs += coin.GetAmount();
            if (coin.m_input_bytes <= 0) {
                return false; // Not solvable, can't estimate size for fee
            }
            coin.effective_value = coin.GetAmount() - coin.CalculateFee(coin_selection_params.m_effective_feerate);
            if (coin_selection_params.use_bnb) {
                value_to_select -= coin.effective_value;
            } else {
                value_to_select -= coin.GetAmount();
            }
            setPresetCoins.insert(coin);
        } else {
            return false; // TODO: Allow non-wallet inputs
        }
    }

    // remove preset inputs from vCoins
    for (std::vector<COutputCoin>::iterator it = vCoins.begin(); it != vCoins.end() && coin_control.HasSelected();)
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
        SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(1, 6, 0), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used) ||
        SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(1, 1, 0), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, 2), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, std::min((size_t)4, max_ancestors/3), std::min((size_t)4, max_descendants/3)), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, max_ancestors/2, max_descendants/2), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, max_ancestors-1, max_descendants-1), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change && !fRejectLongChains && SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, std::numeric_limits<uint64_t>::max()), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used));

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
        coins[input.prevout] = Coin(wtx.tx->vout[input.prevout.n], wtx.m_confirm.block_height, wtx.IsCoinBase(), wtx.IsHogEx());
    }
    std::map<int, std::string> input_errors;
    return SignTransaction(tx, coins, SIGHASH_ALL, input_errors);
}

bool CWallet::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, std::string>& input_errors) const
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

TransactionError CWallet::FillPSBT(PartiallySignedTransaction& psbtx, bool& complete, int sighash_type, bool sign, bool bip32derivs, size_t * n_signed) const
{
    if (n_signed) {
        *n_signed = 0;
    }
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
        TransactionError res = spk_man->FillPSBT(psbtx, sighash_type, sign, bip32derivs, &n_signed_this_spkm);
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

    CTransactionRef tx_new;
    FeeCalculation fee_calc_out;
    if (!CreateTransaction(vecSend, tx_new, nFeeRet, nChangePosInOut, error, coinControl, fee_calc_out, false)) {
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

        }
        if (lockUnspents) {
            LockCoin(txin.prevout);
        }

    }

    return true;
}

bool CWallet::CreateTransaction(
        const std::vector<CRecipient>& vecSend,
        CTransactionRef& tx,
        CAmount& nFeeRet,
        int& nChangePosInOut,
        bilingual_str& error,
        const CCoinControl& coin_control,
        FeeCalculation& fee_calc_out,
        bool sign)
{
    int nChangePosIn = nChangePosInOut;

    Optional<AssembledTx> tx1 = TxAssembler(*this).AssembleTx(vecSend, coin_control, nChangePosIn, sign, error);
    if (tx1) {
        tx = tx1->tx;
        nFeeRet = tx1->fee;
        nChangePosInOut = tx1->change_position;
        fee_calc_out = tx1->fee_calc;
    }

    // try with avoidpartialspends unless it's enabled already
    if (tx1 && tx1->fee > 0 /* 0 means non-functional fee rate estimation */ && m_max_aps_fee > -1 && !coin_control.m_avoid_partial_spends) {
        CCoinControl tmp_cc = coin_control;
        tmp_cc.m_avoid_partial_spends = true;
        bilingual_str error2; // fired and forgotten; if an error occurs, we discard the results

        Optional<AssembledTx> tx2 = TxAssembler(*this).AssembleTx(vecSend, tmp_cc, nChangePosIn, sign, error2);
        if (tx2) {
            // if fee of this alternative one is within the range of the max fee, we use this one
            const bool use_aps = tx2->fee <= tx1->fee + m_max_aps_fee;
            WalletLogPrintf("Fee non-grouped = %lld, grouped = %lld, using %s\n", tx1->fee, tx2->fee, use_aps ? "grouped" : "non-grouped");
            if (use_aps) {
                tx = tx2->tx;
                nFeeRet = tx2->fee;
                nChangePosInOut = tx2->change_position;
                fee_calc_out = tx2->fee_calc;
            }
        }
    }

    return !!tx1;
}

void CWallet::CommitTransaction(CTransactionRef tx, mapValue_t mapValue, std::vector<std::pair<std::string, std::string>> orderForm)
{
    LOCK(cs_wallet);
    WalletLogPrintf("CommitTransaction:\n%s", tx->ToString()); /* Continued */

    mweb_wallet->RewindOutputs(*tx);

    // Add tx to wallet, because if it has change it's also ours,
    // otherwise just for transaction history.
    AddToWallet(tx, boost::none, {}, [&](CWalletTx& wtx, bool new_tx) {
        CHECK_NONFATAL(wtx.mapValue.empty());
        CHECK_NONFATAL(wtx.vOrderForm.empty());
        wtx.mapValue = std::move(mapValue);
        wtx.vOrderForm = std::move(orderForm);
        wtx.fTimeReceivedIsTxTime = true;
        wtx.fFromMe = true;
        return true;
    });

    // Notify that old coins are spent
    for (const CTxInput& txin : tx->GetInputs()) {
        CWalletTx* coin = FindPrevTx(txin);
        coin->MarkDirty();
        NotifyTransactionChanged(this, coin->GetHash(), CT_UPDATED);
    }

    // Get the inserted-CWalletTx from mapWallet so that the
    // fInMempool flag is cached properly
    CWalletTx& wtx = mapWallet.at(tx->GetHash());

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
        }
    }

    // This wallet is in its first run if there are no ScriptPubKeyMans and it isn't blank or no privkeys
    fFirstRunRet = m_spk_managers.empty() && !IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && !IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET);
    if (fFirstRunRet) {
        assert(m_external_spk_managers.empty());
        assert(m_internal_spk_managers.empty());
    }

    if (nLoadWalletRet != DBErrors::LOAD_OK)
        return nLoadWalletRet;

    return DBErrors::LOAD_OK;
}

DBErrors CWallet::ZapSelectTx(std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut)
{
    AssertLockHeld(cs_wallet);
    DBErrors nZapSelectTxRet = WalletBatch(*database).ZapSelectTx(vHashIn, vHashOut);
    for (const uint256& hash : vHashOut) {
        const auto& it = mapWallet.find(hash);
        wtxOrdered.erase(it->second.m_it_wtxOrdered);
        for (const auto& txin : it->second.GetInputs())
            mapTxSpends.erase(txin.GetIndex());
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
    NotifyAddressBookChanged(this, address, strName, is_mine,
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
    bool is_mine;
    WalletBatch batch(*database);
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

    NotifyAddressBookChanged(this, address, "", is_mine, "", CT_DELETED);

    batch.ErasePurpose(EncodeDestination(address));
    return batch.EraseName(EncodeDestination(address));
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

bool CWallet::GetNewDestination(const OutputType type, const std::string label, CTxDestination& dest, std::string& error)
{
    LOCK(cs_wallet);
    error.clear();

    bool result = false;
    auto spk_man = GetScriptPubKeyMan(type, false /* internal */);
    if (spk_man) {
        spk_man->TopUp();
        result = spk_man->GetNewDestination(type, dest, error);
    } else {
        error = strprintf("Error: No %s addresses available.", FormatOutputType(type));
    }
    if (result) {
        SetAddressBook(dest, label, "receive");
    }

    return result;
}

bool CWallet::GetNewChangeDestination(const OutputType type, CTxDestination& dest, std::string& error)
{
    LOCK(cs_wallet);
    error.clear();

    ReserveDestination reservedest(this, type);
    if (!reservedest.GetReservedDestination(dest, true)) {
        error = _("Error: Keypool ran out, please call keypoolrefill first").translated;
        return false;
    }

    reservedest.KeepDestination();
    return true;
}

int64_t CWallet::GetOldestKeyPoolTime() const
{
    LOCK(cs_wallet);
    int64_t oldestKey = std::numeric_limits<int64_t>::max();
    for (const auto& spk_man_pair : m_spk_managers) {
        oldestKey = std::min(oldestKey, spk_man_pair.second->GetOldestKeyPoolTime());
    }
    return oldestKey;
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
            const CWalletTx& wtx = walletEntry.second;

            if (!IsTrusted(wtx, trusted_parents))
                continue;

            if (wtx.IsImmature())
                continue;

            int nDepth = wtx.GetDepthInMainChain();
            if (nDepth < (wtx.IsFromMe(ISMINE_ALL) ? 0 : 1))
                continue;

            for (const CTxOutput& output : wtx.GetOutputs())
            {
                CTxDestination addr;
                if (!IsMine(output))
                    continue;
                if(!ExtractOutputDestination(output, addr))
                    continue;

                CAmount n = IsSpent(output.GetIndex()) ? 0 : GetValue(output);
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
        const CWalletTx& wtx = walletEntry.second;

        if (wtx.tx->GetInputs().size() > 0)
        {
            bool any_mine = false;
            // group all input addresses with each other
            for (const CTxInput& input : wtx.tx->GetInputs())
            {
                CTxDestination address;
                if (!IsMine(input)) /* If this input isn't mine, ignore it */
                    continue;
                const CWalletTx* prev = FindPrevTx(input);
                if (!prev)
                    continue;
                if (!ExtractOutputDestination(prev->tx->GetOutput(input.GetIndex()), address))
                    continue;
                grouping.insert(address);
                any_mine = true;
            }

            // group change with input addresses
            if (any_mine)
            {
                for (const CTxOutput& txout : wtx.GetOutputs()) {
                    if (IsChange(txout)) {
                        CTxDestination txoutAddr;
                        if (!ExtractOutputDestination(txout, txoutAddr))
                            continue;
                        grouping.insert(txoutAddr);
                    }
                }
            }
            if (grouping.size() > 0)
            {
                groupings.insert(grouping);
                grouping.clear();
            }
        }

        // group lone addrs by themselves
        for (const auto& txout : wtx.GetOutputs())
            if (IsMine(txout))
            {
                CTxDestination address;
                if(!ExtractOutputDestination(txout, address))
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

void CWallet::LockCoin(const OutputIndex& output)
{
    AssertLockHeld(cs_wallet);
    setLockedCoins.insert(output);
}

void CWallet::UnlockCoin(const OutputIndex& output)
{
    AssertLockHeld(cs_wallet);
    setLockedCoins.erase(output);
}

void CWallet::UnlockAllCoins()
{
    AssertLockHeld(cs_wallet);
    setLockedCoins.clear();
}

bool CWallet::IsLockedCoin(const OutputIndex& output) const
{
    AssertLockHeld(cs_wallet);
    return (setLockedCoins.count(output) > 0);
}

void CWallet::ListLockedCoins(std::vector<OutputIndex>& vOutpts) const
{
    AssertLockHeld(cs_wallet);
    for (std::set<OutputIndex>::iterator it = setLockedCoins.begin();
         it != setLockedCoins.end(); it++) {
        OutputIndex outpt = (*it);
        vOutpts.push_back(outpt);
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
            for (const CTxOutput& output : wtx.GetOutputs()) {
                DestinationAddr dest;
                if (!ExtractDestinationScript(output, dest)) {
                    continue;
                }

                // iterate over all their outputs
                for (const auto& keyid : GetAffectedKeys(dest, *spk_man)) {
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
    if (boost::get<CNoDestination>(&dest))
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
    DBErrors nLoadWalletRet = walletInstance->LoadWallet(fFirstRun);
    if (nLoadWalletRet != DBErrors::LOAD_OK) {
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
        // ensure this wallet.dat can only be opened by clients supporting HD with chain split and expects no default key
        walletInstance->SetMinVersion(FEATURE_LATEST);

        walletInstance->AddWalletFlags(wallet_creation_flags);

        // Only create LegacyScriptPubKeyMan when not descriptor wallet
        if (!walletInstance->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
            walletInstance->SetupLegacyScriptPubKeyMan();
        }

        if (!(wallet_creation_flags & (WALLET_FLAG_DISABLE_PRIVATE_KEYS | WALLET_FLAG_BLANK_WALLET))) {
            LOCK(walletInstance->cs_wallet);
            if (walletInstance->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
                walletInstance->SetupDescriptorScriptPubKeyMans();
                // SetupDescriptorScriptPubKeyMans already calls SetupGeneration for us so we don't need to call SetupGeneration separately
            } else {
                // Legacy wallets need SetupGeneration here.
                for (auto spk_man : walletInstance->GetActiveScriptPubKeyMans()) {
                    if (!spk_man->SetupGeneration()) {
                        error = _("Unable to generate initial keys");
                        return nullptr;
                    }
                }
            }
        }

        walletInstance->chainStateFlushed(chain.getTipLocator());
    } else if (wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS) {
        // Make it impossible to disable private keys after creation
        error = strprintf(_("Error loading %s: Private keys can only be disabled during creation"), walletFile);
        return NULL;
    } else if (walletInstance->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        for (auto spk_man : walletInstance->GetActiveScriptPubKeyMans()) {
            if (spk_man->HavePrivateKeys()) {
                warnings.push_back(strprintf(_("Warning: Private keys detected in wallet {%s} with disabled private keys"), walletFile));
                break;
            }
        }
    }

    if (!gArgs.GetArg("-addresstype", "").empty()) {
        if (!ParseOutputType(gArgs.GetArg("-addresstype", ""), walletInstance->m_default_address_type)) {
            error = strprintf(_("Unknown address type '%s'"), gArgs.GetArg("-addresstype", ""));
            return nullptr;
        }
    }

    if (!gArgs.GetArg("-changetype", "").empty()) {
        OutputType out_type;
        if (!ParseOutputType(gArgs.GetArg("-changetype", ""), out_type)) {
            error = strprintf(_("Unknown change type '%s'"), gArgs.GetArg("-changetype", ""));
            return nullptr;
        }
        walletInstance->m_default_change_type = out_type;
    }

    if (gArgs.IsArgSet("-mintxfee")) {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-mintxfee", ""), n) || 0 == n) {
            error = AmountErrMsg("mintxfee", gArgs.GetArg("-mintxfee", ""));
            return nullptr;
        }
        if (n > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-mintxfee") + Untranslated(" ") +
                               _("This is the minimum transaction fee you pay on every transaction."));
        }
        walletInstance->m_min_fee = CFeeRate(n);
    }

    if (gArgs.IsArgSet("-maxapsfee")) {
        const std::string max_aps_fee{gArgs.GetArg("-maxapsfee", "")};
        CAmount n = 0;
        if (max_aps_fee == "-1") {
            n = -1;
        } else if (!ParseMoney(max_aps_fee, n)) {
            error = AmountErrMsg("maxapsfee", max_aps_fee);
            return nullptr;
        }
        if (n > HIGH_APS_FEE) {
            warnings.push_back(AmountHighWarn("-maxapsfee") + Untranslated(" ") +
                              _("This is the maximum transaction fee you pay (in addition to the normal fee) to prioritize partial spend avoidance over regular coin selection."));
        }
        walletInstance->m_max_aps_fee = n;
    }

    if (gArgs.IsArgSet("-fallbackfee")) {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-fallbackfee", ""), nFeePerK)) {
            error = strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'"), gArgs.GetArg("-fallbackfee", ""));
            return nullptr;
        }
        if (nFeePerK > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-fallbackfee") + Untranslated(" ") +
                               _("This is the transaction fee you may pay when fee estimates are not available."));
        }
        walletInstance->m_fallback_fee = CFeeRate(nFeePerK);
    }
    // Disable fallback fee in case value was set to 0, enable if non-null value
    walletInstance->m_allow_fallback_fee = walletInstance->m_fallback_fee.GetFeePerK() != 0;

    if (gArgs.IsArgSet("-discardfee")) {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-discardfee", ""), nFeePerK)) {
            error = strprintf(_("Invalid amount for -discardfee=<amount>: '%s'"), gArgs.GetArg("-discardfee", ""));
            return nullptr;
        }
        if (nFeePerK > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-discardfee") + Untranslated(" ") +
                               _("This is the transaction fee you may discard if change is smaller than dust at this level"));
        }
        walletInstance->m_discard_rate = CFeeRate(nFeePerK);
    }
    if (gArgs.IsArgSet("-paytxfee")) {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-paytxfee", ""), nFeePerK)) {
            error = AmountErrMsg("paytxfee", gArgs.GetArg("-paytxfee", ""));
            return nullptr;
        }
        if (nFeePerK > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-paytxfee") + Untranslated(" ") +
                               _("This is the transaction fee you will pay if you send a transaction."));
        }
        walletInstance->m_pay_tx_fee = CFeeRate(nFeePerK, 1000, 0);
        if (walletInstance->m_pay_tx_fee < chain.relayMinFee()) {
            error = strprintf(_("Invalid amount for -paytxfee=<amount>: '%s' (must be at least %s)"),
                gArgs.GetArg("-paytxfee", ""), chain.relayMinFee().ToString());
            return nullptr;
        }
    }

    if (gArgs.IsArgSet("-maxtxfee")) {
        CAmount nMaxFee = 0;
        if (!ParseMoney(gArgs.GetArg("-maxtxfee", ""), nMaxFee)) {
            error = AmountErrMsg("maxtxfee", gArgs.GetArg("-maxtxfee", ""));
            return nullptr;
        }
        if (nMaxFee > HIGH_MAX_TX_FEE) {
            warnings.push_back(_("-maxtxfee is set very high! Fees this large could be paid on a single transaction."));
        }
        if (CFeeRate(nMaxFee) < chain.relayMinFee()) {
            error = strprintf(_("Invalid amount for -maxtxfee=<amount>: '%s' (must be at least the minrelay fee of %s to prevent stuck transactions)"),
                gArgs.GetArg("-maxtxfee", ""), chain.relayMinFee().ToString());
            return nullptr;
        }
        walletInstance->m_default_max_tx_fee = nMaxFee;
    }

    if (chain.relayMinFee().GetFeePerK() > HIGH_TX_FEE_PER_KB) {
        warnings.push_back(AmountHighWarn("-minrelaytxfee") + Untranslated(" ") +
                           _("The wallet will avoid paying less than the minimum relay fee."));
    }

    walletInstance->m_confirm_target = gArgs.GetArg("-txconfirmtarget", DEFAULT_TX_CONFIRM_TARGET);
    walletInstance->m_spend_zero_conf_change = gArgs.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);
    walletInstance->m_signal_rbf = gArgs.GetBoolArg("-walletrbf", DEFAULT_WALLET_RBF);

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
            if (const Optional<int> fork_height = chain.findLocatorFork(locator)) {
                rescan_height = *fork_height;
            }
        }
    }

    const Optional<int> tip_height = chain.getHeight();
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
        // The way the 'time_first_key' is initialized is just a workaround for the gcc bug #47679 since version 4.6.0.
        Optional<int64_t> time_first_key = MakeOptional(false, int64_t());;
        for (auto spk_man : walletInstance->GetAllScriptPubKeyMans()) {
            int64_t time = spk_man->GetTimeFirstKey();
            if (!time_first_key || time < *time_first_key) time_first_key = time;
        }
        if (time_first_key) {
            if (Optional<int> first_block = chain.findFirstBlockWithTimeAndHeight(*time_first_key - TIMESTAMP_WINDOW, rescan_height, nullptr)) {
                rescan_height = *first_block;
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

    {
        LOCK(cs_wallets);
        for (auto& load_wallet : g_load_wallet_fns) {
            load_wallet(interfaces::MakeWallet(walletInstance));
        }
    }

    walletInstance->SetBroadcastTransactions(gArgs.GetBoolArg("-walletbroadcast", DEFAULT_WALLETBROADCAST));

    {
        walletInstance->WalletLogPrintf("setKeyPool.size() = %u\n",      walletInstance->GetKeyPoolSize());
        walletInstance->WalletLogPrintf("mapWallet.size() = %u\n",       walletInstance->mapWallet.size());
        walletInstance->WalletLogPrintf("m_address_book.size() = %u\n",  walletInstance->m_address_book.size());
    }

    return walletInstance;
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

bool CWallet::UpgradeWallet(int version, bilingual_str& error)
{
    int prev_version = GetVersion();
    if (version == 0) {
        WalletLogPrintf("Performing wallet upgrade to %i\n", FEATURE_LATEST);
        version = FEATURE_LATEST;
    } else {
        WalletLogPrintf("Allowing wallet upgrade up to %i\n", version);
    }
    if (version < prev_version)
    {
        error = _("Cannot downgrade wallet");
        return false;
    }

    LOCK(cs_wallet);

    // Do not upgrade versions to any version between HD_SPLIT and FEATURE_PRE_SPLIT_KEYPOOL unless already supporting HD_SPLIT
    if (!CanSupportFeature(FEATURE_HD_SPLIT) && version >= FEATURE_HD_SPLIT && version < FEATURE_PRE_SPLIT_KEYPOOL) {
        error = _("Cannot upgrade a non HD split wallet without upgrading to support pre split keypool. Please use version 169900 or no version specified.");
        return false;
    }

    // Permanently upgrade to the version
    SetMinVersion(GetClosestWalletFeature(version));

    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        if (!spk_man->Upgrade(prev_version, version, error)) {
            return false;
        }
    }
    return true;
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

bool CWallet::BackupWallet(const std::string& strDest) const
{
    return database->Backup(strDest);
}

CKeyPool::CKeyPool()
{
    nTime = GetTime();
    fInternal = false;
    m_pre_split = false;
    fMWEB = false;
}

CKeyPool::CKeyPool(const CPubKey& vchPubKeyIn, bool internalIn, bool mweb_in)
{
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
    fInternal = internalIn;
    m_pre_split = false;
    fMWEB = mweb_in;
}

int CWalletTx::GetDepthInMainChain() const
{
    assert(pwallet != nullptr);
    AssertLockHeld(pwallet->cs_wallet);
    if (isUnconfirmed() || isAbandoned()) return 0;

    return (pwallet->GetLastBlockHeight() - m_confirm.block_height + 1) * (isConflicted() ? -1 : 1);
}

int CWalletTx::GetBlocksToMaturity() const
{
    if (!IsCoinBase() && !IsHogEx())
        return 0;
    int chain_depth = std::max(0, GetDepthInMainChain());
    if (IsCoinBase()) {
        return std::max(0, (COINBASE_MATURITY + 1) - chain_depth);
    } else {
        return std::max(0, PEGOUT_MATURITY - chain_depth);
    }
}

bool CWalletTx::IsImmature() const
{
    // note GetBlocksToMaturity is 0 for non-coinbase tx and non-hogex tx
    return GetBlocksToMaturity() > 0;
}

bool CWalletTx::IsFinal() const
{
    return pwallet->chain().checkFinalTx(*tx);
}

std::vector<OutputGroup> CWallet::GroupOutputs(const std::vector<COutputCoin>& outputs, bool single_coin, const size_t max_ancestors) const {
    std::vector<OutputGroup> groups;
    std::map<CTxDestination, OutputGroup> gmap;
    std::set<CTxDestination> full_groups;

    for (const COutputCoin& output : outputs) {
        if (output.IsSpendable()) {
            const CWalletTx *wtx = output.GetWalletTx();
            size_t ancestors, descendants;
            chain().getTransactionAncestry(wtx->GetHash(), ancestors, descendants);

            CTxDestination dst;
            if (!single_coin && output.GetDestination(dst)) {
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
                    it->second.Insert(output.GetInputCoin(), output.GetDepth(), wtx->IsFromMe(ISMINE_ALL), ancestors, descendants);
                } else {
                    gmap[dst].Insert(output.GetInputCoin(), output.GetDepth(), wtx->IsFromMe(ISMINE_ALL), ancestors, descendants);
                }
            } else {
                groups.emplace_back(output.GetInputCoin(), output.GetDepth(), wtx->IsFromMe(ISMINE_ALL), ancestors, descendants);
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

bool CWallet::IsLocked() const
{
    if (!IsCrypted()) {
        return false;
    }
    LOCK(cs_wallet);
    return vMasterKey.empty();
}

bool CWallet::Lock()
{
    if (!IsCrypted())
        return false;

    {
        LOCK(cs_wallet);
        vMasterKey.clear();
    }

    // MWEB: Unload MWEB keychain
    auto mweb_spk_man = GetScriptPubKeyMan(OutputType::MWEB, false);
    if (mweb_spk_man) {
        const mw::Keychain::Ptr& keychain = mweb_spk_man->GetMWEBKeychain();
		if (keychain) {
        	keychain->Lock();
		}
    }

    NotifyStatusChanged(this);
    return true;
}

bool CWallet::Unlock(const CKeyingMaterial& vMasterKeyIn, bool accept_no_keys)
{
    {
        LOCK(cs_wallet);
        for (const auto& spk_man_pair : m_spk_managers) {
            if (!spk_man_pair.second->CheckDecryptionKey(vMasterKeyIn, accept_no_keys)) {
                return false;
            }
        }
        vMasterKey = vMasterKeyIn;
    }
    NotifyStatusChanged(this);
    return true;
}

std::set<ScriptPubKeyMan*> CWallet::GetActiveScriptPubKeyMans() const
{
    std::set<ScriptPubKeyMan*> spk_mans;
    for (bool internal : {false, true}) {
        for (OutputType t : OUTPUT_TYPES) {
            auto spk_man = GetScriptPubKeyMan(t, internal);
            if (spk_man) {
                spk_mans.insert(spk_man);
            }
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

ScriptPubKeyMan* CWallet::GetScriptPubKeyMan(const OutputType& type, bool internal) const
{
    const std::map<OutputType, ScriptPubKeyMan*>& spk_managers = internal ? m_internal_spk_managers : m_external_spk_managers;
    std::map<OutputType, ScriptPubKeyMan*>::const_iterator it = spk_managers.find(type);
    if (it == spk_managers.end()) {
        WalletLogPrintf("%s scriptPubKey Manager for output type %d does not exist\n", internal ? "Internal" : "External", static_cast<int>(type));
        return nullptr;
    }
    return it->second;
}

std::set<ScriptPubKeyMan*> CWallet::GetScriptPubKeyMans(const DestinationAddr& dest_addr, SignatureData& sigdata) const
{
    std::set<ScriptPubKeyMan*> spk_mans;
    for (const auto& spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(dest_addr, sigdata)) {
            spk_mans.insert(spk_man_pair.second.get());
        }
    }
    return spk_mans;
}

ScriptPubKeyMan* CWallet::GetScriptPubKeyMan(const DestinationAddr& dest_addr) const
{
    SignatureData sigdata;
    for (const auto& spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(dest_addr, sigdata)) {
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

std::unique_ptr<SigningProvider> CWallet::GetSolvingProvider(const DestinationAddr& script) const
{
    SignatureData sigdata;
    return GetSolvingProvider(script, sigdata);
}

std::unique_ptr<SigningProvider> CWallet::GetSolvingProvider(const DestinationAddr& script, SignatureData& sigdata) const
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
    auto it = m_internal_spk_managers.find(OutputType::LEGACY);
    if (it == m_internal_spk_managers.end()) return nullptr;
    return dynamic_cast<LegacyScriptPubKeyMan*>(it->second);
}

LegacyScriptPubKeyMan* CWallet::GetOrCreateLegacyScriptPubKeyMan()
{
    SetupLegacyScriptPubKeyMan();
    return GetLegacyScriptPubKeyMan();
}

void CWallet::SetupLegacyScriptPubKeyMan()
{
    if (!m_internal_spk_managers.empty() || !m_external_spk_managers.empty() || !m_spk_managers.empty() || IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        return;
    }

    auto spk_manager = std::unique_ptr<ScriptPubKeyMan>(new LegacyScriptPubKeyMan(*this));
    for (const auto& type : OUTPUT_TYPES) {
        m_internal_spk_managers[type] = spk_manager.get();
        m_external_spk_managers[type] = spk_manager.get();
    }
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

void CWallet::LoadDescriptorScriptPubKeyMan(uint256 id, WalletDescriptor& desc)
{
    auto spk_manager = std::unique_ptr<ScriptPubKeyMan>(new DescriptorScriptPubKeyMan(*this, desc));
    m_spk_managers[id] = std::move(spk_manager);
}

void CWallet::SetupDescriptorScriptPubKeyMans()
{
    AssertLockHeld(cs_wallet);

    // Make a seed
    CKey seed_key;
    seed_key.MakeNewKey(true);
    CPubKey seed = seed_key.GetPubKey();
    assert(seed_key.VerifyPubKey(seed));

    // Get the extended key
    CExtKey master_key;
    master_key.SetSeed(seed_key.begin(), seed_key.size());

    for (bool internal : {false, true}) {
        for (OutputType t : OUTPUT_TYPES) {
            auto spk_manager = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(*this, internal));
            if (IsCrypted()) {
                if (IsLocked()) {
                    throw std::runtime_error(std::string(__func__) + ": Wallet is locked, cannot setup new descriptors");
                }
                if (!spk_manager->CheckDecryptionKey(vMasterKey) && !spk_manager->Encrypt(vMasterKey, nullptr)) {
                    throw std::runtime_error(std::string(__func__) + ": Could not encrypt new descriptors");
                }
            }
            spk_manager->SetupDescriptorGeneration(master_key, t);
            uint256 id = spk_manager->GetID();
            m_spk_managers[id] = std::move(spk_manager);
            AddActiveScriptPubKeyMan(id, t, internal);
        }
    }
}

void CWallet::AddActiveScriptPubKeyMan(uint256 id, OutputType type, bool internal)
{
    WalletBatch batch(*database);
    if (!batch.WriteActiveScriptPubKeyMan(static_cast<uint8_t>(type), id, internal)) {
        throw std::runtime_error(std::string(__func__) + ": writing active ScriptPubKeyMan id failed");
    }
    LoadActiveScriptPubKeyMan(id, type, internal);
}

void CWallet::LoadActiveScriptPubKeyMan(uint256 id, OutputType type, bool internal)
{
    WalletLogPrintf("Setting spkMan to active: id = %s, type = %d, internal = %d\n", id.ToString(), static_cast<int>(type), static_cast<int>(internal));
    auto& spk_mans = internal ? m_internal_spk_managers : m_external_spk_managers;
    auto spk_man = m_spk_managers.at(id).get();
    spk_man->SetInternal(internal);
    spk_mans[type] = spk_man;

    NotifyCanGetAddressesChanged();
}

bool CWallet::IsLegacy() const
{
    if (m_internal_spk_managers.count(OutputType::LEGACY) == 0) {
        return false;
    }
    auto spk_man = dynamic_cast<LegacyScriptPubKeyMan*>(m_internal_spk_managers.at(OutputType::LEGACY));
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
    if (!IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        WalletLogPrintf("Cannot add WalletDescriptor to a non-descriptor wallet\n");
        return nullptr;
    }

    LOCK(cs_wallet);
    auto new_spk_man = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(*this, desc));

    // If we already have this descriptor, remove it from the maps but add the existing cache to desc
    auto old_spk_man = GetDescriptorScriptPubKeyMan(desc);
    if (old_spk_man) {
        WalletLogPrintf("Update existing descriptor: %s\n", desc.descriptor->ToString());

        {
            LOCK(old_spk_man->cs_desc_man);
            new_spk_man->SetCache(old_spk_man->GetWalletDescriptor().cache);
        }

        // Remove from maps of active spkMans
        auto old_spk_man_id = old_spk_man->GetID();
        for (bool internal : {false, true}) {
            for (OutputType t : OUTPUT_TYPES) {
                auto active_spk_man = GetScriptPubKeyMan(t, internal);
                if (active_spk_man && active_spk_man->GetID() == old_spk_man_id) {
                    if (internal) {
                        m_internal_spk_managers.erase(t);
                    } else {
                        m_external_spk_managers.erase(t);
                    }
                    break;
                }
            }
        }
        m_spk_managers.erase(old_spk_man_id);
    }

    // Add the private keys to the descriptor
    for (const auto& entry : signing_provider.keys) {
        const CKey& key = entry.second;
        new_spk_man->AddDescriptorKey(key, key.GetPubKey());
    }

    // Top up key pool, the manager will generate new scriptPubKeys internally
    if (!new_spk_man->TopUp()) {
        WalletLogPrintf("Could not top up scriptPubKeys\n");
        return nullptr;
    }

    // Apply the label if necessary
    // Note: we disable labels for ranged descriptors
    if (!desc.descriptor->IsRange()) {
        auto script_pub_keys = new_spk_man->GetScriptPubKeys();
        if (script_pub_keys.empty()) {
            WalletLogPrintf("Could not generate scriptPubKeys (cache is empty)\n");
            return nullptr;
        }

        CTxDestination dest;
        if (!internal && script_pub_keys.at(0).ExtractDestination(dest)) {
            SetAddressBook(dest, label, "receive");
        }
    }

    // Save the descriptor to memory
    auto ret = new_spk_man.get();
    m_spk_managers[new_spk_man->GetID()] = std::move(new_spk_man);

    // Save the descriptor to DB
    ret->WriteDescriptor();

    return ret;
}

bool CWallet::GetCoin(const mw::Hash& output_id, mw::Coin& coin) const
{
    return mweb_wallet->GetCoin(output_id, coin);
}

CAmount CWallet::GetValue(const CTxOutput& output) const
{
    if (output.IsMWEB()) {
        mw::Coin coin;
        if (GetCoin(output.ToMWEB(), coin)) {
            return coin.amount;
        }
    } else {
        return output.GetTxOut().nValue;
    }

    return 0;
}

bool CWallet::ExtractOutputDestination(const CTxOutput& output, CTxDestination& dest) const
{
    if (output.IsMWEB()) {
        mw::Coin coin;
        if (!GetCoin(output.ToMWEB(), coin)) {
            return false;
        }

        StealthAddress address;
        if (!mweb_wallet->GetStealthAddress(coin, address)) {
            return false;
        }

        dest = address;
        return true;
    } else {
        return ExtractDestination(output.GetScriptPubKey(), dest);
    }
}

bool CWallet::ExtractDestinationScript(const CTxOutput& output, DestinationAddr& dest) const
{
    if (output.IsMWEB()) {
        mw::Coin coin;
        if (!GetCoin(output.ToMWEB(), coin)) {
            return false;
        }

        StealthAddress address;
        if (!mweb_wallet->GetStealthAddress(coin, address)) {
            return false;
        }

        dest = address;
        return true;
    } else {
        dest = DestinationAddr(output.GetScriptPubKey());
    }

    return true;
}

const CWalletTx* CWallet::FindWalletTxByKernelId(const mw::Hash& kernel_id) const
{
    LOCK(cs_wallet);
    auto kernel_iter = mapKernelsMWEB.find(kernel_id);
    if (kernel_iter != mapKernelsMWEB.end()) {
        auto tx_iter = mapWallet.find(kernel_iter->second);
        if (tx_iter != mapWallet.end()) {
            return &tx_iter->second;
        }
    }

    return nullptr;
}

const CWalletTx* CWallet::FindWalletTx(const OutputIndex& output) const
{
    LOCK(cs_wallet);
    if (output.type() == typeid(mw::Hash)) {
        auto output_iter = mapOutputsMWEB.find(boost::get<mw::Hash>(output));
        if (output_iter != mapOutputsMWEB.end()) {
            auto tx_iter = mapWallet.find(output_iter->second);
            if (tx_iter != mapWallet.end()) {
                return &tx_iter->second;
            }
        }
    } else {
        auto tx_iter = mapWallet.find(boost::get<COutPoint>(output).hash);
        if (tx_iter != mapWallet.end()) {
            return &tx_iter->second;
        }
    }

    return nullptr;
}

const CWalletTx* CWallet::FindPrevTx(const CTxInput& input) const
{
    LOCK(cs_wallet);
    if (input.IsMWEB()) {
        auto output_iter = mapOutputsMWEB.find(input.ToMWEB());
        if (output_iter != mapOutputsMWEB.end()) {
            auto tx_iter = mapWallet.find(output_iter->second);
            if (tx_iter != mapWallet.end()) {
                return &tx_iter->second;
            }
        }
    } else {
        auto tx_iter = mapWallet.find(input.GetTxIn().prevout.hash);
        if (tx_iter != mapWallet.end()) {
            return &tx_iter->second;
        }
    }

    return nullptr;
}

CWalletTx* CWallet::FindPrevTx(const CTxInput& input)
{
    LOCK(cs_wallet);
    if (input.IsMWEB()) {
        auto output_iter = mapOutputsMWEB.find(input.ToMWEB());
        if (output_iter != mapOutputsMWEB.end()) {
            auto tx_iter = mapWallet.find(output_iter->second);
            if (tx_iter != mapWallet.end()) {
                return &tx_iter->second;
            }
        }
    } else {
        auto tx_iter = mapWallet.find(input.GetTxIn().prevout.hash);
        if (tx_iter != mapWallet.end()) {
            return &tx_iter->second;
        }
    }

    return nullptr;
}