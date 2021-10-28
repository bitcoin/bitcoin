// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_KEYMAN_H
#define BITCOIN_WALLET_KEYMAN_H

#include <sync.h>
#include <wallet/db.h>
#include <wallet/storage.h>

#include <map>
#include <optional>
#include <set>
#include <vector>

class CKey;
class CKeyID;
class CPubKey;

namespace wallet {
class WalletBatch;

class KeyManager
{
private:
    WalletStorage& m_storage;

    std::map<CKeyID, CKey> m_map_keys GUARDED_BY(cs_keyman);
    std::map<CKeyID, CExtPubKey> m_map_xpubs GUARDED_BY(cs_keyman);
    std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char>>> m_map_crypted_keys GUARDED_BY(cs_keyman);

    bool m_decryption_thoroughly_checked = false;

    CExtPubKey m_active_xpub GUARDED_BY(cs_keyman);

    bool AddKeyInner(WalletBatch& batch, const CKey& key, const CPubKey& pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_keyman);
    std::vector<unsigned char> AddCryptedKeyInner(WalletBatch& batch, const CKey& key, const CPubKey& pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_keyman);

public:
    mutable RecursiveMutex cs_keyman;

    KeyManager(WalletStorage& storage) : m_storage(storage) {}
    KeyManager() = delete;

    void GenerateAndSetHDKey();
    void SetActiveHDKey(const CExtPubKey& extpub);

    bool AddDescriptorKey(WalletBatch& batch, const uint256& desc_id, const CKey& key, const CPubKey& pubkey);
    bool AddHDKey(WalletBatch& batch, const CExtKey& extkey, const CExtPubKey& extpub);

    std::optional<CExtKey> GetActiveHDKey() const EXCLUSIVE_LOCKS_REQUIRED(cs_keyman);
    std::map<CKeyID, CKey> GetKeys() const EXCLUSIVE_LOCKS_REQUIRED(cs_keyman);
    std::optional<std::pair<CPubKey, std::vector<unsigned char>>> GetCryptedKey(const CKeyID& id) const EXCLUSIVE_LOCKS_REQUIRED(cs_keyman);

    void LoadKey(const CKeyID&, const CKey& key);
    bool LoadCryptedKey(const CKeyID&, const CPubKey& pubkey, const std::vector<unsigned char>& ckey);
    void LoadHDKey(const CKeyID& key_id, const CExtPubKey& xpub);
    void LoadActiveHDKey(const CExtPubKey& extpub);

    bool CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys);
    bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch);
    bool HavePrivateKeys() const;
};
} // namespace wallet

#endif // BITCOIN_WALLET_KEYMAN_H
