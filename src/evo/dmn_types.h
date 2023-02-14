// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_DMN_TYPES_H
#define BITCOIN_EVO_DMN_TYPES_H

#include <amount.h>
#include <cassert>
#include <string_view>

class CDeterministicMNType
{
public:
    uint8_t index;
    int32_t voting_weight;
    CAmount collat_amount;
    std::string_view description;
};

namespace MnType {
constexpr auto Regular = CDeterministicMNType{
    .index = 0,
    .voting_weight = 1,
    .collat_amount = 1000 * COIN,
    .description = "Regular",
};
constexpr auto HighPerformance = CDeterministicMNType{
    .index = 1,
    .voting_weight = 4,
    .collat_amount = 4000 * COIN,
    .description = "HighPerformance",
};
} // namespace MnType

constexpr const auto& GetMnType(int index)
{
    switch (index) {
    case 0: return MnType::Regular;
    case 1: return MnType::HighPerformance;
    default: assert(false);
    }
}

#endif // BITCOIN_EVO_DMN_TYPES_H
