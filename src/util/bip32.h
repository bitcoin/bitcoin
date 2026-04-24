// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BIP32_H
#define BITCOIN_UTIL_BIP32_H

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

/** BIP32 unhardened derivation index (no high bit set) */
static constexpr uint32_t BIP32_UNHARDENED_FLAG = 0x0;
/** BIP32 hardened derivation flag (2^31) */
static constexpr uint32_t BIP32_HARDENED_FLAG = 0x80000000;

/** Parse a single key path element like "0", "0'", or "0h".
 *  Returns the derivation index and sets is_hardened, or nullopt on failure
 *  (in which case `error` is populated with a human-readable message). */
std::optional<uint32_t> ParseKeyPathElement(std::span<const char> elem, bool& is_hardened, std::string& error);

/** Parse an HD keypaths like "m/7/0'/2000". */
[[nodiscard]] bool ParseHDKeypath(const std::string& keypath_str, std::vector<uint32_t>& keypath);

/** Write HD keypaths as strings */
std::string WriteHDKeypath(const std::vector<uint32_t>& keypath, bool apostrophe = false);
std::string FormatHDKeypath(const std::vector<uint32_t>& path, bool apostrophe = false);

#endif // BITCOIN_UTIL_BIP32_H
