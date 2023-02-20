// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_UNDERLYING_H
#define BITCOIN_UTIL_UNDERLYING_H
#include <type_traits>

template <typename E>
[[nodiscard]] inline constexpr typename std::underlying_type<E>::type ToUnderlying(E e) noexcept {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

#endif //BITCOIN_UTIL_UNDERLYING_H
