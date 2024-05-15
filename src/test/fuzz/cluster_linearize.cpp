// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cluster_linearize.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/util/cluster_linearize.h>
#include <util/bitset.h>
#include <util/feefrac.h>

#include <algorithm>
#include <stdint.h>
#include <vector>
#include <utility>

using namespace cluster_linearize;

namespace {


/** A simple finder class for candidate sets.
 *
 * This class matches SearchCandidateFinder in interface and behavior, though with fewer
 * optimizations.
 */
template<typename SetType>
class SimpleCandidateFinder
{
    /** Internal dependency graph. */
    const DepGraph<SetType>& m_depgraph;
    /** Which transaction are left to include. */
    SetType m_todo;

public:
    /** Construct an SimpleCandidateFinder for a given graph. */
    SimpleCandidateFinder(const DepGraph<SetType>& depgraph LIFETIMEBOUND) noexcept :
        m_depgraph(depgraph), m_todo{SetType::Fill(depgraph.TxCount())} {}

    /** Remove a set of transactions from the set of to-be-linearized ones. */
    void MarkDone(SetType select) noexcept { m_todo -= select; }

    /** Find a candidate set using at most max_iterations iterations, and the number of iterations
     *  actually performed. If that number is less than max_iterations, then the result is optimal.
     */
    std::pair<SetInfo<SetType>, uint64_t> FindCandidateSet(uint64_t max_iterations) const noexcept
    {
        uint64_t iterations_left = max_iterations;
        // Queue of work units. Each consists of:
        // - inc: set of transactions definitely included
        // - und: set of transactions that can be added to inc still
        std::vector<std::pair<SetType, SetType>> queue;
        // Initially we have just one queue element, with the entire graph in und.
        queue.emplace_back(SetType{}, m_todo);
        // Best solution so far.
        SetInfo best(m_depgraph, m_todo);
        // Process the queue.
        while (!queue.empty() && iterations_left) {
            --iterations_left;
            // Pop top element of the queue.
            auto [inc, und] = queue.back();
            queue.pop_back();
            // Look for a transaction to consider adding/removing.
            bool inc_none = inc.None();
            for (auto split : und) {
                // If inc is empty, consider any split transaction. Otherwise only consider
                // transactions that share ancestry with inc so far (which means only connected
                // sets will be considered).
                if (inc_none || inc.Overlaps(m_depgraph.Ancestors(split))) {
                    // Add a queue entry with split included.
                    SetInfo new_inc(m_depgraph, inc | (m_todo & m_depgraph.Ancestors(split)));
                    queue.emplace_back(new_inc.transactions, und - new_inc.transactions);
                    // Add a queue entry with split excluded.
                    queue.emplace_back(inc, und - m_depgraph.Descendants(split));
                    // Update statistics to account for the candidate new_inc.
                    if (new_inc.feerate > best.feerate) best = new_inc;
                    break;
                }
            }
        }
        return {std::move(best), max_iterations - iterations_left};
    }
};

/** A very simple finder class for optimal candidate sets, which tries every subset.
 *
 * It is even simpler than SimpleCandidateFinder, and is primarily included here to test the
 * correctness of SimpleCandidateFinder, which is then used to test the correctness of
 * SearchCandidateFinder.
 */
template<typename SetType>
class ExhaustiveCandidateFinder
{
    /** Internal dependency graph. */
    const DepGraph<SetType>& m_depgraph;
    /** Which transaction are left to include. */
    SetType m_todo;

public:
    /** Construct an ExhaustiveCandidateFinder for a given graph. */
    ExhaustiveCandidateFinder(const DepGraph<SetType>& depgraph LIFETIMEBOUND) noexcept :
        m_depgraph(depgraph), m_todo{SetType::Fill(depgraph.TxCount())} {}

    /** Remove a set of transactions from the set of to-be-linearized ones. */
    void MarkDone(SetType select) noexcept { m_todo -= select; }

    /** Find the optimal remaining candidate set. */
    SetInfo<SetType> FindCandidateSet() const noexcept
    {
        // Best solution so far.
        SetInfo<SetType> best{m_todo, m_depgraph.FeeRate(m_todo)};
        // The number of combinations to try.
        uint64_t limit = (uint64_t{1} << m_todo.Count()) - 1;
        // Try the transitive closure of every non-empty subset of m_todo.
        for (uint64_t x = 1; x < limit; ++x) {
            // If bit number b is set in x, then the remaining ancestors of the b'th remaining
            // transaction in m_todo are included.
            SetType txn;
            auto x_shifted{x};
            for (auto i : m_todo) {
                if (x_shifted & 1) txn |= m_depgraph.Ancestors(i);
                x_shifted >>= 1;
            }
            SetInfo cur(m_depgraph, txn & m_todo);
            if (cur.feerate > best.feerate) best = cur;
        }
        return best;
    }
};

/** A simple linearization algorithm.
 *
 * This matches Linearize() in interface and behavior, though with fewer optimizations, lacking
 * the ability to pass in an existing linearization, and using just SimpleCandidateFinder rather
 * than AncestorCandidateFinder and SearchCandidateFinder.
 */
template<typename SetType>
std::pair<std::vector<ClusterIndex>, bool> SimpleLinearize(const DepGraph<SetType>& depgraph, uint64_t max_iterations)
{
    std::vector<ClusterIndex> linearization;
    SimpleCandidateFinder finder(depgraph);
    SetType todo = SetType::Fill(depgraph.TxCount());
    bool optimal = true;
    while (todo.Any()) {
        auto [candidate, iterations_done] = finder.FindCandidateSet(max_iterations);
        if (iterations_done == max_iterations) optimal = false;
        depgraph.AppendTopo(linearization, candidate.transactions);
        todo -= candidate.transactions;
        finder.MarkDone(candidate.transactions);
        max_iterations -= iterations_done;
    }
    return {std::move(linearization), optimal};
}

/** Stitch connected components together in a DepGraph, guaranteeing its corresponding cluster is connected. */
template<typename BS>
void MakeConnected(DepGraph<BS>& depgraph)
{
    auto todo = BS::Fill(depgraph.TxCount());
    auto comp = depgraph.FindConnectedComponent(todo);
    todo -= comp;
    while (todo.Any()) {
        auto nextcomp = depgraph.FindConnectedComponent(todo);
        depgraph.AddDependency(comp.Last(), nextcomp.First());
        todo -= nextcomp;
        comp = nextcomp;
    }
}

/** Given a dependency graph, and a todo set, read a topological subset of todo from reader. */
template<typename SetType>
SetType ReadTopologicalSet(const DepGraph<SetType>& depgraph, const SetType& todo, SpanReader& reader)
{
    uint64_t mask{0};
    try {
        reader >> VARINT(mask);
    } catch(const std::ios_base::failure&) {}
    SetType ret;
    for (auto i : todo) {
        if (!ret[i]) {
            if (mask & 1) ret |= depgraph.Ancestors(i);
            mask >>= 1;
        }
    }
    return ret & todo;
}

/** Compute the chunks for a given linearization. */
template<typename SetType>
std::vector<FeeFrac> ChunkLinearization(const DepGraph<SetType>& depgraph, Span<const ClusterIndex> linearization) noexcept
{
    std::vector<FeeFrac> ret;
    for (ClusterIndex i : linearization) {
        /** The new chunk to be added, initially a singleton. */
        auto new_chunk = depgraph.FeeRate(i);
        // As long as the new chunk has a higher feerate than the last chunk so far, absorb it.
        while (!ret.empty() && new_chunk >> ret.back()) {
            new_chunk += ret.back();
            ret.pop_back();
        }
        // Actually move that new chunk into the chunking.
        ret.push_back(std::move(new_chunk));
    }
    return ret;
}

/** Given a dependency graph, construct any valid linearization for it, reading from a SpanReader. */
template<typename BS>
std::vector<ClusterIndex> ReadLinearization(const DepGraph<BS>& depgraph, SpanReader& reader)
{
    std::vector<ClusterIndex> linearization;
    TestBitSet todo = TestBitSet::Fill(depgraph.TxCount());
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        TestBitSet potential_next;
        for (auto j : todo) {
            if ((depgraph.Ancestors(j) & todo) == TestBitSet::Singleton(j)) {
                potential_next.Set(j);
            }
        }
        assert(potential_next.Any());
        uint64_t idx{0};
        try {
            reader >> VARINT(idx);
        } catch (const std::ios_base::failure&) {}
        idx %= potential_next.Count();
        for (auto j : potential_next) {
            if (idx == 0) {
                linearization.push_back(j);
                todo.Reset(j);
                break;
            }
            --idx;
        }
    }
    return linearization;
}

} // namespace

FUZZ_TARGET(clusterlin_add_dependency)
{
    // Verify that computing a DepGraph from a cluster, or building it step by step using AddDependency
    // have the same effect.

    // Construct a cluster of a certain length, with no dependencies.
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    auto num_tx = provider.ConsumeIntegralInRange<ClusterIndex>(2, 32);
    Cluster<TestBitSet> cluster(num_tx, std::pair{FeeFrac{0, 1}, TestBitSet{}});
    // Construct the corresponding DepGraph object (also no dependencies).
    DepGraph depgraph(cluster);
    SanityCheck(depgraph);
    // Read (parent, child) pairs, and add them to the cluster and depgraph.
    LIMITED_WHILE(provider.remaining_bytes() > 0, TestBitSet::Size() * TestBitSet::Size()) {
        auto parent = provider.ConsumeIntegralInRange<ClusterIndex>(0, num_tx - 1);
        auto child = provider.ConsumeIntegralInRange<ClusterIndex>(0, num_tx - 2);
        child += (child >= parent);
        cluster[child].second.Set(parent);
        depgraph.AddDependency(parent, child);
        assert(depgraph.Ancestors(child)[parent]);
        assert(depgraph.Descendants(parent)[child]);
    }
    // Sanity check the result.
    SanityCheck(depgraph);
    // Verify that the resulting DepGraph matches one recomputed from the cluster.
    assert(DepGraph(cluster) == depgraph);
}

FUZZ_TARGET(clusterlin_cluster_serialization)
{
    // Verify that any graph of transaction has its ancestry correctly computed by DepGraph, and if
    // it is a DAG, it can be serialized as a DepGraph in a way that roundtrips. This guarantees
    // that any acyclic cluster has a corresponding DepGraph serialization.

    FuzzedDataProvider provider(buffer.data(), buffer.size());

    // Construct a cluster in a naive way (using a FuzzedDataProvider-based serialization).
    Cluster<TestBitSet> cluster;
    auto num_tx = provider.ConsumeIntegralInRange<ClusterIndex>(1, 32);
    cluster.resize(num_tx);
    for (ClusterIndex i = 0; i < num_tx; ++i) {
        cluster[i].first.size = provider.ConsumeIntegralInRange<int32_t>(1, 0x3fffff);
        cluster[i].first.fee = provider.ConsumeIntegralInRange<int64_t>(-0x8000000000000, 0x7ffffffffffff);
        for (ClusterIndex j = 0; j < num_tx; ++j) {
            if (i == j) continue;
            if (provider.ConsumeBool()) cluster[i].second.Set(j);
        }
    }

    // Construct dependency graph. The sanity check here includes a round-trip check.
    DepGraph depgraph(cluster);
    SanityCheck(depgraph);

    // Verify that ancestry is computed correctly.
    for (ClusterIndex i = 0; i < num_tx; ++i) {
        //! Ancestors of transaction i.
        TestBitSet anc;
        // Start with being equal to just i itself.
        anc.Set(i);
        // Loop as long as more ancestors are being added.
        while (true) {
            bool changed{false};
            // For every known ancestor of i, include its parents into anc.
            for (auto i : anc) {
                if (!cluster[i].second.IsSubsetOf(anc)) {
                    changed = true;
                    anc |= cluster[i].second;
                }
            }
            if (!changed) break;
        }
        // Compare with depgraph.
        assert(depgraph.Ancestors(i) == anc);
    }
}

FUZZ_TARGET(clusterlin_depgraph_serialization)
{
    // Verify that any deserialized depgraph is acyclic and roundtrips to an identical depgraph.

    // Construct a graph by deserializing.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}
    SanityCheck(depgraph);

    // Verify the graph is a DAG.
    assert(IsAcyclic(depgraph));
}

FUZZ_TARGET(clusterlin_make_connected)
{
    // Verify that MakeConnected makes graphs connected.

    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}
    MakeConnected(depgraph);
    SanityCheck(depgraph);
    assert(depgraph.IsConnected());
}

FUZZ_TARGET(clusterlin_ancestor_finder)
{
    // Verify that AncestorCandidateFinder works as expected.

    // Retrieve a depgraph from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}
    MakeConnected(depgraph);

    AncestorCandidateFinder anc_finder(depgraph);
    auto todo = TestBitSet::Fill(depgraph.TxCount());
    while (todo.Any()) {
        // Call the ancestor finder's FindCandidateSet for what remains of the graph.
        auto best_anc = anc_finder.FindCandidateSet();
        // Sanity check the result.
        assert(best_anc.transactions.Any());
        assert(best_anc.transactions.IsSubsetOf(todo));
        assert(depgraph.FeeRate(best_anc.transactions) == best_anc.feerate);
        assert(depgraph.IsConnected(best_anc.transactions));
        // Check that it is topologically valid.
        for (auto i : best_anc.transactions) {
            assert((depgraph.Ancestors(i) & todo).IsSubsetOf(best_anc.transactions));
        }

        // Compute all remaining ancestor sets.
        std::optional<SetInfo<TestBitSet>> real_best_anc;
        for (auto i : todo) {
            SetInfo info(depgraph, todo & depgraph.Ancestors(i));
            if (!real_best_anc.has_value() || info.feerate > real_best_anc->feerate) {
                real_best_anc = info;
            }
        }
        // The set returned by anc_finder must equal the real best ancestor sets.
        assert(real_best_anc.has_value());
        assert(*real_best_anc == best_anc);

        // Find a topologically valid subset of transactions to remove from the graph.
        auto del_set = ReadTopologicalSet(depgraph, todo, reader);
        // If we did not find anything, use best_anc itself, because we should remove something.
        if (del_set.None()) del_set = best_anc.transactions;
        todo -= del_set;
        anc_finder.MarkDone(del_set);
    }
}

static constexpr auto MAX_SIMPLE_ITERATIONS = 300000;

FUZZ_TARGET(clusterlin_search_finder)
{
    // Verify that SearchCandidateFinder works as expected by sanity checking the results
    // and comparing with the results from SimpleCandidateFinder, ExhaustiveCandidateFinder, and
    // AncestorCandidateFinder.

    // Retrieve an RNG seed and a depgraph from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    uint64_t rng_seed{0};
    try {
        reader >> Using<DepGraphFormatter>(depgraph) >> rng_seed;
    } catch (const std::ios_base::failure&) {}
    MakeConnected(depgraph);

    // Instantiate ALL the candidate finders.
    SearchCandidateFinder src_finder(depgraph, rng_seed);
    SimpleCandidateFinder smp_finder(depgraph);
    ExhaustiveCandidateFinder exh_finder(depgraph);
    AncestorCandidateFinder anc_finder(depgraph);

    auto todo = TestBitSet::Fill(depgraph.TxCount());
    while (todo.Any()) {
        // For each iteration, read an iteration count limit from the fuzz input.
        uint64_t max_iterations = 1;
        try {
            reader >> VARINT(max_iterations);
        } catch (const std::ios_base::failure&) {}
        max_iterations &= 0xfffff;

        // Read an initial subset from the fuzz input.
        SetInfo init_best(depgraph, ReadTopologicalSet(depgraph, todo, reader));

        // Call the search finder's FindCandidateSet for what remains of the graph.
        auto [found, iterations_done] = src_finder.FindCandidateSet(max_iterations, init_best);

        // Sanity check the result.
        assert(iterations_done <= max_iterations);
        assert(found.transactions.Any());
        assert(found.transactions.IsSubsetOf(todo));
        assert(depgraph.FeeRate(found.transactions) == found.feerate);
        if (!init_best.feerate.IsEmpty()) assert(found.feerate >= init_best.feerate);
        // Check that it is topologically valid.
        for (auto i : found.transactions) {
            assert(found.transactions.IsSupersetOf(depgraph.Ancestors(i) & todo));
        }

        // At most 2^N-1 iterations can be required: the number of non-empty subsets a graph with N
        // transactions has.
        assert(iterations_done <= ((uint64_t{1} << todo.Count()) - 1));

        // Perform quality checks only if SearchCandidateFinder claims an optimal result.
        if (iterations_done < max_iterations) {
            // Optimal sets are always connected.
            assert(depgraph.IsConnected(found.transactions));

            // Compare with SimpleCandidateFinder.
            auto [simple, simple_iters] = smp_finder.FindCandidateSet(MAX_SIMPLE_ITERATIONS);
            assert(found.feerate >= simple.feerate);
            if (simple_iters < MAX_SIMPLE_ITERATIONS) {
                assert(found.feerate == simple.feerate);
            }

            // Compare with AncestorCandidateFinder;
            auto anc = anc_finder.FindCandidateSet();
            assert(found.feerate >= anc.feerate);

            // If todo isn't too big, also compare with ExhaustiveCandidateFinder.
            if (todo.Count() <= 12) {
                auto exhaustive = exh_finder.FindCandidateSet();
                assert(exhaustive.feerate == found.feerate);
                // Also compare ExhaustiveCandidateFinder with SimpleCandidateFinder (this is
                // primarily a test for SimpleCandidateFinder's correctness).
                assert(exhaustive.feerate >= simple.feerate);
                if (simple_iters < MAX_SIMPLE_ITERATIONS) {
                    assert(exhaustive.feerate == simple.feerate);
                }
            }
        }

        // Find a topologically valid subset of transactions to remove from the graph.
        auto del_set = ReadTopologicalSet(depgraph, todo, reader);
        // If we did not find anything, use found itself, because we should remove something.
        if (del_set.None()) del_set = found.transactions;
        todo -= del_set;
        src_finder.MarkDone(del_set);
        smp_finder.MarkDone(del_set);
        exh_finder.MarkDone(del_set);
        anc_finder.MarkDone(del_set);
    }
}

FUZZ_TARGET(clusterlin_linearize)
{
    // Verify the behavior of Linearize().

    // Retrieve an RNG seed, an iteration count, and a depgraph from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    uint64_t rng_seed{0};
    uint64_t iter_count{0};
    try {
        reader >> VARINT(iter_count) >> Using<DepGraphFormatter>(depgraph) >> rng_seed;
    } catch (const std::ios_base::failure&) {}
    MakeConnected(depgraph);

    // Optionally construct an old linearization for it.
    std::vector<ClusterIndex> old_linearization;
    {
        uint8_t have_old_linearization{0};
        try {
            reader >> have_old_linearization;
        } catch(const std::ios_base::failure&) {}
        if (have_old_linearization & 1) {
            old_linearization = ReadLinearization(depgraph, reader);
            SanityCheck(depgraph, old_linearization);
        }
    }

    // Invoke Linearize().
    iter_count &= 0x7ffff;
    auto [linearization, optimal] = Linearize(depgraph, iter_count, rng_seed, old_linearization);
    SanityCheck(depgraph, linearization);
    auto chunking = ChunkLinearization(depgraph, linearization);

    // Linearization must always be as good as the old one, if provided.
    if (!old_linearization.empty()) {
        auto old_chunking = ChunkLinearization(depgraph, old_linearization);
        auto cmp = CompareChunks(chunking, old_chunking);
        assert(cmp >= 0);
    }

    // If the iteration count is sufficiently high, an optimal linearization must be found.
    // Each linearization step can use up to 2^k iterations, with steps k=1..n. That sum is
    // 2 * (2^n - 1)
    const uint64_t n = depgraph.TxCount();
    if (n <= 18 && iter_count > 2U * ((uint64_t{1} << n) - 1U)) {
        assert(optimal);
    }

    // If Linearize claims optimal result, run quality tests.
    if (optimal) {
        // It must be as good as SimpleLinearize.
        auto [simple_linearization, simple_optimal] = SimpleLinearize(depgraph, MAX_SIMPLE_ITERATIONS);
        SanityCheck(depgraph, simple_linearization);
        auto simple_chunking = ChunkLinearization(depgraph, simple_linearization);
        auto cmp = CompareChunks(chunking, simple_chunking);
        assert(cmp >= 0);
        // If SimpleLinearize finds the optimal result too, they must be equal (if not,
        // SimpleLinearize is broken).
        if (simple_optimal) assert(cmp == 0);

        // Only for very small clusters, test every topologically-valid permutation.
        if (depgraph.TxCount() <= 7) {
            std::vector<ClusterIndex> perm_linearization(depgraph.TxCount());
            for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) perm_linearization[i] = i;
            // Iterate over all valid permutations.
            do {
                // Determine whether perm_linearization is topological.
                TestBitSet perm_done;
                bool perm_is_topo{true};
                for (auto i : perm_linearization) {
                    perm_done.Set(i);
                    if (!depgraph.Ancestors(i).IsSubsetOf(perm_done)) {
                        perm_is_topo = false;
                        break;
                    }
                }
                // If so, verify that the obtained linearization is as good as the permutation.
                if (perm_is_topo) {
                    auto perm_chunking = ChunkLinearization(depgraph, perm_linearization);
                    auto cmp = CompareChunks(chunking, perm_chunking);
                    assert(cmp >= 0);
                }
            } while(std::next_permutation(perm_linearization.begin(), perm_linearization.end()));
        }
    }
}
