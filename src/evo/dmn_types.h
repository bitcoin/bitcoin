// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_DMN_TYPES_H
#define BITCOIN_EVO_DMN_TYPES_H

#include <amount.h>

#include <limits>
#include <string_view>

enum class MnType : uint16_t {
    Regular = 0,
    Evo = 1,
    COUNT,
    Invalid = std::numeric_limits<uint16_t>::max(),
};

template<typename T> struct is_serializable_enum;
template<> struct is_serializable_enum<MnType> : std::true_type {};

namespace dmn_types {

struct mntype_struct
{
    const int32_t voting_weight;
    const CAmount collat_amount;
    const std::string_view description;
};

constexpr auto Regular = mntype_struct{
    .voting_weight = 1,
    .collat_amount = 1000 * COIN,
    .description = "Regular",
};
constexpr auto Evo = mntype_struct{
    .voting_weight = 4,
    .collat_amount = 4000 * COIN,
    .description = "Evo",
};
constexpr auto Invalid = mntype_struct{
    .voting_weight = 0,
    .collat_amount = MAX_MONEY,
    .description = "Invalid",
};

[[nodiscard]] static constexpr bool IsCollateralAmount(CAmount amount)
{
    return amount == Regular.collat_amount ||
        amount == Evo.collat_amount;
}

} // namespace dmn_types

[[nodiscard]] constexpr const dmn_types::mntype_struct GetMnType(MnType type)
{
    switch (type) {
        case MnType::Regular: return dmn_types::Regular;
        case MnType::Evo: return dmn_types::Evo;
        default: return dmn_types::Invalid;
    }
}

[[nodiscard]] constexpr const bool IsValidMnType(MnType type)
{
    return type < MnType::COUNT;
}

#endif // BITCOIN_EVO_DMN_TYPES_H
