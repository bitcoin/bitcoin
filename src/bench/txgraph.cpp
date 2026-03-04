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

std::strong_ordering PointerComparator(const TxGraph::Ref& a, const TxGraph::Ref& b) noexcept
{
    return (&a) <=> (&b);
}

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
    /** Set a very high number for acceptable cost, so that we certainly benchmark optimal
     *  linearization. */
    static constexpr uint64_t HIGH_ACCEPTABLE_COST = 100'000'000;

    /** Refs to all top transactions. */
    std::vector<TxGraph::Ref> top_refs;
    /** Refs to all bottom transactions. */
    std::vector<TxGraph::Ref> bottom_refs;
    /** Indexes into top_refs for some transaction of each component, in arbitrary order.
     *  Initially these are the last transactions in each chains, but as bottom transactions are
     *  added, entries will be removed when they get merged, and randomized. */
    std::vector<size_t> top_components;

    InsecureRandomContext rng(11);
    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT, MAX_CLUSTER_SIZE, HIGH_ACCEPTABLE_COST, PointerComparator);

    // Construct the top chains.
    for (int chain = 0; chain < NUM_TOP_CHAINS; ++chain) {
        for (int chaintx = 0; chaintx < NUM_TX_PER_TOP_CHAIN; ++chaintx) {
            int64_t fee = rng.randbits<27>() + 100;
            FeePerWeight feerate{fee, 1};
            graph->AddTransaction(top_refs.emplace_back(), feerate);
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
        TxGraph::Ref bottom_tx;
        graph->AddTransaction(bottom_tx, feerate);
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

/** Benchmark: connect NUM_TX singleton clusters into a single chain via TryChainMerge.
 *
 * This exercises the ApplyDependencies hot path when all clusters are chain-compatible
 * (singleton -> singleton -> ... -> singleton) and the resulting topology is a linear chain.
 * All NUM_TX-1 deps are submitted at once before ApplyDependencies is triggered.
 *
 * Run once since the merge is irreversible. */
void BenchTxGraphChainMergeSingletons(benchmark::Bench& bench)
{
    /** Number of transactions (= max cluster count, the largest possible single chain). */
    static constexpr int NUM_TX = 64;
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000 * 100;
    static constexpr uint64_t HIGH_ACCEPTABLE_COST = 100'000'000;

    InsecureRandomContext rng(42);
    auto graph = MakeTxGraph(NUM_TX, MAX_CLUSTER_SIZE, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs(NUM_TX);
    for (int i = 0; i < NUM_TX; ++i) {
        int64_t fee = rng.randbits<27>() + 100;
        graph->AddTransaction(refs[i], FeePerWeight{fee, 100});
    }

    bench.epochIterations(1).epochs(1).run([&] {
        for (int i = 0; i < NUM_TX - 1; ++i) {
            graph->AddDependency(refs[i], refs[i + 1]);
        }
        // Trigger ApplyDependencies, which calls TryChainMerge once for all NUM_TX singletons.
        graph->GetBlockBuilder();
    });

    assert(graph->GetTransactionCount(TxGraph::Level::TOP) == NUM_TX);
}

/** Benchmark: connect NUM_CHAINS pre-built ChainClusters into a single large chain.
 *
 * Each chain has CHAIN_LEN transactions and is first materialized as a ChainCluster
 * via GetBlockBuilder(). Then NUM_CHAINS-1 tail->head deps connect them into one chain.
 * This exercises TryChainMerge when inputs are already ChainClusters (not singletons).
 *
 * Run once since the merge is irreversible.
 * NUM_CHAINS * CHAIN_LEN must not exceed max_cluster_count (64) to avoid oversized state. */
void BenchTxGraphChainMergeFromChains(benchmark::Bench& bench)
{
    static constexpr int NUM_CHAINS = 32;
    static constexpr int CHAIN_LEN = 2; // 32 chains of 2 txs = 64 txs total
    static constexpr int NUM_TX = NUM_CHAINS * CHAIN_LEN;
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000 * 100;
    static constexpr uint64_t HIGH_ACCEPTABLE_COST = 100'000'000;

    InsecureRandomContext rng(42);
    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT_LIMIT, MAX_CLUSTER_SIZE, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs(NUM_TX);
    for (int chain = 0; chain < NUM_CHAINS; ++chain) {
        for (int pos = 0; pos < CHAIN_LEN; ++pos) {
            int idx = chain * CHAIN_LEN + pos;
            int64_t fee = rng.randbits<27>() + 100;
            graph->AddTransaction(refs[idx], FeePerWeight{fee, 100});
            if (pos > 0) graph->AddDependency(refs[idx - 1], refs[idx]);
        }
    }
    // Materialize all chains as ChainClusters before the benchmark starts.
    graph->GetBlockBuilder();

    bench.epochIterations(1).epochs(1).run([&] {
        // Connect chain_i tail -> chain_{i+1} head for all consecutive chain pairs.
        for (int chain = 0; chain < NUM_CHAINS - 1; ++chain) {
            int tail = chain * CHAIN_LEN + (CHAIN_LEN - 1);
            int head = (chain + 1) * CHAIN_LEN;
            graph->AddDependency(refs[tail], refs[head]);
        }
        // Trigger ApplyDependencies, which calls TryChainMerge NUM_CHAINS-1 times,
        // each time merging two ChainClusters until one large chain remains.
        graph->GetBlockBuilder();
    });

    assert(graph->GetTransactionCount(TxGraph::Level::TOP) == NUM_TX);
}

/** Benchmark: repeated GetBlockBuilder calls on a single ChainCluster of NUM_TX transactions.
 *
 * Measures the cost of iterating the optimal linearization order for a chain cluster,
 * which is what the mempool does on every block template request.
 *
 * Repeatable: GetBlockBuilder does not modify the graph. */
void BenchTxGraphChainBlockBuilder(benchmark::Bench& bench)
{
    static constexpr int NUM_TX = 64;
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000 * 100;
    static constexpr uint64_t HIGH_ACCEPTABLE_COST = 100'000'000;

    InsecureRandomContext rng(42);
    auto graph = MakeTxGraph(NUM_TX, MAX_CLUSTER_SIZE, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs(NUM_TX);
    for (int i = 0; i < NUM_TX; ++i) {
        int64_t fee = rng.randbits<27>() + 100;
        graph->AddTransaction(refs[i], FeePerWeight{fee, 100});
        if (i > 0) graph->AddDependency(refs[i - 1], refs[i]);
    }
    // Build and materialize the ChainCluster before benchmarking.
    graph->GetBlockBuilder();

    bench.run([&] {
        auto builder = graph->GetBlockBuilder();
        while (auto chunk = builder->GetCurrentChunk()) {
            builder->Include();
        }
    });
}

} // namespace

static void TxGraphTrim(benchmark::Bench& bench) { BenchTxGraphTrim(bench); }
static void TxGraphChainMergeSingletons(benchmark::Bench& bench) { BenchTxGraphChainMergeSingletons(bench); }
static void TxGraphChainMergeFromChains(benchmark::Bench& bench) { BenchTxGraphChainMergeFromChains(bench); }
static void TxGraphChainBlockBuilder(benchmark::Bench& bench) { BenchTxGraphChainBlockBuilder(bench); }

BENCHMARK(TxGraphTrim);
BENCHMARK(TxGraphChainMergeSingletons);
BENCHMARK(TxGraphChainMergeFromChains);
BENCHMARK(TxGraphChainBlockBuilder);
