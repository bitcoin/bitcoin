// Copyright (c) 2019-2022 The Bitcoin Core developers
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

/** A key from a CWallet's keypool
 *
 * The wallet holds one (for pre HD-split wallets) or several keypools. These
 * are sets of keys that have not yet been used to provide addresses or receive
 * change.
 *
 * The Bitcoin Core wallet was originally a collection of unrelated private
 * keys with their associated addresses. If a non-HD wallet generated a
 * key/address, gave that address out and then restored a backup from before
 * that key's generation, then any funds sent to that address would be
 * lost definitively.
 *
 * The keypool was implemented to avoid this scenario (commit: 10384941). The
 * wallet would generate a set of keys (100 by default). When a new public key
 * was required, either to give out as an address or to use in a change output,
 * it would be drawn from the keypool. The keypool would then be topped up to
 * maintain 100 keys. This ensured that as long as the wallet hadn't used more
 * than 100 keys since the previous backup, all funds would be safe, since a
 * restored wallet would be able to scan for all owned addresses.
 *
 * A keypool also allowed encrypted wallets to give out addresses without
 * having to be decrypted to generate a new private key.
 *
 * With the introduction of HD wallets (commit: f1902510), the keypool
 * essentially became an address look-ahead pool. Restoring old backups can no
 * longer definitively lose funds as long as the addresses used were from the
 * wallet's HD seed (since all private keys can be rederived from the seed).
 * However, if many addresses were used since the backup, then the wallet may
 * not know how far ahead in the HD chain to look for its addresses. The
 * keypool is used to implement a 'gap limit'. The keypool maintains a set of
 * keys (by default 1000) ahead of the last used key and scans for the
 * addresses of those keys.  This avoids the risk of not seeing transactions
 * involving the wallet's addresses, or of re-using the same address.
 * In the unlikely case where none of the addresses in the `gap limit` are
 * used on-chain, the look-ahead will not be incremented to keep
 * a constant size and addresses beyond this range will not be detected by an
 * old backup. For this reason, it is not recommended to decrease keypool size
 * lower than default value.
 *
 * The HD-split wallet feature added a second keypool (commit: 02592f4c). There
 * is an external keypool (for addresses to hand out) and an internal keypool
 * (for change addresses).
 *
 * Keypool keys are stored in the wallet/keystore's keymap. The keypool data is
 * stored as sets of indexes in the wallet (setInternalKeyPool,
 * setExternalKeyPool and set_pre_split_keypool), and a map from the key to the
 * index (m_pool_key_to_index). The CKeyPool object is used to
 * serialize/deserialize the pool data to/from the database.
 */
class CKeyPool
{
public:
    //! The time at which the key was generated. Set in AddKeypoolPubKeyWithDB
    int64_t nTime;
    //! The public key
    CPubKey vchPubKey;
    //! Whether this keypool entry is in the internal keypool (for change outputs)
    bool fInternal;
    //! Whether this key was generated for a keypool before the wallet was upgraded to HD-split
    bool m_pre_split;

    CKeyPool();
    CKeyPool(const CPubKey& vchPubKeyIn, bool internalIn);

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        s << int{259900}; // Unused field, writes the highest client version ever written
        s << nTime << vchPubKey << fInternal << m_pre_split;
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        s >> int{}; // Discard unused field
        s >> nTime >> vchPubKey;
        try {
            s >> fInternal;
        } catch (std::ios_base::failure&) {
            /* flag as external address if we can't read the internal boolean
               (this will be the case for any wallet before the HD chain split version) */
            fInternal = false;
        }
        try {
            s >> m_pre_split;
        } catch (std::ios_base::failure&) {
            /* flag as postsplit address if we can't read the m_pre_split boolean
               (this will be the case for any wallet that upgrades to HD chain split) */
            m_pre_split = false;
        }
    }
};

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

    virtual util::Result<CTxDestination> GetReservedDestination(const OutputType type, bool internal, int64_t& index, CKeyPool& keypool) { return util::Error{Untranslated("Not supported")}; }
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

    /** Sets up the key generation stuff, i.e. generates new HD seeds and sets them as active.
      * Returns false if already setup or setup fails, true if setup is successful
      * Set force=true to make it re-setup if already setup, used for upgrades
      */
    virtual bool SetupGeneration(bool force = false) { return false; }

    /* Returns true if HD is enabled */
    virtual bool IsHDEnabled() const { return false; }

    /* Returns true if the wallet can give out new addresses. This means it has keys in the keypool or can generate new keys */
    virtual bool CanGetAddresses(bool internal = false) const { return false; }

    /** Upgrades the wallet to the specified version */
    virtual bool Upgrade(int prev_version, int new_version, bilingual_str& error) { return true; }

    virtual bool HavePrivateKeys() const { return false; }

    //! The action to do when the DB needs rewrite
    virtual void RewriteDB() {}

    virtual std::optional<int64_t> GetOldestKeyPoolTime() const { return GetTime(); }

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
    virtual std::optional<common::PSBTError> FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, int sighash_type = SIGHASH_DEFAULT, bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const { return common::PSBTError::UNSUPPORTED; }

    virtual uint256 GetID() const { return uint256(); }

    /** Returns a set of all the scriptPubKeys that this ScriptPubKeyMan watches */
    virtual std::unordered_set<CScript, SaltedSipHasher> GetScriptPubKeys() const { return {}; };

    /** Prepends the wallet name in logging output to ease debugging in multi-wallet use cases */
    template <typename... Params>
    void WalletLogPrintf(util::ConstevalFormatString<sizeof...(Params)> wallet_fmt, const Params&... params) const
    {
        LogInfo("%s %s", m_storage.GetDisplayName(), tfm::format(wallet_fmt, params...));
    };

    /** Watch-only address added */
    boost::signals2::signal<void (bool fHaveWatchOnly)> NotifyWatchonlyChanged;

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

class DescriptorScriptPubKeyMan;

// Manages the data for a LegacyScriptPubKeyMan.
// This is the minimum necessary to load a legacy wallet so that it can be migrated.
class LegacyDataSPKM : public ScriptPubKeyMan, public FillableSigningProvider
{
protected:
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
    // TODO: Remove IsMine when deleting LegacyScriptPubKeyMan
    isminetype IsMine(const CScript& script) const override;

    // FillableSigningProvider overrides
    bool HaveKey(const CKeyID &address) const override;
    bool GetKey(const CKeyID &address, CKey& keyOut) const override;
    bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;

    std::set<int64_t> setInternalKeyPool GUARDED_BY(cs_KeyStore);
    std::set<int64_t> setExternalKeyPool GUARDED_BY(cs_KeyStore);
    std::set<int64_t> set_pre_split_keypool GUARDED_BY(cs_KeyStore);
    int64_t m_max_keypool_index GUARDED_BY(cs_KeyStore) = 0;
    std::map<CKeyID, int64_t> m_pool_key_to_index;

    //! Load metadata (used by LoadWallet)
    virtual void LoadKeyMetadata(const CKeyID& keyID, const CKeyMetadata &metadata);
    virtual void LoadScriptMetadata(const CScriptID& script_id, const CKeyMetadata &metadata);

    //! Adds a watch-only address to the store, without saving it to disk (used by LoadWallet)
    bool LoadWatchOnly(const CScript &dest);
    //! Returns whether the watch-only script is in the wallet
    bool HaveWatchOnly(const CScript &dest) const;
    //! Returns whether there are any watch-only things in the wallet
    bool HaveWatchOnly() const;
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
    //! Load a keypool entry
    void LoadKeyPool(int64_t nIndex, const CKeyPool &keypool);

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
    bool DeleteRecords();
    bool DeleteRecordsWithDB(WalletBatch& batch);
};

// Implements the full legacy wallet behavior
class LegacyScriptPubKeyMan : public LegacyDataSPKM
{
private:
    WalletBatch *encrypted_batch GUARDED_BY(cs_KeyStore) = nullptr;

    // By default, do not scan any block until keys/scripts are generated/imported
    int64_t nTimeFirstKey GUARDED_BY(cs_KeyStore) = UNKNOWN_TIME;

    //! Number of pre-generated keys/scripts (part of the look-ahead process, used to detect payments)
    int64_t m_keypool_size GUARDED_BY(cs_KeyStore){DEFAULT_KEYPOOL_SIZE};

    bool AddKeyPubKeyInner(const CKey& key, const CPubKey &pubkey) override;

    /**
     * Private version of AddWatchOnly method which does not accept a
     * timestamp, and which will reset the wallet's nTimeFirstKey value to 1 if
     * the watch key did not previously have a timestamp associated with it.
     * Because this is an inherited virtual method, it is accessible despite
     * being marked private, but it is marked private anyway to encourage use
     * of the other AddWatchOnly which accepts a timestamp and sets
     * nTimeFirstKey more intelligently for more efficient rescans.
     */
    bool AddWatchOnly(const CScript& dest) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    //! Adds a watch-only address to the store, and saves it to disk.
    bool AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest, int64_t create_time) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    //! Adds a key to the store, and saves it to disk.
    bool AddKeyPubKeyWithDB(WalletBatch &batch,const CKey& key, const CPubKey &pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    void AddKeypoolPubkeyWithDB(const CPubKey& pubkey, const bool internal, WalletBatch& batch);

    //! Adds a script to the store and saves it to disk
    bool AddCScriptWithDB(WalletBatch& batch, const CScript& script);

    /** Add a KeyOriginInfo to the wallet */
    bool AddKeyOriginWithDB(WalletBatch& batch, const CPubKey& pubkey, const KeyOriginInfo& info);

    /* HD derive new child key (on internal or external chain) */
    void DeriveNewChildKey(WalletBatch& batch, CKeyMetadata& metadata, CKey& secret, CHDChain& hd_chain, bool internal = false) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    // Tracks keypool indexes to CKeyIDs of keys that have been taken out of the keypool but may be returned to it
    std::map<int64_t, CKeyID> m_index_to_reserved_key;

    //! Fetches a key from the keypool
    bool GetKeyFromPool(CPubKey &key, const OutputType type);

    /**
     * Reserves a key from the keypool and sets nIndex to its index
     *
     * @param[out] nIndex the index of the key in keypool
     * @param[out] keypool the keypool the key was drawn from, which could be the
     *     the pre-split pool if present, or the internal or external pool
     * @param fRequestedInternal true if the caller would like the key drawn
     *     from the internal keypool, false if external is preferred
     *
     * @return true if succeeded, false if failed due to empty keypool
     * @throws std::runtime_error if keypool read failed, key was invalid,
     *     was not found in the wallet, or was misclassified in the internal
     *     or external keypool
     */
    bool ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool, bool fRequestedInternal);

    /**
     * Like TopUp() but adds keys for inactive HD chains.
     * Ensures that there are at least -keypool number of keys derived after the given index.
     *
     * @param seed_id the CKeyID for the HD seed.
     * @param index the index to start generating keys from
     * @param internal whether the internal chain should be used. true for internal chain, false for external chain.
     *
     * @return true if seed was found and keys were derived. false if unable to derive seeds
     */
    bool TopUpInactiveHDChain(const CKeyID seed_id, int64_t index, bool internal);

    bool TopUpChain(WalletBatch& batch, CHDChain& chain, unsigned int size);
public:
    LegacyScriptPubKeyMan(WalletStorage& storage, int64_t keypool_size) : LegacyDataSPKM(storage), m_keypool_size(keypool_size) {}

    util::Result<CTxDestination> GetNewDestination(const OutputType type) override;

    bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch) override;

    util::Result<CTxDestination> GetReservedDestination(const OutputType type, bool internal, int64_t& index, CKeyPool& keypool) override;
    void KeepDestination(int64_t index, const OutputType& type) override;
    void ReturnDestination(int64_t index, bool internal, const CTxDestination&) override;

    bool TopUp(unsigned int size = 0) override;

    std::vector<WalletDestination> MarkUnusedAddresses(const CScript& script) override;

    //! Upgrade stored CKeyMetadata objects to store key origin info as KeyOriginInfo
    void UpgradeKeyMetadata();

    bool IsHDEnabled() const override;

    bool SetupGeneration(bool force = false) override;

    bool Upgrade(int prev_version, int new_version, bilingual_str& error) override;

    bool HavePrivateKeys() const override;

    void RewriteDB() override;

    std::optional<int64_t> GetOldestKeyPoolTime() const override;
    size_t KeypoolCountExternalKeys() const;
    unsigned int GetKeyPoolSize() const override;

    int64_t GetTimeFirstKey() const override;

    std::unique_ptr<CKeyMetadata> GetMetadata(const CTxDestination& dest) const override;

    bool CanGetAddresses(bool internal = false) const override;

    bool CanProvide(const CScript& script, SignatureData& sigdata) override;

    bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const override;
    SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const override;
    std::optional<common::PSBTError> FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, int sighash_type = SIGHASH_DEFAULT, bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const override;

    uint256 GetID() const override;

    //! Adds a key to the store, and saves it to disk.
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey) override;
    //! Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    void UpdateTimeFirstKey(int64_t nCreateTime) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    //! Load metadata (used by LoadWallet)
    void LoadKeyMetadata(const CKeyID& keyID, const CKeyMetadata &metadata) override;
    void LoadScriptMetadata(const CScriptID& script_id, const CKeyMetadata &metadata) override;
    //! Generate a new key
    CPubKey GenerateNewKey(WalletBatch& batch, CHDChain& hd_chain, bool internal = false) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    /* Set the HD chain model (chain child index counters) and writes it to the database */
    void AddHDChain(const CHDChain& chain);

    //! Remove a watch only script from the keystore
    bool RemoveWatchOnly(const CScript &dest);
    bool AddWatchOnly(const CScript& dest, int64_t nCreateTime) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    /* SigningProvider overrides */
    bool AddCScript(const CScript& redeemScript) override;

    bool NewKeyPool();
    void MarkPreSplitKeys() EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    bool ImportScripts(const std::set<CScript> scripts, int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool ImportPubKeys(const std::vector<std::pair<CKeyID, bool>>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool ImportScriptPubKeys(const std::set<CScript>& script_pub_keys, const bool have_solving_data, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    /* Returns true if the wallet can generate new keys */
    bool CanGenerateKeys() const;

    /* Generates a new HD seed (will not be activated) */
    CPubKey GenerateNewSeed();

    /* Derives a new HD seed (will not be activated) */
    CPubKey DeriveNewSeed(const CKey& key);

    /* Set the current HD seed (will reset the chain child index counters)
       Sets the seed's version based on the current wallet version (so the
       caller must ensure the current wallet version is correct before calling
       this function). */
    void SetHDSeed(const CPubKey& key);

    /**
     * Explicitly make the wallet learn the related scripts for outputs to the
     * given key. This is purely to make the wallet file compatible with older
     * software, as FillableSigningProvider automatically does this implicitly for all
     * keys now.
     */
    void LearnRelatedScripts(const CPubKey& key, OutputType);

    /**
     * Same as LearnRelatedScripts, but when the OutputType is not known (and could
     * be anything).
     */
    void LearnAllRelatedScripts(const CPubKey& key);

    /**
     * Marks all keys in the keypool up to and including the provided key as used.
     *
     * @param keypool_id determines the last key to mark as used
     *
     * @return All affected keys
     */
    std::vector<CKeyPool> MarkReserveKeysAsUsed(int64_t keypool_id) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    const std::map<CKeyID, int64_t>& GetAllReserveKeys() const { return m_pool_key_to_index; }

    std::set<CKeyID> GetKeys() const override;
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
    // Fetch the SigningProvider for the given pubkey and always include private keys. This should only be called by signing code.
    std::unique_ptr<FlatSigningProvider> GetSigningProvider(const CPubKey& pubkey) const;
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

    util::Result<CTxDestination> GetReservedDestination(const OutputType type, bool internal, int64_t& index, CKeyPool& keypool) override;
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

    std::optional<int64_t> GetOldestKeyPoolTime() const override;
    unsigned int GetKeyPoolSize() const override;

    int64_t GetTimeFirstKey() const override;

    std::unique_ptr<CKeyMetadata> GetMetadata(const CTxDestination& dest) const override;

    bool CanGetAddresses(bool internal = false) const override;

    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const override;

    bool CanProvide(const CScript& script, SignatureData& sigdata) override;

    bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const override;
    SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const override;
    std::optional<common::PSBTError> FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, int sighash_type = SIGHASH_DEFAULT, bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const override;

    uint256 GetID() const override;

    void SetCache(const DescriptorCache& cache);

    bool AddKey(const CKeyID& key_id, const CKey& key);
    bool AddCryptedKey(const CKeyID& key_id, const CPubKey& pubkey, const std::vector<unsigned char>& crypted_key);

    bool HasWalletDescriptor(const WalletDescriptor& desc) const;
    void UpdateWalletDescriptor(WalletDescriptor& descriptor);
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
