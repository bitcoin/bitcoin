// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BYTE_UNITS_H
#define BITCOIN_UTIL_BYTE_UNITS_H

#include <util/overflow.h>

#include <limits>
#include <stdexcept>

namespace util::detail {
template <unsigned SHIFT>
constexpr size_t ByteUnitsToBytes(unsigned long long units, const char* exception_msg)
{
    const auto bytes{CheckedLeftShift(units, SHIFT)};
    if (!bytes || *bytes > std::numeric_limits<size_t>::max()) {
        throw std::overflow_error(exception_msg);
    }
    return *bytes;
}
} // namespace util::detail

//! Overflow-safe conversion of MiB to bytes.
constexpr size_t operator""_MiB(unsigned long long mebibytes)
{
    return util::detail::ByteUnitsToBytes<20>(mebibytes, "MiB value too large for size_t byte conversion");
}

//! Overflow-safe conversion of GiB to bytes.
constexpr size_t operator""_GiB(unsigned long long gibibytes)
{
    return util::detail::ByteUnitsToBytes<30>(gibibytes, "GiB value too large for size_t byte conversion");
}

#endif // BITCOIN_UTIL_BYTE_UNITS_H
