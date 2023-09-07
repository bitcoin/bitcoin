// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SCRIPTPUBKEYMAN_H
#define BITCOIN_WALLET_SCRIPTPUBKEYMAN_H

#include <psbt.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <util/error.h>
#include <util/message.h>
#include <util/time.h>
#include <wallet/crypter.h>
#include <wallet/ismine.h>
#include <wallet/walletdb.h>
#include <wallet/walletutil.h>

#include <boost/signals2/signal.hpp>

#include <optional>

// Wallet storage things that ScriptPubKeyMans need in order to be able to store things to the wallet database.
// It provides access to things that are part of the entire wallet and not specific to a ScriptPubKeyMan such as
// wallet flags, wallet version, encryption keys, encryption status, and the database itself. This allows a
// ScriptPubKeyMan to have callbacks into CWallet without causing a circular dependency.
// WalletStorage should be the same for all ScriptPubKeyMans of a wallet.
class WalletStorage
{
public:
    virtual ~WalletStorage() = default;
    virtual const std::string GetDisplayName() const = 0;
    virtual WalletDatabase& GetDatabase() const = 0;
    virtual bool IsWalletFlagSet(uint64_t) const = 0;
    virtual void UnsetBlankWalletFlag(WalletBatch&) = 0;
    virtual bool CanSupportFeature(enum WalletFeature) const = 0;
    virtual void SetMinVersion(enum WalletFeature, WalletBatch* = nullptr, bool = false) = 0;
    virtual const CKeyingMaterial& GetEncryptionKey() const = 0;
    virtual bool HasEncryptionKeys() const = 0;
    virtual bool IsLocked(bool fForMixing = false) const = 0;

    // methods below are unique from Dash due to different implementation of HD
    virtual void NewKeyPoolCallback() = 0;
    virtual void KeepDestinationCallback(bool erased) = 0;
};

//! Default for -keypool
static const unsigned int DEFAULT_KEYPOOL_SIZE = 1000;

std::vector<CKeyID> GetAffectedKeys(const CScript& spk, const SigningProvider& provider);

/** A key from a CWallet's keypool
 *
 * The wallet holds several keypools. These are sets of keys that have not
 * yet been used to provide addresses or receive change.
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
 * There is an external keypool (for addresses to hand out) and an internal keypool
 * (for change addresses).
 *
 * Keypool keys are stored in the wallet/keystore's keymap. The keypool data is
 * stored as sets of indexes in the wallet (setInternalKeyPool and
 * setExternalKeyPool), and a map from the key to the
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

    CKeyPool();
    CKeyPool(const CPubKey& vchPubKeyIn, bool fInternalIn);

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH)) {
            s << nVersion;
        }
        s << nTime << vchPubKey << fInternal;
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH)) {
            s >> nVersion;
        }
        s >> nTime >> vchPubKey;
        try {
            s >> fInternal;
        } catch (std::ios_base::failure&) {
            /* flag as external address if we can't read the internal boolean
               (this will be the case for any wallet before the HD chain split version) */
            fInternal = false;
        }
    }
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

    virtual ~ScriptPubKeyMan() {};
    virtual bool GetNewDestination(CTxDestination& dest, std::string& error) { return false; }
    virtual isminetype IsMine(const CScript& script) const { return ISMINE_NO; }
    virtual isminetype IsMine(const CTxDestination& dest) const { return ISMINE_NO; }

    //! Check that the given decryption key is valid for this ScriptPubKeyMan, i.e. it decrypts all of the keys handled by it.
    virtual bool CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys = false) { return false; }
    virtual bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch) { return false; }

    virtual bool GetReservedDestination(bool internal, CTxDestination& address, int64_t& index, CKeyPool& keypool) { return false; }
    virtual void KeepDestination(int64_t index) {}
    virtual void ReturnDestination(int64_t index, bool internal, const CTxDestination& addr) {}

    /** Fills internal address pool. Use within ScriptPubKeyMan implementations should be used sparingly and only
      * when something from the address pool is removed, excluding GetNewDestination and GetReservedDestination.
      * External wallet code is primarily responsible for topping up prior to fetching new addresses
      */
    virtual bool TopUp(unsigned int size = 0) { return false; }

    //! Mark unused addresses as being used
    virtual void MarkUnusedAddresses(WalletBatch &batch, const CScript& script, const std::optional<int64_t>& block_time) {}

    /* Returns true if HD is enabled */
    virtual bool IsHDEnabled() const { return false; }

    /* Returns true if the wallet can give out new addresses. This means it has keys in the keypool or can generate new keys */
    virtual bool CanGetAddresses(bool internal = false) const { return false; }

    virtual bool HavePrivateKeys() const { return false; }

    //! The action to do when the DB needs rewrite
    virtual void RewriteDB() {}

    virtual int64_t GetOldestKeyPoolTime() const { return GetTime(); }

    virtual size_t KeypoolCountExternalKeys() const { return 0; }
    virtual size_t KeypoolCountInternalKeys() const { return 0; }
    virtual unsigned int GetKeyPoolSize() const { return 0; }

    virtual int64_t GetTimeFirstKey() const { return 0; }

    virtual const CKeyMetadata* GetMetadata(const CTxDestination& dest) const { return nullptr; }

    virtual std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const { return nullptr; }

    /** Whether this ScriptPubKeyMan can provide a SigningProvider (via GetSolvingProvider) that, combined with
      * sigdata, can produce solving data.
      */
    virtual bool CanProvide(const CScript& script, SignatureData& sigdata) { return false; }

    /** Creates new signatures and adds them to the transaction. Returns whether all inputs were signed */
    virtual bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, std::string>& input_errors) const { return false; }
    /** Sign a message with the given script */
    virtual SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const { return SigningResult::SIGNING_FAILED; };
    /** Adds script and derivation path information to a PSBT, and optionally signs it. */
    virtual TransactionError FillPSBT(PartiallySignedTransaction& psbt, int sighash_type = 1 /* SIGHASH_ALL */, bool sign = true, bool bip32derivs = false) const { return TransactionError::INVALID_PSBT; }

    virtual uint256 GetID() const { return uint256(); }

    /** Prepends the wallet name in logging output to ease debugging in multi-wallet use cases */
    template<typename... Params>
    void WalletLogPrintf(std::string fmt, Params... parameters) const {
        LogPrintf(("%s " + fmt).c_str(), m_storage.GetDisplayName(), parameters...);
    };

    /** Watch-only address added */
    boost::signals2::signal<void (bool fHaveWatchOnly)> NotifyWatchonlyChanged;

    /** Keypool has new keys */
    boost::signals2::signal<void ()> NotifyCanGetAddressesChanged;
};

class LegacyScriptPubKeyMan : public ScriptPubKeyMan, public FillableSigningProvider
{
private:
    //! keeps track of whether Unlock has run a thorough check before
    bool fDecryptionThoroughlyChecked = true;

    using WatchOnlySet = std::set<CScript>;
    using WatchKeyMap = std::map<CKeyID, CPubKey>;
    using HDPubKeyMap = std::map<CKeyID, CHDPubKey>;


    WalletBatch *encrypted_batch GUARDED_BY(cs_KeyStore) = nullptr;

    using CryptedKeyMap = std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char>>>;

    CryptedKeyMap mapCryptedKeys GUARDED_BY(cs_KeyStore);
    WatchOnlySet setWatchOnly GUARDED_BY(cs_KeyStore);
    WatchKeyMap mapWatchKeys GUARDED_BY(cs_KeyStore);
    HDPubKeyMap mapHdPubKeys GUARDED_BY(cs_KeyStore); //<! memory map of HD extended pubkeys

    int64_t nTimeFirstKey GUARDED_BY(cs_KeyStore) = 0;

    bool HaveKeyInner(const CKeyID &address) const;
    bool AddKeyPubKeyInner(const CKey& key, const CPubKey &pubkey);
    bool AddCryptedKeyInner(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool GetKeyInner(const CKeyID &address, CKey& keyOut) const;
    bool GetPubKeyInner(const CKeyID &address, CPubKey& vchPubKeyOut) const;

    bool TopUpInner(unsigned int size = 0) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
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
    bool AddWatchOnlyInMem(const CScript &dest);

    void AddKeypoolPubkeyWithDB(const CPubKey& pubkey, const bool internal, WalletBatch& batch);

    /** Add a KeyOriginInfo to the wallet */
    bool AddKeyOriginWithDB(WalletBatch& batch, const CPubKey& pubkey, const KeyOriginInfo& info);

    /* the HD chain data model (external chain counters) */
    CHDChain hdChain GUARDED_BY(cs_KeyStore);
    CHDChain cryptedHDChain GUARDED_BY(cs_KeyStore);

    /* HD derive new child key (on internal or external chain) */
    void DeriveNewChildKey(WalletBatch& batch, CKeyMetadata& metadata, CKey& secretRet, uint32_t nAccountIndex, bool fInternal /*= false*/) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    std::set<int64_t> setInternalKeyPool GUARDED_BY(cs_KeyStore);
    std::set<int64_t> setExternalKeyPool GUARDED_BY(cs_KeyStore);
    int64_t m_max_keypool_index GUARDED_BY(cs_KeyStore) = 0;
    std::map<CKeyID, int64_t> m_pool_key_to_index;
    // Tracks keypool indexes to CKeyIDs of keys that have been taken out of the keypool but may be returned to it
    std::map<int64_t, CKeyID> m_index_to_reserved_key;

    //! Fetches a key from the keypool
    bool GetKeyFromPool(CPubKey &key, bool fInternal /*= false*/);

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

public:
    using ScriptPubKeyMan::ScriptPubKeyMan;

    bool GetNewDestination(CTxDestination& dest, std::string& error) override;
    isminetype IsMine(const CScript& script) const override;
    isminetype IsMine(const CTxDestination& dest) const override;

    bool CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys = false) override;
    bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch) override;

    bool GetReservedDestination(bool internal, CTxDestination& address, int64_t& index, CKeyPool& keypool) override;
    void KeepDestination(int64_t index) override;
    void ReturnDestination(int64_t index, bool internal, const CTxDestination&) override;

    bool TopUp(unsigned int size = 0) override;

    void MarkUnusedAddresses(WalletBatch &batch, const CScript& script, const std::optional<int64_t>& block_time) override;

    //! Upgrade stored CKeyMetadata objects to store key origin info as KeyOriginInfo
    void UpgradeKeyMetadata();

    /* Returns true if HD is enabled */
    bool IsHDEnabled() const override;

    bool HavePrivateKeys() const override;

    void RewriteDB() override;

    int64_t GetOldestKeyPoolTime() const override;
    size_t KeypoolCountExternalKeys() const override;
    size_t KeypoolCountInternalKeys() const override;

    unsigned int GetKeyPoolSize() const override;

    int64_t GetTimeFirstKey() const override;

    const CKeyMetadata* GetMetadata(const CTxDestination& dest) const override;

    bool CanGetAddresses(bool internal = false) const override;

    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const override;

    bool CanProvide(const CScript& script, SignatureData& sigdata) override;

    bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, std::string>& input_errors) const override;
    SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const override;
    TransactionError FillPSBT(PartiallySignedTransaction& psbt, int sighash_type = 1 /* SIGHASH_ALL */, bool sign = true, bool bip32derivs = false) const override;

    uint256 GetID() const override;

    // Map from Key ID to key metadata.
    std::map<CKeyID, CKeyMetadata> mapKeyMetadata GUARDED_BY(cs_KeyStore);

    // Map from Script ID to key metadata (for watch-only keys).
    std::map<CScriptID, CKeyMetadata> m_script_metadata GUARDED_BY(cs_KeyStore);

    //! Adds a script to the store and saves it to disk
    bool AddCScriptWithDB(WalletBatch& batch, const CScript& script);

    //! Adds a key to the store, and saves it to disk.
    bool AddKeyPubKeyWithDB(WalletBatch &batch,const CKey& key, const CPubKey &pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    //! Adds a key to the store, and saves it to disk.
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey) override;
    //! Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKey(const CKey& key, const CPubKey &pubkey);
    //! Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    //! Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
    bool LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret, bool checksum_valid);
    void UpdateTimeFirstKey(int64_t nCreateTime) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    //! Adds a CScript to the store
    bool LoadCScript(const CScript& redeemScript);
    //! Load metadata (used by LoadWallet)
    void LoadKeyMetadata(const CKeyID& keyID, const CKeyMetadata &metadata);
    bool WriteKeyMetadata(const CKeyMetadata& meta, const CPubKey& pubkey, bool overwrite);
    void LoadScriptMetadata(const CScriptID& script_id, const CKeyMetadata &metadata);
    //! Generate a new key
    CPubKey GenerateNewKey(WalletBatch& batch, uint32_t nAccountIndex, bool fInternal /*= false*/) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    /* Set the HD chain model (chain child index counters) */
    bool SetHDChain(WalletBatch &batch, const CHDChain& chain, bool memonly);
    bool SetCryptedHDChain(WalletBatch &batch, const CHDChain& chain, bool memonly);
    /**
     * Set the HD chain model (chain child index counters) using temporary wallet db object
     * which causes db flush every time these methods are used
     */
    bool SetHDChainSingle(const CHDChain& chain, bool memonly);
    bool SetCryptedHDChainSingle(const CHDChain& chain, bool memonly);

    //! Adds a watch-only address to the store, without saving it to disk (used by LoadWallet)
    bool LoadWatchOnly(const CScript &dest);
    //! Returns whether the watch-only script is in the wallet
    bool HaveWatchOnly(const CScript &dest) const;
    //! Returns whether there are any watch-only things in the wallet
    bool HaveWatchOnly() const;
    //! Remove a watch only script from the keystore
    bool RemoveWatchOnly(const CScript &dest);
    //! Adds a watch-only address to the store, and saves it to disk.
    bool AddWatchOnly(const CScript& dest, int64_t nCreateTime) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    //! Adds a watch-only address to the store, and saves it to disk.
    bool AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest, int64_t create_time) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    //! Fetches a pubkey from mapWatchKeys if it exists there
    bool GetWatchPubKey(const CKeyID &address, CPubKey &pubkey_out) const;

    //! HaveHDKey similar to HaveKey() but checks only mapHdPubKeys
    bool HaveHDKey(const CKeyID &address, CHDChain& hdChainCurrent) const;

    /* SigningProvider overrides */
    //! HaveKey implementation that also checks the mapHdPubKeys
    bool HaveKey(const CKeyID &address) const override;
    //! GetKey implementation that can derive a HD private key on the fly
    bool GetKey(const CKeyID &address, CKey& keyOut) const override;
    //! GetPubKey implementation that also checks the mapHdPubKeys
    bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const override;
    bool AddCScript(const CScript& redeemScript) override;
    /** Implement lookup of key origin information through wallet key metadata. */
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
    /** Add a KeyOriginInfo to the wallet */
    bool AddKeyOrigin(const CPubKey& pubkey, const KeyOriginInfo& info);

    void LoadKeyPool(int64_t nIndex, const CKeyPool &keypool);
    bool NewKeyPool();
    // Seems as not used now anywhere in code
    // void AddKeypoolPubkey(const CPubKey& pubkey, const bool internal);

    bool ImportScripts(const std::set<CScript> scripts, int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    bool ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool ImportPubKeys(const std::vector<CKeyID>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const bool internal, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool ImportScriptPubKeys(const std::set<CScript>& script_pub_keys, const bool have_solving_data, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    /* Returns true if the wallet can generate new keys */
    bool CanGenerateKeys() const;

    //! Adds a HDPubKey into the wallet(database)
    bool AddHDPubKey(WalletBatch &batch, const CExtPubKey &extPubKey, bool fInternal);
    //! loads a HDPubKey into the wallets memory
    bool LoadHDPubKey(const CHDPubKey &hdPubKey) /*EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore) */;

    /**
     * HD Wallet Functions
     */

    bool EncryptHDChain(const CKeyingMaterial& vMasterKeyIn, const CHDChain& chain = CHDChain());
    bool DecryptHDChain(const CKeyingMaterial& vMasterKeyIn, CHDChain& hdChainRet) const;
    bool SetHDChain(const CHDChain& chain);
    bool GetHDChain(CHDChain& hdChainRet) const;
    bool SetCryptedHDChain(const CHDChain& chain);
    bool GetDecryptedHDChain(CHDChain& hdChainRet);

    /* Generates a new HD chain */
    void GenerateNewHDChain(const SecureString& secureMnemonic, const SecureString& secureMnemonicPassphrase);

    /**
     * Explicitly make the wallet learn the related scripts for outputs to the
     * given key. This is purely to make the wallet file compatible with older
     * software, as FillableSigningProvider automatically does this implicitly for all
     * keys now.
     */
    // void LearnRelatedScripts(const CPubKey& key, OutputType);

    /**
     * Same as LearnRelatedScripts, but when the OutputType is not known (and could
     * be anything).
     */
    // void LearnAllRelatedScripts(const CPubKey& key);

    /**
     * Marks all keys in the keypool up to and including reserve_key as used.
     */
    void MarkReserveKeysAsUsed(int64_t keypool_id) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    const std::map<CKeyID, int64_t>& GetAllReserveKeys() const { return m_pool_key_to_index; }

    std::set<CKeyID> GetKeys() const override;
};

/** Wraps a LegacyScriptPubKeyMan so that it can be returned in a new unique_ptr. Does not provide privkeys */
class LegacySigningProvider : public SigningProvider
{
private:
    const LegacyScriptPubKeyMan& m_spk_man;
public:
    explicit LegacySigningProvider(const LegacyScriptPubKeyMan& spk_man) : m_spk_man(spk_man) {}

    bool GetCScript(const CScriptID &scriptid, CScript& script) const override { return m_spk_man.GetCScript(scriptid, script); }
    bool HaveCScript(const CScriptID &scriptid) const override { return m_spk_man.HaveCScript(scriptid); }
    bool GetPubKey(const CKeyID &address, CPubKey& pubkey) const override { return m_spk_man.GetPubKey(address, pubkey); }
    bool GetKey(const CKeyID &address, CKey& key) const override { return false; }
    bool HaveKey(const CKeyID &address) const override { return false; }
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override { return m_spk_man.GetKeyOrigin(keyid, info); }
};

#endif // BITCOIN_WALLET_SCRIPTPUBKEYMAN_H
