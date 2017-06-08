// Copyright (c) 2014-2015 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPAT_ENDIAN_H
#define BITCOIN_COMPAT_ENDIAN_H

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include <stdint.h>

#include "compat/byteswap.h"

#if defined(HAVE_ENDIAN_H)
#include <endian.h>
#elif defined(HAVE_SYS_ENDIAN_H)
#include <sys/endian.h>
#endif

#if defined(WORDS_BIGENDIAN)

#if HAVE_DECL_HTOBE16 == 0
inline uint16_t htobe16(uint16_t host_16bits)
{
    return host_16bits;
}
#endif // HAVE_DECL_HTOBE16

#if HAVE_DECL_HTOLE16 == 0
inline uint16_t htole16(uint16_t host_16bits)
{
    return bswap_16(host_16bits);
}
#endif // HAVE_DECL_HTOLE16

#if HAVE_DECL_BE16TOH == 0
inline uint16_t be16toh(uint16_t big_endian_16bits)
{
    return big_endian_16bits;
}
#endif // HAVE_DECL_BE16TOH

#if HAVE_DECL_LE16TOH == 0
inline uint16_t le16toh(uint16_t little_endian_16bits)
{
    return bswap_16(little_endian_16bits);
}
#endif // HAVE_DECL_LE16TOH

#if HAVE_DECL_HTOBE32 == 0
inline uint32_t htobe32(uint32_t host_32bits)
{
    return host_32bits;
}
#endif // HAVE_DECL_HTOBE32

#if HAVE_DECL_HTOLE32 == 0
inline uint32_t htole32(uint32_t host_32bits)
{
    return bswap_32(host_32bits);
}
#endif // HAVE_DECL_HTOLE32

#if HAVE_DECL_BE32TOH == 0
inline uint32_t be32toh(uint32_t big_endian_32bits)
{
    return big_endian_32bits;
}
#endif // HAVE_DECL_BE32TOH

#if HAVE_DECL_LE32TOH == 0
inline uint32_t le32toh(uint32_t little_endian_32bits)
{
    return bswap_32(little_endian_32bits);
}
#endif // HAVE_DECL_LE32TOH

#if HAVE_DECL_HTOBE64 == 0
inline uint64_t htobe64(uint64_t host_64bits)
{
    return host_64bits;
}
#endif // HAVE_DECL_HTOBE64

#if HAVE_DECL_HTOLE64 == 0
inline uint64_t htole64(uint64_t host_64bits)
{
    return bswap_64(host_64bits);
}
#endif // HAVE_DECL_HTOLE64

#if HAVE_DECL_BE64TOH == 0
inline uint64_t be64toh(uint64_t big_endian_64bits)
{
    return big_endian_64bits;
}
#endif // HAVE_DECL_BE64TOH

#if HAVE_DECL_LE64TOH == 0
inline uint64_t le64toh(uint64_t little_endian_64bits)
{
    return bswap_64(little_endian_64bits);
}
#endif // HAVE_DECL_LE64TOH

#else // WORDS_BIGENDIAN

#if HAVE_DECL_HTOBE16 == 0
inline uint16_t htobe16(uint16_t host_16bits)
{
    return bswap_16(host_16bits);
}
#endif // HAVE_DECL_HTOBE16

#if HAVE_DECL_HTOLE16 == 0
inline uint16_t htole16(uint16_t host_16bits)
{
    return host_16bits;
}
#endif // HAVE_DECL_HTOLE16

#if HAVE_DECL_BE16TOH == 0
inline uint16_t be16toh(uint16_t big_endian_16bits)
{
    return bswap_16(big_endian_16bits);
}
#endif // HAVE_DECL_BE16TOH

#if HAVE_DECL_LE16TOH == 0
inline uint16_t le16toh(uint16_t little_endian_16bits)
{
    return little_endian_16bits;
}
#endif // HAVE_DECL_LE16TOH

#if HAVE_DECL_HTOBE32 == 0
inline uint32_t htobe32(uint32_t host_32bits)
{
    return bswap_32(host_32bits);
}
#endif // HAVE_DECL_HTOBE32

#if HAVE_DECL_HTOLE32 == 0
inline uint32_t htole32(uint32_t host_32bits)
{
    return host_32bits;
}
#endif // HAVE_DECL_HTOLE32

#if HAVE_DECL_BE32TOH == 0
inline uint32_t be32toh(uint32_t big_endian_32bits)
{
    return bswap_32(big_endian_32bits);
}
#endif // HAVE_DECL_BE32TOH

#if HAVE_DECL_LE32TOH == 0
inline uint32_t le32toh(uint32_t little_endian_32bits)
{
    return little_endian_32bits;
}
#endif // HAVE_DECL_LE32TOH

#if HAVE_DECL_HTOBE64 == 0
inline uint64_t htobe64(uint64_t host_64bits)
{
    return bswap_64(host_64bits);
}
#endif // HAVE_DECL_HTOBE64

#if HAVE_DECL_HTOLE64 == 0
inline uint64_t htole64(uint64_t host_64bits)
{
    return host_64bits;
}
#endif // HAVE_DECL_HTOLE64

#if HAVE_DECL_BE64TOH == 0
inline uint64_t be64toh(uint64_t big_endian_64bits)
{
    return bswap_64(big_endian_64bits);
}
#endif // HAVE_DECL_BE64TOH

#if HAVE_DECL_LE64TOH == 0
inline uint64_t le64toh(uint64_t little_endian_64bits)
{
    return little_endian_64bits;
}
#endif // HAVE_DECL_LE64TOH

#endif // WORDS_BIGENDIAN

#endif // BITCOIN_COMPAT_ENDIAN_H
