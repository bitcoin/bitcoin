// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/pos/helpers.h>

#include <iostream>
#include <util/strencodings.h>

namespace blsct {
uint256
CalculateKernelHash(const uint32_t& prevTime, const uint64_t& stakeModifier, const MclG1Point& phi, const uint32_t& time)
{
    HashWriter ss{};

    ss << prevTime << stakeModifier << phi << time;

    return ss.GetHash();
}
} // namespace blsct