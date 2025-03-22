// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>

#include <bit>

#define SIPROUND do { \
    v0 += v1; v1 = std::rotl(v1, 13); v1 ^= v0; \
    v0 = std::rotl(v0, 32); \
    v2 += v3; v3 = std::rotl(v3, 16); v3 ^= v2; \
    v0 += v3; v3 = std::rotl(v3, 21); v3 ^= v0; \
    v2 += v1; v1 = std::rotl(v1, 17); v1 ^= v2; \
    v2 = std::rotl(v2, 32); \
} while (0)

CSipHasher::CSipHasher(uint64_t k0, uint64_t k1)
{
    v[0] = 0x736f6d6570736575ULL ^ k0;
    v[1] = 0x646f72616e646f6dULL ^ k1;
    v[2] = 0x6c7967656e657261ULL ^ k0;
    v[3] = 0x7465646279746573ULL ^ k1;
    count = 0;
    tmp = 0;
}

CSipHasher& CSipHasher::Write(uint64_t data)
{
    uint64_t v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];

    assert(count % 8 == 0);

    v3 ^= data;
    SIPROUND;
    SIPROUND;
    v0 ^= data;

    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;

    count += 8;
    return *this;
}

CSipHasher& CSipHasher::Write(Span<const unsigned char> data)
{
    uint64_t v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];
    uint64_t t = tmp;
    uint8_t c = count;

    while (data.size() > 0) {
        t |= uint64_t{data.front()} << (8 * (c % 8));
        c++;
        if ((c & 7) == 0) {
            v3 ^= t;
            SIPROUND;
            SIPROUND;
            v0 ^= t;
            t = 0;
        }
        data = data.subspan(1);
    }

    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
    count = c;
    tmp = t;

    return *this;
}

uint64_t CSipHasher::Finalize() const
{
    uint64_t v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];

    uint64_t t = tmp | (((uint64_t)count) << 56);

    v3 ^= t;
    SIPROUND;
    SIPROUND;
    v0 ^= t;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}

uint64_t SipHashUint256(uint64_t k0, uint64_t k1, const uint256& val)
{
    /* Specialized implementation for efficiency */
    uint64_t d = val.GetUint64(0);

    uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
    uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
    uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
    uint64_t v3 = 0x7465646279746573ULL ^ k1 ^ d;

    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(1);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(2);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(3);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    v3 ^= (uint64_t{4}) << 59;
    SIPROUND;
    SIPROUND;
    v0 ^= (uint64_t{4}) << 59;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}

uint64_t SipHashUint256Extra(uint64_t k0, uint64_t k1, const uint256& val, uint32_t extra)
{
    /* Specialized implementation for efficiency */
    uint64_t d = val.GetUint64(0);

    uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
    uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
    uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
    uint64_t v3 = 0x7465646279746573ULL ^ k1 ^ d;

    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(1);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(2);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(3);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = ((uint64_t{36}) << 56) | extra;
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}


uint64_t ModifiedRapidHash(uint64_t k0, uint64_t k1, const uint256& val, uint32_t extra)
{
    auto const rapid_mum = [](uint64_t* a, uint64_t* b) {
#if defined(__SIZEOF_INT128__)
        __uint128_t r = *a;
        r *= *b;
        *a = (uint64_t)r;
        *b = (uint64_t)(r >> 64);
#elif defined(_MSC_VER) && (defined(_WIN64) || defined(_M_HYBRID_CHPE_ARM64))
#if defined(_M_X64)
        *a = _umul128(*a, *b, b);
#else
        uint64_t c = __umulh(*a, *b);
        *a = *a * *b;
        *b = c;
#endif
#else
        uint64_t ha = *a >> 32, hb = *b >> 32, la = (uint32_t)*a, lb = (uint32_t)*b, hi, lo;
        uint64_t rh = ha * hb, rm0 = ha * lb, rm1 = hb * la, rl = la * lb, t = rl + (rm0 << 32), c = t < rl;
        lo = t + (rm1 << 32);
        c += lo < t;
        hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
        *a = lo;
        *b = hi;
#endif
    };

    auto const rapid_mix = [&rapid_mum](uint64_t a, uint64_t b) -> uint64_t {
        rapid_mum(&a, &b);
        return a ^ b;
    };

    // This effectifely behaves like rapidhash with that input:
    // seed: k0, data: [val, k1, extra]
    // So it hashes 32+8+4 = 44 bytes, plus uses the 8 byte as seed.

    // Default secret parameters.
    static constexpr uint64_t secret[3] = {0x2d358dccaa6c78a5ull, 0x8bb84b93962eacc9ull, 0x4b33a62ed433d4a3ull};

    // no need to mix the seed itself, as it is purely random.
    uint64_t seed = k0;
    seed = rapid_mix(val.GetUint64(0) ^ secret[2], val.GetUint64(1) ^ seed ^ secret[1]);
    seed = rapid_mix(val.GetUint64(2) ^ secret[2], val.GetUint64(3) ^ seed);
    uint64_t a = k1 ^ secret[1];
    uint64_t b = extra ^ seed;
    rapid_mum(&a, &b);
    return rapid_mix(a ^ secret[0], b ^ secret[1]);
}
