// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_ASMAP_H
#define BITCOIN_UTIL_ASMAP_H

#include <uint256.h>
#include <util/fs.h>

#include <cstdint>
#include <span>
#include <vector>

uint32_t Interpret(const std::span<const std::byte>& asmap, const std::span<const std::byte>& ip);

bool SanityCheckASMap(const std::span<const std::byte>& asmap, int bits);

/** Read and check asmap from provided binary file */
std::vector<std::byte> DecodeAsmap(fs::path path);
/** Check asmap from embedded data */
std::span<const std::byte> CheckAsmap(const std::span<const std::byte>& data);
/** Calculate asmap checksum */
uint256 AsmapChecksum(const std::span<const std::byte>& data);

#endif // BITCOIN_UTIL_ASMAP_H
