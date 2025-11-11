// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_SWIFTSYNC_H
#define BITCOIN_SWIFTSYNC_H
#include <arith_uint256.h>
#include <primitives/transaction.h>
#include <util/hasher.h>

namespace swiftsync {
/** An aggregate for the SwiftSync protocol.
 * This class is intentionally left opaque, as internal changes may occur,
 * but all aggregates will have the concept of "create" and "spending" an
 * outpoint.
 *
 * The current implementation uses the existing `SaltedOutpointHasher` and
 * maintains two rotating integers.
 * */
class Aggregate
{
    arith_uint256 m_created{};
    arith_uint256 m_spent{};
    SaltedOutpointHasher m_hasher{};

public:
    bool IsBalanced() const { return m_created == m_spent; }
    void Create(const COutPoint& outpoint) { m_created += m_hasher(outpoint); }
    void Spend(const COutPoint& outpoint) { m_spent += m_hasher(outpoint); }
};
} // namespace swiftsync
#endif // BITCOIN_SWIFTSYNC_H
