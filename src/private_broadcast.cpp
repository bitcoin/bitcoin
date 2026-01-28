// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <private_broadcast.h>
#include <util/check.h>

#include <algorithm>
#include <ranges>

/// If a transaction is not received back from the network for this duration
/// after it is broadcast, then we consider it stale / for rebroadcasting.
static constexpr auto STALE_DURATION{1min};

bool PrivateBroadcast::Add(const CTransactionRef& tx)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    auto [it, inserted]{m_transactions.try_emplace(tx)};
    if (!inserted && it->second.final_state.has_value()) {
        // Re-schedule a previously finished transaction for private broadcast again.
        it->second.final_state.reset();
        it->second.sent_to.clear();
        inserted = true;
    }
    return inserted;
}

std::optional<size_t> PrivateBroadcast::StopBroadcasting(const CTransactionRef& tx, util::Expected<CService, std::string> final_state)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto it{m_transactions.find(tx)};
    if (it == m_transactions.end()) return std::nullopt;

    // If we've already finished broadcasting:
    // - do nothing for repeated aborts / repeated received-from updates
    // - if we previously aborted but now have a received-from address, update the stored final state
    //   to record the address. Don't return a value to avoid double accounting in callers.
    if (it->second.final_state.has_value()) {
        if (!it->second.final_state->result.has_value() && final_state.has_value()) {
            it->second.final_state->result = std::move(final_state);
            it->second.final_state->time = NodeClock::now();
        }
        return std::nullopt;
    }

    it->second.final_state.emplace(PrivateBroadcast::FinalState{
        .result = std::move(final_state),
        .time = NodeClock::now(),
    });
    const auto p{DerivePriority(it->second.sent_to)};
    return p.num_confirmed;
}

std::optional<CTransactionRef> PrivateBroadcast::PickTxForSend(const NodeId& will_send_to_nodeid, const CService& will_send_to_address)
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);

    auto pending = m_transactions | std::views::filter([](const auto& el) {
        return !el.second.final_state.has_value();
    });

    const auto it{std::ranges::max_element(
        pending,
        [](const auto& a, const auto& b) { return a < b; },
        [](const auto& el) { return DerivePriority(el.second.sent_to); })};

    if (it != pending.end()) {
        it->second.sent_to.emplace_back(will_send_to_nodeid, will_send_to_address, NodeClock::now());
        return it->first;
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
    return std::ranges::any_of(m_transactions, [](const auto& el) { return !el.second.final_state.has_value(); });
}

std::vector<CTransactionRef> PrivateBroadcast::GetStale() const
    EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    const auto stale_time{NodeClock::now() - STALE_DURATION};
    std::vector<CTransactionRef> stale;
    for (const auto& [tx, state] : m_transactions) {
        if (state.final_state.has_value()) continue;
        const Priority p{DerivePriority(state.sent_to)};
        if (p.last_confirmed < stale_time) {
            stale.push_back(tx);
        }
    }
    return stale;
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
        if (state.final_state.has_value()) continue;
        for (auto& send_status : state.sent_to) {
            if (send_status.nodeid == nodeid) {
                return TxAndSendStatusForNode{.tx = tx, .send_status = send_status};
            }
        }
    }
    return std::nullopt;
}
