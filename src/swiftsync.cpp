// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <primitives/transaction.h>
#include <random.h>
#include <swiftsync.h>
#include <uint256.h>
#include <span>

using namespace swiftsync;

Aggregate::Aggregate()
{
    std::span<unsigned char> salt;
    GetStrongRandBytes(salt);
    m_salted_hasher.write(std::as_writable_bytes(salt));
}

void Aggregate::Add(const COutPoint& outpoint)
{
    auto hash = (HashWriter(m_salted_hasher) << outpoint).GetSHA256();
    m_limb0 += hash.GetUint64(0);
    m_limb1 += hash.GetUint64(1);
    m_limb2 += hash.GetUint64(2);
    m_limb3 += hash.GetUint64(3);
}

void Aggregate::Spend(const COutPoint& outpoint)
{
    auto hash = (HashWriter(m_salted_hasher) << outpoint).GetSHA256();
    m_limb0 -= hash.GetUint64(0);
    m_limb1 -= hash.GetUint64(1);
    m_limb2 -= hash.GetUint64(2);
    m_limb3 -= hash.GetUint64(3);
}
