// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_DESCRIPTOR_H
#define BITCOIN_TEST_FUZZ_UTIL_DESCRIPTOR_H

#include <key_io.h>
#include <util/strencodings.h>
#include <script/descriptor.h>
#include <test/fuzz/fuzz.h>

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

//! Default maximum number of derivation indexes in a single derivation path when limiting its depth.
constexpr int MAX_DEPTH{2};

/**
 * Whether the buffer, if it represents a valid descriptor, contains a derivation path deeper than
 * a given maximum depth. Note this may also be hit for deriv paths in origins.
 */
bool HasDeepDerivPath(const FuzzBufferType& buff, const int max_depth = MAX_DEPTH);

//! Default maximum number of sub-fragments.
constexpr int MAX_SUBS{1'000};
//! Maximum number of nested sub-fragments we'll allow in a descriptor.
constexpr size_t MAX_NESTED_SUBS{10'000};

/**
 * Whether the buffer, if it represents a valid descriptor, contains a fragment with more
 * sub-fragments than the given maximum.
 */
bool HasTooManySubFrag(const FuzzBufferType& buff, const int max_subs = MAX_SUBS,
                       const size_t max_nested_subs = MAX_NESTED_SUBS);

//! Default maximum number of wrappers per fragment.
constexpr int MAX_WRAPPERS{100};

/**
 * Whether the buffer, if it represents a valid descriptor, contains a fragment with more
 * wrappers than the given maximum.
 */
bool HasTooManyWrappers(const FuzzBufferType& buff, const int max_wrappers = MAX_WRAPPERS);

#endif // BITCOIN_TEST_FUZZ_UTIL_DESCRIPTOR_H
