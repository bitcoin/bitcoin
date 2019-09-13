// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SCRIPTPUBKEYMAN_H
#define BITCOIN_WALLET_SCRIPTPUBKEYMAN_H

#include <script/signingprovider.h>
#include <script/standard.h>
#include <wallet/crypter.h>
#include <wallet/ismine.h>
#include <wallet/walletdb.h>
#include <wallet/walletutil.h>

#include <boost/signals2/signal.hpp>

enum class OutputType;

// Wallet storage things that ScriptPubKeyMans need in order to be able to store things to the wallet database.
// It provides access to things that are part of the entire wallet and not specific to a ScriptPubKeyMan such as
// wallet flags, wallet version, encryption keys, encryption status, and the database itself. This allows a
// ScriptPubKeyMan to have callbacks into CWallet without causing a circular dependency.
// WalletStorage should be the same for all ScriptPubKeyMans.
class WalletStorage
{
public:
    virtual ~WalletStorage() = default;
    virtual const std::string GetDisplayName() const = 0;
    virtual std::shared_ptr<WalletDatabase> GetDatabase() = 0;
    virtual bool IsWalletFlagSet(uint64_t) const = 0;
    virtual void SetWalletFlag(uint64_t) = 0;
    virtual void UnsetWalletFlagWithDB(WalletBatch&, uint64_t) = 0;
    virtual bool CanSupportFeature(enum WalletFeature) const = 0;
    virtual void SetMinVersion(enum WalletFeature, WalletBatch* = nullptr, bool = false) = 0;
    virtual const CKeyingMaterial& GetEncryptionKey() const = 0;
    virtual bool HasEncryptionKeys() const = 0;
    virtual bool IsLocked() const = 0;
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
    std::shared_ptr<WalletDatabase> m_database;

    const std::string GetDisplayName() const { return m_storage.GetDisplayName(); }
    bool IsWalletFlagSet(uint64_t flag) const { return m_storage.IsWalletFlagSet(flag); }
    void SetWalletFlag(uint64_t flag) { return m_storage.SetWalletFlag(flag); }
    void UnsetWalletFlagWithDB(WalletBatch& batch, uint64_t flag) { return m_storage.UnsetWalletFlagWithDB(batch, flag); }
    bool CanSupportFeature(enum WalletFeature wf) const { return m_storage.CanSupportFeature(wf); }
    void SetMinVersion(enum WalletFeature wf) { return m_storage.SetMinVersion(wf); }
    const CKeyingMaterial& GetEncryptionKey() const { return m_storage.GetEncryptionKey(); }
    bool HasEncryptionKeys() const { return m_storage.HasEncryptionKeys(); }
    bool IsLocked() const { return m_storage.IsLocked(); }

public:
    ScriptPubKeyMan(WalletStorage& storage) : m_storage(storage), m_database(storage.GetDatabase()) {}

    virtual ~ScriptPubKeyMan() {};
    virtual bool GetNewDestination(const OutputType type, CTxDestination& dest, std::string& error) { return false; }
    virtual isminetype IsMine(const CScript& script) const { return ISMINE_NO; }

    //! Check that the given decryption key is valid for this ScriptPubKeyMan, i.e. it decrypts all of the keys handled by it.
    virtual bool CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys = false) { return false; }
    virtual bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch) { return false; }

    virtual bool GetReservedDestination(const OutputType type, bool internal, CTxDestination& address, int64_t& index, CKeyPool& keypool) { return false; }
    virtual void KeepDestination(int64_t index) {}
    virtual void ReturnDestination(int64_t index, bool internal, const CTxDestination& addr) {}

    virtual bool TopUp(unsigned int size = 0) { return false; }

    //! Mark unused addresses as being used
    virtual void MarkUnusedAddresses(const CScript& script) {}

    //! Upgrade stored CKeyMetadata objects to store key origin info as KeyOriginInfo
    virtual void UpgradeKeyMetadata() {}

    /** Sets up the key generation stuff, i.e. generates new HD seeds and sets them as active.
      * Returns false if already setup or setup fails, true if setup is successful
      * Set force=true to make it re-setup if already setup, used for upgrades
      */
    virtual bool SetupGeneration(bool force = false) { return false; }

    /* Returns true if HD is enabled */
    virtual bool IsHDEnabled() const { return false; }

    /* Returns true if the wallet can give out new addresses. This means it has keys in the keypool or can generate new keys */
    virtual bool CanGetAddresses(bool internal = false) { return false; }

    /** Upgrades the wallet to the specified version */
    virtual bool Upgrade(int prev_version, std::string& error) { return false; }

    virtual bool HavePrivateKeys() const { return false; }

    //! The action to do when the DB needs rewrite
    virtual void RewriteDB() {}

    virtual int64_t GetOldestKeyPoolTime() { return GetTime(); }

    virtual size_t KeypoolCountExternalKeys() { return 0; }
    virtual unsigned int GetKeyPoolSize() const { return 0; }

    virtual int64_t GetTimeFirstKey() const { return 0; }

    virtual const CKeyMetadata* GetMetadata(uint160 id) const { return nullptr; }

    virtual std::unique_ptr<SigningProvider> GetSigningProvider(const CScript& script) const { return nullptr; }

    /** Whether this ScriptPubKeyMan can provide a SigningProvider (via GetSigningProvider) that, combined with
      * sigdata, can produce a valid signature.
      */
    virtual bool CanProvide(const CScript& script, SignatureData& sigdata) { return false; }

    virtual uint256 GetID() const { return uint256(); }

    /** Watch-only address added */
    boost::signals2::signal<void (bool fHaveWatchOnly)> NotifyWatchonlyChanged;

    /** Keypool has new keys */
    boost::signals2::signal<void ()> NotifyCanGetAddressesChanged;
};

class LegacyScriptPubKeyMan : public ScriptPubKeyMan, public FillableSigningProvider
{
public:
    using ScriptPubKeyMan::ScriptPubKeyMan;

    bool GetNewDestination(const OutputType type, CTxDestination& dest, std::string& error) override;
    isminetype IsMine(const CScript& script) const override;

    bool CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys = false) override;
    bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch) override;

    bool GetReservedDestination(const OutputType type, bool internal, CTxDestination& address, int64_t& index, CKeyPool& keypool) override;
    void KeepDestination(int64_t index) override;
    void ReturnDestination(int64_t index, bool internal, const CTxDestination& addr) override;

    bool TopUp(unsigned int size = 0) override;

    void MarkUnusedAddresses(const CScript& script) override;

    void UpgradeKeyMetadata() override EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

    bool IsHDEnabled() const override;

    bool SetupGeneration(bool force = false) override;

    bool Upgrade(int prev_version, std::string& error) override;

    bool HavePrivateKeys() const override;

    void RewriteDB() override;

    int64_t GetOldestKeyPoolTime() override;
    size_t KeypoolCountExternalKeys() override EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    unsigned int GetKeyPoolSize() const override;

    int64_t GetTimeFirstKey() const override;

    const CKeyMetadata* GetMetadata(uint160 id) const override;

    bool CanGetAddresses(bool internal = false) override;

    std::unique_ptr<SigningProvider> GetSigningProvider(const CScript& script) const override;

    bool CanProvide(const CScript& script, SignatureData& sigdata) override;

    uint256 GetID() const override;
};

#endif // BITCOIN_WALLET_SCRIPTPUBKEYMAN_H
