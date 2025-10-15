// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <node/blockstorage.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>

namespace {

struct OldCBlockIndexWorkComparator {
    bool operator()(const CBlockIndex* pa, const CBlockIndex* pb) const
    {
        // First sort by most total work, ...
        if (pa->nChainWork > pb->nChainWork) return false;
        if (pa->nChainWork < pb->nChainWork) return true;

        // ... then by earliest activatable time, ...
        if (pa->nSequenceId < pb->nSequenceId) return false;
        if (pa->nSequenceId > pb->nSequenceId) return true;

        // Use pointer address as tie breaker (should only happen with blocks
        // loaded from disk, as those share the same id: 0 for blocks on the
        // best chain, 1 for all others).
        if (pa < pb) return false;
        if (pa > pb) return true;

        // Identical blocks.
        return false;
    }
};

} // namespace

FUZZ_TARGET(block_index_work_comparator)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    std::array<CBlockIndex, 2> storage{}; // defined pointer ordering
    CBlockIndex* a{&storage[0]}, *b{&storage[1]};

    // Generate random chain work and sequence IDs
    a->nChainWork = ConsumeArithUInt256(provider);
    a->nSequenceId = provider.ConsumeIntegral<int32_t>();
    // Add some duplicates to test self-equality and pointer tie break
    if (provider.ConsumeBool()) {
        b = a;
    } else {
        // Exercise both equal and unequal values.
        b->nChainWork = provider.ConsumeBool() ? a->nChainWork : ConsumeArithUInt256(provider);
        b->nSequenceId = provider.ConsumeBool() ? a->nSequenceId : provider.ConsumeIntegral<int32_t>();
    }

    constexpr node::CBlockIndexWorkComparator comparator;
    assert(OldCBlockIndexWorkComparator{}(a, b) == comparator(a, b));
    assert(OldCBlockIndexWorkComparator{}(b, a) == comparator(b, a));
}
