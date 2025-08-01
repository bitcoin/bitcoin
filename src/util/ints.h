// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_INTS_H
#define BITCOIN_UTIL_INTS_H

#include <cstddef>

/** Return *n* rounded up to the next even value (or *n* itself if already even). */
[[nodiscard]] static constexpr std::size_t RoundUpToEven(std::size_t n) noexcept { return n + (n & 1U); }

#endif // BITCOIN_UTIL_INTS_H
