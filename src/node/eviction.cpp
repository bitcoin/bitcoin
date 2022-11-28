// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/eviction.h>
#include <node/eviction_impl.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

using namespace std::chrono_literals;

static bool ReverseCompareNodeMinPingTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    return a.m_min_ping_time > b.m_min_ping_time;
}

static bool ReverseCompareNodeTimeConnected(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    return a.m_connected > b.m_connected;
}

static bool CompareNetGroupKeyed(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b) {
    return a.nKeyedNetGroup < b.nKeyedNetGroup;
}

static bool CompareNodeBlockTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    // There is a fall-through here because it is common for a node to have many peers which have not yet relayed a block.
    if (a.m_last_block_time != b.m_last_block_time) return a.m_last_block_time < b.m_last_block_time;
    if (a.fRelevantServices != b.fRelevantServices) return b.fRelevantServices;
    return a.m_connected > b.m_connected;
}

static bool CompareNodeTXTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    // There is a fall-through here because it is common for a node to have more than a few peers that have not yet relayed txn.
    if (a.m_last_tx_time != b.m_last_tx_time) return a.m_last_tx_time < b.m_last_tx_time;
    if (a.m_relay_txs != b.m_relay_txs) return b.m_relay_txs;
    if (a.fBloomFilter != b.fBloomFilter) return a.fBloomFilter;
    return a.m_connected > b.m_connected;
}

// Pick out the potential block-relay only peers, and sort them by last block time.
static bool CompareNodeBlockRelayOnlyTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    if (a.m_relay_txs != b.m_relay_txs) return a.m_relay_txs;
    if (a.m_last_block_time != b.m_last_block_time) return a.m_last_block_time < b.m_last_block_time;
    if (a.fRelevantServices != b.fRelevantServices) return b.fRelevantServices;
    return a.m_connected > b.m_connected;
}

/**
 * Sort eviction candidates by network/localhost and connection uptime.
 * Candidates near the beginning are more likely to be evicted, and those
 * near the end are more likely to be protected, e.g. less likely to be evicted.
 * - First, nodes that are not `is_local` and that do not belong to `network`,
 *   sorted by increasing uptime (from most recently connected to connected longer).
 * - Then, nodes that are `is_local` or belong to `network`, sorted by increasing uptime.
 */
struct CompareNodeNetworkTime {
    const bool m_is_local;
    const Network m_network;
    CompareNodeNetworkTime(bool is_local, Network network) : m_is_local(is_local), m_network(network) {}
    bool operator()(const NodeEvictionCandidate& a, const NodeEvictionCandidate& b) const
    {
        if (m_is_local && a.m_is_local != b.m_is_local) return b.m_is_local;
        if ((a.m_network == m_network) != (b.m_network == m_network)) return b.m_network == m_network;
        return a.m_connected > b.m_connected;
    };
};

//! Sort an array by the specified comparator, then erase the last K elements where predicate is true.
template <typename T, typename Comparator>
static void EraseLastKElements(
    std::vector<T>& elements, Comparator comparator, size_t k,
    std::function<bool(const NodeEvictionCandidate&)> predicate = [](const NodeEvictionCandidate& n) { return true; })
{
    std::sort(elements.begin(), elements.end(), comparator);
    size_t eraseSize = std::min(k, elements.size());
    elements.erase(std::remove_if(elements.end() - eraseSize, elements.end(), predicate), elements.end());
}

void ProtectNoBanConnections(std::vector<NodeEvictionCandidate>& eviction_candidates)
{
    eviction_candidates.erase(std::remove_if(eviction_candidates.begin(), eviction_candidates.end(),
                                             [](NodeEvictionCandidate const& n) {
                                                 return n.m_noban;
                                             }),
                              eviction_candidates.end());
}

void ProtectOutboundConnections(std::vector<NodeEvictionCandidate>& eviction_candidates)
{
    eviction_candidates.erase(std::remove_if(eviction_candidates.begin(), eviction_candidates.end(),
                                             [](NodeEvictionCandidate const& n) {
                                                 return n.m_conn_type != ConnectionType::INBOUND;
                                             }),
                              eviction_candidates.end());
}

void ProtectEvictionCandidatesByRatio(std::vector<NodeEvictionCandidate>& eviction_candidates)
{
    // Protect the half of the remaining nodes which have been connected the longest.
    // This replicates the non-eviction implicit behavior, and precludes attacks that start later.
    // To favorise the diversity of our peer connections, reserve up to half of these protected
    // spots for Tor/onion, localhost, I2P, and CJDNS peers, even if they're not longest uptime
    // overall. This helps protect these higher-latency peers that tend to be otherwise
    // disadvantaged under our eviction criteria.
    const size_t initial_size = eviction_candidates.size();
    const size_t total_protect_size{initial_size / 2};

    // Disadvantaged networks to protect. In the case of equal counts, earlier array members
    // have the first opportunity to recover unused slots from the previous iteration.
    struct Net { bool is_local; Network id; size_t count; };
    std::array<Net, 4> networks{
        {{false, NET_CJDNS, 0}, {false, NET_I2P, 0}, {/*localhost=*/true, NET_MAX, 0}, {false, NET_ONION, 0}}};

    // Count and store the number of eviction candidates per network.
    for (Net& n : networks) {
        n.count = std::count_if(eviction_candidates.cbegin(), eviction_candidates.cend(),
                                [&n](const NodeEvictionCandidate& c) {
                                    return n.is_local ? c.m_is_local : c.m_network == n.id;
                                });
    }
    // Sort `networks` by ascending candidate count, to give networks having fewer candidates
    // the first opportunity to recover unused protected slots from the previous iteration.
    std::stable_sort(networks.begin(), networks.end(), [](Net a, Net b) { return a.count < b.count; });

    // Protect up to 25% of the eviction candidates by disadvantaged network.
    const size_t max_protect_by_network{total_protect_size / 2};
    size_t num_protected{0};

    while (num_protected < max_protect_by_network) {
        // Count the number of disadvantaged networks from which we have peers to protect.
        auto num_networks = std::count_if(networks.begin(), networks.end(), [](const Net& n) { return n.count; });
        if (num_networks == 0) {
            break;
        }
        const size_t disadvantaged_to_protect{max_protect_by_network - num_protected};
        const size_t protect_per_network{std::max(disadvantaged_to_protect / num_networks, static_cast<size_t>(1))};
        // Early exit flag if there are no remaining candidates by disadvantaged network.
        bool protected_at_least_one{false};

        for (Net& n : networks) {
            if (n.count == 0) continue;
            const size_t before = eviction_candidates.size();
            EraseLastKElements(eviction_candidates, CompareNodeNetworkTime(n.is_local, n.id),
                               protect_per_network, [&n](const NodeEvictionCandidate& c) {
                                   return n.is_local ? c.m_is_local : c.m_network == n.id;
                               });
            const size_t after = eviction_candidates.size();
            if (before > after) {
                protected_at_least_one = true;
                const size_t delta{before - after};
                num_protected += delta;
                if (num_protected >= max_protect_by_network) {
                    break;
                }
                n.count -= delta;
            }
        }
        if (!protected_at_least_one) {
            break;
        }
    }

    // Calculate how many we removed, and update our total number of peers that
    // we want to protect based on uptime accordingly.
    assert(num_protected == initial_size - eviction_candidates.size());
    const size_t remaining_to_protect{total_protect_size - num_protected};
    EraseLastKElements(eviction_candidates, ReverseCompareNodeTimeConnected, remaining_to_protect);
}

[[nodiscard]] std::optional<NodeId> SelectNodeToEvict(std::vector<NodeEvictionCandidate>&& vEvictionCandidates)
{
    // Protect connections with certain characteristics

    ProtectNoBanConnections(vEvictionCandidates);

    ProtectOutboundConnections(vEvictionCandidates);

    // Deterministically select 4 peers to protect by netgroup.
    // An attacker cannot predict which netgroups will be protected
    EraseLastKElements(vEvictionCandidates, CompareNetGroupKeyed, 4);
    // Protect the 8 nodes with the lowest minimum ping time.
    // An attacker cannot manipulate this metric without physically moving nodes closer to the target.
    EraseLastKElements(vEvictionCandidates, ReverseCompareNodeMinPingTime, 8);
    // Protect 4 nodes that most recently sent us novel transactions accepted into our mempool.
    // An attacker cannot manipulate this metric without performing useful work.
    EraseLastKElements(vEvictionCandidates, CompareNodeTXTime, 4);
    // Protect up to 8 non-tx-relay peers that have sent us novel blocks.
    EraseLastKElements(vEvictionCandidates, CompareNodeBlockRelayOnlyTime, 8,
                       [](const NodeEvictionCandidate& n) { return !n.m_relay_txs && n.fRelevantServices; });

    // Protect 4 nodes that most recently sent us novel blocks.
    // An attacker cannot manipulate this metric without performing useful work.
    EraseLastKElements(vEvictionCandidates, CompareNodeBlockTime, 4);

    // Protect some of the remaining eviction candidates by ratios of desirable
    // or disadvantaged characteristics.
    ProtectEvictionCandidatesByRatio(vEvictionCandidates);

    if (vEvictionCandidates.empty()) return std::nullopt;

    // If any remaining peers are preferred for eviction consider only them.
    // This happens after the other preferences since if a peer is really the best by other criteria (esp relaying blocks)
    //  then we probably don't want to evict it no matter what.
    if (std::any_of(vEvictionCandidates.begin(),vEvictionCandidates.end(),[](NodeEvictionCandidate const &n){return n.prefer_evict;})) {
        vEvictionCandidates.erase(std::remove_if(vEvictionCandidates.begin(),vEvictionCandidates.end(),
                                  [](NodeEvictionCandidate const &n){return !n.prefer_evict;}),vEvictionCandidates.end());
    }

    // Identify the network group with the most connections and youngest member.
    // (vEvictionCandidates is already sorted by reverse connect time)
    uint64_t naMostConnections;
    unsigned int nMostConnections = 0;
    std::chrono::seconds nMostConnectionsTime{0};
    std::map<uint64_t, std::vector<NodeEvictionCandidate> > mapNetGroupNodes;
    for (const NodeEvictionCandidate &node : vEvictionCandidates) {
        std::vector<NodeEvictionCandidate> &group = mapNetGroupNodes[node.nKeyedNetGroup];
        group.push_back(node);
        const auto grouptime{group[0].m_connected};

        if (group.size() > nMostConnections || (group.size() == nMostConnections && grouptime > nMostConnectionsTime)) {
            nMostConnections = group.size();
            nMostConnectionsTime = grouptime;
            naMostConnections = node.nKeyedNetGroup;
        }
    }

    // Reduce to the network group with the most connections
    vEvictionCandidates = std::move(mapNetGroupNodes[naMostConnections]);

    // Disconnect from the network group with the most connections
    return vEvictionCandidates.front().id;
}

EvictionManagerImpl::EvictionManagerImpl() {}
EvictionManagerImpl::~EvictionManagerImpl() = default;

void EvictionManagerImpl::AddCandidate(NodeId id, std::chrono::seconds connected,
                                       uint64_t keyed_net_group, bool prefer_evict,
                                       bool is_local, Network network,
                                       bool noban, ConnectionType conn_type)
{
    LOCK(m_candidates_mutex);
    // The default values used to initialise the eviction candidates result in
    // newer candidates being more likely to be evicted.
    NodeEvictionCandidate candidate{
        .id = id,
        .m_connected = connected,
        // Initial value corresponds to "the peer has never sent a pong
        // response".
        .m_min_ping_time = std::chrono::microseconds::max(),
        // Initial values correspond to "the peer has never sent us a block or
        // transaction".
        .m_last_block_time = 0s,
        .m_last_tx_time = 0s,
        // We only learn if the peer advertises relevant services during the
        // version handshake.
        .fRelevantServices = false,
        // Whether we relay transactions to this peer is determined later
        // (during or after the version handshake).
        .m_relay_txs = false,
        // At this point the peer has not sent us a bloom filter.
        .fBloomFilter = false,
        .nKeyedNetGroup = keyed_net_group,
        .prefer_evict = prefer_evict,
        .m_is_local = is_local,
        .m_network = network,
        .m_noban = noban,
        .m_conn_type = conn_type,
    };
    m_candidates.emplace_hint(m_candidates.end(), id, std::move(candidate));
}

bool EvictionManagerImpl::RemoveCandidate(NodeId id)
{
    LOCK(m_candidates_mutex);
    return m_candidates.erase(id) != 0;
}

std::optional<NodeId> EvictionManagerImpl::SelectNodeToEvict() const
{
    std::vector<NodeEvictionCandidate> candidates;
    {
        LOCK(m_candidates_mutex);
        for (const auto& [id, candidate] : m_candidates) {
            candidates.push_back(candidate);
        }
    }
    return ::SelectNodeToEvict(std::move(candidates));
}

void EvictionManagerImpl::UpdateMinPingTime(NodeId id, std::chrono::microseconds ping_time)
{
    LOCK(m_candidates_mutex);
    if (const auto& it = m_candidates.find(id); it != m_candidates.end()) {
        it->second.m_min_ping_time = std::min(it->second.m_min_ping_time, ping_time);
    }
}

std::optional<std::chrono::microseconds> EvictionManagerImpl::GetMinPingTime(NodeId id) const
{
    LOCK(m_candidates_mutex);
    if (const auto& it = m_candidates.find(id); it != m_candidates.end()) {
        return {it->second.m_min_ping_time};
    }
    return {};
}

void EvictionManagerImpl::UpdateLastBlockTime(NodeId id, std::chrono::seconds block_time)
{
    LOCK(m_candidates_mutex);
    if (const auto& it = m_candidates.find(id); it != m_candidates.end()) {
        it->second.m_last_block_time = block_time;
    }
}

std::optional<std::chrono::seconds> EvictionManagerImpl::GetLastBlockTime(NodeId id) const
{
    LOCK(m_candidates_mutex);
    if (const auto& it = m_candidates.find(id); it != m_candidates.end()) {
        return {it->second.m_last_block_time};
    }
    return {};
}

void EvictionManagerImpl::UpdateLastTxTime(NodeId id, std::chrono::seconds tx_time)
{
    LOCK(m_candidates_mutex);
    if (const auto& it = m_candidates.find(id); it != m_candidates.end()) {
        it->second.m_last_tx_time = tx_time;
    }
}

std::optional<std::chrono::seconds> EvictionManagerImpl::GetLastTxTime(NodeId id) const
{
    LOCK(m_candidates_mutex);
    if (const auto& it = m_candidates.find(id); it != m_candidates.end()) {
        return {it->second.m_last_tx_time};
    }
    return {};
}

void EvictionManagerImpl::UpdateRelevantServices(NodeId id, bool has_relevant_flags)
{
    LOCK(m_candidates_mutex);
    if (const auto& it = m_candidates.find(id); it != m_candidates.end()) {
        it->second.fRelevantServices = has_relevant_flags;
    }
}

void EvictionManagerImpl::UpdateLoadedBloomFilter(NodeId id, bool bloom_filter_loaded)
{
    LOCK(m_candidates_mutex);
    if (const auto& it = m_candidates.find(id); it != m_candidates.end()) {
        it->second.fBloomFilter = bloom_filter_loaded;
    }
}

void EvictionManagerImpl::UpdateRelayTxs(NodeId id)
{
    LOCK(m_candidates_mutex);
    if (const auto& it = m_candidates.find(id); it != m_candidates.end()) {
        it->second.m_relay_txs = true;
    }
}

bool EvictionManagerImpl::HasCandidate(NodeId id) const
{
    LOCK(m_candidates_mutex);
    if (const auto& it = m_candidates.find(id); it != m_candidates.end()) {
        return true;
    }

    return false;
}

EvictionManager::EvictionManager()
    : m_impl(std::make_unique<EvictionManagerImpl>()) {}
EvictionManager::~EvictionManager() = default;

void EvictionManager::AddCandidate(NodeId id, std::chrono::seconds connected,
                                   uint64_t keyed_net_group, bool prefer_evict,
                                   bool is_local, Network network,
                                   bool noban, ConnectionType conn_type)
{
    m_impl->AddCandidate(id, connected, keyed_net_group, prefer_evict,
                         is_local, network, noban, conn_type);
}

bool EvictionManager::RemoveCandidate(NodeId id)
{
    return m_impl->RemoveCandidate(id);
}

std::optional<NodeId> EvictionManager::SelectNodeToEvict() const
{
    return m_impl->SelectNodeToEvict();
}

void EvictionManager::UpdateMinPingTime(NodeId id, std::chrono::microseconds ping_time)
{
    m_impl->UpdateMinPingTime(id, ping_time);
}

std::optional<std::chrono::microseconds> EvictionManager::GetMinPingTime(NodeId id) const
{
    return m_impl->GetMinPingTime(id);
}

void EvictionManager::UpdateLastBlockTime(NodeId id, std::chrono::seconds block_time)
{
    m_impl->UpdateLastBlockTime(id, block_time);
}

std::optional<std::chrono::seconds> EvictionManager::GetLastBlockTime(NodeId id) const
{
    return m_impl->GetLastBlockTime(id);
}

void EvictionManager::UpdateLastTxTime(NodeId id, std::chrono::seconds tx_time)
{
    m_impl->UpdateLastTxTime(id, tx_time);
}

std::optional<std::chrono::seconds> EvictionManager::GetLastTxTime(NodeId id) const
{
    return m_impl->GetLastTxTime(id);
}

void EvictionManager::UpdateRelevantServices(NodeId id, bool has_relevant_flags)
{
    m_impl->UpdateRelevantServices(id, has_relevant_flags);
}

void EvictionManager::UpdateLoadedBloomFilter(NodeId id, bool bloom_filter_loaded)
{
    m_impl->UpdateLoadedBloomFilter(id, bloom_filter_loaded);
}

void EvictionManager::UpdateRelayTxs(NodeId id)
{
    m_impl->UpdateRelayTxs(id);
}

bool EvictionManager::HasCandidate(NodeId id) const
{
    return m_impl->HasCandidate(id);
}
