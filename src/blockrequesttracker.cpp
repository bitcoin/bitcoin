// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <blockrequesttracker.h>

template <typename T, typename V>
static bool Contain(const T& t, const V& value) { return std::find(t.begin(), t.end(), value) != t.end(); }

bool BlockRequestTracker::track(const uint256& block_hash, std::optional<NodeId> peer_id)
{
    LOCK(cs_block_tracker);
    auto it = m_tracked_blocks.emplace(block_hash, BlockRequest{});
    if (!it.second) return false; // block already tracked

    if (peer_id) {
        // Add in-flight request
        BlockRequest& req = it.first->second;
        if (std::find(req.in_flight.begin(), req.in_flight.end(), *peer_id) != req.in_flight.end()) {
            return false; // Already existent request
        }
        req.in_flight.emplace_back(*peer_id);
    } else {
        // Add block to pending list
        m_pending.emplace_back(it.first);
    }
    return true;
}

void BlockRequestTracker::untrack_internal(const uint256& block_hash)
{
    auto it = m_tracked_blocks.find(block_hash);
    if (it == m_tracked_blocks.end()) return;
    auto pending_it = std::remove(m_pending.begin(), m_pending.end(), it);
    if (pending_it != m_pending.end()) m_pending.erase(pending_it);
    m_tracked_blocks.erase(it);
}

void BlockRequestTracker::untrack(const uint256& block_hash)
{
    LOCK(cs_block_tracker);
    untrack_internal(block_hash);
}

void BlockRequestTracker::untrack_request(NodeId peer_id, const uint256& block_hash)
{
    LOCK(cs_block_tracker);
    // Get tracked block request
    const auto& it = m_tracked_blocks.find(block_hash);
    if (it == m_tracked_blocks.end()) return;

    // Check if we were waiting for this peer
    BlockRequest& request = it->second;
    auto remove_it = std::remove(request.in_flight.begin(), request.in_flight.end(), peer_id);
    if (remove_it == request.in_flight.end()) return;

    // Clear request
    request.in_flight.erase(remove_it);

    // And add it to the pending vector if there are no other inflight request for this block
    if (request.in_flight.empty() && !Contain(m_pending, it)) {
        m_pending.emplace_back(it);
    }
}

void BlockRequestTracker::for_pending(NodeId peer_id, const std::function<ForPendingStatus(const uint256&)>& check)
{
    LOCK(cs_block_tracker);

    for (auto it = m_pending.begin(); it != m_pending.end();) {
        const auto& pending = *it;
        switch(check(pending->first)) {
            case ForPendingStatus::POP: {
                BlockRequest& request = pending->second;
                request.in_flight.emplace_back(peer_id);
                it = m_pending.erase(it);
                break;
            }
            case ForPendingStatus::SKIP:
                ++it;
                break;
            case ForPendingStatus::STOP:
                return;
            case ForPendingStatus::CANCEL:
                untrack_internal(pending->first);
                ++it;
                break;
        }
    }
}