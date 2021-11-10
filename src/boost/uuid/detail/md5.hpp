/*
 * This RFC 1321 compatible MD5 implementation originated at:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
 *
 * Author:
 * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
 *
 * This software was written by Alexander Peslyak in 2001.  No copyright is
 * claimed, and the software is hereby placed in the public domain.
 * In case this attempt to disclaim copyright and place the software in the
 * public domain is deemed null and void, then the software is
 * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 */

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UUID_MD5_HPP
#define BOOST_UUID_MD5_HPP

#include <boost/cast.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/uuid/uuid.hpp> // for version
#include <boost/predef/other/endian.h>
#include <string.h>

namespace boost {
namespace uuids {
namespace detail {

class md5
{
public:
    typedef unsigned int(digest_type)[4];

    md5()
    {
        MD5_Init(&ctx_);
    }

    void process_byte(unsigned char byte)
    {
        MD5_Update(&ctx_, &byte, 1);
    }

    void process_bytes(void const* buffer, std::size_t byte_count)
    {
        MD5_Update(&ctx_, buffer, boost::numeric_cast<unsigned long>(byte_count));
    }

    void get_digest(digest_type& digest)
    {
        MD5_Final(reinterpret_cast<unsigned char *>(&digest[0]), &ctx_);
    }

    unsigned char get_version() const
    {
        // RFC 4122 Section 4.1.3
        return uuid::version_name_based_md5;
    }

private:

    /* Any 32-bit or wider unsigned integer data type will do */
    typedef uint32_t MD5_u32plus;

    typedef struct {
        MD5_u32plus lo, hi;
        MD5_u32plus a, b, c, d;
        unsigned char buffer[64];
        MD5_u32plus block[16];
    } MD5_CTX;

    /*
     * The basic MD5 functions.
     *
     * F and G are optimized compared to their RFC 1321 definitions for
     * architectures that lack an AND-NOT instruction, just like in Colin Plumb's
     * implementation.
     */
    BOOST_FORCEINLINE MD5_u32plus BOOST_UUID_DETAIL_MD5_F(MD5_u32plus x, MD5_u32plus y, MD5_u32plus z) { return ((z) ^ ((x) & ((y) ^ (z)))); }
    BOOST_FORCEINLINE MD5_u32plus BOOST_UUID_DETAIL_MD5_G(MD5_u32plus x, MD5_u32plus y, MD5_u32plus z) { return ((y) ^ ((z) & ((x) ^ (y)))); }
    BOOST_FORCEINLINE MD5_u32plus BOOST_UUID_DETAIL_MD5_H(MD5_u32plus x, MD5_u32plus y, MD5_u32plus z) { return (((x) ^ (y)) ^ (z)); }
    BOOST_FORCEINLINE MD5_u32plus BOOST_UUID_DETAIL_MD5_H2(MD5_u32plus x, MD5_u32plus y, MD5_u32plus z) { return ((x) ^ ((y) ^ (z))); }
    BOOST_FORCEINLINE MD5_u32plus BOOST_UUID_DETAIL_MD5_I(MD5_u32plus x, MD5_u32plus y, MD5_u32plus z) { return ((y) ^ ((x) | ~(z))); }

    /*
     * The MD5 transformation for all four rounds.
     */
    #define BOOST_UUID_DETAIL_MD5_STEP(f, a, b, c, d, x, t, s) \
        (a) += f((b), (c), (d)) + (x) + (t); \
        (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
        (a) += (b);

    /*
     * SET reads 4 input bytes in little-endian byte order and stores them in a
     * properly aligned word in host byte order.
     *
     * The check for little-endian architectures that tolerate unaligned memory
     * accesses is just an optimization.  Nothing will break if it fails to detect
     * a suitable architecture.
     *
     * Unfortunately, this optimization may be a C strict aliasing rules violation
     * if the caller's data buffer has effective type that cannot be aliased by
     * MD5_u32plus.  In practice, this problem may occur if these MD5 routines are
     * inlined into a calling function, or with future and dangerously advanced
     * link-time optimizations.  For the time being, keeping these MD5 routines in
     * their own translation unit avoids the problem.
     */
    #if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
    #define BOOST_UUID_DETAIL_MD5_SET(n) \
        (*(MD5_u32plus *)&ptr[(n) * 4])
    #define BOOST_UUID_DETAIL_MD5_GET(n) \
        BOOST_UUID_DETAIL_MD5_SET(n)
    #else
    #define BOOST_UUID_DETAIL_MD5_SET(n) \
        (ctx->block[(n)] = \
        (MD5_u32plus)ptr[(n) * 4] | \
        ((MD5_u32plus)ptr[(n) * 4 + 1] << 8) | \
        ((MD5_u32plus)ptr[(n) * 4 + 2] << 16) | \
        ((MD5_u32plus)ptr[(n) * 4 + 3] << 24))
    #define BOOST_UUID_DETAIL_MD5_GET(n) \
        (ctx->block[(n)])
    #endif

    /*
     * This processes one or more 64-byte data blocks, but does NOT update the bit
     * counters.  There are no alignment requirements.
     */
    const void *body(MD5_CTX *ctx, const void *data, unsigned long size)
    {
        const unsigned char *ptr;
        MD5_u32plus a, b, c, d;
        MD5_u32plus saved_a, saved_b, saved_c, saved_d;

        ptr = (const unsigned char *)data;

        a = ctx->a;
        b = ctx->b;
        c = ctx->c;
        d = ctx->d;

        do {
            saved_a = a;
            saved_b = b;
            saved_c = c;
            saved_d = d;

    /* Round 1 */
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, a, b, c, d, BOOST_UUID_DETAIL_MD5_SET(0), 0xd76aa478, 7)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, d, a, b, c, BOOST_UUID_DETAIL_MD5_SET(1), 0xe8c7b756, 12)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, c, d, a, b, BOOST_UUID_DETAIL_MD5_SET(2), 0x242070db, 17)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, b, c, d, a, BOOST_UUID_DETAIL_MD5_SET(3), 0xc1bdceee, 22)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, a, b, c, d, BOOST_UUID_DETAIL_MD5_SET(4), 0xf57c0faf, 7)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, d, a, b, c, BOOST_UUID_DETAIL_MD5_SET(5), 0x4787c62a, 12)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, c, d, a, b, BOOST_UUID_DETAIL_MD5_SET(6), 0xa8304613, 17)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, b, c, d, a, BOOST_UUID_DETAIL_MD5_SET(7), 0xfd469501, 22)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, a, b, c, d, BOOST_UUID_DETAIL_MD5_SET(8), 0x698098d8, 7)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, d, a, b, c, BOOST_UUID_DETAIL_MD5_SET(9), 0x8b44f7af, 12)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, c, d, a, b, BOOST_UUID_DETAIL_MD5_SET(10), 0xffff5bb1, 17)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, b, c, d, a, BOOST_UUID_DETAIL_MD5_SET(11), 0x895cd7be, 22)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, a, b, c, d, BOOST_UUID_DETAIL_MD5_SET(12), 0x6b901122, 7)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, d, a, b, c, BOOST_UUID_DETAIL_MD5_SET(13), 0xfd987193, 12)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, c, d, a, b, BOOST_UUID_DETAIL_MD5_SET(14), 0xa679438e, 17)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_F, b, c, d, a, BOOST_UUID_DETAIL_MD5_SET(15), 0x49b40821, 22)

    /* Round 2 */
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(1), 0xf61e2562, 5)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(6), 0xc040b340, 9)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(11), 0x265e5a51, 14)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(0), 0xe9b6c7aa, 20)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(5), 0xd62f105d, 5)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(10), 0x02441453, 9)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(15), 0xd8a1e681, 14)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(4), 0xe7d3fbc8, 20)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(9), 0x21e1cde6, 5)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(14), 0xc33707d6, 9)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(3), 0xf4d50d87, 14)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(8), 0x455a14ed, 20)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(13), 0xa9e3e905, 5)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(2), 0xfcefa3f8, 9)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(7), 0x676f02d9, 14)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_G, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(12), 0x8d2a4c8a, 20)

    /* Round 3 */
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(5), 0xfffa3942, 4)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H2, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(8), 0x8771f681, 11)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(11), 0x6d9d6122, 16)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H2, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(14), 0xfde5380c, 23)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(1), 0xa4beea44, 4)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H2, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(4), 0x4bdecfa9, 11)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(7), 0xf6bb4b60, 16)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H2, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(10), 0xbebfbc70, 23)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(13), 0x289b7ec6, 4)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H2, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(0), 0xeaa127fa, 11)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(3), 0xd4ef3085, 16)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H2, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(6), 0x04881d05, 23)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(9), 0xd9d4d039, 4)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H2, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(12), 0xe6db99e5, 11)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(15), 0x1fa27cf8, 16)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_H2, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(2), 0xc4ac5665, 23)

    /* Round 4 */
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(0), 0xf4292244, 6)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(7), 0x432aff97, 10)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(14), 0xab9423a7, 15)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(5), 0xfc93a039, 21)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(12), 0x655b59c3, 6)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(3), 0x8f0ccc92, 10)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(10), 0xffeff47d, 15)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(1), 0x85845dd1, 21)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(8), 0x6fa87e4f, 6)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(15), 0xfe2ce6e0, 10)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(6), 0xa3014314, 15)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(13), 0x4e0811a1, 21)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, a, b, c, d, BOOST_UUID_DETAIL_MD5_GET(4), 0xf7537e82, 6)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, d, a, b, c, BOOST_UUID_DETAIL_MD5_GET(11), 0xbd3af235, 10)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, c, d, a, b, BOOST_UUID_DETAIL_MD5_GET(2), 0x2ad7d2bb, 15)
            BOOST_UUID_DETAIL_MD5_STEP(BOOST_UUID_DETAIL_MD5_I, b, c, d, a, BOOST_UUID_DETAIL_MD5_GET(9), 0xeb86d391, 21)

            a += saved_a;
            b += saved_b;
            c += saved_c;
            d += saved_d;

            ptr += 64;
        } while (size -= 64);

        ctx->a = a;
        ctx->b = b;
        ctx->c = c;
        ctx->d = d;

        return ptr;
    }

    void MD5_Init(MD5_CTX *ctx)
    {
        ctx->a = 0x67452301;
        ctx->b = 0xefcdab89;
        ctx->c = 0x98badcfe;
        ctx->d = 0x10325476;

        ctx->lo = 0;
        ctx->hi = 0;
    }

    void MD5_Update(MD5_CTX *ctx, const void *data, unsigned long size)
    {
        MD5_u32plus saved_lo;
        unsigned long used, available;

        saved_lo = ctx->lo;
        if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo)
            ctx->hi++;
        ctx->hi += size >> 29;

        used = saved_lo & 0x3f;

        if (used) {
            available = 64 - used;

            if (size < available) {
                memcpy(&ctx->buffer[used], data, size);
                return;
            }

            memcpy(&ctx->buffer[used], data, available);
            data = (const unsigned char *)data + available;
            size -= available;
            body(ctx, ctx->buffer, 64);
        }

        if (size >= 64) {
            data = body(ctx, data, size & ~(unsigned long)0x3f);
            size &= 0x3f;
        }

        memcpy(ctx->buffer, data, size);
    }

    // This must remain consistent no matter the endianness
    #define BOOST_UUID_DETAIL_MD5_OUT(dst, src) \
        (dst)[0] = (unsigned char)(src); \
        (dst)[1] = (unsigned char)((src) >> 8); \
        (dst)[2] = (unsigned char)((src) >> 16); \
        (dst)[3] = (unsigned char)((src) >> 24);

    //
    // A big-endian issue with MD5 results was resolved
    // in boost 1.71.  If you generated md5 name-based uuids
    // with boost 1.66 through 1.70 and stored them, then
    // set the following compatibility flag to ensure that
    // your hash generation remains consistent.
    //
#if defined(BOOST_UUID_COMPAT_PRE_1_71_MD5)
    #define BOOST_UUID_DETAIL_MD5_BYTE_OUT(dst, src) \
        BOOST_UUID_DETAIL_MD5_OUT(dst, src)
#else
    //
    // We're copying into a byte buffer which is actually
    // backed by an unsigned int array, which later on
    // is then swabbed one more time by the basic name
    // generator.  Therefore the logic here is reversed.
    // This was done to minimize the impact to existing
    // name-based hash generation.  The correct fix would
    // be to make this and name generation endian-correct
    // but that would even break previously generated sha1
    // hashes too.
    //
#if BOOST_ENDIAN_LITTLE_BYTE
    #define BOOST_UUID_DETAIL_MD5_BYTE_OUT(dst, src) \
        (dst)[0] = (unsigned char)((src) >> 24); \
        (dst)[1] = (unsigned char)((src) >> 16); \
        (dst)[2] = (unsigned char)((src) >> 8); \
        (dst)[3] = (unsigned char)(src);
#else
    #define BOOST_UUID_DETAIL_MD5_BYTE_OUT(dst, src) \
        (dst)[0] = (unsigned char)(src); \
        (dst)[1] = (unsigned char)((src) >> 8); \
        (dst)[2] = (unsigned char)((src) >> 16); \
        (dst)[3] = (unsigned char)((src) >> 24);
#endif
#endif // BOOST_UUID_COMPAT_PRE_1_71_MD5

    void MD5_Final(unsigned char *result, MD5_CTX *ctx)
    {
        unsigned long used, available;

        used = ctx->lo & 0x3f;

        ctx->buffer[used++] = 0x80;

        available = 64 - used;

        if (available < 8) {
            memset(&ctx->buffer[used], 0, available);
            body(ctx, ctx->buffer, 64);
            used = 0;
            available = 64;
        }

        memset(&ctx->buffer[used], 0, available - 8);

        ctx->lo <<= 3;
        BOOST_UUID_DETAIL_MD5_OUT(&ctx->buffer[56], ctx->lo)
        BOOST_UUID_DETAIL_MD5_OUT(&ctx->buffer[60], ctx->hi)

        body(ctx, ctx->buffer, 64);

        BOOST_UUID_DETAIL_MD5_BYTE_OUT(&result[0], ctx->a)
        BOOST_UUID_DETAIL_MD5_BYTE_OUT(&result[4], ctx->b)
        BOOST_UUID_DETAIL_MD5_BYTE_OUT(&result[8], ctx->c)
        BOOST_UUID_DETAIL_MD5_BYTE_OUT(&result[12], ctx->d)

        memset(ctx, 0, sizeof(*ctx));
    }

#undef BOOST_UUID_DETAIL_MD5_OUT
#undef BOOST_UUID_DETAIL_MD5_SET
#undef BOOST_UUID_DETAIL_MD5_GET
#undef BOOST_UUID_DETAIL_MD5_STEP

    MD5_CTX ctx_;
};


} // detail
} // uuids
} // boost

#endif // BOOST_UUID_MD5_HPP
