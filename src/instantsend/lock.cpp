// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <instantsend/lock.h>

#include <hash.h>
#include <primitives/transaction.h>

#include <set>
#include <string>

static const std::string_view ISLOCK_REQUESTID_PREFIX = "islock";

namespace instantsend {
uint256 InstantSendLock::GetRequestId() const
{
    CHashWriter hw(SER_GETHASH, 0);
    hw << ISLOCK_REQUESTID_PREFIX;
    hw << inputs;
    return hw.GetHash();
}

/**
 * Handles trivial ISLock verification
 * @return returns false if verification failed, otherwise true
 */
bool InstantSendLock::TriviallyValid() const
{
    if (txid.IsNull() || inputs.empty()) {
        return false;
    }

    // Check that each input is unique
    std::set<COutPoint> dups;
    for (const auto& o : inputs) {
        if (!dups.emplace(o).second) {
            return false;
        }
    }

    return true;
}
} // namespace instantsend
