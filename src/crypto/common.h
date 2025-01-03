// Copyright (c) 2014-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_COMMON_H
#define BITCOIN_CRYPTO_COMMON_H

#include <compat/endian.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>

template <typename B>
concept ByteType = std::same_as<B, uint8_t> || std::same_as<B, std::byte>;

uint16_t ReadLE16(const ByteType auto* ptr) noexcept
{
    uint16_t x;
    std::memcpy(&x, ptr, sizeof(x));
    return le16toh_internal(x);
}

uint32_t ReadLE32(const ByteType auto* ptr) noexcept
{
    uint32_t x;
    std::memcpy(&x, ptr, sizeof(x));
    return le32toh_internal(x);
}

uint64_t ReadLE64(const ByteType auto* ptr) noexcept
{
    uint64_t x;
    std::memcpy(&x, ptr, sizeof(x));
    return le64toh_internal(x);
}

void WriteLE16(ByteType auto* ptr, const uint16_t x) noexcept
{
    const uint16_t v{htole16_internal(x)};
    std::memcpy(ptr, &v, sizeof(v));
}

void WriteLE32(ByteType auto* ptr, const uint32_t x) noexcept
{
    const uint32_t v{htole32_internal(x)};
    std::memcpy(ptr, &v, sizeof(v));
}

void WriteLE64(ByteType auto* ptr, const uint64_t x) noexcept
{
    const uint64_t v{htole64_internal(x)};
    std::memcpy(ptr, &v, sizeof(v));
}

uint16_t ReadBE16(const ByteType auto* ptr) noexcept
{
    uint16_t x;
    std::memcpy(&x, ptr, sizeof(x));
    return be16toh_internal(x);
}

uint32_t ReadBE32(const ByteType auto* ptr) noexcept
{
    uint32_t x;
    std::memcpy(&x, ptr, sizeof(x));
    return be32toh_internal(x);
}

uint64_t ReadBE64(const ByteType auto* ptr) noexcept
{
    uint64_t x;
    std::memcpy(&x, ptr, sizeof(x));
    return be64toh_internal(x);
}

void WriteBE16(ByteType auto* ptr, const uint16_t x) noexcept
{
    const uint16_t v{htobe16_internal(x)};
    std::memcpy(ptr, &v, sizeof(v));
}

void WriteBE32(ByteType auto* ptr, const uint32_t x) noexcept
{
    const uint32_t v{htobe32_internal(x)};
    std::memcpy(ptr, &v, sizeof(v));
}

void WriteBE64(ByteType auto* ptr, const uint64_t x) noexcept
{
    const uint64_t v{htobe64_internal(x)};
    std::memcpy(ptr, &v, sizeof(v));
}

#endif // BITCOIN_CRYPTO_COMMON_H
