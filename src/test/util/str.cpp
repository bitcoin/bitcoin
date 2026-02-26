// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <string>

#include <test/util/str.h>

std::vector<std::byte> StringToBuffer(const std::string& str)
{
    auto span = std::as_bytes(std::span(str));
    return {span.begin(), span.end()};
}
