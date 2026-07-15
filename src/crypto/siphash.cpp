// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>

#include <uint256.h>

#include <cassert>
#include <span>

using siphash_detail::Finalize24;
using siphash_detail::ProcessNormal24;

CSipHasher::CSipHasher(uint64_t k0, uint64_t k1) : m_state{k0, k1} {}

CSipHasher& CSipHasher::Write(uint64_t data)
{
    uint64_t v0{m_state.v[0]}, v1{m_state.v[1]}, v2{m_state.v[2]}, v3{m_state.v[3]};

    assert(m_count % 8 == 0);

    ProcessNormal24(v0, v1, v2, v3, data);

    m_state.v[0] = v0; m_state.v[1] = v1; m_state.v[2] = v2; m_state.v[3] = v3;

    m_count += 8;
    return *this;
}

CSipHasher& CSipHasher::Write(std::span<const unsigned char> data)
{
    uint64_t v0{m_state.v[0]}, v1{m_state.v[1]}, v2{m_state.v[2]}, v3{m_state.v[3]};
    uint64_t t{m_tmp};
    uint8_t c{m_count};

    while (data.size() > 0) {
        t |= uint64_t{data.front()} << (8 * (c % 8));
        c++;
        if ((c & 7) == 0) {
            ProcessNormal24(v0, v1, v2, v3, t);
            t = 0;
        }
        data = data.subspan(1);
    }

    m_state.v[0] = v0; m_state.v[1] = v1; m_state.v[2] = v2; m_state.v[3] = v3;
    m_count = c;
    m_tmp = t;

    return *this;
}

uint64_t CSipHasher::Finalize() const
{
    return Finalize24(m_state.v[0], m_state.v[1], m_state.v[2], m_state.v[3], m_tmp | (uint64_t{m_count} << 56));
}

uint64_t PresaltedSipHasher::operator()(const uint256& val) const noexcept
{
    uint64_t v0{m_state.v[0]}, v1{m_state.v[1]}, v2{m_state.v[2]}, v3{m_state.v[3]};
    ProcessNormal24(v0, v1, v2, v3, val.GetUint64(0));
    ProcessNormal24(v0, v1, v2, v3, val.GetUint64(1));
    ProcessNormal24(v0, v1, v2, v3, val.GetUint64(2));
    ProcessNormal24(v0, v1, v2, v3, val.GetUint64(3));
    return Finalize24(v0, v1, v2, v3, uint64_t{4} << 59);
}

/** Specialized implementation for efficiency */
uint64_t PresaltedSipHasher::operator()(const uint256& val, uint32_t extra) const noexcept
{
    uint64_t v0{m_state.v[0]}, v1{m_state.v[1]}, v2{m_state.v[2]}, v3{m_state.v[3]};
    ProcessNormal24(v0, v1, v2, v3, val.GetUint64(0));
    ProcessNormal24(v0, v1, v2, v3, val.GetUint64(1));
    ProcessNormal24(v0, v1, v2, v3, val.GetUint64(2));
    ProcessNormal24(v0, v1, v2, v3, val.GetUint64(3));
    return Finalize24(v0, v1, v2, v3, (uint64_t{36} << 56) | extra);
}
