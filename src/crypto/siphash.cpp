// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>

#include <uint256.h>

#include <bit>
#include <cassert>
#include <span>

#define SIPROUND do { \
    v0 += v1; v1 = std::rotl(v1, 13); v1 ^= v0; \
    v0 = std::rotl(v0, 32); \
    v2 += v3; v3 = std::rotl(v3, 16); v3 ^= v2; \
    v0 += v3; v3 = std::rotl(v3, 21); v3 ^= v0; \
    v2 += v1; v1 = std::rotl(v1, 17); v1 ^= v2; \
    v2 = std::rotl(v2, 32); \
} while (0)

CSipHasher::CSipHasher(uint64_t k0, uint64_t k1) : m_state{k0, k1} {}

CSipHasher& CSipHasher::Write(uint64_t data)
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];

    assert(m_count % 8 == 0);

    v3 ^= data;
    SIPROUND;
    SIPROUND;
    v0 ^= data;

    m_state.v[0] = v0;
    m_state.v[1] = v1;
    m_state.v[2] = v2;
    m_state.v[3] = v3;

    m_count += 8;
    return *this;
}

CSipHasher& CSipHasher::Write(std::span<const unsigned char> data)
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t t = m_tmp;
    uint8_t c = m_count;

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

    m_state.v[0] = v0;
    m_state.v[1] = v1;
    m_state.v[2] = v2;
    m_state.v[3] = v3;
    m_count = c;
    m_tmp = t;

    return *this;
}

uint64_t CSipHasher::Finalize() const
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];

    uint64_t t = m_tmp | (((uint64_t)m_count) << 56);

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

uint64_t PresaltedSipHasher::operator()(const uint256& val) const noexcept
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t d = val.GetUint64(0);
    v3 ^= d;

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

/** Specialized implementation for efficiency */
uint64_t PresaltedSipHasher::operator()(const uint256& val, uint32_t extra) const noexcept
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t d = val.GetUint64(0);
    v3 ^= d;
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
