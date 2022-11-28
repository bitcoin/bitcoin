// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_EVICTION_H
#define BITCOIN_NODE_EVICTION_H

#include <node/connection_types.h>
#include <net_permissions.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

class EvictionManagerImpl;

typedef int64_t NodeId;

class EvictionManager
{
private:
    const std::unique_ptr<EvictionManagerImpl> m_impl;

public:
    EvictionManager();
    ~EvictionManager();

    void AddCandidate(NodeId id, std::chrono::seconds connected,
                      uint64_t keyed_net_group, bool prefer_evict,
                      bool is_local, Network network,
                      bool noban, ConnectionType conn_type);
    bool RemoveCandidate(NodeId id);

    /**
     * Select an inbound peer to evict after filtering out (protecting) peers having
     * distinct, difficult-to-forge characteristics. The protection logic picks out
     * fixed numbers of desirable peers per various criteria, followed by (mostly)
     * ratios of desirable or disadvantaged peers. If any eviction candidates
     * remain, the selection logic chooses a peer to evict.
     */
    [[nodiscard]] std::optional<NodeId> SelectNodeToEvict() const;

    /** A ping-pong round trip has completed successfully. Update the candidate's minimum ping time. */
    void UpdateMinPingTime(NodeId id, std::chrono::microseconds ping_time);
    std::optional<std::chrono::microseconds> GetMinPingTime(NodeId id) const;

    /** A new valid block was received. Update the candidate's last block time. */
    void UpdateLastBlockTime(NodeId id, std::chrono::seconds block_time);
    std::optional<std::chrono::seconds> GetLastBlockTime(NodeId id) const;

    /** A new valid transaction was received. Update the candidate's last tx time. */
    void UpdateLastTxTime(NodeId id, std::chrono::seconds tx_time);
    std::optional<std::chrono::seconds> GetLastTxTime(NodeId id) const;

    /** Update the candidate's relevant services flag. */
    void UpdateRelevantServices(NodeId id, bool has_relevant_flags);

    /** Update the candidate's bloom filter loaded flag. */
    void UpdateLoadedBloomFilter(NodeId id, bool bloom_filter_loaded);

    /** Set the candidate's tx relay status to true. */
    void UpdateRelayTxs(NodeId id);

    bool HasCandidate(NodeId id) const;
};

#endif // BITCOIN_NODE_EVICTION_H
