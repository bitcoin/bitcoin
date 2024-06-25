// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>

#include <bit>

inline static void SipRound(uint64_t (&v)[4])
{
    v[0] += v[1]; v[1] = std::rotl(v[1], 13); v[1] ^= v[0];
    v[0] = std::rotl(v[0], 32);
    v[2] += v[3]; v[3] = std::rotl(v[3], 16); v[3] ^= v[2];
    v[0] += v[3]; v[3] = std::rotl(v[3], 21); v[3] ^= v[0];
    v[2] += v[1]; v[1] = std::rotl(v[1], 17); v[1] ^= v[2];
    v[2] = std::rotl(v[2], 32);
}

inline static void SipRound2(uint64_t (&v)[4])
{
    SipRound(v);
    SipRound(v);
}

inline static void ProcessBlock(uint64_t (&v)[4], uint64_t d)
{
    v[3] ^= d;
    SipRound2(v);
    v[0] ^= d;
}

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
    assert(count % 8 == 0);
    ProcessBlock(v, data);
    count += 8;
    return *this;
}

CSipHasher& CSipHasher::Write(Span<const unsigned char> data)
{
    uint64_t t = tmp;
    uint8_t c = count;

    while (data.size() > 0) {
        t |= uint64_t{data.front()} << (8 * (c % 8));
        c++;
        if ((c & 7) == 0) {
            ProcessBlock(v, t);
            t = 0;
        }
        data = data.subspan(1);
    }

    count = c;
    tmp = t;

    return *this;
}

uint64_t CSipHasher::Finalize() const
{
    uint64_t v_final[] = {v[0], v[1], v[2], v[3]};

    uint64_t t = tmp | (((uint64_t) count) << 56);

    ProcessBlock(v_final, t);
    v_final[2] ^= 0xFF;
    SipRound2(v_final);
    SipRound2(v_final);
    return v_final[0] ^ v_final[1] ^ v_final[2] ^ v_final[3];
}

/* Specialized implementation for efficiency */
uint64_t SipHashUint256(uint64_t k0, uint64_t k1, const uint256& val)
{
    uint64_t v[] = {
        0x736f6d6570736575ULL ^ k0, 0x646f72616e646f6dULL ^ k1,
        0x6c7967656e657261ULL ^ k0, 0x7465646279746573ULL ^ k1
    };

    uint64_t d = val.GetUint64(0);
    ProcessBlock(v, d);
    d = val.GetUint64(1);
    ProcessBlock(v, d);
    d = val.GetUint64(2);
    ProcessBlock(v, d);
    d = val.GetUint64(3);
    ProcessBlock(v, d);
    d = uint64_t{4} << 59;
    ProcessBlock(v, d);
    v[2] ^= 0xFF;
    SipRound2(v);
    SipRound2(v);
    return v[0] ^ v[1] ^ v[2] ^ v[3];
}

/* Specialized implementation for efficiency */
uint64_t SipHashUint256Extra(uint64_t k0, uint64_t k1, const uint256& val, uint32_t extra)
{
    uint64_t v[] = {
        0x736f6d6570736575ULL ^ k0, 0x646f72616e646f6dULL ^ k1,
        0x6c7967656e657261ULL ^ k0, 0x7465646279746573ULL ^ k1
    };

    uint64_t d = val.GetUint64(0);
    ProcessBlock(v, d);
    d = val.GetUint64(1);
    ProcessBlock(v, d);
    d = val.GetUint64(2);
    ProcessBlock(v, d);
    d = val.GetUint64(3);
    ProcessBlock(v, d);
    d = (uint64_t{36} << 56) | extra;
    ProcessBlock(v, d);
    v[2] ^= 0xFF;
    SipRound2(v);
    SipRound2(v);
    return v[0] ^ v[1] ^ v[2] ^ v[3];
}
