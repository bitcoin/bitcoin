// Copyright (c) 2014-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPAT_BYTESWAP_H
#define BITCOIN_COMPAT_BYTESWAP_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <stdint.h>

#if defined(HAVE_BYTESWAP_H)
#include <byteswap.h>
#endif

#if defined(MAC_OSX)

#if !defined(bswap_16)

// Mac OS X / Darwin features; we include a check for bswap_16 because if it is already defined, protobuf has
// defined these macros for us already; if it isn't, we do it ourselves. In either case, we get the exact same
// result regardless which path was taken
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#endif // !defined(bswap_16)

#else
// Non-Mac OS X / non-Darwin

#if HAVE_DECL_BSWAP_16 == 0
inline uint16_t bswap_16(uint16_t x)
{
    return (x >> 8) | (x << 8);
}
#endif // HAVE_DECL_BSWAP16 == 0

#if HAVE_DECL_BSWAP_32 == 0
inline uint32_t bswap_32(uint32_t x)
{
    return (((x & 0xff000000U) >> 24) | ((x & 0x00ff0000U) >>  8) |
            ((x & 0x0000ff00U) <<  8) | ((x & 0x000000ffU) << 24));
}
#endif // HAVE_DECL_BSWAP32 == 0

#if HAVE_DECL_BSWAP_64 == 0
inline uint64_t bswap_64(uint64_t x)
{
     return (((x & 0xff00000000000000ull) >> 56)
          | ((x & 0x00ff000000000000ull) >> 40)
          | ((x & 0x0000ff0000000000ull) >> 24)
          | ((x & 0x000000ff00000000ull) >> 8)
          | ((x & 0x00000000ff000000ull) << 8)
          | ((x & 0x0000000000ff0000ull) << 24)
          | ((x & 0x000000000000ff00ull) << 40)
          | ((x & 0x00000000000000ffull) << 56));
}
#endif // HAVE_DECL_BSWAP64 == 0

#endif // defined(MAC_OSX)

#endif // BITCOIN_COMPAT_BYTESWAP_H
