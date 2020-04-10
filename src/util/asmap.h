// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_ASMAP_H
#define BITCOIN_UTIL_ASMAP_H

#include <stdint.h>
#include <vector>

uint32_t Interpret(const std::vector<bool> &asmap, const std::vector<bool> &ip);

bool SanityCheckASMap(const std::vector<bool>& asmap, int bits);

std::vector<std::pair<std::vector<bool>, uint32_t>> DecodeASMap(const std::vector<bool>& asmap, int bits);

/** Encode a mapping to an asmap.
 *
 * If approx is true, unmapped prefixes will be reassigned to minimize output size.
 */
std::vector<bool> EncodeASMap(std::vector<std::pair<std::vector<bool>, uint32_t>> input, bool approx);

#endif // BITCOIN_UTIL_ASMAP_H
