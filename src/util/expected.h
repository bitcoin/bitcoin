// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_UTIL_EXPECTED_H
#define BITCOIN_UTIL_EXPECTED_H

#include <expected>

// New code is free to use std::(un)expected.
// Old code can be converted at some point, possibly with a scripted-diff.
namespace util {
template <class T, class E>
using Expected = std::expected<T, E>;
template <class E>
using Unexpected = std::unexpected<E>;
} // namespace util

#endif // BITCOIN_UTIL_EXPECTED_H
