// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_DESCRIPTOR_H
#define BITCOIN_TEST_FUZZ_UTIL_DESCRIPTOR_H

#include <array>
#include <cinttypes>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>

/**
 * Converts a mocked descriptor string to a valid one. Every key in a mocked descriptor is
 * represented by 2 hex characters preceded by the '%' character. We parse the two hex characters
 * as an index in a list of pre-generated keys. This list contains keys of the various types
 * accepted in descriptor key expressions.
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
bool HasDeepDerivPath(std::span<const uint8_t> buff, int max_depth = MAX_DEPTH);

//! Default maximum number of sub-fragments.
constexpr int MAX_SUBS{1'000};
//! Maximum number of nested sub-fragments we'll allow in a descriptor.
constexpr size_t MAX_NESTED_SUBS{10'000};

/**
 * Whether the buffer, if it represents a valid descriptor, contains a fragment with more
 * sub-fragments than the given maximum.
 */
bool HasTooManySubFrag(std::span<const uint8_t> buff, int max_subs = MAX_SUBS,
                       size_t max_nested_subs = MAX_NESTED_SUBS);

//! Default maximum number of wrappers per fragment.
constexpr int MAX_WRAPPERS{100};

/**
 * Whether the buffer, if it represents a valid descriptor, contains a fragment with more
 * wrappers than the given maximum.
 */
bool HasTooManyWrappers(std::span<const uint8_t> buff, int max_wrappers = MAX_WRAPPERS);

/// Default maximum leaf size. This should be large enough to cover an extended
/// key, including paths "/", inside and outside of "[]".
constexpr uint32_t MAX_LEAF_SIZE{200};

/// Whether the expanded buffer (after calling GetDescriptor() in
/// MockedDescriptorConverter) has a leaf size too large.
bool HasTooLargeLeafSize(std::span<const uint8_t> buff, uint32_t max_leaf_size = MAX_LEAF_SIZE);

/// Deriving "expensive" descriptors will consume useful fuzz compute. The
/// compute is better spent on a smaller subset of descriptors, which still
/// covers all real end-user settings.
///
/// Use this function after MockedDescriptorConverter::GetDescriptor()
inline bool IsTooExpensive(std::span<const uint8_t> buffer)
{
    // Key derivation is expensive. Deriving deep derivation paths takes a lot of compute and we'd
    // rather spend time elsewhere in this target, like on the actual descriptor syntax. So rule
    // out strings which could correspond to a descriptor containing a too large derivation path.
    if (HasDeepDerivPath(buffer)) return true;

    // Some fragments can take a virtually unlimited number of sub-fragments (thresh, multi_a) but
    // may perform quadratic operations on them. Limit the number of sub-fragments per fragment.
    if (HasTooManySubFrag(buffer)) return true;

    // The script building logic performs quadratic copies in the number of nested wrappers. Limit
    // the number of nested wrappers per fragment.
    if (HasTooManyWrappers(buffer)) return true;

    // If any suspected leaf is too large, it will likely not represent a valid
    // use-case. Also, possible base58 parsing in the leaf is quadratic. So
    // limit the leaf size.
    if (HasTooLargeLeafSize(buffer)) return true;

    return false;
}
#endif // BITCOIN_TEST_FUZZ_UTIL_DESCRIPTOR_H
