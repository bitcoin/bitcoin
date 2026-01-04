// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_RANGES_H
#define BITCOIN_UTIL_RANGES_H

#include <optional>
#include <ranges>

namespace ranges {
using namespace std::ranges;

template <typename X, typename Z>
constexpr inline auto find_if_opt(const X& ds, const Z& fn)
{
    const auto it = std::ranges::find_if(ds, fn);
    if (it != std::end(ds)) {
        return std::make_optional(*it);
    }
    return std::optional<std::decay_t<decltype(*it)>>{};
}
}

#endif // BITCOIN_UTIL_RANGES_H
