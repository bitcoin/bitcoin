// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_DESCRIPTOR_H
#define BITCOIN_TEST_FUZZ_UTIL_DESCRIPTOR_H

#include <key_io.h>
#include <util/strencodings.h>
#include <script/descriptor.h>

#include <functional>

/**
 * Converts a mocked descriptor string to a valid one. Every key in a mocked descriptor key is
 * represented by 2 hex characters preceded by the '%' character. We parse the two hex characters
 * as an index in a list of pre-generated keys. This list contains keys of the various types
 * accepted in descriptor keys expressions.
 */
class MockedDescriptorConverter {
private:
    //! Types are raw (un)compressed pubkeys, raw xonly pubkeys, raw privkeys (WIF), xpubs, xprvs.
    static constexpr uint8_t KEY_TYPES_COUNT{6};
    //! How many keys we'll generate in total.
    static constexpr size_t TOTAL_KEYS_GENERATED{std::numeric_limits<uint8_t>::max() + 1};
    //! 256 keys of various types.
    std::array<std::string, TOTAL_KEYS_GENERATED> keys_str;

public:
    // We derive the type of key to generate from the 1-byte id parsed from hex.
    bool IdIsCompPubKey(uint8_t idx) const { return idx % KEY_TYPES_COUNT == 0; }
    bool IdIsUnCompPubKey(uint8_t idx) const { return idx % KEY_TYPES_COUNT == 1; }
    bool IdIsXOnlyPubKey(uint8_t idx) const { return idx % KEY_TYPES_COUNT == 2; }
    bool IdIsConstPrivKey(uint8_t idx) const { return idx % KEY_TYPES_COUNT == 3; }
    bool IdIsXpub(uint8_t idx) const { return idx % KEY_TYPES_COUNT == 4; }
    bool IdIsXprv(uint8_t idx) const { return idx % KEY_TYPES_COUNT == 5; }

    //! When initializing the target, populate the list of keys.
    void Init();

    //! Parse an id in the keys vectors from a 2-characters hex string.
    std::optional<uint8_t> IdxFromHex(std::string_view hex_characters) const;

    //! Get an actual descriptor string from a descriptor string whose keys were mocked.
    std::optional<std::string> GetDescriptor(std::string_view mocked_desc) const;
};

/** Test a successfully parsed descriptor. */
static void TestDescriptor(const Descriptor& desc, FlatSigningProvider& sig_provider, std::string& dummy)
{
    // Trivial helpers.
    (void)desc.IsRange();
    const bool is_solvable{desc.IsSolvable()};
    (void)desc.IsSingleType();
    (void)desc.GetOutputType();

    // Serialization to string representation.
    (void)desc.ToString();
    (void)desc.ToPrivateString(sig_provider, dummy);
    (void)desc.ToNormalizedString(sig_provider, dummy);

    // Serialization to Script.
    DescriptorCache cache;
    std::vector<CScript> out_scripts;
    (void)desc.Expand(0, sig_provider, out_scripts, sig_provider, &cache);
    (void)desc.ExpandPrivate(0, sig_provider, sig_provider);
    (void)desc.ExpandFromCache(0, cache, out_scripts, sig_provider);

    // If we could serialize to script we must be able to infer using the same provider.
    if (!out_scripts.empty()) {
        assert(InferDescriptor(out_scripts.back(), sig_provider));

        // The ScriptSize() must match the size of the serialized Script. (ScriptSize() is set for all descs but 'combo()'.)
        const bool is_combo{!desc.IsSingleType()};
        assert(is_combo || desc.ScriptSize() == out_scripts.back().size());
    }

    const auto max_sat_maxsig{desc.MaxSatisfactionWeight(true)};
    const auto max_sat_nonmaxsig{desc.MaxSatisfactionWeight(true)};
    const auto max_elems{desc.MaxSatisfactionElems()};
    // We must be able to estimate the max satisfaction size for any solvable descriptor (but combo).
    const bool is_nontop_or_nonsolvable{!is_solvable || !desc.GetOutputType()};
    const bool is_input_size_info_set{max_sat_maxsig && max_sat_nonmaxsig && max_elems};
    assert(is_input_size_info_set || is_nontop_or_nonsolvable);
}

#endif // BITCOIN_TEST_FUZZ_UTIL_DESCRIPTOR_H
