// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/sign.h>

#include <consensus/amount.h>
#include <key.h>
#include <musig.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/keyorigin.h>
#include <script/miniscript.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <uint256.h>
#include <util/translation.h>
#include <util/vector.h>

typedef std::vector<unsigned char> valtype;

MutableTransactionSignatureCreator::MutableTransactionSignatureCreator(const CMutableTransaction& tx, unsigned int input_idx, const CAmount& amount, int hash_type)
    : m_txto{tx}, nIn{input_idx}, nHashType{hash_type}, amount{amount}, checker{&m_txto, nIn, amount, MissingDataBehavior::FAIL},
      m_txdata(nullptr)
{
}

MutableTransactionSignatureCreator::MutableTransactionSignatureCreator(const CMutableTransaction& tx, unsigned int input_idx, const CAmount& amount, const PrecomputedTransactionData* txdata, int hash_type)
    : m_txto{tx}, nIn{input_idx}, nHashType{hash_type}, amount{amount},
      checker{txdata ? MutableTransactionSignatureChecker{&m_txto, nIn, amount, *txdata, MissingDataBehavior::FAIL} :
                       MutableTransactionSignatureChecker{&m_txto, nIn, amount, MissingDataBehavior::FAIL}},
      m_txdata(txdata)
{
}

bool MutableTransactionSignatureCreator::CreateSig(const SigningProvider& provider, std::vector<unsigned char>& vchSig, const CKeyID& address, const CScript& scriptCode, SigVersion sigversion) const
{
    assert(sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0);

    CKey key;
    if (!provider.GetKey(address, key))
        return false;

    // Signing with uncompressed keys is disabled in witness scripts
    if (sigversion == SigVersion::WITNESS_V0 && !key.IsCompressed())
        return false;

    // Signing without known amount does not work in witness scripts.
    if (sigversion == SigVersion::WITNESS_V0 && !MoneyRange(amount)) return false;

    // BASE/WITNESS_V0 signatures don't support explicit SIGHASH_DEFAULT, use SIGHASH_ALL instead.
    const int hashtype = nHashType == SIGHASH_DEFAULT ? SIGHASH_ALL : nHashType;

    uint256 hash = SignatureHash(scriptCode, m_txto, nIn, hashtype, amount, sigversion, m_txdata);
    if (!key.Sign(hash, vchSig))
        return false;
    vchSig.push_back((unsigned char)hashtype);
    return true;
}

std::optional<uint256> MutableTransactionSignatureCreator::ComputeSchnorrSignatureHash(const uint256* leaf_hash, SigVersion sigversion) const
{
    assert(sigversion == SigVersion::TAPROOT || sigversion == SigVersion::TAPSCRIPT);

    // BIP341/BIP342 signing needs lots of precomputed transaction data. While some
    // (non-SIGHASH_DEFAULT) sighash modes exist that can work with just some subset
    // of data present, for now, only support signing when everything is provided.
    if (!m_txdata || !m_txdata->m_bip341_taproot_ready || !m_txdata->m_spent_outputs_ready) return std::nullopt;

    ScriptExecutionData execdata;
    execdata.m_annex_init = true;
    execdata.m_annex_present = false; // Only support annex-less signing for now.
    if (sigversion == SigVersion::TAPSCRIPT) {
        execdata.m_codeseparator_pos_init = true;
        execdata.m_codeseparator_pos = 0xFFFFFFFF; // Only support non-OP_CODESEPARATOR BIP342 signing for now.
        if (!leaf_hash) return std::nullopt; // BIP342 signing needs leaf hash.
        execdata.m_tapleaf_hash_init = true;
        execdata.m_tapleaf_hash = *leaf_hash;
    }
    uint256 hash;
    if (!SignatureHashSchnorr(hash, execdata, m_txto, nIn, nHashType, sigversion, *m_txdata, MissingDataBehavior::FAIL)) return std::nullopt;
    return hash;
}

bool MutableTransactionSignatureCreator::CreateSchnorrSig(const SigningProvider& provider, std::vector<unsigned char>& sig, const XOnlyPubKey& pubkey, const uint256* leaf_hash, const uint256* merkle_root, SigVersion sigversion) const
{
    assert(sigversion == SigVersion::TAPROOT || sigversion == SigVersion::TAPSCRIPT);

    CKey key;
    if (!provider.GetKeyByXOnly(pubkey, key)) return false;

    std::optional<uint256> hash = ComputeSchnorrSignatureHash(leaf_hash, sigversion);
    if (!hash.has_value()) return false;

    sig.resize(64);
    // Use uint256{} as aux_rnd for now.
    if (!key.SignSchnorr(*hash, sig, merkle_root, {})) return false;
    if (nHashType) sig.push_back(nHashType);
    return true;
}

std::vector<uint8_t> MutableTransactionSignatureCreator::CreateMuSig2Nonce(const SigningProvider& provider, const CPubKey& aggregate_pubkey, const CPubKey& script_pubkey, const CPubKey& part_pubkey, const uint256* leaf_hash, const uint256* merkle_root, SigVersion sigversion, const SignatureData& sigdata) const
{
    assert(sigversion == SigVersion::TAPROOT || sigversion == SigVersion::TAPSCRIPT);

    // Retrieve the private key
    CKey key;
    if (!provider.GetKey(part_pubkey.GetID(), key)) return {};

    // Retrieve participant pubkeys
    auto it = sigdata.musig2_pubkeys.find(aggregate_pubkey);
    if (it == sigdata.musig2_pubkeys.end()) return {};
    const std::vector<CPubKey>& pubkeys = it->second;
    if (std::find(pubkeys.begin(), pubkeys.end(), part_pubkey) == pubkeys.end()) return {};

    // Compute sighash
    std::optional<uint256> sighash = ComputeSchnorrSignatureHash(leaf_hash, sigversion);
    if (!sighash.has_value()) return {};

    MuSig2SecNonce secnonce;
    std::vector<uint8_t> out = key.CreateMuSig2Nonce(secnonce, *sighash, aggregate_pubkey, pubkeys);
    if (out.empty()) return {};

    // Store the secnonce in the SigningProvider
    provider.SetMuSig2SecNonce(MuSig2SessionID(script_pubkey, part_pubkey, *sighash), std::move(secnonce));

    return out;
}

bool MutableTransactionSignatureCreator::CreateMuSig2PartialSig(const SigningProvider& provider, uint256& partial_sig, const CPubKey& aggregate_pubkey, const CPubKey& script_pubkey, const CPubKey& part_pubkey, const uint256* leaf_hash, const std::vector<std::pair<uint256, bool>>& tweaks, SigVersion sigversion, const SignatureData& sigdata) const
{
    assert(sigversion == SigVersion::TAPROOT || sigversion == SigVersion::TAPSCRIPT);

    // Retrieve private key
    CKey key;
    if (!provider.GetKey(part_pubkey.GetID(), key)) return false;

    // Retrieve participant pubkeys
    auto it = sigdata.musig2_pubkeys.find(aggregate_pubkey);
    if (it == sigdata.musig2_pubkeys.end()) return false;
    const std::vector<CPubKey>& pubkeys = it->second;
    if (std::find(pubkeys.begin(), pubkeys.end(), part_pubkey) == pubkeys.end()) return {};

    // Retrieve pubnonces
    auto this_leaf_aggkey = std::make_pair(script_pubkey, leaf_hash ? *leaf_hash : uint256());
    auto pubnonce_it = sigdata.musig2_pubnonces.find(this_leaf_aggkey);
    if (pubnonce_it == sigdata.musig2_pubnonces.end()) return false;
    const std::map<CPubKey, std::vector<uint8_t>>& pubnonces = pubnonce_it->second;

    // Check if enough pubnonces
    if (pubnonces.size() != pubkeys.size()) return false;

    // Compute sighash
    std::optional<uint256> sighash = ComputeSchnorrSignatureHash(leaf_hash, sigversion);
    if (!sighash.has_value()) return false;

    // Retrieve the secnonce
    uint256 session_id = MuSig2SessionID(script_pubkey, part_pubkey, *sighash);
    std::optional<std::reference_wrapper<MuSig2SecNonce>> secnonce = provider.GetMuSig2SecNonce(session_id);
    if (!secnonce || !secnonce->get().IsValid()) return false;

    // Compute the sig
    std::optional<uint256> sig = key.CreateMuSig2PartialSig(*sighash, aggregate_pubkey, pubkeys, pubnonces, *secnonce, tweaks);
    if (!sig) return false;
    partial_sig = std::move(*sig);

    // Delete the secnonce now that we're done with it
    assert(!secnonce->get().IsValid());
    provider.DeleteMuSig2Session(session_id);

    return true;
}

bool MutableTransactionSignatureCreator::CreateMuSig2AggregateSig(const std::vector<CPubKey>& participants, std::vector<uint8_t>& sig, const CPubKey& aggregate_pubkey, const CPubKey& script_pubkey, const uint256* leaf_hash, const std::vector<std::pair<uint256, bool>>& tweaks, SigVersion sigversion, const SignatureData& sigdata) const
{
    assert(sigversion == SigVersion::TAPROOT || sigversion == SigVersion::TAPSCRIPT);
    if (!participants.size()) return false;

    // Retrieve pubnonces and partial sigs
    auto this_leaf_aggkey = std::make_pair(script_pubkey, leaf_hash ? *leaf_hash : uint256());
    auto pubnonce_it = sigdata.musig2_pubnonces.find(this_leaf_aggkey);
    if (pubnonce_it == sigdata.musig2_pubnonces.end()) return false;
    const std::map<CPubKey, std::vector<uint8_t>>& pubnonces = pubnonce_it->second;
    auto partial_sigs_it = sigdata.musig2_partial_sigs.find(this_leaf_aggkey);
    if (partial_sigs_it == sigdata.musig2_partial_sigs.end()) return false;
    const std::map<CPubKey, uint256>& partial_sigs = partial_sigs_it->second;

    // Check if enough pubnonces and partial sigs
    if (pubnonces.size() != participants.size()) return false;
    if (partial_sigs.size() != participants.size()) return false;

    // Compute sighash
    std::optional<uint256> sighash = ComputeSchnorrSignatureHash(leaf_hash, sigversion);
    if (!sighash.has_value()) return false;

    std::optional<std::vector<uint8_t>> res = ::CreateMuSig2AggregateSig(participants, aggregate_pubkey, tweaks, *sighash, pubnonces, partial_sigs);
    if (!res) return false;
    sig = res.value();
    if (nHashType) sig.push_back(nHashType);

    return true;
}

static bool GetCScript(const SigningProvider& provider, const SignatureData& sigdata, const CScriptID& scriptid, CScript& script)
{
    if (provider.GetCScript(scriptid, script)) {
        return true;
    }
    // Look for scripts in SignatureData
    if (CScriptID(sigdata.redeem_script) == scriptid) {
        script = sigdata.redeem_script;
        return true;
    } else if (CScriptID(sigdata.witness_script) == scriptid) {
        script = sigdata.witness_script;
        return true;
    }
    return false;
}

static bool GetPubKey(const SigningProvider& provider, const SignatureData& sigdata, const CKeyID& address, CPubKey& pubkey)
{
    // Look for pubkey in all partial sigs
    const auto it = sigdata.signatures.find(address);
    if (it != sigdata.signatures.end()) {
        pubkey = it->second.first;
        return true;
    }
    // Look for pubkey in pubkey lists
    const auto& pk_it = sigdata.misc_pubkeys.find(address);
    if (pk_it != sigdata.misc_pubkeys.end()) {
        pubkey = pk_it->second.first;
        return true;
    }
    const auto& tap_pk_it = sigdata.tap_pubkeys.find(address);
    if (tap_pk_it != sigdata.tap_pubkeys.end()) {
        pubkey = tap_pk_it->second.GetEvenCorrespondingCPubKey();
        return true;
    }
    // Query the underlying provider
    return provider.GetPubKey(address, pubkey);
}

static bool CreateSig(const BaseSignatureCreator& creator, SignatureData& sigdata, const SigningProvider& provider, std::vector<unsigned char>& sig_out, const CPubKey& pubkey, const CScript& scriptcode, SigVersion sigversion)
{
    CKeyID keyid = pubkey.GetID();
    const auto it = sigdata.signatures.find(keyid);
    if (it != sigdata.signatures.end()) {
        sig_out = it->second.second;
        return true;
    }
    KeyOriginInfo info;
    if (provider.GetKeyOrigin(keyid, info)) {
        sigdata.misc_pubkeys.emplace(keyid, std::make_pair(pubkey, std::move(info)));
    }
    if (creator.CreateSig(provider, sig_out, keyid, scriptcode, sigversion)) {
        auto i = sigdata.signatures.emplace(keyid, SigPair(pubkey, sig_out));
        assert(i.second);
        return true;
    }
    // Could not make signature or signature not found, add keyid to missing
    sigdata.missing_sigs.push_back(keyid);
    return false;
}

static bool SignMuSig2(const BaseSignatureCreator& creator, SignatureData& sigdata, const SigningProvider& provider, std::vector<unsigned char>& sig_out, const XOnlyPubKey& script_pubkey, const uint256* merkle_root, const uint256* leaf_hash, SigVersion sigversion)
{
    Assert(sigversion == SigVersion::TAPROOT || sigversion == SigVersion::TAPSCRIPT);

    // Lookup derivation paths for the script pubkey
    KeyOriginInfo agg_info;
    auto misc_pk_it = sigdata.taproot_misc_pubkeys.find(script_pubkey);
    if (misc_pk_it != sigdata.taproot_misc_pubkeys.end()) {
        agg_info = misc_pk_it->second.second;
    }

    for (const auto& [agg_pub, part_pks] : sigdata.musig2_pubkeys) {
        if (part_pks.empty()) continue;

        // Fill participant derivation path info
        for (const auto& part_pk : part_pks) {
            KeyOriginInfo part_info;
            if (provider.GetKeyOrigin(part_pk.GetID(), part_info)) {
                XOnlyPubKey xonly_part(part_pk);
                auto it = sigdata.taproot_misc_pubkeys.find(xonly_part);
                if (it == sigdata.taproot_misc_pubkeys.end()) {
                    it = sigdata.taproot_misc_pubkeys.emplace(xonly_part, std::make_pair(std::set<uint256>(), part_info)).first;
                }
                if (leaf_hash) it->second.first.insert(*leaf_hash);
            }
        }

        // The pubkey in the script may not be the actual aggregate of the participants, but derived from it.
        // Check the derivation, and compute the BIP 32 derivation tweaks
        std::vector<std::pair<uint256, bool>> tweaks;
        CPubKey plain_pub = agg_pub;
        if (XOnlyPubKey(agg_pub) != script_pubkey) {
            if (agg_info.path.empty()) continue;
            // Compute and compare fingerprint
            CKeyID keyid = agg_pub.GetID();
            if (!std::equal(agg_info.fingerprint, agg_info.fingerprint + sizeof(agg_info.fingerprint), keyid.data())) {
                continue;
            }
            // Get the BIP32 derivation tweaks
            CExtPubKey extpub = CreateMuSig2SyntheticXpub(agg_pub);
            for (const int i : agg_info.path) {
                auto& [t, xonly] = tweaks.emplace_back();
                xonly = false;
                if (!extpub.Derive(extpub, i, &t)) {
                    return false;
                }
            }
            Assert(XOnlyPubKey(extpub.pubkey) == script_pubkey);
            plain_pub = extpub.pubkey;
        }

        // Add the merkle root tweak
        if (sigversion == SigVersion::TAPROOT && merkle_root) {
            tweaks.emplace_back(script_pubkey.ComputeTapTweakHash(merkle_root->IsNull() ? nullptr : merkle_root), true);
            std::optional<std::pair<XOnlyPubKey, bool>> tweaked = script_pubkey.CreateTapTweak(merkle_root->IsNull() ? nullptr : merkle_root);
            if (!Assume(tweaked)) return false;
            plain_pub = tweaked->first.GetCPubKeys().at(tweaked->second ? 1 : 0);
        }

        // First try to aggregate
        if (creator.CreateMuSig2AggregateSig(part_pks, sig_out, agg_pub, plain_pub, leaf_hash, tweaks, sigversion, sigdata)) {
            if (sigversion == SigVersion::TAPROOT) {
                sigdata.taproot_key_path_sig = sig_out;
            } else {
                auto lookup_key = std::make_pair(script_pubkey, leaf_hash ? *leaf_hash : uint256());
                sigdata.taproot_script_sigs[lookup_key] = sig_out;
            }
            continue;
        }
        // Cannot aggregate, try making partial sigs for every participant
        auto pub_key_leaf_hash = std::make_pair(plain_pub, leaf_hash ? *leaf_hash : uint256());
        for (const CPubKey& part_pk : part_pks) {
            uint256 partial_sig;
            if (creator.CreateMuSig2PartialSig(provider, partial_sig, agg_pub, plain_pub, part_pk, leaf_hash, tweaks, sigversion, sigdata) && Assume(!partial_sig.IsNull())) {
                sigdata.musig2_partial_sigs[pub_key_leaf_hash].emplace(part_pk, partial_sig);
            }
        }
        // If there are any partial signatures, exit early
        auto partial_sigs_it = sigdata.musig2_partial_sigs.find(pub_key_leaf_hash);
        if (partial_sigs_it != sigdata.musig2_partial_sigs.end() && !partial_sigs_it->second.empty()) {
            continue;
        }
        // No partial sigs, try to make pubnonces
        std::map<CPubKey, std::vector<uint8_t>>& pubnonces = sigdata.musig2_pubnonces[pub_key_leaf_hash];
        for (const CPubKey& part_pk : part_pks) {
            if (pubnonces.contains(part_pk)) continue;
            std::vector<uint8_t> pubnonce = creator.CreateMuSig2Nonce(provider, agg_pub, plain_pub, part_pk, leaf_hash, merkle_root, sigversion, sigdata);
            if (pubnonce.empty()) continue;
            pubnonces[part_pk] = std::move(pubnonce);
        }
    }
    return true;
}

static bool CreateTaprootScriptSig(const BaseSignatureCreator& creator, SignatureData& sigdata, const SigningProvider& provider, std::vector<unsigned char>& sig_out, const XOnlyPubKey& pubkey, const uint256& leaf_hash, SigVersion sigversion)
{
    KeyOriginInfo info;
    if (provider.GetKeyOriginByXOnly(pubkey, info)) {
        auto it = sigdata.taproot_misc_pubkeys.find(pubkey);
        if (it == sigdata.taproot_misc_pubkeys.end()) {
            sigdata.taproot_misc_pubkeys.emplace(pubkey, std::make_pair(std::set<uint256>({leaf_hash}), info));
        } else {
            it->second.first.insert(leaf_hash);
        }
    }

    auto lookup_key = std::make_pair(pubkey, leaf_hash);
    auto it = sigdata.taproot_script_sigs.find(lookup_key);
    if (it != sigdata.taproot_script_sigs.end()) {
        sig_out = it->second;
        return true;
    }

    if (creator.CreateSchnorrSig(provider, sig_out, pubkey, &leaf_hash, nullptr, sigversion)) {
        sigdata.taproot_script_sigs[lookup_key] = sig_out;
    } else if (!SignMuSig2(creator, sigdata, provider, sig_out, pubkey, /*merkle_root=*/nullptr, &leaf_hash, sigversion)) {
        return false;
    }

    return sigdata.taproot_script_sigs.contains(lookup_key);
}

template<typename M, typename K, typename V>
miniscript::Availability MsLookupHelper(const M& map, const K& key, V& value)
{
    auto it = map.find(key);
    if (it != map.end()) {
        value = it->second;
        return miniscript::Availability::YES;
    }
    return miniscript::Availability::NO;
}

/**
 * Context for solving a Miniscript.
 * If enough material (access to keys, hash preimages, ..) is given, produces a valid satisfaction.
 */
template<typename Pk>
struct Satisfier {
    using Key = Pk;

    const SigningProvider& m_provider;
    SignatureData& m_sig_data;
    const BaseSignatureCreator& m_creator;
    const CScript& m_witness_script;
    //! The context of the script we are satisfying (either P2WSH or Tapscript).
    const miniscript::MiniscriptContext m_script_ctx;

    explicit Satisfier(const SigningProvider& provider LIFETIMEBOUND, SignatureData& sig_data LIFETIMEBOUND,
                       const BaseSignatureCreator& creator LIFETIMEBOUND,
                       const CScript& witscript LIFETIMEBOUND,
                       miniscript::MiniscriptContext script_ctx) : m_provider(provider),
                                                                   m_sig_data(sig_data),
                                                                   m_creator(creator),
                                                                   m_witness_script(witscript),
                                                                   m_script_ctx(script_ctx) {}

    static bool KeyCompare(const Key& a, const Key& b) {
        return a < b;
    }

    //! Get a CPubKey from a key hash. Note the key hash may be of an xonly pubkey.
    template<typename I>
    std::optional<CPubKey> CPubFromPKHBytes(I first, I last) const {
        assert(last - first == 20);
        CPubKey pubkey;
        CKeyID key_id;
        std::copy(first, last, key_id.begin());
        if (GetPubKey(m_provider, m_sig_data, key_id, pubkey)) return pubkey;
        m_sig_data.missing_pubkeys.push_back(key_id);
        return {};
    }

    //! Conversion to raw public key.
    std::vector<unsigned char> ToPKBytes(const Key& key) const { return {key.begin(), key.end()}; }

    //! Time lock satisfactions.
    bool CheckAfter(uint32_t value) const { return m_creator.Checker().CheckLockTime(CScriptNum(value)); }
    bool CheckOlder(uint32_t value) const { return m_creator.Checker().CheckSequence(CScriptNum(value)); }

    //! Hash preimage satisfactions.
    miniscript::Availability SatSHA256(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return MsLookupHelper(m_sig_data.sha256_preimages, hash, preimage);
    }
    miniscript::Availability SatRIPEMD160(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return MsLookupHelper(m_sig_data.ripemd160_preimages, hash, preimage);
    }
    miniscript::Availability SatHASH256(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return MsLookupHelper(m_sig_data.hash256_preimages, hash, preimage);
    }
    miniscript::Availability SatHASH160(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return MsLookupHelper(m_sig_data.hash160_preimages, hash, preimage);
    }

    miniscript::MiniscriptContext MsContext() const {
        return m_script_ctx;
    }
};

/** Miniscript satisfier specific to P2WSH context. */
struct WshSatisfier: Satisfier<CPubKey> {
    explicit WshSatisfier(const SigningProvider& provider LIFETIMEBOUND, SignatureData& sig_data LIFETIMEBOUND,
                          const BaseSignatureCreator& creator LIFETIMEBOUND, const CScript& witscript LIFETIMEBOUND)
                          : Satisfier(provider, sig_data, creator, witscript, miniscript::MiniscriptContext::P2WSH) {}

    //! Conversion from a raw compressed public key.
    template <typename I>
    std::optional<CPubKey> FromPKBytes(I first, I last) const {
        CPubKey pubkey{first, last};
        if (pubkey.IsValid()) return pubkey;
        return {};
    }

    //! Conversion from a raw compressed public key hash.
    template<typename I>
    std::optional<CPubKey> FromPKHBytes(I first, I last) const {
        return Satisfier::CPubFromPKHBytes(first, last);
    }

    //! Satisfy an ECDSA signature check.
    miniscript::Availability Sign(const CPubKey& key, std::vector<unsigned char>& sig) const {
        if (CreateSig(m_creator, m_sig_data, m_provider, sig, key, m_witness_script, SigVersion::WITNESS_V0)) {
            return miniscript::Availability::YES;
        }
        return miniscript::Availability::NO;
    }
};

/** Miniscript satisfier specific to Tapscript context. */
struct TapSatisfier: Satisfier<XOnlyPubKey> {
    const uint256& m_leaf_hash;

    explicit TapSatisfier(const SigningProvider& provider LIFETIMEBOUND, SignatureData& sig_data LIFETIMEBOUND,
                          const BaseSignatureCreator& creator LIFETIMEBOUND, const CScript& script LIFETIMEBOUND,
                          const uint256& leaf_hash LIFETIMEBOUND)
                          : Satisfier(provider, sig_data, creator, script, miniscript::MiniscriptContext::TAPSCRIPT),
                            m_leaf_hash(leaf_hash) {}

    //! Conversion from a raw xonly public key.
    template <typename I>
    std::optional<XOnlyPubKey> FromPKBytes(I first, I last) const {
        if (last - first != 32) return {};
        XOnlyPubKey pubkey;
        std::copy(first, last, pubkey.begin());
        return pubkey;
    }

    //! Conversion from a raw xonly public key hash.
    template<typename I>
    std::optional<XOnlyPubKey> FromPKHBytes(I first, I last) const {
        if (auto pubkey = Satisfier::CPubFromPKHBytes(first, last)) return XOnlyPubKey{*pubkey};
        return {};
    }

    //! Satisfy a BIP340 signature check.
    miniscript::Availability Sign(const XOnlyPubKey& key, std::vector<unsigned char>& sig) const {
        if (CreateTaprootScriptSig(m_creator, m_sig_data, m_provider, sig, key, m_leaf_hash, SigVersion::TAPSCRIPT)) {
            return miniscript::Availability::YES;
        }
        return miniscript::Availability::NO;
    }
};

static bool SignTaprootScript(const SigningProvider& provider, const BaseSignatureCreator& creator, SignatureData& sigdata, int leaf_version, std::span<const unsigned char> script_bytes, std::vector<valtype>& result)
{
    // Only BIP342 tapscript signing is supported for now.
    if (leaf_version != TAPROOT_LEAF_TAPSCRIPT) return false;

    uint256 leaf_hash = ComputeTapleafHash(leaf_version, script_bytes);
    CScript script = CScript(script_bytes.begin(), script_bytes.end());

    TapSatisfier ms_satisfier{provider, sigdata, creator, script, leaf_hash};
    const auto ms = miniscript::FromScript(script, ms_satisfier);
    return ms && ms->Satisfy(ms_satisfier, result) == miniscript::Availability::YES;
}

static bool SignTaproot(const SigningProvider& provider, const BaseSignatureCreator& creator, const WitnessV1Taproot& output, SignatureData& sigdata, std::vector<valtype>& result)
{
    TaprootSpendData spenddata;
    TaprootBuilder builder;

    // Gather information about this output.
    if (provider.GetTaprootSpendData(output, spenddata)) {
        sigdata.tr_spenddata.Merge(spenddata);
    }
    if (provider.GetTaprootBuilder(output, builder)) {
        sigdata.tr_builder = builder;
    }
    if (auto agg_keys = provider.GetAllMuSig2ParticipantPubkeys(); !agg_keys.empty()) {
        sigdata.musig2_pubkeys.insert(agg_keys.begin(), agg_keys.end());
    }


    // Try key path spending.
    {
        KeyOriginInfo internal_key_info;
        if (provider.GetKeyOriginByXOnly(sigdata.tr_spenddata.internal_key, internal_key_info)) {
            auto it = sigdata.taproot_misc_pubkeys.find(sigdata.tr_spenddata.internal_key);
            if (it == sigdata.taproot_misc_pubkeys.end()) {
                sigdata.taproot_misc_pubkeys.emplace(sigdata.tr_spenddata.internal_key, std::make_pair(std::set<uint256>(), internal_key_info));
            }
        }

        KeyOriginInfo output_key_info;
        if (provider.GetKeyOriginByXOnly(output, output_key_info)) {
            auto it = sigdata.taproot_misc_pubkeys.find(output);
            if (it == sigdata.taproot_misc_pubkeys.end()) {
                sigdata.taproot_misc_pubkeys.emplace(output, std::make_pair(std::set<uint256>(), output_key_info));
            }
        }

        auto make_keypath_sig = [&](const XOnlyPubKey& pk, const uint256* merkle_root) {
            std::vector<unsigned char> sig;
            if (creator.CreateSchnorrSig(provider, sig, pk, nullptr, merkle_root, SigVersion::TAPROOT)) {
                sigdata.taproot_key_path_sig = sig;
            } else {
                SignMuSig2(creator, sigdata, provider, sig, pk, merkle_root, /*leaf_hash=*/nullptr, SigVersion::TAPROOT);
            }
        };

        // First try signing with internal key
        if (sigdata.taproot_key_path_sig.size() == 0) {
            make_keypath_sig(sigdata.tr_spenddata.internal_key, &sigdata.tr_spenddata.merkle_root);
        }
        // Try signing with output key if still no signature
        if (sigdata.taproot_key_path_sig.size() == 0) {
            make_keypath_sig(output, nullptr);
        }
        if (sigdata.taproot_key_path_sig.size()) {
            result = Vector(sigdata.taproot_key_path_sig);
            return true;
        }
    }

    // Try script path spending.
    std::vector<std::vector<unsigned char>> smallest_result_stack;
    for (const auto& [key, control_blocks] : sigdata.tr_spenddata.scripts) {
        const auto& [script, leaf_ver] = key;
        std::vector<std::vector<unsigned char>> result_stack;
        if (SignTaprootScript(provider, creator, sigdata, leaf_ver, script, result_stack)) {
            result_stack.emplace_back(std::begin(script), std::end(script)); // Push the script
            result_stack.push_back(*control_blocks.begin()); // Push the smallest control block
            if (smallest_result_stack.size() == 0 ||
                GetSerializeSize(result_stack) < GetSerializeSize(smallest_result_stack)) {
                smallest_result_stack = std::move(result_stack);
            }
        }
    }
    if (smallest_result_stack.size() != 0) {
        result = std::move(smallest_result_stack);
        return true;
    }

    return false;
}

/**
 * Sign scriptPubKey using signature made with creator.
 * Signatures are returned in scriptSigRet (or returns false if scriptPubKey can't be signed),
 * unless whichTypeRet is TxoutType::SCRIPTHASH, in which case scriptSigRet is the redemption script.
 * Returns false if scriptPubKey could not be completely satisfied.
 */
static bool SignStep(const SigningProvider& provider, const BaseSignatureCreator& creator, const CScript& scriptPubKey,
                     std::vector<valtype>& ret, TxoutType& whichTypeRet, SigVersion sigversion, SignatureData& sigdata)
{
    CScript scriptRet;
    ret.clear();
    std::vector<unsigned char> sig;

    std::vector<valtype> vSolutions;
    whichTypeRet = Solver(scriptPubKey, vSolutions);

    switch (whichTypeRet) {
    case TxoutType::NONSTANDARD:
    case TxoutType::NULL_DATA:
    case TxoutType::WITNESS_UNKNOWN:
        return false;
    case TxoutType::PUBKEY:
        if (!CreateSig(creator, sigdata, provider, sig, CPubKey(vSolutions[0]), scriptPubKey, sigversion)) return false;
        ret.push_back(std::move(sig));
        return true;
    case TxoutType::PUBKEYHASH: {
        CKeyID keyID = CKeyID(uint160(vSolutions[0]));
        CPubKey pubkey;
        if (!GetPubKey(provider, sigdata, keyID, pubkey)) {
            // Pubkey could not be found, add to missing
            sigdata.missing_pubkeys.push_back(keyID);
            return false;
        }
        if (!CreateSig(creator, sigdata, provider, sig, pubkey, scriptPubKey, sigversion)) return false;
        ret.push_back(std::move(sig));
        ret.push_back(ToByteVector(pubkey));
        return true;
    }
    case TxoutType::SCRIPTHASH: {
        uint160 h160{vSolutions[0]};
        if (GetCScript(provider, sigdata, CScriptID{h160}, scriptRet)) {
            ret.emplace_back(scriptRet.begin(), scriptRet.end());
            return true;
        }
        // Could not find redeemScript, add to missing
        sigdata.missing_redeem_script = h160;
        return false;
    }
    case TxoutType::MULTISIG: {
        size_t required = vSolutions.front()[0];
        ret.emplace_back(); // workaround CHECKMULTISIG bug
        for (size_t i = 1; i < vSolutions.size() - 1; ++i) {
            CPubKey pubkey = CPubKey(vSolutions[i]);
            // We need to always call CreateSig in order to fill sigdata with all
            // possible signatures that we can create. This will allow further PSBT
            // processing to work as it needs all possible signature and pubkey pairs
            if (CreateSig(creator, sigdata, provider, sig, pubkey, scriptPubKey, sigversion)) {
                if (ret.size() < required + 1) {
                    ret.push_back(std::move(sig));
                }
            }
        }
        bool ok = ret.size() == required + 1;
        for (size_t i = 0; i + ret.size() < required + 1; ++i) {
            ret.emplace_back();
        }
        return ok;
    }
    case TxoutType::WITNESS_V0_KEYHASH:
        ret.push_back(vSolutions[0]);
        return true;

    case TxoutType::WITNESS_V0_SCRIPTHASH:
        if (GetCScript(provider, sigdata, CScriptID{RIPEMD160(vSolutions[0])}, scriptRet)) {
            ret.emplace_back(scriptRet.begin(), scriptRet.end());
            return true;
        }
        // Could not find witnessScript, add to missing
        sigdata.missing_witness_script = uint256(vSolutions[0]);
        return false;

    case TxoutType::WITNESS_V1_TAPROOT:
        return SignTaproot(provider, creator, WitnessV1Taproot(XOnlyPubKey{vSolutions[0]}), sigdata, ret);

    case TxoutType::ANCHOR:
        return true;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

static CScript PushAll(const std::vector<valtype>& values)
{
    CScript result;
    for (const valtype& v : values) {
        if (v.size() == 0) {
            result << OP_0;
        } else if (v.size() == 1 && v[0] >= 1 && v[0] <= 16) {
            result << CScript::EncodeOP_N(v[0]);
        } else if (v.size() == 1 && v[0] == 0x81) {
            result << OP_1NEGATE;
        } else {
            result << v;
        }
    }
    return result;
}

bool ProduceSignature(const SigningProvider& provider, const BaseSignatureCreator& creator, const CScript& fromPubKey, SignatureData& sigdata)
{
    if (sigdata.complete) return true;

    std::vector<valtype> result;
    TxoutType whichType;
    bool solved = SignStep(provider, creator, fromPubKey, result, whichType, SigVersion::BASE, sigdata);
    bool P2SH = false;
    CScript subscript;

    if (solved && whichType == TxoutType::SCRIPTHASH)
    {
        // Solver returns the subscript that needs to be evaluated;
        // the final scriptSig is the signatures from that
        // and then the serialized subscript:
        subscript = CScript(result[0].begin(), result[0].end());
        sigdata.redeem_script = subscript;
        solved = solved && SignStep(provider, creator, subscript, result, whichType, SigVersion::BASE, sigdata) && whichType != TxoutType::SCRIPTHASH;
        P2SH = true;
    }

    if (solved && whichType == TxoutType::WITNESS_V0_KEYHASH)
    {
        CScript witnessscript;
        witnessscript << OP_DUP << OP_HASH160 << ToByteVector(result[0]) << OP_EQUALVERIFY << OP_CHECKSIG;
        TxoutType subType;
        solved = solved && SignStep(provider, creator, witnessscript, result, subType, SigVersion::WITNESS_V0, sigdata);
        sigdata.scriptWitness.stack = result;
        sigdata.witness = true;
        result.clear();
    }
    else if (solved && whichType == TxoutType::WITNESS_V0_SCRIPTHASH)
    {
        CScript witnessscript(result[0].begin(), result[0].end());
        sigdata.witness_script = witnessscript;

        TxoutType subType{TxoutType::NONSTANDARD};
        solved = solved && SignStep(provider, creator, witnessscript, result, subType, SigVersion::WITNESS_V0, sigdata) && subType != TxoutType::SCRIPTHASH && subType != TxoutType::WITNESS_V0_SCRIPTHASH && subType != TxoutType::WITNESS_V0_KEYHASH;

        // If we couldn't find a solution with the legacy satisfier, try satisfying the script using Miniscript.
        // Note we need to check if the result stack is empty before, because it might be used even if the Script
        // isn't fully solved. For instance the CHECKMULTISIG satisfaction in SignStep() pushes partial signatures
        // and the extractor relies on this behaviour to combine witnesses.
        if (!solved && result.empty()) {
            WshSatisfier ms_satisfier{provider, sigdata, creator, witnessscript};
            const auto ms = miniscript::FromScript(witnessscript, ms_satisfier);
            solved = ms && ms->Satisfy(ms_satisfier, result) == miniscript::Availability::YES;
        }
        result.emplace_back(witnessscript.begin(), witnessscript.end());

        sigdata.scriptWitness.stack = result;
        sigdata.witness = true;
        result.clear();
    } else if (whichType == TxoutType::WITNESS_V1_TAPROOT && !P2SH) {
        sigdata.witness = true;
        if (solved) {
            sigdata.scriptWitness.stack = std::move(result);
        }
        result.clear();
    } else if (solved && whichType == TxoutType::WITNESS_UNKNOWN) {
        sigdata.witness = true;
    }

    if (!sigdata.witness) sigdata.scriptWitness.stack.clear();
    if (P2SH) {
        result.emplace_back(subscript.begin(), subscript.end());
    }
    sigdata.scriptSig = PushAll(result);

    // Test solution
    sigdata.complete = solved && VerifyScript(sigdata.scriptSig, fromPubKey, &sigdata.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, creator.Checker());
    return sigdata.complete;
}

namespace {
class SignatureExtractorChecker final : public DeferringSignatureChecker
{
private:
    SignatureData& sigdata;

public:
    SignatureExtractorChecker(SignatureData& sigdata, BaseSignatureChecker& checker) : DeferringSignatureChecker(checker), sigdata(sigdata) {}

    bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override
    {
        if (m_checker.CheckECDSASignature(scriptSig, vchPubKey, scriptCode, sigversion)) {
            CPubKey pubkey(vchPubKey);
            sigdata.signatures.emplace(pubkey.GetID(), SigPair(pubkey, scriptSig));
            return true;
        }
        return false;
    }
};

struct Stacks
{
    std::vector<valtype> script;
    std::vector<valtype> witness;

    Stacks() = delete;
    Stacks(const Stacks&) = delete;
    explicit Stacks(const SignatureData& data) : witness(data.scriptWitness.stack) {
        EvalScript(script, data.scriptSig, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE);
    }
};
}

// Extracts signatures and scripts from incomplete scriptSigs. Please do not extend this, use PSBT instead
SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn, const CTxOut& txout)
{
    SignatureData data;
    assert(tx.vin.size() > nIn);
    data.scriptSig = tx.vin[nIn].scriptSig;
    data.scriptWitness = tx.vin[nIn].scriptWitness;
    Stacks stack(data);

    // Get signatures
    MutableTransactionSignatureChecker tx_checker(&tx, nIn, txout.nValue, MissingDataBehavior::FAIL);
    SignatureExtractorChecker extractor_checker(data, tx_checker);
    if (VerifyScript(data.scriptSig, txout.scriptPubKey, &data.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, extractor_checker)) {
        data.complete = true;
        return data;
    }

    // Get scripts
    std::vector<std::vector<unsigned char>> solutions;
    TxoutType script_type = Solver(txout.scriptPubKey, solutions);
    SigVersion sigversion = SigVersion::BASE;
    CScript next_script = txout.scriptPubKey;

    if (script_type == TxoutType::SCRIPTHASH && !stack.script.empty() && !stack.script.back().empty()) {
        // Get the redeemScript
        CScript redeem_script(stack.script.back().begin(), stack.script.back().end());
        data.redeem_script = redeem_script;
        next_script = std::move(redeem_script);

        // Get redeemScript type
        script_type = Solver(next_script, solutions);
        stack.script.pop_back();
    }
    if (script_type == TxoutType::WITNESS_V0_SCRIPTHASH && !stack.witness.empty() && !stack.witness.back().empty()) {
        // Get the witnessScript
        CScript witness_script(stack.witness.back().begin(), stack.witness.back().end());
        data.witness_script = witness_script;
        next_script = std::move(witness_script);

        // Get witnessScript type
        script_type = Solver(next_script, solutions);
        stack.witness.pop_back();
        stack.script = std::move(stack.witness);
        stack.witness.clear();
        sigversion = SigVersion::WITNESS_V0;
    }
    if (script_type == TxoutType::MULTISIG && !stack.script.empty()) {
        // Build a map of pubkey -> signature by matching sigs to pubkeys:
        assert(solutions.size() > 1);
        unsigned int num_pubkeys = solutions.size()-2;
        unsigned int last_success_key = 0;
        for (const valtype& sig : stack.script) {
            for (unsigned int i = last_success_key; i < num_pubkeys; ++i) {
                const valtype& pubkey = solutions[i+1];
                // We either have a signature for this pubkey, or we have found a signature and it is valid
                if (data.signatures.count(CPubKey(pubkey).GetID()) || extractor_checker.CheckECDSASignature(sig, pubkey, next_script, sigversion)) {
                    last_success_key = i + 1;
                    break;
                }
            }
        }
    }

    return data;
}

void UpdateInput(CTxIn& input, const SignatureData& data)
{
    input.scriptSig = data.scriptSig;
    input.scriptWitness = data.scriptWitness;
}

void SignatureData::MergeSignatureData(SignatureData sigdata)
{
    if (complete) return;
    if (sigdata.complete) {
        *this = std::move(sigdata);
        return;
    }
    if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
        redeem_script = sigdata.redeem_script;
    }
    if (witness_script.empty() && !sigdata.witness_script.empty()) {
        witness_script = sigdata.witness_script;
    }
    signatures.insert(std::make_move_iterator(sigdata.signatures.begin()), std::make_move_iterator(sigdata.signatures.end()));
}

namespace {
/** Dummy signature checker which accepts all signatures. */
class DummySignatureChecker final : public BaseSignatureChecker
{
public:
    DummySignatureChecker() = default;
    bool CheckECDSASignature(const std::vector<unsigned char>& sig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override { return sig.size() != 0; }
    bool CheckSchnorrSignature(std::span<const unsigned char> sig, std::span<const unsigned char> pubkey, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror) const override { return sig.size() != 0; }
    bool CheckLockTime(const CScriptNum& nLockTime) const override { return true; }
    bool CheckSequence(const CScriptNum& nSequence) const override { return true; }
};
}

const BaseSignatureChecker& DUMMY_CHECKER = DummySignatureChecker();

namespace {
class DummySignatureCreator final : public BaseSignatureCreator {
private:
    char m_r_len = 32;
    char m_s_len = 32;
public:
    DummySignatureCreator(char r_len, char s_len) : m_r_len(r_len), m_s_len(s_len) {}
    const BaseSignatureChecker& Checker() const override { return DUMMY_CHECKER; }
    bool CreateSig(const SigningProvider& provider, std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const override
    {
        // Create a dummy signature that is a valid DER-encoding
        vchSig.assign(m_r_len + m_s_len + 7, '\000');
        vchSig[0] = 0x30;
        vchSig[1] = m_r_len + m_s_len + 4;
        vchSig[2] = 0x02;
        vchSig[3] = m_r_len;
        vchSig[4] = 0x01;
        vchSig[4 + m_r_len] = 0x02;
        vchSig[5 + m_r_len] = m_s_len;
        vchSig[6 + m_r_len] = 0x01;
        vchSig[6 + m_r_len + m_s_len] = SIGHASH_ALL;
        return true;
    }
    bool CreateSchnorrSig(const SigningProvider& provider, std::vector<unsigned char>& sig, const XOnlyPubKey& pubkey, const uint256* leaf_hash, const uint256* tweak, SigVersion sigversion) const override
    {
        sig.assign(64, '\000');
        return true;
    }
    std::vector<uint8_t> CreateMuSig2Nonce(const SigningProvider& provider, const CPubKey& aggregate_pubkey, const CPubKey& script_pubkey, const CPubKey& part_pubkey, const uint256* leaf_hash, const uint256* merkle_root, SigVersion sigversion, const SignatureData& sigdata) const override
    {
        std::vector<uint8_t> out;
        out.assign(MUSIG2_PUBNONCE_SIZE, '\000');
        return out;
    }
    bool CreateMuSig2PartialSig(const SigningProvider& provider, uint256& partial_sig, const CPubKey& aggregate_pubkey, const CPubKey& script_pubkey, const CPubKey& part_pubkey, const uint256* leaf_hash, const std::vector<std::pair<uint256, bool>>& tweaks, SigVersion sigversion, const SignatureData& sigdata) const override
    {
        partial_sig = uint256::ONE;
        return true;
    }
    bool CreateMuSig2AggregateSig(const std::vector<CPubKey>& participants, std::vector<uint8_t>& sig, const CPubKey& aggregate_pubkey, const CPubKey& script_pubkey, const uint256* leaf_hash, const std::vector<std::pair<uint256, bool>>& tweaks, SigVersion sigversion, const SignatureData& sigdata) const override
    {
        sig.assign(64, '\000');
        return true;
    }
};

}

const BaseSignatureCreator& DUMMY_SIGNATURE_CREATOR = DummySignatureCreator(32, 32);
const BaseSignatureCreator& DUMMY_MAXIMUM_SIGNATURE_CREATOR = DummySignatureCreator(33, 32);

bool IsSegWitOutput(const SigningProvider& provider, const CScript& script)
{
    int version;
    valtype program;
    if (script.IsWitnessProgram(version, program)) return true;
    if (script.IsPayToScriptHash()) {
        std::vector<valtype> solutions;
        auto whichtype = Solver(script, solutions);
        if (whichtype == TxoutType::SCRIPTHASH) {
            auto h160 = uint160(solutions[0]);
            CScript subscript;
            if (provider.GetCScript(CScriptID{h160}, subscript)) {
                if (subscript.IsWitnessProgram(version, program)) return true;
            }
        }
    }
    return false;
}

bool SignTransaction(CMutableTransaction& mtx, const SigningProvider* keystore, const std::map<COutPoint, Coin>& coins, int nHashType, std::map<int, bilingual_str>& input_errors)
{
    bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mtx);

    PrecomputedTransactionData txdata;
    std::vector<CTxOut> spent_outputs;
    for (unsigned int i = 0; i < mtx.vin.size(); ++i) {
        CTxIn& txin = mtx.vin[i];
        auto coin = coins.find(txin.prevout);
        if (coin == coins.end() || coin->second.IsSpent()) {
            txdata.Init(txConst, /*spent_outputs=*/{}, /*force=*/true);
            break;
        } else {
            spent_outputs.emplace_back(coin->second.out.nValue, coin->second.out.scriptPubKey);
        }
    }
    if (spent_outputs.size() == mtx.vin.size()) {
        txdata.Init(txConst, std::move(spent_outputs), true);
    }

    // Sign what we can:
    for (unsigned int i = 0; i < mtx.vin.size(); ++i) {
        CTxIn& txin = mtx.vin[i];
        auto coin = coins.find(txin.prevout);
        if (coin == coins.end() || coin->second.IsSpent()) {
            input_errors[i] = _("Input not found or already spent");
            continue;
        }
        const CScript& prevPubKey = coin->second.out.scriptPubKey;
        const CAmount& amount = coin->second.out.nValue;

        SignatureData sigdata = DataFromTransaction(mtx, i, coin->second.out);
        // Only sign SIGHASH_SINGLE if there's a corresponding output:
        if (!fHashSingle || (i < mtx.vout.size())) {
            ProduceSignature(*keystore, MutableTransactionSignatureCreator(mtx, i, amount, &txdata, nHashType), prevPubKey, sigdata);
        }

        UpdateInput(txin, sigdata);

        // amount must be specified for valid segwit signature
        if (amount == MAX_MONEY && !txin.scriptWitness.IsNull()) {
            input_errors[i] = _("Missing amount");
            continue;
        }

        ScriptError serror = SCRIPT_ERR_OK;
        if (!sigdata.complete && !VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txConst, i, amount, txdata, MissingDataBehavior::FAIL), &serror)) {
            if (serror == SCRIPT_ERR_INVALID_STACK_OPERATION) {
                // Unable to sign input and verification failed (possible attempt to partially sign).
                input_errors[i] = Untranslated("Unable to sign input, invalid stack size (possibly missing key)");
            } else if (serror == SCRIPT_ERR_SIG_NULLFAIL) {
                // Verification failed (possibly due to insufficient signatures).
                input_errors[i] = Untranslated("CHECK(MULTI)SIG failing with non-zero signature (possibly need more signatures)");
            } else {
                input_errors[i] = Untranslated(ScriptErrorString(serror));
            }
        } else {
            // If this input succeeds, make sure there is no error set for it
            input_errors.erase(i);
        }
    }
    return input_errors.empty();
}
