// Copyright (c) 2018-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>
#include <random.h>
#include <util/bytevectorhash.h>

#include <vector>

ByteVectorHash::ByteVectorHash() :
    m_k0(FastRandomContext().rand64()),
    m_k1(FastRandomContext().rand64())
{
}

size_t ByteVectorHash::operator()(const std::vector<unsigned char>& input) const
{
    return CSipHasher(m_k0, m_k1).Write(input).Finalize();
}
