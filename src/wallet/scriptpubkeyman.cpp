// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>
#include <chainparams.h>
#include <logging.h>
#include <script/descriptor.h>
#include <script/sign.h>
#include <shutdown.h>
#include <util/bip32.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/translation.h>
#include <wallet/scriptpubkeyman.h>

bool LegacyScriptPubKeyMan::GetNewDestination(CTxDestination& dest, std::string& error)
{
    LOCK(cs_KeyStore);
    error.clear();

    // Generate a new key that is added to wallet
    CPubKey new_key;
    if (!GetKeyFromPool(new_key, false)) {
        error = _("Error: Keypool ran out, please call keypoolrefill first").translated;
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
//! @param script              script to solve
//! @param sigversion          script type (top-level / redeemscript / witnessscript)
//! @param recurse_scripthash  whether to recurse into nested p2sh and p2wsh
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
        if (!keyPass && !accept_no_keys && cryptedHDChain.IsNull()) {
            return false;
        }

        if(!cryptedHDChain.IsNull()) {
            // try to decrypt seed and make sure it matches
            CHDChain hdChainTmp;
            if (!DecryptHDChain(master_key, hdChainTmp) || (cryptedHDChain.GetID() != hdChainTmp.GetSeedHash())) {
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
    encrypted_batch = nullptr;
    return true;
}

bool LegacyScriptPubKeyMan::GetReservedDestination(bool internal, CTxDestination& address, int64_t& index, CKeyPool& keypool)
{
    LOCK(cs_KeyStore);
    if (!CanGetAddresses(internal)) {
        return false;
    }

    if (!ReserveKeyFromKeyPool(index, keypool, internal)) {
        return false;
    }
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
    if (m_storage.IsLocked() || m_storage.IsWalletFlagSet(WALLET_FLAG_KEY_ORIGIN_METADATA) || !IsHDEnabled()) {
        return;
    }

    CHDChain hdChainCurrent;
    if (!GetHDChain(hdChainCurrent))
        throw std::runtime_error(std::string(__func__) + ": GetHDChain failed");
    if (!DecryptHDChain(m_storage.GetEncryptionKey(), hdChainCurrent))
        throw std::runtime_error(std::string(__func__) + ": DecryptHDChain failed");

    CExtKey masterKey;
    SecureVector vchSeed = hdChainCurrent.GetSeed();
    masterKey.SetSeed(vchSeed);
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

void LegacyScriptPubKeyMan::GenerateNewHDChain(const SecureString& secureMnemonic, const SecureString& secureMnemonicPassphrase)
{
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    CHDChain newHdChain;

    // NOTE: an empty mnemonic means "generate a new one for me"
    // NOTE: default mnemonic passphrase is an empty string
    if (!newHdChain.SetMnemonic(secureMnemonic, secureMnemonicPassphrase, true)) {
        throw std::runtime_error(std::string(__func__) + ": SetMnemonic failed");
    }

    // add default account
    newHdChain.AddAccount();
    newHdChain.Debug(__func__);

    if (!SetHDChainSingle(newHdChain, false)) {
        throw std::runtime_error(std::string(__func__) + ": SetHDChainSingle failed");
    }

    if (!NewKeyPool()) {
        throw std::runtime_error(std::string(__func__) + ": NewKeyPool failed");
    }
}

bool LegacyScriptPubKeyMan::SetHDChain(WalletBatch &batch, const CHDChain& chain, bool memonly)
{
    LOCK(cs_KeyStore);

    if (!SetHDChain(chain))
        return false;

    if (!memonly) {
        if (!batch.WriteHDChain(chain)) {
            throw std::runtime_error(std::string(__func__) + ": WriteHDChain failed");
        }

        m_storage.UnsetBlankWalletFlag(batch);
    }

    return true;
}

bool LegacyScriptPubKeyMan::SetCryptedHDChain(WalletBatch &batch, const CHDChain& chain, bool memonly)
{
    LOCK(cs_KeyStore);

    if (!SetCryptedHDChain(chain))
        return false;

    if (!memonly) {
        if (encrypted_batch) {
            if (!encrypted_batch->WriteCryptedHDChain(chain))
                throw std::runtime_error(std::string(__func__) + ": WriteCryptedHDChain failed");
        } else {
            if (!batch.WriteCryptedHDChain(chain))
                throw std::runtime_error(std::string(__func__) + ": WriteCryptedHDChain failed");
        }
        m_storage.UnsetBlankWalletFlag(batch);
    }

    return true;
}

bool LegacyScriptPubKeyMan::SetHDChainSingle(const CHDChain& chain, bool memonly)
{
    WalletBatch batch(m_storage.GetDatabase());
    return SetHDChain(batch, chain, memonly);
}

bool LegacyScriptPubKeyMan::SetCryptedHDChainSingle(const CHDChain& chain, bool memonly)
{
    WalletBatch batch(m_storage.GetDatabase());
    return SetCryptedHDChain(batch, chain, memonly);
}

bool LegacyScriptPubKeyMan::GetDecryptedHDChain(CHDChain& hdChainRet)
{
    LOCK(cs_KeyStore);

    CHDChain hdChainTmp;
    if (!GetHDChain(hdChainTmp)) {
        return false;
    }

    if (!DecryptHDChain(m_storage.GetEncryptionKey(), hdChainTmp))
        return false;

    // make sure seed matches this chain
    if (hdChainTmp.GetID() != hdChainTmp.GetSeedHash())
        return false;

    hdChainRet = hdChainTmp;

    return true;
}

bool LegacyScriptPubKeyMan::EncryptHDChain(const CKeyingMaterial& vMasterKeyIn, const CHDChain& chain)
{
    LOCK(cs_KeyStore);
    // should call EncryptKeys first
    if (!m_storage.HasEncryptionKeys())
        return false;

    if (!cryptedHDChain.IsNull())
        return true;

    if (cryptedHDChain.IsCrypted())
        return true;

    if (hdChain.IsNull() && !chain.IsNull()) {
        // Encrypting a new HDChain for an already encrypted non-HD wallet
        hdChain = chain;
    }

    // make sure seed matches this chain
    if (hdChain.GetID() != hdChain.GetSeedHash())
        return false;

    std::vector<unsigned char> vchCryptedSeed;
    if (!EncryptSecret(vMasterKeyIn, hdChain.GetSeed(), hdChain.GetID(), vchCryptedSeed))
        return false;

    hdChain.Debug(__func__);
    cryptedHDChain = hdChain;
    cryptedHDChain.SetCrypted(true);

    SecureVector vchSecureCryptedSeed(vchCryptedSeed.begin(), vchCryptedSeed.end());
    if (!cryptedHDChain.SetSeed(vchSecureCryptedSeed, false))
        return false;

    SecureVector vchMnemonic;
    SecureVector vchMnemonicPassphrase;

    // it's ok to have no mnemonic if wallet was initialized via hdseed
    if (hdChain.GetMnemonic(vchMnemonic, vchMnemonicPassphrase)) {
        std::vector<unsigned char> vchCryptedMnemonic;
        std::vector<unsigned char> vchCryptedMnemonicPassphrase;

        if (!vchMnemonic.empty() && !EncryptSecret(vMasterKeyIn, vchMnemonic, hdChain.GetID(), vchCryptedMnemonic))
            return false;
        if (!vchMnemonicPassphrase.empty() && !EncryptSecret(vMasterKeyIn, vchMnemonicPassphrase, hdChain.GetID(), vchCryptedMnemonicPassphrase))
            return false;

        SecureVector vchSecureCryptedMnemonic(vchCryptedMnemonic.begin(), vchCryptedMnemonic.end());
        SecureVector vchSecureCryptedMnemonicPassphrase(vchCryptedMnemonicPassphrase.begin(), vchCryptedMnemonicPassphrase.end());
        if (!cryptedHDChain.SetMnemonic(vchSecureCryptedMnemonic, vchSecureCryptedMnemonicPassphrase, false))
            return false;
    }

    if (!hdChain.SetNull())
        return false;

    return true;
}

bool LegacyScriptPubKeyMan::DecryptHDChain(const CKeyingMaterial& vMasterKeyIn, CHDChain& hdChainRet) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys())
        return true;

    if (cryptedHDChain.IsNull())
        return false;

    if (!cryptedHDChain.IsCrypted())
        return false;

    SecureVector vchSecureSeed;
    SecureVector vchSecureCryptedSeed = cryptedHDChain.GetSeed();
    std::vector<unsigned char> vchCryptedSeed(vchSecureCryptedSeed.begin(), vchSecureCryptedSeed.end());
    if (!DecryptSecret(vMasterKeyIn, vchCryptedSeed, cryptedHDChain.GetID(), vchSecureSeed))
        return false;

    hdChainRet = cryptedHDChain;
    if (!hdChainRet.SetSeed(vchSecureSeed, false))
        return false;

    // hash of decrypted seed must match chain id
    if (hdChainRet.GetSeedHash() != cryptedHDChain.GetID())
        return false;

    SecureVector vchSecureCryptedMnemonic;
    SecureVector vchSecureCryptedMnemonicPassphrase;

    // it's ok to have no mnemonic if wallet was initialized via hdseed
    if (cryptedHDChain.GetMnemonic(vchSecureCryptedMnemonic, vchSecureCryptedMnemonicPassphrase)) {
        SecureVector vchSecureMnemonic;
        SecureVector vchSecureMnemonicPassphrase;

        std::vector<unsigned char> vchCryptedMnemonic(vchSecureCryptedMnemonic.begin(), vchSecureCryptedMnemonic.end());
        std::vector<unsigned char> vchCryptedMnemonicPassphrase(vchSecureCryptedMnemonicPassphrase.begin(), vchSecureCryptedMnemonicPassphrase.end());

        if (!vchCryptedMnemonic.empty() && !DecryptSecret(vMasterKeyIn, vchCryptedMnemonic, cryptedHDChain.GetID(), vchSecureMnemonic))
            return false;
        if (!vchCryptedMnemonicPassphrase.empty() && !DecryptSecret(vMasterKeyIn, vchCryptedMnemonicPassphrase, cryptedHDChain.GetID(), vchSecureMnemonicPassphrase))
            return false;

        if (!hdChainRet.SetMnemonic(vchSecureMnemonic, vchSecureMnemonicPassphrase, false))
            return false;
    }

    hdChainRet.SetCrypted(false);
    hdChainRet.Debug(__func__);

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
    if (internal && m_storage.CanSupportFeature(FEATURE_HD)) {
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

int64_t LegacyScriptPubKeyMan::GetOldestKeyPoolTime() const
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

size_t LegacyScriptPubKeyMan::KeypoolCountInternalKeys() const
{
    LOCK(cs_KeyStore); // setInternalKeyPool
    return setInternalKeyPool.size();
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
        // If, given the stuff in sigdata, we could make a valid sigature, then we can provide for this script
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

bool LegacyScriptPubKeyMan::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, std::string>& input_errors) const
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

TransactionError LegacyScriptPubKeyMan::FillPSBT(PartiallySignedTransaction& psbtx, int sighash_type, bool sign, bool bip32derivs, int* n_signed) const
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
        if (sign && input.sighash_type > 0 && input.sighash_type != sighash_type) {
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
        SignatureData sigdata;
        input.FillSignatureData(sigdata);
        SignPSBTInput(HidingSigningProvider(this, !sign, !bip32derivs), psbtx, i, sighash_type);

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

const CKeyMetadata* LegacyScriptPubKeyMan::GetMetadata(const CTxDestination& dest) const
{
    LOCK(cs_KeyStore);

    CKeyID key_id = GetKeyForDestination(*this, dest);
    if (!key_id.IsNull()) {
        auto it = mapKeyMetadata.find(key_id);
        if (it != mapKeyMetadata.end()) {
            return &it->second;
        }
    }

    CScript scriptPubKey = GetScriptForDestination(dest);
    auto it = m_script_metadata.find(CScriptID(scriptPubKey));
    if (it != m_script_metadata.end()) {
        return &it->second;
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
    if (!EncryptSecret(m_storage.GetEncryptionKey(), vchSecret, pubkey.GetHash(), vchCryptedSecret)) {
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
        return DecryptKey(m_storage.GetEncryptionKey(), vchCryptedSecret, vchPubKey, keyOut);
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

bool LegacyScriptPubKeyMan::SetHDChain(const CHDChain& chain)
{
    LOCK(cs_KeyStore);
    if (m_storage.HasEncryptionKeys())
        return false;

    if (chain.IsCrypted())
        return false;

    hdChain = chain;
    return true;
}

bool LegacyScriptPubKeyMan::SetCryptedHDChain(const CHDChain& chain)
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys())
        return false;

    if (!chain.IsCrypted())
        return false;

    cryptedHDChain = chain;
    return true;
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
        if (!DecryptHDChain(m_storage.GetEncryptionKey(), hdChainCurrent))
            throw std::runtime_error(std::string(__func__) + ": DecryptHDChain failed");
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

bool LegacyScriptPubKeyMan::AddKeyOrigin(const CPubKey& pubkey, const KeyOriginInfo& info)
{
    LOCK(cs_KeyStore);
    std::copy(info.fingerprint, info.fingerprint + 4, mapKeyMetadata[pubkey.GetID()].key_origin.fingerprint);
    mapKeyMetadata[pubkey.GetID()].key_origin.path = info.path;
    mapKeyMetadata[pubkey.GetID()].has_key_origin = true;
    return WriteKeyMetadata(mapKeyMetadata[pubkey.GetID()], pubkey, true);
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

    if (!DecryptHDChain(m_storage.GetEncryptionKey(), hdChainTmp))
        throw std::runtime_error(std::string(__func__) + ": DecryptHDChain failed");
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

    if (m_storage.HasEncryptionKeys()) {
        if (!SetCryptedHDChain(batch, hdChainCurrent, false))
            throw std::runtime_error(std::string(__func__) + ": SetCryptedHDChain failed");
    }
    else {
        if (!SetHDChain(batch, hdChainCurrent, false))
            throw std::runtime_error(std::string(__func__) + ": SetHDChain failed");
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
            nTargetSize = std::max(gArgs.GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t) 0);

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
        const std::string strMsg = _("Topping up keypool...").translated;

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
    for (const auto& entry : key_origins) {
        AddKeyOrigin(entry.second.first, entry.second.second);
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
    if (m_storage.HasEncryptionKeys() && !cryptedHDChain.IsNull()) {
        hdChainRet = cryptedHDChain;
        return true;
    }

    hdChainRet = hdChain;
    return !hdChain.IsNull();
}
