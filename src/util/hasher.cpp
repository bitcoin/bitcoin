// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <util/hasher.h>

#include <limits>

SaltedTxidHasher::SaltedTxidHasher() : k0(GetRand(std::numeric_limits<uint64_t>::max())), k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

SaltedOutpointHasher::SaltedOutpointHasher() : k0(GetRand(std::numeric_limits<uint64_t>::max())), k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

SaltedSipHasher::SaltedSipHasher() : m_k0(GetRand(std::numeric_limits<uint64_t>::max())), m_k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

size_t SaltedSipHasher::operator()(const Span<const unsigned char>& script) const
{
    return CSipHasher(m_k0, m_k1).Write(script.data(), script.size()).Finalize();
}
