// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SCRIPTPUBKEYMAN_H
#define BITCOIN_WALLET_SCRIPTPUBKEYMAN_H

#include <addresstype.h>
#include <common/messages.h>
#include <common/signmessage.h>
#include <common/types.h>
#include <logging.h>
#include <node/types.h>
#include <psbt.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <util/result.h>
#include <util/time.h>
#include <wallet/crypter.h>
#include <wallet/types.h>
#include <wallet/walletdb.h>
#include <wallet/walletutil.h>

#include <boost/signals2/signal.hpp>

#include <functional>
#include <optional>
#include <unordered_map>

enum class OutputType;

namespace wallet {
struct MigrationData;
class ScriptPubKeyMan;

// Wallet storage things that ScriptPubKeyMans need in order to be able to store things to the wallet database.
// It provides access to things that are part of the entire wallet and not specific to a ScriptPubKeyMan such as
// wallet flags, wallet version, encryption keys, encryption status, and the database itself. This allows a
// ScriptPubKeyMan to have callbacks into CWallet without causing a circular dependency.
// WalletStorage should be the same for all ScriptPubKeyMans of a wallet.
class WalletStorage
{
public:
    virtual ~WalletStorage() = default;
    virtual std::string GetDisplayName() const = 0;
    virtual WalletDatabase& GetDatabase() const = 0;
    virtual bool IsWalletFlagSet(uint64_t) const = 0;
    virtual void UnsetBlankWalletFlag(WalletBatch&) = 0;
    virtual bool CanSupportFeature(enum WalletFeature) const = 0;
    virtual void SetMinVersion(enum WalletFeature, WalletBatch* = nullptr) = 0;
    //! Pass the encryption key to cb().
    virtual bool WithEncryptionKey(std::function<bool (const CKeyingMaterial&)> cb) const = 0;
    virtual bool HasEncryptionKeys() const = 0;
    virtual bool IsLocked() const = 0;
    //! Callback function for after TopUp completes containing any scripts that were added by a SPKMan
    virtual void TopUpCallback(const std::set<CScript>&, ScriptPubKeyMan*) = 0;
};

//! Constant representing an unknown spkm creation time
static constexpr int64_t UNKNOWN_TIME = std::numeric_limits<int64_t>::max();

//! Default for -keypool
static const unsigned int DEFAULT_KEYPOOL_SIZE = 1000;

std::vector<CKeyID> GetAffectedKeys(const CScript& spk, const SigningProvider& provider);

struct WalletDestination
{
    CTxDestination dest;
    std::optional<bool> internal;
};

/*
 * A class implementing ScriptPubKeyMan manages some (or all) scriptPubKeys used in a wallet.
 * It contains the scripts and keys related to the scriptPubKeys it manages.
 * A ScriptPubKeyMan will be able to give out scriptPubKeys to be used, as well as marking
 * when a scriptPubKey has been used. It also handles when and how to store a scriptPubKey
 * and its related scripts and keys, including encryption.
 */
class ScriptPubKeyMan
{
protected:
    WalletStorage& m_storage;

public:
    explicit ScriptPubKeyMan(WalletStorage& storage) : m_storage(storage) {}
    virtual ~ScriptPubKeyMan() = default;
    virtual util::Result<CTxDestination> GetNewDestination(const OutputType type) { return util::Error{Untranslated("Not supported")}; }
    virtual isminetype IsMine(const CScript& script) const { return ISMINE_NO; }

    //! Check that the given decryption key is valid for this ScriptPubKeyMan, i.e. it decrypts all of the keys handled by it.
    virtual bool CheckDecryptionKey(const CKeyingMaterial& master_key) { return false; }
    virtual bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch) { return false; }

    virtual util::Result<CTxDestination> GetReservedDestination(const OutputType type, bool internal, int64_t& index) { return util::Error{Untranslated("Not supported")}; }
    virtual void KeepDestination(int64_t index, const OutputType& type) {}
    virtual void ReturnDestination(int64_t index, bool internal, const CTxDestination& addr) {}

    /** Fills internal address pool. Use within ScriptPubKeyMan implementations should be used sparingly and only
      * when something from the address pool is removed, excluding GetNewDestination and GetReservedDestination.
      * External wallet code is primarily responsible for topping up prior to fetching new addresses
      */
    virtual bool TopUp(unsigned int size = 0) { return false; }

    /** Mark unused addresses as being used
     * Affects all keys up to and including the one determined by provided script.
     *
     * @param script determines the last key to mark as used
     *
     * @return All of the addresses affected
     */
    virtual std::vector<WalletDestination> MarkUnusedAddresses(const CScript& script) { return {}; }

    /* Returns true if HD is enabled */
    virtual bool IsHDEnabled() const { return false; }

    /* Returns true if the wallet can give out new addresses. This means it has keys in the keypool or can generate new keys */
    virtual bool CanGetAddresses(bool internal = false) const { return false; }

    virtual bool HavePrivateKeys() const { return false; }
    virtual bool HaveCryptedKeys() const { return false; }

    //! The action to do when the DB needs rewrite
    virtual void RewriteDB() {}

    virtual unsigned int GetKeyPoolSize() const { return 0; }

    virtual int64_t GetTimeFirstKey() const { return 0; }

    virtual std::unique_ptr<CKeyMetadata> GetMetadata(const CTxDestination& dest) const { return nullptr; }

    virtual std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const { return nullptr; }

    /** Whether this ScriptPubKeyMan can provide a SigningProvider (via GetSolvingProvider) that, combined with
      * sigdata, can produce solving data.
      */
    virtual bool CanProvide(const CScript& script, SignatureData& sigdata) { return false; }

    /** Creates new signatures and adds them to the transaction. Returns whether all inputs were signed */
    virtual bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const { return false; }
    /** Sign a message with the given script */
    virtual SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const { return SigningResult::SIGNING_FAILED; };
    /** Adds script and derivation path information to a PSBT, and optionally signs it. */
    virtual std::optional<common::PSBTError> FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, std::optional<int> sighash_type = std::nullopt, bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const { return common::PSBTError::UNSUPPORTED; }

    virtual uint256 GetID() const { return uint256(); }

    /** Returns a set of all the scriptPubKeys that this ScriptPubKeyMan watches */
    virtual std::unordered_set<CScript, SaltedSipHasher> GetScriptPubKeys() const { return {}; };

    /** Prepends the wallet name in logging output to ease debugging in multi-wallet use cases */
    template <typename... Params>
    void WalletLogPrintf(util::ConstevalFormatString<sizeof...(Params)> wallet_fmt, const Params&... params) const
    {
        LogInfo("%s %s", m_storage.GetDisplayName(), tfm::format(wallet_fmt, params...));
    };

    /** Keypool has new keys */
    boost::signals2::signal<void ()> NotifyCanGetAddressesChanged;

    /** Birth time changed */
    boost::signals2::signal<void (const ScriptPubKeyMan* spkm, int64_t new_birth_time)> NotifyFirstKeyTimeChanged;
};

/** OutputTypes supported by the LegacyScriptPubKeyMan */
static const std::unordered_set<OutputType> LEGACY_OUTPUT_TYPES {
    OutputType::LEGACY,
    OutputType::P2SH_SEGWIT,
    OutputType::BECH32,
};

// Manages the data for a LegacyScriptPubKeyMan.
// This is the minimum necessary to load a legacy wallet so that it can be migrated.
class LegacyDataSPKM : public ScriptPubKeyMan, public FillableSigningProvider
{
private:
    using WatchOnlySet = std::set<CScript>;
    using WatchKeyMap = std::map<CKeyID, CPubKey>;
    using CryptedKeyMap = std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char>>>;

    CryptedKeyMap mapCryptedKeys GUARDED_BY(cs_KeyStore);
    WatchOnlySet setWatchOnly GUARDED_BY(cs_KeyStore);
    WatchKeyMap mapWatchKeys GUARDED_BY(cs_KeyStore);

    /* the HD chain data model (external chain counters) */
    CHDChain m_hd_chain;
    std::unordered_map<CKeyID, CHDChain, SaltedSipHasher> m_inactive_hd_chains;

    //! keeps track of whether Unlock has run a thorough check before
    bool fDecryptionThoroughlyChecked = true;

    bool AddWatchOnlyInMem(const CScript &dest);
    virtual bool AddKeyPubKeyInner(const CKey& key, const CPubKey &pubkey);
    bool AddCryptedKeyInner(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);

    // Helper function to retrieve a conservative superset of all output scripts that may be relevant to this LegacyDataSPKM.
    // It may include scripts that are invalid or not actually watched by this LegacyDataSPKM.
    // Used only in migration.
    std::unordered_set<CScript, SaltedSipHasher> GetCandidateScriptPubKeys() const;

    isminetype IsMine(const CScript& script) const override;
    bool CanProvide(const CScript& script, SignatureData& sigdata) override;
public:
    using ScriptPubKeyMan::ScriptPubKeyMan;

    // Map from Key ID to key metadata.
    std::map<CKeyID, CKeyMetadata> mapKeyMetadata GUARDED_BY(cs_KeyStore);

    // Map from Script ID to key metadata (for watch-only keys).
    std::map<CScriptID, CKeyMetadata> m_script_metadata GUARDED_BY(cs_KeyStore);

    // ScriptPubKeyMan overrides
    bool CheckDecryptionKey(const CKeyingMaterial& master_key) override;
    std::unordered_set<CScript, SaltedSipHasher> GetScriptPubKeys() const override;
    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const override;
    uint256 GetID() const override { return uint256::ONE; }

    // FillableSigningProvider overrides
    bool HaveKey(const CKeyID &address) const override;
    bool GetKey(const CKeyID &address, CKey& keyOut) const override;
    bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;

    //! Load metadata (used by LoadWallet)
    virtual void LoadKeyMetadata(const CKeyID& keyID, const CKeyMetadata &metadata);
    virtual void LoadScriptMetadata(const CScriptID& script_id, const CKeyMetadata &metadata);

    //! Adds a watch-only address to the store, without saving it to disk (used by LoadWallet)
    bool LoadWatchOnly(const CScript &dest);
    //! Returns whether the watch-only script is in the wallet
    bool HaveWatchOnly(const CScript &dest) const;
    //! Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKey(const CKey& key, const CPubKey &pubkey);
    //! Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
    bool LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret, bool checksum_valid);
    //! Adds a CScript to the store
    bool LoadCScript(const CScript& redeemScript);
    //! Load a HD chain model (used by LoadWallet)
    void LoadHDChain(const CHDChain& chain);
    void AddInactiveHDChain(const CHDChain& chain);
    const CHDChain& GetHDChain() const { return m_hd_chain; }

    //! Fetches a pubkey from mapWatchKeys if it exists there
    bool GetWatchPubKey(const CKeyID &address, CPubKey &pubkey_out) const;

    /**
     * Retrieves scripts that were imported by bugs into the legacy spkm and are
     * simply invalid, such as a sh(sh(pkh())) script, or not watched.
     */
    std::unordered_set<CScript, SaltedSipHasher> GetNotMineScriptPubKeys() const;

    /** Get the DescriptorScriptPubKeyMans (with private keys) that have the same scriptPubKeys as this LegacyScriptPubKeyMan.
     * Does not modify this ScriptPubKeyMan. */
    std::optional<MigrationData> MigrateToDescriptor();
    /** Delete all the records of this LegacyScriptPubKeyMan from disk*/
    bool DeleteRecordsWithDB(WalletBatch& batch);
};

/** Wraps a LegacyScriptPubKeyMan so that it can be returned in a new unique_ptr. Does not provide privkeys */
class LegacySigningProvider : public SigningProvider
{
private:
    const LegacyDataSPKM& m_spk_man;
public:
    explicit LegacySigningProvider(const LegacyDataSPKM& spk_man) : m_spk_man(spk_man) {}

    bool GetCScript(const CScriptID &scriptid, CScript& script) const override { return m_spk_man.GetCScript(scriptid, script); }
    bool HaveCScript(const CScriptID &scriptid) const override { return m_spk_man.HaveCScript(scriptid); }
    bool GetPubKey(const CKeyID &address, CPubKey& pubkey) const override { return m_spk_man.GetPubKey(address, pubkey); }
    bool GetKey(const CKeyID &address, CKey& key) const override { return false; }
    bool HaveKey(const CKeyID &address) const override { return false; }
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override { return m_spk_man.GetKeyOrigin(keyid, info); }
};

class DescriptorScriptPubKeyMan : public ScriptPubKeyMan
{
    friend class LegacyDataSPKM;
private:
    using ScriptPubKeyMap = std::map<CScript, int32_t>; // Map of scripts to descriptor range index
    using PubKeyMap = std::map<CPubKey, int32_t>; // Map of pubkeys involved in scripts to descriptor range index
    using CryptedKeyMap = std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char>>>;
    using KeyMap = std::map<CKeyID, CKey>;

    ScriptPubKeyMap m_map_script_pub_keys GUARDED_BY(cs_desc_man);
    PubKeyMap m_map_pubkeys GUARDED_BY(cs_desc_man);
    int32_t m_max_cached_index = -1;

    KeyMap m_map_keys GUARDED_BY(cs_desc_man);
    CryptedKeyMap m_map_crypted_keys GUARDED_BY(cs_desc_man);

    //! keeps track of whether Unlock has run a thorough check before
    bool m_decryption_thoroughly_checked = false;

    //! Number of pre-generated keys/scripts (part of the look-ahead process, used to detect payments)
    int64_t m_keypool_size GUARDED_BY(cs_desc_man){DEFAULT_KEYPOOL_SIZE};

    bool AddDescriptorKeyWithDB(WalletBatch& batch, const CKey& key, const CPubKey &pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_desc_man);

    KeyMap GetKeys() const EXCLUSIVE_LOCKS_REQUIRED(cs_desc_man);

    // Cached FlatSigningProviders to avoid regenerating them each time they are needed.
    mutable std::map<int32_t, FlatSigningProvider> m_map_signing_providers;
    // Fetch the SigningProvider for the given script and optionally include private keys
    std::unique_ptr<FlatSigningProvider> GetSigningProvider(const CScript& script, bool include_private = false) const;
    // Fetch the SigningProvider for a given index and optionally include private keys. Called by the above functions.
    std::unique_ptr<FlatSigningProvider> GetSigningProvider(int32_t index, bool include_private = false) const EXCLUSIVE_LOCKS_REQUIRED(cs_desc_man);

protected:
    WalletDescriptor m_wallet_descriptor GUARDED_BY(cs_desc_man);

    //! Same as 'TopUp' but designed for use within a batch transaction context
    bool TopUpWithDB(WalletBatch& batch, unsigned int size = 0);

public:
    DescriptorScriptPubKeyMan(WalletStorage& storage, WalletDescriptor& descriptor, int64_t keypool_size)
        :   ScriptPubKeyMan(storage),
            m_keypool_size(keypool_size),
            m_wallet_descriptor(descriptor)
        {}
    DescriptorScriptPubKeyMan(WalletStorage& storage, int64_t keypool_size)
        :   ScriptPubKeyMan(storage),
            m_keypool_size(keypool_size)
        {}

    mutable RecursiveMutex cs_desc_man;

    util::Result<CTxDestination> GetNewDestination(const OutputType type) override;
    isminetype IsMine(const CScript& script) const override;

    bool CheckDecryptionKey(const CKeyingMaterial& master_key) override;
    bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch) override;

    util::Result<CTxDestination> GetReservedDestination(const OutputType type, bool internal, int64_t& index) override;
    void ReturnDestination(int64_t index, bool internal, const CTxDestination& addr) override;

    // Tops up the descriptor cache and m_map_script_pub_keys. The cache is stored in the wallet file
    // and is used to expand the descriptor in GetNewDestination. DescriptorScriptPubKeyMan relies
    // more on ephemeral data than LegacyScriptPubKeyMan. For wallets using unhardened derivation
    // (with or without private keys), the "keypool" is a single xpub.
    bool TopUp(unsigned int size = 0) override;

    std::vector<WalletDestination> MarkUnusedAddresses(const CScript& script) override;

    bool IsHDEnabled() const override;

    //! Setup descriptors based on the given CExtkey
    bool SetupDescriptorGeneration(WalletBatch& batch, const CExtKey& master_key, OutputType addr_type, bool internal);

    bool HavePrivateKeys() const override;
    bool HasPrivKey(const CKeyID& keyid) const EXCLUSIVE_LOCKS_REQUIRED(cs_desc_man);
    //! Retrieve the particular key if it is available. Returns nullopt if the key is not in the wallet, or if the wallet is locked.
    std::optional<CKey> GetKey(const CKeyID& keyid) const EXCLUSIVE_LOCKS_REQUIRED(cs_desc_man);
    bool HaveCryptedKeys() const override;

    unsigned int GetKeyPoolSize() const override;

    int64_t GetTimeFirstKey() const override;

    std::unique_ptr<CKeyMetadata> GetMetadata(const CTxDestination& dest) const override;

    bool CanGetAddresses(bool internal = false) const override;

    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const override;

    bool CanProvide(const CScript& script, SignatureData& sigdata) override;

    // Fetch the SigningProvider for the given pubkey and always include private keys. This should only be called by signing code.
    std::unique_ptr<FlatSigningProvider> GetSigningProvider(const CPubKey& pubkey) const;

    bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const override;
    SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const override;
    std::optional<common::PSBTError> FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, std::optional<int> sighash_type = std::nullopt, bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const override;

    uint256 GetID() const override;

    void SetCache(const DescriptorCache& cache);

    bool AddKey(const CKeyID& key_id, const CKey& key);
    bool AddCryptedKey(const CKeyID& key_id, const CPubKey& pubkey, const std::vector<unsigned char>& crypted_key);

    bool HasWalletDescriptor(const WalletDescriptor& desc) const;
    util::Result<void> UpdateWalletDescriptor(WalletDescriptor& descriptor);
    bool CanUpdateToWalletDescriptor(const WalletDescriptor& descriptor, std::string& error);
    void AddDescriptorKey(const CKey& key, const CPubKey &pubkey);
    void WriteDescriptor();

    WalletDescriptor GetWalletDescriptor() const EXCLUSIVE_LOCKS_REQUIRED(cs_desc_man);
    std::unordered_set<CScript, SaltedSipHasher> GetScriptPubKeys() const override;
    std::unordered_set<CScript, SaltedSipHasher> GetScriptPubKeys(int32_t minimum_index) const;
    int32_t GetEndRange() const;

    [[nodiscard]] bool GetDescriptorString(std::string& out, const bool priv) const;

    void UpgradeDescriptorCache();
};

/** struct containing information needed for migrating legacy wallets to descriptor wallets */
struct MigrationData
{
    CExtKey master_key;
    std::vector<std::pair<std::string, int64_t>> watch_descs;
    std::vector<std::pair<std::string, int64_t>> solvable_descs;
    std::vector<std::unique_ptr<DescriptorScriptPubKeyMan>> desc_spkms;
    std::shared_ptr<CWallet> watchonly_wallet{nullptr};
    std::shared_ptr<CWallet> solvable_wallet{nullptr};
};

} // namespace wallet

#endif // BITCOIN_WALLET_SCRIPTPUBKEYMAN_H
