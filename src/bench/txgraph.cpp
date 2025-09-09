// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <random.h>
#include <txgraph.h>
#include <util/feefrac.h>

#include <cassert>
#include <cstdint>

namespace {

void BenchTxGraphTrim(benchmark::Bench& bench)
{
    // The from-block transactions consist of 1000 fully linear clusters, each with 64
    // transactions. The mempool contains 11 transactions that together merge all of these into
    // a single cluster.
    //
    // (1000 chains of 64 transactions, 64000 T's total)
    //
    //      T          T          T          T          T          T          T          T
    //      |          |          |          |          |          |          |          |
    //      T          T          T          T          T          T          T          T
    //      |          |          |          |          |          |          |          |
    //      T          T          T          T          T          T          T          T
    //      |          |          |          |          |          |          |          |
    //      T          T          T          T          T          T          T          T
    //  (64 long)  (64 long)  (64 long)  (64 long)  (64 long)  (64 long)  (64 long)  (64 long)
    //      |          |          |          |          |          |          |          |
    //      |          |         / \         |         / \         |          |         /
    //      \----------+--------/   \--------+--------/   \--------+-----+----+--------/
    //                 |                     |                           |
    //                 B                     B                           B
    //
    //  (11 B's, each attaching to up to 100 chains of 64 T's)
    //
    /** The maximum cluster count used in this test. */
    static constexpr int MAX_CLUSTER_COUNT = 64;
    /** The number of "top" (from-block) chains of transactions. */
    static constexpr int NUM_TOP_CHAINS = 1000;
    /** The number of transactions per top chain. */
    static constexpr int NUM_TX_PER_TOP_CHAIN = MAX_CLUSTER_COUNT;
    /** The (maximum) number of dependencies per bottom transaction. */
    static constexpr int NUM_DEPS_PER_BOTTOM_TX = 100;
    /** Set a very large cluster size limit so that only the count limit is triggered. */
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000 * 100;
    /** Set a very high number for acceptable iterations, so that we certainly benchmark optimal
     *  linearization. */
    static constexpr uint64_t NUM_ACCEPTABLE_ITERS = 100'000'000;

    /** Refs to all top transactions. */
    std::vector<TxGraph::Ref> top_refs;
    /** Refs to all bottom transactions. */
    std::vector<TxGraph::Ref> bottom_refs;
    /** Indexes into top_refs for some transaction of each component, in arbitrary order.
     *  Initially these are the last transactions in each chains, but as bottom transactions are
     *  added, entries will be removed when they get merged, and randomized. */
    std::vector<size_t> top_components;

    InsecureRandomContext rng(11);
    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT, MAX_CLUSTER_SIZE, NUM_ACCEPTABLE_ITERS);

    // Construct the top chains.
    for (int chain = 0; chain < NUM_TOP_CHAINS; ++chain) {
        for (int chaintx = 0; chaintx < NUM_TX_PER_TOP_CHAIN; ++chaintx) {
            int64_t fee = rng.randbits<27>() + 100;
            FeePerWeight feerate{fee, 1};
            top_refs.push_back(graph->AddTransaction(feerate));
            // Add internal dependencies linking the chain transactions together.
            if (chaintx > 0) {
                 graph->AddDependency(*(top_refs.rbegin()), *(top_refs.rbegin() + 1));
            }
        }
        // Remember the last transaction in each chain, to attach the bottom transactions to.
        top_components.push_back(top_refs.size() - 1);
    }

    // Make the graph linearize all clusters acceptably.
    graph->GetBlockBuilder();

    // Construct the bottom transactions, and dependencies to the top chains.
    while (top_components.size() > 1) {
        // Construct the transaction.
        int64_t fee = rng.randbits<27>() + 100;
        FeePerWeight feerate{fee, 1};
        auto bottom_tx = graph->AddTransaction(feerate);
        // Determine the number of dependencies this transaction will have.
        int deps = std::min<int>(NUM_DEPS_PER_BOTTOM_TX, top_components.size());
        for (int dep = 0; dep < deps; ++dep) {
            // Pick an transaction in top_components to attach to.
            auto idx = rng.randrange(top_components.size());
            // Add dependency.
            graph->AddDependency(/*parent=*/top_refs[top_components[idx]], /*child=*/bottom_tx);
            // Unless this is the last dependency being added, remove from top_components, as
            // the component will be merged with that one.
            if (dep < deps - 1) {
                // Move entry top the back.
                if (idx != top_components.size() - 1) std::swap(top_components.back(), top_components[idx]);
                // And pop it.
                top_components.pop_back();
            }
        }
        bottom_refs.push_back(std::move(bottom_tx));
    }

    // Run the benchmark exactly once. Running it multiple times would require the setup to be
    // redone, which takes a very non-negligible time compared to the trimming itself.
    bench.epochIterations(1).epochs(1).run([&] {
        // Call Trim() to remove transactions and bring the cluster back within limits.
        graph->Trim();
        // And relinearize everything that remains acceptably.
        graph->GetBlockBuilder();
    });

    assert(!graph->IsOversized(TxGraph::Level::TOP));
    // At least 99% of chains must survive.
    assert(graph->GetTransactionCount(TxGraph::Level::TOP) >= (NUM_TOP_CHAINS * NUM_TX_PER_TOP_CHAIN * 99) / 100);
}

} // namespace

static void TxGraphTrim(benchmark::Bench& bench) { BenchTxGraphTrim(bench); }

BENCHMARK(TxGraphTrim, benchmark::PriorityLevel::HIGH);
