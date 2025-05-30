// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>
#include <random.h>
#include <span.h>
#include <util/hasher.h>

SaltedTxidHasher::SaltedTxidHasher() :
    k0{FastRandomContext().rand64()},
    k1{FastRandomContext().rand64()} {}

SaltedOutpointHasher::SaltedOutpointHasher(bool deterministic) :
    k0{deterministic ? 0x8e819f2607a18de6 : FastRandomContext().rand64()},
    k1{deterministic ? 0xf4020d2e3983b0eb : FastRandomContext().rand64()}
{}

SaltedSipHasher::SaltedSipHasher() :
    m_k0{FastRandomContext().rand64()},
    m_k1{FastRandomContext().rand64()} {}

size_t SaltedSipHasher::operator()(const std::span<const unsigned char>& script) const
{
    return CSipHasher(m_k0, m_k1).Write(script).Finalize();
}
