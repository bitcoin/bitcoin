// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>

#include <bit>

constexpr uint64_t SIPHASH_CONST_0 = 0x736f6d6570736575ULL;
constexpr uint64_t SIPHASH_CONST_1 = 0x646f72616e646f6dULL;
constexpr uint64_t SIPHASH_CONST_2 = 0x6c7967656e657261ULL;
constexpr uint64_t SIPHASH_CONST_3 = 0x7465646279746573ULL;

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
    v[0] = SIPHASH_CONST_0 ^ k0;
    v[1] = SIPHASH_CONST_1 ^ k1;
    v[2] = SIPHASH_CONST_2 ^ k0;
    v[3] = SIPHASH_CONST_3 ^ k1;
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

    for (unsigned char byte : data) {
        t |= uint64_t{byte} << (8 * (c % 8));
        c++;
        if ((c & 7) == 0) {
            v3 ^= t;
            SIPROUND;
            SIPROUND;
            v0 ^= t;
            t = 0;
        }
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

static uint64_t SipHashCommon(uint64_t k0, uint64_t k1, const uint256& val, uint64_t extra_data, bool is_extra)
{
    uint64_t v0 = SIPHASH_CONST_0 ^ k0;
    uint64_t v1 = SIPHASH_CONST_1 ^ k1;
    uint64_t v2 = SIPHASH_CONST_2 ^ k0;
    uint64_t v3 = SIPHASH_CONST_3 ^ k1 ^ val.GetUint64(0);

    SIPROUND;
    SIPROUND;
    v0 ^= val.GetUint64(0);
    v3 ^= val.GetUint64(1);
    SIPROUND;
    SIPROUND;
    v0 ^= val.GetUint64(1);
    v3 ^= val.GetUint64(2);
    SIPROUND;
    SIPROUND;
    v0 ^= val.GetUint64(2);
    v3 ^= val.GetUint64(3);
    SIPROUND;
    SIPROUND;
    v0 ^= val.GetUint64(3);

    v3 ^= is_extra ? (extra_data | ((uint64_t{36}) << 56)) : (uint64_t{4} << 59);
    SIPROUND;
    SIPROUND;
    v0 ^= is_extra ? (extra_data | ((uint64_t{36}) << 56)) : (uint64_t{4} << 59);

    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}

uint64_t SipHashUint256(uint64_t k0, uint64_t k1, const uint256& val)
{
    return SipHashCommon(k0, k1, val, 0, false);
}

uint64_t SipHashUint256Extra(uint64_t k0, uint64_t k1, const uint256& val, uint32_t extra)
{
    return SipHashCommon(k0, k1, val, extra, true);
}
