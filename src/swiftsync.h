// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_SWIFTSYNC_H
#define BITCOIN_SWIFTSYNC_H
#include <hash.h>
#include <primitives/transaction.h>
#include <cstdint>

namespace swiftsync {
/** An aggregate for the SwiftSync protocol.
 * This class is intentionally left opaque, as internal changes may occur,
 * but all aggregates will have the concept of "adding" and "spending" an
 * outpoint.
 *
 * The current implementation uses a salted SHA-256 hash and updates two
 * 64-bit integers by taking the first 16 bytes of the hash and adding or
 * subtracting according to if the outpoint was added or spent.
 * */
class Aggregate
{
private:
    uint64_t m_limb0{}, m_limb1{}, m_limb2{}, m_limb3{};
    HashWriter m_salted_hasher{};

public:
    Aggregate();
    /** Is the internal state zero, representing the empty set. */
    bool IsZero() const { return m_limb0 == 0 && m_limb1 == 0 && m_limb2 == 0 && m_limb3 == 0; }
    /** Add an outpoint created in a block. */
    void Add(const COutPoint& outpoint);
    /** Spend an outpoint used in a block. */
    void Spend(const COutPoint& outpoint);
};
} // namespace swiftsync
#endif // BITCOIN_SWIFTSYNC_H
