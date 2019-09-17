// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <saltedhash.h>

#include <limits>

SaltedTxidHasher::SaltedTxidHasher() : k0(GetRand(std::numeric_limits<uint64_t>::max())), k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

SaltedOutpointHasher::SaltedOutpointHasher() : k0(GetRand(std::numeric_limits<uint64_t>::max())), k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

SaltedKeyIDHasher::SaltedKeyIDHasher() : m_k0(GetRand(std::numeric_limits<uint64_t>::max())), m_k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

size_t SaltedKeyIDHasher::operator()(const CKeyID& id) const
{
    return CSipHasher(m_k0, m_k1).Write(id.begin(), id.size()).Finalize();
}

SaltedScriptIDHasher::SaltedScriptIDHasher() : m_k0(GetRand(std::numeric_limits<uint64_t>::max())), m_k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

size_t SaltedScriptIDHasher::operator()(const CScriptID& id) const
{
    return CSipHasher(m_k0, m_k1).Write(id.begin(), id.size()).Finalize();
}

SaltedScriptHasher::SaltedScriptHasher() : m_k0(GetRand(std::numeric_limits<uint64_t>::max())), m_k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

size_t SaltedScriptHasher::operator()(const CScript& script) const
{
    return CSipHasher(m_k0, m_k1).Write(script.data(), script.size()).Finalize();
}

SaltedPubkeyHasher::SaltedPubkeyHasher() : m_k0(GetRand(std::numeric_limits<uint64_t>::max())), m_k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

size_t SaltedPubkeyHasher::operator()(const CPubKey& script) const
{
    return CSipHasher(m_k0, m_k1).Write(script.data(), script.size()).Finalize();
}
