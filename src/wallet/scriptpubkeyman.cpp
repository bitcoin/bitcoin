// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>
#include <chainparams.h>
#include <logging.h>
#include <messagesigner.h>
#include <script/descriptor.h>
#include <script/sign.h>
#include <shutdown.h>
#include <util/bip32.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/translation.h>
#include <wallet/bip39.h>
#include <wallet/scriptpubkeyman.h>

namespace wallet {
bool LegacyScriptPubKeyMan::GetNewDestination(CTxDestination& dest, bilingual_str& error)
{
    LOCK(cs_KeyStore);
    error.clear();

    // Generate a new key that is added to wallet
    CPubKey new_key;
    if (!GetKeyFromPool(new_key, false)) {
        error = _("Error: Keypool ran out, please call keypoolrefill first");
        return false;
    }
    //LearnRelatedScripts(new_key);
    dest = PKHash(new_key);
    return true;
}

typedef std::vector<unsigned char> valtype;

namespace {

/**
 * This is an enum that tracks the execution context of a script, similar to
 * SigVersion in script/interpreter. It is separate however because we want to
 * distinguish between top-level scriptPubKey execution and P2SH redeemScript
 * execution (a distinction that has no impact on consensus rules).
 */
enum class IsMineSigVersion
{
    TOP = 0,        //! scriptPubKey execution
    P2SH = 1,       //! P2SH redeemScript
};

/**
 * This is an internal representation of isminetype + invalidity.
 * Its order is significant, as we return the max of all explored
 * possibilities.
 */
enum class IsMineResult
{
    NO = 0,          //! Not ours
    WATCH_ONLY = 1,  //! Included in watch-only balance
    SPENDABLE = 2,   //! Included in all balances
    INVALID = 3,     //! Not spendable by anyone (P2SH inside P2SH)
};

bool PermitsUncompressed(IsMineSigVersion sigversion)
{
    return sigversion == IsMineSigVersion::TOP || sigversion == IsMineSigVersion::P2SH;
}

bool HaveKeys(const std::vector<valtype>& pubkeys, const LegacyScriptPubKeyMan& keystore)
{
    for (const valtype& pubkey : pubkeys) {
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (!keystore.HaveKey(keyID)) return false;
    }
    return true;
}

//! Recursively solve script and return spendable/watchonly/invalid status.
//!
//! @param keystore            legacy key and script store
//! @param scriptPubKey        script to solve
//! @param sigversion          script type (top-level / redeemscript)
//! @param recurse_scripthash  whether to recurse into nested p2sh
//!                            scripts or simply treat any script that has been
//!                            stored in the keystore as spendable
IsMineResult IsMineInner(const LegacyScriptPubKeyMan& keystore, const CScript& scriptPubKey, IsMineSigVersion sigversion, bool recurse_scripthash=true)
{
    IsMineResult ret = IsMineResult::NO;

    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);

    CKeyID keyID;
    switch (whichType) {
    case TxoutType::NONSTANDARD:
    case TxoutType::NULL_DATA:
        break;
    case TxoutType::PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        if (!PermitsUncompressed(sigversion) && vSolutions[0].size() != 33) {
            return IsMineResult::INVALID;
        }
        if (keystore.HaveKey(keyID)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    case TxoutType::PUBKEYHASH:
        keyID = CKeyID(uint160(vSolutions[0]));
        if (!PermitsUncompressed(sigversion)) {
            CPubKey pubkey;
            if (keystore.GetPubKey(keyID, pubkey) && !pubkey.IsCompressed()) {
                return IsMineResult::INVALID;
            }
        }
        if (keystore.HaveKey(keyID)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    case TxoutType::SCRIPTHASH:
    {
        if (sigversion != IsMineSigVersion::TOP) {
            // P2SH inside P2SH is invalid.
            return IsMineResult::INVALID;
        }
        CScriptID scriptID = CScriptID(uint160(vSolutions[0]));
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            ret = std::max(ret, recurse_scripthash ? IsMineInner(keystore, subscript, IsMineSigVersion::P2SH) : IsMineResult::SPENDABLE);
        }
        break;
    }
    case TxoutType::MULTISIG:
    {
        // Never treat bare multisig outputs as ours (they can still be made watchonly-though)
        if (sigversion == IsMineSigVersion::TOP) {
            break;
        }

        // Only consider transactions "mine" if we own ALL the
        // keys involved. Multi-signature transactions that are
        // partially owned (somebody else has a key that can spend
        // them) enable spend-out-from-under-you attacks, especially
        // in shared-wallet situations.
        std::vector<valtype> keys(vSolutions.begin()+1, vSolutions.begin()+vSolutions.size()-1);
        if (!PermitsUncompressed(sigversion)) {
            for (size_t i = 0; i < keys.size(); i++) {
                if (keys[i].size() != 33) {
                    return IsMineResult::INVALID;
                }
            }
        }
        if (HaveKeys(keys, keystore)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    }
    } // no default case, so the compiler can warn about missing cases

    if (ret == IsMineResult::NO && keystore.HaveWatchOnly(scriptPubKey)) {
        ret = std::max(ret, IsMineResult::WATCH_ONLY);
    }
    return ret;
}

} // namespace

isminetype LegacyScriptPubKeyMan::IsMine(const CScript& scriptPubKey) const
{
    switch (IsMineInner(*this, scriptPubKey, IsMineSigVersion::TOP)) {
    case IsMineResult::INVALID:
    case IsMineResult::NO:
        return ISMINE_NO;
    case IsMineResult::WATCH_ONLY:
        return ISMINE_WATCH_ONLY;
    case IsMineResult::SPENDABLE:
        return ISMINE_SPENDABLE;
    }
    assert(false);
}

isminetype LegacyScriptPubKeyMan::IsMine(const CTxDestination& dest) const
{
    CScript script = GetScriptForDestination(dest);
    return IsMine(script);
}

bool LegacyScriptPubKeyMan::CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys)
{
    {
        LOCK(cs_KeyStore);
        assert(mapKeys.empty());

        bool keyPass = mapCryptedKeys.empty(); // Always pass when there are no encrypted keys
        bool keyFail = false;
        CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
        WalletBatch batch(m_storage.GetDatabase());
        for (; mi != mapCryptedKeys.end(); ++mi)
        {
            const CPubKey &vchPubKey = (*mi).second.first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
            CKey key;
            if (!DecryptKey(master_key, vchCryptedSecret, vchPubKey, key))
            {
                keyFail = true;
                break;
            }
            keyPass = true;
            if (fDecryptionThoroughlyChecked)
                break;
            else {
                // Rewrite these encrypted keys with checksums
                batch.WriteCryptedKey(vchPubKey, vchCryptedSecret, mapKeyMetadata[vchPubKey.GetID()]);
            }
        }
        if (keyPass && keyFail)
        {
            LogPrintf("The wallet is probably corrupted: Some keys decrypt but not all.\n");
            throw std::runtime_error("Error unlocking wallet: some keys decrypt but not all. Your wallet file may be corrupt.");
        }
        if (keyFail) {
            return false;
        }
        if (!keyPass && !accept_no_keys && (m_hd_chain.IsNull() || !m_hd_chain.IsCrypted())) {
            return false;
        }

        if(!m_hd_chain.IsNull() && !m_hd_chain.IsCrypted()) {
            // try to decrypt seed and make sure it matches
            CHDChain hdChainTmp;
            if (!DecryptHDChain(master_key, hdChainTmp) || (m_hd_chain.GetID() != hdChainTmp.GetSeedHash())) {
                return false;
            }
        }
        fDecryptionThoroughlyChecked = true;
    }
    return true;
}

bool LegacyScriptPubKeyMan::Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch)
{
    LOCK(cs_KeyStore);

    encrypted_batch = batch;
    if (!mapCryptedKeys.empty()) {
        encrypted_batch = nullptr;
        return false;
    }

    // must get current HD chain before EncryptKeys
    CHDChain hdChainCurrent;
    GetHDChain(hdChainCurrent);

    KeyMap keys_to_encrypt;
    keys_to_encrypt.swap(mapKeys); // Clear mapKeys so AddCryptedKeyInner will succeed.
    for (const KeyMap::value_type& mKey : keys_to_encrypt)
    {
        const CKey &key = mKey.second;
        CPubKey vchPubKey = key.GetPubKey();
        CKeyingMaterial vchSecret(key.begin(), key.end());
        std::vector<unsigned char> vchCryptedSecret;
        if (!EncryptSecret(master_key, vchSecret, vchPubKey.GetHash(), vchCryptedSecret)) {
            encrypted_batch = nullptr;
            return false;
        }
        if (!AddCryptedKey(vchPubKey, vchCryptedSecret)) {
            encrypted_batch = nullptr;
            return false;
        }
    }

    if (!hdChainCurrent.IsNull()) {
        bool res = EncryptHDChain(master_key, m_hd_chain);
        assert(res);
        res = LoadHDChain(m_hd_chain);
        assert(res);

        CHDChain hdChainCrypted;
        res = GetHDChain(hdChainCrypted);
        assert(res);

        // ids should match, seed hashes should not
        assert(hdChainCurrent.GetID() == hdChainCrypted.GetID());
        assert(hdChainCurrent.GetSeedHash() != hdChainCrypted.GetSeedHash());

        res = AddHDChain(*encrypted_batch, hdChainCrypted);
        assert(res);
    }

    encrypted_batch = nullptr;
    return true;
}

bool LegacyScriptPubKeyMan::GetReservedDestination(bool internal, CTxDestination& address, int64_t& index, CKeyPool& keypool, bilingual_str& error)
{
    LOCK(cs_KeyStore);
    if (!CanGetAddresses(internal)) {
        error = _("Error: Keypool ran out, please call keypoolrefill first");
        return false;
    }

    if (!ReserveKeyFromKeyPool(index, keypool, internal)) {
        error = _("Error: Keypool ran out, please call keypoolrefill first");
        return false;
    }
    // TODO: unify with bitcoin and use here GetDestinationForKey even if we have no type
    address = PKHash(keypool.vchPubKey);
    return true;
}

void LegacyScriptPubKeyMan::MarkUnusedAddresses(WalletBatch &batch, const CScript& script, const std::optional<int64_t>& block_time)
{
    LOCK(cs_KeyStore);
    // extract addresses and check if they match with an unused keypool key
    for (const auto& keyid : GetAffectedKeys(script, *this)) {
        std::map<CKeyID, int64_t>::const_iterator mi = m_pool_key_to_index.find(keyid);
        if (mi != m_pool_key_to_index.end()) {
            WalletLogPrintf("%s: Detected a used keypool key, mark all keypool key up to this key as used\n", __func__);
            MarkReserveKeysAsUsed(mi->second);

            if (!TopUpInner()) {
                WalletLogPrintf("%s: Topping up keypool failed (locked wallet)\n", __func__);
            }
        }
        if (block_time) {
            if (mapKeyMetadata[keyid].nCreateTime > *block_time) {
                WalletLogPrintf("%s: Found a key which appears to be used earlier than we expected, updating metadata\n", __func__);
                CPubKey vchPubKey;
                bool res = GetPubKey(keyid, vchPubKey);
                assert(res); // this should never fail
                mapKeyMetadata[keyid].nCreateTime = *block_time;
                batch.WriteKeyMetadata(mapKeyMetadata[keyid], vchPubKey, true);
                UpdateTimeFirstKey(*block_time);
            }
        }
    }
}

void LegacyScriptPubKeyMan::UpgradeKeyMetadata()
{
    LOCK(cs_KeyStore); // mapKeyMetadata
    if (m_storage.IsLocked(false) || m_storage.IsWalletFlagSet(WALLET_FLAG_KEY_ORIGIN_METADATA) || !IsHDEnabled()) {
        return;
    }

    CHDChain hdChainCurrent;
    if (!GetHDChain(hdChainCurrent))
        throw std::runtime_error(std::string(__func__) + ": GetHDChain failed");
    if (!m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
            return DecryptHDChain(encryption_key, hdChainCurrent);
        })) {
        throw std::runtime_error(std::string(__func__) + ": DecryptHDChain failed");
    }

    CExtKey masterKey;
    SecureVector vchSeed = hdChainCurrent.GetSeed();
    masterKey.SetSeed(MakeByteSpan(vchSeed));
    CKeyID master_id = masterKey.key.GetPubKey().GetID();

    std::unique_ptr<WalletBatch> batch = std::make_unique<WalletBatch>(m_storage.GetDatabase());
    size_t cnt = 0;
    for (auto& meta_pair : mapKeyMetadata) {
        const CKeyID& keyid = meta_pair.first;
        CKeyMetadata& meta = meta_pair.second;
        if (!meta.has_key_origin) {
            HDPubKeyMap::const_iterator mi = mapHdPubKeys.find(keyid);
            if (mi == mapHdPubKeys.end()) {
                continue;
            }

            // Add to map
            std::copy(master_id.begin(), master_id.begin() + 4, meta.key_origin.fingerprint);
            if (!ParseHDKeypath(mi->second.GetKeyPath(), meta.key_origin.path)) {
                throw std::runtime_error("Invalid HD keypath");
            }
            meta.has_key_origin = true;
            if (meta.nVersion < CKeyMetadata::VERSION_WITH_KEY_ORIGIN) {
                meta.nVersion = CKeyMetadata::VERSION_WITH_KEY_ORIGIN;
            }

            // Write meta to wallet
            batch->WriteKeyMetadata(meta, mi->second.extPubKey.pubkey, true);
            if (++cnt % 1000 == 0) {
                // avoid creating overlarge in-memory batches in case the wallet contains large amounts of keys
                batch.reset(new WalletBatch(m_storage.GetDatabase()));
            }
        }
    }
}

void LegacyScriptPubKeyMan::GenerateNewHDChain(const SecureString& secureMnemonic, const SecureString& secureMnemonicPassphrase, std::optional<CKeyingMaterial> vMasterKeyOpt)
{
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    CHDChain newHdChain;

    // NOTE: an empty mnemonic means "generate a new one for me"
    // NOTE: default mnemonic passphrase is an empty string
    if (!newHdChain.SetMnemonic(secureMnemonic, secureMnemonicPassphrase, /* fUpdateID = */ true)) {
        throw std::runtime_error(std::string(__func__) + ": SetMnemonic failed");
    }

    // Add default account
    newHdChain.AddAccount();

    // Encryption routine if vMasterKey has been supplied
    if (vMasterKeyOpt.has_value()) {
        auto vMasterKey = vMasterKeyOpt.value();
        if (vMasterKey.size() != WALLET_CRYPTO_KEY_SIZE) {
            throw std::runtime_error(strprintf("%s : invalid vMasterKey size, got %zd (expected %lld)", __func__, vMasterKey.size(), WALLET_CRYPTO_KEY_SIZE));
        }

        // Maintain an unencrypted copy of the chain for sanity checking
        CHDChain prevHdChain{newHdChain};

        bool res = EncryptHDChain(vMasterKey, newHdChain);
        assert(res);
        res = LoadHDChain(newHdChain);
        assert(res);
        res = GetHDChain(newHdChain);
        assert(res);

        // IDs should match, seed hashes should not
        assert(prevHdChain.GetID() == newHdChain.GetID());
        assert(prevHdChain.GetSeedHash() != newHdChain.GetSeedHash());
    }

    if (!AddHDChainSingle(newHdChain)) {
        throw std::runtime_error(std::string(__func__) + ": AddHDChainSingle failed");
    }

    if (!NewKeyPool()) {
        throw std::runtime_error(std::string(__func__) + ": NewKeyPool failed");
    }
}

bool LegacyScriptPubKeyMan::LoadHDChain(const CHDChain& chain)
{
    LOCK(cs_KeyStore);

    if (m_storage.HasEncryptionKeys() != chain.IsCrypted()) return false;

    m_hd_chain = chain;
    return true;
}

bool LegacyScriptPubKeyMan::AddHDChain(WalletBatch &batch, const CHDChain& chain)
{
    LOCK(cs_KeyStore);

    if (!LoadHDChain(chain))
        return false;

    {
        if (chain.IsCrypted() && encrypted_batch) {
            if (!encrypted_batch->WriteHDChain(chain))
                throw std::runtime_error(std::string(__func__) + ": WriteHDChain failed for encrypted batch");
        } else {
            if (!batch.WriteHDChain(chain)) {
                throw std::runtime_error(std::string(__func__) + ": WriteHDChain failed");
            }
        }

        m_storage.UnsetBlankWalletFlag(batch);
    }

    return true;
}

bool LegacyScriptPubKeyMan::AddHDChainSingle(const CHDChain& chain)
{
    WalletBatch batch(m_storage.GetDatabase());
    return AddHDChain(batch, chain);
}

bool LegacyScriptPubKeyMan::GetDecryptedHDChain(CHDChain& hdChainRet) const
{
    LOCK(cs_KeyStore);

    CHDChain hdChainTmp;
    if (!GetHDChain(hdChainTmp)) {
        return false;
    }

    if (!m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
            return DecryptHDChain(encryption_key, hdChainTmp);
        })) {
        return false;
    }

    // make sure seed matches this chain
    if (hdChainTmp.GetID() != hdChainTmp.GetSeedHash())
        return false;

    hdChainRet = hdChainTmp;

    return true;
}

bool LegacyScriptPubKeyMan::EncryptHDChain(const CKeyingMaterial& vMasterKeyIn, CHDChain& chain)
{
    LOCK(cs_KeyStore);
    // should call EncryptKeys first
    if (!m_storage.HasEncryptionKeys())
        return false;

    if (chain.IsCrypted())
        return true;

    // make sure seed matches this chain
    if (chain.GetID() != chain.GetSeedHash())
        return false;

    std::vector<unsigned char> vchCryptedSeed;
    if (!EncryptSecret(vMasterKeyIn, chain.GetSeed(), chain.GetID(), vchCryptedSeed))
        return false;

    CHDChain cryptedChain = chain;
    cryptedChain.SetCrypted(true);

    SecureVector vchSecureCryptedSeed(vchCryptedSeed.begin(), vchCryptedSeed.end());
    if (!cryptedChain.SetSeed(vchSecureCryptedSeed, false))
        return false;

    SecureVector vchMnemonic;
    SecureVector vchMnemonicPassphrase;

    // it's ok to have no mnemonic if wallet was initialized via hdseed
    if (chain.GetMnemonic(vchMnemonic, vchMnemonicPassphrase)) {
        std::vector<unsigned char> vchCryptedMnemonic;
        std::vector<unsigned char> vchCryptedMnemonicPassphrase;

        if (!vchMnemonic.empty() && !EncryptSecret(vMasterKeyIn, vchMnemonic, chain.GetID(), vchCryptedMnemonic))
            return false;
        if (!vchMnemonicPassphrase.empty() && !EncryptSecret(vMasterKeyIn, vchMnemonicPassphrase, chain.GetID(), vchCryptedMnemonicPassphrase))
            return false;

        SecureVector vchSecureCryptedMnemonic(vchCryptedMnemonic.begin(), vchCryptedMnemonic.end());
        SecureVector vchSecureCryptedMnemonicPassphrase(vchCryptedMnemonicPassphrase.begin(), vchCryptedMnemonicPassphrase.end());
        if (!cryptedChain.SetMnemonic(vchSecureCryptedMnemonic, vchSecureCryptedMnemonicPassphrase, false))
            return false;
    }

    chain = cryptedChain;
    return true;
}

bool LegacyScriptPubKeyMan::DecryptHDChain(const CKeyingMaterial& vMasterKeyIn, CHDChain& hdChainRet) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys())
        return true;

    if (m_hd_chain.IsNull())
        return false;

    if (!m_hd_chain.IsCrypted())
        return false;

    SecureVector vchSecureSeed;
    SecureVector vchSecureCryptedSeed = m_hd_chain.GetSeed();
    std::vector<unsigned char> vchCryptedSeed(vchSecureCryptedSeed.begin(), vchSecureCryptedSeed.end());
    if (!DecryptSecret(vMasterKeyIn, vchCryptedSeed, m_hd_chain.GetID(), vchSecureSeed))
        return false;

    hdChainRet = m_hd_chain;
    if (!hdChainRet.SetSeed(vchSecureSeed, false))
        return false;

    // hash of decrypted seed must match chain id
    if (hdChainRet.GetSeedHash() != m_hd_chain.GetID())
        return false;

    SecureVector vchSecureCryptedMnemonic;
    SecureVector vchSecureCryptedMnemonicPassphrase;

    // it's ok to have no mnemonic if wallet was initialized via hdseed
    if (m_hd_chain.GetMnemonic(vchSecureCryptedMnemonic, vchSecureCryptedMnemonicPassphrase)) {
        SecureVector vchSecureMnemonic;
        SecureVector vchSecureMnemonicPassphrase;

        std::vector<unsigned char> vchCryptedMnemonic(vchSecureCryptedMnemonic.begin(), vchSecureCryptedMnemonic.end());
        std::vector<unsigned char> vchCryptedMnemonicPassphrase(vchSecureCryptedMnemonicPassphrase.begin(), vchSecureCryptedMnemonicPassphrase.end());

        if (!vchCryptedMnemonic.empty() && !DecryptSecret(vMasterKeyIn, vchCryptedMnemonic, m_hd_chain.GetID(), vchSecureMnemonic))
            return false;
        if (!vchCryptedMnemonicPassphrase.empty() && !DecryptSecret(vMasterKeyIn, vchCryptedMnemonicPassphrase, m_hd_chain.GetID(), vchSecureMnemonicPassphrase))
            return false;

        if (!hdChainRet.SetMnemonic(vchSecureMnemonic, vchSecureMnemonicPassphrase, false))
            return false;
    }

    hdChainRet.SetCrypted(false);

    return true;
}

bool LegacyScriptPubKeyMan::IsHDEnabled() const
{
    CHDChain hdChainCurrent;
    return GetHDChain(hdChainCurrent);
}

bool LegacyScriptPubKeyMan::CanGetAddresses(bool internal) const
{
    LOCK(cs_KeyStore);
    // Check if the keypool has keys
    bool keypool_has_keys;
    if (internal) {
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

bool LegacyScriptPubKeyMan::HavePrivateKeys() const
{
    LOCK(cs_KeyStore);
    return !mapKeys.empty() || !mapCryptedKeys.empty();
}

void LegacyScriptPubKeyMan::RewriteDB()
{
    LOCK(cs_KeyStore);
    setInternalKeyPool.clear();
    setExternalKeyPool.clear();
    m_pool_key_to_index.clear();
    // Note: can't top-up keypool here, because wallet is locked.
    // User will be prompted to unlock wallet the next operation
    // that requires a new key.
}

static int64_t GetOldestKeyTimeInPool(const std::set<int64_t>& setKeyPool, WalletBatch& batch) {
    if (setKeyPool.empty()) {
        // if the keypool is empty, return <NOW>
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

std::optional<int64_t> LegacyScriptPubKeyMan::GetOldestKeyPoolTime() const
{
    LOCK(cs_KeyStore);

    WalletBatch batch(m_storage.GetDatabase());
    int64_t oldestKey = GetOldestKeyTimeInPool(setExternalKeyPool, batch);

    if (IsHDEnabled()) {
        oldestKey = std::max(GetOldestKeyTimeInPool(setInternalKeyPool, batch), oldestKey);
    }
    return oldestKey;
}

size_t LegacyScriptPubKeyMan::KeypoolCountExternalKeys() const
{
    LOCK(cs_KeyStore);
    return setExternalKeyPool.size();
}

unsigned int LegacyScriptPubKeyMan::GetKeyPoolSize() const
{
    LOCK(cs_KeyStore);
    return setInternalKeyPool.size() + setExternalKeyPool.size();
}

int64_t LegacyScriptPubKeyMan::GetTimeFirstKey() const
{
    LOCK(cs_KeyStore);
    return nTimeFirstKey;
}

std::unique_ptr<SigningProvider> LegacyScriptPubKeyMan::GetSolvingProvider(const CScript& script) const
{
    return std::make_unique<LegacySigningProvider>(*this);
}

bool LegacyScriptPubKeyMan::CanProvide(const CScript& script, SignatureData& sigdata)
{
    IsMineResult ismine = IsMineInner(*this, script, IsMineSigVersion::TOP, /* recurse_scripthash= */ false);
    if (ismine == IsMineResult::SPENDABLE || ismine == IsMineResult::WATCH_ONLY) {
        // If ismine, it means we recognize keys or script ids in the script, or
        // are watching the script itself, and we can at least provide metadata
        // or solving information, even if not able to sign fully.
        return true;
    } else {
        // If, given the stuff in sigdata, we could make a valid signature, then we can provide for this script
        ProduceSignature(*this, DUMMY_SIGNATURE_CREATOR, script, sigdata);
        if (!sigdata.signatures.empty()) {
            // If we could make signatures, make sure we have a private key to actually make a signature
            bool has_privkeys = false;
            for (const auto& key_sig_pair : sigdata.signatures) {
                has_privkeys |= HaveKey(key_sig_pair.first);
            }
            return has_privkeys;
        }
        return false;
    }
}

bool LegacyScriptPubKeyMan::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const
{
    return ::SignTransaction(tx, this, coins, sighash, input_errors);
}

SigningResult LegacyScriptPubKeyMan::SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const
{
    CKey key;
    if (!GetKey(ToKeyID(pkhash), key)) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }

    if (MessageSign(key, message, str_sig)) {
        return SigningResult::OK;
    }
    return SigningResult::SIGNING_FAILED;
}

bool LegacyScriptPubKeyMan::SignSpecialTxPayload(const uint256& hash, const CKeyID& keyid, std::vector<unsigned char>& vchSig) const
{
    CKey key;
    if (!GetKey(keyid, key)) {
        return false;
    }

    return CHashSigner::SignHash(hash, key, vchSig);
}

TransactionError LegacyScriptPubKeyMan::FillPSBT(PartiallySignedTransaction& psbtx, const PrecomputedTransactionData& txdata, int sighash_type, bool sign, bool bip32derivs, int* n_signed, bool finalize) const
{
    if (n_signed) {
        *n_signed = 0;
    }
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // Get the Sighash type
        if (sign && input.sighash_type != std::nullopt && *input.sighash_type != sighash_type) {
            return TransactionError::SIGHASH_MISMATCH;
        }

        // Check non_witness_utxo has specified prevout
        if (input.non_witness_utxo) {
            if (txin.prevout.n >= input.non_witness_utxo->vout.size()) {
                return TransactionError::MISSING_INPUTS;
            }
        } else {
            // There's no UTXO so we can just skip this now
            continue;
        }
        SignPSBTInput(HidingSigningProvider(this, !sign, !bip32derivs), psbtx, i, &txdata, sighash_type, nullptr, finalize);

        bool signed_one = PSBTInputSigned(input);
        if (n_signed && (signed_one || !sign)) {
            // If sign is false, we assume that we _could_ sign if we get here. This
            // will never have false negatives; it is hard to tell under what i
            // circumstances it could have false positives.
            (*n_signed)++;
        }
    }

    // Fill in the bip32 keypaths and redeemscripts for the outputs so that hardware wallets can identify change
    for (unsigned int i = 0; i < psbtx.tx->vout.size(); ++i) {
        UpdatePSBTOutput(HidingSigningProvider(this, true, !bip32derivs), psbtx, i);
    }

    return TransactionError::OK;
}

std::unique_ptr<CKeyMetadata> LegacyScriptPubKeyMan::GetMetadata(const CTxDestination& dest) const
{
    LOCK(cs_KeyStore);

    CKeyID key_id = GetKeyForDestination(*this, dest);
    if (!key_id.IsNull()) {
        auto it = mapKeyMetadata.find(key_id);
        if (it != mapKeyMetadata.end()) {
            return std::make_unique<CKeyMetadata>(it->second);
        }
    }

    CScript scriptPubKey = GetScriptForDestination(dest);
    auto it = m_script_metadata.find(CScriptID(scriptPubKey));
    if (it != m_script_metadata.end()) {
        return std::make_unique<CKeyMetadata>(it->second);
    }

    return nullptr;
}

uint256 LegacyScriptPubKeyMan::GetID() const
{
    return uint256::ONE;
}

/**
 * Update wallet first key creation time. This should be called whenever keys
 * are added to the wallet, with the oldest key creation time.
 */
void LegacyScriptPubKeyMan::UpdateTimeFirstKey(int64_t nCreateTime)
{
    AssertLockHeld(cs_KeyStore);
    if (nCreateTime <= 1) {
        // Cannot determine birthday information, so set the wallet birthday to
        // the beginning of time.
        nTimeFirstKey = 1;
    } else if (!nTimeFirstKey || nCreateTime < nTimeFirstKey) {
        nTimeFirstKey = nCreateTime;
    }
}

bool LegacyScriptPubKeyMan::LoadKey(const CKey& key, const CPubKey &pubkey)
{
    return AddKeyPubKeyInner(key, pubkey);
}

bool LegacyScriptPubKeyMan::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    WalletBatch batch(m_storage.GetDatabase());

    return LegacyScriptPubKeyMan::AddKeyPubKeyWithDB(batch, secret, pubkey);
}

bool LegacyScriptPubKeyMan::AddKeyPubKeyWithDB(WalletBatch& batch, const CKey& secret, const CPubKey& pubkey)
{
    AssertLockHeld(cs_KeyStore);

    // Make sure we aren't adding private keys to private key disabled wallets
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));

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

    if (!m_storage.HasEncryptionKeys()) {
        return batch.WriteKey(pubkey,
                                 secret.GetPrivKey(),
                                 mapKeyMetadata[pubkey.GetID()]);
    }
    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}

bool LegacyScriptPubKeyMan::LoadCScript(const CScript& redeemScript)
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

void LegacyScriptPubKeyMan::LoadKeyMetadata(const CKeyID& keyID, const CKeyMetadata& meta)
{
    LOCK(cs_KeyStore);
    UpdateTimeFirstKey(meta.nCreateTime);
    mapKeyMetadata[keyID] = meta;
}

void LegacyScriptPubKeyMan::LoadScriptMetadata(const CScriptID& script_id, const CKeyMetadata& meta)
{
    LOCK(cs_KeyStore);
    UpdateTimeFirstKey(meta.nCreateTime);
    m_script_metadata[script_id] = meta;
}

bool LegacyScriptPubKeyMan::AddKeyPubKeyInner(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return FillableSigningProvider::AddKeyPubKey(key, pubkey);
    }

    if (m_storage.IsLocked(true)) {
        return false;
    }

    std::vector<unsigned char> vchCryptedSecret;
    CKeyingMaterial vchSecret(key.begin(), key.end());
    if (!m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
            return EncryptSecret(encryption_key, vchSecret, pubkey.GetHash(), vchCryptedSecret);
        })) {
        return false;
    }

    if (!AddCryptedKey(pubkey, vchCryptedSecret)) {
        return false;
    }
    return true;
}

bool LegacyScriptPubKeyMan::GetKeyInner(const CKeyID &address, CKey& keyOut) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return FillableSigningProvider::GetKey(address, keyOut);
    }

    CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
    if (mi != mapCryptedKeys.end())
    {
        const CPubKey &vchPubKey = (*mi).second.first;
        const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
        return m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
            return DecryptKey(encryption_key, vchCryptedSecret, vchPubKey, keyOut);
        });
    }
    return false;
}

bool LegacyScriptPubKeyMan::GetPubKeyInner(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
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

bool LegacyScriptPubKeyMan::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret, bool checksum_valid)
{
    // Set fDecryptionThoroughlyChecked to false when the checksum is invalid
    if (!checksum_valid) {
        fDecryptionThoroughlyChecked = false;
    }

    return AddCryptedKeyInner(vchPubKey, vchCryptedSecret);
}

bool LegacyScriptPubKeyMan::HaveKeyInner(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return FillableSigningProvider::HaveKey(address);
    }
    return mapCryptedKeys.count(address) > 0;
}

bool LegacyScriptPubKeyMan::AddCryptedKeyInner(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    LOCK(cs_KeyStore);
    assert(mapKeys.empty());

    mapCryptedKeys[vchPubKey.GetID()] = make_pair(vchPubKey, vchCryptedSecret);
    return true;
}

bool LegacyScriptPubKeyMan::AddCryptedKey(const CPubKey &vchPubKey,
                            const std::vector<unsigned char> &vchCryptedSecret)
{
    if (!AddCryptedKeyInner(vchPubKey, vchCryptedSecret))
        return false;
    {
        LOCK(cs_KeyStore);
        if (encrypted_batch)
            return encrypted_batch->WriteCryptedKey(vchPubKey,
                                                        vchCryptedSecret,
                                                        mapKeyMetadata[vchPubKey.GetID()]);
        else
            return WalletBatch(m_storage.GetDatabase()).WriteCryptedKey(vchPubKey,
                                                            vchCryptedSecret,
                                                            mapKeyMetadata[vchPubKey.GetID()]);
    }
}

bool LegacyScriptPubKeyMan::HaveWatchOnly(const CScript &dest) const
{
    LOCK(cs_KeyStore);
    return setWatchOnly.count(dest) > 0;
}

bool LegacyScriptPubKeyMan::HaveWatchOnly() const
{
    LOCK(cs_KeyStore);
    return (!setWatchOnly.empty());
}

bool LegacyScriptPubKeyMan::GetWatchPubKey(const CKeyID &address, CPubKey &pubkey_out) const
{
    LOCK(cs_KeyStore);
    WatchKeyMap::const_iterator it = mapWatchKeys.find(address);
    if (it != mapWatchKeys.end()) {
        pubkey_out = it->second;
        return true;
    }
    return false;
}

static bool ExtractPubKey(const CScript &dest, CPubKey& pubKeyOut)
{
    std::vector<std::vector<unsigned char>> solutions;
    return Solver(dest, solutions) == TxoutType::PUBKEY &&
        (pubKeyOut = CPubKey(solutions[0])).IsFullyValid();
}

bool LegacyScriptPubKeyMan::RemoveWatchOnly(const CScript &dest)
{
    {
        LOCK(cs_KeyStore);
        setWatchOnly.erase(dest);
        CPubKey pubKey;
        if (ExtractPubKey(dest, pubKey)) {
            mapWatchKeys.erase(pubKey.GetID());
        }
    }

    if (!HaveWatchOnly())
        NotifyWatchonlyChanged(false);
    if (!WalletBatch(m_storage.GetDatabase()).EraseWatchOnly(dest))
        return false;

    return true;
}

bool LegacyScriptPubKeyMan::LoadWatchOnly(const CScript &dest)
{
    return AddWatchOnlyInMem(dest);
}

bool LegacyScriptPubKeyMan::AddWatchOnlyInMem(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.insert(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey)) {
        mapWatchKeys[pubKey.GetID()] = pubKey;
    }
    return true;
}

bool LegacyScriptPubKeyMan::AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest)
{
    if (!AddWatchOnlyInMem(dest))
        return false;
    const CKeyMetadata& meta = m_script_metadata[CScriptID(dest)];
    UpdateTimeFirstKey(meta.nCreateTime);
    NotifyWatchonlyChanged(true);
    if (batch.WriteWatchOnly(dest, meta)) {
        m_storage.UnsetBlankWalletFlag(batch);
        return true;
    }
    return false;
}

bool LegacyScriptPubKeyMan::AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest, int64_t create_time)
{
    m_script_metadata[CScriptID(dest)].nCreateTime = create_time;
    return AddWatchOnlyWithDB(batch, dest);
}

bool LegacyScriptPubKeyMan::AddWatchOnly(const CScript& dest)
{
    WalletBatch batch(m_storage.GetDatabase());
    return AddWatchOnlyWithDB(batch, dest);
}

bool LegacyScriptPubKeyMan::AddWatchOnly(const CScript& dest, int64_t nCreateTime)
{
    m_script_metadata[CScriptID(dest)].nCreateTime = nCreateTime;
    return AddWatchOnly(dest);
}

bool LegacyScriptPubKeyMan::HaveHDKey(const CKeyID &address, CHDChain& hdChainCurrent) const
{
    LOCK(cs_KeyStore);

    if (!mapHdPubKeys.count(address)) return false;
    return GetHDChain(hdChainCurrent);
}

bool LegacyScriptPubKeyMan::HaveKey(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    if (mapHdPubKeys.count(address) > 0)
        return true;
    return HaveKeyInner(address);
}

bool LegacyScriptPubKeyMan::AddHDPubKey(WalletBatch &batch, const CExtPubKey &extPubKey, bool fInternal)
{
    CHDChain hdChainCurrent;
    GetHDChain(hdChainCurrent);

    CHDPubKey hdPubKey;
    hdPubKey.extPubKey = extPubKey;
    hdPubKey.hdchainID = hdChainCurrent.GetID();
    hdPubKey.nChangeIndex = fInternal ? 1 : 0;
    LoadHDPubKey(hdPubKey);

    // check if we need to remove from watch-only
    CScript script;
    script = GetScriptForDestination(PKHash(extPubKey.pubkey));
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);
    script = GetScriptForRawPubKey(extPubKey.pubkey);
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);

    LOCK(cs_KeyStore);

    if (!batch.WriteHDPubKey(hdPubKey, mapKeyMetadata[extPubKey.pubkey.GetID()])) {
        return false;
    }
    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}

bool LegacyScriptPubKeyMan::LoadHDPubKey(const CHDPubKey &hdPubKey)
{
    LOCK(cs_KeyStore);
    mapHdPubKeys[hdPubKey.extPubKey.pubkey.GetID()] = hdPubKey;
    return true;
}

bool LegacyScriptPubKeyMan::GetKey(const CKeyID &address, CKey& keyOut) const
{
    LOCK(cs_KeyStore);
    HDPubKeyMap::const_iterator mi = mapHdPubKeys.find(address);
    if (mi != mapHdPubKeys.end())
    {
        // if the key has been found in mapHdPubKeys, derive it on the fly
        const CHDPubKey &hdPubKey = (*mi).second;
        CHDChain hdChainCurrent;
        if (!GetHDChain(hdChainCurrent))
            throw std::runtime_error(std::string(__func__) + ": GetHDChain failed");
        if (!m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
                return DecryptHDChain(encryption_key, hdChainCurrent);
            })) {
            throw std::runtime_error(std::string(__func__) + ": DecryptHDChain failed");
        }
        // make sure seed matches this chain
        if (hdChainCurrent.GetID() != hdChainCurrent.GetSeedHash())
            throw std::runtime_error(std::string(__func__) + ": Wrong HD chain!");

        CExtKey extkey;
        KeyOriginInfo key_origin_tmp;
        hdChainCurrent.DeriveChildExtKey(hdPubKey.nAccountIndex, hdPubKey.nChangeIndex != 0, hdPubKey.extPubKey.nChild, extkey, key_origin_tmp);
        keyOut = extkey.key;

        return true;
    }
    else {
        return GetKeyInner(address, keyOut);
    }
}

bool LegacyScriptPubKeyMan::GetKeyOrigin(const CKeyID& keyID, KeyOriginInfo& info) const {
    CKeyMetadata meta;
    {
        LOCK(cs_KeyStore);
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

bool LegacyScriptPubKeyMan::AddKeyOriginWithDB(WalletBatch& batch, const CPubKey& pubkey, const KeyOriginInfo& info)
{
    LOCK(cs_KeyStore);
    std::copy(info.fingerprint, info.fingerprint + 4, mapKeyMetadata[pubkey.GetID()].key_origin.fingerprint);
    mapKeyMetadata[pubkey.GetID()].key_origin.path = info.path;
    mapKeyMetadata[pubkey.GetID()].has_key_origin = true;
    return batch.WriteKeyMetadata(mapKeyMetadata[pubkey.GetID()], pubkey, true);
}

bool LegacyScriptPubKeyMan::GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    LOCK(cs_KeyStore);
    HDPubKeyMap::const_iterator mi = mapHdPubKeys.find(address);
    if (mi != mapHdPubKeys.end())
    {
        const CHDPubKey &hdPubKey = (*mi).second;
        vchPubKeyOut = hdPubKey.extPubKey.pubkey;
        return true;
    }
    else
        return GetPubKeyInner(address, vchPubKeyOut);
}

// Writes a keymetadata for a public key. overwrite specifies whether to overwrite an existing metadata for that key if there exists one.
bool LegacyScriptPubKeyMan::WriteKeyMetadata(const CKeyMetadata& meta, const CPubKey& pubkey, const bool overwrite)
{
    return WalletBatch(m_storage.GetDatabase()).WriteKeyMetadata(meta, pubkey, overwrite);
}

CPubKey LegacyScriptPubKeyMan::GenerateNewKey(WalletBatch &batch, uint32_t nAccountIndex, bool fInternal)
{
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET));
    AssertLockHeld(cs_KeyStore);
    bool fCompressed = m_storage.CanSupportFeature(FEATURE_COMPRPUBKEY); // default to compressed public keys if we want 0.6.0 wallets

    CKey secret;

    // Create new metadata
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    CPubKey pubkey;
    // use HD key derivation if HD was enabled during wallet creation and a non-null HD chain is present
    if (IsHDEnabled()) {
        DeriveNewChildKey(batch, metadata, secret, nAccountIndex, fInternal);
        pubkey = secret.GetPubKey();
    } else {
        secret.MakeNewKey(fCompressed);

        // Compressed public keys were introduced in version 0.6.0
        if (fCompressed) {
            m_storage.SetMinVersion(FEATURE_COMPRPUBKEY);
        }

        pubkey = secret.GetPubKey();
        assert(secret.VerifyPubKey(pubkey));

        // Create new metadata
        mapKeyMetadata[pubkey.GetID()] = metadata;
        UpdateTimeFirstKey(nCreationTime);

        if (!AddKeyPubKeyWithDB(batch, secret, pubkey)) {
            throw std::runtime_error(std::string(__func__) + ": AddKey failed");
        }
    }
    return pubkey;
}

void LegacyScriptPubKeyMan::DeriveNewChildKey(WalletBatch &batch, CKeyMetadata& metadata, CKey& secretRet, uint32_t nAccountIndex, bool fInternal)
{
    CHDChain hdChainTmp;
    if (!GetHDChain(hdChainTmp)) {
        throw std::runtime_error(std::string(__func__) + ": GetHDChain failed");
    }

    if (!m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
            return DecryptHDChain(encryption_key, hdChainTmp);
        })) {
        throw std::runtime_error(std::string(__func__) + ": DecryptHDChain failed");
    }
    // make sure seed matches this chain
    if (hdChainTmp.GetID() != hdChainTmp.GetSeedHash())
        throw std::runtime_error(std::string(__func__) + ": Wrong HD chain!");

    CHDAccount acc;
    if (!hdChainTmp.GetAccount(nAccountIndex, acc))
        throw std::runtime_error(std::string(__func__) + ": Wrong HD account!");

    // derive child key at next index, skip keys already known to the wallet
    CExtKey childKey;
    KeyOriginInfo key_origin_tmp;
    uint32_t nChildIndex = fInternal ? acc.nInternalChainCounter : acc.nExternalChainCounter;
    do {
        // NOTE: DeriveChildExtKey updates key_origin, make sure to clear it.
        key_origin_tmp.clear();
        hdChainTmp.DeriveChildExtKey(nAccountIndex, fInternal, nChildIndex, childKey, key_origin_tmp);
        // increment childkey index
        nChildIndex++;
    } while (HaveKey(childKey.key.GetPubKey().GetID()));
    metadata.key_origin = key_origin_tmp;
    assert(!metadata.has_key_origin);
    metadata.has_key_origin = true;
    secretRet = childKey.key;

    CPubKey pubkey = secretRet.GetPubKey();
    assert(secretRet.VerifyPubKey(pubkey));

    // store metadata
    mapKeyMetadata[pubkey.GetID()] = metadata;
    UpdateTimeFirstKey(metadata.nCreateTime);

    // update the chain model in the database
    CHDChain hdChainCurrent;
    GetHDChain(hdChainCurrent);

    if (fInternal) {
        acc.nInternalChainCounter = nChildIndex;
    }
    else {
        acc.nExternalChainCounter = nChildIndex;
    }

    if (!hdChainCurrent.SetAccount(nAccountIndex, acc))
        throw std::runtime_error(std::string(__func__) + ": SetAccount failed");

    if (!AddHDChain(batch, hdChainCurrent)) {
        throw std::runtime_error(std::string(__func__) + ": AddHDChain failed");
    }

    if (!AddHDPubKey(batch, childKey.Neuter(), fInternal))
        throw std::runtime_error(std::string(__func__) + ": AddHDPubKey failed");
}

void LegacyScriptPubKeyMan::LoadKeyPool(int64_t nIndex, const CKeyPool &keypool)
{
    LOCK(cs_KeyStore);
    if (keypool.fInternal) {
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

bool LegacyScriptPubKeyMan::CanGenerateKeys() const
{
    LOCK(cs_KeyStore);
    // TODO : unify with bitcoin after backporting SetupGeneration
    // return IsHDEnabled() || !m_storage.CanSupportFeature(FEATURE_HD);

    if (m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) || m_storage.IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET)) {
        return false;
    }
    return true;
}

/**
 * Mark old keypool keys as used,
 * and generate all new keys
 */
bool LegacyScriptPubKeyMan::NewKeyPool()
{
    if (m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        return false;
    }
    {
        LOCK(cs_KeyStore);
        WalletBatch batch(m_storage.GetDatabase());
        for (const int64_t nIndex : setInternalKeyPool) {
            batch.ErasePool(nIndex);
        }
        setInternalKeyPool.clear();
        for (const int64_t nIndex : setExternalKeyPool) {
            batch.ErasePool(nIndex);
        }
        setExternalKeyPool.clear();

        m_storage.NewKeyPoolCallback();
        m_pool_key_to_index.clear();

        if (!TopUpInner()) {
            return false;
        }

        WalletLogPrintf("LegacyScriptPubKeyMan::NewKeyPool rewrote keypool\n");
    }
    return true;
}

bool LegacyScriptPubKeyMan::TopUp(unsigned int kpSize) {
    LOCK(cs_KeyStore);
    return TopUpInner(kpSize);
}

bool LegacyScriptPubKeyMan::TopUpInner(unsigned int kpSize)
{
    AssertLockHeld(cs_KeyStore);
    if (!CanGenerateKeys()) {
        return false;
    }
    {
        if (m_storage.IsLocked(true)) return false;

        // Top up key pool
        unsigned int nTargetSize;
        if (kpSize > 0)
            nTargetSize = kpSize;
        else
            nTargetSize = std::max(gArgs.GetIntArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t) 0);

        // count amount of available keys (internal, external)
        // make sure the keypool of external and internal keys fits the user selected target (-keypool)
        int64_t amountExternal = setExternalKeyPool.size();
        int64_t amountInternal = setInternalKeyPool.size();
        int64_t missingExternal = std::max(std::max((int64_t) nTargetSize, (int64_t) 1) - amountExternal, (int64_t) 0);
        int64_t missingInternal = std::max(std::max((int64_t) nTargetSize, (int64_t) 1) - amountInternal, (int64_t) 0);

        if (!IsHDEnabled())
        {
            // don't create extra internal keys
            missingInternal = 0;
        }

        const int64_t total_missing = missingInternal + missingExternal;
        if (total_missing == 0) return true;

        constexpr int64_t PROGRESS_REPORT_INTERVAL = 1; // in seconds
        const bool should_show_progress = total_missing > 100;
        const std::string strMsg = _("Topping up keypool…").translated;

        int64_t progress_report_time = GetTime();
        WalletLogPrintf("%s\n", strMsg);
        if (should_show_progress) {
            m_storage.UpdateProgress(strMsg, 0);
        }

        bool fInternal = false;
        int64_t current_index{0};
        WalletBatch batch(m_storage.GetDatabase());

        for (current_index = 0; current_index < total_missing; ++current_index) {
            if (current_index == missingExternal) {
                fInternal = true;
            }

            // TODO: implement keypools for all accounts?
            CPubKey pubkey(GenerateNewKey(batch, 0, fInternal));
            AddKeypoolPubkeyWithDB(pubkey, fInternal, batch);

            if (GetTime() >= progress_report_time + PROGRESS_REPORT_INTERVAL) {
                const double dProgress = 100.f * current_index / total_missing;
                const int iProgress = static_cast<int>(dProgress);
                progress_report_time = GetTime();
                WalletLogPrintf("Still topping up. At key %lld. Progress=%f\n", current_index, dProgress);
                if (should_show_progress && iProgress > 0) {
                    m_storage.UpdateProgress(strMsg, iProgress);
                }
            }
        }
        WalletLogPrintf("Keypool added %d keys, size=%u (%u internal)\n",
                  current_index + 1, setInternalKeyPool.size() + setExternalKeyPool.size(), setInternalKeyPool.size());
        if (should_show_progress) {
            m_storage.UpdateProgress("", 100);
        }
    }
    NotifyCanGetAddressesChanged();
    return true;
}

/*
void LegacyScriptPubKeyMan::AddKeypoolPubkey(const CPubKey& pubkey, const bool internal)
{
    WalletBatch batch(m_storage.GetDatabase());
    AddKeypoolPubkeyWithDB(pubkey, internal, batch);
    NotifyCanGetAddressesChanged();
}
*/

void LegacyScriptPubKeyMan::AddKeypoolPubkeyWithDB(const CPubKey& pubkey, const bool internal, WalletBatch& batch)
{
    LOCK(cs_KeyStore);
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

void LegacyScriptPubKeyMan::KeepDestination(int64_t nIndex)
{
    // Remove from key pool
    {
        LOCK(cs_KeyStore);
        WalletBatch batch(m_storage.GetDatabase());
        bool erased = batch.ErasePool(nIndex);
        m_storage.KeepDestinationCallback(erased);
        CPubKey pubkey;
        bool have_pk = GetPubKey(m_index_to_reserved_key.at(nIndex), pubkey);
        assert(have_pk);
        m_index_to_reserved_key.erase(nIndex);
    }
    WalletLogPrintf("keypool keep %d\n", nIndex);
}

void LegacyScriptPubKeyMan::ReturnDestination(int64_t nIndex, bool fInternal, const CTxDestination&)
{
    // Return to key pool
    {
        LOCK(cs_KeyStore);
        if (fInternal) {
            setInternalKeyPool.insert(nIndex);
        } else {
            setExternalKeyPool.insert(nIndex);
        }
        CKeyID& pubkey_id = m_index_to_reserved_key.at(nIndex);
        m_pool_key_to_index[pubkey_id] = nIndex;
        m_index_to_reserved_key.erase(nIndex);
        NotifyCanGetAddressesChanged();
    }
    WalletLogPrintf("keypool return %d\n", nIndex);
}

bool LegacyScriptPubKeyMan::GetKeyFromPool(CPubKey& result, bool internal)
{
    if (!CanGetAddresses(internal)) {
        return false;
    }

    CKeyPool keypool;
    {
        LOCK(cs_KeyStore);
        int64_t nIndex;
        if (!ReserveKeyFromKeyPool(nIndex, keypool, internal) && !m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
            if (m_storage.IsLocked(true)) return false;
            // TODO: implement keypool for all accouts?
            WalletBatch batch(m_storage.GetDatabase());
            result = GenerateNewKey(batch, 0, internal);
            return true;
        }
        KeepDestination(nIndex);
        result = keypool.vchPubKey;
    }
    return true;
}

bool LegacyScriptPubKeyMan::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool, bool fRequestedInternal)
{
    nIndex = -1;
    keypool.vchPubKey = CPubKey();
    {
        LOCK(cs_KeyStore);

        bool fReturningInternal = fRequestedInternal;
        fReturningInternal &= IsHDEnabled() || m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
        std::set<int64_t>& setKeyPool = fReturningInternal ? setInternalKeyPool : setExternalKeyPool;

        // Get the oldest key
        if (setKeyPool.empty()) {
            return false;
        }

        WalletBatch batch(m_storage.GetDatabase());

        nIndex = *setKeyPool.begin();
        setKeyPool.erase(nIndex);
        if (!batch.ReadPool(nIndex, keypool)) {
            throw std::runtime_error(std::string(__func__) + ": read failed");
        }
        CPubKey pk;
        if (!GetPubKey(keypool.vchPubKey.GetID(), pk)) {
            throw std::runtime_error(std::string(__func__) + ": unknown key in key pool");
        }
        if (keypool.fInternal != fReturningInternal) {
            throw std::runtime_error(std::string(__func__) + ": keypool entry misclassified");
        }
        if (!keypool.vchPubKey.IsValid()) {
            throw std::runtime_error(std::string(__func__) + ": keypool entry invalid");
        }

        assert(m_index_to_reserved_key.count(nIndex) == 0);
        m_index_to_reserved_key[nIndex] = keypool.vchPubKey.GetID();
        m_pool_key_to_index.erase(keypool.vchPubKey.GetID());
        WalletLogPrintf("keypool reserve %d\n", nIndex);
    }
    NotifyCanGetAddressesChanged();
    return true;
}

void LegacyScriptPubKeyMan::MarkReserveKeysAsUsed(int64_t keypool_id)
{
    AssertLockHeld(cs_KeyStore);
    bool internal = setInternalKeyPool.count(keypool_id);
    if (!internal) assert(setExternalKeyPool.count(keypool_id));
    std::set<int64_t> *setKeyPool = internal ? &setInternalKeyPool : &setExternalKeyPool;
    auto it = setKeyPool->begin();

    WalletBatch batch(m_storage.GetDatabase());
    while (it != std::end(*setKeyPool)) {
        const int64_t& index = *(it);
        if (index > keypool_id) break; // set*KeyPool is ordered

        CKeyPool keypool;
        if (batch.ReadPool(index, keypool)) { //TODO: This should be unnecessary
            m_pool_key_to_index.erase(keypool.vchPubKey.GetID());
        }
        batch.ErasePool(index);
        WalletLogPrintf("keypool index %d removed\n", index);
        it = setKeyPool->erase(it);
    }
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

bool LegacyScriptPubKeyMan::AddCScript(const CScript& redeemScript)
{
    WalletBatch batch(m_storage.GetDatabase());
    return AddCScriptWithDB(batch, redeemScript);
}

bool LegacyScriptPubKeyMan::AddCScriptWithDB(WalletBatch& batch, const CScript& redeemScript)
{
    if (!FillableSigningProvider::AddCScript(redeemScript))
        return false;
    if (batch.WriteCScript(Hash160(redeemScript), redeemScript)) {
        m_storage.UnsetBlankWalletFlag(batch);
        return true;
    }
    return false;
}

bool LegacyScriptPubKeyMan::ImportScripts(const std::set<CScript> scripts, int64_t timestamp)
{
    WalletBatch batch(m_storage.GetDatabase());
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

bool LegacyScriptPubKeyMan::ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp)
{
    WalletBatch batch(m_storage.GetDatabase());
    for (const auto& entry : privkey_map) {
        const CKey& key = entry.second;
        CPubKey pubkey = key.GetPubKey();
        const CKeyID& id = entry.first;
        assert(key.VerifyPubKey(pubkey));
        mapKeyMetadata[id].nCreateTime = timestamp;
        // Skip if we already have the key
        if (HaveKey(id)) {
            WalletLogPrintf("Already have key with pubkey %s, skipping\n", HexStr(pubkey));
            continue;
        }
        // If the private key is not present in the wallet, insert it.
        if (!AddKeyPubKeyWithDB(batch, key, pubkey)) {
            return false;
        }
        UpdateTimeFirstKey(timestamp);
    }
    return true;
}

bool LegacyScriptPubKeyMan::ImportPubKeys(const std::vector<CKeyID>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const bool internal, const int64_t timestamp)
{
    WalletBatch batch(m_storage.GetDatabase());
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

bool LegacyScriptPubKeyMan::ImportScriptPubKeys(const std::set<CScript>& script_pub_keys, const bool have_solving_data, const int64_t timestamp)
{
    WalletBatch batch(m_storage.GetDatabase());
    for (const CScript& script : script_pub_keys) {
        if (!have_solving_data || !IsMine(script)) { // Always call AddWatchOnly for non-solvable watch-only, so that watch timestamp gets updated
            if (!AddWatchOnlyWithDB(batch, script, timestamp)) {
                return false;
            }
        }
    }
    return true;
}

std::set<CKeyID> LegacyScriptPubKeyMan::GetKeys() const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return FillableSigningProvider::GetKeys();
    }
    std::set<CKeyID> set_address;
    for (const auto& mi : mapCryptedKeys) {
        set_address.insert(mi.first);
    }
    return set_address;
}

bool LegacyScriptPubKeyMan::GetHDChain(CHDChain& hdChainRet) const
{
    LOCK(cs_KeyStore);
    hdChainRet = m_hd_chain;
    return !m_hd_chain.IsNull();
}

bool DescriptorScriptPubKeyMan::GetNewDestination(CTxDestination& dest, bilingual_str& error)
{
    // Returns true if this descriptor supports getting new addresses. Conditions where we may be unable to fetch them (e.g. locked) are caught later
    if (!CanGetAddresses()) {
        error = _("No addresses available");
        return false;
    }
    {
        LOCK(cs_desc_man);
        assert(m_wallet_descriptor.descriptor->IsSingleType()); // This is a combo descriptor which should not be an active descriptor

        TopUp();

        // Get the scriptPubKey from the descriptor
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        if (m_wallet_descriptor.range_end <= m_max_cached_index && !TopUp(1)) {
            // We can't generate anymore keys
            error = _("Error: Keypool ran out, please call keypoolrefill first");
            return false;
        }
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(m_wallet_descriptor.next_index, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            // We can't generate anymore keys
            error = _("Error: Keypool ran out, please call keypoolrefill first");
            return false;
        }
        const OutputType type{OutputType::LEGACY};
        std::optional<OutputType> out_script_type = m_wallet_descriptor.descriptor->GetOutputType();
        if (out_script_type && out_script_type == type) {
            ExtractDestination(scripts_temp[0], dest);
        } else {
            throw std::runtime_error(std::string(__func__) + ": Types are inconsistent. Stored type does not match type of newly generated address");
        }
        m_wallet_descriptor.next_index++;
        WalletBatch(m_storage.GetDatabase()).WriteDescriptor(GetID(), m_wallet_descriptor);
        return true;
    }
}

isminetype DescriptorScriptPubKeyMan::IsMine(const CScript& script) const
{
    LOCK(cs_desc_man);
    if (m_map_script_pub_keys.count(script) > 0) {
        return ISMINE_SPENDABLE;
    }
    return ISMINE_NO;
}

isminetype DescriptorScriptPubKeyMan::IsMine(const CTxDestination& dest) const
{
    CScript script = GetScriptForDestination(dest);
    return IsMine(script);
}

bool DescriptorScriptPubKeyMan::CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys)
{
    LOCK(cs_desc_man);
    if (!m_map_keys.empty()) {
        return false;
    }

    bool keyPass = m_map_crypted_keys.empty(); // Always pass when there are no encrypted keys
    bool keyFail = false;
    for (const auto& mi : m_map_crypted_keys) {
        const CPubKey &pubkey = mi.second.first;
        const std::vector<unsigned char> &crypted_secret = mi.second.second;
        CKey key;
        if (!DecryptKey(master_key, crypted_secret, pubkey, key)) {
            keyFail = true;
            break;
        }
        // TODO: test for mnemonics
        keyPass = true;
        if (m_decryption_thoroughly_checked)
            break;
    }
    if (keyPass && keyFail) {
        LogPrintf("The wallet is probably corrupted: Some keys decrypt but not all.\n");
        throw std::runtime_error("Error unlocking wallet: some keys decrypt but not all. Your wallet file may be corrupt.");
    }
    if (keyFail || (!keyPass && !accept_no_keys)) {
        return false;
    }
    m_decryption_thoroughly_checked = true;
    return true;
}

bool DescriptorScriptPubKeyMan::Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch)
{
    LOCK(cs_desc_man);
    if (!m_map_crypted_keys.empty()) {
        return false;
    }

    for (const KeyMap::value_type& key_in : m_map_keys)
    {
        const CKey &key = key_in.second;
        CPubKey pubkey = key.GetPubKey();
        assert(pubkey.GetID() == key_in.first);
        const auto mnemonic_in = m_mnemonics.find(key_in.first);
        CKeyingMaterial secret(key.begin(), key.end());
        std::vector<unsigned char> crypted_secret;
        if (!EncryptSecret(master_key, secret, pubkey.GetHash(), crypted_secret)) {
            return false;
        }
        std::vector<unsigned char> crypted_mnemonic;
        std::vector<unsigned char> crypted_mnemonic_passphrase;
        if (mnemonic_in != m_mnemonics.end()) {
            const Mnemonic mnemonic = mnemonic_in->second;

            CKeyingMaterial mnemonic_secret(mnemonic.first.begin(), mnemonic.first.end());
            CKeyingMaterial mnemonic_passphrase_secret(mnemonic.second.begin(), mnemonic.second.end());
            if (!EncryptSecret(master_key, mnemonic_secret, pubkey.GetHash(), crypted_mnemonic)) {
                return false;
            }
            if (!EncryptSecret(master_key, mnemonic_passphrase_secret, pubkey.GetHash(), crypted_mnemonic_passphrase)) {
                return false;
            }
        }

        m_map_crypted_keys[pubkey.GetID()] = make_pair(pubkey, crypted_secret);
        m_crypted_mnemonics[pubkey.GetID()] = make_pair(crypted_mnemonic, crypted_mnemonic_passphrase);
        batch->WriteCryptedDescriptorKey(GetID(), pubkey, crypted_secret, crypted_mnemonic, crypted_mnemonic_passphrase);
    }
    m_map_keys.clear();
    m_mnemonics.clear();
    return true;
}

bool DescriptorScriptPubKeyMan::GetReservedDestination(bool internal, CTxDestination& address, int64_t& index, CKeyPool& keypool, bilingual_str& error)
{
    LOCK(cs_desc_man);
    bool result = GetNewDestination(address, error);
    index = m_wallet_descriptor.next_index - 1;
    return result;
}

void DescriptorScriptPubKeyMan::ReturnDestination(int64_t index, bool internal, const CTxDestination& addr)
{
    LOCK(cs_desc_man);
    // Only return when the index was the most recent
    if (m_wallet_descriptor.next_index - 1 == index) {
        m_wallet_descriptor.next_index--;
    }
    WalletBatch(m_storage.GetDatabase()).WriteDescriptor(GetID(), m_wallet_descriptor);
    NotifyCanGetAddressesChanged();
}

std::map<CKeyID, CKey> DescriptorScriptPubKeyMan::GetKeys() const
{
    AssertLockHeld(cs_desc_man);
    if (m_storage.HasEncryptionKeys() && !m_storage.IsLocked(true)) {
        KeyMap keys;
        for (auto key_pair : m_map_crypted_keys) {
            const CPubKey& pubkey = key_pair.second.first;
            const std::vector<unsigned char>& crypted_secret = key_pair.second.second;
            CKey key;
            m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
                return DecryptKey(encryption_key, crypted_secret, pubkey, key);
            });
            keys[pubkey.GetID()] = key;
        }
        return keys;
    }
    return m_map_keys;
}

bool DescriptorScriptPubKeyMan::TopUp(unsigned int size)
{
    LOCK(cs_desc_man);
    unsigned int target_size;
    if (size > 0) {
        target_size = size;
    } else {
        target_size = std::max(gArgs.GetIntArg("-keypool", DEFAULT_KEYPOOL_SIZE), int64_t{1});
    }

    // Calculate the new range_end
    int32_t new_range_end = std::max(m_wallet_descriptor.next_index + (int32_t)target_size, m_wallet_descriptor.range_end);

    // If the descriptor is not ranged, we actually just want to fill the first cache item
    if (!m_wallet_descriptor.descriptor->IsRange()) {
        new_range_end = 1;
        m_wallet_descriptor.range_end = 1;
        m_wallet_descriptor.range_start = 0;
    }

    FlatSigningProvider provider;
    provider.keys = GetKeys();

    WalletBatch batch(m_storage.GetDatabase());
    uint256 id = GetID();
    for (int32_t i = m_max_cached_index + 1; i < new_range_end; ++i) {
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        DescriptorCache temp_cache;
        // Maybe we have a cached xpub and we can expand from the cache first
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(i, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            if (!m_wallet_descriptor.descriptor->Expand(i, provider, scripts_temp, out_keys, &temp_cache)) return false;
        }
        // Add all of the scriptPubKeys to the scriptPubKey set
        for (const CScript& script : scripts_temp) {
            m_map_script_pub_keys[script] = i;
        }
        for (const auto& pk_pair : out_keys.pubkeys) {
            const CPubKey& pubkey = pk_pair.second;
            if (m_map_pubkeys.count(pubkey) != 0) {
                // We don't need to give an error here.
                // It doesn't matter which of many valid indexes the pubkey has, we just need an index where we can derive it and it's private key
                continue;
            }
            m_map_pubkeys[pubkey] = i;
        }
        // Merge and write the cache
        DescriptorCache new_items = m_wallet_descriptor.cache.MergeAndDiff(temp_cache);
        if (!batch.WriteDescriptorCacheItems(id, new_items)) {
            throw std::runtime_error(std::string(__func__) + ": writing cache items failed");
        }
        m_max_cached_index++;
    }
    m_wallet_descriptor.range_end = new_range_end;
    batch.WriteDescriptor(GetID(), m_wallet_descriptor);

    // By this point, the cache size should be the size of the entire range
    assert(m_wallet_descriptor.range_end - 1 == m_max_cached_index);

    NotifyCanGetAddressesChanged();
    return true;
}

void DescriptorScriptPubKeyMan::MarkUnusedAddresses(WalletBatch &batch, const CScript& script, const std::optional<int64_t>& block_time)
{
    LOCK(cs_desc_man);
    if (IsMine(script)) {
        int32_t index = m_map_script_pub_keys[script];
        if (index >= m_wallet_descriptor.next_index) {
            WalletLogPrintf("%s: Detected a used keypool item at index %d, mark all keypool items up to this item as used\n", __func__, index);
            m_wallet_descriptor.next_index = index + 1;
        }
        if (!TopUp()) {
            WalletLogPrintf("%s: Topping up keypool failed (locked wallet)\n", __func__);
        }
    }
}

void DescriptorScriptPubKeyMan::AddDescriptorKey(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_desc_man);
    WalletBatch batch(m_storage.GetDatabase());
    if (!AddDescriptorKeyWithDB(batch, key, pubkey, "", "")) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor private key failed");
    }
}

bool DescriptorScriptPubKeyMan::AddDescriptorKeyWithDB(WalletBatch& batch, const CKey& key, const CPubKey &pubkey, const SecureString& mnemonic, const SecureString& mnemonic_passphrase)
{
    AssertLockHeld(cs_desc_man);
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));

    // Check if provided key already exists
    if (m_map_keys.find(pubkey.GetID()) != m_map_keys.end() ||
        m_map_crypted_keys.find(pubkey.GetID()) != m_map_crypted_keys.end()) {
        return true;
    }

    if (m_storage.HasEncryptionKeys()) {
        if (m_storage.IsLocked(true)) {
            return false;
        }

        std::vector<unsigned char> crypted_secret;
        std::vector<unsigned char> crypted_mnemonic;
        std::vector<unsigned char> crypted_mnemonic_passphrase;
        CKeyingMaterial secret(key.begin(), key.end());
        CKeyingMaterial mnemonic_secret(mnemonic.begin(), mnemonic.end());
        CKeyingMaterial mnemonic_passphrase_secret(mnemonic_passphrase.begin(), mnemonic_passphrase.end());
        if (!m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
                if (!EncryptSecret(encryption_key, secret, pubkey.GetHash(), crypted_secret)) return false;
                if (!mnemonic.empty()) {
                    if (!EncryptSecret(encryption_key, mnemonic_secret, pubkey.GetHash(), crypted_mnemonic)) {
                        return false;
                    }
                    if (!EncryptSecret(encryption_key, mnemonic_passphrase_secret, pubkey.GetHash(), crypted_mnemonic_passphrase)) {
                        return false;
                    }
                }
                return true;
            })) {
            return false;
        }

        m_map_crypted_keys[pubkey.GetID()] = make_pair(pubkey, crypted_secret);
        m_crypted_mnemonics[pubkey.GetID()] = make_pair(crypted_mnemonic, crypted_mnemonic_passphrase);
        return batch.WriteCryptedDescriptorKey(GetID(), pubkey, crypted_secret, crypted_mnemonic, crypted_mnemonic_passphrase);
    } else {
        m_map_keys[pubkey.GetID()] = key;
        m_mnemonics[pubkey.GetID()] = make_pair(mnemonic, mnemonic_passphrase);
        return batch.WriteDescriptorKey(GetID(), pubkey, key.GetPrivKey(), mnemonic, mnemonic_passphrase);
    }
}

bool DescriptorScriptPubKeyMan::SetupDescriptorGeneration(const CExtKey& master_key, const SecureString& secure_mnemonic, const SecureString& secure_mnemonic_passphrase, bool internal)
{
    LOCK(cs_desc_man);
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));

    // Ignore when there is already a descriptor
    if (m_wallet_descriptor.descriptor) {
        return false;
    }

    if (!secure_mnemonic.empty()) {
        // TODO: remove duplicated code with AddKey()
        SecureVector seed_key_tmp;
        CMnemonic::ToSeed(secure_mnemonic, secure_mnemonic_passphrase, seed_key_tmp);

        CExtKey master_key_tmp;
        master_key_tmp.SetSeed(MakeByteSpan(seed_key_tmp));
        assert(master_key == master_key_tmp);
    }

    int64_t creation_time = GetTime();

    std::string xpub = EncodeExtPubKey(master_key.Neuter());

    // Build descriptor string
    std::string desc_prefix = strprintf("pkh(%s/44'/%d'", xpub, Params().ExtCoinType());
    std::string desc_suffix = "/*)";

    std::string internal_path = internal ? "/1" : "/0";
    std::string desc_str = desc_prefix + "/0'" + internal_path + desc_suffix;

    // Make the descriptor
    FlatSigningProvider keys;
    std::string error;
    std::unique_ptr<Descriptor> desc = Parse(desc_str, keys, error, false);
    WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
    m_wallet_descriptor = w_desc;

    // Store the master private key, and descriptor
    WalletBatch batch(m_storage.GetDatabase());
    if (!AddDescriptorKeyWithDB(batch, master_key.key, master_key.key.GetPubKey(), secure_mnemonic, secure_mnemonic_passphrase)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor master private key failed");
    }
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }

    // TopUp
    TopUp();

    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}

bool DescriptorScriptPubKeyMan::IsHDEnabled() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.descriptor->IsRange();
}

bool DescriptorScriptPubKeyMan::CanGetAddresses(bool internal) const
{
    // We can only give out addresses from descriptors that are single type (not combo), ranged,
    // and either have cached keys or can generate more keys (ignoring encryption)
    LOCK(cs_desc_man);
    return m_wallet_descriptor.descriptor->IsSingleType() &&
           m_wallet_descriptor.descriptor->IsRange() &&
           (HavePrivateKeys() || m_wallet_descriptor.next_index < m_wallet_descriptor.range_end);
}

bool DescriptorScriptPubKeyMan::HavePrivateKeys() const
{
    LOCK(cs_desc_man);
    return m_map_keys.size() > 0 || m_map_crypted_keys.size() > 0;
}

std::optional<int64_t> DescriptorScriptPubKeyMan::GetOldestKeyPoolTime() const
{
    // This is only used for getwalletinfo output and isn't relevant to descriptor wallets.
    return std::nullopt;
}


unsigned int DescriptorScriptPubKeyMan::GetKeyPoolSize() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.range_end - m_wallet_descriptor.next_index;
}

int64_t DescriptorScriptPubKeyMan::GetTimeFirstKey() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.creation_time;
}

std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(const CScript& script, bool include_private) const
{
    LOCK(cs_desc_man);

    // Find the index of the script
    auto it = m_map_script_pub_keys.find(script);
    if (it == m_map_script_pub_keys.end()) {
        return nullptr;
    }
    int32_t index = it->second;

    return GetSigningProvider(index, include_private);
}

std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(const CPubKey& pubkey) const
{
    LOCK(cs_desc_man);

    // Find index of the pubkey
    auto it = m_map_pubkeys.find(pubkey);
    if (it == m_map_pubkeys.end()) {
        return nullptr;
    }
    int32_t index = it->second;

    // Always try to get the signing provider with private keys. This function should only be called during signing anyways
    return GetSigningProvider(index, true);
}

std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(int32_t index, bool include_private) const
{
    AssertLockHeld(cs_desc_man);

    std::unique_ptr<FlatSigningProvider> out_keys = std::make_unique<FlatSigningProvider>();

    // Fetch SigningProvider from cache to avoid re-deriving
    auto it = m_map_signing_providers.find(index);
    if (it != m_map_signing_providers.end()) {
        *out_keys = Merge(*out_keys, it->second);
    } else {
        // Get the scripts, keys, and key origins for this script
        std::vector<CScript> scripts_temp;
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(index, m_wallet_descriptor.cache, scripts_temp, *out_keys)) return nullptr;

        // Cache SigningProvider so we don't need to re-derive if we need this SigningProvider again
        m_map_signing_providers[index] = *out_keys;
    }

    if (HavePrivateKeys() && include_private) {
        FlatSigningProvider master_provider;
        master_provider.keys = GetKeys();
        m_wallet_descriptor.descriptor->ExpandPrivate(index, master_provider, *out_keys);
    }

    return out_keys;
}

std::unique_ptr<SigningProvider> DescriptorScriptPubKeyMan::GetSolvingProvider(const CScript& script) const
{
    return GetSigningProvider(script, false);
}

bool DescriptorScriptPubKeyMan::CanProvide(const CScript& script, SignatureData& sigdata)
{
    return IsMine(script);
}

bool DescriptorScriptPubKeyMan::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const
{
    std::unique_ptr<FlatSigningProvider> keys = std::make_unique<FlatSigningProvider>();
    for (const auto& coin_pair : coins) {
        std::unique_ptr<FlatSigningProvider> coin_keys = GetSigningProvider(coin_pair.second.out.scriptPubKey, true);
        if (!coin_keys) {
            continue;
        }
        *keys = Merge(*keys, *coin_keys);
    }

    return ::SignTransaction(tx, keys.get(), coins, sighash, input_errors);
}

SigningResult DescriptorScriptPubKeyMan::SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const
{
    std::unique_ptr<FlatSigningProvider> keys = GetSigningProvider(GetScriptForDestination(pkhash), true);
    if (!keys) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }

    CKey key;
    if (!keys->GetKey(ToKeyID(pkhash), key)) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }

    if (!MessageSign(key, message, str_sig)) {
        return SigningResult::SIGNING_FAILED;
    }
    return SigningResult::OK;
}

bool DescriptorScriptPubKeyMan::SignSpecialTxPayload(const uint256& hash, const CKeyID& keyid, std::vector<unsigned char>& vchSig) const
{
    std::unique_ptr<FlatSigningProvider> keys = GetSigningProvider(GetScriptForDestination(PKHash(keyid)), true);
    if (!keys) {
        return false;
    }

    CKey key;
    if (!keys->GetKey(keyid, key)) {
        return false;
    }

    return CHashSigner::SignHash(hash, key, vchSig);
}

TransactionError DescriptorScriptPubKeyMan::FillPSBT(PartiallySignedTransaction& psbtx, const PrecomputedTransactionData& txdata, int sighash_type, bool sign, bool bip32derivs, int* n_signed, bool finalize) const
{
    if (n_signed) {
        *n_signed = 0;
    }
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // Get the Sighash type
        if (sign && input.sighash_type != std::nullopt && *input.sighash_type != sighash_type) {
            return TransactionError::SIGHASH_MISMATCH;
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
            continue;
        }

        std::unique_ptr<FlatSigningProvider> keys = std::make_unique<FlatSigningProvider>();
        std::unique_ptr<FlatSigningProvider> script_keys = GetSigningProvider(script, sign);
        if (script_keys) {
            *keys = Merge(*keys, *script_keys);
        } else {
            // Maybe there are pubkeys listed that we can sign for
            script_keys = std::make_unique<FlatSigningProvider>();
            for (const auto& pk_pair : input.hd_keypaths) {
                const CPubKey& pubkey = pk_pair.first;
                std::unique_ptr<FlatSigningProvider> pk_keys = GetSigningProvider(pubkey);
                if (pk_keys) {
                    *keys = Merge(*keys, *pk_keys);
                }
            }
        }

        SignPSBTInput(HidingSigningProvider(keys.get(), !sign, !bip32derivs), psbtx, i, &txdata, sighash_type, nullptr, finalize);

        bool signed_one = PSBTInputSigned(input);
        if (n_signed && (signed_one || !sign)) {
            // If sign is false, we assume that we _could_ sign if we get here. This
            // will never have false negatives; it is hard to tell under what i
            // circumstances it could have false positives.
            (*n_signed)++;
        }
    }

    // Fill in the bip32 keypaths and redeemscripts for the outputs so that hardware wallets can identify change
    for (unsigned int i = 0; i < psbtx.tx->vout.size(); ++i) {
        std::unique_ptr<SigningProvider> keys = GetSolvingProvider(psbtx.tx->vout.at(i).scriptPubKey);
        if (!keys) {
            continue;
        }
        UpdatePSBTOutput(HidingSigningProvider(keys.get(), true, !bip32derivs), psbtx, i);
    }

    return TransactionError::OK;
}

std::unique_ptr<CKeyMetadata> DescriptorScriptPubKeyMan::GetMetadata(const CTxDestination& dest) const
{
    std::unique_ptr<SigningProvider> provider = GetSigningProvider(GetScriptForDestination(dest));
    if (provider) {
        KeyOriginInfo orig;
        CKeyID key_id = GetKeyForDestination(*provider, dest);
        if (provider->GetKeyOrigin(key_id, orig)) {
            LOCK(cs_desc_man);
            std::unique_ptr<CKeyMetadata> meta = std::make_unique<CKeyMetadata>();
            meta->key_origin = orig;
            meta->has_key_origin = true;
            meta->nCreateTime = m_wallet_descriptor.creation_time;
            return meta;
        }
    }
    return nullptr;
}

uint256 DescriptorScriptPubKeyMan::GetID() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.id;
}

void DescriptorScriptPubKeyMan::SetCache(const DescriptorCache& cache)
{
    LOCK(cs_desc_man);
    m_wallet_descriptor.cache = cache;
    for (int32_t i = m_wallet_descriptor.range_start; i < m_wallet_descriptor.range_end; ++i) {
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(i, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            throw std::runtime_error("Error: Unable to expand wallet descriptor from cache");
        }
        // Add all of the scriptPubKeys to the scriptPubKey set
        for (const CScript& script : scripts_temp) {
            if (m_map_script_pub_keys.count(script) != 0) {
                throw std::runtime_error(strprintf("Error: Already loaded script at index %d as being at index %d", i, m_map_script_pub_keys[script]));
            }
            m_map_script_pub_keys[script] = i;
        }
        for (const auto& pk_pair : out_keys.pubkeys) {
            const CPubKey& pubkey = pk_pair.second;
            if (m_map_pubkeys.count(pubkey) != 0) {
                // We don't need to give an error here.
                // It doesn't matter which of many valid indexes the pubkey has, we just need an index where we can derive it and it's private key
                continue;
            }
            m_map_pubkeys[pubkey] = i;
        }
        m_max_cached_index++;
    }
}

bool DescriptorScriptPubKeyMan::AddKey(const CKeyID& key_id, const CKey& key, const SecureString& mnemonic, const SecureString& mnemonic_passphrase)
{
    LOCK(cs_desc_man);
    if (!mnemonic.empty()) {
        // TODO: remove duplicated code with AddKey()
        SecureVector seed_key_tmp;
        CMnemonic::ToSeed(mnemonic, mnemonic_passphrase, seed_key_tmp);

        CExtKey master_key_tmp;
        master_key_tmp.SetSeed(MakeByteSpan(seed_key_tmp));
        assert(key == master_key_tmp.key);
    }

    m_map_keys[key_id] = key;
    m_mnemonics[key_id] = make_pair(mnemonic, mnemonic_passphrase);

    return true;
}

bool DescriptorScriptPubKeyMan::AddCryptedKey(const CKeyID& key_id, const CPubKey& pubkey, const std::vector<unsigned char>& crypted_key, const std::vector<unsigned char>& crypted_mnemonic,const std::vector<unsigned char>& crypted_mnemonic_passphrase)
{
    LOCK(cs_desc_man);
    if (!m_map_keys.empty()) {
        return false;
    }

    m_map_crypted_keys[key_id] = make_pair(pubkey, crypted_key);
    m_crypted_mnemonics[key_id] = make_pair(crypted_mnemonic, crypted_mnemonic_passphrase);
    return true;
}

bool DescriptorScriptPubKeyMan::HasWalletDescriptor(const WalletDescriptor& desc) const
{
    LOCK(cs_desc_man);
    return !m_wallet_descriptor.id.IsNull() && !desc.id.IsNull() && m_wallet_descriptor.id == desc.id;
}

void DescriptorScriptPubKeyMan::WriteDescriptor()
{
    LOCK(cs_desc_man);
    WalletBatch batch(m_storage.GetDatabase());
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }
}

WalletDescriptor DescriptorScriptPubKeyMan::GetWalletDescriptor() const
{
    return m_wallet_descriptor;
}

std::vector<CScript> DescriptorScriptPubKeyMan::GetScriptPubKeys() const
{
    LOCK(cs_desc_man);
    std::vector<CScript> script_pub_keys;
    script_pub_keys.reserve(m_map_script_pub_keys.size());

    for (auto const& script_pub_key: m_map_script_pub_keys) {
        script_pub_keys.push_back(script_pub_key.first);
    }
    return script_pub_keys;
}

bool DescriptorScriptPubKeyMan::GetDescriptorString(std::string& out, const bool priv) const
{
    LOCK(cs_desc_man);

    FlatSigningProvider provider;
    provider.keys = GetKeys();
    if (priv) {
        // For the private version, always return the master key to avoid
        // exposing child private keys. The risk implications of exposing child
        // private keys together with the parent xpub may be non-obvious for users.
        return m_wallet_descriptor.descriptor->ToPrivateString(provider, out);
    }

    return m_wallet_descriptor.descriptor->ToNormalizedString(provider, out, &m_wallet_descriptor.cache);
}

bool DescriptorScriptPubKeyMan::GetMnemonicString(SecureString& mnemonic_out, SecureString& mnemonic_passphrase_out) const
{
    LOCK(cs_desc_man);

    mnemonic_out.clear();
    mnemonic_passphrase_out.clear();

    if (m_mnemonics.empty() && m_crypted_mnemonics.empty()) {
        WalletLogPrintf("%s: Descriptor wallet has no mnemonic defined\n", __func__);
        return false;
    }
    if (m_storage.IsLocked(false)) return false;

    if (m_mnemonics.size() + m_crypted_mnemonics.size() > 1) {
        WalletLogPrintf("%s: ERROR: One descriptor has multiple mnemonics. Can't match it\n", __func__);
        return false;
    }
    if (m_storage.HasEncryptionKeys() && !m_storage.IsLocked(true)) {
        if (!m_crypted_mnemonics.empty() && m_map_crypted_keys.size() != 1) {
            WalletLogPrintf("%s: ERROR: can't choose encryption key for mnemonic out of %lld\n", __func__, m_map_crypted_keys.size());
            return false;
        }
        const CPubKey& pubkey = m_map_crypted_keys.begin()->second.first;
        const auto mnemonic = m_crypted_mnemonics.begin()->second;
        const std::vector<unsigned char>& crypted_mnemonic = mnemonic.first;
        const std::vector<unsigned char>& crypted_mnemonic_passphrase = mnemonic.second;

        SecureVector mnemonic_v;
        SecureVector mnemonic_passphrase_v;
        if (!m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
            return DecryptSecret(encryption_key, crypted_mnemonic, pubkey.GetHash(), mnemonic_v);
        })) {
            WalletLogPrintf("%s: ERROR: can't decrypt mnemonic pubkey %s crypted: %s\n", __func__, pubkey.GetHash().ToString(), HexStr(crypted_mnemonic));
            return false;
        }
        if (!crypted_mnemonic_passphrase.empty()) {
            if (!m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
                return DecryptSecret(encryption_key, crypted_mnemonic_passphrase, pubkey.GetHash(), mnemonic_passphrase_v);
            })) {
                WalletLogPrintf("%s: ERROR: can't decrypt mnemonic passphrase\n", __func__);
                return false;
            }
        }

        std::copy(mnemonic_v.begin(), mnemonic_v.end(), std::back_inserter(mnemonic_out));
        std::copy(mnemonic_passphrase_v.begin(), mnemonic_passphrase_v.end(), std::back_inserter(mnemonic_passphrase_out));

        return true;
    }
    if (m_mnemonics.empty()) return false;

    const auto mnemonic_it = m_mnemonics.begin();

    mnemonic_out = mnemonic_it->second.first;
    mnemonic_passphrase_out = mnemonic_it->second.second;

    return true;
}

void DescriptorScriptPubKeyMan::UpgradeDescriptorCache()
{
    LOCK(cs_desc_man);
    if (m_storage.IsLocked(false) || m_storage.IsWalletFlagSet(WALLET_FLAG_LAST_HARDENED_XPUB_CACHED)) {
        return;
    }

    // Skip if we have the last hardened xpub cache
    if (m_wallet_descriptor.cache.GetCachedLastHardenedExtPubKeys().size() > 0) {
        return;
    }

    // Expand the descriptor
    FlatSigningProvider provider;
    provider.keys = GetKeys();
    FlatSigningProvider out_keys;
    std::vector<CScript> scripts_temp;
    DescriptorCache temp_cache;
    if (!m_wallet_descriptor.descriptor->Expand(0, provider, scripts_temp, out_keys, &temp_cache)){
        throw std::runtime_error("Unable to expand descriptor");
    }

    // Cache the last hardened xpubs
    DescriptorCache diff = m_wallet_descriptor.cache.MergeAndDiff(temp_cache);
    if (!WalletBatch(m_storage.GetDatabase()).WriteDescriptorCacheItems(GetID(), diff)) {
        throw std::runtime_error(std::string(__func__) + ": writing cache items failed");
    }
}

void DescriptorScriptPubKeyMan::UpdateWalletDescriptor(WalletDescriptor& descriptor)
{
    LOCK(cs_desc_man);
    std::string error;
    if (!CanUpdateToWalletDescriptor(descriptor, error)) {
        throw std::runtime_error(std::string(__func__) + ": " + error);
    }

    m_map_pubkeys.clear();
    m_map_script_pub_keys.clear();
    m_max_cached_index = -1;
    m_wallet_descriptor = descriptor;
}

bool DescriptorScriptPubKeyMan::CanUpdateToWalletDescriptor(const WalletDescriptor& descriptor, std::string& error)
{
    LOCK(cs_desc_man);
    if (!HasWalletDescriptor(descriptor)) {
        error = "can only update matching descriptor";
        return false;
    }

    if (descriptor.range_start > m_wallet_descriptor.range_start ||
        descriptor.range_end < m_wallet_descriptor.range_end) {
        // Use inclusive range for error
        error = strprintf("new range must include current range = [%d,%d]",
                          m_wallet_descriptor.range_start,
                          m_wallet_descriptor.range_end - 1);
        return false;
    }

    return true;
}
} // namespace wallet
