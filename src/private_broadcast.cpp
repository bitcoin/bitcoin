// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <private_broadcast.h>
#include <util/check.h>

/// If a transaction is not received back from the network for this duration
/// after it is broadcast, then we  consider it stale / for rebroadcasting.
static constexpr auto STALE_DURATION{1min};

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

std::optional<size_t> PrivateBroadcast::Remove(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    auto iters = Find(tx->GetHash());
    if (!iters || iters->by_txid->second.tx->GetWitnessHash() != tx->GetWitnessHash()) {
        return std::nullopt;
    }
    const size_t num_broadcasted{iters->by_priority->first.num_broadcasted};
    m_by_priority.erase(iters->by_priority);
    m_by_txid.erase(iters->by_txid);
    return num_broadcasted;
}

std::optional<CTransactionRef> PrivateBroadcast::GetTxForBroadcast() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (m_by_priority.empty()) {
        return std::nullopt;
    }
    const Txid& txid = m_by_priority.begin()->second;
    auto it = m_by_txid.find(txid);
    if (Assume(it != m_by_txid.end())) {
        return it->second.tx;
    }
    m_by_priority.erase(m_by_priority.begin());
    return std::nullopt;
}

void PrivateBroadcast::PushedToNode(const NodeId& nodeid, const Txid& txid) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    m_by_nodeid.emplace(nodeid, txid);
}

std::optional<CTransactionRef> PrivateBroadcast::GetTxPushedToNode(const NodeId& nodeid) const
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);

    auto it_by_node = m_by_nodeid.find(nodeid);
    if (it_by_node == m_by_nodeid.end()) {
        return std::nullopt;
    }
    const Txid txid{it_by_node->second};

    auto it_by_txid = m_by_txid.find(txid);
    if (it_by_txid == m_by_txid.end()) {
        return std::nullopt;
    }
    return it_by_txid->second.tx;
}

bool PrivateBroadcast::FinishBroadcast(const NodeId& nodeid, bool confirmed_by_node) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    auto it = m_by_nodeid.find(nodeid);
    if (it == m_by_nodeid.end()) {
        return false;
    }
    const Txid txid{it->second};
    m_by_nodeid.erase(it);

    auto iters = Find(txid);

    if (!iters.has_value()) {
        return false;
    }
    if (!confirmed_by_node) {
        return true;
    }

    // Update broadcast stats, since txid was found and its reception is confirmed by the node.

    Priority& priority = iters->by_txid->second.priority;

    ++priority.num_broadcasted;
    priority.last_broadcasted = NodeClock::now();

    // Remove and re-add the entry in the m_by_priority map because we have changed the key.
    m_by_priority.erase(iters->by_priority);
    m_by_priority.emplace(priority, txid);

    return true;
}

std::vector<CTransactionRef> PrivateBroadcast::GetStale() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto stale_time = NodeClock::now() - STALE_DURATION;
    std::vector<CTransactionRef> stale;
    for (const auto& [txid, tx_with_priority] : m_by_txid) {
        if (tx_with_priority.priority.last_broadcasted < stale_time) {
            stale.push_back(tx_with_priority.tx);
        }
    }
    return stale;
}

bool PrivateBroadcast::Priority::operator<(const Priority& other) const
{
    if (num_broadcasted < other.num_broadcasted) {
        return true;
    }
    return last_broadcasted < other.last_broadcasted;
}

std::optional<PrivateBroadcast::Iterators> PrivateBroadcast::Find(const Txid& txid) EXCLUSIVE_LOCKS_REQUIRED(m_mutex)
{
    AssertLockHeld(m_mutex);
    auto i = m_by_txid.find(txid);
    if (i == m_by_txid.end()) {
        return std::nullopt;
    }
    const Priority& priority = i->second.priority;
    for (auto j = m_by_priority.lower_bound(priority); j != m_by_priority.end(); ++j) {
        if (j->second == txid) {
            return Iterators{.by_txid = i, .by_priority = j};
        }
    }
    return std::nullopt;
}
