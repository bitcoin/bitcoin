// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <private_broadcast.h>

#include <util/check.h>

#include <algorithm>
#include <ranges>


PrivateBroadcast::AddResult PrivateBroadcast::Add(const CTransactionRef& tx)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    if (const auto it{m_transactions.find(tx)}; it != m_transactions.end()) {
        if (IsPending(it->second)) return AddResult::AlreadyPresent;

        // An exhausted transaction can be explicitly retried by adding it again.
        it->second.time_added = NodeClock::now();
        it->second.send_statuses.clear();
        return AddResult::Added;
    }

    if (m_transactions.size() >= m_max_transactions) return AddResult::QueueFull;

    m_transactions.try_emplace(tx);
    return AddResult::Added;
}

std::optional<size_t> PrivateBroadcast::Remove(const CTransactionRef& tx)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto handle{m_transactions.extract(tx)};
    if (handle) {
        const auto p{DerivePriority(handle.mapped().send_statuses)};
        return p.num_confirmed;
    }
    return std::nullopt;
}

std::optional<CTransactionRef> PrivateBroadcast::PickTxForSend(const NodeId& will_send_to_nodeid, const CService& will_send_to_address)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);

    if (GetSendStatusByNode(will_send_to_nodeid).has_value()) { // nodeid reuse, shouldn't send >1 tx to a given node
        Assume(false);
        return std::nullopt;
    }

    auto pending_transactions{m_transactions | std::views::filter([this](const auto& entry) { return IsPending(entry.second); })};
    const auto it{std::ranges::max_element(
            pending_transactions,
            [](const auto& a, const auto& b) { return a < b; },
            [](const auto& el) { return DerivePriority(el.second.send_statuses); })};

    if (it != pending_transactions.end()) {
        auto& [tx, state]{*it};
        state.send_statuses.emplace_back(will_send_to_nodeid, will_send_to_address, NodeClock::now());
        return tx;
    }

    return std::nullopt;
}

std::optional<CTransactionRef> PrivateBroadcast::GetTxForNode(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto tx_and_status{GetSendStatusByNode(nodeid)};
    if (tx_and_status.has_value()) {
        return tx_and_status.value().tx;
    }
    return std::nullopt;
}

void PrivateBroadcast::NodeConfirmedReception(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto tx_and_status{GetSendStatusByNode(nodeid)};
    if (tx_and_status.has_value()) {
        tx_and_status.value().send_status.confirmed = NodeClock::now();
    }
}

bool PrivateBroadcast::DidNodeConfirmReception(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto tx_and_status{GetSendStatusByNode(nodeid)};
    if (tx_and_status.has_value()) {
        return tx_and_status.value().send_status.confirmed.has_value();
    }
    return false;
}

bool PrivateBroadcast::HavePendingTransactions()
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    return std::ranges::any_of(m_transactions, [this](const auto& entry) { return IsPending(entry.second); });
}

std::vector<CTransactionRef> PrivateBroadcast::GetStale() const
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto now{NodeClock::now()};
    std::vector<CTransactionRef> stale;
    for (const auto& [tx, state] : m_transactions) {
        if (!IsPending(state)) continue;
        const Priority p{DerivePriority(state.send_statuses)};
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
    std::vector<TxBroadcastInfo> entries;
    entries.reserve(m_transactions.size());

    for (const auto& [tx, state] : m_transactions) {
        std::vector<PeerSendInfo> peers;
        peers.reserve(state.send_statuses.size());
        for (const auto& status : state.send_statuses) {
            peers.emplace_back(PeerSendInfo{.address = status.address, .sent = status.picked, .received = status.confirmed});
        }
        const size_t attempts_remaining{m_max_send_attempts - std::min(state.send_statuses.size(), m_max_send_attempts)};
        entries.emplace_back(TxBroadcastInfo{.tx = tx, .time_added = state.time_added, .attempts_remaining = attempts_remaining, .peers = std::move(peers)});
    }

    return entries;
}

bool PrivateBroadcast::IsPending(const TxSendStatus& status) const
{
    return status.send_statuses.size() < m_max_send_attempts;
}

PrivateBroadcast::Priority PrivateBroadcast::DerivePriority(const std::vector<SendStatus>& sent_to)
{
    Priority p;
    p.num_picked = sent_to.size();
    for (const auto& send_status : sent_to) {
        p.last_picked = std::max(p.last_picked, send_status.picked);
        if (send_status.confirmed.has_value()) {
            ++p.num_confirmed;
            p.last_confirmed = std::max(p.last_confirmed, send_status.confirmed.value());
        }
    }
    return p;
}

std::optional<PrivateBroadcast::TxAndSendStatusForNode> PrivateBroadcast::GetSendStatusByNode(const NodeId& nodeid)
    EXCLUSIVE_LOCKS_REQUIRED(m_mutex)
{
    AssertLockHeld(m_mutex);
    for (auto& [tx, state] : m_transactions) {
        for (auto& send_status : state.send_statuses) {
            if (send_status.nodeid == nodeid) {
                return TxAndSendStatusForNode{.tx = tx, .send_status = send_status};
            }
        }
    }
    return std::nullopt;
}
