// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net.h>
#include <optional.h>
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
    while (fuzzed_data_provider.ConsumeBool()) {
        eviction_candidates.push_back({
            fuzzed_data_provider.ConsumeIntegral<NodeId>(),
            fuzzed_data_provider.ConsumeIntegral<int64_t>(),
            fuzzed_data_provider.ConsumeIntegral<int64_t>(),
            fuzzed_data_provider.ConsumeIntegral<int64_t>(),
            fuzzed_data_provider.ConsumeIntegral<int64_t>(),
            fuzzed_data_provider.ConsumeBool(),
            fuzzed_data_provider.ConsumeBool(),
            fuzzed_data_provider.ConsumeBool(),
            fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
            fuzzed_data_provider.ConsumeBool(),
            fuzzed_data_provider.ConsumeBool(),
        });
    }
    // Make a copy since eviction_candidates may be in some valid but otherwise
    // indeterminate state after the SelectNodeToEvict(&&) call.
    const std::vector<NodeEvictionCandidate> eviction_candidates_copy = eviction_candidates;
    const Optional<NodeId> node_to_evict = SelectNodeToEvict(std::move(eviction_candidates));
    if (node_to_evict) {
        assert(std::any_of(eviction_candidates_copy.begin(), eviction_candidates_copy.end(), [&node_to_evict](const NodeEvictionCandidate& eviction_candidate) { return *node_to_evict == eviction_candidate.id; }));
    }
}
