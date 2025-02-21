// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_OVERFLOW_H
#define BITCOIN_UTIL_OVERFLOW_H

#include <climits>
#include <concepts>
#include <limits>
#include <optional>
#include <type_traits>

template <class T>
[[nodiscard]] bool AdditionOverflow(const T i, const T j) noexcept
{
    static_assert(std::is_integral_v<T>, "Integral required.");
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

/**
 * @brief Left bit shift with overflow checking.
 * @param input The input value to be left shifted.
 * @param shift The number of bits to left shift.
 * @return (input * 2^shift) or nullopt if it would not fit in the return type.
 */
template <std::integral T>
constexpr std::optional<T> CheckedLeftShift(T input, unsigned shift) noexcept
{
    if (shift == 0 || input == 0) return input;
    // Avoid undefined c++ behaviour if shift is >= number of bits in T.
    if (shift >= sizeof(T) * CHAR_BIT) return std::nullopt;
    // If input << shift is too big to fit in T, return nullopt.
    if (input > (std::numeric_limits<T>::max() >> shift)) return std::nullopt;
    if (input < (std::numeric_limits<T>::min() >> shift)) return std::nullopt;
    return input << shift;
}

/**
 * @brief Left bit shift with safe minimum and maximum values.
 * @param input The input value to be left shifted.
 * @param shift The number of bits to left shift.
 * @return (input * 2^shift) clamped to fit between the lowest and highest
 *         representable values of the type T.
 */
template <std::integral T>
constexpr T SaturatingLeftShift(T input, unsigned shift) noexcept
{
    if (auto result{CheckedLeftShift(input, shift)}) return *result;
    // If input << shift is too big to fit in T, return biggest positive or negative
    // number that fits.
    return input < 0 ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
}

#endif // BITCOIN_UTIL_OVERFLOW_H
