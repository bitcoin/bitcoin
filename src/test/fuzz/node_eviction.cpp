// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>

FUZZ_TARGET(node_eviction)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<NodeEvictionCandidate> eviction_candidates;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        eviction_candidates.push_back({
            /* id */ fuzzed_data_provider.ConsumeIntegral<NodeId>(),
            /* nTimeConnected */ fuzzed_data_provider.ConsumeIntegral<int64_t>(),
            /* m_min_ping_time */ std::chrono::microseconds{fuzzed_data_provider.ConsumeIntegral<int64_t>()},
            /* nLastBlockTime */ fuzzed_data_provider.ConsumeIntegral<int64_t>(),
            /* nLastTXTime */ fuzzed_data_provider.ConsumeIntegral<int64_t>(),
            /* fRelevantServices */ fuzzed_data_provider.ConsumeBool(),
            /* fRelayTxes */ fuzzed_data_provider.ConsumeBool(),
            /* fBloomFilter */ fuzzed_data_provider.ConsumeBool(),
            /* nKeyedNetGroup */ fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
            /* prefer_evict */ fuzzed_data_provider.ConsumeBool(),
            /* m_is_local */ fuzzed_data_provider.ConsumeBool(),
            /* m_network */ fuzzed_data_provider.PickValueInArray(ALL_NETWORKS),
        });
    }
    // Make a copy since eviction_candidates may be in some valid but otherwise
    // indeterminate state after the SelectNodeToEvict(&&) call.
    const std::vector<NodeEvictionCandidate> eviction_candidates_copy = eviction_candidates;
    const std::optional<NodeId> node_to_evict = SelectNodeToEvict(std::move(eviction_candidates));
    if (node_to_evict) {
        assert(std::any_of(eviction_candidates_copy.begin(), eviction_candidates_copy.end(), [&node_to_evict](const NodeEvictionCandidate& eviction_candidate) { return *node_to_evict == eviction_candidate.id; }));
    }
}
