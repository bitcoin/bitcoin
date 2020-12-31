// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OPTIONAL_H
#define BITCOIN_OPTIONAL_H

#include <optional>
#include <utility>

//! Substitute for C++17 std::optional
//! DEPRECATED use std::optional in new code.
template <typename T>
using Optional = std::optional<T>;

//! Substitute for C++17 std::nullopt
//! DEPRECATED use std::nullopt in new code.
static auto& nullopt = std::nullopt;

#endif // BITCOIN_OPTIONAL_H
