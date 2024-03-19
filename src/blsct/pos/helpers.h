// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_POS_HELPERS_H
#define BLSCT_POS_HELPERS_H

#include <blsct/arith/mcl/mcl.h>
#include <uint256.h>

#define MODIFIER_INTERVAL_RATIO 3

namespace blsct {
uint256 CalculateKernelHash(const uint32_t& prevTime, const uint64_t& stakeModifier, const MclG1Point& phi, const uint32_t& time);
} // namespace blsct

#endif // BLSCT_POS_H