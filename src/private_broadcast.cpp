// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <private_broadcast.h>

#include <random.h>
#include <util/check.h>

#include <algorithm>
#include <ranges>


PrivateBroadcast::AddResult PrivateBroadcast::Add(const CTransactionRef& tx,
                                                  std::optional<NodeClock::time_point> release_time)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto now{NodeClock::now()};
    const bool released{!release_time.has_value() || release_time.value() <= now};
    const auto set_times{[&](TxSendStatus& status) {
        status.time_added = now;
        status.release_time = released ? now : release_time.value();
        status.released = released;
    }};

    if (const auto it{m_transactions.find(tx)}; it != m_transactions.end()) {
        if (IsPending(it->second)) return AddResult::AlreadyPresent;

        // An exhausted transaction can be explicitly retried by adding it again.
        set_times(it->second);
        it->second.send_statuses.clear();
        return AddResult::Added;
    }

    if (m_transactions.size() >= m_max_transactions) return AddResult::QueueFull;

    set_times(m_transactions.try_emplace(tx).first->second);
    return AddResult::Added;
}

std::optional<PrivateBroadcast::RemoveResult> PrivateBroadcast::Remove(const CTransactionRef& tx)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto handle{m_transactions.extract(tx)};
    if (handle) {
        const auto p{DerivePriority(handle.mapped().send_statuses)};
        return RemoveResult{.num_confirmed = p.num_confirmed, .released = handle.mapped().released};
    }
    return std::nullopt;
}

std::vector<CTransactionRef> PrivateBroadcast::ReleaseDue()
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto now{NodeClock::now()};
    std::vector<CTransactionRef> due;
    for (auto& [tx, state] : m_transactions) {
        if (state.released || state.release_time > now) continue;
        state.released = true;
        due.push_back(tx);
    }
    return due;
}

std::chrono::seconds PrivateBroadcast::RandomizeDelay(std::chrono::seconds requested, FastRandomContext& rng)
{
    if (requested <= 0s) return 0s;
    requested = std::min(requested, MAX_DELAY);
    const std::chrono::seconds range{std::max(requested * DELAY_RANDOMIZATION_PERCENT / 100, MIN_DELAY_RANDOMIZATION)};
    return requested + rng.randrange<std::chrono::seconds>(range + 1s);
}

std::optional<CTransactionRef> PrivateBroadcast::PickTxForSend(const NodeId& will_send_to_nodeid, const CService& will_send_to_address)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);

    if (GetSendStatusByNode(will_send_to_nodeid).has_value()) { // nodeid reuse, shouldn't send >1 tx to a given node
        Assume(false);
        return std::nullopt;
    }

    auto pending_transactions{m_transactions | std::views::filter([this](const auto& entry) { return IsSendable(entry.second); })};
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
    return std::ranges::any_of(m_transactions, [this](const auto& entry) { return IsSendable(entry.second); });
}

std::vector<CTransactionRef> PrivateBroadcast::GetStale() const
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto now{NodeClock::now()};
    std::vector<CTransactionRef> stale;
    for (const auto& [tx, state] : m_transactions) {
        if (!IsSendable(state)) continue;
        const Priority p{DerivePriority(state.send_statuses)};
        if (p.num_confirmed == 0) {
            if (state.release_time < now - INITIAL_STALE_DURATION) stale.push_back(tx);
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
        entries.emplace_back(TxBroadcastInfo{.tx = tx, .time_added = state.time_added, .release_time = state.release_time, .attempts_remaining = attempts_remaining, .peers = std::move(peers)});
    }

    return entries;
}

bool PrivateBroadcast::IsPending(const TxSendStatus& status) const
{
    return status.send_statuses.size() < m_max_send_attempts;
}

bool PrivateBroadcast::IsSendable(const TxSendStatus& status) const
{
    return status.released && IsPending(status);
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
