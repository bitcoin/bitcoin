// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/MIT.

#include <crypto/siphash.h>
#include <random.h>
#include <util/bytevectorhash.h>

#include <vector>

ByteVectorHash::ByteVectorHash() :
    m_k0(GetRand<uint64_t>()),
    m_k1(GetRand<uint64_t>())
{
}

size_t ByteVectorHash::operator()(const std::vector<unsigned char>& input) const
{
    return CSipHasher(m_k0, m_k1).Write(input.data(), input.size()).Finalize();
}
