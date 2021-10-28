// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <logging.h>
#include <pubkey.h>
#include <random.h>
#include <wallet/crypter.h>
#include <wallet/keyman.h>
#include <wallet/walletdb.h>

namespace wallet {
void KeyManager::GenerateAndSetHDKey()
{
    LOCK(cs_keyman);
    CKey key;
    key.MakeNewKey(true);
    CPubKey seed_pub = key.GetPubKey();
    assert(key.VerifyPubKey(seed_pub));

    CExtKey master_key;
    master_key.SetSeed(key);
    CExtPubKey master_xpub = master_key.Neuter();

    WalletBatch batch(m_storage.GetDatabase());
    AddHDKey(batch, master_key, master_xpub);
    SetActiveHDKey(master_xpub);
}

void KeyManager::LoadActiveHDKey(const CExtPubKey& extpub)
{
    LOCK(cs_keyman);
    m_active_xpub = extpub;
}

void KeyManager::SetActiveHDKey(const CExtPubKey& extpub)
{
    LOCK(cs_keyman);
    LoadActiveHDKey(extpub);
    WalletBatch batch(m_storage.GetDatabase());
    batch.WriteActiveHDKey(extpub);
}

std::optional<CExtKey> KeyManager::GetActiveHDKey() const
{
    if (!m_active_xpub.pubkey.IsValid()) {
        return std::nullopt;
    }

    CKey key;
    if (m_storage.HasEncryptionKeys()) {
        if (m_storage.IsLocked()) {
            return std::nullopt;
        }
        const auto& it = m_map_crypted_keys.find(m_active_xpub.pubkey.GetID());
        assert(it != m_map_crypted_keys.end());
        const auto& [pubkey, ckey] = it->second;

        if (!DecryptKey(m_storage.GetEncryptionKey(), ckey, pubkey, key)) {
            return std::nullopt;
        }
    } else {
        const auto& it = m_map_keys.find(m_active_xpub.pubkey.GetID());
        assert(it != m_map_keys.end());
        key = it->second;
    }
    assert(key.IsValid());

    CExtKey master_key;
    master_key.nDepth = m_active_xpub.nDepth;
    std::copy(m_active_xpub.vchFingerprint, m_active_xpub.vchFingerprint + sizeof(master_key.vchFingerprint), master_key.vchFingerprint);
    master_key.nChild = m_active_xpub.nChild;
    master_key.chaincode = m_active_xpub.chaincode;
    master_key.key = key;
    return master_key;
}

bool KeyManager::AddKeyInner(WalletBatch& batch, const CKey& key, const CPubKey& pubkey)
{
    AssertLockHeld(cs_keyman);
    assert(!m_storage.HasEncryptionKeys());

    const CKeyID& id = pubkey.GetID();
    if (m_map_keys.find(id) != m_map_keys.end()) {
        return true;
    }

    m_map_keys[id] = key;
    return batch.WriteKeyManKey(pubkey, key.GetPrivKey());
}

std::vector<unsigned char> KeyManager::AddCryptedKeyInner(WalletBatch& batch, const CKey& key, const CPubKey& pubkey)
{
    AssertLockHeld(cs_keyman);

    assert(m_storage.HasEncryptionKeys());
    if (m_storage.IsLocked()) {
        return {};
    }

    const CKeyID& id = pubkey.GetID();
    const auto& it = m_map_crypted_keys.find(id);
    if (it != m_map_crypted_keys.end()) {
        return it->second.second;
    }

    std::vector<unsigned char> crypted_secret;
    CKeyingMaterial secret(key.begin(), key.end());
    if (!EncryptSecret(m_storage.GetEncryptionKey(), secret, pubkey.GetHash(), crypted_secret)) {
        return {};
    }

    m_map_crypted_keys[id] = make_pair(pubkey, crypted_secret);

    if (!batch.WriteCryptedKeyManKey(pubkey, crypted_secret)) {
        return {};
    }

    return crypted_secret;
}

bool KeyManager::AddDescriptorKey(WalletBatch& batch, const uint256& desc_id, const CKey& key, const CPubKey& pubkey)
{
    LOCK(cs_keyman);
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));

    if (m_storage.HasEncryptionKeys()) {
        std::vector<unsigned char> ckey = AddCryptedKeyInner(batch, key, pubkey);
        if (ckey.empty()) {
            return false;
        }
        return batch.WriteCryptedDescriptorKey(desc_id, pubkey, ckey);
    } else {
        return AddKeyInner(batch, key, pubkey) && batch.WriteDescriptorKey(desc_id, pubkey, key.GetPrivKey());
    }
}

bool KeyManager::AddHDKey(WalletBatch& batch, const CExtKey& extkey, const CExtPubKey& extpub)
{
    LOCK(cs_keyman);
    const CKeyID& id = extpub.pubkey.GetID();
    m_map_xpubs[id] = extpub;
    batch.WriteHDPubKey(extpub);

    if (m_storage.HasEncryptionKeys()) {
        std::vector<unsigned char> ckey = AddCryptedKeyInner(batch, extkey.key, extpub.pubkey);
        return !ckey.empty();
    } else {
        return AddKeyInner(batch, extkey.key, extpub.pubkey);
    }
}

void KeyManager::LoadKey(const CKeyID& key_id, const CKey& key)
{
    LOCK(cs_keyman);
    m_map_keys[key_id] = key;
}

bool KeyManager::LoadCryptedKey(const CKeyID& key_id, const CPubKey& pubkey, const std::vector<unsigned char>& ckey)
{
    LOCK(cs_keyman);
    if (!m_map_keys.empty()) {
        return false;
    }

    m_map_crypted_keys[key_id] = make_pair(pubkey, ckey);
    return true;
}

void KeyManager::LoadHDKey(const CKeyID& key_id, const CExtPubKey& xpub)
{
    LOCK(cs_keyman);
    m_map_xpubs[key_id] = xpub;
}

bool KeyManager::CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys)
{
    LOCK(cs_keyman);
    if (!m_map_keys.empty()) {
        return false;
    }

    bool keyPass = m_map_crypted_keys.empty(); // Always pass when there are no encrypted keys
    bool keyFail = false;
    for (const auto& mi : m_map_crypted_keys) {
        const CPubKey& pubkey = mi.second.first;
        const std::vector<unsigned char>& crypted_secret = mi.second.second;
        CKey key;
        if (!DecryptKey(master_key, crypted_secret, pubkey, key)) {
            keyFail = true;
            break;
        }
        keyPass = true;
        if (m_decryption_thoroughly_checked) {
            break;
        }
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

bool KeyManager::Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch)
{
    LOCK(cs_keyman);

    // Nothing to encrypt
    if (m_map_keys.empty()) {
        return true;
    }

    if (!m_map_crypted_keys.empty()) {
        return false;
    }

    for (const auto& [id, key] : m_map_keys)
    {
        CPubKey pubkey = key.GetPubKey();
        CKeyingMaterial secret(key.begin(), key.end());
        std::vector<unsigned char> crypted_secret;
        if (!EncryptSecret(master_key, secret, pubkey.GetHash(), crypted_secret)) {
            return false;
        }
        m_map_crypted_keys[id] = make_pair(pubkey, crypted_secret);
        batch->WriteCryptedKeyManKey(pubkey, crypted_secret);
    }
    m_map_keys.clear();
    return true;
}

std::map<CKeyID, CKey> KeyManager::GetKeys() const
{
    AssertLockHeld(cs_keyman);
    if (m_storage.HasEncryptionKeys() && !m_storage.IsLocked()) {
        std::map<CKeyID, CKey> keys;
        for (const auto& [id, key_pair] : m_map_crypted_keys) {
            const auto& [pubkey, crypted_secret] = key_pair;
            CKey key;
            bool ok = DecryptKey(m_storage.GetEncryptionKey(), crypted_secret, pubkey, key);
            assert(ok);
            keys[id] = key;
        }
        return keys;
    }
    // If the wallet is encrypted and locked, then this will just be an empty map
    return m_map_keys;
}

bool KeyManager::HavePrivateKeys() const
{
    LOCK(cs_keyman);
    return !m_map_keys.empty() || !m_map_crypted_keys.empty();
}

std::optional<std::pair<CPubKey, std::vector<unsigned char>>> KeyManager::GetCryptedKey(const CKeyID& id) const
{
    AssertLockHeld(cs_keyman);
    if (m_map_crypted_keys.count(id) == 0) {
        return std::nullopt;
    }
    return m_map_crypted_keys.at(id);
}
} // namespace wallet
