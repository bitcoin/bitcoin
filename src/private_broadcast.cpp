// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <private_broadcast.h>

#include <util/check.h>

#include <algorithm>


[[nodiscard]] PrivateBroadcast::AddResult PrivateBroadcast::Add(const CTransactionRef& tx)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    // Re-adding an already-tracked transaction is a no-op regardless of the cap.
    if (m_transactions.contains(tx)) return AddResult::AlreadyPresent;

    if (m_transactions.size() >= m_max_transactions) return AddResult::QueueFull;

    m_transactions.try_emplace(tx);
    return AddResult::Added;
}

std::optional<size_t> PrivateBroadcast::Remove(const CTransactionRef& tx)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto handle{m_transactions.extract(tx)};
    if (!handle) {
        return std::nullopt;
    }
    std::erase_if(m_by_node, [&tx](const auto& entry) {
        return entry.second.tx->GetWitnessHash() == tx->GetWitnessHash();
    });
    return handle.mapped().priority.num_confirmed;
}

std::optional<CTransactionRef> PrivateBroadcast::PickTxForSend(const NodeId& will_send_to_nodeid, const CService& will_send_to_address)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);

    if (m_by_node.contains(will_send_to_nodeid)) { // nodeid reuse, shouldn't send >1 tx to a given node
        Assume(false);
        return std::nullopt;
    }

    const auto it{std::ranges::max_element(
            m_transactions,
            [](const auto& a, const auto& b) { return a < b; },
            [](const auto& el) { return el.second.priority; })};

    if (it != m_transactions.end()) {
        auto& [tx, state]{*it};
        const auto now{NodeClock::now()};
        ++state.priority.num_picked;
        state.priority.last_picked = now;
        m_by_node.emplace(will_send_to_nodeid,
                          NodeSend{.tx = tx, .address = will_send_to_address, .sent = now, .confirmed = std::nullopt});
        return tx;
    }

    return std::nullopt;
}

std::optional<CTransactionRef> PrivateBroadcast::GetTxForNode(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto it{m_by_node.find(nodeid)};
    if (it != m_by_node.end()) {
        return it->second.tx;
    }
    return std::nullopt;
}

void PrivateBroadcast::NodeConfirmedReception(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto it{m_by_node.find(nodeid)};
    if (it == m_by_node.end() || it->second.confirmed.has_value()) {
        return;
    }
    const auto now{NodeClock::now()};
    it->second.confirmed = now;
    const auto tx_it{m_transactions.find(it->second.tx)};
    if (Assume(tx_it != m_transactions.end())) {
        ++tx_it->second.priority.num_confirmed;
        tx_it->second.priority.last_confirmed = now;
    }
}

bool PrivateBroadcast::NodeDisconnected(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto handle{m_by_node.extract(nodeid)};
    return handle && handle.mapped().confirmed.has_value();
}

bool PrivateBroadcast::HavePendingTransactions()
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    return !m_transactions.empty();
}

std::vector<CTransactionRef> PrivateBroadcast::GetStale() const
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto now{NodeClock::now()};
    std::vector<CTransactionRef> stale;
    for (const auto& [tx, state] : m_transactions) {
        const Priority& p{state.priority};
        if (p.num_confirmed == 0) {
            if (state.time_added < now - INITIAL_STALE_DURATION) stale.push_back(tx);
        } else {
            if (p.last_confirmed < now - STALE_DURATION) stale.push_back(tx);
        }
    }
    return stale;
}

std::vector<PrivateBroadcast::TxBroadcastInfo> PrivateBroadcast::GetBroadcastInfo() const
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    std::unordered_map<CTransactionRef, std::vector<PeerSendInfo>, CTransactionRefHash, CTransactionRefComp> peers_by_tx;
    for (const auto& [nodeid, node_send] : m_by_node) {
        peers_by_tx[node_send.tx].emplace_back(PeerSendInfo{.address = node_send.address, .sent = node_send.sent, .received = node_send.confirmed});
    }

    std::vector<TxBroadcastInfo> entries;
    entries.reserve(m_transactions.size());

    for (const auto& [tx, state] : m_transactions) {
        std::vector<PeerSendInfo> peers;
        if (const auto it{peers_by_tx.find(tx)}; it != peers_by_tx.end()) {
            peers = std::move(it->second);
        }
        entries.emplace_back(TxBroadcastInfo{.tx = tx,
                                             .time_added = state.time_added,
                                             .num_sent = state.priority.num_picked,
                                             .num_confirmed = state.priority.num_confirmed,
                                             .peers = std::move(peers)});
    }

    return entries;
}
