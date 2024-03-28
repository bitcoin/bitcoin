// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKREQUESTTRACKER_H
#define BITCOIN_BLOCKREQUESTTRACKER_H

#include <chain.h>
#include <net.h>
#include <serialize.h>
#include <util/hasher.h>

#include <list>

/**
 * Structure in charge of tracking single blocks download progress.
 * For now, only used to track blocks that aren't part of the automatic download process.
 */
class BlockRequestTracker
{
private:
    struct BlockRequest {
        // The in-flight request vector (preserving insertion order).
        // Could be empty if:
        // 1) peer failed to provide us the block and got disconnected.
        // 2) if there are no available peers to request the block from.
        // And, only when this is empty, 'm_pending' will contain an entry pointing
        // to this structure. Which can be assigned to a peer by calling 'pop_pending()'.
        std::vector<NodeId> in_flight;

        explicit BlockRequest() {}
    };

    Mutex cs_block_tracker;
    // The blocks we are tracking
    using BlockTrackMap = std::unordered_map<uint256, BlockRequest, BlockHasher>;
    BlockTrackMap m_tracked_blocks GUARDED_BY(cs_block_tracker);
    // Block requests waiting for getdata submission
    std::list<BlockTrackMap::iterator> m_pending;

    void untrack_internal(const uint256& block_hash) EXCLUSIVE_LOCKS_REQUIRED(cs_block_tracker);

public:

    /**
     * Add block to the tracking list.
     * Returns false when block is already tracked or when request is
     * already in-flight for 'peer_id'.
     */
    bool track(const uint256& block_hash, std::optional<NodeId> peer_id=std::nullopt) EXCLUSIVE_LOCKS_REQUIRED(!cs_block_tracker);

    /** Stop tracking the block */
    void untrack(const uint256& block_hash) EXCLUSIVE_LOCKS_REQUIRED(!cs_block_tracker);

    /** Remove request made to peer_id */
    void untrack_request(NodeId peer_id, const uint256& block_hash) EXCLUSIVE_LOCKS_REQUIRED(!cs_block_tracker);

    /**
     * Try to pop pending requests and assign them to a certain peer.
     * The node must send the getdata request after acquiring the pending request.
     */
    enum class ForPendingStatus {
        POP,    // Mark the current pending request as in-flight.
        SKIP,   // Skip the current pending request.
        STOP,   // Stop traversing the pending list.
        CANCEL  // Untrack the request. Remove it from the tracking list.
    };
    void for_pending(NodeId peer_id, const std::function<ForPendingStatus(const uint256&)>& check) EXCLUSIVE_LOCKS_REQUIRED(!cs_block_tracker);
};

#endif // BITCOIN_BLOCKREQUESTTRACKER_H
