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

typedef int64_t NodeId;

struct NodeEvictionCandidate;

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

    std::optional<NodeEvictionCandidate> GetCandidate(NodeId id) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_candidates_mutex);
};

#endif // BITCOIN_NODE_EVICTION_IMPL_H
