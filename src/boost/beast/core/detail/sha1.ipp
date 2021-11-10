//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_SHA1_IPP
#define BOOST_BEAST_DETAIL_SHA1_IPP

#include <boost/beast/core/detail/sha1.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>

// Based on https://github.com/vog/sha1
/*
    Original authors:
        Steve Reid (Original C Code)
        Bruce Guenter (Small changes to fit into bglibs)
        Volker Grabsch (Translation to simpler C++ Code)
        Eugene Hopkinson (Safety improvements)
        Vincent Falco (beast adaptation)
*/

namespace boost {
namespace beast {
namespace detail {

namespace sha1 {

inline
std::uint32_t
rol(std::uint32_t value, std::size_t bits)
{
    return (value << bits) | (value >> (32 - bits));
}

inline
std::uint32_t
blk(std::uint32_t block[BLOCK_INTS], std::size_t i)
{
    return rol(
        block[(i+13)&15] ^ block[(i+8)&15] ^
        block[(i+2)&15]  ^ block[i], 1);
}

inline
void
R0(std::uint32_t block[BLOCK_INTS], std::uint32_t v,
    std::uint32_t &w, std::uint32_t x, std::uint32_t y,
        std::uint32_t &z, std::size_t i)
{
    z += ((w&(x^y))^y) + block[i] + 0x5a827999 + rol(v, 5);
    w = rol(w, 30);
}


inline
void
R1(std::uint32_t block[BLOCK_INTS], std::uint32_t v,
    std::uint32_t &w, std::uint32_t x, std::uint32_t y,
        std::uint32_t &z, std::size_t i)
{
    block[i] = blk(block, i);
    z += ((w&(x^y))^y) + block[i] + 0x5a827999 + rol(v, 5);
    w = rol(w, 30);
}

inline
void
R2(std::uint32_t block[BLOCK_INTS], std::uint32_t v,
    std::uint32_t &w, std::uint32_t x, std::uint32_t y,
        std::uint32_t &z, std::size_t i)
{
    block[i] = blk(block, i);
    z += (w^x^y) + block[i] + 0x6ed9eba1 + rol(v, 5);
    w = rol(w, 30);
}

inline
void
R3(std::uint32_t block[BLOCK_INTS], std::uint32_t v,
    std::uint32_t &w, std::uint32_t x, std::uint32_t y,
        std::uint32_t &z, std::size_t i)
{
    block[i] = blk(block, i);
    z += (((w|x)&y)|(w&x)) + block[i] + 0x8f1bbcdc + rol(v, 5);
    w = rol(w, 30);
}

inline
void
R4(std::uint32_t block[BLOCK_INTS], std::uint32_t v,
    std::uint32_t &w, std::uint32_t x, std::uint32_t y,
        std::uint32_t &z, std::size_t i)
{
    block[i] = blk(block, i);
    z += (w^x^y) + block[i] + 0xca62c1d6 + rol(v, 5);
    w = rol(w, 30);
}

inline
void
make_block(std::uint8_t const* p,
    std::uint32_t block[BLOCK_INTS])
{
    for(std::size_t i = 0; i < BLOCK_INTS; i++)
        block[i] =
            (static_cast<std::uint32_t>(p[4*i+3]))     |
            (static_cast<std::uint32_t>(p[4*i+2]))<< 8 |
            (static_cast<std::uint32_t>(p[4*i+1]))<<16 |
            (static_cast<std::uint32_t>(p[4*i+0]))<<24;
}

inline
void
transform(
    std::uint32_t digest[], std::uint32_t block[BLOCK_INTS])
{
    std::uint32_t a = digest[0];
    std::uint32_t b = digest[1];
    std::uint32_t c = digest[2];
    std::uint32_t d = digest[3];
    std::uint32_t e = digest[4];

    R0(block, a, b, c, d, e,  0);
    R0(block, e, a, b, c, d,  1);
    R0(block, d, e, a, b, c,  2);
    R0(block, c, d, e, a, b,  3);
    R0(block, b, c, d, e, a,  4);
    R0(block, a, b, c, d, e,  5);
    R0(block, e, a, b, c, d,  6);
    R0(block, d, e, a, b, c,  7);
    R0(block, c, d, e, a, b,  8);
    R0(block, b, c, d, e, a,  9);
    R0(block, a, b, c, d, e, 10);
    R0(block, e, a, b, c, d, 11);
    R0(block, d, e, a, b, c, 12);
    R0(block, c, d, e, a, b, 13);
    R0(block, b, c, d, e, a, 14);
    R0(block, a, b, c, d, e, 15);
    R1(block, e, a, b, c, d,  0);
    R1(block, d, e, a, b, c,  1);
    R1(block, c, d, e, a, b,  2);
    R1(block, b, c, d, e, a,  3);
    R2(block, a, b, c, d, e,  4);
    R2(block, e, a, b, c, d,  5);
    R2(block, d, e, a, b, c,  6);
    R2(block, c, d, e, a, b,  7);
    R2(block, b, c, d, e, a,  8);
    R2(block, a, b, c, d, e,  9);
    R2(block, e, a, b, c, d, 10);
    R2(block, d, e, a, b, c, 11);
    R2(block, c, d, e, a, b, 12);
    R2(block, b, c, d, e, a, 13);
    R2(block, a, b, c, d, e, 14);
    R2(block, e, a, b, c, d, 15);
    R2(block, d, e, a, b, c,  0);
    R2(block, c, d, e, a, b,  1);
    R2(block, b, c, d, e, a,  2);
    R2(block, a, b, c, d, e,  3);
    R2(block, e, a, b, c, d,  4);
    R2(block, d, e, a, b, c,  5);
    R2(block, c, d, e, a, b,  6);
    R2(block, b, c, d, e, a,  7);
    R3(block, a, b, c, d, e,  8);
    R3(block, e, a, b, c, d,  9);
    R3(block, d, e, a, b, c, 10);
    R3(block, c, d, e, a, b, 11);
    R3(block, b, c, d, e, a, 12);
    R3(block, a, b, c, d, e, 13);
    R3(block, e, a, b, c, d, 14);
    R3(block, d, e, a, b, c, 15);
    R3(block, c, d, e, a, b,  0);
    R3(block, b, c, d, e, a,  1);
    R3(block, a, b, c, d, e,  2);
    R3(block, e, a, b, c, d,  3);
    R3(block, d, e, a, b, c,  4);
    R3(block, c, d, e, a, b,  5);
    R3(block, b, c, d, e, a,  6);
    R3(block, a, b, c, d, e,  7);
    R3(block, e, a, b, c, d,  8);
    R3(block, d, e, a, b, c,  9);
    R3(block, c, d, e, a, b, 10);
    R3(block, b, c, d, e, a, 11);
    R4(block, a, b, c, d, e, 12);
    R4(block, e, a, b, c, d, 13);
    R4(block, d, e, a, b, c, 14);
    R4(block, c, d, e, a, b, 15);
    R4(block, b, c, d, e, a,  0);
    R4(block, a, b, c, d, e,  1);
    R4(block, e, a, b, c, d,  2);
    R4(block, d, e, a, b, c,  3);
    R4(block, c, d, e, a, b,  4);
    R4(block, b, c, d, e, a,  5);
    R4(block, a, b, c, d, e,  6);
    R4(block, e, a, b, c, d,  7);
    R4(block, d, e, a, b, c,  8);
    R4(block, c, d, e, a, b,  9);
    R4(block, b, c, d, e, a, 10);
    R4(block, a, b, c, d, e, 11);
    R4(block, e, a, b, c, d, 12);
    R4(block, d, e, a, b, c, 13);
    R4(block, c, d, e, a, b, 14);
    R4(block, b, c, d, e, a, 15);

    digest[0] += a;
    digest[1] += b;
    digest[2] += c;
    digest[3] += d;
    digest[4] += e;
}

} // sha1

void
init(sha1_context& ctx) noexcept
{
    ctx.buflen = 0;
    ctx.blocks = 0;
    ctx.digest[0] = 0x67452301;
    ctx.digest[1] = 0xefcdab89;
    ctx.digest[2] = 0x98badcfe;
    ctx.digest[3] = 0x10325476;
    ctx.digest[4] = 0xc3d2e1f0;
}

void
update(
    sha1_context& ctx,
    void const* message,
    std::size_t size) noexcept
{
    auto p = static_cast<
        std::uint8_t const*>(message);
    for(;;)
    {
        auto const n = (std::min)(
            size, sizeof(ctx.buf) - ctx.buflen);
        std::memcpy(ctx.buf + ctx.buflen, p, n);
        ctx.buflen += n;
        if(ctx.buflen != 64)
            return;
        p += n;
        size -= n;
        ctx.buflen = 0;
        std::uint32_t block[sha1::BLOCK_INTS];
        sha1::make_block(ctx.buf, block);
        sha1::transform(ctx.digest, block);
        ++ctx.blocks;
    }
}

void
finish(
    sha1_context& ctx,
    void* digest) noexcept
{
    using sha1::BLOCK_INTS;
    using sha1::BLOCK_BYTES;

    std::uint64_t total_bits =
        (ctx.blocks*64 + ctx.buflen) * 8;
    // pad
    ctx.buf[ctx.buflen++] = 0x80;
    auto const buflen = ctx.buflen;
    while(ctx.buflen < 64)
        ctx.buf[ctx.buflen++] = 0x00;
    std::uint32_t block[BLOCK_INTS];
    sha1::make_block(ctx.buf, block);
    if(buflen > BLOCK_BYTES - 8)
    {
        sha1::transform(ctx.digest, block);
        for(size_t i = 0; i < BLOCK_INTS - 2; i++)
            block[i] = 0;
    }

    /* Append total_bits, split this uint64_t into two uint32_t */
    block[BLOCK_INTS - 1] = total_bits & 0xffffffff;
    block[BLOCK_INTS - 2] = (total_bits >> 32);
    sha1::transform(ctx.digest, block);
    for(std::size_t i = 0; i < sha1::DIGEST_BYTES/4; i++)
    {
        std::uint8_t* d =
            static_cast<std::uint8_t*>(digest) + 4 * i;
        d[3] =  ctx.digest[i]        & 0xff;
        d[2] = (ctx.digest[i] >>  8) & 0xff;
        d[1] = (ctx.digest[i] >> 16) & 0xff;
        d[0] = (ctx.digest[i] >> 24) & 0xff;
    }
}

} // detail
} // beast
} // boost

#endif
