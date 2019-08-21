// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/splitstring.h>

std::function<bool(char)> IsAnyOf(const std::string& any_of_separator)
{
    auto predicate = [&](char c) {
        return (any_of_separator.find(c) != std::string::npos);
    };

    return predicate;
}
