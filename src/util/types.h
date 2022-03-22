// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TYPES_H
#define BITCOIN_UTIL_TYPES_H

template <class>
inline constexpr bool ALWAYS_FALSE{false};

struct Disable_Implicit_Copies {
    Disable_Implicit_Copies() = default;
    Disable_Implicit_Copies(Disable_Implicit_Copies&&) = default;
    Disable_Implicit_Copies& operator=(Disable_Implicit_Copies&&) = default;
};

/**
 * Helper to disable implicit copies.
 *
 * This can be used on types that are expensive to copy, and cheap to move.
 *
 * To use, place this as the last line into a class or struct.
 */
#define DISABLE_IMPLICIT_COPIES() \
    Disable_Implicit_Copies _disable_implicit_copies {}

#endif // BITCOIN_UTIL_TYPES_H
