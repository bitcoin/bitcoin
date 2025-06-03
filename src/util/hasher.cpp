// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>
#include <random.h>
#include <span.h>
#include <util/hasher.h>

#include <limits>

SaltedTxidHasher::SaltedTxidHasher() : k0(GetRand(std::numeric_limits<uint64_t>::max())), k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

SaltedOutpointHasher::SaltedOutpointHasher(bool deterministic) :
    k0(deterministic ? 0x8e819f2607a18de6 : GetRand(std::numeric_limits<uint64_t>::max())),
    k1(deterministic ? 0xf4020d2e3983b0eb : GetRand(std::numeric_limits<uint64_t>::max()))
{}

SaltedSipHasher::SaltedSipHasher() : m_k0(GetRand(std::numeric_limits<uint64_t>::max())), m_k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

size_t SaltedSipHasher::operator()(const Span<const unsigned char>& script) const
{
    return CSipHasher(m_k0, m_k1).Write(script.data(), script.size()).Finalize();
}
