// Copyright (c) 2019-2020 The Revolt Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef REVOLT_UTIL_BIP32_H
#define REVOLT_UTIL_BIP32_H

#include <cstdint>
#include <string>
#include <vector>

/** Parse an HD keypaths like "m/7/0'/2000". */
[[nodiscard]] bool ParseHDKeypath(const std::string& keypath_str, std::vector<uint32_t>& keypath);

/** Write HD keypaths as strings */
std::string WriteHDKeypath(const std::vector<uint32_t>& keypath);
std::string FormatHDKeypath(const std::vector<uint32_t>& path);

#endif // REVOLT_UTIL_BIP32_H
