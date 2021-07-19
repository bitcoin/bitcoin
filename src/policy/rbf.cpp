// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/rbf.h>

#include <policy/settings.h>
#include <tinyformat.h>

bool GetEntriesForConflicts(const CTransaction& tx,
                            CTxMemPool& m_pool,
                            const CTxMemPool::setEntries& setIterConflicting,
                            CTxMemPool::setEntries& allConflicting,
                            std::string& err_string)
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
            err_string = strprintf("rejecting replacement %s; too many potential replacements (%d > %d)\n",
                        hash.ToString(),
                        nConflictingCount,
                        MAX_BIP125_REPLACEMENT_CANDIDATES);
            return false;
        }
    }
    // If not too many to replace, then calculate the set of
    // transactions that would have to be evicted
    for (CTxMemPool::txiter it : setIterConflicting) {
        m_pool.CalculateDescendants(it, allConflicting);
    }
    return true;
}

