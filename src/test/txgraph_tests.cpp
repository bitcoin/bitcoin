// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <txgraph.h>

#include <random.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <vector>

BOOST_AUTO_TEST_SUITE(txgraph_tests)

/** The number used as acceptable_iters argument in these tests. High enough that everything
 *  should be optimal, always. */
static constexpr uint64_t NUM_ACCEPTABLE_ITERS = 100'000'000;

BOOST_AUTO_TEST_CASE(txgraph_trim_zigzag)
{
    // T     T     T     T     T     T     T     T     T     T     T     T     T     T (50 T's)
    //  \   / \   / \   / \   / \   / \   / \   / \   / \   / \   / \   / \   / \   /
    //   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /
    //    B     B     B     B     B     B     B     B     B     B     B     B     B    (49 B's)
    //
    /** The maximum cluster count used in this test. */
    static constexpr int MAX_CLUSTER_COUNT = 50;
    /** The number of "bottom" transactions, which are in the mempool already. */
    static constexpr int NUM_BOTTOM_TX = 49;
    /** The number of "top" transactions, which come from disconnected blocks. These are re-added
     *  to the mempool and, while connecting them to the already-in-mempool transactions, we
     *   discover the resulting cluster is oversized. */
    static constexpr int NUM_TOP_TX = 50;
    /** The total number of transactions in the test. */
    static constexpr int NUM_TOTAL_TX = NUM_BOTTOM_TX + NUM_TOP_TX;
    static_assert(NUM_TOTAL_TX > MAX_CLUSTER_COUNT);
    /** Set a very large cluster size limit so that only the count limit is triggered. */
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000 * 100;

    // Create a new graph for the test.
    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT, MAX_CLUSTER_SIZE, NUM_ACCEPTABLE_ITERS);

    // Add all transactions and store their Refs.
    std::vector<TxGraph::Ref> refs;
    refs.reserve(NUM_TOTAL_TX);
    // First all bottom transactions: the i'th bottom transaction is at position i.
    for (unsigned int i = 0; i < NUM_BOTTOM_TX; ++i) {
        refs.push_back(graph->AddTransaction(FeePerWeight{200 - i, 100}));
    }
    // Then all top transactions: the i'th top transaction is at position NUM_BOTTOM_TX + i.
    for (unsigned int i = 0; i < NUM_TOP_TX; ++i) {
        refs.push_back(graph->AddTransaction(FeePerWeight{100 - i, 100}));
    }

    // Create the zigzag dependency structure.
    // Each transaction in the bottom row depends on two adjacent transactions from the top row.
    graph->SanityCheck();
    for (unsigned int i = 0; i < NUM_BOTTOM_TX; ++i) {
        graph->AddDependency(/*parent=*/refs[NUM_BOTTOM_TX + i], /*child=*/refs[i]);
        graph->AddDependency(/*parent=*/refs[NUM_BOTTOM_TX + i + 1], /*child=*/refs[i]);
    }

    // Check that the graph is now oversized. This also forces the graph to
    // group clusters and compute the oversized status.
    graph->SanityCheck();
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), NUM_TOTAL_TX);
    BOOST_CHECK(graph->IsOversized(TxGraph::Level::TOP));

    // Call Trim() to remove transactions and bring the cluster back within limits.
    auto removed_refs = graph->Trim();
    graph->SanityCheck();
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::TOP));

    // We only need to trim the middle bottom transaction to end up with 2 clusters each within cluster limits.
    BOOST_CHECK_EQUAL(removed_refs.size(), 1);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), MAX_CLUSTER_COUNT * 2 - 2);
    for (unsigned int i = 0; i < refs.size(); ++i) {
        BOOST_CHECK_EQUAL(graph->Exists(refs[i], TxGraph::Level::TOP), i != (NUM_BOTTOM_TX / 2));
    }
}

BOOST_AUTO_TEST_CASE(txgraph_trim_flower)
{
    // We will build an oversized flower-shaped graph: all transactions are spent by 1 descendant.
    //
    //   T   T   T   T   T   T   T   T (100 T's)
    //   |   |   |   |   |   |   |   |
    //   |   |   |   |   |   |   |   |
    //   \---+---+---+-+-+---+---+---/
    //                 |
    //                 B (1 B)
    //
    /** The maximum cluster count used in this test. */
    static constexpr int MAX_CLUSTER_COUNT = 50;
    /** The number of "top" transactions, which come from disconnected blocks. These are re-added
     *  to the mempool and, connecting them to the already-in-mempool transactions, we discover the
     *  resulting cluster is oversized. */
    static constexpr int NUM_TOP_TX = MAX_CLUSTER_COUNT * 2;
    /** The total number of transactions in this test. */
    static constexpr int NUM_TOTAL_TX = NUM_TOP_TX + 1;
    /** Set a very large cluster size limit so that only the count limit is triggered. */
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000 * 100;

    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT, MAX_CLUSTER_SIZE, NUM_ACCEPTABLE_ITERS);

    // Add all transactions and store their Refs.
    std::vector<TxGraph::Ref> refs;
    refs.reserve(NUM_TOTAL_TX);

    // Add all transactions. They are in individual clusters.
    refs.push_back(graph->AddTransaction({1, 100}));
    for (unsigned int i = 0; i < NUM_TOP_TX; ++i) {
        refs.push_back(graph->AddTransaction(FeePerWeight{500 + i, 100}));
    }
    graph->SanityCheck();

    // The 0th transaction spends all the top transactions.
    for (unsigned int i = 1; i < NUM_TOTAL_TX; ++i) {
        graph->AddDependency(/*parent=*/refs[i], /*child=*/refs[0]);
    }
    graph->SanityCheck();

    // Check that the graph is now oversized. This also forces the graph to
    // group clusters and compute the oversized status.
    BOOST_CHECK(graph->IsOversized(TxGraph::Level::TOP));

    // Call Trim() to remove transactions and bring the cluster back within limits.
    auto removed_refs = graph->Trim();
    graph->SanityCheck();
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::TOP));

    // Since only the bottom transaction connects these clusters, we only need to remove it.
    BOOST_CHECK_EQUAL(removed_refs.size(), 1);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), MAX_CLUSTER_COUNT * 2);
    BOOST_CHECK(!graph->Exists(refs[0], TxGraph::Level::TOP));
    for (unsigned int i = 1; i < refs.size(); ++i) {
        BOOST_CHECK(graph->Exists(refs[i], TxGraph::Level::TOP));
    }
}

BOOST_AUTO_TEST_CASE(txgraph_trim_huge)
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
    /** The number of bottom transactions that are expected to be created. */
    static constexpr int NUM_BOTTOM_TX = (NUM_TOP_CHAINS - 1 + (NUM_DEPS_PER_BOTTOM_TX - 2)) / (NUM_DEPS_PER_BOTTOM_TX - 1);
    /** The total number of transactions created in this test. */
    static constexpr int NUM_TOTAL_TX = NUM_TOP_CHAINS * NUM_TX_PER_TOP_CHAIN + NUM_BOTTOM_TX;
    /** Set a very large cluster size limit so that only the count limit is triggered. */
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000 * 100;

    /** Refs to all top transactions. */
    std::vector<TxGraph::Ref> top_refs;
    /** Refs to all bottom transactions. */
    std::vector<TxGraph::Ref> bottom_refs;
    /** Indexes into top_refs for some transaction of each component, in arbitrary order.
     *  Initially these are the last transactions in each chains, but as bottom transactions are
     *  added, entries will be removed when they get merged, and randomized. */
    std::vector<size_t> top_components;

    FastRandomContext rng;
    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT, MAX_CLUSTER_SIZE, NUM_ACCEPTABLE_ITERS);

    // Construct the top chains.
    for (int chain = 0; chain < NUM_TOP_CHAINS; ++chain) {
        for (int chaintx = 0; chaintx < NUM_TX_PER_TOP_CHAIN; ++chaintx) {
            // Use random fees, size 1.
            int64_t fee = rng.randbits<27>() + 100;
            FeePerWeight feerate{fee, 1};
            top_refs.push_back(graph->AddTransaction(feerate));
            // Add internal dependencies linked the chain transactions together.
            if (chaintx > 0) {
                 graph->AddDependency(*(top_refs.rbegin()), *(top_refs.rbegin() + 1));
            }
        }
        // Remember the last transaction in each chain, to attach the bottom transactions to.
        top_components.push_back(top_refs.size() - 1);
    }
    graph->SanityCheck();

    // Not oversized so far (just 1000 clusters of 64).
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::TOP));

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
    graph->SanityCheck();

    // Now we are oversized (one cluster of 64011).
    BOOST_CHECK(graph->IsOversized(TxGraph::Level::TOP));
    const auto total_tx_count = graph->GetTransactionCount(TxGraph::Level::TOP);
    BOOST_CHECK(total_tx_count == top_refs.size() + bottom_refs.size());
    BOOST_CHECK(total_tx_count == NUM_TOTAL_TX);

    // Call Trim() to remove transactions and bring the cluster back within limits.
    auto removed_refs = graph->Trim();
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::TOP));
    BOOST_CHECK(removed_refs.size() == total_tx_count - graph->GetTransactionCount(TxGraph::Level::TOP));
    graph->SanityCheck();

    // At least 99% of chains must survive.
    BOOST_CHECK(graph->GetTransactionCount(TxGraph::Level::TOP) >= (NUM_TOP_CHAINS * NUM_TX_PER_TOP_CHAIN * 99) / 100);
}

BOOST_AUTO_TEST_CASE(txgraph_trim_big_singletons)
{
    // Mempool consists of 100 singleton clusters; there are no dependencies. Some are oversized. Trim() should remove all of the oversized ones.
    static constexpr int MAX_CLUSTER_COUNT = 64;
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000;
    static constexpr int NUM_TOTAL_TX = 100;

    // Create a new graph for the test.
    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT, MAX_CLUSTER_SIZE, NUM_ACCEPTABLE_ITERS);

    // Add all transactions and store their Refs.
    std::vector<TxGraph::Ref> refs;
    refs.reserve(NUM_TOTAL_TX);

    // Add all transactions. They are in individual clusters.
    for (unsigned int i = 0; i < NUM_TOTAL_TX; ++i) {
        // The 88th transaction is oversized.
        // Every 20th transaction is oversized.
        const FeePerWeight feerate{500 + i, (i == 88 || i % 20 == 0) ? MAX_CLUSTER_SIZE + 1 : 100};
        refs.push_back(graph->AddTransaction(feerate));
    }
    graph->SanityCheck();

    // Check that the graph is now oversized. This also forces the graph to
    // group clusters and compute the oversized status.
    BOOST_CHECK(graph->IsOversized(TxGraph::Level::TOP));

    // Call Trim() to remove transactions and bring the cluster back within limits.
    auto removed_refs = graph->Trim();
    graph->SanityCheck();
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), NUM_TOTAL_TX - 6);
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::TOP));

    // Check that all the oversized transactions were removed.
    for (unsigned int i = 0; i < refs.size(); ++i) {
        BOOST_CHECK_EQUAL(graph->Exists(refs[i], TxGraph::Level::TOP), i != 88 && i % 20 != 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
