// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>

#include <uint256.h>

#include <cassert>
#include <span>

CSipHasher::CSipHasher(uint64_t k0, uint64_t k1) : m_state{k0, k1} {}

CSipHasher& CSipHasher::Write(uint64_t data)
{
    assert(m_count % 8 == 0);
    m_state.Compress2(data);
    m_count += 8;
    return *this;
}

CSipHasher& CSipHasher::Write(std::span<const unsigned char> data)
{
    SipHashState state{m_state.Copy()};
    uint64_t t{m_tmp};
    uint8_t c{m_count};

    while (data.size() > 0) {
        t |= uint64_t{data.front()} << (8 * (c % 8));
        c++;
        if ((c & 7) == 0) {
            state.Compress2(t);
            t = 0;
        }
        data = data.subspan(1);
    }

    m_state = state;
    m_count = c;
    m_tmp = t;

    return *this;
}

uint64_t CSipHasher::Finalize() const
{
    return m_state.Copy()
                  .Compress2(m_tmp | (uint64_t{m_count} << 56))
                  .Finalize4();
}

SipHasher13UJ& SipHasher13UJ::Write(uint64_t data) noexcept
{
    m_state.Compress1(data);
    return *this;
}

SipHasher13UJ& SipHasher13UJ::WriteJumbo(const uint256& hash) noexcept
{
    m_state.Compress1Jumbo(hash);
    return *this;
}

uint64_t SipHasher13UJ::Finalize() const noexcept
{
    return m_state.Copy().Finalize3U();
}

uint64_t PresaltedSipHasher::operator()(const uint256& val) const noexcept
{
    return m_state.Copy()
                  .Compress2(val.GetUint64(0))
                  .Compress2(val.GetUint64(1))
                  .Compress2(val.GetUint64(2))
                  .Compress2(val.GetUint64(3))
                  .Compress2(uint64_t{32} << 56)
                  .Finalize4();
}

uint64_t PresaltedSipHasher::operator()(const uint256& val, uint32_t extra) const noexcept
{
    return m_state.Copy()
                  .Compress2(val.GetUint64(0))
                  .Compress2(val.GetUint64(1))
                  .Compress2(val.GetUint64(2))
                  .Compress2(val.GetUint64(3))
                  .Compress2((uint64_t{36} << 56) | extra)
                  .Finalize4();
}
