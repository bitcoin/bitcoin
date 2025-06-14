// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <key_io.h>
#include <logging.h>
#include <node/types.h>
#include <outputtype.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/solver.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <util/translation.h>
#include <wallet/scriptpubkeyman.h>

#include <optional>

using common::PSBTError;
using util::ToString;

namespace wallet {

typedef std::vector<unsigned char> valtype;

// Legacy wallet IsMine(). Used only in migration
// DO NOT USE ANYTHING IN THIS NAMESPACE OUTSIDE OF MIGRATION
namespace {

/**
 * This is an enum that tracks the execution context of a script, similar to
 * SigVersion in script/interpreter. It is separate however because we want to
 * distinguish between top-level scriptPubKey execution and P2SH redeemScript
 * execution (a distinction that has no impact on consensus rules).
 */
enum class IsMineSigVersion
{
    TOP = 0,        //!< scriptPubKey execution
    P2SH = 1,       //!< P2SH redeemScript
    WITNESS_V0 = 2, //!< P2WSH witness script execution
};

/**
 * This is an internal representation of isminetype + invalidity.
 * Its order is significant, as we return the max of all explored
 * possibilities.
 */
enum class IsMineResult
{
    NO = 0,         //!< Not ours
    WATCH_ONLY = 1, //!< Included in watch-only balance
    SPENDABLE = 2,  //!< Included in all balances
    INVALID = 3,    //!< Not spendable by anyone (uncompressed pubkey in segwit, P2SH inside P2SH or witness, witness inside witness)
};

bool PermitsUncompressed(IsMineSigVersion sigversion)
{
    return sigversion == IsMineSigVersion::TOP || sigversion == IsMineSigVersion::P2SH;
}

bool HaveKeys(const std::vector<valtype>& pubkeys, const LegacyDataSPKM& keystore)
{
    for (const valtype& pubkey : pubkeys) {
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (!keystore.HaveKey(keyID)) return false;
    }
    return true;
}

//! Recursively solve script and return spendable/watchonly/invalid status.
//!
//! @param keystore            legacy key and script store
//! @param scriptPubKey        script to solve
//! @param sigversion          script type (top-level / redeemscript / witnessscript)
//! @param recurse_scripthash  whether to recurse into nested p2sh and p2wsh
//!                            scripts or simply treat any script that has been
//!                            stored in the keystore as spendable
// NOLINTNEXTLINE(misc-no-recursion)
IsMineResult LegacyWalletIsMineInnerDONOTUSE(const LegacyDataSPKM& keystore, const CScript& scriptPubKey, IsMineSigVersion sigversion, bool recurse_scripthash=true)
{
    IsMineResult ret = IsMineResult::NO;

    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);

    CKeyID keyID;
    switch (whichType) {
    case TxoutType::NONSTANDARD:
    case TxoutType::NULL_DATA:
    case TxoutType::WITNESS_UNKNOWN:
    case TxoutType::WITNESS_V1_TAPROOT:
    case TxoutType::ANCHOR:
        break;
    case TxoutType::PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        if (!PermitsUncompressed(sigversion) && vSolutions[0].size() != 33) {
            return IsMineResult::INVALID;
        }
        if (keystore.HaveKey(keyID)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    case TxoutType::WITNESS_V0_KEYHASH:
    {
        if (sigversion == IsMineSigVersion::WITNESS_V0) {
            // P2WPKH inside P2WSH is invalid.
            return IsMineResult::INVALID;
        }
        if (sigversion == IsMineSigVersion::TOP && !keystore.HaveCScript(CScriptID(CScript() << OP_0 << vSolutions[0]))) {
            // We do not support bare witness outputs unless the P2SH version of it would be
            // acceptable as well. This protects against matching before segwit activates.
            // This also applies to the P2WSH case.
            break;
        }
        ret = std::max(ret, LegacyWalletIsMineInnerDONOTUSE(keystore, GetScriptForDestination(PKHash(uint160(vSolutions[0]))), IsMineSigVersion::WITNESS_V0));
        break;
    }
    case TxoutType::PUBKEYHASH:
        keyID = CKeyID(uint160(vSolutions[0]));
        if (!PermitsUncompressed(sigversion)) {
            CPubKey pubkey;
            if (keystore.GetPubKey(keyID, pubkey) && !pubkey.IsCompressed()) {
                return IsMineResult::INVALID;
            }
        }
        if (keystore.HaveKey(keyID)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    case TxoutType::SCRIPTHASH:
    {
        if (sigversion != IsMineSigVersion::TOP) {
            // P2SH inside P2WSH or P2SH is invalid.
            return IsMineResult::INVALID;
        }
        CScriptID scriptID = CScriptID(uint160(vSolutions[0]));
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            ret = std::max(ret, recurse_scripthash ? LegacyWalletIsMineInnerDONOTUSE(keystore, subscript, IsMineSigVersion::P2SH) : IsMineResult::SPENDABLE);
        }
        break;
    }
    case TxoutType::WITNESS_V0_SCRIPTHASH:
    {
        if (sigversion == IsMineSigVersion::WITNESS_V0) {
            // P2WSH inside P2WSH is invalid.
            return IsMineResult::INVALID;
        }
        if (sigversion == IsMineSigVersion::TOP && !keystore.HaveCScript(CScriptID(CScript() << OP_0 << vSolutions[0]))) {
            break;
        }
        CScriptID scriptID{RIPEMD160(vSolutions[0])};
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            ret = std::max(ret, recurse_scripthash ? LegacyWalletIsMineInnerDONOTUSE(keystore, subscript, IsMineSigVersion::WITNESS_V0) : IsMineResult::SPENDABLE);
        }
        break;
    }

    case TxoutType::MULTISIG:
    {
        // Never treat bare multisig outputs as ours (they can still be made watchonly-though)
        if (sigversion == IsMineSigVersion::TOP) {
            break;
        }

        // Only consider transactions "mine" if we own ALL the
        // keys involved. Multi-signature transactions that are
        // partially owned (somebody else has a key that can spend
        // them) enable spend-out-from-under-you attacks, especially
        // in shared-wallet situations.
        std::vector<valtype> keys(vSolutions.begin()+1, vSolutions.begin()+vSolutions.size()-1);
        if (!PermitsUncompressed(sigversion)) {
            for (size_t i = 0; i < keys.size(); i++) {
                if (keys[i].size() != 33) {
                    return IsMineResult::INVALID;
                }
            }
        }
        if (HaveKeys(keys, keystore)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    }
    } // no default case, so the compiler can warn about missing cases

    if (ret == IsMineResult::NO && keystore.HaveWatchOnly(scriptPubKey)) {
        ret = std::max(ret, IsMineResult::WATCH_ONLY);
    }
    return ret;
}

} // namespace

isminetype LegacyDataSPKM::IsMine(const CScript& script) const
{
    switch (LegacyWalletIsMineInnerDONOTUSE(*this, script, IsMineSigVersion::TOP)) {
    case IsMineResult::INVALID:
    case IsMineResult::NO:
        return ISMINE_NO;
    case IsMineResult::WATCH_ONLY:
    case IsMineResult::SPENDABLE:
        return ISMINE_SPENDABLE;
    }
    assert(false);
}

bool LegacyDataSPKM::CheckDecryptionKey(const CKeyingMaterial& master_key)
{
    {
        LOCK(cs_KeyStore);
        assert(mapKeys.empty());

        bool keyPass = mapCryptedKeys.empty(); // Always pass when there are no encrypted keys
        bool keyFail = false;
        CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
        WalletBatch batch(m_storage.GetDatabase());
        for (; mi != mapCryptedKeys.end(); ++mi)
        {
            const CPubKey &vchPubKey = (*mi).second.first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
            CKey key;
            if (!DecryptKey(master_key, vchCryptedSecret, vchPubKey, key))
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
        if (keyFail || !keyPass)
            return false;
        fDecryptionThoroughlyChecked = true;
    }
    return true;
}

std::unique_ptr<SigningProvider> LegacyDataSPKM::GetSolvingProvider(const CScript& script) const
{
    return std::make_unique<LegacySigningProvider>(*this);
}

bool LegacyDataSPKM::CanProvide(const CScript& script, SignatureData& sigdata)
{
    IsMineResult ismine = LegacyWalletIsMineInnerDONOTUSE(*this, script, IsMineSigVersion::TOP, /* recurse_scripthash= */ false);
    if (ismine == IsMineResult::SPENDABLE || ismine == IsMineResult::WATCH_ONLY) {
        // If ismine, it means we recognize keys or script ids in the script, or
        // are watching the script itself, and we can at least provide metadata
        // or solving information, even if not able to sign fully.
        return true;
    } else {
        // If, given the stuff in sigdata, we could make a valid signature, then we can provide for this script
        ProduceSignature(*this, DUMMY_SIGNATURE_CREATOR, script, sigdata);
        if (!sigdata.signatures.empty()) {
            // If we could make signatures, make sure we have a private key to actually make a signature
            bool has_privkeys = false;
            for (const auto& key_sig_pair : sigdata.signatures) {
                has_privkeys |= HaveKey(key_sig_pair.first);
            }
            return has_privkeys;
        }
        return false;
    }
}

bool LegacyDataSPKM::LoadKey(const CKey& key, const CPubKey &pubkey)
{
    return AddKeyPubKeyInner(key, pubkey);
}

bool LegacyDataSPKM::LoadCScript(const CScript& redeemScript)
{
    /* A sanity check was added in pull #3843 to avoid adding redeemScripts
     * that never can be redeemed. However, old wallets may still contain
     * these. Do not add them to the wallet and warn. */
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
    {
        std::string strAddr = EncodeDestination(ScriptHash(redeemScript));
        WalletLogPrintf("%s: Warning: This wallet contains a redeemScript of size %i which exceeds maximum size %i thus can never be redeemed. Do not use address %s.\n", __func__, redeemScript.size(), MAX_SCRIPT_ELEMENT_SIZE, strAddr);
        return true;
    }

    return FillableSigningProvider::AddCScript(redeemScript);
}

void LegacyDataSPKM::LoadKeyMetadata(const CKeyID& keyID, const CKeyMetadata& meta)
{
    LOCK(cs_KeyStore);
    mapKeyMetadata[keyID] = meta;
}

void LegacyDataSPKM::LoadScriptMetadata(const CScriptID& script_id, const CKeyMetadata& meta)
{
    LOCK(cs_KeyStore);
    m_script_metadata[script_id] = meta;
}

bool LegacyDataSPKM::AddKeyPubKeyInner(const CKey& key, const CPubKey& pubkey)
{
    LOCK(cs_KeyStore);
    return FillableSigningProvider::AddKeyPubKey(key, pubkey);
}

bool LegacyDataSPKM::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret, bool checksum_valid)
{
    // Set fDecryptionThoroughlyChecked to false when the checksum is invalid
    if (!checksum_valid) {
        fDecryptionThoroughlyChecked = false;
    }

    return AddCryptedKeyInner(vchPubKey, vchCryptedSecret);
}

bool LegacyDataSPKM::AddCryptedKeyInner(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    LOCK(cs_KeyStore);
    assert(mapKeys.empty());

    mapCryptedKeys[vchPubKey.GetID()] = make_pair(vchPubKey, vchCryptedSecret);
    ImplicitlyLearnRelatedKeyScripts(vchPubKey);
    return true;
}

bool LegacyDataSPKM::HaveWatchOnly(const CScript &dest) const
{
    LOCK(cs_KeyStore);
    return setWatchOnly.count(dest) > 0;
}

bool LegacyDataSPKM::LoadWatchOnly(const CScript &dest)
{
    return AddWatchOnlyInMem(dest);
}

static bool ExtractPubKey(const CScript &dest, CPubKey& pubKeyOut)
{
    std::vector<std::vector<unsigned char>> solutions;
    return Solver(dest, solutions) == TxoutType::PUBKEY &&
        (pubKeyOut = CPubKey(solutions[0])).IsFullyValid();
}

bool LegacyDataSPKM::AddWatchOnlyInMem(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.insert(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey)) {
        mapWatchKeys[pubKey.GetID()] = pubKey;
        ImplicitlyLearnRelatedKeyScripts(pubKey);
    }
    return true;
}

void LegacyDataSPKM::LoadHDChain(const CHDChain& chain)
{
    LOCK(cs_KeyStore);
    m_hd_chain = chain;
}

void LegacyDataSPKM::AddInactiveHDChain(const CHDChain& chain)
{
    LOCK(cs_KeyStore);
    assert(!chain.seed_id.IsNull());
    m_inactive_hd_chains[chain.seed_id] = chain;
}

bool LegacyDataSPKM::HaveKey(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return FillableSigningProvider::HaveKey(address);
    }
    return mapCryptedKeys.count(address) > 0;
}

bool LegacyDataSPKM::GetKey(const CKeyID &address, CKey& keyOut) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return FillableSigningProvider::GetKey(address, keyOut);
    }

    CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
    if (mi != mapCryptedKeys.end())
    {
        const CPubKey &vchPubKey = (*mi).second.first;
        const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
        return m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
            return DecryptKey(encryption_key, vchCryptedSecret, vchPubKey, keyOut);
        });
    }
    return false;
}

bool LegacyDataSPKM::GetKeyOrigin(const CKeyID& keyID, KeyOriginInfo& info) const
{
    CKeyMetadata meta;
    {
        LOCK(cs_KeyStore);
        auto it = mapKeyMetadata.find(keyID);
        if (it == mapKeyMetadata.end()) {
            return false;
        }
        meta = it->second;
    }
    if (meta.has_key_origin) {
        std::copy(meta.key_origin.fingerprint, meta.key_origin.fingerprint + 4, info.fingerprint);
        info.path = meta.key_origin.path;
    } else { // Single pubkeys get the master fingerprint of themselves
        std::copy(keyID.begin(), keyID.begin() + 4, info.fingerprint);
    }
    return true;
}

bool LegacyDataSPKM::GetWatchPubKey(const CKeyID &address, CPubKey &pubkey_out) const
{
    LOCK(cs_KeyStore);
    WatchKeyMap::const_iterator it = mapWatchKeys.find(address);
    if (it != mapWatchKeys.end()) {
        pubkey_out = it->second;
        return true;
    }
    return false;
}

bool LegacyDataSPKM::GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        if (!FillableSigningProvider::GetPubKey(address, vchPubKeyOut)) {
            return GetWatchPubKey(address, vchPubKeyOut);
        }
        return true;
    }

    CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
    if (mi != mapCryptedKeys.end())
    {
        vchPubKeyOut = (*mi).second.first;
        return true;
    }
    // Check for watch-only pubkeys
    return GetWatchPubKey(address, vchPubKeyOut);
}

std::unordered_set<CScript, SaltedSipHasher> LegacyDataSPKM::GetCandidateScriptPubKeys() const
{
    LOCK(cs_KeyStore);
    std::unordered_set<CScript, SaltedSipHasher> candidate_spks;

    // For every private key in the wallet, there should be a P2PK, P2PKH, P2WPKH, and P2SH-P2WPKH
    const auto& add_pubkey = [&candidate_spks](const CPubKey& pub) -> void {
        candidate_spks.insert(GetScriptForRawPubKey(pub));
        candidate_spks.insert(GetScriptForDestination(PKHash(pub)));

        CScript wpkh = GetScriptForDestination(WitnessV0KeyHash(pub));
        candidate_spks.insert(wpkh);
        candidate_spks.insert(GetScriptForDestination(ScriptHash(wpkh)));
    };
    for (const auto& [_, key] : mapKeys) {
        add_pubkey(key.GetPubKey());
    }
    for (const auto& [_, ckeypair] : mapCryptedKeys) {
        add_pubkey(ckeypair.first);
    }

    // mapScripts contains all redeemScripts and witnessScripts. Therefore each script in it has
    // itself, P2SH, P2WSH, and P2SH-P2WSH as a candidate.
    // Invalid scripts such as P2SH-P2SH and P2WSH-P2SH, among others, will be added as candidates.
    // Callers of this function will need to remove such scripts.
    const auto& add_script = [&candidate_spks](const CScript& script) -> void {
        candidate_spks.insert(script);
        candidate_spks.insert(GetScriptForDestination(ScriptHash(script)));

        CScript wsh = GetScriptForDestination(WitnessV0ScriptHash(script));
        candidate_spks.insert(wsh);
        candidate_spks.insert(GetScriptForDestination(ScriptHash(wsh)));
    };
    for (const auto& [_, script] : mapScripts) {
        add_script(script);
    }

    // Although setWatchOnly should only contain output scripts, we will also include each script's
    // P2SH, P2WSH, and P2SH-P2WSH as a precaution.
    for (const auto& script : setWatchOnly) {
        add_script(script);
    }

    return candidate_spks;
}

std::unordered_set<CScript, SaltedSipHasher> LegacyDataSPKM::GetScriptPubKeys() const
{
    // Run IsMine() on each candidate output script. Any script that is not ISMINE_NO is an output
    // script to return.
    // This both filters out things that are not watched by the wallet, and things that are invalid.
    std::unordered_set<CScript, SaltedSipHasher> spks;
    for (const CScript& script : GetCandidateScriptPubKeys()) {
        if (IsMine(script) != ISMINE_NO) {
            spks.insert(script);
        }
    }

    return spks;
}

std::unordered_set<CScript, SaltedSipHasher> LegacyDataSPKM::GetNotMineScriptPubKeys() const
{
    LOCK(cs_KeyStore);
    std::unordered_set<CScript, SaltedSipHasher> spks;
    for (const CScript& script : setWatchOnly) {
        if (IsMine(script) == ISMINE_NO) spks.insert(script);
    }
    return spks;
}

std::optional<MigrationData> LegacyDataSPKM::MigrateToDescriptor()
{
    LOCK(cs_KeyStore);
    if (m_storage.IsLocked()) {
        return std::nullopt;
    }

    MigrationData out;

    std::unordered_set<CScript, SaltedSipHasher> spks{GetScriptPubKeys()};

    // Get all key ids
    std::set<CKeyID> keyids;
    for (const auto& key_pair : mapKeys) {
        keyids.insert(key_pair.first);
    }
    for (const auto& key_pair : mapCryptedKeys) {
        keyids.insert(key_pair.first);
    }

    // Get key metadata and figure out which keys don't have a seed
    // Note that we do not ignore the seeds themselves because they are considered IsMine!
    for (auto keyid_it = keyids.begin(); keyid_it != keyids.end();) {
        const CKeyID& keyid = *keyid_it;
        const auto& it = mapKeyMetadata.find(keyid);
        if (it != mapKeyMetadata.end()) {
            const CKeyMetadata& meta = it->second;
            if (meta.hdKeypath == "s" || meta.hdKeypath == "m") {
                keyid_it++;
                continue;
            }
            if (!meta.hd_seed_id.IsNull() && (m_hd_chain.seed_id == meta.hd_seed_id || m_inactive_hd_chains.count(meta.hd_seed_id) > 0)) {
                keyid_it = keyids.erase(keyid_it);
                continue;
            }
        }
        keyid_it++;
    }

    WalletBatch batch(m_storage.GetDatabase());
    if (!batch.TxnBegin()) {
        LogPrintf("Error generating descriptors for migration, cannot initialize db transaction\n");
        return std::nullopt;
    }

    // keyids is now all non-HD keys. Each key will have its own combo descriptor
    for (const CKeyID& keyid : keyids) {
        CKey key;
        if (!GetKey(keyid, key)) {
            assert(false);
        }

        // Get birthdate from key meta
        uint64_t creation_time = 0;
        const auto& it = mapKeyMetadata.find(keyid);
        if (it != mapKeyMetadata.end()) {
            creation_time = it->second.nCreateTime;
        }

        // Get the key origin
        // Maybe this doesn't matter because floating keys here shouldn't have origins
        KeyOriginInfo info;
        bool has_info = GetKeyOrigin(keyid, info);
        std::string origin_str = has_info ? "[" + HexStr(info.fingerprint) + FormatHDKeypath(info.path) + "]" : "";

        // Construct the combo descriptor
        std::string desc_str = "combo(" + origin_str + HexStr(key.GetPubKey()) + ")";
        FlatSigningProvider keys;
        std::string error;
        std::vector<std::unique_ptr<Descriptor>> descs = Parse(desc_str, keys, error, false);
        CHECK_NONFATAL(descs.size() == 1); // It shouldn't be possible to have an invalid or multipath descriptor
        WalletDescriptor w_desc(std::move(descs.at(0)), creation_time, 0, 0, 0);

        // Make the DescriptorScriptPubKeyMan and get the scriptPubKeys
        auto desc_spk_man = std::make_unique<DescriptorScriptPubKeyMan>(m_storage, w_desc, /*keypool_size=*/0);
        WITH_LOCK(desc_spk_man->cs_desc_man, desc_spk_man->AddDescriptorKeyWithDB(batch, key, key.GetPubKey()));
        desc_spk_man->TopUpWithDB(batch);
        auto desc_spks = desc_spk_man->GetScriptPubKeys();

        // Remove the scriptPubKeys from our current set
        for (const CScript& spk : desc_spks) {
            size_t erased = spks.erase(spk);
            assert(erased == 1);
            assert(IsMine(spk) == ISMINE_SPENDABLE);
        }

        out.desc_spkms.push_back(std::move(desc_spk_man));
    }

    // Handle HD keys by using the CHDChains
    std::vector<CHDChain> chains;
    chains.push_back(m_hd_chain);
    for (const auto& chain_pair : m_inactive_hd_chains) {
        chains.push_back(chain_pair.second);
    }
    for (const CHDChain& chain : chains) {
        for (int i = 0; i < 2; ++i) {
            // Skip if doing internal chain and split chain is not supported
            if (chain.seed_id.IsNull() || (i == 1 && !m_storage.CanSupportFeature(FEATURE_HD_SPLIT))) {
                continue;
            }
            // Get the master xprv
            CKey seed_key;
            if (!GetKey(chain.seed_id, seed_key)) {
                assert(false);
            }
            CExtKey master_key;
            master_key.SetSeed(seed_key);

            // Make the combo descriptor
            std::string xpub = EncodeExtPubKey(master_key.Neuter());
            std::string desc_str = "combo(" + xpub + "/0h/" + ToString(i) + "h/*h)";
            FlatSigningProvider keys;
            std::string error;
            std::vector<std::unique_ptr<Descriptor>> descs = Parse(desc_str, keys, error, false);
            CHECK_NONFATAL(descs.size() == 1); // It shouldn't be possible to have an invalid or multipath descriptor
            uint32_t chain_counter = std::max((i == 1 ? chain.nInternalChainCounter : chain.nExternalChainCounter), (uint32_t)0);
            WalletDescriptor w_desc(std::move(descs.at(0)), 0, 0, chain_counter, 0);

            // Make the DescriptorScriptPubKeyMan and get the scriptPubKeys
            auto desc_spk_man = std::make_unique<DescriptorScriptPubKeyMan>(m_storage, w_desc, /*keypool_size=*/0);
            WITH_LOCK(desc_spk_man->cs_desc_man, desc_spk_man->AddDescriptorKeyWithDB(batch, master_key.key, master_key.key.GetPubKey()));
            desc_spk_man->TopUpWithDB(batch);
            auto desc_spks = desc_spk_man->GetScriptPubKeys();

            // Remove the scriptPubKeys from our current set
            for (const CScript& spk : desc_spks) {
                size_t erased = spks.erase(spk);
                assert(erased == 1);
                assert(IsMine(spk) == ISMINE_SPENDABLE);
            }

            out.desc_spkms.push_back(std::move(desc_spk_man));
        }
    }
    // Add the current master seed to the migration data
    if (!m_hd_chain.seed_id.IsNull()) {
        CKey seed_key;
        if (!GetKey(m_hd_chain.seed_id, seed_key)) {
            assert(false);
        }
        out.master_key.SetSeed(seed_key);
    }

    // Handle the rest of the scriptPubKeys which must be imports and may not have all info
    for (auto it = spks.begin(); it != spks.end();) {
        const CScript& spk = *it;

        // Get birthdate from script meta
        uint64_t creation_time = 0;
        const auto& mit = m_script_metadata.find(CScriptID(spk));
        if (mit != m_script_metadata.end()) {
            creation_time = mit->second.nCreateTime;
        }

        // InferDescriptor as that will get us all the solving info if it is there
        std::unique_ptr<Descriptor> desc = InferDescriptor(spk, *GetSolvingProvider(spk));

        // Past bugs in InferDescriptor have caused it to create descriptors which cannot be re-parsed.
        // Re-parse the descriptors to detect that, and skip any that do not parse.
        {
            std::string desc_str = desc->ToString();
            FlatSigningProvider parsed_keys;
            std::string parse_error;
            std::vector<std::unique_ptr<Descriptor>> parsed_descs = Parse(desc_str, parsed_keys, parse_error);
            if (parsed_descs.empty()) {
                // Remove this scriptPubKey from the set
                it = spks.erase(it);
                continue;
            }
        }

        // Get the private keys for this descriptor
        std::vector<CScript> scripts;
        FlatSigningProvider keys;
        if (!desc->Expand(0, DUMMY_SIGNING_PROVIDER, scripts, keys)) {
            assert(false);
        }
        std::set<CKeyID> privkeyids;
        for (const auto& key_orig_pair : keys.origins) {
            privkeyids.insert(key_orig_pair.first);
        }

        std::vector<CScript> desc_spks;

        // Make the descriptor string with private keys
        std::string desc_str;
        bool watchonly = !desc->ToPrivateString(*this, desc_str);
        if (watchonly && !m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
            out.watch_descs.emplace_back(desc->ToString(), creation_time);

            // Get the scriptPubKeys without writing this to the wallet
            FlatSigningProvider provider;
            desc->Expand(0, provider, desc_spks, provider);
        } else {
            // Make the DescriptorScriptPubKeyMan and get the scriptPubKeys
            WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
            auto desc_spk_man = std::make_unique<DescriptorScriptPubKeyMan>(m_storage, w_desc, /*keypool_size=*/0);
            for (const auto& keyid : privkeyids) {
                CKey key;
                if (!GetKey(keyid, key)) {
                    continue;
                }
                WITH_LOCK(desc_spk_man->cs_desc_man, desc_spk_man->AddDescriptorKeyWithDB(batch, key, key.GetPubKey()));
            }
            desc_spk_man->TopUpWithDB(batch);
            auto desc_spks_set = desc_spk_man->GetScriptPubKeys();
            desc_spks.insert(desc_spks.end(), desc_spks_set.begin(), desc_spks_set.end());

            out.desc_spkms.push_back(std::move(desc_spk_man));
        }

        // Remove the scriptPubKeys from our current set
        for (const CScript& desc_spk : desc_spks) {
            auto del_it = spks.find(desc_spk);
            assert(del_it != spks.end());
            assert(IsMine(desc_spk) != ISMINE_NO);
            it = spks.erase(del_it);
        }
    }

    // Make sure that we have accounted for all scriptPubKeys
    if (!Assume(spks.empty())) {
        LogPrintf("%s\n", STR_INTERNAL_BUG("Error: Some output scripts were not migrated.\n"));
        return std::nullopt;
    }

    // Legacy wallets can also contain scripts whose P2SH, P2WSH, or P2SH-P2WSH it is not watching for
    // but can provide script data to a PSBT spending them. These "solvable" output scripts will need to
    // be put into the separate "solvables" wallet.
    // These can be detected by going through the entire candidate output scripts, finding the ISMINE_NO scripts,
    // and checking CanProvide() which will dummy sign.
    for (const CScript& script : GetCandidateScriptPubKeys()) {
        // Since we only care about P2SH, P2WSH, and P2SH-P2WSH, filter out any scripts that are not those
        if (!script.IsPayToScriptHash() && !script.IsPayToWitnessScriptHash()) {
            continue;
        }
        if (IsMine(script) != ISMINE_NO) {
            continue;
        }
        SignatureData dummy_sigdata;
        if (!CanProvide(script, dummy_sigdata)) {
            continue;
        }

        // Get birthdate from script meta
        uint64_t creation_time = 0;
        const auto& it = m_script_metadata.find(CScriptID(script));
        if (it != m_script_metadata.end()) {
            creation_time = it->second.nCreateTime;
        }

        // InferDescriptor as that will get us all the solving info if it is there
        std::unique_ptr<Descriptor> desc = InferDescriptor(script, *GetSolvingProvider(script));
        if (!desc->IsSolvable()) {
            // The wallet was able to provide some information, but not enough to make a descriptor that actually
            // contains anything useful. This is probably because the script itself is actually unsignable (e.g. P2WSH-P2WSH).
            continue;
        }

        // Past bugs in InferDescriptor have caused it to create descriptors which cannot be re-parsed
        // Re-parse the descriptors to detect that, and skip any that do not parse.
        {
            std::string desc_str = desc->ToString();
            FlatSigningProvider parsed_keys;
            std::string parse_error;
            std::vector<std::unique_ptr<Descriptor>> parsed_descs = Parse(desc_str, parsed_keys, parse_error, false);
            if (parsed_descs.empty()) {
                continue;
            }
        }

        out.solvable_descs.emplace_back(desc->ToString(), creation_time);
    }

    // Finalize transaction
    if (!batch.TxnCommit()) {
        LogPrintf("Error generating descriptors for migration, cannot commit db transaction\n");
        return std::nullopt;
    }

    return out;
}

bool LegacyDataSPKM::DeleteRecordsWithDB(WalletBatch& batch)
{
    LOCK(cs_KeyStore);
    return batch.EraseRecords(DBKeys::LEGACY_TYPES);
}

util::Result<CTxDestination> DescriptorScriptPubKeyMan::GetNewDestination(const OutputType type)
{
    // Returns true if this descriptor supports getting new addresses. Conditions where we may be unable to fetch them (e.g. locked) are caught later
    if (!CanGetAddresses()) {
        return util::Error{_("No addresses available")};
    }
    {
        LOCK(cs_desc_man);
        assert(m_wallet_descriptor.descriptor->IsSingleType()); // This is a combo descriptor which should not be an active descriptor
        std::optional<OutputType> desc_addr_type = m_wallet_descriptor.descriptor->GetOutputType();
        assert(desc_addr_type);
        if (type != *desc_addr_type) {
            throw std::runtime_error(std::string(__func__) + ": Types are inconsistent. Stored type does not match type of newly generated address");
        }

        TopUp();

        // Get the scriptPubKey from the descriptor
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        if (m_wallet_descriptor.range_end <= m_max_cached_index && !TopUp(1)) {
            // We can't generate anymore keys
            return util::Error{_("Error: Keypool ran out, please call keypoolrefill first")};
        }
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(m_wallet_descriptor.next_index, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            // We can't generate anymore keys
            return util::Error{_("Error: Keypool ran out, please call keypoolrefill first")};
        }

        CTxDestination dest;
        if (!ExtractDestination(scripts_temp[0], dest)) {
            return util::Error{_("Error: Cannot extract destination from the generated scriptpubkey")}; // shouldn't happen
        }
        m_wallet_descriptor.next_index++;
        WalletBatch(m_storage.GetDatabase()).WriteDescriptor(GetID(), m_wallet_descriptor);
        return dest;
    }
}

isminetype DescriptorScriptPubKeyMan::IsMine(const CScript& script) const
{
    LOCK(cs_desc_man);
    if (m_map_script_pub_keys.count(script) > 0) {
        return ISMINE_SPENDABLE;
    }
    return ISMINE_NO;
}

bool DescriptorScriptPubKeyMan::CheckDecryptionKey(const CKeyingMaterial& master_key)
{
    LOCK(cs_desc_man);
    if (!m_map_keys.empty()) {
        return false;
    }

    bool keyPass = m_map_crypted_keys.empty(); // Always pass when there are no encrypted keys
    bool keyFail = false;
    for (const auto& mi : m_map_crypted_keys) {
        const CPubKey &pubkey = mi.second.first;
        const std::vector<unsigned char> &crypted_secret = mi.second.second;
        CKey key;
        if (!DecryptKey(master_key, crypted_secret, pubkey, key)) {
            keyFail = true;
            break;
        }
        keyPass = true;
        if (m_decryption_thoroughly_checked)
            break;
    }
    if (keyPass && keyFail) {
        LogPrintf("The wallet is probably corrupted: Some keys decrypt but not all.\n");
        throw std::runtime_error("Error unlocking wallet: some keys decrypt but not all. Your wallet file may be corrupt.");
    }
    if (keyFail || !keyPass) {
        return false;
    }
    m_decryption_thoroughly_checked = true;
    return true;
}

bool DescriptorScriptPubKeyMan::Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch)
{
    LOCK(cs_desc_man);
    if (!m_map_crypted_keys.empty()) {
        return false;
    }

    for (const KeyMap::value_type& key_in : m_map_keys)
    {
        const CKey &key = key_in.second;
        CPubKey pubkey = key.GetPubKey();
        CKeyingMaterial secret{UCharCast(key.begin()), UCharCast(key.end())};
        std::vector<unsigned char> crypted_secret;
        if (!EncryptSecret(master_key, secret, pubkey.GetHash(), crypted_secret)) {
            return false;
        }
        m_map_crypted_keys[pubkey.GetID()] = make_pair(pubkey, crypted_secret);
        batch->WriteCryptedDescriptorKey(GetID(), pubkey, crypted_secret);
    }
    m_map_keys.clear();
    return true;
}

util::Result<CTxDestination> DescriptorScriptPubKeyMan::GetReservedDestination(const OutputType type, bool internal, int64_t& index)
{
    LOCK(cs_desc_man);
    auto op_dest = GetNewDestination(type);
    index = m_wallet_descriptor.next_index - 1;
    return op_dest;
}

void DescriptorScriptPubKeyMan::ReturnDestination(int64_t index, bool internal, const CTxDestination& addr)
{
    LOCK(cs_desc_man);
    // Only return when the index was the most recent
    if (m_wallet_descriptor.next_index - 1 == index) {
        m_wallet_descriptor.next_index--;
    }
    WalletBatch(m_storage.GetDatabase()).WriteDescriptor(GetID(), m_wallet_descriptor);
    NotifyCanGetAddressesChanged();
}

std::map<CKeyID, CKey> DescriptorScriptPubKeyMan::GetKeys() const
{
    AssertLockHeld(cs_desc_man);
    if (m_storage.HasEncryptionKeys() && !m_storage.IsLocked()) {
        KeyMap keys;
        for (const auto& key_pair : m_map_crypted_keys) {
            const CPubKey& pubkey = key_pair.second.first;
            const std::vector<unsigned char>& crypted_secret = key_pair.second.second;
            CKey key;
            m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
                return DecryptKey(encryption_key, crypted_secret, pubkey, key);
            });
            keys[pubkey.GetID()] = key;
        }
        return keys;
    }
    return m_map_keys;
}

bool DescriptorScriptPubKeyMan::HasPrivKey(const CKeyID& keyid) const
{
    AssertLockHeld(cs_desc_man);
    return m_map_keys.contains(keyid) || m_map_crypted_keys.contains(keyid);
}

std::optional<CKey> DescriptorScriptPubKeyMan::GetKey(const CKeyID& keyid) const
{
    AssertLockHeld(cs_desc_man);
    if (m_storage.HasEncryptionKeys() && !m_storage.IsLocked()) {
        const auto& it = m_map_crypted_keys.find(keyid);
        if (it == m_map_crypted_keys.end()) {
            return std::nullopt;
        }
        const std::vector<unsigned char>& crypted_secret = it->second.second;
        CKey key;
        if (!Assume(m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
            return DecryptKey(encryption_key, crypted_secret, it->second.first, key);
        }))) {
            return std::nullopt;
        }
        return key;
    }
    const auto& it = m_map_keys.find(keyid);
    if (it == m_map_keys.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool DescriptorScriptPubKeyMan::TopUp(unsigned int size)
{
    WalletBatch batch(m_storage.GetDatabase());
    if (!batch.TxnBegin()) return false;
    bool res = TopUpWithDB(batch, size);
    if (!batch.TxnCommit()) throw std::runtime_error(strprintf("Error during descriptors keypool top up. Cannot commit changes for wallet %s", m_storage.GetDisplayName()));
    return res;
}

bool DescriptorScriptPubKeyMan::TopUpWithDB(WalletBatch& batch, unsigned int size)
{
    LOCK(cs_desc_man);
    std::set<CScript> new_spks;
    unsigned int target_size;
    if (size > 0) {
        target_size = size;
    } else {
        target_size = m_keypool_size;
    }

    // Calculate the new range_end
    int32_t new_range_end = std::max(m_wallet_descriptor.next_index + (int32_t)target_size, m_wallet_descriptor.range_end);

    // If the descriptor is not ranged, we actually just want to fill the first cache item
    if (!m_wallet_descriptor.descriptor->IsRange()) {
        new_range_end = 1;
        m_wallet_descriptor.range_end = 1;
        m_wallet_descriptor.range_start = 0;
    }

    FlatSigningProvider provider;
    provider.keys = GetKeys();

    uint256 id = GetID();
    for (int32_t i = m_max_cached_index + 1; i < new_range_end; ++i) {
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        DescriptorCache temp_cache;
        // Maybe we have a cached xpub and we can expand from the cache first
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(i, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            if (!m_wallet_descriptor.descriptor->Expand(i, provider, scripts_temp, out_keys, &temp_cache)) return false;
        }
        // Add all of the scriptPubKeys to the scriptPubKey set
        new_spks.insert(scripts_temp.begin(), scripts_temp.end());
        for (const CScript& script : scripts_temp) {
            m_map_script_pub_keys[script] = i;
        }
        for (const auto& pk_pair : out_keys.pubkeys) {
            const CPubKey& pubkey = pk_pair.second;
            if (m_map_pubkeys.count(pubkey) != 0) {
                // We don't need to give an error here.
                // It doesn't matter which of many valid indexes the pubkey has, we just need an index where we can derive it and its private key
                continue;
            }
            m_map_pubkeys[pubkey] = i;
        }
        // Merge and write the cache
        DescriptorCache new_items = m_wallet_descriptor.cache.MergeAndDiff(temp_cache);
        if (!batch.WriteDescriptorCacheItems(id, new_items)) {
            throw std::runtime_error(std::string(__func__) + ": writing cache items failed");
        }
        m_max_cached_index++;
    }
    m_wallet_descriptor.range_end = new_range_end;
    batch.WriteDescriptor(GetID(), m_wallet_descriptor);

    // By this point, the cache size should be the size of the entire range
    assert(m_wallet_descriptor.range_end - 1 == m_max_cached_index);

    m_storage.TopUpCallback(new_spks, this);
    NotifyCanGetAddressesChanged();
    return true;
}

std::vector<WalletDestination> DescriptorScriptPubKeyMan::MarkUnusedAddresses(const CScript& script)
{
    LOCK(cs_desc_man);
    std::vector<WalletDestination> result;
    if (IsMine(script)) {
        int32_t index = m_map_script_pub_keys[script];
        if (index >= m_wallet_descriptor.next_index) {
            WalletLogPrintf("%s: Detected a used keypool item at index %d, mark all keypool items up to this item as used\n", __func__, index);
            auto out_keys = std::make_unique<FlatSigningProvider>();
            std::vector<CScript> scripts_temp;
            while (index >= m_wallet_descriptor.next_index) {
                if (!m_wallet_descriptor.descriptor->ExpandFromCache(m_wallet_descriptor.next_index, m_wallet_descriptor.cache, scripts_temp, *out_keys)) {
                    throw std::runtime_error(std::string(__func__) + ": Unable to expand descriptor from cache");
                }
                CTxDestination dest;
                ExtractDestination(scripts_temp[0], dest);
                result.push_back({dest, std::nullopt});
                m_wallet_descriptor.next_index++;
            }
        }
        if (!TopUp()) {
            WalletLogPrintf("%s: Topping up keypool failed (locked wallet)\n", __func__);
        }
    }

    return result;
}

void DescriptorScriptPubKeyMan::AddDescriptorKey(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_desc_man);
    WalletBatch batch(m_storage.GetDatabase());
    if (!AddDescriptorKeyWithDB(batch, key, pubkey)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor private key failed");
    }
}

bool DescriptorScriptPubKeyMan::AddDescriptorKeyWithDB(WalletBatch& batch, const CKey& key, const CPubKey &pubkey)
{
    AssertLockHeld(cs_desc_man);
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));

    // Check if provided key already exists
    if (m_map_keys.find(pubkey.GetID()) != m_map_keys.end() ||
        m_map_crypted_keys.find(pubkey.GetID()) != m_map_crypted_keys.end()) {
        return true;
    }

    if (m_storage.HasEncryptionKeys()) {
        if (m_storage.IsLocked()) {
            return false;
        }

        std::vector<unsigned char> crypted_secret;
        CKeyingMaterial secret{UCharCast(key.begin()), UCharCast(key.end())};
        if (!m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
                return EncryptSecret(encryption_key, secret, pubkey.GetHash(), crypted_secret);
            })) {
            return false;
        }

        m_map_crypted_keys[pubkey.GetID()] = make_pair(pubkey, crypted_secret);
        return batch.WriteCryptedDescriptorKey(GetID(), pubkey, crypted_secret);
    } else {
        m_map_keys[pubkey.GetID()] = key;
        return batch.WriteDescriptorKey(GetID(), pubkey, key.GetPrivKey());
    }
}

bool DescriptorScriptPubKeyMan::SetupDescriptorGeneration(WalletBatch& batch, const CExtKey& master_key, OutputType addr_type, bool internal)
{
    LOCK(cs_desc_man);
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));

    // Ignore when there is already a descriptor
    if (m_wallet_descriptor.descriptor) {
        return false;
    }

    m_wallet_descriptor = GenerateWalletDescriptor(master_key.Neuter(), addr_type, internal);

    // Store the master private key, and descriptor
    if (!AddDescriptorKeyWithDB(batch, master_key.key, master_key.key.GetPubKey())) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor master private key failed");
    }
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }

    // TopUp
    TopUpWithDB(batch);

    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}

bool DescriptorScriptPubKeyMan::IsHDEnabled() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.descriptor->IsRange();
}

bool DescriptorScriptPubKeyMan::CanGetAddresses(bool internal) const
{
    // We can only give out addresses from descriptors that are single type (not combo), ranged,
    // and either have cached keys or can generate more keys (ignoring encryption)
    LOCK(cs_desc_man);
    return m_wallet_descriptor.descriptor->IsSingleType() &&
           m_wallet_descriptor.descriptor->IsRange() &&
           (HavePrivateKeys() || m_wallet_descriptor.next_index < m_wallet_descriptor.range_end);
}

bool DescriptorScriptPubKeyMan::HavePrivateKeys() const
{
    LOCK(cs_desc_man);
    return m_map_keys.size() > 0 || m_map_crypted_keys.size() > 0;
}

bool DescriptorScriptPubKeyMan::HaveCryptedKeys() const
{
    LOCK(cs_desc_man);
    return !m_map_crypted_keys.empty();
}

unsigned int DescriptorScriptPubKeyMan::GetKeyPoolSize() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.range_end - m_wallet_descriptor.next_index;
}

int64_t DescriptorScriptPubKeyMan::GetTimeFirstKey() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.creation_time;
}

std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(const CScript& script, bool include_private) const
{
    LOCK(cs_desc_man);

    // Find the index of the script
    auto it = m_map_script_pub_keys.find(script);
    if (it == m_map_script_pub_keys.end()) {
        return nullptr;
    }
    int32_t index = it->second;

    return GetSigningProvider(index, include_private);
}

std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(const CPubKey& pubkey) const
{
    LOCK(cs_desc_man);

    // Find index of the pubkey
    auto it = m_map_pubkeys.find(pubkey);
    if (it == m_map_pubkeys.end()) {
        return nullptr;
    }
    int32_t index = it->second;

    // Always try to get the signing provider with private keys. This function should only be called during signing anyways
    std::unique_ptr<FlatSigningProvider> out = GetSigningProvider(index, true);
    if (!out->HaveKey(pubkey.GetID())) {
        return nullptr;
    }
    return out;
}

std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(int32_t index, bool include_private) const
{
    AssertLockHeld(cs_desc_man);

    std::unique_ptr<FlatSigningProvider> out_keys = std::make_unique<FlatSigningProvider>();

    // Fetch SigningProvider from cache to avoid re-deriving
    auto it = m_map_signing_providers.find(index);
    if (it != m_map_signing_providers.end()) {
        out_keys->Merge(FlatSigningProvider{it->second});
    } else {
        // Get the scripts, keys, and key origins for this script
        std::vector<CScript> scripts_temp;
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(index, m_wallet_descriptor.cache, scripts_temp, *out_keys)) return nullptr;

        // Cache SigningProvider so we don't need to re-derive if we need this SigningProvider again
        m_map_signing_providers[index] = *out_keys;
    }

    if (HavePrivateKeys() && include_private) {
        FlatSigningProvider master_provider;
        master_provider.keys = GetKeys();
        m_wallet_descriptor.descriptor->ExpandPrivate(index, master_provider, *out_keys);
    }

    return out_keys;
}

std::unique_ptr<SigningProvider> DescriptorScriptPubKeyMan::GetSolvingProvider(const CScript& script) const
{
    return GetSigningProvider(script, false);
}

bool DescriptorScriptPubKeyMan::CanProvide(const CScript& script, SignatureData& sigdata)
{
    return IsMine(script);
}

bool DescriptorScriptPubKeyMan::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const
{
    std::unique_ptr<FlatSigningProvider> keys = std::make_unique<FlatSigningProvider>();
    for (const auto& coin_pair : coins) {
        std::unique_ptr<FlatSigningProvider> coin_keys = GetSigningProvider(coin_pair.second.out.scriptPubKey, true);
        if (!coin_keys) {
            continue;
        }
        keys->Merge(std::move(*coin_keys));
    }

    return ::SignTransaction(tx, keys.get(), coins, sighash, input_errors);
}

SigningResult DescriptorScriptPubKeyMan::SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const
{
    std::unique_ptr<FlatSigningProvider> keys = GetSigningProvider(GetScriptForDestination(pkhash), true);
    if (!keys) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }

    CKey key;
    if (!keys->GetKey(ToKeyID(pkhash), key)) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }

    if (!MessageSign(key, message, str_sig)) {
        return SigningResult::SIGNING_FAILED;
    }
    return SigningResult::OK;
}

std::optional<PSBTError> DescriptorScriptPubKeyMan::FillPSBT(PartiallySignedTransaction& psbtx, const PrecomputedTransactionData& txdata, std::optional<int> sighash_type, bool sign, bool bip32derivs, int* n_signed, bool finalize) const
{
    if (n_signed) {
        *n_signed = 0;
    }
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // Get the scriptPubKey to know which SigningProvider to use
        CScript script;
        if (!input.witness_utxo.IsNull()) {
            script = input.witness_utxo.scriptPubKey;
        } else if (input.non_witness_utxo) {
            if (txin.prevout.n >= input.non_witness_utxo->vout.size()) {
                return PSBTError::MISSING_INPUTS;
            }
            script = input.non_witness_utxo->vout[txin.prevout.n].scriptPubKey;
        } else {
            // There's no UTXO so we can just skip this now
            continue;
        }

        std::unique_ptr<FlatSigningProvider> keys = std::make_unique<FlatSigningProvider>();
        std::unique_ptr<FlatSigningProvider> script_keys = GetSigningProvider(script, /*include_private=*/sign);
        if (script_keys) {
            keys->Merge(std::move(*script_keys));
        } else {
            // Maybe there are pubkeys listed that we can sign for
            std::vector<CPubKey> pubkeys;
            pubkeys.reserve(input.hd_keypaths.size() + 2);

            // ECDSA Pubkeys
            for (const auto& [pk, _] : input.hd_keypaths) {
                pubkeys.push_back(pk);
            }

            // Taproot output pubkey
            std::vector<std::vector<unsigned char>> sols;
            if (Solver(script, sols) == TxoutType::WITNESS_V1_TAPROOT) {
                sols[0].insert(sols[0].begin(), 0x02);
                pubkeys.emplace_back(sols[0]);
                sols[0][0] = 0x03;
                pubkeys.emplace_back(sols[0]);
            }

            // Taproot pubkeys
            for (const auto& pk_pair : input.m_tap_bip32_paths) {
                const XOnlyPubKey& pubkey = pk_pair.first;
                for (unsigned char prefix : {0x02, 0x03}) {
                    unsigned char b[33] = {prefix};
                    std::copy(pubkey.begin(), pubkey.end(), b + 1);
                    CPubKey fullpubkey;
                    fullpubkey.Set(b, b + 33);
                    pubkeys.push_back(fullpubkey);
                }
            }

            for (const auto& pubkey : pubkeys) {
                std::unique_ptr<FlatSigningProvider> pk_keys = GetSigningProvider(pubkey);
                if (pk_keys) {
                    keys->Merge(std::move(*pk_keys));
                }
            }
        }

        PSBTError res = SignPSBTInput(HidingSigningProvider(keys.get(), /*hide_secret=*/!sign, /*hide_origin=*/!bip32derivs), psbtx, i, &txdata, sighash_type, nullptr, finalize);
        if (res != PSBTError::OK && res != PSBTError::INCOMPLETE) {
            return res;
        }

        bool signed_one = PSBTInputSigned(input);
        if (n_signed && (signed_one || !sign)) {
            // If sign is false, we assume that we _could_ sign if we get here. This
            // will never have false negatives; it is hard to tell under what i
            // circumstances it could have false positives.
            (*n_signed)++;
        }
    }

    // Fill in the bip32 keypaths and redeemscripts for the outputs so that hardware wallets can identify change
    for (unsigned int i = 0; i < psbtx.tx->vout.size(); ++i) {
        std::unique_ptr<SigningProvider> keys = GetSolvingProvider(psbtx.tx->vout.at(i).scriptPubKey);
        if (!keys) {
            continue;
        }
        UpdatePSBTOutput(HidingSigningProvider(keys.get(), /*hide_secret=*/true, /*hide_origin=*/!bip32derivs), psbtx, i);
    }

    return {};
}

std::unique_ptr<CKeyMetadata> DescriptorScriptPubKeyMan::GetMetadata(const CTxDestination& dest) const
{
    std::unique_ptr<SigningProvider> provider = GetSigningProvider(GetScriptForDestination(dest));
    if (provider) {
        KeyOriginInfo orig;
        CKeyID key_id = GetKeyForDestination(*provider, dest);
        if (provider->GetKeyOrigin(key_id, orig)) {
            LOCK(cs_desc_man);
            std::unique_ptr<CKeyMetadata> meta = std::make_unique<CKeyMetadata>();
            meta->key_origin = orig;
            meta->has_key_origin = true;
            meta->nCreateTime = m_wallet_descriptor.creation_time;
            return meta;
        }
    }
    return nullptr;
}

uint256 DescriptorScriptPubKeyMan::GetID() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.id;
}

void DescriptorScriptPubKeyMan::SetCache(const DescriptorCache& cache)
{
    LOCK(cs_desc_man);
    std::set<CScript> new_spks;
    m_wallet_descriptor.cache = cache;
    for (int32_t i = m_wallet_descriptor.range_start; i < m_wallet_descriptor.range_end; ++i) {
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(i, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            throw std::runtime_error("Error: Unable to expand wallet descriptor from cache");
        }
        // Add all of the scriptPubKeys to the scriptPubKey set
        new_spks.insert(scripts_temp.begin(), scripts_temp.end());
        for (const CScript& script : scripts_temp) {
            if (m_map_script_pub_keys.count(script) != 0) {
                throw std::runtime_error(strprintf("Error: Already loaded script at index %d as being at index %d", i, m_map_script_pub_keys[script]));
            }
            m_map_script_pub_keys[script] = i;
        }
        for (const auto& pk_pair : out_keys.pubkeys) {
            const CPubKey& pubkey = pk_pair.second;
            if (m_map_pubkeys.count(pubkey) != 0) {
                // We don't need to give an error here.
                // It doesn't matter which of many valid indexes the pubkey has, we just need an index where we can derive it and its private key
                continue;
            }
            m_map_pubkeys[pubkey] = i;
        }
        m_max_cached_index++;
    }
    // Make sure the wallet knows about our new spks
    m_storage.TopUpCallback(new_spks, this);
}

bool DescriptorScriptPubKeyMan::AddKey(const CKeyID& key_id, const CKey& key)
{
    LOCK(cs_desc_man);
    m_map_keys[key_id] = key;
    return true;
}

bool DescriptorScriptPubKeyMan::AddCryptedKey(const CKeyID& key_id, const CPubKey& pubkey, const std::vector<unsigned char>& crypted_key)
{
    LOCK(cs_desc_man);
    if (!m_map_keys.empty()) {
        return false;
    }

    m_map_crypted_keys[key_id] = make_pair(pubkey, crypted_key);
    return true;
}

bool DescriptorScriptPubKeyMan::HasWalletDescriptor(const WalletDescriptor& desc) const
{
    LOCK(cs_desc_man);
    return !m_wallet_descriptor.id.IsNull() && !desc.id.IsNull() && m_wallet_descriptor.id == desc.id;
}

void DescriptorScriptPubKeyMan::WriteDescriptor()
{
    LOCK(cs_desc_man);
    WalletBatch batch(m_storage.GetDatabase());
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }
}

WalletDescriptor DescriptorScriptPubKeyMan::GetWalletDescriptor() const
{
    return m_wallet_descriptor;
}

std::unordered_set<CScript, SaltedSipHasher> DescriptorScriptPubKeyMan::GetScriptPubKeys() const
{
    return GetScriptPubKeys(0);
}

std::unordered_set<CScript, SaltedSipHasher> DescriptorScriptPubKeyMan::GetScriptPubKeys(int32_t minimum_index) const
{
    LOCK(cs_desc_man);
    std::unordered_set<CScript, SaltedSipHasher> script_pub_keys;
    script_pub_keys.reserve(m_map_script_pub_keys.size());

    for (auto const& [script_pub_key, index] : m_map_script_pub_keys) {
        if (index >= minimum_index) script_pub_keys.insert(script_pub_key);
    }
    return script_pub_keys;
}

int32_t DescriptorScriptPubKeyMan::GetEndRange() const
{
    return m_max_cached_index + 1;
}

bool DescriptorScriptPubKeyMan::GetDescriptorString(std::string& out, const bool priv) const
{
    LOCK(cs_desc_man);

    FlatSigningProvider provider;
    provider.keys = GetKeys();

    if (priv) {
        // For the private version, always return the master key to avoid
        // exposing child private keys. The risk implications of exposing child
        // private keys together with the parent xpub may be non-obvious for users.
        return m_wallet_descriptor.descriptor->ToPrivateString(provider, out);
    }

    return m_wallet_descriptor.descriptor->ToNormalizedString(provider, out, &m_wallet_descriptor.cache);
}

void DescriptorScriptPubKeyMan::UpgradeDescriptorCache()
{
    LOCK(cs_desc_man);
    if (m_storage.IsLocked() || m_storage.IsWalletFlagSet(WALLET_FLAG_LAST_HARDENED_XPUB_CACHED)) {
        return;
    }

    // Skip if we have the last hardened xpub cache
    if (m_wallet_descriptor.cache.GetCachedLastHardenedExtPubKeys().size() > 0) {
        return;
    }

    // Expand the descriptor
    FlatSigningProvider provider;
    provider.keys = GetKeys();
    FlatSigningProvider out_keys;
    std::vector<CScript> scripts_temp;
    DescriptorCache temp_cache;
    if (!m_wallet_descriptor.descriptor->Expand(0, provider, scripts_temp, out_keys, &temp_cache)){
        throw std::runtime_error("Unable to expand descriptor");
    }

    // Cache the last hardened xpubs
    DescriptorCache diff = m_wallet_descriptor.cache.MergeAndDiff(temp_cache);
    if (!WalletBatch(m_storage.GetDatabase()).WriteDescriptorCacheItems(GetID(), diff)) {
        throw std::runtime_error(std::string(__func__) + ": writing cache items failed");
    }
}

util::Result<void> DescriptorScriptPubKeyMan::UpdateWalletDescriptor(WalletDescriptor& descriptor)
{
    LOCK(cs_desc_man);
    std::string error;
    if (!CanUpdateToWalletDescriptor(descriptor, error)) {
        return util::Error{Untranslated(std::move(error))};
    }

    m_map_pubkeys.clear();
    m_map_script_pub_keys.clear();
    m_max_cached_index = -1;
    m_wallet_descriptor = descriptor;

    NotifyFirstKeyTimeChanged(this, m_wallet_descriptor.creation_time);
    return {};
}

bool DescriptorScriptPubKeyMan::CanUpdateToWalletDescriptor(const WalletDescriptor& descriptor, std::string& error)
{
    LOCK(cs_desc_man);
    if (!HasWalletDescriptor(descriptor)) {
        error = "can only update matching descriptor";
        return false;
    }

    if (!descriptor.descriptor->IsRange()) {
        // Skip range check for non-range descriptors
        return true;
    }

    if (descriptor.range_start > m_wallet_descriptor.range_start ||
        descriptor.range_end < m_wallet_descriptor.range_end) {
        // Use inclusive range for error
        error = strprintf("new range must include current range = [%d,%d]",
                          m_wallet_descriptor.range_start,
                          m_wallet_descriptor.range_end - 1);
        return false;
    }

    return true;
}
} // namespace wallet
