// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_OVERFLOW_H
#define BITCOIN_UTIL_OVERFLOW_H

#include <limits>
#include <optional>
#include <type_traits>

template <class T>
[[nodiscard]] bool AdditionOverflow(const T i, const T j) noexcept
{
    static_assert(std::is_integral<T>::value, "Integral required.");
    if constexpr (std::numeric_limits<T>::is_signed) {
        return (i > 0 && j > std::numeric_limits<T>::max() - i) ||
               (i < 0 && j < std::numeric_limits<T>::min() - i);
    }
    return std::numeric_limits<T>::max() - i < j;
}

template <class T>
[[nodiscard]] std::optional<T> CheckedAdd(const T i, const T j) noexcept
{
    if (AdditionOverflow(i, j)) {
        return std::nullopt;
    }
    return i + j;
}

template <class T>
[[nodiscard]] T SaturatingAdd(const T i, const T j) noexcept
{
    if constexpr (std::numeric_limits<T>::is_signed) {
        if (i > 0 && j > std::numeric_limits<T>::max() - i) {
            return std::numeric_limits<T>::max();
        }
        if (i < 0 && j < std::numeric_limits<T>::min() - i) {
            return std::numeric_limits<T>::min();
        }
    } else {
        if (std::numeric_limits<T>::max() - i < j) {
            return std::numeric_limits<T>::max();
        }
    }
    return i + j;
}

#endif // BITCOIN_UTIL_OVERFLOW_H
