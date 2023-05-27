// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_KEYMAN_H
#define NAVCOIN_BLSCT_KEYMAN_H

#include <blsct/arith/mcl/mcl.h>
#include <blsct/eip_2333/bls12_381_keygen.h>
#include <blsct/double_public_key.h>
#include <blsct/public_key.h>
#include <blsct/private_key.h>
#include <blsct/wallet/address.h>
#include <blsct/wallet/keyring.h>
#include <blsct/wallet/hdchain.h>
#include <logging.h>
#include <wallet/crypter.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/walletdb.h>

namespace blsct {
class Manager
{
protected:
    wallet::WalletStorage& m_storage;

public:
    explicit Manager(wallet::WalletStorage& storage) : m_storage(storage) {}
    virtual ~Manager() {};

    virtual bool SetupGeneration(bool force = false) { return false; }

    /* Returns true if HD is enabled */
    virtual bool IsHDEnabled() const { return false; }
};

class KeyMan : public Manager, public KeyRing {
private:
    blsct::HDChain m_hd_chain;
    std::unordered_map<CKeyID, blsct::HDChain, SaltedSipHasher> m_inactive_hd_chains;

    bool AddKeyPubKeyInner(const PrivateKey& key, const PublicKey &pubkey);
    bool AddCryptedKeyInner(const PublicKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);

    wallet::WalletBatch *encrypted_batch GUARDED_BY(cs_KeyStore) = nullptr;

    using CryptedKeyMap = std::map<CKeyID, std::pair<PublicKey, std::vector<unsigned char>>>;

    CryptedKeyMap mapCryptedKeys GUARDED_BY(cs_KeyStore);

    int64_t nTimeFirstKey GUARDED_BY(cs_KeyStore) = 0;

    bool fDecryptionThoroughlyChecked = true;
public:
    KeyMan(wallet::WalletStorage& storage) : Manager(storage), KeyRing() {}

    bool SetupGeneration(bool force = false) override;
    bool IsHDEnabled() const override;

    /* Returns true if the wallet can generate new keys */
    bool CanGenerateKeys() const;

    /* Generates a new HD seed (will not be activated) */
    PrivateKey GenerateNewSeed();

    /* Set the current HD seed (will reset the chain child index counters)
       Sets the seed's version based on the current wallet version (so the
       caller must ensure the current wallet version is correct before calling
       this function). */
    void SetHDSeed(const PrivateKey& key);

    //! Adds a key to the store, and saves it to disk.
    bool AddKeyPubKey(const PrivateKey& key, const PublicKey &pubkey) override;
    bool AddViewKey(const PrivateKey& key, const PublicKey &pubkey) override;
    bool AddSpendKey(const PublicKey &pubkey) override;

    //! Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKey(const PrivateKey& key, const PublicKey &pubkey);
    bool LoadViewKey(const PrivateKey& key, const PublicKey &pubkey);
    //! Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const PublicKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    //! Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
    bool LoadCryptedKey(const PublicKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret, bool checksum_valid);
    bool AddKeyPubKeyWithDB(wallet::WalletBatch& batch, const PrivateKey& secret, const PublicKey& pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    /* KeyRing overrides */
    bool HaveKey(const CKeyID &address) const override;
    bool GetKey(const CKeyID &address, PrivateKey& keyOut) const override;

    bool Encrypt(const wallet::CKeyingMaterial& master_key, wallet::WalletBatch* batch);
    bool CheckDecryptionKey(const wallet::CKeyingMaterial& master_key, bool accept_no_keys);

    SubAddress GetAddress(const SubAddressIdentifier& id = {0,0});

    /* Set the HD chain model (chain child index counters) and writes it to the database */
    void AddHDChain(const blsct::HDChain& chain);
    void LoadHDChain(const blsct::HDChain& chain);
    const blsct::HDChain& GetHDChain() const { return m_hd_chain; }
    void AddInactiveHDChain(const blsct::HDChain& chain);

    //! Load metadata (used by LoadWallet)
    void LoadKeyMetadata(const CKeyID& keyID, const wallet::CKeyMetadata &metadata);
    void UpdateTimeFirstKey(int64_t nCreateTime) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    bool DeleteRecords();
    bool DeleteKeys();

    /** Keypool has new keys */
    boost::signals2::signal<void ()> NotifyCanGetAddressesChanged;

    // Map from Key ID to key metadata.
    std::map<CKeyID, wallet::CKeyMetadata> mapKeyMetadata GUARDED_BY(cs_KeyStore);
};
}

#endif // NAVCOIN_BLSCT_KEYMAN_H
