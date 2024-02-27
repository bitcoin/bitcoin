// Copyright (c) 2014-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPAT_ENDIAN_H
#define BITCOIN_COMPAT_ENDIAN_H

#include <compat/byteswap.h>

#include <bit>
#include <cstdint>

inline BSWAP_CONSTEXPR uint16_t htobe16_internal(uint16_t host_16bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_16(host_16bits);
        else return host_16bits;
}
inline BSWAP_CONSTEXPR uint16_t htole16_internal(uint16_t host_16bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_16(host_16bits);
        else return host_16bits;
}
inline BSWAP_CONSTEXPR uint16_t be16toh_internal(uint16_t big_endian_16bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_16(big_endian_16bits);
        else return big_endian_16bits;
}
inline BSWAP_CONSTEXPR uint16_t le16toh_internal(uint16_t little_endian_16bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_16(little_endian_16bits);
        else return little_endian_16bits;
}
inline BSWAP_CONSTEXPR uint32_t htobe32_internal(uint32_t host_32bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_32(host_32bits);
        else return host_32bits;
}
inline BSWAP_CONSTEXPR uint32_t htole32_internal(uint32_t host_32bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_32(host_32bits);
        else return host_32bits;
}
inline BSWAP_CONSTEXPR uint32_t be32toh_internal(uint32_t big_endian_32bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_32(big_endian_32bits);
        else return big_endian_32bits;
}
inline BSWAP_CONSTEXPR uint32_t le32toh_internal(uint32_t little_endian_32bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_32(little_endian_32bits);
        else return little_endian_32bits;
}
inline BSWAP_CONSTEXPR uint64_t htobe64_internal(uint64_t host_64bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_64(host_64bits);
        else return host_64bits;
}
inline BSWAP_CONSTEXPR uint64_t htole64_internal(uint64_t host_64bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_64(host_64bits);
        else return host_64bits;
}
inline BSWAP_CONSTEXPR uint64_t be64toh_internal(uint64_t big_endian_64bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_64(big_endian_64bits);
        else return big_endian_64bits;
}
inline BSWAP_CONSTEXPR uint64_t le64toh_internal(uint64_t little_endian_64bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_64(little_endian_64bits);
        else return little_endian_64bits;
}

#endif // BITCOIN_COMPAT_ENDIAN_H
