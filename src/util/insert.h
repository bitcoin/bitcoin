// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_INSERT_H
#define BITCOIN_UTIL_INSERT_H

#include <set>

namespace util {

//! Simplification of std insertion
template <typename TsetT, typename Compare, typename Tsrc>
inline void insert(std::set<TsetT, Compare>& dst, const Tsrc& src) {
    dst.insert(src.begin(), src.end());
}

} // namespace util

#endif // BITCOIN_UTIL_INSERT_H
