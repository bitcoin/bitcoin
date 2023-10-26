// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BIP32_H
#define BITCOIN_UTIL_BIP32_H

#include <cstdint>
#include <string>
#include <vector>

/** Parse an HD keypath
    *
    * @param[in] keypath_str the path, e.g. "m/7'/0/2000" or "m/0h/0h"
    * @param[out] keypath the parsed path values
    * @param[in] check_hardened_marker Check that hardened deriviation markers h and ' are not mixed.
    */
[[nodiscard]] bool ParseHDKeypath(const std::string& keypath_str, std::vector<uint32_t>& keypath, bool check_hardened_marker = false);

/** Write HD keypaths as strings */
std::string WriteHDKeypath(const std::vector<uint32_t>& keypath, bool apostrophe = false);
std::string FormatHDKeypath(const std::vector<uint32_t>& path, bool apostrophe = false);

#endif // BITCOIN_UTIL_BIP32_H
