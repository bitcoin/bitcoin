// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>
#include <random.h>
#include <util/bytevectorhash.h>

ByteVectorHash::ByteVectorHash()
{
    GetRandBytes(reinterpret_cast<unsigned char*>(&m_k0), sizeof(m_k0));
    GetRandBytes(reinterpret_cast<unsigned char*>(&m_k1), sizeof(m_k1));
}

size_t ByteVectorHash::operator()(const std::vector<unsigned char>& input) const
{
    return CSipHasher(m_k0, m_k1).Write(input.data(), input.size()).Finalize();
}
