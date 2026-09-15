// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BIP32_H
#define BITCOIN_UTIL_BIP32_H

#include <cstdint>
#include <span>
#include <string>
#include <util/expected.h>
#include <vector>

/** BIP32 unhardened derivation index (no high bit set) */
static constexpr uint32_t BIP32_UNHARDENED_FLAG = 0x0;
/** BIP32 hardened derivation flag (2^31) */
static constexpr uint32_t BIP32_HARDENED_FLAG = 0x80000000;

struct KeyPathElement {
    /** Derivation index, without the hardened flag */
    uint32_t index;
    bool is_hardened;

    /** Derivation index with the hardened flag applied */
    uint32_t ChildNumber() const { return index | (is_hardened ? BIP32_HARDENED_FLAG : BIP32_UNHARDENED_FLAG); }
};

/** Parse a single key path element like "0", "0'", or "0h".
 *  Returns the derivation index and hardened status, or an error message. */
util::Expected<KeyPathElement, std::string> ParseKeyPathElement(std::span<const char> elem);

/** Parse an HD keypaths like "m/7/0'/2000". */
[[nodiscard]] bool ParseHDKeypathLegacy(const std::string& keypath_str, std::vector<uint32_t>& keypath);

/** Write HD keypaths as strings */
std::string WriteHDKeypath(const std::vector<uint32_t>& keypath, bool apostrophe = false);
std::string FormatHDKeypath(const std::vector<uint32_t>& path, bool apostrophe = false);

/** Whether a parsed HD keypath contains at least one hardened derivation step. */
bool HasHardenedDerivation(std::span<const uint32_t> keypath);

#endif // BITCOIN_UTIL_BIP32_H
