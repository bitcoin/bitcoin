// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_EXCEPTION_H
#define BITCOIN_UTIL_EXCEPTION_H

#include <exception>
#include <source_location>
#include <string_view>

void PrintExceptionContinue(
    const std::exception* pex,
    std::string_view context = {},
    std::source_location caught_at = std::source_location::current());

#endif // BITCOIN_UTIL_EXCEPTION_H
