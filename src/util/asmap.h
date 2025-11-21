// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_ASMAP_H
#define BITCOIN_UTIL_ASMAP_H

#include <uint256.h>
#include <util/fs.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

uint32_t Interpret(std::span<const std::byte> asmap, std::span<const std::byte> ip);

bool SanityCheckAsmap(std::span<const std::byte> asmap, int bits);

/** Read and check asmap from provided binary file */
std::vector<std::byte> DecodeAsmap(fs::path path);
/** Check asmap from embedded data */
bool CheckAsmap(std::span<const std::byte> data);
/** Calculate the asmap version, a checksum identifying the asmap being used. */
uint256 AsmapVersion(std::span<const std::byte> data);

#endif // BITCOIN_UTIL_ASMAP_H
