// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
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
#include <policy/fees.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <util/bip32.h>
#include <util/error.h>
#include <util/fees.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/translation.h>
#include <util/validation.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>

#include <algorithm>
#include <assert.h>
#include <future>

#include <boost/algorithm/string/replace.hpp>

const std::map<uint64_t,std::string> WALLET_FLAG_CAVEATS{
    {WALLET_FLAG_AVOID_REUSE,
        "You need to rescan the blockchain in order to correctly mark used "
        "destinations in the past. Until this is done, some destinations may "
        "be considered unused, even if the opposite is the case."
    },
};

static const size_t OUTPUT_GROUP_MAX_ENTRIES = 10;

static CCriticalSection cs_wallets;
static std::vector<std::shared_ptr<CWallet>> vpwallets GUARDED_BY(cs_wallets);
static std::list<LoadWalletFn> g_load_wallet_fns GUARDED_BY(cs_wallets);

bool AddWallet(const std::shared_ptr<CWallet>& wallet)
{
    LOCK(cs_wallets);
    assert(wallet);
    std::vector<std::shared_ptr<CWallet>>::const_iterator i = std::find(vpwallets.begin(), vpwallets.end(), wallet);
    if (i != vpwallets.end()) return false;
    vpwallets.push_back(wallet);
    return true;
}

bool RemoveWallet(const std::shared_ptr<CWallet>& wallet)
{
    LOCK(cs_wallets);
    assert(wallet);
    std::vector<std::shared_ptr<CWallet>>::iterator i = std::find(vpwallets.begin(), vpwallets.end(), wallet);
    if (i == vpwallets.end()) return false;
    vpwallets.erase(i);
    return true;
}

bool HasWallets()
{
    LOCK(cs_wallets);
    return !vpwallets.empty();
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

static Mutex g_wallet_release_mutex;
static std::condition_variable g_wallet_release_cv;
static std::set<std::string> g_unloading_wallet_set;

// Custom deleter for shared_ptr<CWallet>.
static void ReleaseWallet(CWallet* wallet)
{
    // Unregister and delete the wallet right after BlockUntilSyncedToCurrentChain
    // so that it's in sync with the current chainstate.
    const std::string name = wallet->GetName();
    wallet->WalletLogPrintf("Releasing wallet\n");
    wallet->BlockUntilSyncedToCurrentChain();
    wallet->Flush();
    wallet->m_chain_notifications_handler.reset();
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

std::shared_ptr<CWallet> LoadWallet(interfaces::Chain& chain, const WalletLocation& location, std::string& error, std::string& warning)
{
    if (!CWallet::Verify(chain, location, false, error, warning)) {
        error = "Wallet file verification failed: " + error;
        return nullptr;
    }

    std::shared_ptr<CWallet> wallet = CWallet::CreateWalletFromFile(chain, location);
    if (!wallet) {
        error = "Wallet loading failed.";
        return nullptr;
    }
    AddWallet(wallet);
    wallet->postInitProcess();
    return wallet;
}

std::shared_ptr<CWallet> LoadWallet(interfaces::Chain& chain, const std::string& name, std::string& error, std::string& warning)
{
    return LoadWallet(chain, WalletLocation(name), error, warning);
}

WalletCreationStatus CreateWallet(interfaces::Chain& chain, const SecureString& passphrase, uint64_t wallet_creation_flags, const std::string& name, std::string& error, std::string& warning, std::shared_ptr<CWallet>& result)
{
    // Indicate that the wallet is actually supposed to be blank and not just blank to make it encrypted
    bool create_blank = (wallet_creation_flags & WALLET_FLAG_BLANK_WALLET);

    // Born encrypted wallets need to be created blank first.
    if (!passphrase.empty()) {
        wallet_creation_flags |= WALLET_FLAG_BLANK_WALLET;
    }

    // Check the wallet file location
    WalletLocation location(name);
    if (location.Exists()) {
        error = "Wallet " + location.GetName() + " already exists.";
        return WalletCreationStatus::CREATION_FAILED;
    }

    // Wallet::Verify will check if we're trying to create a wallet with a duplicate name.
    std::string wallet_error;
    if (!CWallet::Verify(chain, location, false, wallet_error, warning)) {
        error = "Wallet file verification failed: " + wallet_error;
        return WalletCreationStatus::CREATION_FAILED;
    }

    // Do not allow a passphrase when private keys are disabled
    if (!passphrase.empty() && (wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        error = "Passphrase provided but private keys are disabled. A passphrase is only used to encrypt private keys, so cannot be used for wallets with private keys disabled.";
        return WalletCreationStatus::CREATION_FAILED;
    }

    // Make the wallet
    std::shared_ptr<CWallet> wallet = CWallet::CreateWalletFromFile(chain, location, wallet_creation_flags);
    if (!wallet) {
        error = "Wallet creation failed";
        return WalletCreationStatus::CREATION_FAILED;
    }

    // Encrypt the wallet
    if (!passphrase.empty() && !(wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        if (!wallet->EncryptWallet(passphrase)) {
            error = "Error: Wallet created but failed to encrypt.";
            return WalletCreationStatus::ENCRYPTION_FAILED;
        }
        if (!create_blank) {
            // Unlock the wallet
            if (!wallet->Unlock(passphrase)) {
                error = "Error: Wallet was encrypted but could not be unlocked";
                return WalletCreationStatus::ENCRYPTION_FAILED;
            }

            // Set a seed for the wallet
            CPubKey master_pub_key = wallet->GenerateNewSeed();
            wallet->SetHDSeed(master_pub_key);
            wallet->NewKeyPool();

            // Relock the wallet
            wallet->Lock();
        }
    }
    AddWallet(wallet);
    wallet->postInitProcess();
    result = wallet;
    return WalletCreationStatus::SUCCESS;
}

const uint32_t BIP32_HARDENED_KEY_LIMIT = 0x80000000;

const uint256 CWalletTx::ABANDON_HASH(uint256S("0000000000000000000000000000000000000000000000000000000000000001"));

/** @defgroup mapWallet
 *
 * @{
 */

std::string COutput::ToString() const
{
    return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString(), i, nDepth, FormatMoney(tx->tx->vout[i].nValue));
}

std::vector<CKeyID> GetAffectedKeys(const CScript& spk, const SigningProvider& provider)
{
    std::vector<CScript> dummy;
    FlatSigningProvider out;
    InferDescriptor(spk, provider)->Expand(0, DUMMY_SIGNING_PROVIDER, dummy, out);
    std::vector<CKeyID> ret;
    for (const auto& entry : out.pubkeys) {
        ret.push_back(entry.first);
    }
    return ret;
}

const CWalletTx* CWallet::GetWalletTx(const uint256& hash) const
{
    LOCK(cs_wallet);
    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(hash);
    if (it == mapWallet.end())
        return nullptr;
    return &(it->second);
}

CPubKey CWallet::GenerateNewKey(WalletBatch &batch, bool internal)
{
    assert(!IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    assert(!IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET));
    AssertLockHeld(cs_wallet);
    bool fCompressed = CanSupportFeature(FEATURE_COMPRPUBKEY); // default to compressed public keys if we want 0.6.0 wallets

    CKey secret;

    // Create new metadata
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    // use HD key derivation if HD was enabled during wallet creation and a seed is present
    if (IsHDEnabled()) {
        DeriveNewChildKey(batch, metadata, secret, (CanSupportFeature(FEATURE_HD_SPLIT) ? internal : false));
    } else {
        secret.MakeNewKey(fCompressed);
    }

    // Compressed public keys were introduced in version 0.6.0
    if (fCompressed) {
        SetMinVersion(FEATURE_COMPRPUBKEY);
    }

    CPubKey pubkey = secret.GetPubKey();
    assert(secret.VerifyPubKey(pubkey));

    mapKeyMetadata[pubkey.GetID()] = metadata;
    UpdateTimeFirstKey(nCreationTime);

    if (!AddKeyPubKeyWithDB(batch, secret, pubkey)) {
        throw std::runtime_error(std::string(__func__) + ": AddKey failed");
    }
    return pubkey;
}

void CWallet::DeriveNewChildKey(WalletBatch &batch, CKeyMetadata& metadata, CKey& secret, bool internal)
{
    // for now we use a fixed keypath scheme of m/0'/0'/k
    CKey seed;                     //seed (256bit)
    CExtKey masterKey;             //hd master key
    CExtKey accountKey;            //key at m/0'
    CExtKey chainChildKey;         //key at m/0'/0' (external) or m/0'/1' (internal)
    CExtKey childKey;              //key at m/0'/0'/<n>'

    // try to get the seed
    if (!GetKey(hdChain.seed_id, seed))
        throw std::runtime_error(std::string(__func__) + ": seed not found");

    masterKey.SetSeed(seed.begin(), seed.size());

    // derive m/0'
    // use hardened derivation (child keys >= 0x80000000 are hardened after bip32)
    masterKey.Derive(accountKey, BIP32_HARDENED_KEY_LIMIT);

    // derive m/0'/0' (external chain) OR m/0'/1' (internal chain)
    assert(internal ? CanSupportFeature(FEATURE_HD_SPLIT) : true);
    accountKey.Derive(chainChildKey, BIP32_HARDENED_KEY_LIMIT+(internal ? 1 : 0));

    // derive child key at next index, skip keys already known to the wallet
    do {
        // always derive hardened keys
        // childIndex | BIP32_HARDENED_KEY_LIMIT = derive childIndex in hardened child-index-range
        // example: 1 | BIP32_HARDENED_KEY_LIMIT == 0x80000001 == 2147483649
        if (internal) {
            chainChildKey.Derive(childKey, hdChain.nInternalChainCounter | BIP32_HARDENED_KEY_LIMIT);
            metadata.hdKeypath = "m/0'/1'/" + std::to_string(hdChain.nInternalChainCounter) + "'";
            metadata.key_origin.path.push_back(0 | BIP32_HARDENED_KEY_LIMIT);
            metadata.key_origin.path.push_back(1 | BIP32_HARDENED_KEY_LIMIT);
            metadata.key_origin.path.push_back(hdChain.nInternalChainCounter | BIP32_HARDENED_KEY_LIMIT);
            hdChain.nInternalChainCounter++;
        }
        else {
            chainChildKey.Derive(childKey, hdChain.nExternalChainCounter | BIP32_HARDENED_KEY_LIMIT);
            metadata.hdKeypath = "m/0'/0'/" + std::to_string(hdChain.nExternalChainCounter) + "'";
            metadata.key_origin.path.push_back(0 | BIP32_HARDENED_KEY_LIMIT);
            metadata.key_origin.path.push_back(0 | BIP32_HARDENED_KEY_LIMIT);
            metadata.key_origin.path.push_back(hdChain.nExternalChainCounter | BIP32_HARDENED_KEY_LIMIT);
            hdChain.nExternalChainCounter++;
        }
    } while (HaveKey(childKey.key.GetPubKey().GetID()));
    secret = childKey.key;
    metadata.hd_seed_id = hdChain.seed_id;
    CKeyID master_id = masterKey.key.GetPubKey().GetID();
    std::copy(master_id.begin(), master_id.begin() + 4, metadata.key_origin.fingerprint);
    metadata.has_key_origin = true;
    // update the chain model in the database
    if (!batch.WriteHDChain(hdChain))
        throw std::runtime_error(std::string(__func__) + ": Writing HD chain model failed");
}

bool CWallet::AddKeyPubKeyWithDB(WalletBatch& batch, const CKey& secret, const CPubKey& pubkey)
{
    AssertLockHeld(cs_wallet);

    // Make sure we aren't adding private keys to private key disabled wallets
    assert(!IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));

    // FillableSigningProvider has no concept of wallet databases, but calls AddCryptedKey
    // which is overridden below.  To avoid flushes, the database handle is
    // tunneled through to it.
    bool needsDB = !encrypted_batch;
    if (needsDB) {
        encrypted_batch = &batch;
    }
    if (!AddKeyPubKeyInner(secret, pubkey)) {
        if (needsDB) encrypted_batch = nullptr;
        return false;
    }
    if (needsDB) encrypted_batch = nullptr;

    // check if we need to remove from watch-only
    CScript script;
    script = GetScriptForDestination(PKHash(pubkey));
    if (HaveWatchOnly(script)) {
        RemoveWatchOnly(script);
    }
    script = GetScriptForRawPubKey(pubkey);
    if (HaveWatchOnly(script)) {
        RemoveWatchOnly(script);
    }

    if (!IsCrypted()) {
        return batch.WriteKey(pubkey,
                                                 secret.GetPrivKey(),
                                                 mapKeyMetadata[pubkey.GetID()]);
    }
    UnsetWalletFlagWithDB(batch, WALLET_FLAG_BLANK_WALLET);
    return true;
}

bool CWallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    WalletBatch batch(*database);
    return CWallet::AddKeyPubKeyWithDB(batch, secret, pubkey);
}

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey,
                            const std::vector<unsigned char> &vchCryptedSecret)
{
    if (!AddCryptedKeyInner(vchPubKey, vchCryptedSecret))
        return false;
    {
        LOCK(cs_wallet);
        if (encrypted_batch)
            return encrypted_batch->WriteCryptedKey(vchPubKey,
                                                        vchCryptedSecret,
                                                        mapKeyMetadata[vchPubKey.GetID()]);
        else
            return WalletBatch(*database).WriteCryptedKey(vchPubKey,
                                                            vchCryptedSecret,
                                                            mapKeyMetadata[vchPubKey.GetID()]);
    }
}

void CWallet::LoadKeyMetadata(const CKeyID& keyID, const CKeyMetadata& meta)
{
    AssertLockHeld(cs_wallet);
    UpdateTimeFirstKey(meta.nCreateTime);
    mapKeyMetadata[keyID] = meta;
}

void CWallet::LoadScriptMetadata(const CScriptID& script_id, const CKeyMetadata& meta)
{
    AssertLockHeld(cs_wallet);
    UpdateTimeFirstKey(meta.nCreateTime);
    m_script_metadata[script_id] = meta;
}

void CWallet::UpgradeKeyMetadata()
{
    AssertLockHeld(cs_wallet);
    if (IsLocked() || IsWalletFlagSet(WALLET_FLAG_KEY_ORIGIN_METADATA)) {
        return;
    }

    std::unique_ptr<WalletBatch> batch = MakeUnique<WalletBatch>(*database);
    for (auto& meta_pair : mapKeyMetadata) {
        CKeyMetadata& meta = meta_pair.second;
        if (!meta.hd_seed_id.IsNull() && !meta.has_key_origin && meta.hdKeypath != "s") { // If the hdKeypath is "s", that's the seed and it doesn't have a key origin
            CKey key;
            GetKey(meta.hd_seed_id, key);
            CExtKey masterKey;
            masterKey.SetSeed(key.begin(), key.size());
            // Add to map
            CKeyID master_id = masterKey.key.GetPubKey().GetID();
            std::copy(master_id.begin(), master_id.begin() + 4, meta.key_origin.fingerprint);
            if (!ParseHDKeypath(meta.hdKeypath, meta.key_origin.path)) {
                throw std::runtime_error("Invalid stored hdKeypath");
            }
            meta.has_key_origin = true;
            if (meta.nVersion < CKeyMetadata::VERSION_WITH_KEY_ORIGIN) {
                meta.nVersion = CKeyMetadata::VERSION_WITH_KEY_ORIGIN;
            }

            // Write meta to wallet
            CPubKey pubkey;
            if (GetPubKey(meta_pair.first, pubkey)) {
                batch->WriteKeyMetadata(meta, pubkey, true);
            }
        }
    }
    batch.reset(); //write before setting the flag
    SetWalletFlag(WALLET_FLAG_KEY_ORIGIN_METADATA);
}

bool CWallet::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    return AddCryptedKeyInner(vchPubKey, vchCryptedSecret);
}

/**
 * Update wallet first key creation time. This should be called whenever keys
 * are added to the wallet, with the oldest key creation time.
 */
void CWallet::UpdateTimeFirstKey(int64_t nCreateTime)
{
    AssertLockHeld(cs_wallet);
    if (nCreateTime <= 1) {
        // Cannot determine birthday information, so set the wallet birthday to
        // the beginning of time.
        nTimeFirstKey = 1;
    } else if (!nTimeFirstKey || nCreateTime < nTimeFirstKey) {
        nTimeFirstKey = nCreateTime;
    }
}

bool CWallet::AddCScript(const CScript& redeemScript)
{
    WalletBatch batch(*database);
    return AddCScriptWithDB(batch, redeemScript);
}

bool CWallet::AddCScriptWithDB(WalletBatch& batch, const CScript& redeemScript)
{
    if (!FillableSigningProvider::AddCScript(redeemScript))
        return false;
    if (batch.WriteCScript(Hash160(redeemScript), redeemScript)) {
        UnsetWalletFlagWithDB(batch, WALLET_FLAG_BLANK_WALLET);
        return true;
    }
    return false;
}

bool CWallet::LoadCScript(const CScript& redeemScript)
{
    /* A sanity check was added in pull #3843 to avoid adding redeemScripts
     * that never can be redeemed. However, old wallets may still contain
     * these. Do not add them to the wallet and warn. */
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
    {
        std::string strAddr = EncodeDestination(ScriptHash(redeemScript));
        WalletLogPrintf("%s: Warning: This wallet contains a redeemScript of size %i which exceeds maximum size %i thus can never be redeemed. Do not use address %s.\n", __func__, redeemScript.size(), MAX_SCRIPT_ELEMENT_SIZE, strAddr);
        return true;
    }

    return FillableSigningProvider::AddCScript(redeemScript);
}

static bool ExtractPubKey(const CScript &dest, CPubKey& pubKeyOut)
{
    std::vector<std::vector<unsigned char>> solutions;
    return Solver(dest, solutions) == TX_PUBKEY &&
        (pubKeyOut = CPubKey(solutions[0])).IsFullyValid();
}

bool CWallet::AddWatchOnlyInMem(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.insert(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey)) {
        mapWatchKeys[pubKey.GetID()] = pubKey;
        ImplicitlyLearnRelatedKeyScripts(pubKey);
    }
    return true;
}

bool CWallet::AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest)
{
    if (!AddWatchOnlyInMem(dest))
        return false;
    const CKeyMetadata& meta = m_script_metadata[CScriptID(dest)];
    UpdateTimeFirstKey(meta.nCreateTime);
    NotifyWatchonlyChanged(true);
    if (batch.WriteWatchOnly(dest, meta)) {
        UnsetWalletFlagWithDB(batch, WALLET_FLAG_BLANK_WALLET);
        return true;
    }
    return false;
}

bool CWallet::AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest, int64_t create_time)
{
    m_script_metadata[CScriptID(dest)].nCreateTime = create_time;
    return AddWatchOnlyWithDB(batch, dest);
}

bool CWallet::AddWatchOnly(const CScript& dest)
{
    WalletBatch batch(*database);
    return AddWatchOnlyWithDB(batch, dest);
}

bool CWallet::AddWatchOnly(const CScript& dest, int64_t nCreateTime)
{
    m_script_metadata[CScriptID(dest)].nCreateTime = nCreateTime;
    return AddWatchOnly(dest);
}

bool CWallet::RemoveWatchOnly(const CScript &dest)
{
    AssertLockHeld(cs_wallet);
    {
        LOCK(cs_KeyStore);
        setWatchOnly.erase(dest);
        CPubKey pubKey;
        if (ExtractPubKey(dest, pubKey)) {
            mapWatchKeys.erase(pubKey.GetID());
        }
        // Related CScripts are not removed; having superfluous scripts around is
        // harmless (see comment in ImplicitlyLearnRelatedKeyScripts).
    }

    if (!HaveWatchOnly())
        NotifyWatchonlyChanged(false);
    if (!WalletBatch(*database).EraseWatchOnly(dest))
        return false;

    return true;
}

bool CWallet::LoadWatchOnly(const CScript &dest)
{
    return AddWatchOnlyInMem(dest);
}

bool CWallet::HaveWatchOnly(const CScript &dest) const
{
    LOCK(cs_KeyStore);
    return setWatchOnly.count(dest) > 0;
}

bool CWallet::HaveWatchOnly() const
{
    LOCK(cs_KeyStore);
    return (!setWatchOnly.empty());
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

void CWallet::ChainStateFlushed(const CBlockLocator& loc)
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

bool CWallet::HasWalletSpend(const uint256& txid) const
{
    AssertLockHeld(cs_wallet);
    auto iter = mapTxSpends.lower_bound(COutPoint(txid, 0));
    return (iter != mapTxSpends.end() && iter->first.hash == txid);
}

void CWallet::Flush(bool shutdown)
{
    database->Flush(shutdown);
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
bool CWallet::IsSpent(interfaces::Chain::Lock& locked_chain, const uint256& hash, unsigned int n) const
{
    const COutPoint outpoint(hash, n);
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    range = mapTxSpends.equal_range(outpoint);

    for (TxSpends::const_iterator it = range.first; it != range.second; ++it)
    {
        const uint256& wtxid = it->second;
        std::map<uint256, CWalletTx>::const_iterator mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end()) {
            int depth = mit->second.GetDepthInMainChain(locked_chain);
            if (depth > 0  || (depth == 0 && !mit->second.isAbandoned()))
                return true; // Spent
        }
    }
    return false;
}

void CWallet::AddToSpends(const COutPoint& outpoint, const uint256& wtxid)
{
    mapTxSpends.insert(std::make_pair(outpoint, wtxid));

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
        assert(!encrypted_batch);
        encrypted_batch = new WalletBatch(*database);
        if (!encrypted_batch->TxnBegin()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            return false;
        }
        encrypted_batch->WriteMasterKey(nMasterKeyMaxID, kMasterKey);

        if (!EncryptKeys(_vMasterKey))
        {
            encrypted_batch->TxnAbort();
            delete encrypted_batch;
            encrypted_batch = nullptr;
            // We now probably have half of our keys encrypted in memory, and half not...
            // die and let the user reload the unencrypted wallet.
            assert(false);
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

        // if we are using HD, replace the HD seed with a new one
        if (IsHDEnabled()) {
            SetHDSeed(GenerateNewSeed());
        }

        NewKeyPool();
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

    WalletBatch batch(*database, "r+");

    bool success = true;
    if (!batch.WriteTx(wtx)) {
        WalletLogPrintf("%s: Updating batch tx %s failed\n", __func__, wtx.GetHash().ToString());
        success = false;
    }

    NotifyTransactionChanged(this, originalHash, CT_UPDATED);

    return success;
}

void CWallet::SetUsedDestinationState(const uint256& hash, unsigned int n, bool used, std::set<CTxDestination>& tx_destinations)
{
    const CWalletTx* srctx = GetWalletTx(hash);
    if (!srctx) return;

    CTxDestination dst;
    if (ExtractDestination(srctx->tx->vout[n].scriptPubKey, dst)) {
        if (::IsMine(*this, dst)) {
            LOCK(cs_wallet);
            if (used && !GetDestData(dst, "used", nullptr)) {
                if (AddDestData(dst, "used", "p")) { // p for "present", opposite of absent (null)
                    tx_destinations.insert(dst);
                }
            } else if (!used && GetDestData(dst, "used", nullptr)) {
                EraseDestData(dst, "used");
            }
        }
    }
}

bool CWallet::IsUsedDestination(const uint256& hash, unsigned int n) const
{
    AssertLockHeld(cs_wallet);
    CTxDestination dst;
    const CWalletTx* srctx = GetWalletTx(hash);
    if (srctx) {
        assert(srctx->tx->vout.size() > n);
        // When descriptor wallets arrive, these additional checks are
        // likely superfluous and can be optimized out
        for (const auto& keyid : GetAffectedKeys(srctx->tx->vout[n].scriptPubKey, *this)) {
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

bool CWallet::AddToWallet(const CWalletTx& wtxIn, bool fFlushOnClose)
{
    LOCK(cs_wallet);

    WalletBatch batch(*database, "r+", fFlushOnClose);

    uint256 hash = wtxIn.GetHash();

    if (IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE)) {
        // Mark used destinations
        std::set<CTxDestination> tx_destinations;

        for (const CTxIn& txin : wtxIn.tx->vin) {
            const COutPoint& op = txin.prevout;
            SetUsedDestinationState(op.hash, op.n, true, tx_destinations);
        }

        MarkDestinationsDirty(tx_destinations);
    }

    // Inserts only if not already there, returns tx inserted or tx found
    std::pair<std::map<uint256, CWalletTx>::iterator, bool> ret = mapWallet.insert(std::make_pair(hash, wtxIn));
    CWalletTx& wtx = (*ret.first).second;
    wtx.BindWallet(this);
    bool fInsertedNew = ret.second;
    if (fInsertedNew) {
        wtx.nTimeReceived = chain().getAdjustedTime();
        wtx.nOrderPos = IncOrderPosNext(&batch);
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
        wtx.nTimeSmart = ComputeTimeSmart(wtx);
        AddToSpends(hash);
    }

    bool fUpdated = false;
    if (!fInsertedNew)
    {
        if (wtxIn.m_confirm.status != wtx.m_confirm.status) {
            wtx.m_confirm.status = wtxIn.m_confirm.status;
            wtx.m_confirm.nIndex = wtxIn.m_confirm.nIndex;
            wtx.m_confirm.hashBlock = wtxIn.m_confirm.hashBlock;
            fUpdated = true;
        } else {
            assert(wtx.m_confirm.nIndex == wtxIn.m_confirm.nIndex);
            assert(wtx.m_confirm.hashBlock == wtxIn.m_confirm.hashBlock);
        }
        if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
        {
            wtx.fFromMe = wtxIn.fFromMe;
            fUpdated = true;
        }
        // If we have a witness-stripped version of this transaction, and we
        // see a new version with a witness, then we must be upgrading a pre-segwit
        // wallet.  Store the new version of the transaction with the witness,
        // as the stripped-version must be invalid.
        // TODO: Store all versions of the transaction, instead of just one.
        if (wtxIn.tx->HasWitness() && !wtx.tx->HasWitness()) {
            wtx.SetTx(wtxIn.tx);
            fUpdated = true;
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
        boost::replace_all(strCmd, "%s", wtxIn.GetHash().GetHex());
        std::thread t(runCommand, strCmd);
        t.detach(); // thread runs free
    }
#endif

    return true;
}

void CWallet::LoadToWallet(CWalletTx& wtxIn)
{
    // If wallet doesn't have a chain (e.g wallet-tool), lock can't be taken.
    auto locked_chain = LockChain();
    // If tx hasn't been reorged out of chain while wallet being shutdown
    // change tx status to UNCONFIRMED and reset hashBlock/nIndex.
    if (!wtxIn.m_confirm.hashBlock.IsNull()) {
        if (locked_chain && !locked_chain->getBlockHeight(wtxIn.m_confirm.hashBlock)) {
            wtxIn.setUnconfirmed();
            wtxIn.m_confirm.hashBlock = uint256();
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
                MarkConflicted(prevtx.m_confirm.hashBlock, wtx.GetHash());
            }
        }
    }
}

bool CWallet::AddToWalletIfInvolvingMe(const CTransactionRef& ptx, CWalletTx::Status status, const uint256& block_hash, int posInBlock, bool fUpdate)
{
    const CTransaction& tx = *ptx;
    {
        AssertLockHeld(cs_wallet);

        if (!block_hash.IsNull()) {
            for (const CTxIn& txin : tx.vin) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(txin.prevout);
                while (range.first != range.second) {
                    if (range.first->second != tx.GetHash()) {
                        WalletLogPrintf("Transaction %s (in block %s) conflicts with wallet transaction %s (both spend %s:%i)\n", tx.GetHash().ToString(), block_hash.ToString(), range.first->second.ToString(), range.first->first.hash.ToString(), range.first->first.n);
                        MarkConflicted(block_hash, range.first->second);
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

            // loop though all outputs
            for (const CTxOut& txout: tx.vout) {
                // extract addresses and check if they match with an unused keypool key
                for (const auto& keyid : GetAffectedKeys(txout.scriptPubKey, *this)) {
                    std::map<CKeyID, int64_t>::const_iterator mi = m_pool_key_to_index.find(keyid);
                    if (mi != m_pool_key_to_index.end()) {
                        WalletLogPrintf("%s: Detected a used keypool key, mark all keypool key up to this key as used\n", __func__);
                        MarkReserveKeysAsUsed(mi->second);

                        if (!TopUpKeyPool()) {
                            WalletLogPrintf("%s: Topping up keypool failed (locked wallet)\n", __func__);
                        }
                    }
                }
            }

            CWalletTx wtx(this, ptx);

            // Block disconnection override an abandoned tx as unconfirmed
            // which means user may have to call abandontransaction again
            wtx.SetConf(status, block_hash, posInBlock);

            return AddToWallet(wtx, false);
        }
    }
    return false;
}

bool CWallet::TransactionCanBeAbandoned(const uint256& hashTx) const
{
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);
    const CWalletTx* wtx = GetWalletTx(hashTx);
    return wtx && !wtx->isAbandoned() && wtx->GetDepthInMainChain(*locked_chain) == 0 && !wtx->InMempool();
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

bool CWallet::AbandonTransaction(interfaces::Chain::Lock& locked_chain, const uint256& hashTx)
{
    auto locked_chain_recursive = chain().lock();  // Temporary. Removed in upcoming lock cleanup
    LOCK(cs_wallet);

    WalletBatch batch(*database, "r+");

    std::set<uint256> todo;
    std::set<uint256> done;

    // Can't mark abandoned if confirmed or in mempool
    auto it = mapWallet.find(hashTx);
    assert(it != mapWallet.end());
    CWalletTx& origtx = it->second;
    if (origtx.GetDepthInMainChain(locked_chain) != 0 || origtx.InMempool()) {
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
        int currentconfirm = wtx.GetDepthInMainChain(locked_chain);
        // If the orig tx was not in block, none of its spends can be
        assert(currentconfirm <= 0);
        // if (currentconfirm < 0) {Tx and spends are already conflicted, no need to abandon}
        if (currentconfirm == 0 && !wtx.isAbandoned()) {
            // If the orig tx was not in block/mempool, none of its spends can be in mempool
            assert(!wtx.InMempool());
            wtx.m_confirm.nIndex = 0;
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

    return true;
}

void CWallet::MarkConflicted(const uint256& hashBlock, const uint256& hashTx)
{
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    int conflictconfirms = -locked_chain->getBlockDepth(hashBlock);
    // If number of conflict confirms cannot be determined, this means
    // that the block is still unknown or not yet part of the main chain,
    // for example when loading the wallet during a reindex. Do nothing in that
    // case.
    if (conflictconfirms >= 0)
        return;

    // Do not flush the wallet here for performance reasons
    WalletBatch batch(*database, "r+", false);

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
        int currentconfirm = wtx.GetDepthInMainChain(*locked_chain);
        if (conflictconfirms < currentconfirm) {
            // Block is 'more conflicted' than current confirm; update.
            // Mark transaction as conflicted with this block.
            wtx.m_confirm.nIndex = 0;
            wtx.m_confirm.hashBlock = hashBlock;
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
}

void CWallet::SyncTransaction(const CTransactionRef& ptx, CWalletTx::Status status, const uint256& block_hash, int posInBlock, bool update_tx)
{
    if (!AddToWalletIfInvolvingMe(ptx, status, block_hash, posInBlock, update_tx))
        return; // Not one of ours

    // If a transaction changes 'conflicted' state, that changes the balance
    // available of the outputs it spends. So force those to be
    // recomputed, also:
    MarkInputsDirty(ptx);
}

void CWallet::TransactionAddedToMempool(const CTransactionRef& ptx) {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);
    SyncTransaction(ptx, CWalletTx::Status::UNCONFIRMED, {} /* block hash */, 0 /* position in block */);

    auto it = mapWallet.find(ptx->GetHash());
    if (it != mapWallet.end()) {
        it->second.fInMempool = true;
    }
}

void CWallet::TransactionRemovedFromMempool(const CTransactionRef &ptx) {
    LOCK(cs_wallet);
    auto it = mapWallet.find(ptx->GetHash());
    if (it != mapWallet.end()) {
        it->second.fInMempool = false;
    }
}

void CWallet::BlockConnected(const CBlock& block, const std::vector<CTransactionRef>& vtxConflicted) {
    const uint256& block_hash = block.GetHash();
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    for (size_t i = 0; i < block.vtx.size(); i++) {
        SyncTransaction(block.vtx[i], CWalletTx::Status::CONFIRMED, block_hash, i);
        TransactionRemovedFromMempool(block.vtx[i]);
    }
    for (const CTransactionRef& ptx : vtxConflicted) {
        TransactionRemovedFromMempool(ptx);
    }

    m_last_block_processed = block_hash;
}

void CWallet::BlockDisconnected(const CBlock& block) {
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    // At block disconnection, this will change an abandoned transaction to
    // be unconfirmed, whether or not the transaction is added back to the mempool.
    // User may have to call abandontransaction again. It may be addressed in the
    // future with a stickier abandoned state or even removing abandontransaction call.
    for (const CTransactionRef& ptx : block.vtx) {
        SyncTransaction(ptx, CWalletTx::Status::UNCONFIRMED, {} /* block hash */, 0 /* position in block */);
    }
}

void CWallet::UpdatedBlockTip()
{
    m_best_block_time = GetTime();
}


void CWallet::BlockUntilSyncedToCurrentChain() {
    AssertLockNotHeld(cs_wallet);
    // Skip the queue-draining stuff if we know we're caught up with
    // ::ChainActive().Tip(), otherwise put a callback in the validation interface queue and wait
    // for the queue to drain enough to execute it (indicating we are caught up
    // at least with the time we entered this function).
    uint256 last_block_hash = WITH_LOCK(cs_wallet, return m_last_block_processed);
    chain().waitForNotificationsIfNewBlocksConnected(last_block_hash);
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

isminetype CWallet::IsMine(const CTxOut& txout) const
{
    return ::IsMine(*this, txout.scriptPubKey);
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
    if (::IsMine(*this, script))
    {
        CTxDestination address;
        if (!ExtractDestination(script, address))
            return true;

        LOCK(cs_wallet);
        if (!mapAddressBook.count(address))
            return true;
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

CPubKey CWallet::GenerateNewSeed()
{
    assert(!IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    CKey key;
    key.MakeNewKey(true);
    return DeriveNewSeed(key);
}

CPubKey CWallet::DeriveNewSeed(const CKey& key)
{
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    // calculate the seed
    CPubKey seed = key.GetPubKey();
    assert(key.VerifyPubKey(seed));

    // set the hd keypath to "s" -> Seed, refers the seed to itself
    metadata.hdKeypath     = "s";
    metadata.has_key_origin = false;
    metadata.hd_seed_id = seed.GetID();

    {
        LOCK(cs_wallet);

        // mem store the metadata
        mapKeyMetadata[seed.GetID()] = metadata;

        // write the key&metadata to the database
        if (!AddKeyPubKey(key, seed))
            throw std::runtime_error(std::string(__func__) + ": AddKeyPubKey failed");
    }

    return seed;
}

void CWallet::SetHDSeed(const CPubKey& seed)
{
    LOCK(cs_wallet);
    // store the keyid (hash160) together with
    // the child index counter in the database
    // as a hdchain object
    CHDChain newHdChain;
    newHdChain.nVersion = CanSupportFeature(FEATURE_HD_SPLIT) ? CHDChain::VERSION_HD_CHAIN_SPLIT : CHDChain::VERSION_HD_BASE;
    newHdChain.seed_id = seed.GetID();
    SetHDChain(newHdChain, false);
    NotifyCanGetAddressesChanged();
    UnsetWalletFlag(WALLET_FLAG_BLANK_WALLET);
}

void CWallet::SetHDChain(const CHDChain& chain, bool memonly)
{
    LOCK(cs_wallet);
    if (!memonly && !WalletBatch(*database).WriteHDChain(chain))
        throw std::runtime_error(std::string(__func__) + ": writing chain failed");

    hdChain = chain;
}

bool CWallet::IsHDEnabled() const
{
    return !hdChain.seed_id.IsNull();
}

bool CWallet::CanGenerateKeys()
{
    // A wallet can generate keys if it has an HD seed (IsHDEnabled) or it is a non-HD wallet (pre FEATURE_HD)
    LOCK(cs_wallet);
    return IsHDEnabled() || !CanSupportFeature(FEATURE_HD);
}

bool CWallet::CanGetAddresses(bool internal)
{
    LOCK(cs_wallet);
    // Check if the keypool has keys
    bool keypool_has_keys;
    if (internal && CanSupportFeature(FEATURE_HD_SPLIT)) {
        keypool_has_keys = setInternalKeyPool.size() > 0;
    } else {
        keypool_has_keys = KeypoolCountExternalKeys() > 0;
    }
    // If the keypool doesn't have keys, check if we can generate them
    if (!keypool_has_keys) {
        return CanGenerateKeys();
    }
    return keypool_has_keys;
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

    if (!ProduceSignature(*this, use_max_sig ? DUMMY_MAXIMUM_SIGNATURE_CREATOR : DUMMY_SIGNATURE_CREATOR, scriptPubKey, sigdata)) {
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
    WalletBatch batch(*database);
    for (const auto& entry : scripts) {
        CScriptID id(entry);
        if (HaveCScript(id)) {
            WalletLogPrintf("Already have script %s, skipping\n", HexStr(entry));
            continue;
        }
        if (!AddCScriptWithDB(batch, entry)) {
            return false;
        }

        if (timestamp > 0) {
            m_script_metadata[CScriptID(entry)].nCreateTime = timestamp;
        }
    }
    if (timestamp > 0) {
        UpdateTimeFirstKey(timestamp);
    }

    return true;
}

bool CWallet::ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp)
{
    WalletBatch batch(*database);
    for (const auto& entry : privkey_map) {
        const CKey& key = entry.second;
        CPubKey pubkey = key.GetPubKey();
        const CKeyID& id = entry.first;
        assert(key.VerifyPubKey(pubkey));
        // Skip if we already have the key
        if (HaveKey(id)) {
            WalletLogPrintf("Already have key with pubkey %s, skipping\n", HexStr(pubkey));
            continue;
        }
        mapKeyMetadata[id].nCreateTime = timestamp;
        // If the private key is not present in the wallet, insert it.
        if (!AddKeyPubKeyWithDB(batch, key, pubkey)) {
            return false;
        }
        UpdateTimeFirstKey(timestamp);
    }
    return true;
}

bool CWallet::ImportPubKeys(const std::vector<CKeyID>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const bool internal, const int64_t timestamp)
{
    WalletBatch batch(*database);
    for (const auto& entry : key_origins) {
        AddKeyOriginWithDB(batch, entry.second.first, entry.second.second);
    }
    for (const CKeyID& id : ordered_pubkeys) {
        auto entry = pubkey_map.find(id);
        if (entry == pubkey_map.end()) {
            continue;
        }
        const CPubKey& pubkey = entry->second;
        CPubKey temp;
        if (GetPubKey(id, temp)) {
            // Already have pubkey, skipping
            WalletLogPrintf("Already have pubkey %s, skipping\n", HexStr(temp));
            continue;
        }
        if (!AddWatchOnlyWithDB(batch, GetScriptForRawPubKey(pubkey), timestamp)) {
            return false;
        }
        mapKeyMetadata[id].nCreateTime = timestamp;

        // Add to keypool only works with pubkeys
        if (add_keypool) {
            AddKeypoolPubkeyWithDB(pubkey, internal, batch);
            NotifyCanGetAddressesChanged();
        }
    }
    return true;
}

bool CWallet::ImportScriptPubKeys(const std::string& label, const std::set<CScript>& script_pub_keys, const bool have_solving_data, const bool apply_label, const int64_t timestamp)
{
    WalletBatch batch(*database);
    for (const CScript& script : script_pub_keys) {
        if (!have_solving_data || !::IsMine(*this, script)) { // Always call AddWatchOnly for non-solvable watch-only, so that watch timestamp gets updated
            if (!AddWatchOnlyWithDB(batch, script, timestamp)) {
                return false;
            }
        }
        CTxDestination dest;
        ExtractDestination(script, dest);
        if (apply_label && IsValidDestination(dest)) {
            SetAddressBookWithDB(batch, dest, label, "receive");
        }
    }
    return true;
}

int64_t CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, bool use_max_sig)
{
    std::vector<CTxOut> txouts;
    // Look up the inputs.  We should have already checked that this transaction
    // IsAllFromMe(ISMINE_SPENDABLE), so every input should already be in our
    // wallet, with a valid index into the vout array, and the ability to sign.
    for (const CTxIn& input : tx.vin) {
        const auto mi = wallet->mapWallet.find(input.prevout.hash);
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
        // This should never happen, because IsAllFromMe(ISMINE_SPENDABLE)
        // implies that we can sign for every input.
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
    uint256 start_block;
    {
        auto locked_chain = chain().lock();
        const Optional<int> start_height = locked_chain->findFirstBlockWithTimeAndHeight(startTime - TIMESTAMP_WINDOW, 0, &start_block);
        const Optional<int> tip_height = locked_chain->getHeight();
        WalletLogPrintf("%s: Rescanning last %i blocks\n", __func__, tip_height && start_height ? *tip_height - *start_height + 1 : 0);
    }

    if (!start_block.IsNull()) {
        // TODO: this should take into account failure by ScanResult::USER_ABORT
        ScanResult result = ScanForWalletTransactions(start_block, {} /* stop_block */, reserver, update);
        if (result.status == ScanResult::FAILURE) {
            int64_t time_max;
            if (!chain().findBlock(result.last_failed_block, nullptr /* block */, nullptr /* time */, &time_max)) {
                throw std::logic_error("ScanForWalletTransactions returned invalid block hash");
            }
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
 * @param[in] stop_block  Scan ending block. If block is not on the active
 *                        chain, the scan will continue until it reaches the
 *                        chain tip.
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
CWallet::ScanResult CWallet::ScanForWalletTransactions(const uint256& start_block, const uint256& stop_block, const WalletRescanReserver& reserver, bool fUpdate)
{
    int64_t nNow = GetTime();
    int64_t start_time = GetTimeMillis();

    assert(reserver.isReserved());

    uint256 block_hash = start_block;
    ScanResult result;

    WalletLogPrintf("Rescan started from block %s...\n", start_block.ToString());

    fAbortRescan = false;
    ShowProgress(strprintf("%s " + _("Rescanning...").translated, GetDisplayName()), 0); // show rescan progress in GUI as dialog or on splashscreen, if -rescan on startup
    uint256 tip_hash;
    // The way the 'block_height' is initialized is just a workaround for the gcc bug #47679 since version 4.6.0.
    Optional<int> block_height = MakeOptional(false, int());
    double progress_begin;
    double progress_end;
    {
        auto locked_chain = chain().lock();
        if (Optional<int> tip_height = locked_chain->getHeight()) {
            tip_hash = locked_chain->getBlockHash(*tip_height);
        }
        block_height = locked_chain->getBlockHeight(block_hash);
        progress_begin = chain().guessVerificationProgress(block_hash);
        progress_end = chain().guessVerificationProgress(stop_block.IsNull() ? tip_hash : stop_block);
    }
    double progress_current = progress_begin;
    while (block_height && !fAbortRescan && !chain().shutdownRequested()) {
        m_scanning_progress = (progress_current - progress_begin) / (progress_end - progress_begin);
        if (*block_height % 100 == 0 && progress_end - progress_begin > 0.0) {
            ShowProgress(strprintf("%s " + _("Rescanning...").translated, GetDisplayName()), std::max(1, std::min(99, (int)(m_scanning_progress * 100))));
        }
        if (GetTime() >= nNow + 60) {
            nNow = GetTime();
            WalletLogPrintf("Still rescanning. At block %d. Progress=%f\n", *block_height, progress_current);
        }

        CBlock block;
        if (chain().findBlock(block_hash, &block) && !block.IsNull()) {
            auto locked_chain = chain().lock();
            LOCK(cs_wallet);
            if (!locked_chain->getBlockHeight(block_hash)) {
                // Abort scan if current block is no longer active, to prevent
                // marking transactions as coming from the wrong block.
                // TODO: This should return success instead of failure, see
                // https://github.com/bitcoin/bitcoin/pull/14711#issuecomment-458342518
                result.last_failed_block = block_hash;
                result.status = ScanResult::FAILURE;
                break;
            }
            for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
                SyncTransaction(block.vtx[posInBlock], CWalletTx::Status::CONFIRMED, block_hash, posInBlock, fUpdate);
            }
            // scan succeeded, record block as most recent successfully scanned
            result.last_scanned_block = block_hash;
            result.last_scanned_height = *block_height;
        } else {
            // could not scan block, keep scanning but record this block as the most recent failure
            result.last_failed_block = block_hash;
            result.status = ScanResult::FAILURE;
        }
        if (block_hash == stop_block) {
            break;
        }
        {
            auto locked_chain = chain().lock();
            Optional<int> tip_height = locked_chain->getHeight();
            if (!tip_height || *tip_height <= block_height || !locked_chain->getBlockHeight(block_hash)) {
                // break successfully when rescan has reached the tip, or
                // previous block is no longer on the chain due to a reorg
                break;
            }

            // increment block and verification progress
            block_hash = locked_chain->getBlockHash(++*block_height);
            progress_current = chain().guessVerificationProgress(block_hash);

            // handle updated tip hash
            const uint256 prev_tip_hash = tip_hash;
            tip_hash = locked_chain->getBlockHash(*tip_height);
            if (stop_block.IsNull() && prev_tip_hash != tip_hash) {
                // in case the tip has changed, update progress max
                progress_end = chain().guessVerificationProgress(tip_hash);
            }
        }
    }
    ShowProgress(strprintf("%s " + _("Rescanning...").translated, GetDisplayName()), 100); // hide progress dialog in GUI
    if (block_height && fAbortRescan) {
        WalletLogPrintf("Rescan aborted at block %d. Progress=%f\n", *block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else if (block_height && chain().shutdownRequested()) {
        WalletLogPrintf("Rescan interrupted by shutdown request at block %d. Progress=%f\n", *block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else {
        WalletLogPrintf("Rescan completed in %15dms\n", GetTimeMillis() - start_time);
    }
    return result;
}

void CWallet::ReacceptWalletTransactions(interfaces::Chain::Lock& locked_chain)
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

        int nDepth = wtx.GetDepthInMainChain(locked_chain);

        if (!wtx.IsCoinBase() && (nDepth == 0 && !wtx.isAbandoned())) {
            mapSorted.insert(std::make_pair(wtx.nOrderPos, &wtx));
        }
    }

    // Try to add wallet transactions to memory pool
    for (const std::pair<const int64_t, CWalletTx*>& item : mapSorted) {
        CWalletTx& wtx = *(item.second);
        std::string unused_err_string;
        wtx.SubmitMemoryPoolAndRelay(unused_err_string, false, locked_chain);
    }
}

bool CWalletTx::SubmitMemoryPoolAndRelay(std::string& err_string, bool relay, interfaces::Chain::Lock& locked_chain)
{
    // Can't relay if wallet is not broadcasting
    if (!pwallet->GetBroadcastTransactions()) return false;
    // Don't relay abandoned transactions
    if (isAbandoned()) return false;
    // Don't try to submit coinbase transactions. These would fail anyway but would
    // cause log spam.
    if (IsCoinBase()) return false;
    // Don't try to submit conflicted or confirmed transactions.
    if (GetDepthInMainChain(locked_chain) != 0) return false;

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
    bool ret = pwallet->chain().broadcastTransaction(tx, err_string, pwallet->m_default_max_tx_fee, relay);
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
        amount.Set(filter, type == DEBIT ? pwallet->GetDebit(*tx, filter) : pwallet->GetCredit(*tx, filter));
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

CAmount CWalletTx::GetCredit(interfaces::Chain::Lock& locked_chain, const isminefilter& filter) const
{
    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsImmatureCoinBase(locked_chain))
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

CAmount CWalletTx::GetImmatureCredit(interfaces::Chain::Lock& locked_chain, bool fUseCache) const
{
    if (IsImmatureCoinBase(locked_chain) && IsInMainChain(locked_chain)) {
        return GetCachableAmount(IMMATURE_CREDIT, ISMINE_SPENDABLE, !fUseCache);
    }

    return 0;
}

CAmount CWalletTx::GetAvailableCredit(interfaces::Chain::Lock& locked_chain, bool fUseCache, const isminefilter& filter) const
{
    if (pwallet == nullptr)
        return 0;

    // Avoid caching ismine for NO or ALL cases (could remove this check and simplify in the future).
    bool allow_cache = (filter & ISMINE_ALL) && (filter & ISMINE_ALL) != ISMINE_ALL;

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsImmatureCoinBase(locked_chain))
        return 0;

    if (fUseCache && allow_cache && m_amounts[AVAILABLE_CREDIT].m_cached[filter]) {
        return m_amounts[AVAILABLE_CREDIT].m_value[filter];
    }

    bool allow_used_addresses = (filter & ISMINE_USED) || !pwallet->IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE);
    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < tx->vout.size(); i++)
    {
        if (!pwallet->IsSpent(locked_chain, hashTx, i) && (allow_used_addresses || !pwallet->IsUsedDestination(hashTx, i))) {
            const CTxOut &txout = tx->vout[i];
            nCredit += pwallet->GetCredit(txout, filter);
            if (!MoneyRange(nCredit))
                throw std::runtime_error(std::string(__func__) + " : value out of range");
        }
    }

    if (allow_cache) {
        m_amounts[AVAILABLE_CREDIT].Set(filter, nCredit);
    }

    return nCredit;
}

CAmount CWalletTx::GetImmatureWatchOnlyCredit(interfaces::Chain::Lock& locked_chain, const bool fUseCache) const
{
    if (IsImmatureCoinBase(locked_chain) && IsInMainChain(locked_chain)) {
        return GetCachableAmount(IMMATURE_CREDIT, ISMINE_WATCH_ONLY, !fUseCache);
    }

    return 0;
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

bool CWalletTx::IsTrusted(interfaces::Chain::Lock& locked_chain) const
{
    // Quick answer in most cases
    if (!locked_chain.checkFinalTx(*tx)) {
        return false;
    }
    int nDepth = GetDepthInMainChain(locked_chain);
    if (nDepth >= 1)
        return true;
    if (nDepth < 0)
        return false;
    if (!pwallet->m_spend_zero_conf_change || !IsFromMe(ISMINE_ALL)) // using wtx's cached debit
        return false;

    // Don't trust unconfirmed transactions from us unless they are in the mempool.
    if (!InMempool())
        return false;

    // Trusted if all inputs are from us and are in the mempool:
    for (const CTxIn& txin : tx->vin)
    {
        // Transactions not sent by us: not trusted
        const CWalletTx* parent = pwallet->GetWalletTx(txin.prevout.hash);
        if (parent == nullptr)
            return false;
        const CTxOut& parentOut = parent->tx->vout[txin.prevout.n];
        if (pwallet->IsMine(parentOut) != ISMINE_SPENDABLE)
            return false;
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
    nNextResend = GetTime() + GetRand(30 * 60);
    if (fFirst) return;

    // Only do it if there's been a new block since last time
    if (m_best_block_time < nLastResend) return;
    nLastResend = GetTime();

    int submitted_tx_count = 0;

    { // locked_chain and cs_wallet scope
        auto locked_chain = chain().lock();
        LOCK(cs_wallet);

        // Relay transactions
        for (std::pair<const uint256, CWalletTx>& item : mapWallet) {
            CWalletTx& wtx = item.second;
            // Attempt to rebroadcast all txes more than 5 minutes older than
            // the last block. SubmitMemoryPoolAndRelay() will not rebroadcast
            // any confirmed or conflicting txs.
            if (wtx.nTimeReceived > m_best_block_time - 5 * 60) continue;
            std::string unused_err_string;
            if (wtx.SubmitMemoryPoolAndRelay(unused_err_string, true, *locked_chain)) ++submitted_tx_count;
        }
    } // locked_chain and cs_wallet

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
        auto locked_chain = chain().lock();
        LOCK(cs_wallet);
        for (const auto& entry : mapWallet)
        {
            const CWalletTx& wtx = entry.second;
            const bool is_trusted{wtx.IsTrusted(*locked_chain)};
            const int tx_depth{wtx.GetDepthInMainChain(*locked_chain)};
            const CAmount tx_credit_mine{wtx.GetAvailableCredit(*locked_chain, /* fUseCache */ true, ISMINE_SPENDABLE | reuse_filter)};
            const CAmount tx_credit_watchonly{wtx.GetAvailableCredit(*locked_chain, /* fUseCache */ true, ISMINE_WATCH_ONLY | reuse_filter)};
            if (is_trusted && tx_depth >= min_depth) {
                ret.m_mine_trusted += tx_credit_mine;
                ret.m_watchonly_trusted += tx_credit_watchonly;
            }
            if (!is_trusted && tx_depth == 0 && wtx.InMempool()) {
                ret.m_mine_untrusted_pending += tx_credit_mine;
                ret.m_watchonly_untrusted_pending += tx_credit_watchonly;
            }
            ret.m_mine_immature += wtx.GetImmatureCredit(*locked_chain);
            ret.m_watchonly_immature += wtx.GetImmatureWatchOnlyCredit(*locked_chain);
        }
    }
    return ret;
}

CAmount CWallet::GetAvailableBalance(const CCoinControl* coinControl) const
{
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    CAmount balance = 0;
    std::vector<COutput> vCoins;
    AvailableCoins(*locked_chain, vCoins, true, coinControl);
    for (const COutput& out : vCoins) {
        if (out.fSpendable) {
            balance += out.tx->tx->vout[out.i].nValue;
        }
    }
    return balance;
}

void CWallet::AvailableCoins(interfaces::Chain::Lock& locked_chain, std::vector<COutput>& vCoins, bool fOnlySafe, const CCoinControl* coinControl, const CAmount& nMinimumAmount, const CAmount& nMaximumAmount, const CAmount& nMinimumSumAmount, const uint64_t nMaximumCount) const
{
    AssertLockHeld(cs_wallet);

    vCoins.clear();
    CAmount nTotal = 0;
    // Either the WALLET_FLAG_AVOID_REUSE flag is not set (in which case we always allow), or we default to avoiding, and only in the case where
    // a coin control object is provided, and has the avoid address reuse flag set to false, do we allow already used addresses
    bool allow_used_addresses = !IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE) || (coinControl && !coinControl->m_avoid_address_reuse);
    const int min_depth = {coinControl ? coinControl->m_min_depth : DEFAULT_MIN_DEPTH};
    const int max_depth = {coinControl ? coinControl->m_max_depth : DEFAULT_MAX_DEPTH};

    for (const auto& entry : mapWallet)
    {
        const uint256& wtxid = entry.first;
        const CWalletTx& wtx = entry.second;

        if (!locked_chain.checkFinalTx(*wtx.tx)) {
            continue;
        }

        if (wtx.IsImmatureCoinBase(locked_chain))
            continue;

        int nDepth = wtx.GetDepthInMainChain(locked_chain);
        if (nDepth < 0)
            continue;

        // We should not consider coins which aren't at least in our mempool
        // It's possible for these to be conflicted via ancestors which we may never be able to detect
        if (nDepth == 0 && !wtx.InMempool())
            continue;

        bool safeTx = wtx.IsTrusted(locked_chain);

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

        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
            if (wtx.tx->vout[i].nValue < nMinimumAmount || wtx.tx->vout[i].nValue > nMaximumAmount)
                continue;

            if (coinControl && coinControl->HasSelected() && !coinControl->fAllowOtherInputs && !coinControl->IsSelected(COutPoint(entry.first, i)))
                continue;

            if (IsLockedCoin(entry.first, i))
                continue;

            if (IsSpent(locked_chain, wtxid, i))
                continue;

            isminetype mine = IsMine(wtx.tx->vout[i]);

            if (mine == ISMINE_NO) {
                continue;
            }

            if (!allow_used_addresses && IsUsedDestination(wtxid, i)) {
                continue;
            }

            bool solvable = IsSolvable(*this, wtx.tx->vout[i].scriptPubKey);
            bool spendable = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) || (((mine & ISMINE_WATCH_ONLY) != ISMINE_NO) && (coinControl && coinControl->fAllowWatchOnly && solvable));

            vCoins.push_back(COutput(&wtx, i, nDepth, spendable, solvable, safeTx, (coinControl && coinControl->fAllowWatchOnly)));

            // Checks the sum amount of all UTXO's.
            if (nMinimumSumAmount != MAX_MONEY) {
                nTotal += wtx.tx->vout[i].nValue;

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

std::map<CTxDestination, std::vector<COutput>> CWallet::ListCoins(interfaces::Chain::Lock& locked_chain) const
{
    AssertLockHeld(cs_wallet);

    std::map<CTxDestination, std::vector<COutput>> result;
    std::vector<COutput> availableCoins;

    AvailableCoins(locked_chain, availableCoins);

    for (const COutput& coin : availableCoins) {
        CTxDestination address;
        if (coin.fSpendable &&
            ExtractDestination(FindNonChangeParentOutput(*coin.tx->tx, coin.i).scriptPubKey, address)) {
            result[address].emplace_back(std::move(coin));
        }
    }

    std::vector<COutPoint> lockedCoins;
    ListLockedCoins(lockedCoins);
    for (const COutPoint& output : lockedCoins) {
        auto it = mapWallet.find(output.hash);
        if (it != mapWallet.end()) {
            int depth = it->second.GetDepthInMainChain(locked_chain);
            if (depth >= 0 && output.n < it->second.tx->vout.size() &&
                IsMine(it->second.tx->vout[output.n]) == ISMINE_SPENDABLE) {
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

bool CWallet::SelectCoinsMinConf(const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, std::vector<OutputGroup> groups,
                                 std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CoinSelectionParams& coin_selection_params, bool& bnb_used) const
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
                    group.effective_value += effective_value;
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
        return KnapsackSolver(nTargetValue, utxo_pool, setCoinsRet, nValueRet);
    }
}

bool CWallet::SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CCoinControl& coin_control, CoinSelectionParams& coin_selection_params, bool& bnb_used) const
{
    std::vector<COutput> vCoins(vAvailableCoins);

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coin_control.HasSelected() && !coin_control.fAllowOtherInputs)
    {
        // We didn't use BnB here, so set it to false.
        bnb_used = false;

        for (const COutput& out : vCoins)
        {
            if (!out.fSpendable)
                 continue;
            nValueRet += out.tx->tx->vout[out.i].nValue;
            setCoinsRet.insert(out.GetInputCoin());
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
        // For now, don't use BnB if preset inputs are selected. TODO: Enable this later
        bnb_used = false;
        coin_selection_params.use_bnb = false;

        std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it != mapWallet.end())
        {
            const CWalletTx& wtx = it->second;
            // Clearly invalid input, fail
            if (wtx.tx->vout.size() <= outpoint.n)
                return false;
            // Just to calculate the marginal byte size
            nValueFromPresetInputs += wtx.tx->vout[outpoint.n].nValue;
            setPresetCoins.insert(CInputCoin(wtx.tx, outpoint.n));
        } else
            return false; // TODO: Allow non-wallet inputs
    }

    // remove preset inputs from vCoins
    for (std::vector<COutput>::iterator it = vCoins.begin(); it != vCoins.end() && coin_control.HasSelected();)
    {
        if (setPresetCoins.count(it->GetInputCoin()))
            it = vCoins.erase(it);
        else
            ++it;
    }

    // form groups from remaining coins; note that preset coins will not
    // automatically have their associated (same address) coins included
    if (coin_control.m_avoid_partial_spends && vCoins.size() > OUTPUT_GROUP_MAX_ENTRIES) {
        // Cases where we have 11+ outputs all pointing to the same destination may result in
        // privacy leaks as they will potentially be deterministically sorted. We solve that by
        // explicitly shuffling the outputs before processing
        Shuffle(vCoins.begin(), vCoins.end(), FastRandomContext());
    }
    std::vector<OutputGroup> groups = GroupOutputs(vCoins, !coin_control.m_avoid_partial_spends);

    size_t max_ancestors = (size_t)std::max<int64_t>(1, gArgs.GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT));
    size_t max_descendants = (size_t)std::max<int64_t>(1, gArgs.GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT));
    bool fRejectLongChains = gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    bool res = nTargetValue <= nValueFromPresetInputs ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(1, 6, 0), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used) ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(1, 1, 0), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1, 2), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1, std::min((size_t)4, max_ancestors/3), std::min((size_t)4, max_descendants/3)), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1, max_ancestors/2, max_descendants/2), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1, max_ancestors-1, max_descendants-1), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change && !fRejectLongChains && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1, std::numeric_limits<uint64_t>::max()), groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used));

    // because SelectCoinsMinConf clears the setCoinsRet, we now add the possible inputs to the coinset
    util::insert(setCoinsRet, setPresetCoins);

    // add preset inputs to the total value selected
    nValueRet += nValueFromPresetInputs;

    return res;
}

bool CWallet::SignTransaction(CMutableTransaction& tx)
{
    AssertLockHeld(cs_wallet);

    // sign the new tx
    int nIn = 0;
    for (auto& input : tx.vin) {
        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(input.prevout.hash);
        if(mi == mapWallet.end() || input.prevout.n >= mi->second.tx->vout.size()) {
            return false;
        }
        const CScript& scriptPubKey = mi->second.tx->vout[input.prevout.n].scriptPubKey;
        const CAmount& amount = mi->second.tx->vout[input.prevout.n].nValue;
        SignatureData sigdata;
        if (!ProduceSignature(*this, MutableTransactionSignatureCreator(&tx, nIn, amount, SIGHASH_ALL), scriptPubKey, sigdata)) {
            return false;
        }
        UpdateInput(input, sigdata);
        nIn++;
    }
    return true;
}

bool CWallet::FundTransaction(CMutableTransaction& tx, CAmount& nFeeRet, int& nChangePosInOut, std::string& strFailReason, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, CCoinControl coinControl)
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
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    CTransactionRef tx_new;
    if (!CreateTransaction(*locked_chain, vecSend, tx_new, nFeeRet, nChangePosInOut, strFailReason, coinControl, false)) {
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

static bool IsCurrentForAntiFeeSniping(interfaces::Chain& chain, interfaces::Chain::Lock& locked_chain)
{
    if (chain.isInitialBlockDownload()) {
        return false;
    }
    constexpr int64_t MAX_ANTI_FEE_SNIPING_TIP_AGE = 8 * 60 * 60; // in seconds
    if (locked_chain.getBlockTime(*locked_chain.getHeight()) < (GetTime() - MAX_ANTI_FEE_SNIPING_TIP_AGE)) {
        return false;
    }
    return true;
}

/**
 * Return a height-based locktime for new transactions (uses the height of the
 * current chain tip unless we are not synced with the current chain
 */
static uint32_t GetLocktimeForNewTransaction(interfaces::Chain& chain, interfaces::Chain::Lock& locked_chain)
{
    uint32_t const height = locked_chain.getHeight().get_value_or(-1);
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
    if (IsCurrentForAntiFeeSniping(chain, locked_chain)) {
        locktime = height;

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
    assert(locktime <= height);
    assert(locktime < LOCKTIME_THRESHOLD);
    return locktime;
}

OutputType CWallet::TransactionChangeType(OutputType change_type, const std::vector<CRecipient>& vecSend)
{
    // If -changetype is specified, always use that change type.
    if (change_type != OutputType::CHANGE_AUTO) {
        return change_type;
    }

    // if m_default_address_type is legacy, use legacy address as change (even
    // if some of the outputs are P2WPKH or P2WSH).
    if (m_default_address_type == OutputType::LEGACY) {
        return OutputType::LEGACY;
    }

    // if any destination is P2WPKH or P2WSH, use P2WPKH for the change
    // output.
    for (const auto& recipient : vecSend) {
        // Check if any destination contains a witness program:
        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;
        if (recipient.scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            return OutputType::BECH32;
        }
    }

    // else use m_default_address_type for change
    return m_default_address_type;
}

bool CWallet::CreateTransaction(interfaces::Chain::Lock& locked_chain, const std::vector<CRecipient>& vecSend, CTransactionRef& tx, CAmount& nFeeRet,
                         int& nChangePosInOut, std::string& strFailReason, const CCoinControl& coin_control, bool sign)
{
    CAmount nValue = 0;
    ReserveDestination reservedest(this);
    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    for (const auto& recipient : vecSend)
    {
        if (nValue < 0 || recipient.nAmount < 0)
        {
            strFailReason = _("Transaction amounts must not be negative").translated;
            return false;
        }
        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;
    }
    if (vecSend.empty())
    {
        strFailReason = _("Transaction must have at least one recipient").translated;
        return false;
    }

    CMutableTransaction txNew;

    txNew.nLockTime = GetLocktimeForNewTransaction(chain(), locked_chain);

    FeeCalculation feeCalc;
    CAmount nFeeNeeded;
    int nBytes;
    {
        std::set<CInputCoin> setCoins;
        auto locked_chain = chain().lock();
        LOCK(cs_wallet);
        {
            std::vector<COutput> vAvailableCoins;
            AvailableCoins(*locked_chain, vAvailableCoins, true, &coin_control, 1, MAX_MONEY, MAX_MONEY, 0);
            CoinSelectionParams coin_selection_params; // Parameters for coin selection, init with dummy

            // Create change script that will be used if we need change
            // TODO: pass in scriptChange instead of reservedest so
            // change transaction isn't always pay-to-bitcoin-address
            CScript scriptChange;

            // coin control: send change to custom address
            if (!boost::get<CNoDestination>(&coin_control.destChange)) {
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
                    strFailReason = _("Can't generate a change-address key. No keys in the internal keypool and can't generate any keys.").translated;
                    return false;
                }
                CTxDestination dest;
                const OutputType change_type = TransactionChangeType(coin_control.m_change_type ? *coin_control.m_change_type : m_default_change_type, vecSend);
                bool ret = reservedest.GetReservedDestination(change_type, dest, true);
                if (!ret)
                {
                    strFailReason = "Keypool ran out, please call keypoolrefill first";
                    return false;
                }

                scriptChange = GetScriptForDestination(dest);
            }
            CTxOut change_prototype_txout(0, scriptChange);
            coin_selection_params.change_output_size = GetSerializeSize(change_prototype_txout);

            CFeeRate discard_rate = GetDiscardRate(*this);

            // Get the fee rate to use effective values in coin selection
            CFeeRate nFeeRateNeeded = GetMinimumFeeRate(*this, coin_control, &feeCalc);

            nFeeRet = 0;
            bool pick_new_inputs = true;
            CAmount nValueIn = 0;

            // BnB selector is the only selector used when this is true.
            // That should only happen on the first pass through the loop.
            coin_selection_params.use_bnb = nSubtractFeeFromAmount == 0; // If we are doing subtract fee from recipient, then don't use BnB
            // Start with no fee and loop until there is enough fee
            while (true)
            {
                nChangePosInOut = nChangePosRequest;
                txNew.vin.clear();
                txNew.vout.clear();
                bool fFirst = true;

                CAmount nValueToSelect = nValue;
                if (nSubtractFeeFromAmount == 0)
                    nValueToSelect += nFeeRet;

                // vouts to the payees
                coin_selection_params.tx_noinputs_size = 11; // Static vsize overhead + outputs vsize. 4 nVersion, 4 nLocktime, 1 input count, 1 output count, 1 witness overhead (dummy, flag, stack size)
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
                    // Include the fee cost for outputs. Note this is only used for BnB right now
                    coin_selection_params.tx_noinputs_size += ::GetSerializeSize(txout, PROTOCOL_VERSION);

                    if (IsDust(txout, chain().relayDustFee()))
                    {
                        if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
                        {
                            if (txout.nValue < 0)
                                strFailReason = _("The transaction amount is too small to pay the fee").translated;
                            else
                                strFailReason = _("The transaction amount is too small to send after the fee has been deducted").translated;
                        }
                        else
                            strFailReason = _("Transaction amount too small").translated;
                        return false;
                    }
                    txNew.vout.push_back(txout);
                }

                // Choose coins to use
                bool bnb_used;
                if (pick_new_inputs) {
                    nValueIn = 0;
                    setCoins.clear();
                    int change_spend_size = CalculateMaximumSignedInputSize(change_prototype_txout, this);
                    // If the wallet doesn't know how to sign change output, assume p2sh-p2wpkh
                    // as lower-bound to allow BnB to do it's thing
                    if (change_spend_size == -1) {
                        coin_selection_params.change_spend_size = DUMMY_NESTED_P2WPKH_INPUT_SIZE;
                    } else {
                        coin_selection_params.change_spend_size = (size_t)change_spend_size;
                    }
                    coin_selection_params.effective_fee = nFeeRateNeeded;
                    if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coin_control, coin_selection_params, bnb_used))
                    {
                        // If BnB was used, it was the first pass. No longer the first pass and continue loop with knapsack.
                        if (bnb_used) {
                            coin_selection_params.use_bnb = false;
                            continue;
                        }
                        else {
                            strFailReason = _("Insufficient funds").translated;
                            return false;
                        }
                    }
                } else {
                    bnb_used = false;
                }

                const CAmount nChange = nValueIn - nValueToSelect;
                if (nChange > 0)
                {
                    // Fill a vout to ourself
                    CTxOut newTxOut(nChange, scriptChange);

                    // Never create dust outputs; if we would, just
                    // add the dust to the fee.
                    // The nChange when BnB is used is always going to go to fees.
                    if (IsDust(newTxOut, discard_rate) || bnb_used)
                    {
                        nChangePosInOut = -1;
                        nFeeRet += nChange;
                    }
                    else
                    {
                        if (nChangePosInOut == -1)
                        {
                            // Insert change txn at random position:
                            nChangePosInOut = GetRandInt(txNew.vout.size()+1);
                        }
                        else if ((unsigned int)nChangePosInOut > txNew.vout.size())
                        {
                            strFailReason = _("Change index out of range").translated;
                            return false;
                        }

                        std::vector<CTxOut>::iterator position = txNew.vout.begin()+nChangePosInOut;
                        txNew.vout.insert(position, newTxOut);
                    }
                } else {
                    nChangePosInOut = -1;
                }

                // Dummy fill vin for maximum size estimation
                //
                for (const auto& coin : setCoins) {
                    txNew.vin.push_back(CTxIn(coin.outpoint,CScript()));
                }

                nBytes = CalculateMaximumSignedTxSize(CTransaction(txNew), this, coin_control.fAllowWatchOnly);
                if (nBytes < 0) {
                    strFailReason = _("Signing transaction failed").translated;
                    return false;
                }

                nFeeNeeded = GetMinimumFee(*this, nBytes, coin_control, &feeCalc);
                if (feeCalc.reason == FeeReason::FALLBACK && !m_allow_fallback_fee) {
                    // eventually allow a fallback fee
                    strFailReason = _("Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable -fallbackfee.").translated;
                    return false;
                }

                if (nFeeRet >= nFeeNeeded) {
                    // Reduce fee to only the needed amount if possible. This
                    // prevents potential overpayment in fees if the coins
                    // selected to meet nFeeNeeded result in a transaction that
                    // requires less fee than the prior iteration.

                    // If we have no change and a big enough excess fee, then
                    // try to construct transaction again only without picking
                    // new inputs. We now know we only need the smaller fee
                    // (because of reduced tx size) and so we should add a
                    // change output. Only try this once.
                    if (nChangePosInOut == -1 && nSubtractFeeFromAmount == 0 && pick_new_inputs) {
                        unsigned int tx_size_with_change = nBytes + coin_selection_params.change_output_size + 2; // Add 2 as a buffer in case increasing # of outputs changes compact size
                        CAmount fee_needed_with_change = GetMinimumFee(*this, tx_size_with_change, coin_control, nullptr);
                        CAmount minimum_value_for_change = GetDustThreshold(change_prototype_txout, discard_rate);
                        if (nFeeRet >= fee_needed_with_change + minimum_value_for_change) {
                            pick_new_inputs = false;
                            nFeeRet = fee_needed_with_change;
                            continue;
                        }
                    }

                    // If we have change output already, just increase it
                    if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                        CAmount extraFeePaid = nFeeRet - nFeeNeeded;
                        std::vector<CTxOut>::iterator change_position = txNew.vout.begin()+nChangePosInOut;
                        change_position->nValue += extraFeePaid;
                        nFeeRet -= extraFeePaid;
                    }
                    break; // Done, enough fee included.
                }
                else if (!pick_new_inputs) {
                    // This shouldn't happen, we should have had enough excess
                    // fee to pay for the new output and still meet nFeeNeeded
                    // Or we should have just subtracted fee from recipients and
                    // nFeeNeeded should not have changed
                    strFailReason = _("Transaction fee and change calculation failed").translated;
                    return false;
                }

                // Try to reduce change to include necessary fee
                if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                    CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
                    std::vector<CTxOut>::iterator change_position = txNew.vout.begin()+nChangePosInOut;
                    // Only reduce change if remaining amount is still a large enough output.
                    if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
                        change_position->nValue -= additionalFeeNeeded;
                        nFeeRet += additionalFeeNeeded;
                        break; // Done, able to increase fee from change
                    }
                }

                // If subtracting fee from recipients, we now know what fee we
                // need to subtract, we have no reason to reselect inputs
                if (nSubtractFeeFromAmount > 0) {
                    pick_new_inputs = false;
                }

                // Include more fee and try again.
                nFeeRet = nFeeNeeded;
                coin_selection_params.use_bnb = false;
                continue;
            }
        }

        // Shuffle selected coins and fill in final vin
        txNew.vin.clear();
        std::vector<CInputCoin> selected_coins(setCoins.begin(), setCoins.end());
        Shuffle(selected_coins.begin(), selected_coins.end(), FastRandomContext());

        // Note how the sequence number is set to non-maxint so that
        // the nLockTime set above actually works.
        //
        // BIP125 defines opt-in RBF as any nSequence < maxint-1, so
        // we use the highest possible value in that range (maxint-2)
        // to avoid conflicting with other possible uses of nSequence,
        // and in the spirit of "smallest possible change from prior
        // behavior."
        const uint32_t nSequence = coin_control.m_signal_bip125_rbf.get_value_or(m_signal_rbf) ? MAX_BIP125_RBF_SEQUENCE : (CTxIn::SEQUENCE_FINAL - 1);
        for (const auto& coin : selected_coins) {
            txNew.vin.push_back(CTxIn(coin.outpoint, CScript(), nSequence));
        }

        if (sign)
        {
            int nIn = 0;
            for (const auto& coin : selected_coins)
            {
                const CScript& scriptPubKey = coin.txout.scriptPubKey;
                SignatureData sigdata;

                if (!ProduceSignature(*this, MutableTransactionSignatureCreator(&txNew, nIn, coin.txout.nValue, SIGHASH_ALL), scriptPubKey, sigdata))
                {
                    strFailReason = _("Signing transaction failed").translated;
                    return false;
                } else {
                    UpdateInput(txNew.vin.at(nIn), sigdata);
                }

                nIn++;
            }
        }

        // Return the constructed transaction data.
        tx = MakeTransactionRef(std::move(txNew));

        // Limit size
        if (GetTransactionWeight(*tx) > MAX_STANDARD_TX_WEIGHT)
        {
            strFailReason = _("Transaction too large").translated;
            return false;
        }
    }

    if (nFeeRet > m_default_max_tx_fee) {
        strFailReason = TransactionErrorString(TransactionError::MAX_FEE_EXCEEDED);
        return false;
    }

    if (gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        if (!chain().checkChainLimits(tx)) {
            strFailReason = _("Transaction has too long of a mempool chain").translated;
            return false;
        }
    }

    // Before we return success, we assume any change key will be used to prevent
    // accidental re-use.
    reservedest.KeepDestination();

    WalletLogPrintf("Fee Calculation: Fee:%d Bytes:%u Needed:%d Tgt:%d (requested %d) Reason:\"%s\" Decay %.5f: Estimation: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out) Fail: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out)\n",
              nFeeRet, nBytes, nFeeNeeded, feeCalc.returnedTarget, feeCalc.desiredTarget, StringForFeeReason(feeCalc.reason), feeCalc.est.decay,
              feeCalc.est.pass.start, feeCalc.est.pass.end,
              100 * feeCalc.est.pass.withinTarget / (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool),
              feeCalc.est.pass.withinTarget, feeCalc.est.pass.totalConfirmed, feeCalc.est.pass.inMempool, feeCalc.est.pass.leftMempool,
              feeCalc.est.fail.start, feeCalc.est.fail.end,
              100 * feeCalc.est.fail.withinTarget / (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool),
              feeCalc.est.fail.withinTarget, feeCalc.est.fail.totalConfirmed, feeCalc.est.fail.inMempool, feeCalc.est.fail.leftMempool);
    return true;
}

/**
 * Call after CreateTransaction unless you want to abort
 */
bool CWallet::CommitTransaction(CTransactionRef tx, mapValue_t mapValue, std::vector<std::pair<std::string, std::string>> orderForm, CValidationState& state)
{
    {
        auto locked_chain = chain().lock();
        LOCK(cs_wallet);

        CWalletTx wtxNew(this, std::move(tx));
        wtxNew.mapValue = std::move(mapValue);
        wtxNew.vOrderForm = std::move(orderForm);
        wtxNew.fTimeReceivedIsTxTime = true;
        wtxNew.fFromMe = true;

        WalletLogPrintf("CommitTransaction:\n%s", wtxNew.tx->ToString()); /* Continued */
        {

            // Add tx to wallet, because if it has change it's also ours,
            // otherwise just for transaction history.
            AddToWallet(wtxNew);

            // Notify that old coins are spent
            for (const CTxIn& txin : wtxNew.tx->vin)
            {
                CWalletTx &coin = mapWallet.at(txin.prevout.hash);
                coin.BindWallet(this);
                NotifyTransactionChanged(this, coin.GetHash(), CT_UPDATED);
            }
        }

        // Get the inserted-CWalletTx from mapWallet so that the
        // fInMempool flag is cached properly
        CWalletTx& wtx = mapWallet.at(wtxNew.GetHash());

        if (fBroadcastTransactions)
        {
            std::string err_string;
            if (!wtx.SubmitMemoryPoolAndRelay(err_string, true, *locked_chain)) {
                WalletLogPrintf("CommitTransaction(): Transaction cannot be broadcast immediately, %s\n", err_string);
                // TODO: if we expect the failure to be long term or permanent, instead delete wtx from the wallet and return failure.
            }
        }
    }
    return true;
}

DBErrors CWallet::LoadWallet(bool& fFirstRunRet)
{
    // Even if we don't use this lock in this function, we want to preserve
    // lock order in LoadToWallet if query of chain state is needed to know
    // tx status. If lock can't be taken (e.g wallet-tool), tx confirmation
    // status may be not reliable.
    auto locked_chain = LockChain();
    LOCK(cs_wallet);

    fFirstRunRet = false;
    DBErrors nLoadWalletRet = WalletBatch(*database,"cr+").LoadWallet(this);
    if (nLoadWalletRet == DBErrors::NEED_REWRITE)
    {
        if (database->Rewrite("\x04pool"))
        {
            setInternalKeyPool.clear();
            setExternalKeyPool.clear();
            m_pool_key_to_index.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    {
        LOCK(cs_KeyStore);
        // This wallet is in its first run if all of these are empty
        fFirstRunRet = mapKeys.empty() && mapCryptedKeys.empty() && mapWatchKeys.empty() && setWatchOnly.empty() && mapScripts.empty()
            && !IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && !IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET);
    }

    if (nLoadWalletRet != DBErrors::LOAD_OK)
        return nLoadWalletRet;

    return DBErrors::LOAD_OK;
}

DBErrors CWallet::ZapSelectTx(std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut)
{
    AssertLockHeld(cs_wallet);
    DBErrors nZapSelectTxRet = WalletBatch(*database, "cr+").ZapSelectTx(vHashIn, vHashOut);
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
            setInternalKeyPool.clear();
            setExternalKeyPool.clear();
            m_pool_key_to_index.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapSelectTxRet != DBErrors::LOAD_OK)
        return nZapSelectTxRet;

    MarkDirty();

    return DBErrors::LOAD_OK;
}

DBErrors CWallet::ZapWalletTx(std::vector<CWalletTx>& vWtx)
{
    DBErrors nZapWalletTxRet = WalletBatch(*database,"cr+").ZapWalletTx(vWtx);
    if (nZapWalletTxRet == DBErrors::NEED_REWRITE)
    {
        if (database->Rewrite("\x04pool"))
        {
            LOCK(cs_wallet);
            setInternalKeyPool.clear();
            setExternalKeyPool.clear();
            m_pool_key_to_index.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapWalletTxRet != DBErrors::LOAD_OK)
        return nZapWalletTxRet;

    return DBErrors::LOAD_OK;
}

bool CWallet::SetAddressBookWithDB(WalletBatch& batch, const CTxDestination& address, const std::string& strName, const std::string& strPurpose)
{
    bool fUpdated = false;
    {
        LOCK(cs_wallet);
        std::map<CTxDestination, CAddressBookData>::iterator mi = mapAddressBook.find(address);
        fUpdated = mi != mapAddressBook.end();
        mapAddressBook[address].name = strName;
        if (!strPurpose.empty()) /* update purpose only if requested */
            mapAddressBook[address].purpose = strPurpose;
    }
    NotifyAddressBookChanged(this, address, strName, ::IsMine(*this, address) != ISMINE_NO,
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
    {
        LOCK(cs_wallet);

        // Delete destdata tuples associated with address
        std::string strAddress = EncodeDestination(address);
        for (const std::pair<const std::string, std::string> &item : mapAddressBook[address].destdata)
        {
            WalletBatch(*database).EraseDestData(strAddress, item.first);
        }
        mapAddressBook.erase(address);
    }

    NotifyAddressBookChanged(this, address, "", ::IsMine(*this, address) != ISMINE_NO, "", CT_DELETED);

    WalletBatch(*database).ErasePurpose(EncodeDestination(address));
    return WalletBatch(*database).EraseName(EncodeDestination(address));
}

const std::string& CWallet::GetLabelName(const CScript& scriptPubKey) const
{
    CTxDestination address;
    if (ExtractDestination(scriptPubKey, address) && !scriptPubKey.IsUnspendable()) {
        auto mi = mapAddressBook.find(address);
        if (mi != mapAddressBook.end()) {
            return mi->second.name;
        }
    }
    // A scriptPubKey that doesn't have an entry in the address book is
    // associated with the default label ("").
    const static std::string DEFAULT_LABEL_NAME;
    return DEFAULT_LABEL_NAME;
}

/**
 * Mark old keypool keys as used,
 * and generate all new keys
 */
bool CWallet::NewKeyPool()
{
    if (IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        return false;
    }
    {
        LOCK(cs_wallet);
        WalletBatch batch(*database);

        for (const int64_t nIndex : setInternalKeyPool) {
            batch.ErasePool(nIndex);
        }
        setInternalKeyPool.clear();

        for (const int64_t nIndex : setExternalKeyPool) {
            batch.ErasePool(nIndex);
        }
        setExternalKeyPool.clear();

        for (const int64_t nIndex : set_pre_split_keypool) {
            batch.ErasePool(nIndex);
        }
        set_pre_split_keypool.clear();

        m_pool_key_to_index.clear();

        if (!TopUpKeyPool()) {
            return false;
        }
        WalletLogPrintf("CWallet::NewKeyPool rewrote keypool\n");
    }
    return true;
}

size_t CWallet::KeypoolCountExternalKeys()
{
    AssertLockHeld(cs_wallet);
    return setExternalKeyPool.size() + set_pre_split_keypool.size();
}

void CWallet::LoadKeyPool(int64_t nIndex, const CKeyPool &keypool)
{
    AssertLockHeld(cs_wallet);
    if (keypool.m_pre_split) {
        set_pre_split_keypool.insert(nIndex);
    } else if (keypool.fInternal) {
        setInternalKeyPool.insert(nIndex);
    } else {
        setExternalKeyPool.insert(nIndex);
    }
    m_max_keypool_index = std::max(m_max_keypool_index, nIndex);
    m_pool_key_to_index[keypool.vchPubKey.GetID()] = nIndex;

    // If no metadata exists yet, create a default with the pool key's
    // creation time. Note that this may be overwritten by actually
    // stored metadata for that key later, which is fine.
    CKeyID keyid = keypool.vchPubKey.GetID();
    if (mapKeyMetadata.count(keyid) == 0)
        mapKeyMetadata[keyid] = CKeyMetadata(keypool.nTime);
}

bool CWallet::TopUpKeyPool(unsigned int kpSize)
{
    if (!CanGenerateKeys()) {
        return false;
    }
    {
        LOCK(cs_wallet);

        if (IsLocked()) return false;

        // Top up key pool
        unsigned int nTargetSize;
        if (kpSize > 0)
            nTargetSize = kpSize;
        else
            nTargetSize = std::max(gArgs.GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t) 0);

        // count amount of available keys (internal, external)
        // make sure the keypool of external and internal keys fits the user selected target (-keypool)
        int64_t missingExternal = std::max(std::max((int64_t) nTargetSize, (int64_t) 1) - (int64_t)setExternalKeyPool.size(), (int64_t) 0);
        int64_t missingInternal = std::max(std::max((int64_t) nTargetSize, (int64_t) 1) - (int64_t)setInternalKeyPool.size(), (int64_t) 0);

        if (!IsHDEnabled() || !CanSupportFeature(FEATURE_HD_SPLIT))
        {
            // don't create extra internal keys
            missingInternal = 0;
        }
        bool internal = false;
        WalletBatch batch(*database);
        for (int64_t i = missingInternal + missingExternal; i--;)
        {
            if (i < missingInternal) {
                internal = true;
            }

            CPubKey pubkey(GenerateNewKey(batch, internal));
            AddKeypoolPubkeyWithDB(pubkey, internal, batch);
        }
        if (missingInternal + missingExternal > 0) {
            WalletLogPrintf("keypool added %d keys (%d internal), size=%u (%u internal)\n", missingInternal + missingExternal, missingInternal, setInternalKeyPool.size() + setExternalKeyPool.size() + set_pre_split_keypool.size(), setInternalKeyPool.size());
        }
    }
    NotifyCanGetAddressesChanged();
    return true;
}

void CWallet::AddKeypoolPubkeyWithDB(const CPubKey& pubkey, const bool internal, WalletBatch& batch)
{
    LOCK(cs_wallet);
    assert(m_max_keypool_index < std::numeric_limits<int64_t>::max()); // How in the hell did you use so many keys?
    int64_t index = ++m_max_keypool_index;
    if (!batch.WritePool(index, CKeyPool(pubkey, internal))) {
        throw std::runtime_error(std::string(__func__) + ": writing imported pubkey failed");
    }
    if (internal) {
        setInternalKeyPool.insert(index);
    } else {
        setExternalKeyPool.insert(index);
    }
    m_pool_key_to_index[pubkey.GetID()] = index;
}

bool CWallet::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool, bool fRequestedInternal)
{
    nIndex = -1;
    keypool.vchPubKey = CPubKey();
    {
        LOCK(cs_wallet);

        TopUpKeyPool();

        bool fReturningInternal = fRequestedInternal;
        fReturningInternal &= (IsHDEnabled() && CanSupportFeature(FEATURE_HD_SPLIT)) || IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
        bool use_split_keypool = set_pre_split_keypool.empty();
        std::set<int64_t>& setKeyPool = use_split_keypool ? (fReturningInternal ? setInternalKeyPool : setExternalKeyPool) : set_pre_split_keypool;

        // Get the oldest key
        if (setKeyPool.empty()) {
            return false;
        }

        WalletBatch batch(*database);

        auto it = setKeyPool.begin();
        nIndex = *it;
        setKeyPool.erase(it);
        if (!batch.ReadPool(nIndex, keypool)) {
            throw std::runtime_error(std::string(__func__) + ": read failed");
        }
        CPubKey pk;
        if (!GetPubKey(keypool.vchPubKey.GetID(), pk)) {
            throw std::runtime_error(std::string(__func__) + ": unknown key in key pool");
        }
        // If the key was pre-split keypool, we don't care about what type it is
        if (use_split_keypool && keypool.fInternal != fReturningInternal) {
            throw std::runtime_error(std::string(__func__) + ": keypool entry misclassified");
        }
        if (!keypool.vchPubKey.IsValid()) {
            throw std::runtime_error(std::string(__func__) + ": keypool entry invalid");
        }

        m_pool_key_to_index.erase(keypool.vchPubKey.GetID());
        WalletLogPrintf("keypool reserve %d\n", nIndex);
    }
    NotifyCanGetAddressesChanged();
    return true;
}

void CWallet::KeepKey(int64_t nIndex)
{
    // Remove from key pool
    WalletBatch batch(*database);
    batch.ErasePool(nIndex);
    WalletLogPrintf("keypool keep %d\n", nIndex);
}

void CWallet::ReturnKey(int64_t nIndex, bool fInternal, const CPubKey& pubkey)
{
    // Return to key pool
    {
        LOCK(cs_wallet);
        if (fInternal) {
            setInternalKeyPool.insert(nIndex);
        } else if (!set_pre_split_keypool.empty()) {
            set_pre_split_keypool.insert(nIndex);
        } else {
            setExternalKeyPool.insert(nIndex);
        }
        m_pool_key_to_index[pubkey.GetID()] = nIndex;
        NotifyCanGetAddressesChanged();
    }
    WalletLogPrintf("keypool return %d\n", nIndex);
}

bool CWallet::GetKeyFromPool(CPubKey& result, bool internal)
{
    if (!CanGetAddresses(internal)) {
        return false;
    }

    CKeyPool keypool;
    {
        LOCK(cs_wallet);
        int64_t nIndex;
        if (!ReserveKeyFromKeyPool(nIndex, keypool, internal) && !IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
            if (IsLocked()) return false;
            WalletBatch batch(*database);
            result = GenerateNewKey(batch, internal);
            return true;
        }
        KeepKey(nIndex);
        result = keypool.vchPubKey;
    }
    return true;
}

bool CWallet::GetNewDestination(const OutputType type, const std::string label, CTxDestination& dest, std::string& error)
{
    LOCK(cs_wallet);
    error.clear();

    TopUpKeyPool();

    // Generate a new key that is added to wallet
    CPubKey new_key;
    if (!GetKeyFromPool(new_key)) {
        error = "Error: Keypool ran out, please call keypoolrefill first";
        return false;
    }
    LearnRelatedScripts(new_key, type);
    dest = GetDestinationForKey(new_key, type);

    SetAddressBook(dest, label, "receive");
    return true;
}

bool CWallet::GetNewChangeDestination(const OutputType type, CTxDestination& dest, std::string& error)
{
    error.clear();

    TopUpKeyPool();

    ReserveDestination reservedest(this);
    if (!reservedest.GetReservedDestination(type, dest, true)) {
        error = "Error: Keypool ran out, please call keypoolrefill first";
        return false;
    }

    reservedest.KeepDestination();
    return true;
}

static int64_t GetOldestKeyTimeInPool(const std::set<int64_t>& setKeyPool, WalletBatch& batch) {
    if (setKeyPool.empty()) {
        return GetTime();
    }

    CKeyPool keypool;
    int64_t nIndex = *(setKeyPool.begin());
    if (!batch.ReadPool(nIndex, keypool)) {
        throw std::runtime_error(std::string(__func__) + ": read oldest key in keypool failed");
    }
    assert(keypool.vchPubKey.IsValid());
    return keypool.nTime;
}

int64_t CWallet::GetOldestKeyPoolTime()
{
    LOCK(cs_wallet);

    WalletBatch batch(*database);

    // load oldest key from keypool, get time and return
    int64_t oldestKey = GetOldestKeyTimeInPool(setExternalKeyPool, batch);
    if (IsHDEnabled() && CanSupportFeature(FEATURE_HD_SPLIT)) {
        oldestKey = std::max(GetOldestKeyTimeInPool(setInternalKeyPool, batch), oldestKey);
        if (!set_pre_split_keypool.empty()) {
            oldestKey = std::max(GetOldestKeyTimeInPool(set_pre_split_keypool, batch), oldestKey);
        }
    }

    return oldestKey;
}

void CWallet::MarkDestinationsDirty(const std::set<CTxDestination>& destinations) {
    for (auto& entry : mapWallet) {
        CWalletTx& wtx = entry.second;

        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
            CTxDestination dst;

            if (ExtractDestination(wtx.tx->vout[i].scriptPubKey, dst) && destinations.count(dst)) {
                wtx.MarkDirty();
                break;
            }
        }
    }
}

std::map<CTxDestination, CAmount> CWallet::GetAddressBalances(interfaces::Chain::Lock& locked_chain)
{
    std::map<CTxDestination, CAmount> balances;

    {
        LOCK(cs_wallet);
        for (const auto& walletEntry : mapWallet)
        {
            const CWalletTx& wtx = walletEntry.second;

            if (!wtx.IsTrusted(locked_chain))
                continue;

            if (wtx.IsImmatureCoinBase(locked_chain))
                continue;

            int nDepth = wtx.GetDepthInMainChain(locked_chain);
            if (nDepth < (wtx.IsFromMe(ISMINE_ALL) ? 0 : 1))
                continue;

            for (unsigned int i = 0; i < wtx.tx->vout.size(); i++)
            {
                CTxDestination addr;
                if (!IsMine(wtx.tx->vout[i]))
                    continue;
                if(!ExtractDestination(wtx.tx->vout[i].scriptPubKey, addr))
                    continue;

                CAmount n = IsSpent(locked_chain, walletEntry.first, i) ? 0 : wtx.tx->vout[i].nValue;

                if (!balances.count(addr))
                    balances[addr] = 0;
                balances[addr] += n;
            }
        }
    }

    return balances;
}

std::set< std::set<CTxDestination> > CWallet::GetAddressGroupings()
{
    AssertLockHeld(cs_wallet);
    std::set< std::set<CTxDestination> > groupings;
    std::set<CTxDestination> grouping;

    for (const auto& walletEntry : mapWallet)
    {
        const CWalletTx& wtx = walletEntry.second;

        if (wtx.tx->vin.size() > 0)
        {
            bool any_mine = false;
            // group all input addresses with each other
            for (const CTxIn& txin : wtx.tx->vin)
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
               for (const CTxOut& txout : wtx.tx->vout)
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
        for (const auto& txout : wtx.tx->vout)
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
    for (const std::pair<const CTxDestination, CAddressBookData>& item : mapAddressBook)
    {
        const CTxDestination& address = item.first;
        const std::string& strName = item.second.name;
        if (strName == label)
            result.insert(address);
    }
    return result;
}

bool ReserveDestination::GetReservedDestination(const OutputType type, CTxDestination& dest, bool internal)
{
    if (!pwallet->CanGetAddresses(internal)) {
        return false;
    }

    if (nIndex == -1)
    {
        CKeyPool keypool;
        if (!pwallet->ReserveKeyFromKeyPool(nIndex, keypool, internal)) {
            return false;
        }
        vchPubKey = keypool.vchPubKey;
        fInternal = keypool.fInternal;
    }
    assert(vchPubKey.IsValid());
    pwallet->LearnRelatedScripts(vchPubKey, type);
    address = GetDestinationForKey(vchPubKey, type);
    dest = address;
    return true;
}

void ReserveDestination::KeepDestination()
{
    if (nIndex != -1)
        pwallet->KeepKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
    address = CNoDestination();
}

void ReserveDestination::ReturnDestination()
{
    if (nIndex != -1) {
        pwallet->ReturnKey(nIndex, fInternal, vchPubKey);
    }
    nIndex = -1;
    vchPubKey = CPubKey();
    address = CNoDestination();
}

void CWallet::MarkReserveKeysAsUsed(int64_t keypool_id)
{
    AssertLockHeld(cs_wallet);
    bool internal = setInternalKeyPool.count(keypool_id);
    if (!internal) assert(setExternalKeyPool.count(keypool_id) || set_pre_split_keypool.count(keypool_id));
    std::set<int64_t> *setKeyPool = internal ? &setInternalKeyPool : (set_pre_split_keypool.empty() ? &setExternalKeyPool : &set_pre_split_keypool);
    auto it = setKeyPool->begin();

    WalletBatch batch(*database);
    while (it != std::end(*setKeyPool)) {
        const int64_t& index = *(it);
        if (index > keypool_id) break; // set*KeyPool is ordered

        CKeyPool keypool;
        if (batch.ReadPool(index, keypool)) { //TODO: This should be unnecessary
            m_pool_key_to_index.erase(keypool.vchPubKey.GetID());
        }
        LearnAllRelatedScripts(keypool.vchPubKey);
        batch.ErasePool(index);
        WalletLogPrintf("keypool index %d removed\n", index);
        it = setKeyPool->erase(it);
    }
}

void CWallet::LockCoin(const COutPoint& output)
{
    AssertLockHeld(cs_wallet);
    setLockedCoins.insert(output);
}

void CWallet::UnlockCoin(const COutPoint& output)
{
    AssertLockHeld(cs_wallet);
    setLockedCoins.erase(output);
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

/** @} */ // end of Actions

void CWallet::GetKeyBirthTimes(interfaces::Chain::Lock& locked_chain, std::map<CKeyID, int64_t>& mapKeyBirth) const {
    AssertLockHeld(cs_wallet);
    mapKeyBirth.clear();

    // get birth times for keys with metadata
    for (const auto& entry : mapKeyMetadata) {
        if (entry.second.nCreateTime) {
            mapKeyBirth[entry.first] = entry.second.nCreateTime;
        }
    }

    // map in which we'll infer heights of other keys
    const Optional<int> tip_height = locked_chain.getHeight();
    const int max_height = tip_height && *tip_height > 144 ? *tip_height - 144 : 0; // the tip can be reorganized; use a 144-block safety margin
    std::map<CKeyID, int> mapKeyFirstBlock;
    for (const CKeyID &keyid : GetKeys()) {
        if (mapKeyBirth.count(keyid) == 0)
            mapKeyFirstBlock[keyid] = max_height;
    }

    // if there are no such keys, we're done
    if (mapKeyFirstBlock.empty())
        return;

    // find first block that affects those keys, if there are any left
    for (const auto& entry : mapWallet) {
        // iterate over all wallet transactions...
        const CWalletTx &wtx = entry.second;
        if (Optional<int> height = locked_chain.getBlockHeight(wtx.m_confirm.hashBlock)) {
            // ... which are already in a block
            for (const CTxOut &txout : wtx.tx->vout) {
                // iterate over all their outputs
                for (const auto &keyid : GetAffectedKeys(txout.scriptPubKey, *this)) {
                    // ... and all their affected keys
                    std::map<CKeyID, int>::iterator rit = mapKeyFirstBlock.find(keyid);
                    if (rit != mapKeyFirstBlock.end() && *height < rit->second)
                        rit->second = *height;
                }
            }
        }
    }

    // Extract block timestamps for those keys
    for (const auto& entry : mapKeyFirstBlock)
        mapKeyBirth[entry.first] = locked_chain.getBlockTime(entry.second) - TIMESTAMP_WINDOW; // block times can be 2h off
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
        if (chain().findBlock(wtx.m_confirm.hashBlock, nullptr /* block */, &blocktime)) {
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

bool CWallet::AddDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    if (boost::get<CNoDestination>(&dest))
        return false;

    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
    return WalletBatch(*database).WriteDestData(EncodeDestination(dest), key, value);
}

bool CWallet::EraseDestData(const CTxDestination &dest, const std::string &key)
{
    if (!mapAddressBook[dest].destdata.erase(key))
        return false;
    return WalletBatch(*database).EraseDestData(EncodeDestination(dest), key);
}

void CWallet::LoadDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
}

bool CWallet::GetDestData(const CTxDestination &dest, const std::string &key, std::string *value) const
{
    std::map<CTxDestination, CAddressBookData>::const_iterator i = mapAddressBook.find(dest);
    if(i != mapAddressBook.end())
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
    for (const auto& address : mapAddressBook) {
        for (const auto& data : address.second.destdata) {
            if (!data.first.compare(0, prefix.size(), prefix)) {
                values.emplace_back(data.second);
            }
        }
    }
    return values;
}

void CWallet::MarkPreSplitKeys()
{
    WalletBatch batch(*database);
    for (auto it = setExternalKeyPool.begin(); it != setExternalKeyPool.end();) {
        int64_t index = *it;
        CKeyPool keypool;
        if (!batch.ReadPool(index, keypool)) {
            throw std::runtime_error(std::string(__func__) + ": read keypool entry failed");
        }
        keypool.m_pre_split = true;
        if (!batch.WritePool(index, keypool)) {
            throw std::runtime_error(std::string(__func__) + ": writing modified keypool entry failed");
        }
        set_pre_split_keypool.insert(index);
        it = setExternalKeyPool.erase(it);
    }
}

bool CWallet::Verify(interfaces::Chain& chain, const WalletLocation& location, bool salvage_wallet, std::string& error_string, std::string& warning_string)
{
    // Do some checking on wallet path. It should be either a:
    //
    // 1. Path where a directory can be created.
    // 2. Path to an existing directory.
    // 3. Path to a symlink to a directory.
    // 4. For backwards compatibility, the name of a data file in -walletdir.
    LOCK(cs_wallets);
    const fs::path& wallet_path = location.GetPath();
    fs::file_type path_type = fs::symlink_status(wallet_path).type();
    if (!(path_type == fs::file_not_found || path_type == fs::directory_file ||
          (path_type == fs::symlink_file && fs::is_directory(wallet_path)) ||
          (path_type == fs::regular_file && fs::path(location.GetName()).filename() == location.GetName()))) {
        error_string = strprintf(
              "Invalid -wallet path '%s'. -wallet path should point to a directory where wallet.dat and "
              "database/log.?????????? files can be stored, a location where such a directory could be created, "
              "or (for backwards compatibility) the name of an existing data file in -walletdir (%s)",
              location.GetName(), GetWalletDir());
        return false;
    }

    // Make sure that the wallet path doesn't clash with an existing wallet path
    if (IsWalletLoaded(wallet_path)) {
        error_string = strprintf("Error loading wallet %s. Duplicate -wallet filename specified.", location.GetName());
        return false;
    }

    // Keep same database environment instance across Verify/Recover calls below.
    std::unique_ptr<WalletDatabase> database = WalletDatabase::Create(wallet_path);

    try {
        if (!WalletBatch::VerifyEnvironment(wallet_path, error_string)) {
            return false;
        }
    } catch (const fs::filesystem_error& e) {
        error_string = strprintf("Error loading wallet %s. %s", location.GetName(), fsbridge::get_filesystem_error_message(e));
        return false;
    }

    if (salvage_wallet) {
        // Recover readable keypairs:
        CWallet dummyWallet(&chain, WalletLocation(), WalletDatabase::CreateDummy());
        std::string backup_filename;
        // Even if we don't use this lock in this function, we want to preserve
        // lock order in LoadToWallet if query of chain state is needed to know
        // tx status. If lock can't be taken, tx confirmation status may be not
        // reliable.
        auto locked_chain = dummyWallet.LockChain();
        if (!WalletBatch::Recover(wallet_path, (void *)&dummyWallet, WalletBatch::RecoverKeysOnlyFilter, backup_filename)) {
            return false;
        }
    }

    return WalletBatch::VerifyDatabaseFile(wallet_path, warning_string, error_string);
}

std::shared_ptr<CWallet> CWallet::CreateWalletFromFile(interfaces::Chain& chain, const WalletLocation& location, uint64_t wallet_creation_flags)
{
    const std::string walletFile = WalletDataFilePath(location.GetPath()).string();

    // needed to restore wallet transaction meta data after -zapwallettxes
    std::vector<CWalletTx> vWtx;

    if (gArgs.GetBoolArg("-zapwallettxes", false)) {
        chain.initMessage(_("Zapping all transactions from wallet...").translated);

        std::unique_ptr<CWallet> tempWallet = MakeUnique<CWallet>(&chain, location, WalletDatabase::Create(location.GetPath()));
        DBErrors nZapWalletRet = tempWallet->ZapWalletTx(vWtx);
        if (nZapWalletRet != DBErrors::LOAD_OK) {
            chain.initError(strprintf(_("Error loading %s: Wallet corrupted").translated, walletFile));
            return nullptr;
        }
    }

    chain.initMessage(_("Loading wallet...").translated);

    int64_t nStart = GetTimeMillis();
    bool fFirstRun = true;
    // TODO: Can't use std::make_shared because we need a custom deleter but
    // should be possible to use std::allocate_shared.
    std::shared_ptr<CWallet> walletInstance(new CWallet(&chain, location, WalletDatabase::Create(location.GetPath())), ReleaseWallet);
    DBErrors nLoadWalletRet = walletInstance->LoadWallet(fFirstRun);
    if (nLoadWalletRet != DBErrors::LOAD_OK)
    {
        if (nLoadWalletRet == DBErrors::CORRUPT) {
            chain.initError(strprintf(_("Error loading %s: Wallet corrupted").translated, walletFile));
            return nullptr;
        }
        else if (nLoadWalletRet == DBErrors::NONCRITICAL_ERROR)
        {
            chain.initWarning(strprintf(_("Error reading %s! All keys read correctly, but transaction data"
                                          " or address book entries might be missing or incorrect.").translated,
                walletFile));
        }
        else if (nLoadWalletRet == DBErrors::TOO_NEW) {
            chain.initError(strprintf(_("Error loading %s: Wallet requires newer version of %s").translated, walletFile, PACKAGE_NAME));
            return nullptr;
        }
        else if (nLoadWalletRet == DBErrors::NEED_REWRITE)
        {
            chain.initError(strprintf(_("Wallet needed to be rewritten: restart %s to complete").translated, PACKAGE_NAME));
            return nullptr;
        }
        else {
            chain.initError(strprintf(_("Error loading %s").translated, walletFile));
            return nullptr;
        }
    }

    int prev_version = walletInstance->GetVersion();
    if (gArgs.GetBoolArg("-upgradewallet", fFirstRun))
    {
        int nMaxVersion = gArgs.GetArg("-upgradewallet", 0);
        if (nMaxVersion == 0) // the -upgradewallet without argument case
        {
            walletInstance->WalletLogPrintf("Performing wallet upgrade to %i\n", FEATURE_LATEST);
            nMaxVersion = FEATURE_LATEST;
            walletInstance->SetMinVersion(FEATURE_LATEST); // permanently upgrade the wallet immediately
        }
        else
            walletInstance->WalletLogPrintf("Allowing wallet upgrade up to %i\n", nMaxVersion);
        if (nMaxVersion < walletInstance->GetVersion())
        {
            chain.initError(_("Cannot downgrade wallet").translated);
            return nullptr;
        }
        walletInstance->SetMaxVersion(nMaxVersion);
    }

    // Upgrade to HD if explicit upgrade
    if (gArgs.GetBoolArg("-upgradewallet", false)) {
        LOCK(walletInstance->cs_wallet);

        // Do not upgrade versions to any version between HD_SPLIT and FEATURE_PRE_SPLIT_KEYPOOL unless already supporting HD_SPLIT
        int max_version = walletInstance->GetVersion();
        if (!walletInstance->CanSupportFeature(FEATURE_HD_SPLIT) && max_version >= FEATURE_HD_SPLIT && max_version < FEATURE_PRE_SPLIT_KEYPOOL) {
            chain.initError(_("Cannot upgrade a non HD split wallet without upgrading to support pre split keypool. Please use -upgradewallet=169900 or -upgradewallet with no version specified.").translated);
            return nullptr;
        }

        bool hd_upgrade = false;
        bool split_upgrade = false;
        if (walletInstance->CanSupportFeature(FEATURE_HD) && !walletInstance->IsHDEnabled()) {
            walletInstance->WalletLogPrintf("Upgrading wallet to HD\n");
            walletInstance->SetMinVersion(FEATURE_HD);

            // generate a new master key
            CPubKey masterPubKey = walletInstance->GenerateNewSeed();
            walletInstance->SetHDSeed(masterPubKey);
            hd_upgrade = true;
        }
        // Upgrade to HD chain split if necessary
        if (walletInstance->CanSupportFeature(FEATURE_HD_SPLIT)) {
            walletInstance->WalletLogPrintf("Upgrading wallet to use HD chain split\n");
            walletInstance->SetMinVersion(FEATURE_PRE_SPLIT_KEYPOOL);
            split_upgrade = FEATURE_HD_SPLIT > prev_version;
        }
        // Mark all keys currently in the keypool as pre-split
        if (split_upgrade) {
            walletInstance->MarkPreSplitKeys();
        }
        // Regenerate the keypool if upgraded to HD
        if (hd_upgrade) {
            if (!walletInstance->TopUpKeyPool()) {
                chain.initError(_("Unable to generate keys").translated);
                return nullptr;
            }
        }
    }

    if (fFirstRun)
    {
        // ensure this wallet.dat can only be opened by clients supporting HD with chain split and expects no default key
        walletInstance->SetMinVersion(FEATURE_LATEST);

        walletInstance->SetWalletFlags(wallet_creation_flags, false);
        if (!(wallet_creation_flags & (WALLET_FLAG_DISABLE_PRIVATE_KEYS | WALLET_FLAG_BLANK_WALLET))) {
            // generate a new seed
            CPubKey seed = walletInstance->GenerateNewSeed();
            walletInstance->SetHDSeed(seed);
        }

        // Top up the keypool
        if (walletInstance->CanGenerateKeys() && !walletInstance->TopUpKeyPool()) {
            chain.initError(_("Unable to generate initial keys").translated);
            return nullptr;
        }

        auto locked_chain = chain.lock();
        walletInstance->ChainStateFlushed(locked_chain->getTipLocator());
    } else if (wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS) {
        // Make it impossible to disable private keys after creation
        chain.initError(strprintf(_("Error loading %s: Private keys can only be disabled during creation").translated, walletFile));
        return NULL;
    } else if (walletInstance->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        LOCK(walletInstance->cs_KeyStore);
        if (!walletInstance->mapKeys.empty() || !walletInstance->mapCryptedKeys.empty()) {
            chain.initWarning(strprintf(_("Warning: Private keys detected in wallet {%s} with disabled private keys").translated, walletFile));
        }
    }

    if (!gArgs.GetArg("-addresstype", "").empty() && !ParseOutputType(gArgs.GetArg("-addresstype", ""), walletInstance->m_default_address_type)) {
        chain.initError(strprintf(_("Unknown address type '%s'").translated, gArgs.GetArg("-addresstype", "")));
        return nullptr;
    }

    if (!gArgs.GetArg("-changetype", "").empty() && !ParseOutputType(gArgs.GetArg("-changetype", ""), walletInstance->m_default_change_type)) {
        chain.initError(strprintf(_("Unknown change type '%s'").translated, gArgs.GetArg("-changetype", "")));
        return nullptr;
    }

    if (gArgs.IsArgSet("-mintxfee")) {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-mintxfee", ""), n) || 0 == n) {
            chain.initError(AmountErrMsg("mintxfee", gArgs.GetArg("-mintxfee", "")).translated);
            return nullptr;
        }
        if (n > HIGH_TX_FEE_PER_KB) {
            chain.initWarning(AmountHighWarn("-mintxfee").translated + " " +
                              _("This is the minimum transaction fee you pay on every transaction.").translated);
        }
        walletInstance->m_min_fee = CFeeRate(n);
    }

    walletInstance->m_allow_fallback_fee = Params().IsTestChain();
    if (gArgs.IsArgSet("-fallbackfee")) {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-fallbackfee", ""), nFeePerK)) {
            chain.initError(strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'").translated, gArgs.GetArg("-fallbackfee", "")));
            return nullptr;
        }
        if (nFeePerK > HIGH_TX_FEE_PER_KB) {
            chain.initWarning(AmountHighWarn("-fallbackfee").translated + " " +
                              _("This is the transaction fee you may pay when fee estimates are not available.").translated);
        }
        walletInstance->m_fallback_fee = CFeeRate(nFeePerK);
        walletInstance->m_allow_fallback_fee = nFeePerK != 0; //disable fallback fee in case value was set to 0, enable if non-null value
    }
    if (gArgs.IsArgSet("-discardfee")) {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-discardfee", ""), nFeePerK)) {
            chain.initError(strprintf(_("Invalid amount for -discardfee=<amount>: '%s'").translated, gArgs.GetArg("-discardfee", "")));
            return nullptr;
        }
        if (nFeePerK > HIGH_TX_FEE_PER_KB) {
            chain.initWarning(AmountHighWarn("-discardfee").translated + " " +
                              _("This is the transaction fee you may discard if change is smaller than dust at this level").translated);
        }
        walletInstance->m_discard_rate = CFeeRate(nFeePerK);
    }
    if (gArgs.IsArgSet("-paytxfee")) {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-paytxfee", ""), nFeePerK)) {
            chain.initError(AmountErrMsg("paytxfee", gArgs.GetArg("-paytxfee", "")).translated);
            return nullptr;
        }
        if (nFeePerK > HIGH_TX_FEE_PER_KB) {
            chain.initWarning(AmountHighWarn("-paytxfee").translated + " " +
                              _("This is the transaction fee you will pay if you send a transaction.").translated);
        }
        walletInstance->m_pay_tx_fee = CFeeRate(nFeePerK, 1000);
        if (walletInstance->m_pay_tx_fee < chain.relayMinFee()) {
            chain.initError(strprintf(_("Invalid amount for -paytxfee=<amount>: '%s' (must be at least %s)").translated,
                gArgs.GetArg("-paytxfee", ""), chain.relayMinFee().ToString()));
            return nullptr;
        }
    }

    if (gArgs.IsArgSet("-maxtxfee"))
    {
        CAmount nMaxFee = 0;
        if (!ParseMoney(gArgs.GetArg("-maxtxfee", ""), nMaxFee)) {
            chain.initError(AmountErrMsg("maxtxfee", gArgs.GetArg("-maxtxfee", "")).translated);
            return nullptr;
        }
        if (nMaxFee > HIGH_MAX_TX_FEE) {
            chain.initWarning(_("-maxtxfee is set very high! Fees this large could be paid on a single transaction.").translated);
        }
        if (CFeeRate(nMaxFee, 1000) < chain.relayMinFee()) {
            chain.initError(strprintf(_("Invalid amount for -maxtxfee=<amount>: '%s' (must be at least the minrelay fee of %s to prevent stuck transactions)").translated,
                                       gArgs.GetArg("-maxtxfee", ""), chain.relayMinFee().ToString()));
            return nullptr;
        }
        walletInstance->m_default_max_tx_fee = nMaxFee;
    }

    if (chain.relayMinFee().GetFeePerK() > HIGH_TX_FEE_PER_KB) {
        chain.initWarning(AmountHighWarn("-minrelaytxfee").translated + " " +
                    _("The wallet will avoid paying less than the minimum relay fee.").translated);
    }

    walletInstance->m_confirm_target = gArgs.GetArg("-txconfirmtarget", DEFAULT_TX_CONFIRM_TARGET);
    walletInstance->m_spend_zero_conf_change = gArgs.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);
    walletInstance->m_signal_rbf = gArgs.GetBoolArg("-walletrbf", DEFAULT_WALLET_RBF);

    walletInstance->WalletLogPrintf("Wallet completed loading in %15dms\n", GetTimeMillis() - nStart);

    // Try to top up keypool. No-op if the wallet is locked.
    walletInstance->TopUpKeyPool();

    auto locked_chain = chain.lock();
    LOCK(walletInstance->cs_wallet);

    int rescan_height = 0;
    if (!gArgs.GetBoolArg("-rescan", false))
    {
        WalletBatch batch(*walletInstance->database);
        CBlockLocator locator;
        if (batch.ReadBestBlock(locator)) {
            if (const Optional<int> fork_height = locked_chain->findLocatorFork(locator)) {
                rescan_height = *fork_height;
            }
        }
    }

    const Optional<int> tip_height = locked_chain->getHeight();
    if (tip_height) {
        walletInstance->m_last_block_processed = locked_chain->getBlockHash(*tip_height);
    } else {
        walletInstance->m_last_block_processed.SetNull();
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
            while (block_height > 0 && locked_chain->haveBlockOnDisk(block_height - 1) && rescan_height != block_height) {
                --block_height;
            }

            if (rescan_height != block_height) {
                chain.initError(_("Prune: last wallet synchronisation goes beyond pruned data. You need to -reindex (download the whole blockchain again in case of pruned node)").translated);
                return nullptr;
            }
        }

        chain.initMessage(_("Rescanning...").translated);
        walletInstance->WalletLogPrintf("Rescanning last %i blocks (from block %i)...\n", *tip_height - rescan_height, rescan_height);

        // No need to read and scan block if block was created before
        // our wallet birthday (as adjusted for block time variability)
        if (walletInstance->nTimeFirstKey) {
            if (Optional<int> first_block = locked_chain->findFirstBlockWithTimeAndHeight(walletInstance->nTimeFirstKey - TIMESTAMP_WINDOW, rescan_height, nullptr)) {
                rescan_height = *first_block;
            }
        }

        {
            WalletRescanReserver reserver(walletInstance.get());
            if (!reserver.reserve() || (ScanResult::SUCCESS != walletInstance->ScanForWalletTransactions(locked_chain->getBlockHash(rescan_height), {} /* stop block */, reserver, true /* update */).status)) {
                chain.initError(_("Failed to rescan the wallet during initialization").translated);
                return nullptr;
            }
        }
        walletInstance->ChainStateFlushed(locked_chain->getTipLocator());
        walletInstance->database->IncrementUpdateCounter();

        // Restore wallet transaction metadata after -zapwallettxes=1
        if (gArgs.GetBoolArg("-zapwallettxes", false) && gArgs.GetArg("-zapwallettxes", "1") != "2")
        {
            WalletBatch batch(*walletInstance->database);

            for (const CWalletTx& wtxOld : vWtx)
            {
                uint256 hash = wtxOld.GetHash();
                std::map<uint256, CWalletTx>::iterator mi = walletInstance->mapWallet.find(hash);
                if (mi != walletInstance->mapWallet.end())
                {
                    const CWalletTx* copyFrom = &wtxOld;
                    CWalletTx* copyTo = &mi->second;
                    copyTo->mapValue = copyFrom->mapValue;
                    copyTo->vOrderForm = copyFrom->vOrderForm;
                    copyTo->nTimeReceived = copyFrom->nTimeReceived;
                    copyTo->nTimeSmart = copyFrom->nTimeSmart;
                    copyTo->fFromMe = copyFrom->fFromMe;
                    copyTo->nOrderPos = copyFrom->nOrderPos;
                    batch.WriteTx(*copyTo);
                }
            }
        }
    }

    {
        LOCK(cs_wallets);
        for (auto& load_wallet : g_load_wallet_fns) {
            load_wallet(interfaces::MakeWallet(walletInstance));
        }
    }

    // Register with the validation interface. It's ok to do this after rescan since we're still holding locked_chain.
    walletInstance->handleNotifications();

    walletInstance->SetBroadcastTransactions(gArgs.GetBoolArg("-walletbroadcast", DEFAULT_WALLETBROADCAST));

    {
        walletInstance->WalletLogPrintf("setKeyPool.size() = %u\n",      walletInstance->GetKeyPoolSize());
        walletInstance->WalletLogPrintf("mapWallet.size() = %u\n",       walletInstance->mapWallet.size());
        walletInstance->WalletLogPrintf("mapAddressBook.size() = %u\n",  walletInstance->mapAddressBook.size());
    }

    return walletInstance;
}

void CWallet::handleNotifications()
{
    m_chain_notifications_handler = m_chain->handleNotifications(*this);
}

void CWallet::postInitProcess()
{
    auto locked_chain = chain().lock();
    LOCK(cs_wallet);

    // Add wallet transactions that aren't already in a block to mempool
    // Do this here as mempool requires genesis block to be loaded
    ReacceptWalletTransactions(*locked_chain);

    // Update wallet transactions with current mempool transactions.
    chain().requestMempoolTransactions(*this);
}

bool CWallet::BackupWallet(const std::string& strDest)
{
    return database->Backup(strDest);
}

CKeyPool::CKeyPool()
{
    nTime = GetTime();
    fInternal = false;
    m_pre_split = false;
}

CKeyPool::CKeyPool(const CPubKey& vchPubKeyIn, bool internalIn)
{
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
    fInternal = internalIn;
    m_pre_split = false;
}

void CWalletTx::SetConf(Status status, const uint256& block_hash, int posInBlock)
{
    // Update tx status
    m_confirm.status = status;

    // Update the tx's hashBlock
    m_confirm.hashBlock = block_hash;

    // set the position of the transaction in the block
    m_confirm.nIndex = posInBlock;
}

int CWalletTx::GetDepthInMainChain(interfaces::Chain::Lock& locked_chain) const
{
    if (isUnconfirmed() || isAbandoned()) return 0;

    return locked_chain.getBlockDepth(m_confirm.hashBlock) * (isConflicted() ? -1 : 1);
}

int CWalletTx::GetBlocksToMaturity(interfaces::Chain::Lock& locked_chain) const
{
    if (!IsCoinBase())
        return 0;
    int chain_depth = GetDepthInMainChain(locked_chain);
    assert(chain_depth >= 0); // coinbase tx should not be conflicted
    return std::max(0, (COINBASE_MATURITY+1) - chain_depth);
}

bool CWalletTx::IsImmatureCoinBase(interfaces::Chain::Lock& locked_chain) const
{
    // note GetBlocksToMaturity is 0 for non-coinbase tx
    return GetBlocksToMaturity(locked_chain) > 0;
}

void CWallet::LearnRelatedScripts(const CPubKey& key, OutputType type)
{
    if (key.IsCompressed() && (type == OutputType::P2SH_SEGWIT || type == OutputType::BECH32)) {
        CTxDestination witdest = WitnessV0KeyHash(key.GetID());
        CScript witprog = GetScriptForDestination(witdest);
        // Make sure the resulting program is solvable.
        assert(IsSolvable(*this, witprog));
        AddCScript(witprog);
    }
}

void CWallet::LearnAllRelatedScripts(const CPubKey& key)
{
    // OutputType::P2SH_SEGWIT always adds all necessary scripts for all types.
    LearnRelatedScripts(key, OutputType::P2SH_SEGWIT);
}

std::vector<OutputGroup> CWallet::GroupOutputs(const std::vector<COutput>& outputs, bool single_coin) const {
    std::vector<OutputGroup> groups;
    std::map<CTxDestination, OutputGroup> gmap;
    CTxDestination dst;
    for (const auto& output : outputs) {
        if (output.fSpendable) {
            CInputCoin input_coin = output.GetInputCoin();

            size_t ancestors, descendants;
            chain().getTransactionAncestry(output.tx->GetHash(), ancestors, descendants);
            if (!single_coin && ExtractDestination(output.tx->tx->vout[output.i].scriptPubKey, dst)) {
                // Limit output groups to no more than 10 entries, to protect
                // against inadvertently creating a too-large transaction
                // when using -avoidpartialspends
                if (gmap[dst].m_outputs.size() >= OUTPUT_GROUP_MAX_ENTRIES) {
                    groups.push_back(gmap[dst]);
                    gmap.erase(dst);
                }
                gmap[dst].Insert(input_coin, output.nDepth, output.tx->IsFromMe(ISMINE_ALL), ancestors, descendants);
            } else {
                groups.emplace_back(input_coin, output.nDepth, output.tx->IsFromMe(ISMINE_ALL), ancestors, descendants);
            }
        }
    }
    if (!single_coin) {
        for (const auto& it : gmap) groups.push_back(it.second);
    }
    return groups;
}

bool CWallet::GetKeyOrigin(const CKeyID& keyID, KeyOriginInfo& info) const
{
    CKeyMetadata meta;
    {
        LOCK(cs_wallet);
        auto it = mapKeyMetadata.find(keyID);
        if (it != mapKeyMetadata.end()) {
            meta = it->second;
        }
    }
    if (meta.has_key_origin) {
        std::copy(meta.key_origin.fingerprint, meta.key_origin.fingerprint + 4, info.fingerprint);
        info.path = meta.key_origin.path;
    } else { // Single pubkeys get the master fingerprint of themselves
        std::copy(keyID.begin(), keyID.begin() + 4, info.fingerprint);
    }
    return true;
}

bool CWallet::AddKeyOriginWithDB(WalletBatch& batch, const CPubKey& pubkey, const KeyOriginInfo& info)
{
    LOCK(cs_wallet);
    std::copy(info.fingerprint, info.fingerprint + 4, mapKeyMetadata[pubkey.GetID()].key_origin.fingerprint);
    mapKeyMetadata[pubkey.GetID()].key_origin.path = info.path;
    mapKeyMetadata[pubkey.GetID()].has_key_origin = true;
    mapKeyMetadata[pubkey.GetID()].hdKeypath = WriteHDKeypath(info.path);
    return batch.WriteKeyMetadata(mapKeyMetadata[pubkey.GetID()], pubkey, true);
}

bool CWallet::SetCrypted()
{
    LOCK(cs_KeyStore);
    if (fUseCrypto)
        return true;
    if (!mapKeys.empty())
        return false;
    fUseCrypto = true;
    return true;
}

bool CWallet::IsLocked() const
{
    if (!IsCrypted()) {
        return false;
    }
    LOCK(cs_KeyStore);
    return vMasterKey.empty();
}

bool CWallet::Lock()
{
    if (!SetCrypted())
        return false;

    {
        LOCK(cs_KeyStore);
        vMasterKey.clear();
    }

    NotifyStatusChanged(this);
    return true;
}

bool CWallet::Unlock(const CKeyingMaterial& vMasterKeyIn, bool accept_no_keys)
{
    {
        LOCK(cs_KeyStore);
        if (!SetCrypted())
            return false;

        bool keyPass = mapCryptedKeys.empty(); // Always pass when there are no encrypted keys
        bool keyFail = false;
        CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
        for (; mi != mapCryptedKeys.end(); ++mi)
        {
            const CPubKey &vchPubKey = (*mi).second.first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
            CKey key;
            if (!DecryptKey(vMasterKeyIn, vchCryptedSecret, vchPubKey, key))
            {
                keyFail = true;
                break;
            }
            keyPass = true;
            if (fDecryptionThoroughlyChecked)
                break;
        }
        if (keyPass && keyFail)
        {
            LogPrintf("The wallet is probably corrupted: Some keys decrypt but not all.\n");
            throw std::runtime_error("Error unlocking wallet: some keys decrypt but not all. Your wallet file may be corrupt.");
        }
        if (keyFail || (!keyPass && !accept_no_keys))
            return false;
        vMasterKey = vMasterKeyIn;
        fDecryptionThoroughlyChecked = true;
    }
    NotifyStatusChanged(this);
    return true;
}

bool CWallet::HaveKey(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    if (!IsCrypted()) {
        return FillableSigningProvider::HaveKey(address);
    }
    return mapCryptedKeys.count(address) > 0;
}

bool CWallet::GetKey(const CKeyID &address, CKey& keyOut) const
{
    LOCK(cs_KeyStore);
    if (!IsCrypted()) {
        return FillableSigningProvider::GetKey(address, keyOut);
    }

    CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
    if (mi != mapCryptedKeys.end())
    {
        const CPubKey &vchPubKey = (*mi).second.first;
        const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
        return DecryptKey(vMasterKey, vchCryptedSecret, vchPubKey, keyOut);
    }
    return false;
}

bool CWallet::GetWatchPubKey(const CKeyID &address, CPubKey &pubkey_out) const
{
    LOCK(cs_KeyStore);
    WatchKeyMap::const_iterator it = mapWatchKeys.find(address);
    if (it != mapWatchKeys.end()) {
        pubkey_out = it->second;
        return true;
    }
    return false;
}

bool CWallet::GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    LOCK(cs_KeyStore);
    if (!IsCrypted()) {
        if (!FillableSigningProvider::GetPubKey(address, vchPubKeyOut)) {
            return GetWatchPubKey(address, vchPubKeyOut);
        }
        return true;
    }

    CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
    if (mi != mapCryptedKeys.end())
    {
        vchPubKeyOut = (*mi).second.first;
        return true;
    }
    // Check for watch-only pubkeys
    return GetWatchPubKey(address, vchPubKeyOut);
}

std::set<CKeyID> CWallet::GetKeys() const
{
    LOCK(cs_KeyStore);
    if (!IsCrypted()) {
        return FillableSigningProvider::GetKeys();
    }
    std::set<CKeyID> set_address;
    for (const auto& mi : mapCryptedKeys) {
        set_address.insert(mi.first);
    }
    return set_address;
}

bool CWallet::EncryptKeys(CKeyingMaterial& vMasterKeyIn)
{
    LOCK(cs_KeyStore);
    if (!mapCryptedKeys.empty() || IsCrypted())
        return false;

    fUseCrypto = true;
    for (const KeyMap::value_type& mKey : mapKeys)
    {
        const CKey &key = mKey.second;
        CPubKey vchPubKey = key.GetPubKey();
        CKeyingMaterial vchSecret(key.begin(), key.end());
        std::vector<unsigned char> vchCryptedSecret;
        if (!EncryptSecret(vMasterKeyIn, vchSecret, vchPubKey.GetHash(), vchCryptedSecret))
            return false;
        if (!AddCryptedKey(vchPubKey, vchCryptedSecret))
            return false;
    }
    mapKeys.clear();
    return true;
}

bool CWallet::AddKeyPubKeyInner(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    if (!IsCrypted()) {
        return FillableSigningProvider::AddKeyPubKey(key, pubkey);
    }

    if (IsLocked()) {
        return false;
    }

    std::vector<unsigned char> vchCryptedSecret;
    CKeyingMaterial vchSecret(key.begin(), key.end());
    if (!EncryptSecret(vMasterKey, vchSecret, pubkey.GetHash(), vchCryptedSecret)) {
        return false;
    }

    if (!AddCryptedKey(pubkey, vchCryptedSecret)) {
        return false;
    }
    return true;
}


bool CWallet::AddCryptedKeyInner(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    LOCK(cs_KeyStore);
    if (!SetCrypted()) {
        return false;
    }

    mapCryptedKeys[vchPubKey.GetID()] = make_pair(vchPubKey, vchCryptedSecret);
    ImplicitlyLearnRelatedKeyScripts(vchPubKey);
    return true;
}
