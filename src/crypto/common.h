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
concept ByteType = std::same_as<B, unsigned char> || std::same_as<B, std::byte>;

template <ByteType B>
inline uint16_t ReadLE16(const B* ptr)
{
    uint16_t x;
    memcpy(&x, ptr, 2);
    return le16toh_internal(x);
}

template <ByteType B>
inline uint32_t ReadLE32(const B* ptr)
{
    uint32_t x;
    memcpy(&x, ptr, 4);
    return le32toh_internal(x);
}

template <ByteType B>
inline uint64_t ReadLE64(const B* ptr)
{
    uint64_t x;
    memcpy(&x, ptr, 8);
    return le64toh_internal(x);
}

template <ByteType B>
inline void WriteLE16(B* ptr, uint16_t x)
{
    uint16_t v = htole16_internal(x);
    memcpy(ptr, &v, 2);
}

template <ByteType B>
inline void WriteLE32(B* ptr, uint32_t x)
{
    uint32_t v = htole32_internal(x);
    memcpy(ptr, &v, 4);
}

template <ByteType B>
inline void WriteLE64(B* ptr, uint64_t x)
{
    uint64_t v = htole64_internal(x);
    memcpy(ptr, &v, 8);
}

template <ByteType B>
inline uint16_t ReadBE16(const B* ptr)
{
    uint16_t x;
    memcpy(&x, ptr, 2);
    return be16toh_internal(x);
}

template <ByteType B>
inline uint32_t ReadBE32(const B* ptr)
{
    uint32_t x;
    memcpy(&x, ptr, 4);
    return be32toh_internal(x);
}

template <ByteType B>
inline uint64_t ReadBE64(const B* ptr)
{
    uint64_t x;
    memcpy(&x, ptr, 8);
    return be64toh_internal(x);
}

template <ByteType B>
inline void WriteBE16(B* ptr, uint16_t x)
{
    uint16_t v = htobe16_internal(x);
    memcpy(ptr, &v, 2);
}

template <ByteType B>
inline void WriteBE32(B* ptr, uint32_t x)
{
    uint32_t v = htobe32_internal(x);
    memcpy(ptr, &v, 4);
}

template <ByteType B>
inline void WriteBE64(B* ptr, uint64_t x)
{
    uint64_t v = htobe64_internal(x);
    memcpy(ptr, &v, 8);
}

#endif // BITCOIN_CRYPTO_COMMON_H
