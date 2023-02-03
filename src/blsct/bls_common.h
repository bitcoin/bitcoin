// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_BLS12_381_COMMON_H
#define NAVCOIN_BLSCT_BLS12_381_COMMON_H

#include <vector>
#include <cstdint>

namespace blsct {

using Message = std::vector<uint8_t>;

class BLS12_381_Common
{
public:
    inline static const std::vector<uint8_t> BLSCTBALANCE = {
        'B', 'L', 'S', 'C', 'T', 'B', 'A', 'L', 'A', 'N', 'C', 'E'
    };
};

}

#endif  // NAVCOIN_BLSCT_BLS12_381_COMMON_H
