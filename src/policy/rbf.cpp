// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/rbf.h>

#include <policy/settings.h>
#include <tinyformat.h>
#include <util/moneystr.h>
#include <util/rbf.h>

RBFTransactionState IsRBFOptIn(const CTransaction& tx, const CTxMemPool& pool)
{
    AssertLockHeld(pool.cs);

    CTxMemPool::setEntries setAncestors;

    // First check the transaction itself.
    if (SignalsOptInRBF(tx)) {
        return RBFTransactionState::REPLACEABLE_BIP125;
    }

    // If this transaction is not in our mempool, then we can't be sure
    // we will know about all its inputs.
    if (!pool.exists(tx.GetHash())) {
        return RBFTransactionState::UNKNOWN;
    }

    // If all the inputs have nSequence >= maxint-1, it still might be
    // signaled for RBF if any unconfirmed parents have signaled.
    uint64_t noLimit = std::numeric_limits<uint64_t>::max();
    std::string dummy;
    CTxMemPoolEntry entry = *pool.mapTx.find(tx.GetHash());
    pool.CalculateMemPoolAncestors(entry, setAncestors, noLimit, noLimit, noLimit, noLimit, dummy, false);

    for (CTxMemPool::txiter it : setAncestors) {
        if (SignalsOptInRBF(it->GetTx())) {
            return RBFTransactionState::REPLACEABLE_BIP125;
        }
    }
    return RBFTransactionState::FINAL;
}

RBFTransactionState IsRBFOptInEmptyMempool(const CTransaction& tx)
{
    // If we don't have a local mempool we can only check the transaction itself.
    return SignalsOptInRBF(tx) ? RBFTransactionState::REPLACEABLE_BIP125 : RBFTransactionState::UNKNOWN;
}

std::optional<std::string> GetEntriesForConflicts(const CTransaction& tx,
                            CTxMemPool& m_pool,
                            const CTxMemPool::setEntries& setIterConflicting,
                            CTxMemPool::setEntries& allConflicting)
{
    AssertLockHeld(m_pool.cs);
    const uint256 hash = tx.GetHash();
    uint64_t nConflictingCount = 0;
    for (const auto& mi : setIterConflicting) {
        nConflictingCount += mi->GetCountWithDescendants();
        // This potentially overestimates the number of actual descendants
        // but we just want to be conservative to avoid doing too much
        // work.
        if (nConflictingCount > MAX_BIP125_REPLACEMENT_CANDIDATES) {
            return strprintf("rejecting replacement %s; too many potential replacements (%d > %d)\n",
                        hash.ToString(),
                        nConflictingCount,
                        MAX_BIP125_REPLACEMENT_CANDIDATES);
        }
    }
    // If not too many to replace, then calculate the set of
    // transactions that would have to be evicted
    for (CTxMemPool::txiter it : setIterConflicting) {
        m_pool.CalculateDescendants(it, allConflicting);
    }
    return std::nullopt;
}

