// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/keyman.h>

namespace blsct {
bool KeyMan::IsHDEnabled() const
{
    return !m_hd_chain.seed_id.IsNull();
}

bool KeyMan::CanGenerateKeys() const
{
    // A wallet can generate keys if it has an HD seed (IsHDEnabled) or it is a non-HD wallet (pre FEATURE_HD)
    LOCK(cs_KeyStore);
    return IsHDEnabled();
}

bool KeyMan::AddKeyPubKeyInner(const PrivateKey& key, const PublicKey &pubkey)
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return KeyRing::AddKeyPubKey(key, pubkey);
    }

    if (m_storage.IsLocked()) {
        return false;
    }

    std::vector<unsigned char> vchCryptedSecret;
    auto keyVch = key.GetScalar().GetVch();
    wallet::CKeyingMaterial vchSecret(keyVch.begin(), keyVch.end());
    if (!wallet::EncryptSecret(m_storage.GetEncryptionKey(), vchSecret, pubkey.GetHash(), vchCryptedSecret)) {
        return false;
    }

    if (!AddCryptedKey(pubkey, vchCryptedSecret)) {
        return false;
    }
    return true;
}

bool KeyMan::AddKeyPubKey(const PrivateKey& secret, const PublicKey &pubkey)
{
    LOCK(cs_KeyStore);
    wallet::WalletBatch batch(m_storage.GetDatabase());
    return KeyMan::AddKeyPubKeyWithDB(batch, secret, pubkey);
}

bool KeyMan::AddViewKey(const PrivateKey& secret, const PublicKey& pubkey)
{
    LOCK(cs_KeyStore);
    wallet::WalletBatch batch(m_storage.GetDatabase());
    AssertLockHeld(cs_KeyStore);

    if (!fViewKeyDefined) {
        KeyRing::AddViewKey(secret, pubkey);

        return batch.WriteViewKey(pubkey, secret,
                                  mapKeyMetadata[pubkey.GetID()]);
    }
    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}

bool KeyMan::AddSpendKey(const PublicKey& pubkey)
{
    LOCK(cs_KeyStore);
    wallet::WalletBatch batch(m_storage.GetDatabase());
    AssertLockHeld(cs_KeyStore);

    if (!fSpendKeyDefined) {
        KeyRing::AddSpendKey(pubkey);

        if (!batch.WriteSpendKey(pubkey))
            return false;
    }

    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}

bool KeyMan::AddKeyPubKeyWithDB(wallet::WalletBatch& batch, const PrivateKey& secret, const PublicKey& pubkey)
{
    AssertLockHeld(cs_KeyStore);

    // Make sure we aren't adding private keys to private key disabled wallets
    assert(!m_storage.IsWalletFlagSet(wallet::WALLET_FLAG_DISABLE_PRIVATE_KEYS));

    bool needsDB = !encrypted_batch;
    if (needsDB) {
        encrypted_batch = &batch;
    }
    if (!AddKeyPubKeyInner(secret, pubkey)) {
        if (needsDB) encrypted_batch = nullptr;
        return false;
    }
    if (needsDB) encrypted_batch = nullptr;

    if (!m_storage.HasEncryptionKeys()) {
        return batch.WriteKey(pubkey,
                                 secret,
                                 mapKeyMetadata[pubkey.GetID()]);
    }
    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}

bool KeyMan::LoadCryptedKey(const PublicKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret, bool checksum_valid)
{
    // Set fDecryptionThoroughlyChecked to false when the checksum is invalid
    if (!checksum_valid) {
        fDecryptionThoroughlyChecked = false;
    }

    return AddCryptedKeyInner(vchPubKey, vchCryptedSecret);
}

bool KeyMan::AddCryptedKeyInner(const PublicKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    LOCK(cs_KeyStore);
    for (const KeyMap::value_type& mKey : mapKeys)
    {
        const PrivateKey &key = mKey.second;
        PublicKey pubKey = key.GetPublicKey();
    }
    assert(mapKeys.empty());

    mapCryptedKeys[vchPubKey.GetID()] = make_pair(vchPubKey, vchCryptedSecret);
    return true;
}

bool KeyMan::AddCryptedKey(const PublicKey &vchPubKey,
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
            return wallet::WalletBatch(m_storage.GetDatabase()).WriteCryptedKey(vchPubKey,
                                                            vchCryptedSecret,
                                                            mapKeyMetadata[vchPubKey.GetID()]);
    }
}

PrivateKey KeyMan::GenerateNewSeed()
{
    assert(!m_storage.IsWalletFlagSet(wallet::WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    PrivateKey key(BLS12_381_KeyGen::derive_master_SK(MclScalar::Rand(true).GetVch()));
    return key;
}

void KeyMan::LoadHDChain(const blsct::HDChain& chain)
{
    LOCK(cs_KeyStore);
    m_hd_chain = chain;
}

void KeyMan::AddHDChain(const blsct::HDChain& chain)
{
    LOCK(cs_KeyStore);
    // Store the new chain
    if (!wallet::WalletBatch(m_storage.GetDatabase()).WriteBLSCTHDChain(chain)) {
        throw std::runtime_error(std::string(__func__) + ": writing chain failed");
    }
    // When there's an old chain, add it as an inactive chain as we are now rotating hd chains
    if (!m_hd_chain.seed_id.IsNull()) {
        AddInactiveHDChain(m_hd_chain);
    }

    m_hd_chain = chain;
}

void KeyMan::AddInactiveHDChain(const blsct::HDChain& chain)
{
    LOCK(cs_KeyStore);
    assert(!chain.seed_id.IsNull());
    m_inactive_hd_chains[chain.seed_id] = chain;
}


void KeyMan::SetHDSeed(const PrivateKey& key)
{
    LOCK(cs_KeyStore);
    // store the keyid (hash160) together with
    // the child index counter in the database
    // as a hdchain object
    blsct::HDChain newHdChain;

    auto seed = key.GetPublicKey();
    auto scalarMasterKey = key.GetScalar();
    auto childKey = BLS12_381_KeyGen::derive_child_SK(scalarMasterKey, 130);
    auto transactionKey = BLS12_381_KeyGen::derive_child_SK(childKey, 0);
    //auto blindingKey = BLS12_381_KeyGen::derive_child_SK(childKey, 1);
    auto tokenKey = PrivateKey(BLS12_381_KeyGen::derive_child_SK(childKey, 2));
    auto viewKey = PrivateKey(BLS12_381_KeyGen::derive_child_SK(transactionKey, 0));
    auto spendKey = PrivateKey(BLS12_381_KeyGen::derive_child_SK(transactionKey, 1));

    newHdChain.nVersion = blsct::HDChain::VERSION_HD_BASE;
    newHdChain.seed_id = key.GetPublicKey().GetID();
    newHdChain.spend_id = spendKey.GetPublicKey().GetID();
    newHdChain.view_id = viewKey.GetPublicKey().GetID();
    newHdChain.token_id = tokenKey.GetPublicKey().GetID();

    int64_t nCreationTime = GetTime();

    wallet::CKeyMetadata spendMetadata(nCreationTime);
    wallet::CKeyMetadata viewMetadata(nCreationTime);
    wallet::CKeyMetadata tokenMetadata(nCreationTime);

    spendMetadata.hdKeypath      = "spend";
    spendMetadata.has_key_origin = false;
    spendMetadata.hd_seed_id = newHdChain.spend_id;

    viewMetadata.hdKeypath       = "view";
    viewMetadata.has_key_origin  = false;
    viewMetadata.hd_seed_id = newHdChain.view_id;

    tokenMetadata.hdKeypath      = "token";
    tokenMetadata.has_key_origin = false;
    tokenMetadata.hd_seed_id = newHdChain.token_id;

    // mem store the metadata
    mapKeyMetadata[newHdChain.spend_id] = spendMetadata;
    mapKeyMetadata[newHdChain.view_id] = viewMetadata;
    mapKeyMetadata[newHdChain.token_id] = tokenMetadata;

    // write the keys to the database
    if (!AddKeyPubKey(key, seed))
        throw std::runtime_error(std::string(__func__) + ": AddKeyPubKey failed");

    if (!AddKeyPubKey(spendKey, spendKey.GetPublicKey()))
        throw std::runtime_error(std::string(__func__) + ": AddKeyPubKey failed");

    if (!AddSpendKey(spendKey.GetPublicKey()))
        throw std::runtime_error(std::string(__func__) + ": AddSpendKey failed");

    if (!AddViewKey(viewKey, viewKey.GetPublicKey()))
        throw std::runtime_error(std::string(__func__) + ": AddKeyPubKey failed");

    if (!AddKeyPubKey(tokenKey, tokenKey.GetPublicKey()))
        throw std::runtime_error(std::string(__func__) + ": AddKeyPubKey failed");

    AddHDChain(newHdChain);
    NotifyCanGetAddressesChanged();
    wallet::WalletBatch batch(m_storage.GetDatabase());
    m_storage.UnsetBlankWalletFlag(batch);
}

bool KeyMan::SetupGeneration(bool force)
{
    if ((CanGenerateKeys() && !force) || m_storage.IsLocked()) {
        return false;
    }

    SetHDSeed(GenerateNewSeed());
    /*if (!NewKeyPool()) {
        return false;
    }*/
    return true;
}

bool KeyMan::CheckDecryptionKey(const wallet::CKeyingMaterial& master_key, bool accept_no_keys)
{
    {
        LOCK(cs_KeyStore);
        assert(mapKeys.empty());

        bool keyPass = mapCryptedKeys.empty(); // Always pass when there are no encrypted keys
        bool keyFail = false;
        CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
        wallet::WalletBatch batch(m_storage.GetDatabase());
        for (; mi != mapCryptedKeys.end(); ++mi)
        {
            const PublicKey &vchPubKey = (*mi).second.first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
            PrivateKey key;
            if (!wallet::DecryptKey(master_key, vchCryptedSecret, vchPubKey, key))
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
        if (keyFail || (!keyPass && !accept_no_keys))
            return false;
        fDecryptionThoroughlyChecked = true;
    }
    return true;
}

void KeyMan::LoadKeyMetadata(const CKeyID& keyID, const wallet::CKeyMetadata& meta)
{
    LOCK(cs_KeyStore);
    UpdateTimeFirstKey(meta.nCreateTime);
    mapKeyMetadata[keyID] = meta;
}

bool KeyMan::LoadKey(const PrivateKey& key, const PublicKey &pubkey)
{
    return AddKeyPubKeyInner(key, pubkey);
}

bool KeyMan::LoadViewKey(const PrivateKey& key, const PublicKey &pubkey)
{
    return KeyRing::AddViewKey(key, pubkey);
}

/**
 * Update wallet first key creation time. This should be called whenever keys
 * are added to the wallet, with the oldest key creation time.
 */
void KeyMan::UpdateTimeFirstKey(int64_t nCreateTime)
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

SubAddress KeyMan::GetAddress(const SubAddressIdentifier& id)
{
    return SubAddress(viewKey, spendPublicKey, id);
};

bool KeyMan::HaveKey(const CKeyID &id) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return KeyRing::HaveKey(id);
    }
    return mapCryptedKeys.count(id) > 0;
}

bool KeyMan::GetKey(const CKeyID &id, PrivateKey& keyOut) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return KeyRing::GetKey(id, keyOut);
    }

    CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(id);
    if (mi != mapCryptedKeys.end())
    {
        const PublicKey &vchPubKey = (*mi).second.first;
        const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
        return wallet::DecryptKey(m_storage.GetEncryptionKey(), vchCryptedSecret, vchPubKey, keyOut);
    }
    return false;
}

bool KeyMan::DeleteRecords()
{
    LOCK(cs_KeyStore);
    wallet::WalletBatch batch(m_storage.GetDatabase());
    return batch.EraseRecords(wallet::DBKeys::BLSCT_TYPES);
}

bool KeyMan::DeleteKeys()
{
    LOCK(cs_KeyStore);
    wallet::WalletBatch batch(m_storage.GetDatabase());
    return batch.EraseRecords(wallet::DBKeys::BLSCTKEY_TYPES);
}

bool KeyMan::Encrypt(const wallet::CKeyingMaterial& master_key, wallet::WalletBatch* batch)
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
        const PrivateKey &key = mKey.second;
        PublicKey pubKey = key.GetPublicKey();
        wallet::CKeyingMaterial vchSecret(key.begin(), key.end());
        std::vector<unsigned char> vchCryptedSecret;
        if (!wallet::EncryptSecret(master_key, vchSecret, pubKey.GetHash(), vchCryptedSecret)) {
            encrypted_batch = nullptr;
            return false;
        }
        if (!AddCryptedKey(pubKey, vchCryptedSecret)) {
            encrypted_batch = nullptr;
            return false;
        }
    }
    encrypted_batch = nullptr;
    return true;
}
}
