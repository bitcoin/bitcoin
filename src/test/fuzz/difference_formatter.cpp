// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockencodings.h>
#include <streams.h>
#include <random.h>
#include <test/fuzz/fuzz.h>

#include <vector>

namespace {
void VerifyDifferenceFormatterInvariants(const std::vector<uint16_t>& indexes) {
    // Core invariant: strictly monotonic increasing (no duplicates allowed)
    for (size_t i = 1; i < indexes.size(); ++i) {
        assert(indexes[i] > indexes[i-1]); // Must be STRICTLY greater
    }
}
} // namespace

FUZZ_TARGET(difference_formatter)
{
    const auto block_hash = InsecureRandomContext{{}}.rand256();
    DataStream ss{};
    ss << block_hash << std::span{buffer};

    // Test deserialization
    try {
        BlockTransactionsRequest test_container;
        ss >> test_container;
        assert(test_container.blockhash == block_hash);

        // If deserialization succeeded, verify the DifferenceFormatter invariants
        VerifyDifferenceFormatterInvariants(test_container.indexes);

    } catch (const std::ios_base::failure&) {
        // Expected for malformed input
    }
}
