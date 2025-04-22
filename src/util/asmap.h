// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_ASMAP_H
#define BITCOIN_UTIL_ASMAP_H

#include <uint256.h>
#include <util/fs.h>

#include <cstddef>
#include <cstdint>
#include <vector>

uint32_t Interpret(const std::vector<std::byte>& asmap, const std::vector<std::byte>& ip);

bool SanityCheckASMap(const std::vector<std::byte>& asmap, int bits);

/** Read asmap from provided binary file */
std::vector<std::byte> DecodeAsmap(fs::path path);
/** Calculate the asmap version, a checksum identifying the asmap being used. */
uint256 AsmapVersion(const std::vector<std::byte>& data);

#endif // BITCOIN_UTIL_ASMAP_H
