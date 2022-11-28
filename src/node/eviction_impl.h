// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_EVICTION_IMPL_H
#define BITCOIN_NODE_EVICTION_IMPL_H

#include <node/connection_types.h>
#include <net_permissions.h>
#include <sync.h>

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

typedef int64_t NodeId;

struct NodeEvictionCandidate {
    NodeId id;
    std::chrono::seconds m_connected;
    std::chrono::microseconds m_min_ping_time;
    std::chrono::seconds m_last_block_time;
    std::chrono::seconds m_last_tx_time;
    bool fRelevantServices;
    bool m_relay_txs;
    bool fBloomFilter;
    uint64_t nKeyedNetGroup;
    bool prefer_evict;
    bool m_is_local;
    Network m_network;
    bool m_noban;
    ConnectionType m_conn_type;
};

[[nodiscard]] std::optional<NodeId> SelectNodeToEvict(std::vector<NodeEvictionCandidate>&& vEvictionCandidates);

/** Protect desirable or disadvantaged inbound peers from eviction by ratio.
 *
 * This function protects half of the peers which have been connected the
 * longest, to replicate the non-eviction implicit behavior and preclude attacks
 * that start later.
 *
 * Half of these protected spots (1/4 of the total) are reserved for the
 * following categories of peers, sorted by longest uptime, even if they're not
 * longest uptime overall:
 *
 * - onion peers connected via our tor control service
 *
 * - localhost peers, as manually configured hidden services not using
 *   `-bind=addr[:port]=onion` will not be detected as inbound onion connections
 *
 * - I2P peers
 *
 * - CJDNS peers
 *
 * This helps protect these privacy network peers, which tend to be otherwise
 * disadvantaged under our eviction criteria for their higher min ping times
 * relative to IPv4/IPv6 peers, and favorise the diversity of peer connections.
 */
void ProtectEvictionCandidatesByRatio(std::vector<NodeEvictionCandidate>& vEvictionCandidates);

class EvictionManagerImpl
{
private:
    mutable Mutex m_candidates_mutex;
    std::map<NodeId, NodeEvictionCandidate> m_candidates GUARDED_BY(m_candidates_mutex);

public:
    EvictionManagerImpl();
    ~EvictionManagerImpl();

    void AddCandidate(NodeId id, std::chrono::seconds connected,
                      uint64_t keyed_net_group, bool prefer_evict,
                      bool is_local, Network network,
                      bool noban, ConnectionType conn_type)
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);
    bool RemoveCandidate(NodeId id) EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);

    std::optional<NodeId> SelectNodeToEvict() const
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);

    void UpdateMinPingTime(NodeId id, std::chrono::microseconds ping_time)
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);
    std::optional<std::chrono::microseconds> GetMinPingTime(NodeId id) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);

    void UpdateLastBlockTime(NodeId id, std::chrono::seconds block_time)
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);
    std::optional<std::chrono::seconds> GetLastBlockTime(NodeId id) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);

    void UpdateLastTxTime(NodeId id, std::chrono::seconds tx_time)
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);
    std::optional<std::chrono::seconds> GetLastTxTime(NodeId id) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);

    void UpdateRelevantServices(NodeId id, bool has_relevant_flags)
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);

    void UpdateLoadedBloomFilter(NodeId id, bool bloom_filter_loaded)
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);

    void UpdateRelayTxs(NodeId id) EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);

    bool HasCandidate(NodeId id) const EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);
};

#endif // BITCOIN_NODE_EVICTION_IMPL_H
