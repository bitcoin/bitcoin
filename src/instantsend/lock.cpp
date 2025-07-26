// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <instantsend/lock.h>

#include <hash.h>
#include <primitives/transaction.h>
#include <util/hasher.h>

#include <string>
#include <unordered_set>

static constexpr std::string_view ISLOCK_REQUESTID_PREFIX{"islock"};

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
    const std::unordered_set<COutPoint, SaltedOutpointHasher> inputs_set{inputs.begin(), inputs.end()};
    return inputs_set.size() == inputs.size();
}
} // namespace instantsend
