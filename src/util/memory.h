// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_MEMORY_H
#define BITCOIN_UTIL_MEMORY_H

#include <memory>
#include <utility>

//! Substitute for C++14 std::make_unique.
//! DEPRECATED use std::make_unique in new code.
template <typename T, typename... Args>
std::unique_ptr<T> MakeUnique(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

#endif
