// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <test/fuzz/fuzz.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

//! Types are raw (un)compressed pubkeys, raw xonly pubkeys, raw privkeys (WIF), xpubs, xprvs.
static constexpr uint8_t KEY_TYPES_COUNT{6};
//! How many keys we'll generate in total.
static constexpr size_t TOTAL_KEYS_GENERATED{std::numeric_limits<uint8_t>::max() + 1};

/**
 * Converts a mocked descriptor string to a valid one. Every key in a mocked descriptor key is
 * represented by 2 hex characters preceded by the '%' character. We parse the two hex characters
 * as an index in a list of pre-generated keys. This list contains keys of the various types
 * accepted in descriptor keys expressions.
 */
class MockedDescriptorConverter {
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
    void Init() {
        // The data to use as a private key or a seed for an xprv.
        std::array<std::byte, 32> key_data{std::byte{1}};
        // Generate keys of all kinds and store them in the keys array.
        for (size_t i{0}; i < TOTAL_KEYS_GENERATED; i++) {
            key_data[31] = std::byte(i);

            // If this is a "raw" key, generate a normal privkey. Otherwise generate
            // an extended one.
            if (IdIsCompPubKey(i) || IdIsUnCompPubKey(i) || IdIsXOnlyPubKey(i) || IdIsConstPrivKey(i)) {
                CKey privkey;
                privkey.Set(UCharCast(key_data.begin()), UCharCast(key_data.end()), !IdIsUnCompPubKey(i));
                if (IdIsCompPubKey(i) || IdIsUnCompPubKey(i)) {
                    CPubKey pubkey{privkey.GetPubKey()};
                    keys_str[i] = HexStr(pubkey);
                } else if (IdIsXOnlyPubKey(i)) {
                    const XOnlyPubKey pubkey{privkey.GetPubKey()};
                    keys_str[i] = HexStr(pubkey);
                } else {
                    keys_str[i] = EncodeSecret(privkey);
                }
            } else {
                CExtKey ext_privkey;
                ext_privkey.SetSeed(key_data);
                if (IdIsXprv(i)) {
                    keys_str[i] = EncodeExtKey(ext_privkey);
                } else {
                    const CExtPubKey ext_pubkey{ext_privkey.Neuter()};
                    keys_str[i] = EncodeExtPubKey(ext_pubkey);
                }
            }
        }
    }

    //! Parse an id in the keys vectors from a 2-characters hex string.
    std::optional<uint8_t> IdxFromHex(std::string_view hex_characters) const {
        if (hex_characters.size() != 2) return {};
        auto idx = ParseHex(hex_characters);
        if (idx.size() != 1) return {};
        return idx[0];
    }

    //! Get an actual descriptor string from a descriptor string whose keys were mocked.
    std::optional<std::string> GetDescriptor(std::string_view mocked_desc) const {
        // The smallest fragment would be "pk(%00)"
        if (mocked_desc.size() < 7) return {};

        // The actual descriptor string to be returned.
        std::string desc;
        desc.reserve(mocked_desc.size());

        // Replace all occurrences of '%' followed by two hex characters with the corresponding key.
        for (size_t i = 0; i < mocked_desc.size();) {
            if (mocked_desc[i] == '%') {
                if (i + 3 >= mocked_desc.size()) return {};
                if (const auto idx = IdxFromHex(mocked_desc.substr(i + 1, 2))) {
                    desc += keys_str[*idx];
                    i += 3;
                } else {
                    return {};
                }
            } else {
                desc += mocked_desc[i++];
            }
        }

        return desc;
    }
};

//! The converter of mocked descriptors, needs to be initialized when the target is.
MockedDescriptorConverter MOCKED_DESC_CONVERTER;

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

void initialize_descriptor_parse()
{
    ECC_Start();
    SelectParams(ChainType::MAIN);
}

void initialize_mocked_descriptor_parse()
{
    initialize_descriptor_parse();
    MOCKED_DESC_CONVERTER.Init();
}

FUZZ_TARGET(mocked_descriptor_parse, .init = initialize_mocked_descriptor_parse)
{
    const std::string mocked_descriptor{buffer.begin(), buffer.end()};
    if (const auto descriptor = MOCKED_DESC_CONVERTER.GetDescriptor(mocked_descriptor)) {
        FlatSigningProvider signing_provider;
        std::string error;
        const auto desc = Parse(*descriptor, signing_provider, error);
        if (desc) TestDescriptor(*desc, signing_provider, error);
    }
}

FUZZ_TARGET(descriptor_parse, .init = initialize_descriptor_parse)
{
    const std::string descriptor(buffer.begin(), buffer.end());
    FlatSigningProvider signing_provider;
    std::string error;
    for (const bool require_checksum : {true, false}) {
        const auto desc = Parse(descriptor, signing_provider, error, require_checksum);
        if (desc) TestDescriptor(*desc, signing_provider, error);
    }
}
