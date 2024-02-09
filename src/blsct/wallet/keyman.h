// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_KEYMAN_H
#define NAVCOIN_BLSCT_KEYMAN_H

#include <blsct/double_public_key.h>
#include <blsct/eip_2333/bls12_381_keygen.h>
#include <blsct/private_key.h>
#include <blsct/public_key.h>
#include <blsct/range_proof/bulletproofs/amount_recovery_request.h>
#include <blsct/range_proof/bulletproofs/amount_recovery_result.h>
#include <blsct/range_proof/bulletproofs/range_proof_logic.h>
#include <blsct/wallet/address.h>
#include <blsct/wallet/hdchain.h>
#include <blsct/wallet/helpers.h>
#include <blsct/wallet/keyring.h>
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
    virtual ~Manager(){};

    virtual bool SetupGeneration(bool force = false) { return false; }

    /* Returns true if HD is enabled */
    virtual bool IsHDEnabled() const { return false; }
};

class KeyMan : public Manager, public KeyRing
{
private:
    blsct::HDChain m_hd_chain;
    std::unordered_map<CKeyID, blsct::HDChain, SaltedSipHasher> m_inactive_hd_chains;

    bool AddKeyPubKeyInner(const PrivateKey& key, const PublicKey& pubkey);
    bool AddCryptedKeyInner(const PublicKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret);

    wallet::WalletBatch* encrypted_batch GUARDED_BY(cs_KeyStore) = nullptr;

    using CryptedKeyMap = std::map<CKeyID, std::pair<PublicKey, std::vector<unsigned char>>>;
    using SubAddressMap = std::map<CKeyID, SubAddressIdentifier>;
    using SubAddressStrMap = std::map<SubAddress, CKeyID>;
    using SubAddressPoolMapSet = std::map<int64_t, std::set<uint64_t>>;

    CryptedKeyMap mapCryptedKeys GUARDED_BY(cs_KeyStore);
    SubAddressMap mapSubAddresses GUARDED_BY(cs_KeyStore);
    SubAddressStrMap mapSubAddressesStr GUARDED_BY(cs_KeyStore);
    SubAddressPoolMapSet setSubAddressPool GUARDED_BY(cs_KeyStore);
    SubAddressPoolMapSet setSubAddressReservePool GUARDED_BY(cs_KeyStore);

    int64_t nTimeFirstKey GUARDED_BY(cs_KeyStore) = 0;
    //! Number of pre-generated SubAddresses
    int64_t m_keypool_size GUARDED_BY(cs_KeyStore){wallet::DEFAULT_KEYPOOL_SIZE};

    bool fDecryptionThoroughlyChecked = true;

public:
    KeyMan(wallet::WalletStorage& storage, int64_t keypool_size)
        : Manager(storage), KeyRing(), m_keypool_size(keypool_size) {}

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
    bool AddKeyPubKey(const PrivateKey& key, const PublicKey& pubkey) override;
    bool AddViewKey(const PrivateKey& key, const PublicKey& pubkey) override;
    bool AddSpendKey(const PublicKey& pubkey) override;

    //! Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKey(const PrivateKey& key, const PublicKey& pubkey);
    bool LoadViewKey(const PrivateKey& key, const PublicKey& pubkey);
    bool LoadSpendKey(const PublicKey& pubkey);
    //! Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const PublicKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret);
    //! Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
    bool LoadCryptedKey(const PublicKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, bool checksum_valid);
    bool AddKeyPubKeyWithDB(wallet::WalletBatch& batch, const PrivateKey& secret, const PublicKey& pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool AddSubAddressPoolWithDB(wallet::WalletBatch& batch, const SubAddressIdentifier& id, const SubAddress& subAddress, const bool& fLock = true);
    bool AddSubAddressPoolInner(const SubAddressIdentifier& id, const bool& fLock = true);

    /* KeyRing overrides */
    bool HaveKey(const CKeyID& address) const override;
    bool GetKey(const CKeyID& address, PrivateKey& keyOut) const override;

    bool Encrypt(const wallet::CKeyingMaterial& master_key, wallet::WalletBatch* batch);
    bool CheckDecryptionKey(const wallet::CKeyingMaterial& master_key, bool accept_no_keys);

    SubAddress GenerateNewSubAddress(const int64_t& account, SubAddressIdentifier& id);
    SubAddress GetSubAddress(const SubAddressIdentifier& id = {0, 0}) const;
    util::Result<CTxDestination> GetNewDestination(const int64_t& account = 0);

    /* Set the HD chain model (chain child index counters) and writes it to the database */
    void AddHDChain(const blsct::HDChain& chain);
    void LoadHDChain(const blsct::HDChain& chain);
    const blsct::HDChain& GetHDChain() const { return m_hd_chain; }
    void AddInactiveHDChain(const blsct::HDChain& chain);

    //! Load metadata (used by LoadWallet)
    void LoadKeyMetadata(const CKeyID& keyID, const wallet::CKeyMetadata& metadata);
    void UpdateTimeFirstKey(int64_t nCreateTime) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    bool DeleteRecords();
    bool DeleteKeys();

    /** Detect ownership of outputs **/
    bool IsMine(const CTxOut& txout) { return IsMine(txout.blsctData.blindingKey, txout.blsctData.spendingKey, txout.blsctData.viewTag); };
    bool IsMine(const blsct::PublicKey& blindingKey, const blsct::PublicKey& spendingKey, const uint16_t& viewTag);
    CKeyID GetHashId(const CTxOut& txout) const { return GetHashId(txout.blsctData.blindingKey, txout.blsctData.spendingKey); }
    CKeyID GetHashId(const blsct::PublicKey& blindingKey, const blsct::PublicKey& spendingKey) const;
    CTxDestination GetDestination(const CTxOut& txout) const;
    blsct::PrivateKey GetMasterSeedKey() const;
    blsct::PrivateKey GetSpendingKey() const;
    blsct::PrivateKey GetSpendingKeyForOutput(const CTxOut& out) const;
    blsct::PrivateKey GetSpendingKeyForOutput(const CTxOut& out, const CKeyID& id) const;
    blsct::PrivateKey GetSpendingKeyForOutput(const CTxOut& out, const SubAddressIdentifier& id) const;
    bulletproofs::AmountRecoveryResult<Mcl> RecoverOutputs(const std::vector<CTxOut>& outs);

    /** SubAddress keypool */
    void LoadSubAddress(const CKeyID& hashId, const SubAddressIdentifier& index);
    bool AddSubAddress(const CKeyID& hashId, const SubAddressIdentifier& index);
    bool HaveSubAddress(const CKeyID& hashId) const EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool GetSubAddress(const CKeyID& hashId, SubAddress& address) const;
    bool GetSubAddressId(const CKeyID& hashId, SubAddressIdentifier& subAddId) const;
    void LoadSubAddressStr(const SubAddress& subAddress, const CKeyID& hashId);
    bool AddSubAddressStr(const SubAddress& subAddress, const CKeyID& hashId);
    bool HaveSubAddressStr(const SubAddress& subAddress) const;
    bool NewSubAddressPool(const int64_t& account = 0);
    bool TopUp(const unsigned int& size = 0);
    bool TopUpAccount(const int64_t& account, const unsigned int& size = 0);
    void ReserveSubAddressFromPool(const int64_t& account, int64_t& nIndex, SubAddressPool& keypool);
    void KeepSubAddress(const SubAddressIdentifier& id);
    void ReturnSubAddress(const SubAddressIdentifier& id);
    bool GetSubAddressFromPool(const int64_t& account, CKeyID& result, SubAddressIdentifier& id);
    int64_t GetOldestSubAddressPoolTime(const int64_t& account);
    int GetSubAddressPoolSize(const int64_t& account) const;

    bool OutputIsChange(const CTxOut& out) const;

    /** Keypool has new keys */
    boost::signals2::signal<void()>
        NotifyCanGetAddressesChanged;

    // Map from Key ID to key metadata.
    std::map<CKeyID, wallet::CKeyMetadata> mapKeyMetadata GUARDED_BY(cs_KeyStore);

    /** Prepends the wallet name in logging output to ease debugging in multi-wallet use cases */
    template <typename... Params>
    void WalletLogPrintf(std::string fmt, Params... parameters) const
    {
        LogPrintf(("%s " + fmt).c_str(), m_storage.GetDisplayName(), parameters...);
    };
};
} // namespace blsct

#endif // NAVCOIN_BLSCT_KEYMAN_H
