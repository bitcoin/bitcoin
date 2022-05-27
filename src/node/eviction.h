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

/**
 * Select an inbound peer to evict after filtering out (protecting) peers having
 * distinct, difficult-to-forge characteristics. The protection logic picks out
 * fixed numbers of desirable peers per various criteria, followed by (mostly)
 * ratios of desirable or disadvantaged peers. If any eviction candidates
 * remain, the selection logic chooses a peer to evict.
 */
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

    std::optional<NodeEvictionCandidate> GetCandidate(NodeId id) const;

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
};

#endif // BITCOIN_NODE_EVICTION_H
