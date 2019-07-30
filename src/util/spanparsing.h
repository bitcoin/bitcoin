// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SPANPARSING_H
#define BITCOIN_UTIL_SPANPARSING_H

#include <span.h>

#include <string>
#include <vector>

namespace spanparsing {

/** Parse a constant. If successful, sp is updated to skip the constant and return true. */
bool Const(const std::string& str, Span<const char>& sp);

/** Parse a function call. If successful, sp is updated to be the function's argument(s). */
bool Func(const std::string& str, Span<const char>& sp);

/** Return the expression that sp begins with, and update sp to skip it. */
Span<const char> Expr(Span<const char>& sp);

/** Split a string on every instance of sep, returning a vector. */
std::vector<Span<const char>> Split(const Span<const char>& sp, char sep);

} // namespace spanparsing

#endif // BITCOIN_UTIL_SPANPARSING_H
