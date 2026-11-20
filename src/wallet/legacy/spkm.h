// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_LEGACY_SPKM_H
#define BITCOIN_WALLET_LEGACY_SPKM_H

#include <wallet/scriptpubkeyman.h>

namespace wallet {
namespace legacy_migration {
/** Output types associated with LegacyDataSPKM. */
inline const std::unordered_set<OutputType> LEGACY_OUTPUT_TYPES {
    OutputType::LEGACY,
    OutputType::P2SH_SEGWIT,
    OutputType::BECH32,
};

// Manages the minimum data needed to load and migrate a legacy wallet.
class LegacyDataSPKM : public ScriptPubKeyMan, public FillableSigningProvider
{
private:
    using WatchOnlySet = std::set<CScript>;
    using WatchKeyMap = std::map<CKeyID, CPubKey>;

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

    bool IsMine(const CScript& script) const override;
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
    [[nodiscard]] bool LoadCScript(const CScript& redeemScript);
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

    /** Get the DescriptorScriptPubKeyMans (with private keys) that have the same scriptPubKeys as this LegacyDataSPKM.
     * Does not modify this LegacyDataSPKM. */
    std::optional<MigrationData> MigrateToDescriptor();
};

/** SigningProvider wrapper for LegacyDataSPKM that does not provide private keys. */
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

} // namespace legacy_migration
} // wallet
#endif // BITCOIN_WALL_LEGACY_SPKM_H
