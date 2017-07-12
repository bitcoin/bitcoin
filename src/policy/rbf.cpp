// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/rbf.h>

bool SignalsOptInRBF(const CTransaction &tx)
{
    for (const CTxIn &txin : tx.vin) {
        if (txin.nSequence <= MAX_BIP125_RBF_SEQUENCE) {
            return true;
        }
    }
    return false;
}

bool ExpiredOptInRBFPolicy(const int64_t now, const int64_t accepted, const int64_t timeout)
{
    return now - accepted >= timeout;
}

RBFTransactionState IsRBFOptIn(const CTransaction& tx, const CTxMemPool& pool, const int64_t now, const int64_t timeout, const bool enabled_replacement_timeout)
{
    AssertLockHeld(pool.cs);

    CTxMemPool::setEntries setAncestors;

    // First check the transaction itself.
    const int64_t conflicting_time = pool.info(tx.GetHash()).nTime;
    if ((enabled_replacement_timeout && ExpiredOptInRBFPolicy(now, conflicting_time, timeout)) || SignalsOptInRBF(tx)) {
        return RBFTransactionState::REPLACEABLE_BIP125;
    }

    // If this transaction is not in our mempool, then we can't be sure
    // we will know about all its inputs.
    if (!pool.exists(tx.GetHash())) {
        return RBFTransactionState::UNKNOWN;
    }

    // If all the inputs have nSequence >= maxint-1, it still might be
    // signaled for RBF if any unconfirmed parents have signaled.
    const uint64_t noLimit = std::numeric_limits<uint64_t>::max();
    std::string dummy;
    const CTxMemPoolEntry entry = *pool.mapTx.find(tx.GetHash());
    pool.CalculateMemPoolAncestors(entry, setAncestors, noLimit, noLimit, noLimit, noLimit, dummy, false);

    for (const CTxMemPool::txiter it : setAncestors) {
        const int64_t ancestor_time = pool.info(it->GetTx().GetHash()).nTime;
        if ((enabled_replacement_timeout && ExpiredOptInRBFPolicy(now, ancestor_time, timeout)) || SignalsOptInRBF(it->GetTx())) {
            return RBFTransactionState::REPLACEABLE_BIP125;
        }
    }
    return RBFTransactionState::FINAL;
}
