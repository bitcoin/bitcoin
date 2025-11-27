// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/descriptor.h>
#include <script/sign.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <boost/test/unit_test.hpp>

#include <optional>
#include <regex>
#include <string>
#include <vector>

using namespace util::hex_literals;
using util::Split;

namespace {

void CheckUnparsable(const std::string& prv, const std::string& pub, const std::string& expected_error)
{
    FlatSigningProvider keys_priv, keys_pub;
    std::string error;
    auto parse_priv = Parse(prv, keys_priv, error);
    auto parse_pub = Parse(pub, keys_pub, error);
    BOOST_CHECK_MESSAGE(parse_priv.empty(), prv);
    BOOST_CHECK_MESSAGE(parse_pub.empty(), pub);
    BOOST_CHECK_EQUAL(error, expected_error);
}

/** Check that the script is inferred as non-standard */
void CheckInferRaw(const CScript& script)
{
    FlatSigningProvider dummy_provider;
    std::unique_ptr<Descriptor> desc = InferDescriptor(script, dummy_provider);
    BOOST_CHECK(desc->ToString().rfind("raw(", 0) == 0);
}

constexpr int DEFAULT = 0;
constexpr int RANGE = 1 << 0; // Expected to be ranged descriptor
constexpr int HARDENED = 1 << 1; // Derivation needs access to private keys
constexpr int UNSOLVABLE = 1 << 2; // This descriptor is not expected to be solvable
constexpr int SIGNABLE = 1 << 3; // We can sign with this descriptor (this is not true when actual BIP32 derivation is used, as that's not integrated in our signing code)
constexpr int DERIVE_HARDENED = 1 << 4; // The final derivation is hardened, i.e. ends with *' or *h
constexpr int MIXED_PUBKEYS = 1 << 5;
constexpr int XONLY_KEYS = 1 << 6; // X-only pubkeys are in use (and thus inferring/caching may swap parity of pubkeys/keyids)
constexpr int MISSING_PRIVKEYS = 1 << 7; // Not all private keys are available, so ToPrivateString will fail.
constexpr int SIGNABLE_FAILS = 1 << 8; // We can sign with this descriptor, but actually trying to sign will fail
constexpr int MUSIG = 1 << 9; // This is a MuSig so key counts will have an extra key
constexpr int MUSIG_DERIVATION = 1 << 10; // MuSig with BIP 328 derivation from the aggregate key
constexpr int MIXED_MUSIG = 1 << 11; // Both MuSig and normal key expressions are present
constexpr int UNIQUE_XPUBS = 1 << 12; // Whether the xpub count should be of unique xpubs

/** Compare two descriptors. If only one of them has a checksum, the checksum is ignored. */
bool EqualDescriptor(std::string a, std::string b)
{
    bool a_check = (a.size() > 9 && a[a.size() - 9] == '#');
    bool b_check = (b.size() > 9 && b[b.size() - 9] == '#');
    if (a_check != b_check) {
        if (a_check) a = a.substr(0, a.size() - 9);
        if (b_check) b = b.substr(0, b.size() - 9);
    }
    return a == b;
}

bool EqualSigningProviders(const FlatSigningProvider& a, const FlatSigningProvider& b)
{
    return a.scripts == b.scripts
        && a.pubkeys == b.pubkeys
        && a.origins == b.origins
        && a.keys == b.keys
        && a.tr_trees == b.tr_trees;
}

std::string UseHInsteadOfApostrophe(const std::string& desc)
{
    std::string ret = desc;
    while (true) {
        auto it = ret.find('\'');
        if (it == std::string::npos) break;
        ret[it] = 'h';
    }

    // GetDescriptorChecksum returns "" if the checksum exists but is bad.
    // Switching apostrophes with 'h' breaks the checksum if it exists - recalculate it and replace the broken one.
    if (GetDescriptorChecksum(ret) == "") {
        ret = ret.substr(0, desc.size() - 9);
        ret += std::string("#") + GetDescriptorChecksum(ret);
    }
    return ret;
}

// Count the number of times the string "xpub" appears in a descriptor string
static size_t CountXpubs(const std::string& desc)
{
    size_t count = 0;
    size_t p = desc.find("xpub", 0);
    while (p != std::string::npos) {
        count++;
        p = desc.find("xpub", p + 1);
    }
    return count;
}
//
// Count the number of unique xpubs in a descriptor string
static size_t CountUniqueXpubs(const std::string& desc)
{
    std::regex xpub_regex("xpub\\w+?(?=/)");
    auto search_begin = std::sregex_iterator(desc.begin(), desc.end(), xpub_regex);
    auto search_end = std::sregex_iterator();
    std::set<std::string> xpubs;
    for (std::regex_iterator i = search_begin; i != search_end; ++i) {
        xpubs.emplace(i->str());
    }
    return xpubs.size();
}

const std::set<std::vector<uint32_t>> ONLY_EMPTY{{}};

std::set<CPubKey> GetKeyData(const FlatSigningProvider& provider, int flags) {
    std::set<CPubKey> ret;
    for (const auto& [_, pubkey] : provider.pubkeys) {
        if (flags & XONLY_KEYS) {
            unsigned char bytes[33];
            BOOST_CHECK_EQUAL(pubkey.size(), 33);
            std::copy(pubkey.begin(), pubkey.end(), bytes);
            bytes[0] = 0x02;
            CPubKey norm_pubkey{bytes};
            ret.insert(norm_pubkey);
        } else {
            ret.insert(pubkey);
        }
    }
    return ret;
}

std::set<std::pair<CPubKey, KeyOriginInfo>> GetKeyOriginData(const FlatSigningProvider& provider, int flags) {
    std::set<CKeyID> ignored;
    if (flags & MUSIG) {
        for (const auto& [_, part_pks] : provider.aggregate_pubkeys) {
            for (const auto& pk : part_pks) {
                ignored.insert(pk.GetID());
            }
        }
    }

    std::set<std::pair<CPubKey, KeyOriginInfo>> ret;
    for (const auto& [keyid, data] : provider.origins) {
        if (ignored.contains(keyid)) continue;
        if (flags & XONLY_KEYS) {
            unsigned char bytes[33];
            BOOST_CHECK_EQUAL(data.first.size(), 33);
            std::copy(data.first.begin(), data.first.end(), bytes);
            bytes[0] = 0x02;
            CPubKey norm_pubkey{bytes};
            KeyOriginInfo norm_origin = data.second;
            std::fill(std::begin(norm_origin.fingerprint), std::end(norm_origin.fingerprint), 0); // fingerprints don't necessarily match.
            ret.emplace(norm_pubkey, norm_origin);
        } else {
            ret.insert(data);
        }
    }
    return ret;
}

void DoCheck(std::string prv, std::string pub, const std::string& norm_pub, int flags,
             const std::vector<std::vector<std::string>>& scripts, const std::optional<OutputType>& type, std::optional<uint256> op_desc_id = std::nullopt,
             const std::set<std::vector<uint32_t>>& paths = ONLY_EMPTY, bool replace_apostrophe_with_h_in_prv=false,
             bool replace_apostrophe_with_h_in_pub=false, uint32_t spender_nlocktime=0, uint32_t spender_nsequence=CTxIn::SEQUENCE_FINAL,
             std::map<std::vector<uint8_t>, std::vector<uint8_t>> preimages={},
             std::optional<std::string> expected_prv = std::nullopt, std::optional<std::string> expected_pub = std::nullopt, int desc_index = 0)
{
    FlatSigningProvider keys_priv, keys_pub;
    std::set<std::vector<uint32_t>> left_paths = paths;
    std::string error;

    std::vector<std::unique_ptr<Descriptor>> parse_privs;
    std::vector<std::unique_ptr<Descriptor>> parse_pubs;
    // Check that parsing succeeds.
    if (replace_apostrophe_with_h_in_prv) {
        prv = UseHInsteadOfApostrophe(prv);
    }
    parse_privs = Parse(prv, keys_priv, error);
    BOOST_CHECK_MESSAGE(!parse_privs.empty(), error);
    if (replace_apostrophe_with_h_in_pub) {
        pub = UseHInsteadOfApostrophe(pub);
    }
    parse_pubs = Parse(pub, keys_pub, error);
    BOOST_CHECK_MESSAGE(!parse_pubs.empty(), error);

    auto& parse_priv = parse_privs.at(desc_index);
    auto& parse_pub = parse_pubs.at(desc_index);

    // We must be able to estimate the max satisfaction size for any solvable descriptor top descriptor (but combo).
    const bool is_nontop_or_nonsolvable{!parse_priv->IsSolvable() || !parse_priv->GetOutputType()};
    const auto max_sat_maxsig{parse_priv->MaxSatisfactionWeight(true)};
    const auto max_sat_nonmaxsig{parse_priv->MaxSatisfactionWeight(false)};
    BOOST_CHECK(max_sat_nonmaxsig <= max_sat_maxsig);
    const auto max_elems{parse_priv->MaxSatisfactionElems()};
    const bool is_input_size_info_set{max_sat_maxsig && max_sat_nonmaxsig && max_elems};
    BOOST_CHECK_MESSAGE(is_input_size_info_set || is_nontop_or_nonsolvable, prv);

    // The ScriptSize() must match the size of the Script string. (ScriptSize() is set for all descs but 'combo()'.)
    const bool is_combo{!parse_priv->IsSingleType()};
    BOOST_CHECK_MESSAGE(is_combo || parse_priv->ScriptSize() == scripts[0][0].size() / 2, "Invalid ScriptSize() for " + prv);

    // Check that the correct OutputType is inferred
    BOOST_CHECK(parse_priv->GetOutputType() == type);
    BOOST_CHECK(parse_pub->GetOutputType() == type);

    // Check private keys are extracted from the private version but not the public one.
    BOOST_CHECK(keys_priv.keys.size());
    BOOST_CHECK(!keys_pub.keys.size());

    // If expected_pub is provided, check that the serialize matches that.
    // Otherwise check that they serialize back to the public version.
    std::string pub1 = parse_priv->ToString();
    std::string pub2 = parse_pub->ToString();
    if (expected_pub) {
        BOOST_CHECK_MESSAGE(EqualDescriptor(*expected_pub, pub1), "Private ser: " + pub1 + " Public desc: " + *expected_pub);
        BOOST_CHECK_MESSAGE(EqualDescriptor(*expected_pub, pub2), "Public ser: " + pub2 + " Public desc: " + *expected_pub);
    } else {
        BOOST_CHECK_MESSAGE(EqualDescriptor(pub, pub1), "Private ser: " + pub1 + " Public desc: " + pub);
        BOOST_CHECK_MESSAGE(EqualDescriptor(pub, pub2), "Public ser: " + pub2 + " Public desc: " + pub);
    }

    // Check that the COMPAT identifier did not change
    if (op_desc_id) {
        BOOST_CHECK_MESSAGE(DescriptorID(*parse_priv) == *op_desc_id, "DescriptorID() " + DescriptorID(*parse_priv).ToString() + " does not match for priv " + prv);
    }

    // Check that both can be serialized with private key back to the private version, but not without private key.
    if (!(flags & MISSING_PRIVKEYS)) {
        std::string prv1;
        BOOST_CHECK(parse_priv->ToPrivateString(keys_priv, prv1));
        if (expected_prv) {
            BOOST_CHECK_MESSAGE(EqualDescriptor(*expected_prv, prv1), "Private ser: " + prv1 + "Private desc: " + *expected_prv);
        } else {
            BOOST_CHECK_MESSAGE(EqualDescriptor(prv, prv1), "Private ser: " + prv1 + " Private desc: " + prv);
        }
        BOOST_CHECK(!parse_priv->ToPrivateString(keys_pub, prv1));
        BOOST_CHECK(parse_pub->ToPrivateString(keys_priv, prv1));
        if (expected_prv) {
            BOOST_CHECK(EqualDescriptor(*expected_prv, prv1));
            BOOST_CHECK_MESSAGE(EqualDescriptor(*expected_prv, prv1), "Private ser: " + prv1 + " Private desc: " + *expected_prv);
        } else {
            BOOST_CHECK_MESSAGE(EqualDescriptor(prv, prv1), "Private ser: " + prv1 + " Private desc: " + prv);
        }
        BOOST_CHECK(!parse_pub->ToPrivateString(keys_pub, prv1));

        // Check that both can ExpandPrivate and get the same SigningProviders
        FlatSigningProvider priv_prov;
        parse_priv->ExpandPrivate(0, keys_priv, priv_prov);

        FlatSigningProvider pub_prov;
        parse_pub->ExpandPrivate(0, keys_priv, pub_prov);

        BOOST_CHECK_MESSAGE(EqualSigningProviders(priv_prov, pub_prov), "Private desc: " + prv + " Pub desc: " + pub);
    }

    // Check that private can produce the normalized descriptors
    std::string norm1;
    BOOST_CHECK(parse_priv->ToNormalizedString(keys_priv, norm1));
    BOOST_CHECK_MESSAGE(EqualDescriptor(norm1, norm_pub), "priv->ToNormalizedString(): " + norm1 + " Norm. desc: " + norm_pub);
    BOOST_CHECK(parse_pub->ToNormalizedString(keys_priv, norm1));
    BOOST_CHECK_MESSAGE(EqualDescriptor(norm1, norm_pub), "pub->ToNormalizedString(): " + norm1 + " Norm. desc: " + norm_pub);

    // Check whether IsRange on both returns the expected result
    BOOST_CHECK_EQUAL(parse_pub->IsRange(), (flags & RANGE) != 0);
    BOOST_CHECK_EQUAL(parse_priv->IsRange(), (flags & RANGE) != 0);

    // * For ranged descriptors,  the `scripts` parameter is a list of expected result outputs, for subsequent
    //   positions to evaluate the descriptors on (so the first element of `scripts` is for evaluating the
    //   descriptor at 0; the second at 1; and so on). To verify this, we evaluate the descriptors once for
    //   each element in `scripts`.
    // * For non-ranged descriptors, we evaluate the descriptors at positions 0, 1, and 2, but expect the
    //   same result in each case, namely the first element of `scripts`. Because of that, the size of
    //   `scripts` must be one in that case.
    if (!(flags & RANGE)) assert(scripts.size() == 1);
    size_t max = (flags & RANGE) ? scripts.size() : 3;

    // Iterate over the position we'll evaluate the descriptors in.
    for (size_t i = 0; i < max; ++i) {
        // Call the expected result scripts `ref`.
        const auto& ref = scripts[(flags & RANGE) ? i : 0];
        // When t=0, evaluate the `prv` descriptor; when t=1, evaluate the `pub` descriptor.
        for (int t = 0; t < 2; ++t) {
            // When the descriptor is hardened, evaluate with access to the private keys inside.
            const FlatSigningProvider& key_provider = (flags & HARDENED) ? keys_priv : keys_pub;

            // Evaluate the descriptor selected by `t` in position `i`.
            FlatSigningProvider script_provider, script_provider_cached;
            std::vector<CScript> spks, spks_cached;
            DescriptorCache desc_cache;
            BOOST_CHECK((t ? parse_priv : parse_pub)->Expand(i, key_provider, spks, script_provider, &desc_cache));

            // Compare the output with the expected result.
            BOOST_CHECK_EQUAL(spks.size(), ref.size());

            // Try to expand again using cached data, and compare.
            BOOST_CHECK(parse_pub->ExpandFromCache(i, desc_cache, spks_cached, script_provider_cached));
            BOOST_CHECK(spks == spks_cached);
            BOOST_CHECK(GetKeyData(script_provider, flags) == GetKeyData(script_provider_cached, flags));
            BOOST_CHECK(script_provider.scripts == script_provider_cached.scripts);
            BOOST_CHECK(GetKeyOriginData(script_provider, flags) == GetKeyOriginData(script_provider_cached, flags));

            // Check whether keys are in the cache
            const auto& der_xpub_cache = desc_cache.GetCachedDerivedExtPubKeys();
            const auto& parent_xpub_cache = desc_cache.GetCachedParentExtPubKeys();
            size_t num_xpubs = CountXpubs(pub1);
            size_t num_unique_xpubs = CountUniqueXpubs(pub1);
            if (flags & MUSIG_DERIVATION) {
                // Deriving from the aggregate will include the synthetic xpub of the aggregate in the caches and SigningProviders.
                num_xpubs++;
                num_unique_xpubs++;
            }
            if ((flags & RANGE) && !(flags & (DERIVE_HARDENED))) {
                // For ranged, unhardened derivation, None of the keys in origins should appear in the cache but the cache should have parent keys
                // But we can derive one level from each of those parent keys and find them all
                BOOST_CHECK(der_xpub_cache.empty());
                BOOST_CHECK(parent_xpub_cache.size() > 0);
                std::set<CPubKey> pubkeys;
                for (const auto& xpub_pair : parent_xpub_cache) {
                    const CExtPubKey& xpub = xpub_pair.second;
                    CExtPubKey der;
                    BOOST_CHECK(xpub.Derive(der, i));
                    pubkeys.insert(der.pubkey);
                }
                int count_pks = 0;
                for (const auto& origin_pair : script_provider_cached.origins) {
                    const CPubKey& pk = origin_pair.second.first;
                    count_pks += pubkeys.count(pk);
                }
                if (flags & MUSIG_DERIVATION) {
                    if (!(flags & MIXED_MUSIG)) {
                        BOOST_CHECK_EQUAL(count_pks, 1);
                    }
                    BOOST_CHECK_EQUAL(num_xpubs, pubkeys.size());
                } else {
                    if (flags & MUSIG) count_pks++; // One extra key for the aggregate key that is not in the cache
                    if (flags & MIXED_PUBKEYS) {
                        BOOST_CHECK_EQUAL(num_xpubs, count_pks);
                    } else {
                        BOOST_CHECK_EQUAL(script_provider_cached.origins.size(), count_pks);
                    }
                }
            } else if (num_xpubs > 0) {
                // For ranged, hardened derivation, or not ranged, but has an xpub, all of the keys should appear in the cache
                BOOST_CHECK_EQUAL(der_xpub_cache.size() + parent_xpub_cache.size(), num_xpubs);
                if (!(flags & MIXED_PUBKEYS)) {
                    if (flags & UNIQUE_XPUBS) {
                        BOOST_CHECK_EQUAL(script_provider_cached.origins.size(), num_unique_xpubs);
                    } else {
                        BOOST_CHECK_EQUAL(script_provider_cached.origins.size(), num_xpubs);
                    }
                }
                // Get all of the derived pubkeys
                std::set<CPubKey> pubkeys;
                for (const auto& xpub_map_pair : der_xpub_cache) {
                    for (const auto& xpub_pair : xpub_map_pair.second) {
                        const CExtPubKey& xpub = xpub_pair.second;
                        pubkeys.insert(xpub.pubkey);
                    }
                }
                // Derive one level from all of the parents
                for (const auto& xpub_pair : parent_xpub_cache) {
                    const CExtPubKey& xpub = xpub_pair.second;
                    pubkeys.insert(xpub.pubkey);
                    CExtPubKey der;
                    BOOST_CHECK(xpub.Derive(der, i));
                    pubkeys.insert(der.pubkey);
                }
                int count_pks = 0;
                for (const auto& origin_pair : script_provider_cached.origins) {
                    const CPubKey& pk = origin_pair.second.first;
                    count_pks += pubkeys.count(pk);
                }
                if (flags & MUSIG_DERIVATION && !(flags & MIXED_PUBKEYS)) {
                    // pubkeys is one key per xpub + one derived key per xpub
                    BOOST_CHECK_EQUAL(2 * count_pks, pubkeys.size());
                    if (flags & UNIQUE_XPUBS) {
                        BOOST_CHECK_EQUAL(2 * num_unique_xpubs, pubkeys.size());
                    } else {
                        BOOST_CHECK_EQUAL(2 * num_xpubs, pubkeys.size());
                    }
                } else {
                    if (flags & MUSIG) count_pks++; // One extra key for the aggregate key that is not in the cache
                    if (flags & MIXED_PUBKEYS) {
                        BOOST_CHECK_EQUAL(num_xpubs, count_pks);
                    } else {
                        BOOST_CHECK_EQUAL(script_provider_cached.origins.size(), count_pks);
                    }
                }
            } else if (!(flags & MIXED_PUBKEYS)) {
                // Only const pubkeys, nothing should be cached
                BOOST_CHECK(der_xpub_cache.empty());
                BOOST_CHECK(parent_xpub_cache.empty());
            }

            // Make sure we can expand using cached xpubs for unhardened derivation
            if (!(flags & DERIVE_HARDENED)) {
                // Evaluate the descriptor at i + 1
                FlatSigningProvider script_provider1, script_provider_cached1;
                std::vector<CScript> spks1, spk1_from_cache;
                BOOST_CHECK((t ? parse_priv : parse_pub)->Expand(i + 1, key_provider, spks1, script_provider1, nullptr));

                // Try again but use the cache from expanding i. That cache won't have the pubkeys for i + 1, but will have the parent xpub for derivation.
                BOOST_CHECK(parse_pub->ExpandFromCache(i + 1, desc_cache, spk1_from_cache, script_provider_cached1));
                BOOST_CHECK(spks1 == spk1_from_cache);
                BOOST_CHECK(GetKeyData(script_provider1, flags) == GetKeyData(script_provider_cached1, flags));
                BOOST_CHECK(script_provider1.scripts == script_provider_cached1.scripts);
                BOOST_CHECK(GetKeyOriginData(script_provider1, flags) == GetKeyOriginData(script_provider_cached1, flags));
            }

            // For each of the produced scripts, verify solvability, and when possible, try to sign a transaction spending it.
            for (size_t n = 0; n < spks.size(); ++n) {
                BOOST_CHECK_EQUAL(ref[n], HexStr(spks[n]));

                if (flags & (SIGNABLE | SIGNABLE_FAILS)) {
                    CMutableTransaction spend;
                    spend.nLockTime = spender_nlocktime;
                    spend.vin.resize(1);
                    spend.vin[0].nSequence = spender_nsequence;
                    spend.vout.resize(1);
                    std::vector<CTxOut> utxos(1);
                    PrecomputedTransactionData txdata;
                    txdata.Init(spend, std::move(utxos), /*force=*/true);
                    MutableTransactionSignatureCreator creator{spend, 0, CAmount{0}, &txdata, SIGHASH_DEFAULT};
                    SignatureData sigdata;
                    // We assume there is no collision between the hashes (eg h1=SHA256(SHA256(x)) and h2=SHA256(x))
                    sigdata.sha256_preimages = preimages;
                    sigdata.hash256_preimages = preimages;
                    sigdata.ripemd160_preimages = preimages;
                    sigdata.hash160_preimages = preimages;
                    const auto prod_sig_res = ProduceSignature(FlatSigningProvider{keys_priv}.Merge(FlatSigningProvider{script_provider}), creator, spks[n], sigdata);
                    BOOST_CHECK_MESSAGE(prod_sig_res == !(flags & SIGNABLE_FAILS), prv);
                }

                /* Infer a descriptor from the generated script, and verify its solvability and that it roundtrips. */
                auto inferred = InferDescriptor(spks[n], script_provider);
                BOOST_CHECK_EQUAL(inferred->IsSolvable(), !(flags & UNSOLVABLE));
                std::vector<CScript> spks_inferred;
                FlatSigningProvider provider_inferred;
                BOOST_CHECK(inferred->Expand(0, provider_inferred, spks_inferred, provider_inferred));
                BOOST_CHECK_EQUAL(spks_inferred.size(), 1U);
                BOOST_CHECK(spks_inferred[0] == spks[n]);
                BOOST_CHECK_EQUAL(InferDescriptor(spks_inferred[0], provider_inferred)->IsSolvable(), !(flags & UNSOLVABLE));
                BOOST_CHECK(GetKeyOriginData(provider_inferred, flags) == GetKeyOriginData(script_provider, flags));
            }

            // Test whether the observed key path is present in the 'paths' variable (which contains expected, unobserved paths),
            // and then remove it from that set.
            for (const auto& origin : script_provider.origins) {
                BOOST_CHECK_MESSAGE(paths.count(origin.second.second.path), "Unexpected key path: " + prv);
                left_paths.erase(origin.second.second.path);
            }
        }
    }

    // Verify no expected paths remain that were not observed.
    BOOST_CHECK_MESSAGE(left_paths.empty(), "Not all expected key paths found: " + prv);
}

void Check(const std::string& prv, const std::string& pub, const std::string& norm_pub, int flags,
           const std::vector<std::vector<std::string>>& scripts, const std::optional<OutputType>& type, std::optional<uint256> op_desc_id = std::nullopt,
           const std::set<std::vector<uint32_t>>& paths = ONLY_EMPTY, uint32_t spender_nlocktime=0,
           uint32_t spender_nsequence=CTxIn::SEQUENCE_FINAL, std::map<std::vector<uint8_t>, std::vector<uint8_t>> preimages={},
           std::optional<std::string> expected_prv = std::nullopt, std::optional<std::string> expected_pub = std::nullopt, int desc_index = 0)
{
    // Do not replace apostrophes with 'h' in prv and pub
    DoCheck(prv, pub, norm_pub, flags, scripts, type, op_desc_id, paths, /*replace_apostrophe_with_h_in_prv=*/false,
            /*replace_apostrophe_with_h_in_pub=*/false, /*spender_nlocktime=*/spender_nlocktime,
            /*spender_nsequence=*/spender_nsequence, /*preimages=*/preimages,
            expected_prv, expected_pub, desc_index);

    // Replace apostrophes with 'h' both in prv and in pub, if apostrophes are found in both
    if (prv.find('\'') != std::string::npos && pub.find('\'') != std::string::npos) {
        DoCheck(prv, pub, norm_pub, flags, scripts, type, op_desc_id, paths, /*replace_apostrophe_with_h_in_prv=*/true,
                /*replace_apostrophe_with_h_in_pub=*/true, /*spender_nlocktime=*/spender_nlocktime,
                /*spender_nsequence=*/spender_nsequence, /*preimages=*/preimages,
                expected_prv, expected_pub, desc_index);
    }
}

void CheckMultipath(const std::string& prv,
        const std::string& pub,
        const std::vector<std::string>& expanded_prvs,
        const std::vector<std::string>& expanded_pubs,
        const std::vector<std::string>& expanded_norm_pubs,
        int flags,
        const std::vector<std::vector<std::vector<std::string>>>& scripts,
        const std::optional<OutputType>& type,
        const std::vector<std::set<std::vector<uint32_t>>>& paths)
{
    assert(expanded_prvs.size() == expanded_pubs.size());
    assert(expanded_prvs.size() == expanded_norm_pubs.size());
    assert(expanded_prvs.size() == scripts.size());
    assert(expanded_prvs.size() == paths.size());
    for (size_t i = 0; i < expanded_prvs.size(); ++i) {
        Check(prv, pub, expanded_norm_pubs.at(i), flags, scripts.at(i), type, std::nullopt, paths.at(i),
              /*spender_nlocktime=*/0, /*spender_nsequence=*/CTxIn::SEQUENCE_FINAL, /*preimages=*/{},
              expanded_prvs.at(i), expanded_pubs.at(i), i);
    }

    // The descriptor for each path must be standalone. They should not share common references. Test this
    // by parsing a multipath descriptor expression, deallocating all but one of the descriptors and making
    // sure we can perform operations on it.
    FlatSigningProvider prov, out;
    std::string error;
    const auto desc{[&](){
        auto parsed{Parse(pub, prov, error)};
        assert(parsed.size() > 1);
        return std::move(parsed.at(0));
    }()};
    desc->ToString();
    std::vector<CScript> out_scripts;
    desc->Expand(0, prov, out_scripts, out);
}

void CheckInferDescriptor(const std::string& script_hex, const std::string& expected_desc, const std::vector<std::string>& hex_scripts, const std::vector<std::pair<std::string, std::string>>& origin_pubkeys)
{
    std::vector<unsigned char> script_bytes{ParseHex(script_hex)};
    const CScript& script{script_bytes.begin(), script_bytes.end()};

    FlatSigningProvider provider;
    for (const std::string& prov_script_hex : hex_scripts) {
        std::vector<unsigned char> prov_script_bytes{ParseHex(prov_script_hex)};
        const CScript& prov_script{prov_script_bytes.begin(), prov_script_bytes.end()};
        provider.scripts.emplace(CScriptID(prov_script), prov_script);
    }
    for (const auto& [pubkey_hex, origin_str] : origin_pubkeys) {
        CPubKey origin_pubkey{ParseHex(pubkey_hex)};
        provider.pubkeys.emplace(origin_pubkey.GetID(), origin_pubkey);

        if (!origin_str.empty()) {
            KeyOriginInfo info;
            std::span<const char> origin_sp{origin_str};
            std::vector<std::span<const char>> origin_split = Split(origin_sp, "/");
            std::string fpr_str(origin_split[0].begin(), origin_split[0].end());
            auto fpr_bytes = ParseHex(fpr_str);
            std::copy(fpr_bytes.begin(), fpr_bytes.end(), info.fingerprint);
            for (size_t i = 1; i < origin_split.size(); ++i) {
                std::span<const char> elem = origin_split[i];
                bool hardened = false;
                if (elem.size() > 0) {
                    const char last = elem[elem.size() - 1];
                    if (last == '\'' || last == 'h') {
                        elem = elem.first(elem.size() - 1);
                        hardened = true;
                    }
                }
                const uint32_t p{*Assert(ToIntegral<uint32_t>(std::string_view{elem.begin(), elem.end()}))};
                info.path.push_back(p | (((uint32_t)hardened) << 31));
            }

            provider.origins.emplace(origin_pubkey.GetID(), std::make_pair(origin_pubkey, info));
        }
    }

    std::string checksum{GetDescriptorChecksum(expected_desc)};

    std::unique_ptr<Descriptor> desc = InferDescriptor(script, provider);
    BOOST_CHECK_EQUAL(desc->ToString(), expected_desc + "#" + checksum);
}

}

BOOST_FIXTURE_TEST_SUITE(descriptor_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(descriptor_test)
{
    // Basic single-key compressed
    Check("combo(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)", "combo(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", "combo(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", SIGNABLE, {{"2103a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bdac","76a9149a1c78a507689f6f54b847ad1cef1e614ee23f1e88ac","00149a1c78a507689f6f54b847ad1cef1e614ee23f1e","a91484ab21b1b2fd065d4504ff693d832434b6108d7b87"}}, std::nullopt, /*op_desc_id=*/uint256{"8ef71f7b6ac0918663f6706be469d6109f6922e21f484009d7ab49d77da36e8b"});
    Check("pk(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)", "pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", "pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", SIGNABLE, {{"2103a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bdac"}}, std::nullopt, /*op_desc_id=*/uint256{"5fe175b43c58ac2cdde40521dc7d1dbc607f3dd795d00770206f4fdefb42229e"});
    Check("pkh([deadbeef/1/2'/3/4']L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)", "pkh([deadbeef/1/2'/3/4']03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", "pkh([deadbeef/1/2h/3/4h]03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", SIGNABLE, {{"76a9149a1c78a507689f6f54b847ad1cef1e614ee23f1e88ac"}}, OutputType::LEGACY, /*op_desc_id=*/uint256{"628130ae0530f2b24faf1ad2744a83568ac0ffac43e703e30c00d5f137869b84"}, {{1,0x80000002UL,3,0x80000004UL}});
    Check("wpkh(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)", "wpkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", "wpkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", SIGNABLE, {{"00149a1c78a507689f6f54b847ad1cef1e614ee23f1e"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"4a47b7f497721bf3fc48c69a5d22bc1f3617238649a8ba7cb96fbd92fec84a7e"});
    Check("sh(wpkh(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1))", "sh(wpkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", "sh(wpkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", SIGNABLE, {{"a91484ab21b1b2fd065d4504ff693d832434b6108d7b87"}}, OutputType::P2SH_SEGWIT, /*op_desc_id=*/uint256{"a13112753066b5c59473a87c5771b1694a10531944a60e0ab2d7ad66ecb65bcd"});
    Check("tr(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", SIGNABLE | XONLY_KEYS, {{"512077aab6e066f8a7419c5ab714c12c67d25007ed55a43cadcacb4d7a970a093f11"}}, OutputType::BECH32M, /*op_desc_id=*/uint256{"4290f3d017b270be53b91abc56d9d2f23a3ff361d5b1d39550ba011e6cae0da5"});
    CheckUnparsable("sh(wpkh(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY2))", "sh(wpkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5))", "wpkh(): Pubkey '03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5' is invalid"); // Invalid pubkey
    CheckUnparsable("pkh(deadbeef/1/2'/3/4']L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)", "pkh(deadbeef/1/2h/3/4h]03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", "pkh(): Key origin start '[ character expected but not found, got 'd' instead"); // Missing start bracket in key origin
    CheckUnparsable("pkh([deadbeef]/1/2'/3/4']L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)", "pkh([deadbeef]/1/2'/3/4']03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", "pkh(): Multiple ']' characters found for a single pubkey"); // Multiple end brackets in key origin

    // Basic single-key uncompressed
    Check("combo(5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "combo(04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "combo(04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)",SIGNABLE, {{"4104a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235ac","76a914b5bd079c4d57cc7fc28ecf8213a6b791625b818388ac"}}, std::nullopt, /*op_desc_id=*/uint256{"33f6bb5d32c04e9d9e5466a8212836743bd5466aa0b8d5331ce8aa0812371ffd"});
    Check("pk(5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "pk(04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "pk(04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", SIGNABLE, {{"4104a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235ac"}}, std::nullopt, /*op_desc_id=*/uint256{"52306fc1f5d0cb78aacea9d3933092be9252adc27b146f97c16a94d6fcdb652e"});
    Check("pkh(5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "pkh(04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "pkh(04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", SIGNABLE, {{"76a914b5bd079c4d57cc7fc28ecf8213a6b791625b818388ac"}}, OutputType::LEGACY, /*op_desc_id=*/uint256{"36657e8690d4015032da1a8c1e37b315c3f7ccb010e6ada12967878711962991"});
    CheckUnparsable("wpkh(5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "wpkh(04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "wpkh(): Uncompressed keys are not allowed"); // No uncompressed keys in witness
    CheckUnparsable("wsh(pk(5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss))", "wsh(pk(04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235))", "pk(): Uncompressed keys are not allowed"); // No uncompressed keys in witness
    CheckUnparsable("sh(wpkh(5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss))", "sh(wpkh(04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235))", "wpkh(): Uncompressed keys are not allowed"); // No uncompressed keys in witness

    // Equivalent single-key hybrid is not allowed
    CheckUnparsable("", "combo(07a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "combo(): Hybrid public keys are not allowed");
    CheckUnparsable("", "pk(0623542d61708e3fc48ba78fbe8fcc983ba94a520bc33f82b8e45e51dbc47af2726bcf181925eee1bdd868b109314f3ea92a6fc23d6b66057d3acfba04d6b08b58)", "pk(): Hybrid public keys are not allowed");
    CheckUnparsable("", "pkh(07a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "pkh(): Hybrid public keys are not allowed");

    // Some unconventional single-key constructions
    Check("sh(pk(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1))", "sh(pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", "sh(pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", SIGNABLE, {{"a9141857af51a5e516552b3086430fd8ce55f7c1a52487"}}, OutputType::LEGACY);
    Check("sh(pkh(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1))", "sh(pkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", "sh(pkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", SIGNABLE, {{"a9141a31ad23bf49c247dd531a623c2ef57da3c400c587"}}, OutputType::LEGACY);
    Check("wsh(pk(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1))", "wsh(pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", "wsh(pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", SIGNABLE, {{"00202e271faa2325c199d25d22e1ead982e45b64eeb4f31e73dbdf41bd4b5fec23fa"}}, OutputType::BECH32);
    Check("wsh(pkh(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1))", "wsh(pkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", "wsh(pkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", SIGNABLE, {{"0020338e023079b91c58571b20e602d7805fb808c22473cbc391a41b1bd3a192e75b"}}, OutputType::BECH32);
    Check("sh(wsh(pk(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)))", "sh(wsh(pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)))", "sh(wsh(pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)))", SIGNABLE, {{"a91472d0c5a3bfad8c3e7bd5303a72b94240e80b6f1787"}}, OutputType::P2SH_SEGWIT);
    Check("sh(wsh(pkh(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)))", "sh(wsh(pkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)))", "sh(wsh(pkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)))", SIGNABLE, {{"a914b61b92e2ca21bac1e72a3ab859a742982bea960a87"}}, OutputType::P2SH_SEGWIT);
    Check("tr(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5,{pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5),{pk(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1),pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5)}})", "tr(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5,{pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5),{pk(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd),pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5)}})", "tr(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5,{pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5),{pk(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd),pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5)}})", XONLY_KEYS | SIGNABLE | MISSING_PRIVKEYS, {{"51201497ae16f30dacb88523ed9301bff17773b609e8a90518a3f96ea328a47d1500"}}, OutputType::BECH32M);

    // Versions with BIP32 derivations
    Check("combo([01234567]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)", "combo([01234567]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)", "combo([01234567]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)", SIGNABLE, {{"2102d2b36900396c9282fa14628566582f206a5dd0bcc8d5e892611806cafb0301f0ac","76a91431a507b815593dfc51ffc7245ae7e5aee304246e88ac","001431a507b815593dfc51ffc7245ae7e5aee304246e","a9142aafb926eb247cb18240a7f4c07983ad1f37922687"}}, std::nullopt, /*op_desc_id=*/uint256{"7f127f7861594e3ede449eb47a7cc623c753acc0b0f0fc03fbb2dac636c20d6f"});
    Check("pk(xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0)", "pk(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0)", "pk(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0)", DEFAULT, {{"210379e45b3cf75f9c5f9befd8e9506fb962f6a9d185ac87001ec44a8d3df8d4a9e3ac"}}, std::nullopt, /*op_desc_id=*/uint256{"0e54cf04f2bb8d607e2241d611d169c6f7d78f0ab1f15a80642192a19fbdb7cc"}, {{0}});
    Check("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0)", "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0)", "pkh([bd16bee5/2147483647h]xpub69H7F5dQzmVd3vPuLKtcXJziMEQByuDidnX3YdwgtNsecY5HRGtAAQC5mXTt4dsv9RzyjgDjAQs9VGVV6ydYCHnprc9vvaA5YtqWyL6hyds/0)", HARDENED, {{"76a914ebdc90806a9c4356c1c88e42216611e1cb4c1c1788ac"}}, OutputType::LEGACY, /*op_desc_id=*/uint256{"35a5cf511e941a35b9cb0cf515d3ef887aa4246db87d6af463265a386ad856fe"}, {{0xFFFFFFFFUL,0}});

    Check("wpkh([ffffffff/13']xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*)", "wpkh([ffffffff/13']xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*)", "wpkh([ffffffff/13h]xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*)", RANGE, {{"0014326b2249e3a25d5dc60935f044ee835d090ba859"},{"0014af0bd98abc2f2cae66e36896a39ffe2d32984fb7"},{"00141fa798efd1cbf95cebf912c031b8a4a6e9fb9f27"}}, OutputType::BECH32, /*op_desc_id=*/std::nullopt, {{0x8000000DUL, 1, 2, 0}, {0x8000000DUL, 1, 2, 1}, {0x8000000DUL, 1, 2, 2}});
    Check("sh(wpkh(xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "sh(wpkh(xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))", "sh(wpkh(xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*h))", RANGE | HARDENED | DERIVE_HARDENED, {{"a9149a4d9901d6af519b2a23d4a2f51650fcba87ce7b87"},{"a914bed59fc0024fae941d6e20a3b44a109ae740129287"},{"a9148483aa1116eb9c05c482a72bada4b1db24af654387"}}, OutputType::P2SH_SEGWIT, /*op_desc_id=*/std::nullopt, {{10, 20, 30, 40, 0x80000000UL}, {10, 20, 30, 40, 0x80000001UL}, {10, 20, 30, 40, 0x80000002UL}});
    Check("combo(xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8RfQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334/*)", "combo(xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV/*)", "combo(xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV/*)", RANGE, {{"2102df12b7035bdac8e3bab862a3a83d06ea6b17b6753d52edecba9be46f5d09e076ac","76a914f90e3178ca25f2c808dc76624032d352fdbdfaf288ac","0014f90e3178ca25f2c808dc76624032d352fdbdfaf2","a91408f3ea8c68d4a7585bf9e8bda226723f70e445f087"},{"21032869a233c9adff9a994e4966e5b821fd5bac066da6c3112488dc52383b4a98ecac","76a914a8409d1b6dfb1ed2a3e8aa5e0ef2ff26b15b75b788ac","0014a8409d1b6dfb1ed2a3e8aa5e0ef2ff26b15b75b7","a91473e39884cb71ae4e5ac9739e9225026c99763e6687"}}, std::nullopt, /*op_desc_id=*/std::nullopt, {{0}, {1}});
    Check("tr(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/0/*,pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/1/*))", "tr(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/0/*,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/*))", "tr(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/0/*,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/*))", XONLY_KEYS | RANGE, {{"512078bc707124daa551b65af74de2ec128b7525e10f374dc67b64e00ce0ab8b3e12"}, {"512001f0a02a17808c20134b78faab80ef93ffba82261ccef0a2314f5d62b6438f11"}, {"512021024954fcec88237a9386fce80ef2ced5f1e91b422b26c59ccfc174c8d1ad25"}}, OutputType::BECH32M, /*op_desc_id=*/std::nullopt, {{0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 1}, {1, 2}});
    // Mixed xpubs and const pubkeys
    Check("wsh(multi(1,xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8RfQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334/0,L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1))","wsh(multi(1,xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV/0,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))","wsh(multi(1,xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV/0,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", MIXED_PUBKEYS, {{"0020cb155486048b23a6da976d4c6fe071a2dbc8a7b57aaf225b8955f2e2a27b5f00"}},OutputType::BECH32, /*op_desc_id=*/uint256{"88af8e5951779aa054dfe1071ef0f7266ba1c5558487bfd8525c95010fc0aba2"},{{0},{}});
    // Mixed range xpubs and const pubkeys
    Check("multi(1,xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8RfQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334/*,L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)","multi(1,xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV/*,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)","multi(1,xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV/*,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", RANGE | MIXED_PUBKEYS, {{"512102df12b7035bdac8e3bab862a3a83d06ea6b17b6753d52edecba9be46f5d09e0762103a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd52ae"},{"5121032869a233c9adff9a994e4966e5b821fd5bac066da6c3112488dc52383b4a98ec2103a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd52ae"},{"5121035d30b6c66dc1e036c45369da8287518cf7e0d6ed1e2b905171c605708f14ca032103a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd52ae"}}, std::nullopt, /*op_desc_id=*/std::nullopt,{{2},{1},{0},{}});

    CheckUnparsable("combo([012345678]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)", "combo([012345678]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)", "combo(): Fingerprint is not 4 bytes (9 characters instead of 8 characters)"); // Too long key fingerprint
    CheckUnparsable("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483648)", "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483648)", "pkh(): Key path value 2147483648 is out of range"); // BIP 32 path element overflow
    CheckUnparsable("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/1aa)", "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/1aa)", "pkh(): Key path value '1aa' is not a valid uint32"); // Path is not valid uint
    CheckUnparsable("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/+1)", "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/+1)", "pkh(): Key path value '+1' is not a valid uint32"); // Path is not valid uint
    Check("pkh([01234567/10/20]xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0)", "pkh([01234567/10/20]xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0)", "pkh([01234567/10/20/2147483647h]xpub69H7F5dQzmVd3vPuLKtcXJziMEQByuDidnX3YdwgtNsecY5HRGtAAQC5mXTt4dsv9RzyjgDjAQs9VGVV6ydYCHnprc9vvaA5YtqWyL6hyds/0)", HARDENED, {{"76a914ebdc90806a9c4356c1c88e42216611e1cb4c1c1788ac"}}, OutputType::LEGACY, /*op_desc_id=*/std::nullopt, {{10, 20, 0xFFFFFFFFUL, 0}});

    // Multipath versions with BIP32 derivations
    CheckMultipath("pk(xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/<0;1>)",
            "pk(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/<0;1>)",
            {
                "pk(xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0)",
                "pk(xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/1)",
            },
            {
                "pk(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0)",
                "pk(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/1)",
            },
            {
                "pk(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0)",
                "pk(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/1)",
            },
            DEFAULT,
            {
                {{"210379e45b3cf75f9c5f9befd8e9506fb962f6a9d185ac87001ec44a8d3df8d4a9e3ac"}},
                {{"21034f8d02282ac6786737d0f37f0df7655f49daa24843bc7de3f4ea88603d26d10aac"}},
            },
            std::nullopt,
            {
                {{0}},
                {{1}},
            }
    );
    CheckMultipath("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/<2147483647h;0>/0)",
            "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/<2147483647h;0>/0)",
            {
                "pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647h/0)",
                "pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/0/0)",
            },
            {
                "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647h/0)",
                "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/0/0)",
            },
            {
                "pkh([bd16bee5/2147483647h]xpub69H7F5dQzmVd3vPuLKtcXJziMEQByuDidnX3YdwgtNsecY5HRGtAAQC5mXTt4dsv9RzyjgDjAQs9VGVV6ydYCHnprc9vvaA5YtqWyL6hyds/0)",
                "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/0/0)",
            },
            HARDENED,
            {
                {{"76a914ebdc90806a9c4356c1c88e42216611e1cb4c1c1788ac"}},
                {{"76a914f103317b9f0b758a62cb3879281d23e3b1deb90d88ac"}},
            },
            OutputType::LEGACY,
            {
                {{0xFFFFFFFFUL,0}},
                {{0,0}},
            }
    );
    CheckMultipath("wpkh([ffffffff/13h]xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/<1;3>/2/*)",
            "wpkh([ffffffff/13h]xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/<1;3>/2/*)",
            {
                "wpkh([ffffffff/13h]xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*)",
                "wpkh([ffffffff/13h]xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/3/2/*)",
            },
            {
                "wpkh([ffffffff/13h]xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*)",
                "wpkh([ffffffff/13h]xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/3/2/*)",
            },
            {
                "wpkh([ffffffff/13h]xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*)",
                "wpkh([ffffffff/13h]xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/3/2/*)",
            },
            RANGE,
            {
                {{"0014326b2249e3a25d5dc60935f044ee835d090ba859"},{"0014af0bd98abc2f2cae66e36896a39ffe2d32984fb7"},{"00141fa798efd1cbf95cebf912c031b8a4a6e9fb9f27"}},
                {{"001426183882ef9c76b9a44386e9b387f33cee7c3a2d"},{"001447c1b9dc215c3f8b47e572981eb97528768cde4e"},{"00146e92cbaa397f9caeccf9a049460258af6ccd67e2"}},
            },
            OutputType::BECH32,
            {
                {{0x8000000DUL, 1, 2, 0}, {0x8000000DUL, 1, 2, 1}, {0x8000000DUL, 1, 2, 2}},
                {{0x8000000DUL, 3, 2, 0}, {0x8000000DUL, 3, 2, 1}, {0x8000000DUL, 3, 2, 2}},
            }
    );
    CheckMultipath("sh(wpkh(xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/<10;100h>/20/30/40/*h))",
            "sh(wpkh(xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/<10;100h>/20/30/40/*h))",
            {
                "sh(wpkh(xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*h))",
                "sh(wpkh(xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/100h/20/30/40/*h))",
            },
            {
                "sh(wpkh(xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*h))",
                "sh(wpkh(xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/100h/20/30/40/*h))",
            },
            {
                "sh(wpkh(xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*h))",
                "sh(wpkh(xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/100h/20/30/40/*h))",
            },
            RANGE | HARDENED | DERIVE_HARDENED,
            {
                {{"a9149a4d9901d6af519b2a23d4a2f51650fcba87ce7b87"},{"a914bed59fc0024fae941d6e20a3b44a109ae740129287"},{"a9148483aa1116eb9c05c482a72bada4b1db24af654387"}},
                {{"a91470192039cb9529aadf4e53e46d9ac6a13790865787"},{"a914855859faffabf1e4ed2bb7411ab66f4599b1abd287"},{"a9148f2cfd4b486de247c44684160da164617ccf2c2687"}},
            },
            OutputType::P2SH_SEGWIT,
            {
                {{10, 20, 30, 40, 0x80000000UL}, {10, 20, 30, 40, 0x80000001UL}, {10, 20, 30, 40, 0x80000002UL}},
                {{0x80000064UL, 20, 30, 40, 0x80000000UL}, {0x80000064UL, 20, 30, 40, 0x80000001UL}, {0x80000064UL, 20, 30, 40, 0x80000002UL}},
            }
    );
    CheckMultipath("multi(2,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<1;2>/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/<3;4>/0/*)",
            "multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<1;2>/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/<3;4>/0/*)",
            {
                "multi(2,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/1/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/3/0/*)",
                "multi(2,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/2/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/4/0/*)",
            },
            {
                "multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/3/0/*)",
                "multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/2/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/4/0/*)",
            },
            {
                "multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/3/0/*)",
                "multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/2/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/4/0/*)",
            },
            RANGE,
            {
                {{"522103095e95d8c50ae3f3fea93fa8e983f710489f60ff681a658c06eba64622c824b121020443e9e729b42628913f1a69b46b7d43ff87c46e86140e12ee420d7e2e8caf8c52ae"},{"5221027512d6bd74e24eeb1ad752d5be800adc5886ded11c5293a9a701db83658b526a2102371e912dea5fefa56158908fe4c9f66bc925a8939b10f3821e8f8be797b9ca8252ae"},{"522102cc9fd211dc0a1c8bb7a106ff831be0e253bc992f21d08fb8a6fd43fae51b9b892103e43eddc68afc9746c9d09ce0bf8067b4f2416287abbc422ed1ac300673b1104952ae"}},
                {{"5221031c0517fff3d483f06ca769bd2326bf30aca1c4de278e676e6ef760c3301244c6210316e171ff4f82dc62ad3f0d84c97865034fc5041eaa508b48c1d7af77f301c8bd52ae"},{"52210240f010ccff4202ade2ef87756f6b9af57bbf5ebcb0393b949e6e5d45d30bff36210229057a7e03510b8cb66727fab3f47a52a02ea94eae03e7c2e81b72a26781bfde52ae"},{"5221034052522058a07b647bd08fa1a9eaedae0222eac76ddd122ff8096ec969398de721038cb8180dd4c956848bcf191e45aaf297146207559fb8737881156aadaf13704152ae"}},
            },
            std::nullopt,
            {
                {{1, 0}, {1, 1}, {1, 2}, {3, 0, 0}, {3, 0, 1}, {3, 0, 2}},
                {{2, 0}, {2, 1}, {2, 2}, {4, 0, 0}, {4, 0, 1}, {4, 0, 2}},
            }
    );
    CheckMultipath("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/<0;1;2>)",
            "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/<0;1;2>)",
            {
                "pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/0)",
                "pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/1)",
                "pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2)",
            },
            {
                "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/0)",
                "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/1)",
                "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2)",
            },
            {
                "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/0)",
                "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/1)",
                "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2)",
            },
            DEFAULT,
            {
                {{"76a9145a61ff8eb7aaca3010db97ebda76121610b7809688ac"}},
                {{"76a9142f792a782cf4adbb321fe646c8e220563649b8fa88ac"}},
                {{"76a914dcc5b93b52177d78f97b3f2d259b9a86ee1403b188ac"}},
            },
            OutputType::LEGACY,
            {
                {{0}},
                {{1}},
                {{2}},
            }
    );
    CheckMultipath("sh(multi(2,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<1;2;3>/0/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0/*,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/<3;4;5>/*))",
            "sh(multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<1;2;3>/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/<3;4;5>/*))",
            {
                "sh(multi(2,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/1/0/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0/*,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/3/*))",
                "sh(multi(2,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/2/0/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0/*,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/4/*))",
                "sh(multi(2,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/3/0/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0/*,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/5/*))",
            },
            {
                "sh(multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/3/*))",
                "sh(multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/2/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/4/*))",
                "sh(multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/3/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/5/*))",
            },
            {
                "sh(multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/3/*))",
                "sh(multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/2/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/4/*))",
                "sh(multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/3/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/5/*))",
            },
            RANGE,
            {
                {{"a914689cdf7de5836ec04fb971d128cc84858f73e11487"},{"a9142ea7dbaf0a77ee19f080cdacb3e13560e3cd9cf587"},{"a9143da854021f58f5e2d3ff6bb4fcd0ced877deb34987"}},
                {{"a9143dd613d162e89b83369bbf08e5f1977cfdc9b02787"},{"a91449eef5d3df5c465b20a630c66058fe689082d8e187"},{"a91492be56babf54ea2109c577f799ba6d73948e8c3287"}},
                {{"a9140093ca92097bdf557fbb0570bb77e1efd2e7529c87"},{"a914e4d0419d3d2ce8f921a800796811ff5462bb151887"},{"a914997bf69841ac444190dc02f5e6031dd6f8feab4587"}},
            },
            OutputType::LEGACY,
            {
                {{1, 0, 0}, {1, 0, 1}, {1, 0, 2}, {0, 0}, {0, 1}, {0, 2}, {0, 0, 3, 0}, {0, 0, 3, 1}, {0, 0, 3, 2}},
                {{2, 0, 0}, {2, 0, 1}, {2, 0, 2}, {0, 0}, {0, 1}, {0, 2}, {0, 0, 4, 0}, {0, 0, 4, 1}, {0, 0, 4, 2}},
                {{3, 0, 0}, {3, 0, 1}, {3, 0, 2}, {0, 0}, {0, 1}, {0, 2}, {0, 0, 5, 0}, {0, 0, 5, 1}, {0, 0, 5, 2}},
            }
    );
    CheckMultipath("tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/<6;7;8>/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<1;2;3>/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/<3;4;5>/*)})",
            "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/<6;7;8>/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<1;2;3>/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/<3;4;5>/*)})",
            {
                "tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/6/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/1/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/3/*)})",
                "tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/7/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/2/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/4/*)})",
                "tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/8/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/3/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/5/*)})",
            },
            {
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/6/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/3/*)})",
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/7/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/2/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/4/*)})",
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/8/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/3/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/5/*)})",
            },
            {
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/6/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/3/*)})",
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/7/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/2/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/4/*)})",
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/8/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/3/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/5/*)})",
            },
            XONLY_KEYS | RANGE,
            {
                {{"5120993e5b1d71d14cbb0a90c57ea0fed1d5bf77d5804cee206c3dbd7e4d2c67d869"},{"51207b8f629f6d406b92ffa6284f5545085eafb837c469018b715755f619b587163b"},{"512061f52925826e51e4615007557ddbea55b22c817909d7ebcfd3c454c634643ece"}},
                {{"5120633808b2156d0a6597e8b07f59c387bb4c2d5c02c4cb98f1802748e64c6abf5f"},{"5120fc5f06ded29328c170bf7e49e71c9cc8699befa2bf0a2a80802a1f32ab72d291"},{"5120fd05e2227e0dac972dff9941e332db8461bedc320c2a74def44e469ddbad9d21"}},
                {{"51205d19538c7c0901520eb712d079ae6eebed4f691021da466dc24e9575d9815ad0"},{"5120b9fc348ede2b7b9fb1f84c21741bb36bb3fa0905d0bc9417e07145d3142673f7"},{"51203a655bc5181b12efac82a5a5d1d0969b2ceb92c6fc37f505fdf00ee8afa09b33"}},
            },
            OutputType::BECH32M,
            {
                {{6, 0}, {6, 1}, {6, 2}, {1, 0, 0}, {1, 0, 1}, {1, 0, 2}, {0, 0, 3, 0}, {0, 0, 3, 1}, {0, 0, 3, 2}},
                {{7, 0}, {7, 1}, {7, 2}, {2, 0, 0}, {2, 0, 1}, {2, 0, 2}, {0, 0, 4, 0}, {0, 0, 4, 1}, {0, 0, 4, 2}},
                {{8, 0}, {8, 1}, {8, 2}, {3, 0, 0}, {3, 0, 1}, {3, 0, 2}, {0, 0, 5, 0}, {0, 0, 5, 1}, {0, 0, 5, 2}},
            }
    );
    CheckMultipath("tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/6/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<1;2;3>/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/<3;4;5>/*)})",
            "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/6/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<1;2;3>/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/<3;4;5>/*)})",
            {
                "tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/6/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/1/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/3/*)})",
                "tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/6/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/2/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/4/*)})",
                "tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/6/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/3/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/5/*)})",
            },
            {
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/6/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/3/*)})",
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/6/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/2/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/4/*)})",
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/6/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/3/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/5/*)})",
            },
            {
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/6/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/3/*)})",
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/6/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/2/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/4/*)})",
                "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/6/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/3/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/5/*)})",
            },
            XONLY_KEYS | RANGE,
            {
                {{"5120993e5b1d71d14cbb0a90c57ea0fed1d5bf77d5804cee206c3dbd7e4d2c67d869"},{"51207b8f629f6d406b92ffa6284f5545085eafb837c469018b715755f619b587163b"},{"512061f52925826e51e4615007557ddbea55b22c817909d7ebcfd3c454c634643ece"}},
                {{"5120c481a8ada38d1070094f62af526d4f8aae2eb1e44d1fd961be6a25198b4da77b"},{"512034a2d31c091905e62def62b575b88beff41723d83acb02dfada2e73d9c529b40"},{"5120e0ecc278655b092962ded92a5781bd8e86e8408055de05f121e107fa211e5dfb"}},
                {{"51206052cff5efc848e4b38a947803943eb1eb0076523eec1041969851ebcd265555"},{"512009ed83d758c0bdd36e225c961810761c7a360533434a41a17bba709e331e6cd1"},{"5120fcd77851ebaac37564b87e9b351c54492a8fbb1d6afdf7f3a9317703a002b22b"}},
            },
            OutputType::BECH32M,
            {
                {{6, 0}, {6, 1}, {6, 2}, {1, 0, 0}, {1, 0, 1}, {1, 0, 2}, {0, 0, 3, 0}, {0, 0, 3, 1}, {0, 0, 3, 2}},
                {{6, 0}, {6, 1}, {6, 2}, {2, 0, 0}, {2, 0, 1}, {2, 0, 2}, {0, 0, 4, 0}, {0, 0, 4, 1}, {0, 0, 4, 2}},
                {{6, 0}, {6, 1}, {6, 2}, {3, 0, 0}, {3, 0, 1}, {3, 0, 2}, {0, 0, 5, 0}, {0, 0, 5, 1}, {0, 0, 5, 2}},
            }
    );
    CheckMultipath("wsh(or_d(pk([2557c640/48h/1h/0h/2h]xprv9ws7hGFQPbDga6QrETbFM7Gqc7m15UNoJ7KF5kkDhyZCBcANAqRMUCytQ4JM1nLSYvGyFjg6TvBEfNrN3znaFdb67jQoQ7z9kFnd4BUUJiE/<0;1>/*),and_v(v:pkh([00aabb22/48h/1h/0h/2h]xprv9ws7hGFQPbDgbCvNcYVfGeGK8UTSFmAho4iAXZf13yQVJmHuKHN9oMXCv7zsJn8Dcqvqy2iugFWAhDdUUX6r5VLNWkRTpxVoQJ6DbzY9eYa/<0;1>/*),older(2))))",
            "wsh(or_d(pk([2557c640/48h/1h/0h/2h]xpub6ArU6mnJDxmynaVKLV8FiFDaA9bVUw6efLEqt99qGK6B4QVWiNjc21JNFKkXNjgT8NCUmpFpSSBrYFtWEAqGirbqT4J1bRFpWyAnYdzmZUm/<0;1>/*),and_v(v:pkh([00aabb22/48h/1h/0h/2h]xpub6ArU6mnJDxmyogzqia2fdnD3gWHvfDtZAHdmKx4ccJwUBZd3rpgQM9qgmPAn1mqT2yh81uvGGohMkg3fNLoXZzn7sRo4a1X3KnCAVot2yuS/<0;1>/*),older(2))))",
            {
                "wsh(or_d(pk([2557c640/48h/1h/0h/2h]xprv9ws7hGFQPbDga6QrETbFM7Gqc7m15UNoJ7KF5kkDhyZCBcANAqRMUCytQ4JM1nLSYvGyFjg6TvBEfNrN3znaFdb67jQoQ7z9kFnd4BUUJiE/0/*),and_v(v:pkh([00aabb22/48h/1h/0h/2h]xprv9ws7hGFQPbDgbCvNcYVfGeGK8UTSFmAho4iAXZf13yQVJmHuKHN9oMXCv7zsJn8Dcqvqy2iugFWAhDdUUX6r5VLNWkRTpxVoQJ6DbzY9eYa/0/*),older(2))))",
                "wsh(or_d(pk([2557c640/48h/1h/0h/2h]xprv9ws7hGFQPbDga6QrETbFM7Gqc7m15UNoJ7KF5kkDhyZCBcANAqRMUCytQ4JM1nLSYvGyFjg6TvBEfNrN3znaFdb67jQoQ7z9kFnd4BUUJiE/1/*),and_v(v:pkh([00aabb22/48h/1h/0h/2h]xprv9ws7hGFQPbDgbCvNcYVfGeGK8UTSFmAho4iAXZf13yQVJmHuKHN9oMXCv7zsJn8Dcqvqy2iugFWAhDdUUX6r5VLNWkRTpxVoQJ6DbzY9eYa/1/*),older(2))))",
            },
            {
                "wsh(or_d(pk([2557c640/48h/1h/0h/2h]xpub6ArU6mnJDxmynaVKLV8FiFDaA9bVUw6efLEqt99qGK6B4QVWiNjc21JNFKkXNjgT8NCUmpFpSSBrYFtWEAqGirbqT4J1bRFpWyAnYdzmZUm/0/*),and_v(v:pkh([00aabb22/48h/1h/0h/2h]xpub6ArU6mnJDxmyogzqia2fdnD3gWHvfDtZAHdmKx4ccJwUBZd3rpgQM9qgmPAn1mqT2yh81uvGGohMkg3fNLoXZzn7sRo4a1X3KnCAVot2yuS/0/*),older(2))))",
                "wsh(or_d(pk([2557c640/48h/1h/0h/2h]xpub6ArU6mnJDxmynaVKLV8FiFDaA9bVUw6efLEqt99qGK6B4QVWiNjc21JNFKkXNjgT8NCUmpFpSSBrYFtWEAqGirbqT4J1bRFpWyAnYdzmZUm/1/*),and_v(v:pkh([00aabb22/48h/1h/0h/2h]xpub6ArU6mnJDxmyogzqia2fdnD3gWHvfDtZAHdmKx4ccJwUBZd3rpgQM9qgmPAn1mqT2yh81uvGGohMkg3fNLoXZzn7sRo4a1X3KnCAVot2yuS/1/*),older(2))))"
            },
            {
                "wsh(or_d(pk([2557c640/48h/1h/0h/2h]xpub6ArU6mnJDxmynaVKLV8FiFDaA9bVUw6efLEqt99qGK6B4QVWiNjc21JNFKkXNjgT8NCUmpFpSSBrYFtWEAqGirbqT4J1bRFpWyAnYdzmZUm/0/*),and_v(v:pkh([00aabb22/48h/1h/0h/2h]xpub6ArU6mnJDxmyogzqia2fdnD3gWHvfDtZAHdmKx4ccJwUBZd3rpgQM9qgmPAn1mqT2yh81uvGGohMkg3fNLoXZzn7sRo4a1X3KnCAVot2yuS/0/*),older(2))))",
                "wsh(or_d(pk([2557c640/48h/1h/0h/2h]xpub6ArU6mnJDxmynaVKLV8FiFDaA9bVUw6efLEqt99qGK6B4QVWiNjc21JNFKkXNjgT8NCUmpFpSSBrYFtWEAqGirbqT4J1bRFpWyAnYdzmZUm/1/*),and_v(v:pkh([00aabb22/48h/1h/0h/2h]xpub6ArU6mnJDxmyogzqia2fdnD3gWHvfDtZAHdmKx4ccJwUBZd3rpgQM9qgmPAn1mqT2yh81uvGGohMkg3fNLoXZzn7sRo4a1X3KnCAVot2yuS/1/*),older(2))))"
            },
            RANGE,
            {
                {{"0020538436a60f2a638ea9e1e1342e9b93374aa7ec559ff0a805b3a185d4ba855d7f"},{"00203a588d107d604b6913201c7c1e1722f07a0f8fb3a382744f17b9ae5f6ccfcdd7"},{"0020d30fb375f7c491a208e77c7b5d0996ca14cf4a770c2ab5981f915c0e4565c74a"}},
                {{"002072b5fc3a691c48fdbaf485f27e787b4094055d4b434c90c81ed1090f3d48733b"},{"0020a9ccdf4496e5d60db4704b27494d9d74f54a16c180ff954a43ce5e3aa465113a"},{"0020d17e21820a0069ca87049513eca763f08a74b586724441e7d76fc5142bcc327c"}},
            },
            OutputType::BECH32,
            {
                {{0x80000000UL + 48, 0x80000000UL + 1, 0x80000000UL, 0x80000000UL + 2, 0, 0}, {0x80000000UL + 48, 0x80000000UL + 1, 0x80000000UL, 0x80000000UL + 2, 0, 1}, {0x80000000UL + 48, 0x80000000UL + 1, 0x80000000UL, 0x80000000UL + 2, 0, 2}},
                {{0x80000000UL + 48, 0x80000000UL + 1, 0x80000000UL, 0x80000000UL + 2, 1, 0}, {0x80000000UL + 48, 0x80000000UL + 1, 0x80000000UL, 0x80000000UL + 2, 1, 1}, {0x80000000UL + 48, 0x80000000UL + 1, 0x80000000UL, 0x80000000UL + 2, 1, 2}},
            }
    );
    CheckMultipath("tr(xprv9yYge4PS54XkYT9KiLfCRwc8Jeuz8DucxQGtuEecJZYhKNiqbPxYHTPzXtskmzWBqdqkRAGsghNmZzNsfU2wstaB3XjDQFPv567aQSSuPyo,l:pk(xprvA1ADjaN8H3HGnZSmt4VF7YdWoV9oNq8jhqhurxsrYycBAFK555cECoaY22KWt6BTRNLuvobW5VQTF89PN3iA485LAg7epazevPyjCa4xTzd/<2;3>))",
        "tr(xpub6CY33ZvKuS63kwDnpNCCo5YrrgkUXgdUKdCVhd4Dru5gCB3z8wGnqFiUP98Za5pYSYF5KmvBHTY3Ra8FAJGggzBjuHS69WzN8gscPupuZwK,l:pk(xpub6E9a95u27Qqa13XEz62FUgaFMWzHnHrb54dWfMHU7K9A33eDccvUkbu1sHYoByHAgJdR326rWqn9pGZgZHz1afDprW5gGwS4gUX8Ri6aGPZ/<2;3>))",
        {
            "tr(xprv9yYge4PS54XkYT9KiLfCRwc8Jeuz8DucxQGtuEecJZYhKNiqbPxYHTPzXtskmzWBqdqkRAGsghNmZzNsfU2wstaB3XjDQFPv567aQSSuPyo,l:pk(xprvA1ADjaN8H3HGnZSmt4VF7YdWoV9oNq8jhqhurxsrYycBAFK555cECoaY22KWt6BTRNLuvobW5VQTF89PN3iA485LAg7epazevPyjCa4xTzd/2))",
            "tr(xprv9yYge4PS54XkYT9KiLfCRwc8Jeuz8DucxQGtuEecJZYhKNiqbPxYHTPzXtskmzWBqdqkRAGsghNmZzNsfU2wstaB3XjDQFPv567aQSSuPyo,l:pk(xprvA1ADjaN8H3HGnZSmt4VF7YdWoV9oNq8jhqhurxsrYycBAFK555cECoaY22KWt6BTRNLuvobW5VQTF89PN3iA485LAg7epazevPyjCa4xTzd/3))",
        },
        {
            "tr(xpub6CY33ZvKuS63kwDnpNCCo5YrrgkUXgdUKdCVhd4Dru5gCB3z8wGnqFiUP98Za5pYSYF5KmvBHTY3Ra8FAJGggzBjuHS69WzN8gscPupuZwK,l:pk(xpub6E9a95u27Qqa13XEz62FUgaFMWzHnHrb54dWfMHU7K9A33eDccvUkbu1sHYoByHAgJdR326rWqn9pGZgZHz1afDprW5gGwS4gUX8Ri6aGPZ/2))",
            "tr(xpub6CY33ZvKuS63kwDnpNCCo5YrrgkUXgdUKdCVhd4Dru5gCB3z8wGnqFiUP98Za5pYSYF5KmvBHTY3Ra8FAJGggzBjuHS69WzN8gscPupuZwK,l:pk(xpub6E9a95u27Qqa13XEz62FUgaFMWzHnHrb54dWfMHU7K9A33eDccvUkbu1sHYoByHAgJdR326rWqn9pGZgZHz1afDprW5gGwS4gUX8Ri6aGPZ/3))",
        },
        {
            "tr(xpub6CY33ZvKuS63kwDnpNCCo5YrrgkUXgdUKdCVhd4Dru5gCB3z8wGnqFiUP98Za5pYSYF5KmvBHTY3Ra8FAJGggzBjuHS69WzN8gscPupuZwK,l:pk(xpub6E9a95u27Qqa13XEz62FUgaFMWzHnHrb54dWfMHU7K9A33eDccvUkbu1sHYoByHAgJdR326rWqn9pGZgZHz1afDprW5gGwS4gUX8Ri6aGPZ/2))",
            "tr(xpub6CY33ZvKuS63kwDnpNCCo5YrrgkUXgdUKdCVhd4Dru5gCB3z8wGnqFiUP98Za5pYSYF5KmvBHTY3Ra8FAJGggzBjuHS69WzN8gscPupuZwK,l:pk(xpub6E9a95u27Qqa13XEz62FUgaFMWzHnHrb54dWfMHU7K9A33eDccvUkbu1sHYoByHAgJdR326rWqn9pGZgZHz1afDprW5gGwS4gUX8Ri6aGPZ/3))",
        },
        XONLY_KEYS,
        {
            {{"512094cb097990da64eebbad7b979b1326f3cbe356357abf4deb4c4ff80c7acbe902"}},
            {{"5120f091450b88c606f5cbc3f0cebe89e00bc5dd27f92e22f54da06439bc0c401f41"}},
        },
        OutputType::BECH32M,
        {
            {{2}, {}},
            {{3}, {}},
        }
    );
    CheckMultipath("tr(xprv9yYge4PS54XkYT9KiLfCRwc8Jeuz8DucxQGtuEecJZYhKNiqbPxYHTPzXtskmzWBqdqkRAGsghNmZzNsfU2wstaB3XjDQFPv567aQSSuPyo/<2;3>,l:pk(xprvA1ADjaN8H3HGnZSmt4VF7YdWoV9oNq8jhqhurxsrYycBAFK555cECoaY22KWt6BTRNLuvobW5VQTF89PN3iA485LAg7epazevPyjCa4xTzd))",
            "tr(xpub6CY33ZvKuS63kwDnpNCCo5YrrgkUXgdUKdCVhd4Dru5gCB3z8wGnqFiUP98Za5pYSYF5KmvBHTY3Ra8FAJGggzBjuHS69WzN8gscPupuZwK/<2;3>,l:pk(xpub6E9a95u27Qqa13XEz62FUgaFMWzHnHrb54dWfMHU7K9A33eDccvUkbu1sHYoByHAgJdR326rWqn9pGZgZHz1afDprW5gGwS4gUX8Ri6aGPZ))",
            {
                "tr(xprv9yYge4PS54XkYT9KiLfCRwc8Jeuz8DucxQGtuEecJZYhKNiqbPxYHTPzXtskmzWBqdqkRAGsghNmZzNsfU2wstaB3XjDQFPv567aQSSuPyo/2,l:pk(xprvA1ADjaN8H3HGnZSmt4VF7YdWoV9oNq8jhqhurxsrYycBAFK555cECoaY22KWt6BTRNLuvobW5VQTF89PN3iA485LAg7epazevPyjCa4xTzd))",
                "tr(xprv9yYge4PS54XkYT9KiLfCRwc8Jeuz8DucxQGtuEecJZYhKNiqbPxYHTPzXtskmzWBqdqkRAGsghNmZzNsfU2wstaB3XjDQFPv567aQSSuPyo/3,l:pk(xprvA1ADjaN8H3HGnZSmt4VF7YdWoV9oNq8jhqhurxsrYycBAFK555cECoaY22KWt6BTRNLuvobW5VQTF89PN3iA485LAg7epazevPyjCa4xTzd))",
            },
            {
                "tr(xpub6CY33ZvKuS63kwDnpNCCo5YrrgkUXgdUKdCVhd4Dru5gCB3z8wGnqFiUP98Za5pYSYF5KmvBHTY3Ra8FAJGggzBjuHS69WzN8gscPupuZwK/2,l:pk(xpub6E9a95u27Qqa13XEz62FUgaFMWzHnHrb54dWfMHU7K9A33eDccvUkbu1sHYoByHAgJdR326rWqn9pGZgZHz1afDprW5gGwS4gUX8Ri6aGPZ))",
                "tr(xpub6CY33ZvKuS63kwDnpNCCo5YrrgkUXgdUKdCVhd4Dru5gCB3z8wGnqFiUP98Za5pYSYF5KmvBHTY3Ra8FAJGggzBjuHS69WzN8gscPupuZwK/3,l:pk(xpub6E9a95u27Qqa13XEz62FUgaFMWzHnHrb54dWfMHU7K9A33eDccvUkbu1sHYoByHAgJdR326rWqn9pGZgZHz1afDprW5gGwS4gUX8Ri6aGPZ))",
            },
            {
                "tr(xpub6CY33ZvKuS63kwDnpNCCo5YrrgkUXgdUKdCVhd4Dru5gCB3z8wGnqFiUP98Za5pYSYF5KmvBHTY3Ra8FAJGggzBjuHS69WzN8gscPupuZwK/2,l:pk(xpub6E9a95u27Qqa13XEz62FUgaFMWzHnHrb54dWfMHU7K9A33eDccvUkbu1sHYoByHAgJdR326rWqn9pGZgZHz1afDprW5gGwS4gUX8Ri6aGPZ))",
                "tr(xpub6CY33ZvKuS63kwDnpNCCo5YrrgkUXgdUKdCVhd4Dru5gCB3z8wGnqFiUP98Za5pYSYF5KmvBHTY3Ra8FAJGggzBjuHS69WzN8gscPupuZwK/3,l:pk(xpub6E9a95u27Qqa13XEz62FUgaFMWzHnHrb54dWfMHU7K9A33eDccvUkbu1sHYoByHAgJdR326rWqn9pGZgZHz1afDprW5gGwS4gUX8Ri6aGPZ))",
            },
            XONLY_KEYS,
            {
                {{"51200e3c14456bfa30f9f0bed6e55f35e1e9ca17c835e9f71b25bac0dfaab38ff2cd"}},
                {{"51202bdda29337ecaf8fcd5aa395febac6f99b8a866a0e8fb3f7bde2e24b1a7df2ba"}},
            },
            OutputType::BECH32M,
            {
                {{2}, {}},
                {{3}, {}},
            }
    );
    CheckUnparsable("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/<0;1>/<2;3>)", "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/<0;1>/<2;3>)", "pkh(): Multiple multipath key path specifiers found");
    CheckUnparsable("pkh([deadbeef/<0;1>]xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/0)", "pkh([deadbeef/<0;1>]xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/0)", "pkh(): Key path value \'<0;1>\' specifies multipath in a section where multipath is not allowed");
    CheckUnparsable("tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/6/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<1;2;3>/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/<3;4>/*)})", "tr(xpub6B4sSbNr8XFYXqqKB7PeUemqgEaVtCLjgd5Lf2VYtezSHozC7ffCvVNCyu9TCgHntRQdimjV3tHbxmNfocxtuh6saNtZEw91gjXLRhQ3Yar/6/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<1;2;3>/0/*),pk(xpub6AhFhZJJGt9YB8i85RfrJ8jT3T2FF5EejDCXqXfm1DAczFEXkk8HD3CXTg2TmKM8wTbSnSw3wPg5JuyLitUrpRmkjn2BQXyZnqJx16AGy94/0/0/<3;4>/*)})", "tr(): Multipath subscripts have mismatched lengths");
    CheckUnparsable("tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/<6;7;8;9>/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<1;2;3>/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/<3;4;5>/*)})", "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/<6;7;8;9>/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<1;2;3>/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/<3;4;5>/*)})", "tr(): Multipath subscripts have mismatched lengths");
    CheckUnparsable("tr(xprv9s21ZrQH143K2Zu2kTVKcQi9nKhfgJUkYqG73wXsHuhATm1wkt6kcSZeTYEw2PL7krZtJopEYDvBdYWdAai3n3TWUTCVfHvPHqTYJv7smYe/<6;7>/*,{pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<1;2;3>/0/*),pk(xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/<3;4;5>/*)})", "tr(xpub661MyMwAqRbcF3yVrV2KyYetLMYA5mCbv4BhrKwUrFE9LZM6JRR1AEt8Jq4V4C8LwtTke6YEEdCZqgXp85YRk2j74EfJKhe3QybQ9kcUjs4/<6;7>/*,{pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<1;2;3>/0/*),pk(xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/<3;4;5>/*)})", "tr(): Multipath internal key mismatches multipath subscripts lengths");
    CheckUnparsable("sh(multi(2,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<1;2;3>/0/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0/*,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/<3;4>/*))", "sh(multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<1;2;3>/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/<3;4>/*))", "multi(): Multipath derivation paths have mismatched lengths");
    CheckUnparsable("wpkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/<0>/*)", "wpkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/<0>/*)", "wpkh(): Multipath key path specifiers must have at least two items");
    CheckUnparsable("wsh(andor(pk(xprv9xGFvhWa1Koc8dmeEG5JXVfMaNBkioYscFGmn7yx8YnhFQYeydFfudxdKRzR5p7v1kip85ohB6eUQbPpAee9cFZu9M85G9X4ovPP4xw4xbM/0'/<0;1;2;3>/*),older(10000),pk(xprv9x9bas78RYwopceXTStT8vDuTiu6g1u91L6sG3DhHfDDXKPrYdcHcDuDw4Hv1kjZBWKoZnobUHrdoFxBPUMBTMruUs8HwzL8GxGA95MmZ7v/8/<0;1;2>/*)))", "wsh(andor(pk(xpub6BFcLD3TqhMuM7r7LHcJtdc68Q2F8GGiyUCNaWPZgtKg8CsoXAZvTSH7AhaCPnuuewjwzA2gbAm1y6uaDNNxa7JqTiL76cdioT5rxjgxWXF/0'/<0;1;2;3>/*),older(10000),pk(xpub6B8wzNe2FvW736izZURTW4Ae1kjb5UczNZ2U4RdJqzkCQ7j16AvYA2DhnL8Kb5FeWAZJ43NnGPdjpeSKvAeM8YGaqhCzpD743Uv6S87hfAt/8/<0;1;2>/*)))", "Miniscript: Multipath derivation paths have mismatched lengths");
    CheckUnparsable("wpkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/<>/*)", "wpkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/<>/*)", "wpkh(): Multipath key path specifiers must have at least two items");
    CheckUnparsable("wpkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/<0/*)", "wpkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/<0/*)", "wpkh(): Key path value '<0' is not a valid uint32");
    CheckUnparsable("wpkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/0>/*)", "wpkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/0>/*)", "wpkh(): Key path value '0>' is not a valid uint32");
    CheckUnparsable("wpkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/<0;>/*)", "wpkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/<0;>/*)", "wpkh(): Key path value '' is not a valid uint32");
    CheckUnparsable("wpkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/<;1>/*)", "wpkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/<;1>/*)", "wpkh(): Key path value '' is not a valid uint32");
    CheckUnparsable("wpkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/<0;1;>/*)", "wpkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/<0;1;>/*)", "wpkh(): Key path value '' is not a valid uint32");
    CheckUnparsable("wpkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/<1;1>/*)", "wpkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/<1;1>/*)", "wpkh(): Duplicated key path value 1 in multipath specifier");

    // Multisig constructions
    Check("multi(1,L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1,5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "multi(1,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "multi(1,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", SIGNABLE, {{"512103a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd4104a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea23552ae"}}, std::nullopt, /*op_desc_id=*/uint256{"b147e25eb4a9d3da4e86ed8e970d817563ae2cb9c71a756b11cfdeb4dc11b70c"});
    Check("sortedmulti(1,L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1,5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "sortedmulti(1,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "sortedmulti(1,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", SIGNABLE, {{"512103a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd4104a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea23552ae"}}, std::nullopt, /*op_desc_id=*/uint256{"62b59d1e32a62176ef7a17538f3b80c7d1afc53e5644eb753525bdb5d556486c"});
    Check("sortedmulti(1,5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss,L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)", "sortedmulti(1,04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", "sortedmulti(1,04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", SIGNABLE, {{"512103a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd4104a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea23552ae"}}, std::nullopt);
    Check("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))", "sh(multi(2,[00000000/111h/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))", DEFAULT, {{"a91445a9a622a8b0a1269944be477640eedc447bbd8487"}}, OutputType::LEGACY, /*op_desc_id=*/std::nullopt, {{0x8000006FUL,222},{0}});
    Check("sortedmulti(2,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0/0/*)", "sortedmulti(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/0/*)", "sortedmulti(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/0/*)", RANGE, {{"5221025d5fc65ebb8d44a5274b53bac21ff8307fec2334a32df05553459f8b1f7fe1b62102fbd47cc8034098f0e6a94c6aeee8528abf0a2153a5d8e46d325b7284c046784652ae"}, {"52210264fd4d1f5dea8ded94c61e9641309349b62f27fbffe807291f664e286bfbe6472103f4ece6dfccfa37b211eb3d0af4d0c61dba9ef698622dc17eecdf764beeb005a652ae"}, {"5221022ccabda84c30bad578b13c89eb3b9544ce149787e5b538175b1d1ba259cbb83321024d902e1a2fc7a8755ab5b694c575fce742c48d9ff192e63df5193e4c7afe1f9c52ae"}}, std::nullopt, /*op_desc_id=*/std::nullopt, {{0}, {1}, {2}, {0, 0, 0}, {0, 0, 1}, {0, 0, 2}});
    Check("wsh(multi(2,xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))", "wsh(multi(2,[bd16bee5/2147483647h]xpub69H7F5dQzmVd3vPuLKtcXJziMEQByuDidnX3YdwgtNsecY5HRGtAAQC5mXTt4dsv9RzyjgDjAQs9VGVV6ydYCHnprc9vvaA5YtqWyL6hyds/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*h))", HARDENED | RANGE | DERIVE_HARDENED, {{"0020b92623201f3bb7c3771d45b2ad1d0351ea8fbf8cfe0a0e570264e1075fa1948f"},{"002036a08bbe4923af41cf4316817c93b8d37e2f635dd25cfff06bd50df6ae7ea203"},{"0020a96e7ab4607ca6b261bfe3245ffda9c746b28d3f59e83d34820ec0e2b36c139c"}}, OutputType::BECH32, /*op_desc_id=*/std::nullopt, {{0xFFFFFFFFUL,0}, {1,2,0}, {1,2,1}, {1,2,2}, {10, 20, 30, 40, 0x80000000UL}, {10, 20, 30, 40, 0x80000001UL}, {10, 20, 30, 40, 0x80000002UL}});
    Check("sh(wsh(multi(16,KzoAz5CanayRKex3fSLQ2BwJpN7U52gZvxMyk78nDMHuqrUxuSJy,KwGNz6YCCQtYvFzMtrC6D3tKTKdBBboMrLTsjr2NYVBwapCkn7Mr,KxogYhiNfwxuswvXV66eFyKcCpm7dZ7TqHVqujHAVUjJxyivxQ9X,L2BUNduTSyZwZjwNHynQTF14mv2uz2NRq5n5sYWTb4FkkmqgEE9f,L1okJGHGn1kFjdXHKxXjwVVtmCMR2JA5QsbKCSpSb7ReQjezKeoD,KxDCNSST75HFPaW5QKpzHtAyaCQC7p9Vo3FYfi2u4dXD1vgMiboK,L5edQjFtnkcf5UWURn6UuuoFrabgDQUHdheKCziwN42aLwS3KizU,KzF8UWFcEC7BYTq8Go1xVimMkDmyNYVmXV5PV7RuDicvAocoPB8i,L3nHUboKG2w4VSJ5jYZ5CBM97oeK6YuKvfZxrefdShECcjEYKMWZ,KyjHo36dWkYhimKmVVmQTq3gERv3pnqA4xFCpvUgbGDJad7eS8WE,KwsfyHKRUTZPQtysN7M3tZ4GXTnuov5XRgjdF2XCG8faAPmFruRF,KzCUbGhN9LJhdeFfL9zQgTJMjqxdBKEekRGZX24hXdgCNCijkkap,KzgpMBwwsDLwkaC5UrmBgCYaBD2WgZ7PBoGYXR8KT7gCA9UTN5a3,KyBXTPy4T7YG4q9tcAM3LkvfRpD1ybHMvcJ2ehaWXaSqeGUxEdkP,KzJDe9iwJRPtKP2F2AoN6zBgzS7uiuAwhWCfGdNeYJ3PC1HNJ8M8,L1xbHrxynrqLKkoYc4qtoQPx6uy5qYXR5ZDYVYBSRmCV5piU3JG9)))","sh(wsh(multi(16,03669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0,0260b2003c386519fc9eadf2b5cf124dd8eea4c4e68d5e154050a9346ea98ce600,0362a74e399c39ed5593852a30147f2959b56bb827dfa3e60e464b02ccf87dc5e8,0261345b53de74a4d721ef877c255429961b7e43714171ac06168d7e08c542a8b8,02da72e8b46901a65d4374fe6315538d8f368557dda3a1dcf9ea903f3afe7314c8,0318c82dd0b53fd3a932d16e0ba9e278fcc937c582d5781be626ff16e201f72286,0297ccef1ef99f9d73dec9ad37476ddb232f1238aff877af19e72ba04493361009,02e502cfd5c3f972fe9a3e2a18827820638f96b6f347e54d63deb839011fd5765d,03e687710f0e3ebe81c1037074da939d409c0025f17eb86adb9427d28f0f7ae0e9,02c04d3a5274952acdbc76987f3184b346a483d43be40874624b29e3692c1df5af,02ed06e0f418b5b43a7ec01d1d7d27290fa15f75771cb69b642a51471c29c84acd,036d46073cbb9ffee90473f3da429abc8de7f8751199da44485682a989a4bebb24,02f5d1ff7c9029a80a4e36b9a5497027ef7f3e73384a4a94fbfe7c4e9164eec8bc,02e41deffd1b7cce11cde209a781adcffdabd1b91c0ba0375857a2bfd9302419f3,02d76625f7956a7fc505ab02556c23ee72d832f1bac391bcd2d3abce5710a13d06,0399eb0a5487515802dc14544cf10b3666623762fbed2ec38a3975716e2c29c232)))", "sh(wsh(multi(16,03669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0,0260b2003c386519fc9eadf2b5cf124dd8eea4c4e68d5e154050a9346ea98ce600,0362a74e399c39ed5593852a30147f2959b56bb827dfa3e60e464b02ccf87dc5e8,0261345b53de74a4d721ef877c255429961b7e43714171ac06168d7e08c542a8b8,02da72e8b46901a65d4374fe6315538d8f368557dda3a1dcf9ea903f3afe7314c8,0318c82dd0b53fd3a932d16e0ba9e278fcc937c582d5781be626ff16e201f72286,0297ccef1ef99f9d73dec9ad37476ddb232f1238aff877af19e72ba04493361009,02e502cfd5c3f972fe9a3e2a18827820638f96b6f347e54d63deb839011fd5765d,03e687710f0e3ebe81c1037074da939d409c0025f17eb86adb9427d28f0f7ae0e9,02c04d3a5274952acdbc76987f3184b346a483d43be40874624b29e3692c1df5af,02ed06e0f418b5b43a7ec01d1d7d27290fa15f75771cb69b642a51471c29c84acd,036d46073cbb9ffee90473f3da429abc8de7f8751199da44485682a989a4bebb24,02f5d1ff7c9029a80a4e36b9a5497027ef7f3e73384a4a94fbfe7c4e9164eec8bc,02e41deffd1b7cce11cde209a781adcffdabd1b91c0ba0375857a2bfd9302419f3,02d76625f7956a7fc505ab02556c23ee72d832f1bac391bcd2d3abce5710a13d06,0399eb0a5487515802dc14544cf10b3666623762fbed2ec38a3975716e2c29c232)))", SIGNABLE, {{"a9147fc63e13dc25e8a95a3cee3d9a714ac3afd96f1e87"}}, OutputType::P2SH_SEGWIT, /*op_desc_id=*/std::nullopt);
    Check("tr(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1,pk(KzoAz5CanayRKex3fSLQ2BwJpN7U52gZvxMyk78nDMHuqrUxuSJy))", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,pk(669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0))", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,pk(669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0))", SIGNABLE | XONLY_KEYS, {{"512017cf18db381d836d8923b1bdb246cfcd818da1a9f0e6e7907f187f0b2f937754"}}, OutputType::BECH32M, /*op_desc_id=*/uint256{"af482b44c10b737b678e1091584818372e169e2dc5219e2877fabe1b83ae467b"});
    Check("tr(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1,multi_a(1,KzoAz5CanayRKex3fSLQ2BwJpN7U52gZvxMyk78nDMHuqrUxuSJy))", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,multi_a(1,669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0))", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,multi_a(1,669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0))", SIGNABLE | XONLY_KEYS, {{"5120eb5bd3894327d75093891cc3a62506df7d58ec137fcd104cdd285d67816074f3"}}, OutputType::BECH32M);
    CheckUnparsable("sh(multi(16,KzoAz5CanayRKex3fSLQ2BwJpN7U52gZvxMyk78nDMHuqrUxuSJy,KwGNz6YCCQtYvFzMtrC6D3tKTKdBBboMrLTsjr2NYVBwapCkn7Mr,KxogYhiNfwxuswvXV66eFyKcCpm7dZ7TqHVqujHAVUjJxyivxQ9X,L2BUNduTSyZwZjwNHynQTF14mv2uz2NRq5n5sYWTb4FkkmqgEE9f,L1okJGHGn1kFjdXHKxXjwVVtmCMR2JA5QsbKCSpSb7ReQjezKeoD,KxDCNSST75HFPaW5QKpzHtAyaCQC7p9Vo3FYfi2u4dXD1vgMiboK,L5edQjFtnkcf5UWURn6UuuoFrabgDQUHdheKCziwN42aLwS3KizU,KzF8UWFcEC7BYTq8Go1xVimMkDmyNYVmXV5PV7RuDicvAocoPB8i,L3nHUboKG2w4VSJ5jYZ5CBM97oeK6YuKvfZxrefdShECcjEYKMWZ,KyjHo36dWkYhimKmVVmQTq3gERv3pnqA4xFCpvUgbGDJad7eS8WE,KwsfyHKRUTZPQtysN7M3tZ4GXTnuov5XRgjdF2XCG8faAPmFruRF,KzCUbGhN9LJhdeFfL9zQgTJMjqxdBKEekRGZX24hXdgCNCijkkap,KzgpMBwwsDLwkaC5UrmBgCYaBD2WgZ7PBoGYXR8KT7gCA9UTN5a3,KyBXTPy4T7YG4q9tcAM3LkvfRpD1ybHMvcJ2ehaWXaSqeGUxEdkP,KzJDe9iwJRPtKP2F2AoN6zBgzS7uiuAwhWCfGdNeYJ3PC1HNJ8M8,L1xbHrxynrqLKkoYc4qtoQPx6uy5qYXR5ZDYVYBSRmCV5piU3JG9))","sh(multi(16,03669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0,0260b2003c386519fc9eadf2b5cf124dd8eea4c4e68d5e154050a9346ea98ce600,0362a74e399c39ed5593852a30147f2959b56bb827dfa3e60e464b02ccf87dc5e8,0261345b53de74a4d721ef877c255429961b7e43714171ac06168d7e08c542a8b8,02da72e8b46901a65d4374fe6315538d8f368557dda3a1dcf9ea903f3afe7314c8,0318c82dd0b53fd3a932d16e0ba9e278fcc937c582d5781be626ff16e201f72286,0297ccef1ef99f9d73dec9ad37476ddb232f1238aff877af19e72ba04493361009,02e502cfd5c3f972fe9a3e2a18827820638f96b6f347e54d63deb839011fd5765d,03e687710f0e3ebe81c1037074da939d409c0025f17eb86adb9427d28f0f7ae0e9,02c04d3a5274952acdbc76987f3184b346a483d43be40874624b29e3692c1df5af,02ed06e0f418b5b43a7ec01d1d7d27290fa15f75771cb69b642a51471c29c84acd,036d46073cbb9ffee90473f3da429abc8de7f8751199da44485682a989a4bebb24,02f5d1ff7c9029a80a4e36b9a5497027ef7f3e73384a4a94fbfe7c4e9164eec8bc,02e41deffd1b7cce11cde209a781adcffdabd1b91c0ba0375857a2bfd9302419f3,02d76625f7956a7fc505ab02556c23ee72d832f1bac391bcd2d3abce5710a13d06,0399eb0a5487515802dc14544cf10b3666623762fbed2ec38a3975716e2c29c232))", "P2SH script is too large, 547 bytes is larger than 520 bytes"); // P2SH does not fit 16 compressed pubkeys in a redeemscript
    CheckUnparsable("wsh(multi(2,[aaaaaaaa][aaaaaaaa]xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,[aaaaaaaa][aaaaaaaa]xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647h/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*h))", "Multi: Multiple ']' characters found for a single pubkey"); // Double key origin descriptor
    CheckUnparsable("wsh(multi(2,[aaaagaaa]xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,[aaagaaaa]xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647h/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*h))", "Multi: Fingerprint 'aaagaaaa' is not hex"); // Non hex fingerprint
    CheckUnparsable("wsh(multi(2,[aaaaaaaa],xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,[aaaaaaaa],xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*h))", "Multi: No key provided"); // No public key with origin
    CheckUnparsable("wsh(multi(2,[aaaaaaa]xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,[aaaaaaa]xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647h/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*h))", "Multi: Fingerprint is not 4 bytes (7 characters instead of 8 characters)"); // Too short fingerprint
    CheckUnparsable("wsh(multi(2,[aaaaaaaaa]xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,[aaaaaaaaa]xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647h/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*h))", "Multi: Fingerprint is not 4 bytes (9 characters instead of 8 characters)"); // Too long fingerprint
    CheckUnparsable("multi(a,L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1,5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "multi(a,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "Multi threshold 'a' is not valid"); // Invalid threshold
    CheckUnparsable("multi(+1,L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1,5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "multi(+1,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "Multi threshold '+1' is not valid"); // Invalid threshold
    CheckUnparsable("multi(0,L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1,5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "multi(0,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "Multisig threshold cannot be 0, must be at least 1"); // Threshold of 0
    CheckUnparsable("multi(3,L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1,5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "multi(3,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235)", "Multisig threshold cannot be larger than the number of keys; threshold is 3 but only 2 keys specified"); // Threshold larger than number of keys
    CheckUnparsable("", "multi(0)()", "Multi: expected ',', got ')'");
    CheckUnparsable("", "multi(123)", "Cannot have 0 keys in multisig; must have between 1 and 20 keys, inclusive");
    CheckUnparsable("", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,multi_a(1))", "Cannot have 0 keys in multi_a; must have between 1 and 999 keys, inclusive");
    CheckUnparsable("multi(3,KzoAz5CanayRKex3fSLQ2BwJpN7U52gZvxMyk78nDMHuqrUxuSJy,KwGNz6YCCQtYvFzMtrC6D3tKTKdBBboMrLTsjr2NYVBwapCkn7Mr,KxogYhiNfwxuswvXV66eFyKcCpm7dZ7TqHVqujHAVUjJxyivxQ9X,L2BUNduTSyZwZjwNHynQTF14mv2uz2NRq5n5sYWTb4FkkmqgEE9f)", "multi(3,03669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0,0260b2003c386519fc9eadf2b5cf124dd8eea4c4e68d5e154050a9346ea98ce600,0362a74e399c39ed5593852a30147f2959b56bb827dfa3e60e464b02ccf87dc5e8,0261345b53de74a4d721ef877c255429961b7e43714171ac06168d7e08c542a8b8)", "Cannot have 4 pubkeys in bare multisig; only at most 3 pubkeys"); // Threshold larger than number of keys
    CheckUnparsable("sh(multi(16,KzoAz5CanayRKex3fSLQ2BwJpN7U52gZvxMyk78nDMHuqrUxuSJy,KwGNz6YCCQtYvFzMtrC6D3tKTKdBBboMrLTsjr2NYVBwapCkn7Mr,KxogYhiNfwxuswvXV66eFyKcCpm7dZ7TqHVqujHAVUjJxyivxQ9X,L2BUNduTSyZwZjwNHynQTF14mv2uz2NRq5n5sYWTb4FkkmqgEE9f,L1okJGHGn1kFjdXHKxXjwVVtmCMR2JA5QsbKCSpSb7ReQjezKeoD,KxDCNSST75HFPaW5QKpzHtAyaCQC7p9Vo3FYfi2u4dXD1vgMiboK,L5edQjFtnkcf5UWURn6UuuoFrabgDQUHdheKCziwN42aLwS3KizU,KzF8UWFcEC7BYTq8Go1xVimMkDmyNYVmXV5PV7RuDicvAocoPB8i,L3nHUboKG2w4VSJ5jYZ5CBM97oeK6YuKvfZxrefdShECcjEYKMWZ,KyjHo36dWkYhimKmVVmQTq3gERv3pnqA4xFCpvUgbGDJad7eS8WE,KwsfyHKRUTZPQtysN7M3tZ4GXTnuov5XRgjdF2XCG8faAPmFruRF,KzCUbGhN9LJhdeFfL9zQgTJMjqxdBKEekRGZX24hXdgCNCijkkap,KzgpMBwwsDLwkaC5UrmBgCYaBD2WgZ7PBoGYXR8KT7gCA9UTN5a3,KyBXTPy4T7YG4q9tcAM3LkvfRpD1ybHMvcJ2ehaWXaSqeGUxEdkP,KzJDe9iwJRPtKP2F2AoN6zBgzS7uiuAwhWCfGdNeYJ3PC1HNJ8M8,L1xbHrxynrqLKkoYc4qtoQPx6uy5qYXR5ZDYVYBSRmCV5piU3JG9,L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1))","sh(multi(16,03669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0,0260b2003c386519fc9eadf2b5cf124dd8eea4c4e68d5e154050a9346ea98ce600,0362a74e399c39ed5593852a30147f2959b56bb827dfa3e60e464b02ccf87dc5e8,0261345b53de74a4d721ef877c255429961b7e43714171ac06168d7e08c542a8b8,02da72e8b46901a65d4374fe6315538d8f368557dda3a1dcf9ea903f3afe7314c8,0318c82dd0b53fd3a932d16e0ba9e278fcc937c582d5781be626ff16e201f72286,0297ccef1ef99f9d73dec9ad37476ddb232f1238aff877af19e72ba04493361009,02e502cfd5c3f972fe9a3e2a18827820638f96b6f347e54d63deb839011fd5765d,03e687710f0e3ebe81c1037074da939d409c0025f17eb86adb9427d28f0f7ae0e9,02c04d3a5274952acdbc76987f3184b346a483d43be40874624b29e3692c1df5af,02ed06e0f418b5b43a7ec01d1d7d27290fa15f75771cb69b642a51471c29c84acd,036d46073cbb9ffee90473f3da429abc8de7f8751199da44485682a989a4bebb24,02f5d1ff7c9029a80a4e36b9a5497027ef7f3e73384a4a94fbfe7c4e9164eec8bc,02e41deffd1b7cce11cde209a781adcffdabd1b91c0ba0375857a2bfd9302419f3,02d76625f7956a7fc505ab02556c23ee72d832f1bac391bcd2d3abce5710a13d06,0399eb0a5487515802dc14544cf10b3666623762fbed2ec38a3975716e2c29c232,03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", "P2SH script is too large, 581 bytes is larger than 520 bytes"); // Cannot have more than 15 keys in a P2SH multisig, or we exceed maximum push size
    Check("wsh(multi(20,KzoAz5CanayRKex3fSLQ2BwJpN7U52gZvxMyk78nDMHuqrUxuSJy,KwGNz6YCCQtYvFzMtrC6D3tKTKdBBboMrLTsjr2NYVBwapCkn7Mr,KxogYhiNfwxuswvXV66eFyKcCpm7dZ7TqHVqujHAVUjJxyivxQ9X,L2BUNduTSyZwZjwNHynQTF14mv2uz2NRq5n5sYWTb4FkkmqgEE9f,L1okJGHGn1kFjdXHKxXjwVVtmCMR2JA5QsbKCSpSb7ReQjezKeoD,KxDCNSST75HFPaW5QKpzHtAyaCQC7p9Vo3FYfi2u4dXD1vgMiboK,L5edQjFtnkcf5UWURn6UuuoFrabgDQUHdheKCziwN42aLwS3KizU,KzF8UWFcEC7BYTq8Go1xVimMkDmyNYVmXV5PV7RuDicvAocoPB8i,L3nHUboKG2w4VSJ5jYZ5CBM97oeK6YuKvfZxrefdShECcjEYKMWZ,KyjHo36dWkYhimKmVVmQTq3gERv3pnqA4xFCpvUgbGDJad7eS8WE,KwsfyHKRUTZPQtysN7M3tZ4GXTnuov5XRgjdF2XCG8faAPmFruRF,KzCUbGhN9LJhdeFfL9zQgTJMjqxdBKEekRGZX24hXdgCNCijkkap,KzgpMBwwsDLwkaC5UrmBgCYaBD2WgZ7PBoGYXR8KT7gCA9UTN5a3,KyBXTPy4T7YG4q9tcAM3LkvfRpD1ybHMvcJ2ehaWXaSqeGUxEdkP,KzJDe9iwJRPtKP2F2AoN6zBgzS7uiuAwhWCfGdNeYJ3PC1HNJ8M8,L1xbHrxynrqLKkoYc4qtoQPx6uy5qYXR5ZDYVYBSRmCV5piU3JG9,KzRedjSwMggebB3VufhbzpYJnvHfHe9kPJSjCU5QpJdAW3NSZxYS,Kyjtp5858xL7JfeV4PNRCKy2t6XvgqNNepArGY9F9F1SSPqNEMs3,L2D4RLHPiHBidkHS8ftx11jJk1hGFELvxh8LoxNQheaGT58dKenW,KyLPZdwY4td98bKkXqEXTEBX3vwEYTQo1yyLjX2jKXA63GBpmSjv))","wsh(multi(20,03669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0,0260b2003c386519fc9eadf2b5cf124dd8eea4c4e68d5e154050a9346ea98ce600,0362a74e399c39ed5593852a30147f2959b56bb827dfa3e60e464b02ccf87dc5e8,0261345b53de74a4d721ef877c255429961b7e43714171ac06168d7e08c542a8b8,02da72e8b46901a65d4374fe6315538d8f368557dda3a1dcf9ea903f3afe7314c8,0318c82dd0b53fd3a932d16e0ba9e278fcc937c582d5781be626ff16e201f72286,0297ccef1ef99f9d73dec9ad37476ddb232f1238aff877af19e72ba04493361009,02e502cfd5c3f972fe9a3e2a18827820638f96b6f347e54d63deb839011fd5765d,03e687710f0e3ebe81c1037074da939d409c0025f17eb86adb9427d28f0f7ae0e9,02c04d3a5274952acdbc76987f3184b346a483d43be40874624b29e3692c1df5af,02ed06e0f418b5b43a7ec01d1d7d27290fa15f75771cb69b642a51471c29c84acd,036d46073cbb9ffee90473f3da429abc8de7f8751199da44485682a989a4bebb24,02f5d1ff7c9029a80a4e36b9a5497027ef7f3e73384a4a94fbfe7c4e9164eec8bc,02e41deffd1b7cce11cde209a781adcffdabd1b91c0ba0375857a2bfd9302419f3,02d76625f7956a7fc505ab02556c23ee72d832f1bac391bcd2d3abce5710a13d06,0399eb0a5487515802dc14544cf10b3666623762fbed2ec38a3975716e2c29c232,02bc2feaa536991d269aae46abb8f3772a5b3ad592314945e51543e7da84c4af6e,0318bf32e5217c1eb771a6d5ce1cd39395dff7ff665704f175c9a5451d95a2f2ca,02c681a6243f16208c2004bb81f5a8a67edfdd3e3711534eadeec3dcf0b010c759,0249fdd6b69768b8d84b4893f8ff84b36835c50183de20fcae8f366a45290d01fd))", "wsh(multi(20,03669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0,0260b2003c386519fc9eadf2b5cf124dd8eea4c4e68d5e154050a9346ea98ce600,0362a74e399c39ed5593852a30147f2959b56bb827dfa3e60e464b02ccf87dc5e8,0261345b53de74a4d721ef877c255429961b7e43714171ac06168d7e08c542a8b8,02da72e8b46901a65d4374fe6315538d8f368557dda3a1dcf9ea903f3afe7314c8,0318c82dd0b53fd3a932d16e0ba9e278fcc937c582d5781be626ff16e201f72286,0297ccef1ef99f9d73dec9ad37476ddb232f1238aff877af19e72ba04493361009,02e502cfd5c3f972fe9a3e2a18827820638f96b6f347e54d63deb839011fd5765d,03e687710f0e3ebe81c1037074da939d409c0025f17eb86adb9427d28f0f7ae0e9,02c04d3a5274952acdbc76987f3184b346a483d43be40874624b29e3692c1df5af,02ed06e0f418b5b43a7ec01d1d7d27290fa15f75771cb69b642a51471c29c84acd,036d46073cbb9ffee90473f3da429abc8de7f8751199da44485682a989a4bebb24,02f5d1ff7c9029a80a4e36b9a5497027ef7f3e73384a4a94fbfe7c4e9164eec8bc,02e41deffd1b7cce11cde209a781adcffdabd1b91c0ba0375857a2bfd9302419f3,02d76625f7956a7fc505ab02556c23ee72d832f1bac391bcd2d3abce5710a13d06,0399eb0a5487515802dc14544cf10b3666623762fbed2ec38a3975716e2c29c232,02bc2feaa536991d269aae46abb8f3772a5b3ad592314945e51543e7da84c4af6e,0318bf32e5217c1eb771a6d5ce1cd39395dff7ff665704f175c9a5451d95a2f2ca,02c681a6243f16208c2004bb81f5a8a67edfdd3e3711534eadeec3dcf0b010c759,0249fdd6b69768b8d84b4893f8ff84b36835c50183de20fcae8f366a45290d01fd))", SIGNABLE, {{"0020376bd8344b8b6ebe504ff85ef743eaa1aa9272178223bcb6887e9378efb341ac"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"2bb9d418ebdc3a75c465383985881527f3e5d6e520fb3efb152d4191b80e8412"}); // In P2WSH we can have up to 20 keys
    Check("sh(wsh(multi(20,KzoAz5CanayRKex3fSLQ2BwJpN7U52gZvxMyk78nDMHuqrUxuSJy,KwGNz6YCCQtYvFzMtrC6D3tKTKdBBboMrLTsjr2NYVBwapCkn7Mr,KxogYhiNfwxuswvXV66eFyKcCpm7dZ7TqHVqujHAVUjJxyivxQ9X,L2BUNduTSyZwZjwNHynQTF14mv2uz2NRq5n5sYWTb4FkkmqgEE9f,L1okJGHGn1kFjdXHKxXjwVVtmCMR2JA5QsbKCSpSb7ReQjezKeoD,KxDCNSST75HFPaW5QKpzHtAyaCQC7p9Vo3FYfi2u4dXD1vgMiboK,L5edQjFtnkcf5UWURn6UuuoFrabgDQUHdheKCziwN42aLwS3KizU,KzF8UWFcEC7BYTq8Go1xVimMkDmyNYVmXV5PV7RuDicvAocoPB8i,L3nHUboKG2w4VSJ5jYZ5CBM97oeK6YuKvfZxrefdShECcjEYKMWZ,KyjHo36dWkYhimKmVVmQTq3gERv3pnqA4xFCpvUgbGDJad7eS8WE,KwsfyHKRUTZPQtysN7M3tZ4GXTnuov5XRgjdF2XCG8faAPmFruRF,KzCUbGhN9LJhdeFfL9zQgTJMjqxdBKEekRGZX24hXdgCNCijkkap,KzgpMBwwsDLwkaC5UrmBgCYaBD2WgZ7PBoGYXR8KT7gCA9UTN5a3,KyBXTPy4T7YG4q9tcAM3LkvfRpD1ybHMvcJ2ehaWXaSqeGUxEdkP,KzJDe9iwJRPtKP2F2AoN6zBgzS7uiuAwhWCfGdNeYJ3PC1HNJ8M8,L1xbHrxynrqLKkoYc4qtoQPx6uy5qYXR5ZDYVYBSRmCV5piU3JG9,KzRedjSwMggebB3VufhbzpYJnvHfHe9kPJSjCU5QpJdAW3NSZxYS,Kyjtp5858xL7JfeV4PNRCKy2t6XvgqNNepArGY9F9F1SSPqNEMs3,L2D4RLHPiHBidkHS8ftx11jJk1hGFELvxh8LoxNQheaGT58dKenW,KyLPZdwY4td98bKkXqEXTEBX3vwEYTQo1yyLjX2jKXA63GBpmSjv)))","sh(wsh(multi(20,03669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0,0260b2003c386519fc9eadf2b5cf124dd8eea4c4e68d5e154050a9346ea98ce600,0362a74e399c39ed5593852a30147f2959b56bb827dfa3e60e464b02ccf87dc5e8,0261345b53de74a4d721ef877c255429961b7e43714171ac06168d7e08c542a8b8,02da72e8b46901a65d4374fe6315538d8f368557dda3a1dcf9ea903f3afe7314c8,0318c82dd0b53fd3a932d16e0ba9e278fcc937c582d5781be626ff16e201f72286,0297ccef1ef99f9d73dec9ad37476ddb232f1238aff877af19e72ba04493361009,02e502cfd5c3f972fe9a3e2a18827820638f96b6f347e54d63deb839011fd5765d,03e687710f0e3ebe81c1037074da939d409c0025f17eb86adb9427d28f0f7ae0e9,02c04d3a5274952acdbc76987f3184b346a483d43be40874624b29e3692c1df5af,02ed06e0f418b5b43a7ec01d1d7d27290fa15f75771cb69b642a51471c29c84acd,036d46073cbb9ffee90473f3da429abc8de7f8751199da44485682a989a4bebb24,02f5d1ff7c9029a80a4e36b9a5497027ef7f3e73384a4a94fbfe7c4e9164eec8bc,02e41deffd1b7cce11cde209a781adcffdabd1b91c0ba0375857a2bfd9302419f3,02d76625f7956a7fc505ab02556c23ee72d832f1bac391bcd2d3abce5710a13d06,0399eb0a5487515802dc14544cf10b3666623762fbed2ec38a3975716e2c29c232,02bc2feaa536991d269aae46abb8f3772a5b3ad592314945e51543e7da84c4af6e,0318bf32e5217c1eb771a6d5ce1cd39395dff7ff665704f175c9a5451d95a2f2ca,02c681a6243f16208c2004bb81f5a8a67edfdd3e3711534eadeec3dcf0b010c759,0249fdd6b69768b8d84b4893f8ff84b36835c50183de20fcae8f366a45290d01fd)))", "sh(wsh(multi(20,03669b8afcec803a0d323e9a17f3ea8e68e8abe5a278020a929adbec52421adbd0,0260b2003c386519fc9eadf2b5cf124dd8eea4c4e68d5e154050a9346ea98ce600,0362a74e399c39ed5593852a30147f2959b56bb827dfa3e60e464b02ccf87dc5e8,0261345b53de74a4d721ef877c255429961b7e43714171ac06168d7e08c542a8b8,02da72e8b46901a65d4374fe6315538d8f368557dda3a1dcf9ea903f3afe7314c8,0318c82dd0b53fd3a932d16e0ba9e278fcc937c582d5781be626ff16e201f72286,0297ccef1ef99f9d73dec9ad37476ddb232f1238aff877af19e72ba04493361009,02e502cfd5c3f972fe9a3e2a18827820638f96b6f347e54d63deb839011fd5765d,03e687710f0e3ebe81c1037074da939d409c0025f17eb86adb9427d28f0f7ae0e9,02c04d3a5274952acdbc76987f3184b346a483d43be40874624b29e3692c1df5af,02ed06e0f418b5b43a7ec01d1d7d27290fa15f75771cb69b642a51471c29c84acd,036d46073cbb9ffee90473f3da429abc8de7f8751199da44485682a989a4bebb24,02f5d1ff7c9029a80a4e36b9a5497027ef7f3e73384a4a94fbfe7c4e9164eec8bc,02e41deffd1b7cce11cde209a781adcffdabd1b91c0ba0375857a2bfd9302419f3,02d76625f7956a7fc505ab02556c23ee72d832f1bac391bcd2d3abce5710a13d06,0399eb0a5487515802dc14544cf10b3666623762fbed2ec38a3975716e2c29c232,02bc2feaa536991d269aae46abb8f3772a5b3ad592314945e51543e7da84c4af6e,0318bf32e5217c1eb771a6d5ce1cd39395dff7ff665704f175c9a5451d95a2f2ca,02c681a6243f16208c2004bb81f5a8a67edfdd3e3711534eadeec3dcf0b010c759,0249fdd6b69768b8d84b4893f8ff84b36835c50183de20fcae8f366a45290d01fd)))", SIGNABLE, {{"a914c2c9c510e9d7f92fd6131e94803a8d34a8ef675e87"}}, OutputType::P2SH_SEGWIT, /*op_desc_id=*/uint256{"69c3f3153ed2527d12cf78e53e719233fdb7fa6ca9f8a10059ce47d34b49c4cb"}); // Even if it's wrapped into P2SH
    // Check for invalid nesting of structures
    CheckUnparsable("sh(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)", "sh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", "A function is needed within P2SH"); // P2SH needs a script, not a key
    CheckUnparsable("sh(combo(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1))", "sh(combo(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", "Can only have combo() at top level"); // Old must be top level
    CheckUnparsable("wsh(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)", "wsh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)", "A function is needed within P2WSH"); // P2WSH needs a script, not a key
    CheckUnparsable("wsh(wpkh(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1))", "wsh(wpkh(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", "Can only have wpkh() at top level or inside sh()"); // Cannot embed witness inside witness
    CheckUnparsable("wsh(sh(pk(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)))", "wsh(sh(pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)))", "Can only have sh() at top level"); // Cannot embed P2SH inside P2WSH
    CheckUnparsable("sh(sh(pk(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)))", "sh(sh(pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)))", "Can only have sh() at top level"); // Cannot embed P2SH inside P2SH
    CheckUnparsable("wsh(wsh(pk(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)))", "wsh(wsh(pk(03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)))", "Can only have wsh() at top level or inside sh()"); // Cannot embed P2WSH inside P2WSH

    // Check for whitespace into keys
    CheckUnparsable("", "multi(1, L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1,5KYZdUEo39z3FPrtuX2QbbwGnNP5zTd7yyr2SC1j299sBCnWjss)", "Multi: Key ' L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1' is invalid due to whitespace");
    CheckUnparsable("", "pk(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1 )", "pk(): Key 'L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1 ' is invalid due to whitespace");
    CheckUnparsable("", "pk( L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1 )", "pk(): Key ' L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1 ' is invalid due to whitespace");

    // Checksums
    Check("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#ggrsrxfy", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#tjg09x5t", "sh(multi(2,[00000000/111h/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#hgmsckna", DEFAULT, {{"a91445a9a622a8b0a1269944be477640eedc447bbd8487"}}, OutputType::LEGACY, /*op_desc_id=*/uint256{"9339b7bfbe8cfd9d0d55819778ef77f52e5786e85b4c83be8a0d5b976e033f4c"}, {{0x8000006FUL,222},{0}});
    Check("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))", "sh(multi(2,[00000000/111h/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))", DEFAULT, {{"a91445a9a622a8b0a1269944be477640eedc447bbd8487"}}, OutputType::LEGACY, /*op_desc_id=*/uint256{"9339b7bfbe8cfd9d0d55819778ef77f52e5786e85b4c83be8a0d5b976e033f4c"}, {{0x8000006FUL,222},{0}});
    CheckUnparsable("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#", "Expected 8 character checksum, not 0 characters"); // Empty checksum
    CheckUnparsable("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#ggrsrxfyq", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#tjg09x5tq", "Expected 8 character checksum, not 9 characters"); // Too long checksum
    CheckUnparsable("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#ggrsrxf", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#tjg09x5", "Expected 8 character checksum, not 7 characters"); // Too short checksum
    CheckUnparsable("sh(multi(3,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#ggrsrxfy", "sh(multi(3,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#tjg09x5t", "Provided checksum 'tjg09x5t' does not match computed checksum 'd4x0uxyv'"); // Error in payload
    CheckUnparsable("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#ggssrxfy", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#tjq09x4t", "Provided checksum 'tjq09x4t' does not match computed checksum 'tjg09x5t'"); // Error in checksum
    CheckUnparsable("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))##ggssrxfy", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))##tjq09x4t", "Multiple '#' symbols"); // Error in checksum

    // Addr and raw tests
    CheckUnparsable("", "addr(asdf)", "Address is not valid"); // Invalid address
    CheckUnparsable("", "raw(asdf)", "Raw script is not hex"); // Invalid script
    CheckUnparsable("", "raw(Ü)#00000000", "Invalid characters in payload"); // Invalid chars

    Check(
        "rawtr(xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/86'/1'/0'/1/*)#a5gn3t7k",
        "rawtr(xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/86'/1'/0'/1/*)#4ur3xhft",
        "rawtr([5a61ff8e/86h/1h/0h]xpub6DtZpc9PRL2B6pwoNGysmHAaBofDmWv5S6KQEKKGPKhf5fV62ywDtSziSApYVK3JnYY5KUSgiCwiXW5wtd8z7LNBxT9Mu5sEro8itdGfTeA/1/*)#vwgx7hj9",
        RANGE | HARDENED | XONLY_KEYS,
        {{"51205172af752f057d543ce8e4a6f8dcf15548ec6be44041bfa93b72e191cfc8c1ee"}, {"51201b66f20b86f700c945ecb9ad9b0ad1662b73084e2bfea48bee02126350b8a5b1"}, {"512063e70f66d815218abcc2306aa930aaca07c5cde73b75127eb27b5e8c16b58a25"}},
        OutputType::BECH32M,
        /*op_desc_id=*/uint256{"458f0e7f4075a81c990c6be6d5b985027ac8b7f7cef8311696d95b7b49658c7d"},
        {{0x80000056, 0x80000001, 0x80000000, 1, 0}, {0x80000056, 0x80000001, 0x80000000, 1, 1}, {0x80000056, 0x80000001, 0x80000000, 1, 2}});

    Check(
        "rawtr(L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1)",
        "rawtr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)",
        "rawtr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)",
        SIGNABLE | XONLY_KEYS,
        {{"5120a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd"}},
        OutputType::BECH32M,
        /*op_desc_id=*/uint256{"5ba3f7d83cee4795df00e0eaa5070a3e164283c5fc6e8586fd710eaa7a4168ec"});

    CheckUnparsable(
        "",
        "rawtr(xpub68FQ9imX6mCWacw6eNRjaa8q8ynnHmUd5i7MVR51ZMPP5JycyfVHSLQVFPHMYiTybWJnSBL2tCBpy6aJTR2DYrshWYfwAxs8SosGXd66d8/*, xpub69Mvq3QMipdvnd9hAyeTnT5jrkcBuLErV212nsGf3qr7JPWysc9HnNhCsazdzj1etSx28hPSE8D7DnceFbNdw4Kg8SyRfjE2HFLv1P8TSGc/*)",
        "rawtr(): only one key expected.");

    // A 2of4 but using a direct push rather than OP_2
    CScript nonminimalmultisig;
    CKey keys[4];
    nonminimalmultisig << std::vector<unsigned char>{2};
    for (int i = 0; i < 4; i++) {
        keys[i].MakeNewKey(true);
        nonminimalmultisig << ToByteVector(keys[i].GetPubKey());
    }
    nonminimalmultisig << 4 << OP_CHECKMULTISIG;
    CheckInferRaw(nonminimalmultisig);

    // A 2of4 but using a direct push rather than OP_4
    nonminimalmultisig.clear();
    nonminimalmultisig << 2;
    for (int i = 0; i < 4; i++) {
        keys[i].MakeNewKey(true);
        nonminimalmultisig << ToByteVector(keys[i].GetPubKey());
    }
    nonminimalmultisig << std::vector<unsigned char>{4} << OP_CHECKMULTISIG;
    CheckInferRaw(nonminimalmultisig);

    // Miniscript tests

    // Invalid checksum
    CheckUnparsable("wsh(and_v(vc:andor(pk(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd),pk_k(Kx9HCDjGiwFcgVNhTrS5z5NeZdD6veeam61eDxLDCkGWujvL4Gnn),and_v(v:older(1),pk_k(L4o2kDvXXDRH2VS9uBnouScLduWt4dZnM25se7kvEjJeQ285en2A))),after(10)))#abcdef12", "wsh(and_v(vc:andor(pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),pk_k(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0),and_v(v:older(1),pk_k(02aa27e5eb2c185e87cd1dbc3e0efc9cb1175235e0259df1713424941c3cb40402))),after(10)))#abcdef12", "Provided checksum 'abcdef12' does not match computed checksum 'tyzp6a7p'");
    // Only p2wsh or tr contexts are valid
    CheckUnparsable("sh(and_v(vc:andor(pk(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd),pk_k(Kx9HCDjGiwFcgVNhTrS5z5NeZdD6veeam61eDxLDCkGWujvL4Gnn),and_v(v:older(1),pk_k(L4o2kDvXXDRH2VS9uBnouScLduWt4dZnM25se7kvEjJeQ285en2A))),after(10)))", "sh(and_v(vc:andor(pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),pk_k(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0),and_v(v:older(1),pk_k(02aa27e5eb2c185e87cd1dbc3e0efc9cb1175235e0259df1713424941c3cb40402))),after(10)))", "Miniscript expressions can only be used in wsh or tr.");
    CheckUnparsable("tr(and_v(vc:andor(pk(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd),pk_k(Kx9HCDjGiwFcgVNhTrS5z5NeZdD6veeam61eDxLDCkGWujvL4Gnn),and_v(v:older(1),pk_k(L4o2kDvXXDRH2VS9uBnouScLduWt4dZnM25se7kvEjJeQ285en2A))),after(10)))", "tr(and_v(vc:andor(pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),pk_k(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0),and_v(v:older(1),pk_k(02aa27e5eb2c185e87cd1dbc3e0efc9cb1175235e0259df1713424941c3cb40402))),after(10)))", "tr(): key 'and_v(vc:andor(pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),pk_k(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0),and_v(v:older(1),pk_k(02aa27e5eb2c185e87cd1dbc3e0efc9cb1175235e0259df1713424941c3cb40402))),after(10))' is not valid");
    CheckUnparsable("raw(and_v(vc:andor(pk(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd),pk_k(Kx9HCDjGiwFcgVNhTrS5z5NeZdD6veeam61eDxLDCkGWujvL4Gnn),and_v(v:older(1),pk_k(L4o2kDvXXDRH2VS9uBnouScLduWt4dZnM25se7kvEjJeQ285en2A))),after(10)))", "sh(and_v(vc:andor(pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),pk_k(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0),and_v(v:older(1),pk_k(02aa27e5eb2c185e87cd1dbc3e0efc9cb1175235e0259df1713424941c3cb40402))),after(10)))", "Miniscript expressions can only be used in wsh or tr.");
    CheckUnparsable("", "tr(034D2224bbbbbbbbbbcbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb40,{{{{{{{{{{{{{{{{{{{{{{multi(1,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/967808'/9,xprvA1RpRA33e1JQ7ifknakTFNpgXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/968/2/5/8/5/2/5/58/58/2/5/5/5/58/588/2/6/8/5/2/8/2/5/8/2/58/2/5/8/5/2/8/5/8/3/4/5/58/55/2/5/58/58/2/5/5/5/8/5/2/8/5/85/2/8/2/5/8/5/2/5/58/58/2/5/58/58/588/2/58/2/8/5/8/5/4/5/585/2/5/58/58/2/5/5/58/588/2/58/2/5/8/5/2/8/2/5/8/5/5/58/588/2/6/8/5/2/8/2/5/8/5/2/5/58/58/2/5/58/58/2/0/8/5/2/8/5/8/5/4/5/58/588/2/6/8/5/2/8/2/5/8/5/2/5/58/58/2/5/58/58/588/2/58/2/5/8/5/8/24/5/58/52/5/8/5/2/8/24/5/58/588/246/8/5/2/8/2/5/8/5/2/5/58/58/2/5/5/5/58/588/2/6/8/5/2/8/2/5/8/2/58/2/5/8/5/2/8/5/8/5/4/5/58/55/58/2/5/8/55/2/5/8/58/555/58/2/5/8/4//2/5/58/5w/2/5/8/5/2/4/5/58/5558'/2/5/58/58/2/5/5/58/588/2/58/2/5/8/5/2/8/2/5/8/5/5/8/58/2/5/58/58/2/5/8/9/588/2/58/2/5/8/5/2/8/5/8/5/4/5/58/588/2/6/8/5/2/8/2/5/8/5/2/5/58/58/2/5/5/58/588/2/58/2/5/8/5/2/82/5/8/5/5/58/52/6/8/5/2/8/{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{}{{{{{{{{{DDD2/5/8/5/2/5/58/58/2/5/58/58/588/2/58/2/8/5/8/5/4/5/58/588/2/6/8/5/2/8/2/5/8588/246/8/5/2DLDDDDDDDbbD3DDDD/8/2/5/8/5/2/5/58/58/2/5/5/5/58/588/2/6/8/5/2/8/2/5/8/2/58/2/5/8/5/2/8/5/8/3/4/5/58/55/2/5/58/58/2/5/5/5/8/5/2/8/5/85/2/8/2/5/8D)/5/2/5/58/58/2/5/58/58/58/588/2/58/2/5/8/5/25/58/58/2/5/58/58/2/5/8/9/588/2/58/2/6780,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFW/8/5/2/5/58678008')", "'multi(1,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/967808'/9,xprvA1RpRA33e1JQ7ifknakTFNpgXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/968/2/5/8/5/2/5/58/58/2/5/5/5/58/588/2/6/8/5/2/8/2/5/8/2/58/2/5/8/5/2/8/5/8/3/4/5/58/55/2/5/58/58/2/5/5/5/8/5/2/8/5/85/2/8/2/5/8/5/2/5/58/58/2/5/58/58/588/2/58/2/8/5/8/5/4/5/585/2/5/58/58/2/5/5/58/588/2/58/2/5/8/5/2/8/2/5/8/5/5/58/588/2/6/8/5/2/8/2/5/8/5/2/5/58/58/2/5/58/58/2/0/8/5/2/8/5/8/5/4/5/58/588/2/6/8/5/2/8/2/5/8/5/2/5/58/58/2/5/58/58/588/2/58/2/5/8/5/8/24/5/58/52/5/8/5/2/8/24/5/58/588/246/8/5/2/8/2/5/8/5/2/5/58/58/2/5/5/5/58/588/2/6/8/5/2/8/2/5/8/2/58/2/5/8/5/2/8/5/8/5/4/5/58/55/58/2/5/8/55/2/5/8/58/555/58/2/5/8/4//2/5/58/5w/2/5/8/5/2/4/5/58/5558'/2/5/58/58/2/5/5/58/588/2/58/2/5/8/5/2/8/2/5/8/5/5/8/58/2/5/58/58/2/5/8/9/588/2/58/2/5/8/5/2/8/5/8/5/4/5/58/588/2/6/8/5/2/8/2/5/8/5/2/5/58/58/2/5/5/58/588/2/58/2/5/8/5/2/82/5/8/5/5/58/52/6/8/5/2/8/{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{}{{{{{{{{{DDD2/5/8/5/2/5/58/58/2/5/58/58/588/2/58/2/8/5/8/5/4/5/58/588/2/6/8/5/2/8/2/5/8588/246/8/5/2DLDDDDDDDbbD3DDDD/8/2/5/8/5/2/5/58/58/2/5/5/5/58/588/2/6/8/5/2/8/2/5/8/2/58/2/5/8/5/2/8/5/8/3/4/5/58/55/2/5/58/58/2/5/5/5/8/5/2/8/5/85/2/8/2/5/8D)/5/2/5/58/58/2/5/58/58/58/588/2/58/2/5/8/5/25/58/58/2/5/58/58/2/5/8/9/588/2/58/2/6780,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFW/8/5/2/5/58678008'' is not a valid descriptor function");
    // No uncompressed keys allowed
    CheckUnparsable("", "wsh(and_v(vc:andor(pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),pk_k(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0),and_v(v:older(1),pk_k(049228de6902abb4f541791f6d7f925b10e2078ccb1298856e5ea5cc5fd667f930eac37a00cc07f9a91ef3c2d17bf7a17db04552ff90ac312a5b8b4caca6c97aa4))),after(10)))", "Uncompressed keys are not allowed");
    // No hybrid keys allowed
    CheckUnparsable("", "wsh(and_v(vc:andor(pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),pk_k(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0),and_v(v:older(1),pk_k(069228de6902abb4f541791f6d7f925b10e2078ccb1298856e5ea5cc5fd667f930eac37a00cc07f9a91ef3c2d17bf7a17db04552ff90ac312a5b8b4caca6c97aa4))),after(10)))", "Hybrid public keys are not allowed");
    // Insane at top level
    CheckUnparsable("wsh(and_b(vc:andor(pk(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd),pk_k(Kx9HCDjGiwFcgVNhTrS5z5NeZdD6veeam61eDxLDCkGWujvL4Gnn),and_v(v:older(1),pk_k(L4o2kDvXXDRH2VS9uBnouScLduWt4dZnM25se7kvEjJeQ285en2A))),after(10)))", "wsh(and_b(vc:andor(pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),pk_k(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0),and_v(v:older(1),pk_k(02aa27e5eb2c185e87cd1dbc3e0efc9cb1175235e0259df1713424941c3cb40402))),after(10)))", "and_b(vc:andor(pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),pk_k(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0),and_v(v:older(1),pk_k(02aa27e5eb2c185e87cd1dbc3e0efc9cb1175235e0259df1713424941c3cb40402))),after(10)) is invalid");
    // Invalid sub
    CheckUnparsable("wsh(and_v(vc:andor(v:pk_k(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd),pk_k(Kx9HCDjGiwFcgVNhTrS5z5NeZdD6veeam61eDxLDCkGWujvL4Gnn),and_v(v:older(1),pk_k(L4o2kDvXXDRH2VS9uBnouScLduWt4dZnM25se7kvEjJeQ285en2A))),after(10)))", "wsh(and_v(vc:andor(v:pk_k(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),pk_k(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0),and_v(v:older(1),pk_k(02aa27e5eb2c185e87cd1dbc3e0efc9cb1175235e0259df1713424941c3cb40402))),after(10)))", "v:pk_k(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204) is invalid");
    // Insane subs
    CheckUnparsable("wsh(or_i(older(1),pk(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd)))", "wsh(or_i(older(1),pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204)))", "or_i(older(1),pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204)) is not sane: witnesses without signature exist");
    CheckUnparsable("wsh(or_b(sha256(cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),s:pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204)))", "wsh(or_b(sha256(cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),s:pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204)))", "or_b(sha256(cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),s:pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204)) is not sane: malleable witnesses exist");
    CheckUnparsable("wsh(and_b(and_b(older(1),a:older(100000000)),s:pk(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd)))", "wsh(and_b(and_b(older(1),a:older(100000000)),s:pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204)))", "and_b(older(1),a:older(100000000)) is not sane: contains mixes of timelocks expressed in blocks and seconds");
    CheckUnparsable("wsh(and_b(or_b(pkh(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd),s:pk(Kx9HCDjGiwFcgVNhTrS5z5NeZdD6veeam61eDxLDCkGWujvL4Gnn)),s:pk(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd)))", "wsh(and_b(or_b(pkh(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),s:pk(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0)),s:pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204)))", "and_b(or_b(pkh(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),s:pk(032707170c71d8f75e4ca4e3fce870b9409dcaf12b051d3bcadff74747fa7619c0)),s:pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204)) is not sane: contains duplicate public keys");
    // Valid with extended keys.
    Check("wsh(and_v(v:ripemd160(095ff41131e5946f3c85f79e44adbcf8e27e080e),multi(1,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0)))", "wsh(and_v(v:ripemd160(095ff41131e5946f3c85f79e44adbcf8e27e080e),multi(1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0)))", "wsh(and_v(v:ripemd160(095ff41131e5946f3c85f79e44adbcf8e27e080e),multi(1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0)))", DEFAULT, {{"0020acf425291b98a1d7e0d4690139442abc289175be32ef1f75945e339924246d73"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"0634b326edc66f9e2660562564d7a8fcca55f91dc4555ce0a51883cc72e0fa41"}, {{},{0}});
    // Valid under sh(wsh()) and with a mix of xpubs and raw keys.
    Check("sh(wsh(thresh(1,pkh(L4gM1FBdyHNpkzsFh9ipnofLhpZRp2mwobpeULy1a6dBTvw8Ywtd),a:and_n(multi(1,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0),n:older(2)))))", "sh(wsh(thresh(1,pkh(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),a:and_n(multi(1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0),n:older(2)))))", "sh(wsh(thresh(1,pkh(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204),a:and_n(multi(1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0),n:older(2)))))", SIGNABLE | MIXED_PUBKEYS, {{"a914767e9119ff3b3ac0cb6dcfe21de1842ccf85f1c487"}}, OutputType::P2SH_SEGWIT, /*op_desc_id=*/uint256{"3cfcad33bc25579d70b23ce634d317be00a4e5400e758e37c215bdc17b31bfb8"}, {{},{0}});
    // An exotic multisig, we can sign for both branches
    Check("wsh(thresh(1,pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc),a:pkh(xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0)))", "wsh(thresh(1,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL),a:pkh(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0)))", "wsh(thresh(1,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL),a:pkh(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0)))", SIGNABLE, {{"00204a4528fbc0947e02e921b54bd476fc8cc2ebb5c6ae2ccf10ed29fe2937fb6892"}}, OutputType::BECH32, /*op_desc_id=*/std::nullopt, {{},{0}});
    // We can sign for a script requiring the two kinds of timelock.
    // But if we don't set a sequence high enough, we'll fail.
    Check("sh(wsh(thresh(2,ndv:after(1000),a:and_n(multi(1,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0),n:older(2)))))", "sh(wsh(thresh(2,ndv:after(1000),a:and_n(multi(1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0),n:older(2)))))", "sh(wsh(thresh(2,ndv:after(1000),a:and_n(multi(1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0),n:older(2)))))", SIGNABLE_FAILS, {{"a914099f400961f930d4c16c3b33c0e2a58ef53ac38f87"}}, OutputType::P2SH_SEGWIT, /*op_desc_id=*/uint256{"f5c14a15b45d2af1b8ec69acfd3cf4790f069705d1b079efb0b8193fed181f64"}, {{},{0}}, /*spender_nlocktime=*/1000, /*spender_nsequence=*/1);
    // And same for the nLockTime.
    Check("sh(wsh(thresh(2,ndv:after(1000),a:and_n(multi(1,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0),n:older(2)))))", "sh(wsh(thresh(2,ndv:after(1000),a:and_n(multi(1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0),n:older(2)))))", "sh(wsh(thresh(2,ndv:after(1000),a:and_n(multi(1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0),n:older(2)))))", SIGNABLE_FAILS, {{"a914099f400961f930d4c16c3b33c0e2a58ef53ac38f87"}}, OutputType::P2SH_SEGWIT, /*op_desc_id=*/uint256{"f5c14a15b45d2af1b8ec69acfd3cf4790f069705d1b079efb0b8193fed181f64"}, {{},{0}}, /*spender_nlocktime=*/999, /*spender_nsequence=*/2);
    // But if both are set to (at least) the required value, we'll succeed.
    Check("sh(wsh(thresh(2,ndv:after(1000),a:and_n(multi(1,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0),n:older(2)))))", "sh(wsh(thresh(2,ndv:after(1000),a:and_n(multi(1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0),n:older(2)))))", "sh(wsh(thresh(2,ndv:after(1000),a:and_n(multi(1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0),n:older(2)))))", SIGNABLE, {{"a914099f400961f930d4c16c3b33c0e2a58ef53ac38f87"}}, OutputType::P2SH_SEGWIT, /*op_desc_id=*/uint256{"f5c14a15b45d2af1b8ec69acfd3cf4790f069705d1b079efb0b8193fed181f64"}, {{},{0}}, /*spender_nlocktime=*/1000, /*spender_nsequence=*/2);
    // We can't sign for a script requiring a ripemd160 preimage without providing it.
    Check("wsh(and_v(v:ripemd160(ff9aa1829c90d26e73301383f549e1497b7d6325),pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)))", "wsh(and_v(v:ripemd160(ff9aa1829c90d26e73301383f549e1497b7d6325),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", "wsh(and_v(v:ripemd160(ff9aa1829c90d26e73301383f549e1497b7d6325),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", SIGNABLE_FAILS, {{"002001549deda34cbc4a5982263191380f522695a2ddc2f99fc3a65c736264bd6cab"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"1fed6fbd0e408eb4bddfefa075289dc7061e7a3240c84f6ba5b9f294d96a21f4"}, {{}}, /*spender_nlocktime=*/0, /*spender_nsequence=*/CTxIn::SEQUENCE_FINAL, {});
    // But if we provide it, we can.
    Check("wsh(and_v(v:ripemd160(ff9aa1829c90d26e73301383f549e1497b7d6325),pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)))", "wsh(and_v(v:ripemd160(ff9aa1829c90d26e73301383f549e1497b7d6325),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", "wsh(and_v(v:ripemd160(ff9aa1829c90d26e73301383f549e1497b7d6325),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", SIGNABLE, {{"002001549deda34cbc4a5982263191380f522695a2ddc2f99fc3a65c736264bd6cab"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"1fed6fbd0e408eb4bddfefa075289dc7061e7a3240c84f6ba5b9f294d96a21f4"}, {{}}, /*spender_nlocktime=*/0, /*spender_nsequence=*/CTxIn::SEQUENCE_FINAL, {{"ff9aa1829c90d26e73301383f549e1497b7d6325"_hex_v_u8, "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"_hex_v_u8}});
    // Same for sha256
    Check("wsh(and_v(v:sha256(7426ba0604c3f8682c7016b44673f85c5bd9da2fa6c1080810cf53ae320c9863),pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)))", "wsh(and_v(v:sha256(7426ba0604c3f8682c7016b44673f85c5bd9da2fa6c1080810cf53ae320c9863),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", "wsh(and_v(v:sha256(7426ba0604c3f8682c7016b44673f85c5bd9da2fa6c1080810cf53ae320c9863),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", SIGNABLE_FAILS, {{"002071f7283dbbb9a55ed43a54cda16ba0efd0f16dc48fe200f299e57bb5d7be8dd4"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"a1809a65ba5ca2f09a06c114d4881eed95d1b62f38743cf126cf71b2dd411374"}, {{}}, /*spender_nlocktime=*/0, /*spender_nsequence=*/CTxIn::SEQUENCE_FINAL, {});
    Check("wsh(and_v(v:sha256(7426ba0604c3f8682c7016b44673f85c5bd9da2fa6c1080810cf53ae320c9863),pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)))", "wsh(and_v(v:sha256(7426ba0604c3f8682c7016b44673f85c5bd9da2fa6c1080810cf53ae320c9863),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", "wsh(and_v(v:sha256(7426ba0604c3f8682c7016b44673f85c5bd9da2fa6c1080810cf53ae320c9863),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", SIGNABLE, {{"002071f7283dbbb9a55ed43a54cda16ba0efd0f16dc48fe200f299e57bb5d7be8dd4"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"a1809a65ba5ca2f09a06c114d4881eed95d1b62f38743cf126cf71b2dd411374"}, {{}}, /*spender_nlocktime=*/0, /*spender_nsequence=*/CTxIn::SEQUENCE_FINAL, {{"7426ba0604c3f8682c7016b44673f85c5bd9da2fa6c1080810cf53ae320c9863"_hex_v_u8, "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"_hex_v_u8}});
    // Same for hash160
    Check("wsh(and_v(v:hash160(292e2df59e3a22109200beed0cdc84b12e66793e),pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)))", "wsh(and_v(v:hash160(292e2df59e3a22109200beed0cdc84b12e66793e),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", "wsh(and_v(v:hash160(292e2df59e3a22109200beed0cdc84b12e66793e),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", SIGNABLE_FAILS, {{"00209b9d5b45735d0e15df5b41d6594602d3de472262f7b75edc6cf5f3e3fa4e3ae4"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"d7bdbc680503a585925eec72d11fc99396f51855d0a03fce53c90bed4c2e319f"}, {{}}, /*spender_nlocktime=*/0, /*spender_nsequence=*/CTxIn::SEQUENCE_FINAL, {});
    Check("wsh(and_v(v:hash160(292e2df59e3a22109200beed0cdc84b12e66793e),pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)))", "wsh(and_v(v:hash160(292e2df59e3a22109200beed0cdc84b12e66793e),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", "wsh(and_v(v:hash160(292e2df59e3a22109200beed0cdc84b12e66793e),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", SIGNABLE, {{"00209b9d5b45735d0e15df5b41d6594602d3de472262f7b75edc6cf5f3e3fa4e3ae4"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"d7bdbc680503a585925eec72d11fc99396f51855d0a03fce53c90bed4c2e319f"}, {{}}, /*spender_nlocktime=*/0, /*spender_nsequence=*/CTxIn::SEQUENCE_FINAL, {{"292e2df59e3a22109200beed0cdc84b12e66793e"_hex_v_u8, "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"_hex_v_u8}});
    // Same for hash256
    Check("wsh(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)))", "wsh(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", "wsh(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", SIGNABLE_FAILS, {{"0020cf62bf97baf977aec69cbc290c372899f913337a9093e8f066ab59b8657a365c"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"8412ba3ac20ba3a30f81442d10d32e0468fa52814960d04e959bf84a9b813b88"}, {{}}, /*spender_nlocktime=*/0, /*spender_nsequence=*/CTxIn::SEQUENCE_FINAL, {});
    Check("wsh(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)))", "wsh(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", "wsh(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)))", SIGNABLE, {{"0020cf62bf97baf977aec69cbc290c372899f913337a9093e8f066ab59b8657a365c"}}, OutputType::BECH32, /*op_desc_id=*/uint256{"8412ba3ac20ba3a30f81442d10d32e0468fa52814960d04e959bf84a9b813b88"}, {{}}, /*spender_nlocktime=*/0, /*spender_nsequence=*/CTxIn::SEQUENCE_FINAL, {{"ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588"_hex_v_u8, "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"_hex_v_u8}});
    // Can have a Miniscript expression under tr() if it's alone.
    Check("tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,thresh(2,pk(L1NKM8dVA1h52mwDrmk1YreTWkAZZTu2vmKLpmLEbFRqGQYjHeEV),s:pk(Kz3iCBy3HNGP5CZWDsAMmnCMFNwqdDohudVN9fvkrN7tAkzKNtM7),adv:older(42)))", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,thresh(2,pk(30a6069f344fb784a2b4c99540a91ee727c91e3a25ef6aae867d9c65b5f23529),s:pk(9918d400c1b8c3c478340a40117ced4054b6b58f48cdb3c89b836bdfee1f5766),adv:older(42)))", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,thresh(2,pk(30a6069f344fb784a2b4c99540a91ee727c91e3a25ef6aae867d9c65b5f23529),s:pk(9918d400c1b8c3c478340a40117ced4054b6b58f48cdb3c89b836bdfee1f5766),adv:older(42)))", MISSING_PRIVKEYS | XONLY_KEYS | SIGNABLE, {{"512033982eebe204dc66508e4b19cfc31b5ffc6e1bfcbf6e5597dfc2521a52270795"}}, OutputType::BECH32M);
    // Can have a pkh() expression alone as tr() script path (because pkh() is valid Miniscript).
    Check("tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,pkh(L1NKM8dVA1h52mwDrmk1YreTWkAZZTu2vmKLpmLEbFRqGQYjHeEV))", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,pkh(30a6069f344fb784a2b4c99540a91ee727c91e3a25ef6aae867d9c65b5f23529))", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,pkh(30a6069f344fb784a2b4c99540a91ee727c91e3a25ef6aae867d9c65b5f23529))", MISSING_PRIVKEYS | XONLY_KEYS | SIGNABLE, {{"51201e9875f690f5847404e4c5951e2f029887df0525691ee11a682afd37b608aad4"}}, OutputType::BECH32M);
    // Can have a Miniscript expression under tr() if it's part of a tree.
    Check("tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{{pkh(KykUPmR5967F4URzMUeCv9kNMU9CNRWycrPmx3ZvfkWoQLabbimL),pk(L3Enys1jFgTq4E24b8Uom1kAz6cNkz3Z82XZpBKCE2ztErq9fqvJ)},thresh(1,pk(L1NKM8dVA1h52mwDrmk1YreTWkAZZTu2vmKLpmLEbFRqGQYjHeEV),s:pk(Kz3iCBy3HNGP5CZWDsAMmnCMFNwqdDohudVN9fvkrN7tAkzKNtM7))})", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{{pkh(1c9bc926084382e76da33b5a52d17b1fa153c072aae5fb5228ecc2ccf89d79d5),pk(0dd6b52b192ab195558d22dd8437a9ec4519ee5ded496c0d55bc9b1a8b0e8c2b)},thresh(1,pk(30a6069f344fb784a2b4c99540a91ee727c91e3a25ef6aae867d9c65b5f23529),s:pk(9918d400c1b8c3c478340a40117ced4054b6b58f48cdb3c89b836bdfee1f5766))})", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{{pkh(1c9bc926084382e76da33b5a52d17b1fa153c072aae5fb5228ecc2ccf89d79d5),pk(0dd6b52b192ab195558d22dd8437a9ec4519ee5ded496c0d55bc9b1a8b0e8c2b)},thresh(1,pk(30a6069f344fb784a2b4c99540a91ee727c91e3a25ef6aae867d9c65b5f23529),s:pk(9918d400c1b8c3c478340a40117ced4054b6b58f48cdb3c89b836bdfee1f5766))})", MISSING_PRIVKEYS | XONLY_KEYS, {{"5120d8ea39b29de2b550b68bd2ada8b075c888c2b2df3290c7a35856482747848934"}}, OutputType::BECH32M);
    // Can have two Miniscripts in a Taproot with mixed private and public keys, and mixed ranged extended keys and raw keys.
    Check("tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{and_v(v:pk(xpub6AGbgdKcAGeUWaGNKH2o3sRvjtvJCGZ1NwrHqMJDwD4bN1QuwPQSsdeAYkPZGPt2FTAyu6nWGsC3fN2nsBELrLPcRNuwwr5k1X7yW5WV4aX/*),pk(02daf6e3477fc3906a1997820ed2940c8f5fa0942946d0368f981b001fdd85afcb)),and_v(v:pk(xprv9wCN7tTqN5ATsmBGEijuNeUgQjma9tv3GmdWLmbYiuArPsAMj6tD1uASiBfm47kdoi7bDBAVxUZNLM2MkeouPK5menDTyCNZtExQrKhVu7C/*),pk(03272c0c1ae2c07528283b91ca57b45d2cc84e7960e1f17f58815372285f35e99a))})", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{and_v(v:pk(xpub6AGbgdKcAGeUWaGNKH2o3sRvjtvJCGZ1NwrHqMJDwD4bN1QuwPQSsdeAYkPZGPt2FTAyu6nWGsC3fN2nsBELrLPcRNuwwr5k1X7yW5WV4aX/*),pk(02daf6e3477fc3906a1997820ed2940c8f5fa0942946d0368f981b001fdd85afcb)),and_v(v:pk(xpub6ABiXPzjCSim6FFjLkGujnRQxmc4ZMdtdzZ79A1AHEhqGfVWGeCTZhUvZTSf1mNnGUtyNqgfE9eWaYdYReDKbPYqgqi9LLVZSmWnLQRx477/*),pk(03272c0c1ae2c07528283b91ca57b45d2cc84e7960e1f17f58815372285f35e99a))})", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{and_v(v:pk(xpub6AGbgdKcAGeUWaGNKH2o3sRvjtvJCGZ1NwrHqMJDwD4bN1QuwPQSsdeAYkPZGPt2FTAyu6nWGsC3fN2nsBELrLPcRNuwwr5k1X7yW5WV4aX/*),pk(02daf6e3477fc3906a1997820ed2940c8f5fa0942946d0368f981b001fdd85afcb)),and_v(v:pk(xpub6ABiXPzjCSim6FFjLkGujnRQxmc4ZMdtdzZ79A1AHEhqGfVWGeCTZhUvZTSf1mNnGUtyNqgfE9eWaYdYReDKbPYqgqi9LLVZSmWnLQRx477/*),pk(03272c0c1ae2c07528283b91ca57b45d2cc84e7960e1f17f58815372285f35e99a))})", MISSING_PRIVKEYS | XONLY_KEYS | RANGE | MIXED_PUBKEYS, {{"5120793185cd1a9a0bb710fa57df3845ac4ddf7df63b74beadce2573cbb0b508b3a4"}}, OutputType::BECH32M, /*op_desc_id=*/{}, {{}, {0}});
    // Can sign for a Miniscript expression containing a hash challenge inside a Taproot tree. (Fails without the
    // preimages and the sequence, passes with.)
    Check("tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{and_v(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),v:pk(KykUPmR5967F4URzMUeCv9kNMU9CNRWycrPmx3ZvfkWoQLabbimL)),older(42)),multi_a(2,adf586a32ad4b0674a86022b000348b681b4c97a811f67eefe4a6e066e55080c,KztMyyi1pXUtuZfJSB7JzVdmJMAz7wfGVFoSRUR5CVZxXxULXuGR)})", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{and_v(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),v:pk(1c9bc926084382e76da33b5a52d17b1fa153c072aae5fb5228ecc2ccf89d79d5)),older(42)),multi_a(2,adf586a32ad4b0674a86022b000348b681b4c97a811f67eefe4a6e066e55080c,14fa4ad085cdee1e2fc73d491b36a96c192382b1d9a21108eb3533f630364f9f)})", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{and_v(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),v:pk(1c9bc926084382e76da33b5a52d17b1fa153c072aae5fb5228ecc2ccf89d79d5)),older(42)),multi_a(2,adf586a32ad4b0674a86022b000348b681b4c97a811f67eefe4a6e066e55080c,14fa4ad085cdee1e2fc73d491b36a96c192382b1d9a21108eb3533f630364f9f)})", MISSING_PRIVKEYS | XONLY_KEYS | SIGNABLE | SIGNABLE_FAILS, {{"51209a3d79db56fbe3ba4d905d827b62e1ed31cd6df1198b8c759d589c0f4efc27bd"}}, OutputType::BECH32M);
    Check("tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{and_v(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),v:pk(KykUPmR5967F4URzMUeCv9kNMU9CNRWycrPmx3ZvfkWoQLabbimL)),older(42)),multi_a(2,adf586a32ad4b0674a86022b000348b681b4c97a811f67eefe4a6e066e55080c,KztMyyi1pXUtuZfJSB7JzVdmJMAz7wfGVFoSRUR5CVZxXxULXuGR)})", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{and_v(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),v:pk(1c9bc926084382e76da33b5a52d17b1fa153c072aae5fb5228ecc2ccf89d79d5)),older(42)),multi_a(2,adf586a32ad4b0674a86022b000348b681b4c97a811f67eefe4a6e066e55080c,14fa4ad085cdee1e2fc73d491b36a96c192382b1d9a21108eb3533f630364f9f)})", "tr(a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd,{and_v(and_v(v:hash256(ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588),v:pk(1c9bc926084382e76da33b5a52d17b1fa153c072aae5fb5228ecc2ccf89d79d5)),older(42)),multi_a(2,adf586a32ad4b0674a86022b000348b681b4c97a811f67eefe4a6e066e55080c,14fa4ad085cdee1e2fc73d491b36a96c192382b1d9a21108eb3533f630364f9f)})", MISSING_PRIVKEYS | XONLY_KEYS | SIGNABLE, {{"51209a3d79db56fbe3ba4d905d827b62e1ed31cd6df1198b8c759d589c0f4efc27bd"}}, OutputType::BECH32M, /*op_desc_id=*/{}, {{}}, /*spender_nlocktime=*/0, /*spender_nsequence=*/42, /*preimages=*/{{"ae253ca2a54debcac7ecf414f6734f48c56421a08bb59182ff9f39a6fffdb588"_hex_v_u8, "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"_hex_v_u8}});

    // Basic sh(pkh()) with key origin
    CheckInferDescriptor("a9141a31ad23bf49c247dd531a623c2ef57da3c400c587", "sh(pkh([deadbeef/0h/0h/0]03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd))", {"76a9149a1c78a507689f6f54b847ad1cef1e614ee23f1e88ac"}, {{"03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd", "deadbeef/0h/0h/0"}});
    // p2pk script with hybrid key must infer as raw()
    CheckInferDescriptor("41069228de6902abb4f541791f6d7f925b10e2078ccb1298856e5ea5cc5fd667f930eac37a00cc07f9a91ef3c2d17bf7a17db04552ff90ac312a5b8b4caca6c97aa4ac", "raw(41069228de6902abb4f541791f6d7f925b10e2078ccb1298856e5ea5cc5fd667f930eac37a00cc07f9a91ef3c2d17bf7a17db04552ff90ac312a5b8b4caca6c97aa4ac)", {}, {{"069228de6902abb4f541791f6d7f925b10e2078ccb1298856e5ea5cc5fd667f930eac37a00cc07f9a91ef3c2d17bf7a17db04552ff90ac312a5b8b4caca6c97aa4", ""}});
    // p2pkh script with hybrid key must infer as addr()
    CheckInferDescriptor("76a91445ff7c2327866472639d507334a9a00119dfd32688ac", "addr(17P7ge56F2QcdHxxRBa2NyzmejFggPwBJ9)", {}, {{"069228de6902abb4f541791f6d7f925b10e2078ccb1298856e5ea5cc5fd667f930eac37a00cc07f9a91ef3c2d17bf7a17db04552ff90ac312a5b8b4caca6c97aa4", ""}});
    // p2wpkh script with uncompressed key must infer as addr()
    CheckInferDescriptor("001422e363a523947a110d9a9eb114820de183aca313", "addr(bc1qyt3k8ffrj3apzrv6n6c3fqsduxp6egcnk2r66j)", {}, {{"049228de6902abb4f541791f6d7f925b10e2078ccb1298856e5ea5cc5fd667f930eac37a00cc07f9a91ef3c2d17bf7a17db04552ff90ac312a5b8b4caca6c97aa4", ""}});
    // Infer pkh() from p2pkh with uncompressed key
    CheckInferDescriptor("76a914a31725c74421fadc50d35520ab8751ed120af80588ac", "pkh(04c56fe4a92d401bcbf1b3dfbe4ac3dac5602ca155a3681497f02c1b9a733b92d704e2da6ec4162e4846af9236ef4171069ac8b7f8234a8405b6cadd96f34f5a31)", {}, {{"04c56fe4a92d401bcbf1b3dfbe4ac3dac5602ca155a3681497f02c1b9a733b92d704e2da6ec4162e4846af9236ef4171069ac8b7f8234a8405b6cadd96f34f5a31", ""}});
    // Infer pk() from p2pk with uncompressed key
    CheckInferDescriptor("4104032540df1d3c7070a8ab3a9cdd304dfc7fd1e6541369c53c4c3310b2537d91059afc8b8e7673eb812a32978dabb78c40f2e423f7757dca61d11838c7aeeb5220ac", "pk(04032540df1d3c7070a8ab3a9cdd304dfc7fd1e6541369c53c4c3310b2537d91059afc8b8e7673eb812a32978dabb78c40f2e423f7757dca61d11838c7aeeb5220)", {}, {{"04032540df1d3c7070a8ab3a9cdd304dfc7fd1e6541369c53c4c3310b2537d91059afc8b8e7673eb812a32978dabb78c40f2e423f7757dca61d11838c7aeeb5220", ""}});

    // MuSig2 parsing
    Check("rawtr(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "rawtr(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "rawtr(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", XONLY_KEYS | MUSIG, {{"5120789d937bade6673538f3e28d8368dda4d0512f94da44cf477a505716d26a1575"}}, OutputType::BECH32M);
    Check("tr(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "tr(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "tr(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", XONLY_KEYS | MUSIG, {{"512079e6c3e628c9bfbce91de6b7fb28e2aec7713d377cf260ab599dcbc40e542312"}}, OutputType::BECH32M);
    Check("rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*))","rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*))","rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*))", XONLY_KEYS | RANGE | MUSIG, {{"5120754ccfd18ed4051de3b1144b6145cad4b2999387338dfb85ec392f2963ceaa3a"}, {"5120be80016576d2691ccc4077bc91d7ece4db34667d6e84829d5e08480cd4bc0b78"}, {"5120b7139e2f8b92570ad96c40c3b5e6557a5194e288a96df6f29980523365239d58"}}, OutputType::BECH32M, /*op_desc_id=*/std::nullopt, {{}, {0, 0}, {0, 1}, {0, 2}});
    Check("rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*)","rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*)","rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*)", XONLY_KEYS | RANGE | MUSIG | MUSIG_DERIVATION, {{"51209508c08832f3bb9d5e8baf8cb5cfa3669902e2f2da19acea63ff47b93faa9bfc"}, {"51205ca1102663025a83dd9b5dbc214762c5a6309af00d48167d2d6483808525a298"}, {"51207dbed1b89c338df6a1ae137f133a19cae6e03d481196ee6f1a5c7d1aeb56b166"}}, OutputType::BECH32M, /*op_desc_id=*/std::nullopt, {{}, {0, 0}, {0, 1}, {0, 2}});
    Check("rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/0,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/1)","rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/0,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/1)","rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/0,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/1)", XONLY_KEYS | MUSIG | MUSIG_DERIVATION, {{"51200e355f2bc9e754268e12bbd337499c2f7ffafc3101c41792709007b25a862532"}}, OutputType::BECH32M, /*op_desc_id=*/std::nullopt, {{}, {0}, {1}});
    Check("tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*,pk(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S))","tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*,pk(f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9))","tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*,pk(f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9))", XONLY_KEYS | RANGE | MUSIG | MUSIG_DERIVATION, {{"51201d377b637b5c73f670f5c8a96a2c0bb0d1a682a1fca6aba91fe673501a189782"}, {"51208950c83b117a6c208d5205ffefcf75b187b32512eb7f0d8577db8d9102833036"}, {"5120a49a477c61df73691b77fcd563a80a15ea67bb9c75470310ce5c0f25918db60d"}}, OutputType::BECH32M, /*op_desc_id=*/std::nullopt, {{}, {0, 0}, {0, 1}, {0, 2}});
    Check("tr(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,pk(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*))","tr(f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,pk(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*))","tr(f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,pk(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*))", XONLY_KEYS | RANGE | MUSIG | MUSIG_DERIVATION, {{"512068983d461174afc90c26f3b2821d8a9ced9534586a756763b68371a404635cc8"}, {"5120368e2d864115181bdc8bb5dc8684be8d0760d5c33315570d71a21afce4afd43e"}, {"512097a1e6270b33ad85744677418bae5f59ea9136027223bc6e282c47c167b471d5"}}, OutputType::BECH32M, /*op_desc_id=*/std::nullopt, {{}, {0, 0}, {0, 1}, {0, 2}});
    Check("tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/1,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/1)/2)", "tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1)/2)", "tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1)/2)", XONLY_KEYS | MUSIG | MUSIG_DERIVATION | UNIQUE_XPUBS, {{"5120a17ceacd6422bd5ffd9f165807b254b7d68ad39f179cc4f11545a6835227e97c"}}, OutputType::BECH32M, /*op_desc_id=*/std::nullopt, {{1}, {2}});
    CheckMultipath("rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<1;2;3>/0/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0/*,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/<3;4;5>/*))",
            "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<1;2;3>/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/<3;4;5>/*))",
            {
                "rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/1/0/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0/*,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/3/*))",
                "rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/2/0/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0/*,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/4/*))",
                "rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/3/0/*,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0/*,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4/0/0/5/*))",
            },
            {
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/3/*))",
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/2/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/4/*))",
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/3/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/5/*))",
            },
            {
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/3/*))",
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/2/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/4/*))",
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/3/0/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0/*,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t/0/0/5/*))",
            },
            XONLY_KEYS | RANGE | MUSIG,
            {
                {{"51204ba445a411bd8500476ef916e6d4dd7c137a77e0637e5b0e98339210d78d595a"},{"5120800394c4f39743734c9a15eaa171476814bed0ea19ad771037c5f1ceb20244a9"},{"512011658c4e00fae6f22b9adc2b3823ff3ec6367599783788f4aa8fe1ab3dd0a7ea"}},
                {{"5120b977ae89f221762a61ee986fed7a493426462483afef46f7225765e015934961"},{"5120b70bf732ed38fcc2052075f83901f8588f1016f6741aaacce6e439a02235e5ed"},{"5120d7fa329159ae543b41ca81c7b0e916824ce5d13f61de5b6246dc55a3367f8596"}},
                {{"5120cae8685560b38da78300cc06a230a0f47179f20689d71655a665bdd8c5c875cf"},{"5120ad51a056d67374c56c7f6d9bb1a6d0d5a20449f5805628334dbac8d4ed8686b5"},{"5120e080130242eae1fc92d8c84d7390697e80b4d1e54184bdcbccfc7d6c4fe9bb0f"}},
            },
            OutputType::BECH32M,
            {
                {{}, {1, 0, 0}, {1, 0, 1}, {1, 0, 2}, {0, 0}, {0, 1}, {0, 2}, {0, 0, 3, 0}, {0, 0, 3, 1}, {0, 0, 3, 2}},
                {{}, {2, 0, 0}, {2, 0, 1}, {2, 0, 2}, {0, 0}, {0, 1}, {0, 2}, {0, 0, 4, 0}, {0, 0, 4, 1}, {0, 0, 4, 2}},
                {{}, {3, 0, 0}, {3, 0, 1}, {3, 0, 2}, {0, 0}, {0, 1}, {0, 2}, {0, 0, 5, 0}, {0, 0, 5, 1}, {0, 0, 5, 2}},
            }
    );
    CheckMultipath("rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4)/<3;4;5>/*)",
            "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/<3;4;5>/*)",
            {
                "rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4)/3/*)",
                "rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4)/4/*)",
                "rawtr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4)/5/*)",
            },
            {
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/3/*)",
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/4/*)",
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/5/*)",
            },
            {
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/3/*)",
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/4/*)",
                "rawtr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/5/*)",
            },
            XONLY_KEYS | RANGE | MUSIG | MUSIG_DERIVATION,
            {
                {{"51204a0fecdd99c67eb2afca0efa9a008c8bbc4dbb5ccb094b3eee273127b1ababee"},{"512006120155e6bfd6a3abf8a697caaf5669058395ae0052283a1c6e852d373ceccd"},{"5120d46831206710fca12ef7b562a0812250fdda110146dc1b9ac3a099c81ebcef82"}},
                {{"5120f2b491de0be3b53482253865a5e0f2d2dbdc425d59db0c48f01c6bed9c6687c2"},{"5120601daf543e702b9c28a02f33961dfddfad666d9218b3b0b80177420b37619683"},{"512081dc64aac07811399defde8c959e3a66c56b621360e55ff01c2d43dfe7928b66"}},
                {{"51201bde67648efbd371e63fc5d30325113d0ad5fb853afc53e9b78302708d5fd865"},{"51205bf89fde498522610b5db4eb306b3e1499057aac6d9a56dea832adca4722858b"},{"5120b4a81ca1cc45973422d26d687ab3b586d18508a6dbbbcd38e841400c214c4e83"}},
            },
            OutputType::BECH32M,
            {
                {{}, {3, 0}, {3, 1}, {3, 2}},
                {{}, {4, 0}, {4, 1}, {4, 2}},
                {{}, {5, 0}, {5, 1}, {5, 2}},
            }
    );
    CheckMultipath("tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4)/6/*,pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<7;8;9>/*))",
            "tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/6/*,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<7;8;9>/*))",
            {
                "tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4)/6/*,pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/7/*))",
                "tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4)/6/*,pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/8/*))",
                "tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L,xprv9s21ZrQH143K3jUwNHoqQNrtzJnJmx4Yup8NkNLdVQCymYbPbJXnPhwkfTfxZfptcs3rLAPUXS39oDLgrNKQGwbGsEmJJ8BU3RzQuvShEG4)/6/*,pk(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/9/*))",
            },
            {
                "tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/6/*,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/7/*))",
                "tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/6/*,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/8/*))",
                "tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/6/*,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/9/*))",
            },
            {
                "tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/6/*,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/7/*))",
                "tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/6/*,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/8/*))",
                "tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y,xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t)/6/*,pk(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/9/*))",
            },
            XONLY_KEYS | RANGE | MUSIG | MUSIG_DERIVATION | MIXED_MUSIG,
            {
                {{"5120682fd07aa0f18643fd2c8a2411a0525b5391d7ad54e1904c9c0d7f524e49b39c"},{"51200410a642ea6b9ab884ba892205f484ad716cf0d8426dd739bd67d1636cdce870"},{"5120971dc3136dd90be8aa879d9b0f5c449b62e738cbf7129623098bb3b3aa57eb7d"}},
                {{"51208e4158b3e54e32b4ad22f6dffb9f7968c92dfe96fd0b8fd3d30d3e2558a2a694"},{"512094273ac6b6f8ac060ba5681ba0906e54a51ab67fd08092e9b2af568b82aa1c7c"},{"5120ef0c7915708eb5baa95125e75417306391f339cfc533a19000ab8f2f53da78c1"}},
                {{"5120096d8a1f2091b8e8afa24eafd03b714acaea6a14df5cb673b02c7215b7764aeb"},{"512011833d6e7a6531b40c3a8beb5f59793f6dd10f216384630d78cb3f6319c964f1"},{"5120853417543de5ae914e5cc9de9e52342f253a11faada92148f8b0832650f4eb2f"}},
            },
            OutputType::BECH32M,
            {
                {{}, {6, 0}, {6, 1}, {6, 2}, {7, 0}, {7, 1}, {7, 2}},
                {{}, {6, 0}, {6, 1}, {6, 2}, {8, 0}, {8, 1}, {8, 2}},
                {{}, {6, 0}, {6, 1}, {6, 2}, {9, 0}, {9, 1}, {9, 2}},
            }
    );

    // MuSig2 Parsing Failures
    CheckUnparsable("pk(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "pk(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "pk(): musig() is only allowed in tr() and rawtr()");
    CheckUnparsable("pkh(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "pkh(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "pkh(): musig() is only allowed in tr() and rawtr()");
    CheckUnparsable("wpkh(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "wpkh(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "wpkh(): musig() is only allowed in tr() and rawtr()");
    CheckUnparsable("combo(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "combo(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "combo(): musig() is only allowed in tr() and rawtr()");
    CheckUnparsable("sh(wpkh(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66)))", "sh(wpkh(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66)))", "wpkh(): musig() is only allowed in tr() and rawtr()");
    CheckUnparsable("sh(wsh(pk(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66)))", "sh(wsh(pk(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))))", "pk(): musig() is only allowed in tr() and rawtr()");
    CheckUnparsable("wsh(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "wsh(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "A function is needed within P2WSH");
    CheckUnparsable("sh(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "sh(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66))", "A function is needed within P2SH");
    CheckUnparsable("tr(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66)/0/0)", "tr(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,023590a94e768f8e1815c2f24b4d80a8e3149316c3518ce7b7ad338368d038ca66)/0/0)", "tr(): musig(): derivation requires all participants to be xpubs or xprvs");
    CheckUnparsable("tr(musig(KwDiBf89QgGbjEhKnhXJuH7LrciVrZi3qYjgd9M7rFU74sHUHy8S,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)/0/0)", "tr(musig(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9,03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)/0/0)", "tr(): musig(): derivation requires all participants to be xpubs or xprvs");
    CheckUnparsable("tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*)","tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*)", "tr(): musig(): Cannot have ranged participant keys if musig() also has derivation");
    CheckUnparsable("tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0h/*)","tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0h/*)", "tr(): musig(): cannot have hardened derivation steps");
    CheckUnparsable("tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*h)","tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y)/0/*h)", "tr(): musig(): Cannot have hardened child derivation");
    CheckUnparsable("tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/<0;1>,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/<2;3>)/<3;4>)","tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/<0;1>,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/<2;3>)/<3;4>)", "tr(): musig(): Cannot have multipath participant keys if musig() is also multipath");
    CheckUnparsable("tr(musig()/0)", "tr(musig()/0)", "tr(): musig(): Must contain key expressions");
    CheckUnparsable("tr(musig(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/*)/0)","tr(musig(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/*,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/*)/0)", "tr(): musig(): Cannot have ranged participant keys if musig() also has derivation");

    // Fuzzer crash test cases
    CheckUnparsable("pk(musig(dd}uue/00/)k(", "pk(musig(dd}uue/00/)k(", "Invalid musig() expression");
    CheckUnparsable("tr(musig(tuus(oldepk(gg)ggggfgg)<,z(((((((((((((((((((((st)", "tr(musig(tuus(oldepk(gg)ggggfgg)<,z(((((((((((((((((((((st)","tr(): Too many ')' in musig() expression");
}

BOOST_AUTO_TEST_CASE(descriptor_literal_null_byte)
{
    // Trailing '\0' string literal should be ignored.
    FlatSigningProvider keys;
    std::string err;
    auto descs = Parse("pk(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)", keys, err, /*require_checksum=*/false);
    BOOST_REQUIRE_MESSAGE(!descs.empty(), err);
}

BOOST_AUTO_TEST_SUITE_END()
