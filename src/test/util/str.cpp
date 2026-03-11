// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <string>

#include <test/util/str.h>

/** Returns a span view of the string. */
std::span<const std::byte> StringToBytes(std::string_view str LIFETIMEBOUND)
{
    return std::as_bytes(std::span{str});
}
