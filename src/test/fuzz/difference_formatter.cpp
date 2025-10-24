// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockencodings.h>
#include <streams.h>
#include <random.h>
#include <test/fuzz/fuzz.h>

#include <vector>

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

        // Invariant: strictly monotonic increasing (no duplicates allowed)
        for (size_t i = 1; i < test_container.indexes.size(); ++i) {
            assert(test_container.indexes[i] > test_container.indexes[i-1]);
        }

    } catch (const std::ios_base::failure&) {
        // Expected for malformed input
    }
}
