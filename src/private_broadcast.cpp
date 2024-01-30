// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <private_broadcast.h>

bool PrivateBroadcast::Add(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    const Txid& txid = tx->GetHash();
    LOCK(m_mutex);
    auto [pos, inserted] = m_by_txid.emplace(txid, TxWithPriority{.tx = tx, .priority = Priority{}});
    if (inserted) {
        m_by_priority.emplace(Priority{}, txid);
    }
    return inserted;
}

bool PrivateBroadcast::Priority::operator<(const Priority& other) const
{
    if (num_broadcasted < other.num_broadcasted) {
        return true;
    }
    return last_broadcasted < other.last_broadcasted;
}
