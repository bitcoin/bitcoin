// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cluster_linearize.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/cluster_linearize.h>
#include <util/bitset.h>
#include <util/feefrac.h>

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

/*
 * The tests in this file primarily cover the candidate finder classes and linearization algorithms.
 *
 *   <----: An implementation (at the start of the line --) is tested in the test marked with *,
 *          possibly by comparison with other implementations (at the end of the line ->).
 *   <<---: The right side is implemented using the left side.
 *
 *   +-----------------------+
 *   | SearchCandidateFinder | <<---------------------\
 *   +-----------------------+                        |
 *     |                                            +-----------+
 *     |                                            | Linearize |
 *     |                                            +-----------+
 *     |        +-------------------------+           |  |
 *     |        | AncestorCandidateFinder | <<--------/  |
 *     |        +-------------------------+              |
 *     |          |                     ^                |        ^^  PRODUCTION CODE
 *     |          |                     |                |        ||
 *  ==============================================================================================
 *     |          |                     |                |        ||
 *     | clusterlin_ancestor_finder*    |                |        vv  TEST CODE
 *     |                                |                |
 *     |-clusterlin_search_finder*      |                |-clusterlin_linearize*
 *     |                                |                |
 *     v                                |                v
 *   +-----------------------+          |           +-----------------+
 *   | SimpleCandidateFinder | <<-------------------| SimpleLinearize |
 *   +-----------------------+          |           +-----------------+
 *                  |                   |                |
 *                  +-------------------/                |
 *                  |                                    |
 *                  |-clusterlin_simple_finder*          |-clusterlin_simple_linearize*
 *                  v                                    v
 *   +---------------------------+                  +---------------------+
 *   | ExhaustiveCandidateFinder |                  | ExhaustiveLinearize |
 *   +---------------------------+                  +---------------------+
 *
 * More tests are included for lower-level and related functions and classes:
 * - DepGraph tests:
 *   - clusterlin_depgraph_sim
 *   - clusterlin_depgraph_serialization
 *   - clusterlin_components
 * - ChunkLinearization and LinearizationChunking tests:
 *   - clusterlin_chunking
 *   - clusterlin_linearization_chunking
 * - PostLinearize tests:
 *   - clusterlin_postlinearize
 *   - clusterlin_postlinearize_tree
 *   - clusterlin_postlinearize_moved_leaf
 * - MergeLinearization tests:
 *   - clusterlin_merge
 * - FixLinearization tests:
 *   - clusterlin_fix_linearization
 * - MakeConnected tests (a test-only function):
 *   - clusterlin_make_connected
 */

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
        m_depgraph(depgraph), m_todo{depgraph.Positions()} {}

    /** Remove a set of transactions from the set of to-be-linearized ones. */
    void MarkDone(SetType select) noexcept { m_todo -= select; }

    /** Determine whether unlinearized transactions remain. */
    bool AllDone() const noexcept { return m_todo.None(); }

    /** Find a candidate set using at most max_iterations iterations, and the number of iterations
     *  actually performed. If that number is less than max_iterations, then the result is optimal.
     *
     * Always returns a connected set of transactions.
     *
     * Complexity: O(N * M), where M is the number of connected topological subsets of the cluster.
     *             That number is bounded by M <= 2^(N-1).
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
        // Best solution so far. Initialize with the remaining ancestors of the first remaining
        // transaction.
        SetInfo best(m_depgraph, m_depgraph.Ancestors(m_todo.First()) & m_todo);
        // Process the queue.
        while (!queue.empty() && iterations_left) {
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
                    --iterations_left;
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
 * It is even simpler than SimpleCandidateFinder, and exists just to help test the correctness of
 * SimpleCandidateFinder, which is then used to test the correctness of SearchCandidateFinder.
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
        m_depgraph(depgraph), m_todo{depgraph.Positions()} {}

    /** Remove a set of transactions from the set of to-be-linearized ones. */
    void MarkDone(SetType select) noexcept { m_todo -= select; }

    /** Determine whether unlinearized transactions remain. */
    bool AllDone() const noexcept { return m_todo.None(); }

    /** Find the optimal remaining candidate set.
     *
     * Complexity: O(N * 2^N).
     */
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
std::pair<std::vector<DepGraphIndex>, bool> SimpleLinearize(const DepGraph<SetType>& depgraph, uint64_t max_iterations)
{
    std::vector<DepGraphIndex> linearization;
    SimpleCandidateFinder finder(depgraph);
    SetType todo = depgraph.Positions();
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

/** An even simpler linearization algorithm that tries all permutations.
 *
 * This roughly matches SimpleLinearize() (and Linearize) in interface and behavior, but always
 * tries all topologically-valid transaction orderings, has no way to bound how much work it does,
 * and always finds the optimal. With an O(n!) complexity, it should only be used for small
 * clusters.
 */
template<typename SetType>
std::vector<DepGraphIndex> ExhaustiveLinearize(const DepGraph<SetType>& depgraph)
{
    // The best linearization so far, and its chunking.
    std::vector<DepGraphIndex> linearization;
    std::vector<FeeFrac> chunking;

    std::vector<DepGraphIndex> perm_linearization;
    // Initialize with the lexicographically-first linearization.
    for (DepGraphIndex i : depgraph.Positions()) perm_linearization.push_back(i);
    // Iterate over all valid permutations.
    do {
        /** What prefix of perm_linearization is topological. */
        DepGraphIndex topo_length{0};
        TestBitSet perm_done;
        while (topo_length < perm_linearization.size()) {
            auto i = perm_linearization[topo_length];
            perm_done.Set(i);
            if (!depgraph.Ancestors(i).IsSubsetOf(perm_done)) break;
            ++topo_length;
        }
        if (topo_length == perm_linearization.size()) {
            // If all of perm_linearization is topological, check if it is perhaps our best
            // linearization so far.
            auto perm_chunking = ChunkLinearization(depgraph, perm_linearization);
            auto cmp = CompareChunks(perm_chunking, chunking);
            // If the diagram is better, or if it is equal but with more chunks (because we
            // prefer minimal chunks), consider this better.
            if (linearization.empty() || cmp > 0 || (cmp == 0 && perm_chunking.size() > chunking.size())) {
                linearization = perm_linearization;
                chunking = perm_chunking;
            }
        } else {
            // Otherwise, fast forward to the last permutation with the same non-topological
            // prefix.
            auto first_non_topo = perm_linearization.begin() + topo_length;
            assert(std::is_sorted(first_non_topo + 1, perm_linearization.end()));
            std::reverse(first_non_topo + 1, perm_linearization.end());
        }
    } while(std::next_permutation(perm_linearization.begin(), perm_linearization.end()));

    return linearization;
}


/** Stitch connected components together in a DepGraph, guaranteeing its corresponding cluster is connected. */
template<typename BS>
void MakeConnected(DepGraph<BS>& depgraph)
{
    auto todo = depgraph.Positions();
    auto comp = depgraph.FindConnectedComponent(todo);
    Assume(depgraph.IsConnected(comp));
    todo -= comp;
    while (todo.Any()) {
        auto nextcomp = depgraph.FindConnectedComponent(todo);
        Assume(depgraph.IsConnected(nextcomp));
        depgraph.AddDependencies(BS::Singleton(comp.Last()), nextcomp.First());
        todo -= nextcomp;
        comp = nextcomp;
    }
}

/** Given a dependency graph, and a todo set, read a topological subset of todo from reader. */
template<typename SetType>
SetType ReadTopologicalSet(const DepGraph<SetType>& depgraph, const SetType& todo, SpanReader& reader, bool non_empty)
{
    // Read a bitmask from the fuzzing input. Add 1 if non_empty, so the mask is definitely not
    // zero in that case.
    uint64_t mask{0};
    try {
        reader >> VARINT(mask);
    } catch(const std::ios_base::failure&) {}
    if (mask != uint64_t(-1)) mask += non_empty;

    SetType ret;
    for (auto i : todo) {
        if (!ret[i]) {
            if (mask & 1) ret |= depgraph.Ancestors(i);
            mask >>= 1;
        }
    }
    ret &= todo;

    // While mask starts off non-zero if non_empty is true, it is still possible that all its low
    // bits are 0, and ret ends up being empty. As a last resort, use the in-todo ancestry of the
    // first todo position.
    if (non_empty && ret.None()) {
        Assume(todo.Any());
        ret = depgraph.Ancestors(todo.First()) & todo;
        Assume(ret.Any());
    }
    return ret;
}

/** Given a dependency graph, construct any valid linearization for it, reading from a SpanReader. */
template<typename BS>
std::vector<DepGraphIndex> ReadLinearization(const DepGraph<BS>& depgraph, SpanReader& reader)
{
    std::vector<DepGraphIndex> linearization;
    TestBitSet todo = depgraph.Positions();
    // In every iteration one topologically-valid transaction is appended to linearization.
    while (todo.Any()) {
        // Compute the set of transactions with no not-yet-included ancestors.
        TestBitSet potential_next;
        for (auto j : todo) {
            if ((depgraph.Ancestors(j) & todo) == TestBitSet::Singleton(j)) {
                potential_next.Set(j);
            }
        }
        // There must always be one (otherwise there is a cycle in the graph).
        assert(potential_next.Any());
        // Read a number from reader, and interpret it as index into potential_next.
        uint64_t idx{0};
        try {
            reader >> VARINT(idx);
        } catch (const std::ios_base::failure&) {}
        idx %= potential_next.Count();
        // Find out which transaction that corresponds to.
        for (auto j : potential_next) {
            if (idx == 0) {
                // When found, add it to linearization and remove it from todo.
                linearization.push_back(j);
                assert(todo[j]);
                todo.Reset(j);
                break;
            }
            --idx;
        }
    }
    return linearization;
}

} // namespace

FUZZ_TARGET(clusterlin_depgraph_sim)
{
    // Simulation test to verify the full behavior of DepGraph.

    FuzzedDataProvider provider(buffer.data(), buffer.size());

    /** Real DepGraph being tested. */
    DepGraph<TestBitSet> real;
    /** Simulated DepGraph (sim[i] is std::nullopt if position i does not exist; otherwise,
     *  sim[i]->first is its individual feerate, and sim[i]->second is its set of ancestors. */
    std::array<std::optional<std::pair<FeeFrac, TestBitSet>>, TestBitSet::Size()> sim;
    /** The number of non-nullopt position in sim. */
    DepGraphIndex num_tx_sim{0};

    /** Read a valid index of a transaction from the provider. */
    auto idx_fn = [&]() {
        auto offset = provider.ConsumeIntegralInRange<DepGraphIndex>(0, num_tx_sim - 1);
        for (DepGraphIndex i = 0; i < sim.size(); ++i) {
            if (!sim[i].has_value()) continue;
            if (offset == 0) return i;
            --offset;
        }
        assert(false);
        return DepGraphIndex(-1);
    };

    /** Read a valid subset of the transactions from the provider. */
    auto subset_fn = [&]() {
        auto range = (uint64_t{1} << num_tx_sim) - 1;
        const auto mask = provider.ConsumeIntegralInRange<uint64_t>(0, range);
        auto mask_shifted = mask;
        TestBitSet subset;
        for (DepGraphIndex i = 0; i < sim.size(); ++i) {
            if (!sim[i].has_value()) continue;
            if (mask_shifted & 1) {
                subset.Set(i);
            }
            mask_shifted >>= 1;
        }
        assert(mask_shifted == 0);
        return subset;
    };

    /** Read any set of transactions from the provider (including unused positions). */
    auto set_fn = [&]() {
        auto range = (uint64_t{1} << sim.size()) - 1;
        const auto mask = provider.ConsumeIntegralInRange<uint64_t>(0, range);
        TestBitSet set;
        for (DepGraphIndex i = 0; i < sim.size(); ++i) {
            if ((mask >> i) & 1) {
                set.Set(i);
            }
        }
        return set;
    };

    /** Propagate ancestor information in sim. */
    auto anc_update_fn = [&]() {
        while (true) {
            bool updates{false};
            for (DepGraphIndex chl = 0; chl < sim.size(); ++chl) {
                if (!sim[chl].has_value()) continue;
                for (auto par : sim[chl]->second) {
                    if (!sim[chl]->second.IsSupersetOf(sim[par]->second)) {
                        sim[chl]->second |= sim[par]->second;
                        updates = true;
                    }
                }
            }
            if (!updates) break;
        }
    };

    /** Compare the state of transaction i in the simulation with the real one. */
    auto check_fn = [&](DepGraphIndex i) {
        // Compare used positions.
        assert(real.Positions()[i] == sim[i].has_value());
        if (sim[i].has_value()) {
            // Compare feerate.
            assert(real.FeeRate(i) == sim[i]->first);
            // Compare ancestors (note that SanityCheck verifies correspondence between ancestors
            // and descendants, so we can restrict ourselves to ancestors here).
            assert(real.Ancestors(i) == sim[i]->second);
        }
    };

    auto last_compaction_pos{real.PositionRange()};

    LIMITED_WHILE(provider.remaining_bytes() > 0, 1000) {
        int command = provider.ConsumeIntegral<uint8_t>() % 4;
        while (true) {
            // Iterate decreasing command until an applicable branch is found.
            if (num_tx_sim < TestBitSet::Size() && command-- == 0) {
                // AddTransaction.
                auto fee = provider.ConsumeIntegralInRange<int64_t>(-0x8000000000000, 0x7ffffffffffff);
                auto size = provider.ConsumeIntegralInRange<int32_t>(1, 0x3fffff);
                FeeFrac feerate{fee, size};
                // Apply to DepGraph.
                auto idx = real.AddTransaction(feerate);
                // Verify that the returned index is correct.
                assert(!sim[idx].has_value());
                for (DepGraphIndex i = 0; i < TestBitSet::Size(); ++i) {
                    if (!sim[i].has_value()) {
                        assert(idx == i);
                        break;
                    }
                }
                // Update sim.
                sim[idx] = {feerate, TestBitSet::Singleton(idx)};
                ++num_tx_sim;
                break;
            } else if (num_tx_sim > 0 && command-- == 0) {
                // AddDependencies.
                DepGraphIndex child = idx_fn();
                auto parents = subset_fn();
                // Apply to DepGraph.
                real.AddDependencies(parents, child);
                // Apply to sim.
                sim[child]->second |= parents;
                break;
            } else if (num_tx_sim > 0 && command-- == 0) {
                // Remove transactions.
                auto del = set_fn();
                // Propagate all ancestry information before deleting anything in the simulation (as
                // intermediary transactions may be deleted which impact connectivity).
                anc_update_fn();
                // Compare the state of the transactions being deleted.
                for (auto i : del) check_fn(i);
                // Apply to DepGraph.
                real.RemoveTransactions(del);
                // Apply to sim.
                for (DepGraphIndex i = 0; i < sim.size(); ++i) {
                    if (sim[i].has_value()) {
                        if (del[i]) {
                            --num_tx_sim;
                            sim[i] = std::nullopt;
                        } else {
                            sim[i]->second -= del;
                        }
                    }
                }
                break;
            } else if (command-- == 0) {
                // Compact.
                const size_t mem_before{real.DynamicMemoryUsage()};
                real.Compact();
                const size_t mem_after{real.DynamicMemoryUsage()};
                assert(real.PositionRange() < last_compaction_pos ? mem_after < mem_before : mem_after <= mem_before);
                last_compaction_pos = real.PositionRange();
                break;
            }
        }
    }

    // Compare the real obtained depgraph against the simulation.
    anc_update_fn();
    for (DepGraphIndex i = 0; i < sim.size(); ++i) check_fn(i);
    assert(real.TxCount() == num_tx_sim);
    // Sanity check the result (which includes round-tripping serialization, if applicable).
    SanityCheck(real);
}

FUZZ_TARGET(clusterlin_depgraph_serialization)
{
    // Verify that any deserialized depgraph is acyclic and roundtrips to an identical depgraph.

    // Construct a graph by deserializing.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    DepGraphIndex par_code{0}, chl_code{0};
    try {
        reader >> Using<DepGraphFormatter>(depgraph) >> VARINT(par_code) >> VARINT(chl_code);
    } catch (const std::ios_base::failure&) {}
    SanityCheck(depgraph);

    // Verify the graph is a DAG.
    assert(depgraph.IsAcyclic());

    // Introduce a cycle, and then test that IsAcyclic returns false.
    if (depgraph.TxCount() < 2) return;
    DepGraphIndex par(0), chl(0);
    // Pick any transaction of depgraph as parent.
    par_code %= depgraph.TxCount();
    for (auto i : depgraph.Positions()) {
        if (par_code == 0) {
            par = i;
            break;
        }
        --par_code;
    }
    // Pick any ancestor of par (excluding itself) as child, if any.
    auto ancestors = depgraph.Ancestors(par) - TestBitSet::Singleton(par);
    if (ancestors.None()) return;
    chl_code %= ancestors.Count();
    for (auto i : ancestors) {
        if (chl_code == 0) {
            chl = i;
            break;
        }
        --chl_code;
    }
    // Add the cycle-introducing dependency.
    depgraph.AddDependencies(TestBitSet::Singleton(par), chl);
    // Check that we now detect a cycle.
    assert(!depgraph.IsAcyclic());
}

FUZZ_TARGET(clusterlin_components)
{
    // Verify the behavior of DepGraphs's FindConnectedComponent and IsConnected functions.

    // Construct a depgraph.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    std::vector<DepGraphIndex> linearization;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}

    TestBitSet todo = depgraph.Positions();
    while (todo.Any()) {
        // Pick a transaction in todo, or nothing.
        std::optional<DepGraphIndex> picked;
        {
            uint64_t picked_num{0};
            try {
                reader >> VARINT(picked_num);
            } catch (const std::ios_base::failure&) {}
            if (picked_num < todo.Size() && todo[picked_num]) {
                picked = picked_num;
            }
        }

        // Find a connected component inside todo, including picked if any.
        auto component = picked ? depgraph.GetConnectedComponent(todo, *picked)
                                : depgraph.FindConnectedComponent(todo);

        // The component must be a subset of todo and non-empty.
        assert(component.IsSubsetOf(todo));
        assert(component.Any());

        // If picked was provided, the component must include it.
        if (picked) assert(component[*picked]);

        // If todo is the entire graph, and the entire graph is connected, then the component must
        // be the entire graph.
        if (todo == depgraph.Positions()) {
            assert((component == todo) == depgraph.IsConnected());
        }

        // If subset is connected, then component must match subset.
        assert((component == todo) == depgraph.IsConnected(todo));

        // The component cannot have any ancestors or descendants outside of component but in todo.
        for (auto i : component) {
            assert((depgraph.Ancestors(i) & todo).IsSubsetOf(component));
            assert((depgraph.Descendants(i) & todo).IsSubsetOf(component));
        }

        // Starting from any component element, we must be able to reach every element.
        for (auto i : component) {
            // Start with just i as reachable.
            TestBitSet reachable = TestBitSet::Singleton(i);
            // Add in-todo descendants and ancestors to reachable until it does not change anymore.
            while (true) {
                TestBitSet new_reachable = reachable;
                for (auto j : new_reachable) {
                    new_reachable |= depgraph.Ancestors(j) & todo;
                    new_reachable |= depgraph.Descendants(j) & todo;
                }
                if (new_reachable == reachable) break;
                reachable = new_reachable;
            }
            // Verify that the result is the entire component.
            assert(component == reachable);
        }

        // Construct an arbitrary subset of todo.
        uint64_t subset_bits{0};
        try {
            reader >> VARINT(subset_bits);
        } catch (const std::ios_base::failure&) {}
        TestBitSet subset;
        for (DepGraphIndex i : depgraph.Positions()) {
            if (todo[i]) {
                if (subset_bits & 1) subset.Set(i);
                subset_bits >>= 1;
            }
        }
        // Which must be non-empty.
        if (subset.None()) subset = TestBitSet::Singleton(todo.First());
        // Remove it from todo.
        todo -= subset;
    }

    // No components can be found in an empty subset.
    assert(depgraph.FindConnectedComponent(todo).None());
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

FUZZ_TARGET(clusterlin_chunking)
{
    // Verify the correctness of the ChunkLinearization function.

    // Construct a graph by deserializing.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}

    // Read a valid linearization for depgraph.
    auto linearization = ReadLinearization(depgraph, reader);

    // Invoke the chunking function.
    auto chunking = ChunkLinearization(depgraph, linearization);

    // Verify that chunk feerates are monotonically non-increasing.
    for (size_t i = 1; i < chunking.size(); ++i) {
        assert(!(chunking[i] >> chunking[i - 1]));
    }

    // Naively recompute the chunks (each is the highest-feerate prefix of what remains).
    auto todo = depgraph.Positions();
    for (const auto& chunk_feerate : chunking) {
        assert(todo.Any());
        SetInfo<TestBitSet> accumulator, best;
        for (DepGraphIndex idx : linearization) {
            if (todo[idx]) {
                accumulator.Set(depgraph, idx);
                if (best.feerate.IsEmpty() || accumulator.feerate >> best.feerate) {
                    best = accumulator;
                }
            }
        }
        assert(chunk_feerate == best.feerate);
        assert(best.transactions.IsSubsetOf(todo));
        todo -= best.transactions;
    }
    assert(todo.None());
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

    AncestorCandidateFinder anc_finder(depgraph);
    auto todo = depgraph.Positions();
    while (todo.Any()) {
        // Call the ancestor finder's FindCandidateSet for what remains of the graph.
        assert(!anc_finder.AllDone());
        assert(todo.Count() == anc_finder.NumRemaining());
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

        // Find a non-empty topologically valid subset of transactions to remove from the graph.
        // Using an empty set would mean the next iteration is identical to the current one, and
        // could cause an infinite loop.
        auto del_set = ReadTopologicalSet(depgraph, todo, reader, /*non_empty=*/true);
        todo -= del_set;
        anc_finder.MarkDone(del_set);
    }
    assert(anc_finder.AllDone());
    assert(anc_finder.NumRemaining() == 0);
}

static constexpr auto MAX_SIMPLE_ITERATIONS = 300000;

FUZZ_TARGET(clusterlin_simple_finder)
{
    // Verify that SimpleCandidateFinder works as expected by sanity checking the results
    // and comparing them (if claimed to be optimal) against the sets found by
    // ExhaustiveCandidateFinder and AncestorCandidateFinder.
    //
    // Note that SimpleCandidateFinder is only used in tests; the purpose of this fuzz test is to
    // establish confidence in SimpleCandidateFinder, so that it can be used to test
    // SearchCandidateFinder below.

    // Retrieve a depgraph from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}

    // Instantiate the SimpleCandidateFinder to be tested, and the ExhaustiveCandidateFinder and
    // AncestorCandidateFinder it is being tested against.
    SimpleCandidateFinder smp_finder(depgraph);
    ExhaustiveCandidateFinder exh_finder(depgraph);
    AncestorCandidateFinder anc_finder(depgraph);

    auto todo = depgraph.Positions();
    while (todo.Any()) {
        assert(!smp_finder.AllDone());
        assert(!exh_finder.AllDone());
        assert(!anc_finder.AllDone());
        assert(anc_finder.NumRemaining() == todo.Count());

        // Call SimpleCandidateFinder.
        auto [found, iterations_done] = smp_finder.FindCandidateSet(MAX_SIMPLE_ITERATIONS);
        bool optimal = (iterations_done != MAX_SIMPLE_ITERATIONS);

        // Sanity check the result.
        assert(iterations_done <= MAX_SIMPLE_ITERATIONS);
        assert(found.transactions.Any());
        assert(found.transactions.IsSubsetOf(todo));
        assert(depgraph.FeeRate(found.transactions) == found.feerate);
        // Check that it is topologically valid.
        for (auto i : found.transactions) {
            assert(found.transactions.IsSupersetOf(depgraph.Ancestors(i) & todo));
        }

        // At most 2^(N-1) iterations can be required: the number of non-empty connected subsets a
        // graph with N transactions can have. If MAX_SIMPLE_ITERATIONS exceeds this number, the
        // result is necessarily optimal.
        assert(iterations_done <= (uint64_t{1} << (todo.Count() - 1)));
        if (MAX_SIMPLE_ITERATIONS > (uint64_t{1} << (todo.Count() - 1))) assert(optimal);

        // SimpleCandidateFinder only finds connected sets.
        assert(depgraph.IsConnected(found.transactions));

        // Perform further quality checks only if SimpleCandidateFinder claims an optimal result.
        if (optimal) {
            // Compare with AncestorCandidateFinder.
            auto anc = anc_finder.FindCandidateSet();
            assert(anc.feerate <= found.feerate);

            if (todo.Count() <= 12) {
                // Compare with ExhaustiveCandidateFinder. This quickly gets computationally
                // expensive for large clusters (O(2^n)), so only do it for sufficiently small ones.
                auto exhaustive = exh_finder.FindCandidateSet();
                assert(exhaustive.feerate == found.feerate);
            }

            // Compare with a non-empty topological set read from the fuzz input (comparing with an
            // empty set is not interesting).
            auto read_topo = ReadTopologicalSet(depgraph, todo, reader, /*non_empty=*/true);
            assert(found.feerate >= depgraph.FeeRate(read_topo));
        }

        // Find a non-empty topologically valid subset of transactions to remove from the graph.
        // Using an empty set would mean the next iteration is identical to the current one, and
        // could cause an infinite loop.
        auto del_set = ReadTopologicalSet(depgraph, todo, reader, /*non_empty=*/true);
        todo -= del_set;
        smp_finder.MarkDone(del_set);
        exh_finder.MarkDone(del_set);
        anc_finder.MarkDone(del_set);
    }

    assert(smp_finder.AllDone());
    assert(exh_finder.AllDone());
    assert(anc_finder.AllDone());
    assert(anc_finder.NumRemaining() == 0);
}

FUZZ_TARGET(clusterlin_search_finder)
{
    // Verify that SearchCandidateFinder works as expected by sanity checking the results
    // and comparing with the results from SimpleCandidateFinder and AncestorCandidateFinder,
    // if the result is claimed to be optimal.

    // Retrieve an RNG seed, a depgraph, and whether to make it connected, from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    uint64_t rng_seed{0};
    uint8_t make_connected{1};
    try {
        reader >> Using<DepGraphFormatter>(depgraph) >> rng_seed >> make_connected;
    } catch (const std::ios_base::failure&) {}
    // The most complicated graphs are connected ones (other ones just split up). Optionally force
    // the graph to be connected.
    if (make_connected) MakeConnected(depgraph);

    // Instantiate the candidate finders.
    SearchCandidateFinder src_finder(depgraph, rng_seed);
    SimpleCandidateFinder smp_finder(depgraph);
    AncestorCandidateFinder anc_finder(depgraph);

    auto todo = depgraph.Positions();
    while (todo.Any()) {
        assert(!src_finder.AllDone());
        assert(!smp_finder.AllDone());
        assert(!anc_finder.AllDone());
        assert(anc_finder.NumRemaining() == todo.Count());

        // For each iteration, read an iteration count limit from the fuzz input.
        uint64_t max_iterations = 1;
        try {
            reader >> VARINT(max_iterations);
        } catch (const std::ios_base::failure&) {}
        max_iterations &= 0xfffff;

        // Read an initial subset from the fuzz input (allowed to be empty).
        auto init_set = ReadTopologicalSet(depgraph, todo, reader, /*non_empty=*/false);
        SetInfo init_best(depgraph, init_set);

        // Call the search finder's FindCandidateSet for what remains of the graph.
        auto [found, iterations_done] = src_finder.FindCandidateSet(max_iterations, init_best);
        bool optimal = iterations_done < max_iterations;

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

        // At most 2^(N-1) iterations can be required: the maximum number of non-empty topological
        // subsets a (connected) cluster with N transactions can have. Even when the cluster is no
        // longer connected after removing certain transactions, this holds, because the connected
        // components are searched separately.
        assert(iterations_done <= (uint64_t{1} << (todo.Count() - 1)));
        // Additionally, test that no more than sqrt(2^N)+1 iterations are required. This is just
        // an empirical bound that seems to hold, without proof. Still, add a test for it so we
        // can learn about counterexamples if they exist.
        if (iterations_done >= 1 && todo.Count() <= 63) {
            Assume((iterations_done - 1) * (iterations_done - 1) <= uint64_t{1} << todo.Count());
        }

        // Perform quality checks only if SearchCandidateFinder claims an optimal result.
        if (optimal) {
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

            // Compare with a non-empty topological set read from the fuzz input (comparing with an
            // empty set is not interesting).
            auto read_topo = ReadTopologicalSet(depgraph, todo, reader, /*non_empty=*/true);
            assert(found.feerate >= depgraph.FeeRate(read_topo));
        }

        // Find a non-empty topologically valid subset of transactions to remove from the graph.
        // Using an empty set would mean the next iteration is identical to the current one, and
        // could cause an infinite loop.
        auto del_set = ReadTopologicalSet(depgraph, todo, reader, /*non_empty=*/true);
        todo -= del_set;
        src_finder.MarkDone(del_set);
        smp_finder.MarkDone(del_set);
        anc_finder.MarkDone(del_set);
    }

    assert(src_finder.AllDone());
    assert(smp_finder.AllDone());
    assert(anc_finder.AllDone());
    assert(anc_finder.NumRemaining() == 0);
}

FUZZ_TARGET(clusterlin_linearization_chunking)
{
    // Verify the behavior of LinearizationChunking.

    // Retrieve a depgraph from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}

    // Retrieve a topologically-valid subset of depgraph (allowed to be empty, because the argument
    // to LinearizationChunking::Intersect is allowed to be empty).
    auto todo = depgraph.Positions();
    auto subset = SetInfo(depgraph, ReadTopologicalSet(depgraph, todo, reader, /*non_empty=*/false));

    // Retrieve a valid linearization for depgraph.
    auto linearization = ReadLinearization(depgraph, reader);

    // Construct a LinearizationChunking object, initially for the whole linearization.
    LinearizationChunking chunking(depgraph, linearization);

    // Incrementally remove transactions from the chunking object, and check various properties at
    // every step.
    while (todo.Any()) {
        assert(chunking.NumChunksLeft() > 0);

        // Construct linearization with just todo.
        std::vector<DepGraphIndex> linearization_left;
        for (auto i : linearization) {
            if (todo[i]) linearization_left.push_back(i);
        }

        // Compute the chunking for linearization_left.
        auto chunking_left = ChunkLinearization(depgraph, linearization_left);

        // Verify that it matches the feerates of the chunks of chunking.
        assert(chunking.NumChunksLeft() == chunking_left.size());
        for (DepGraphIndex i = 0; i < chunking.NumChunksLeft(); ++i) {
            assert(chunking.GetChunk(i).feerate == chunking_left[i]);
        }

        // Check consistency of chunking.
        TestBitSet combined;
        for (DepGraphIndex i = 0; i < chunking.NumChunksLeft(); ++i) {
            const auto& chunk_info = chunking.GetChunk(i);
            // Chunks must be non-empty.
            assert(chunk_info.transactions.Any());
            // Chunk feerates must be monotonically non-increasing.
            if (i > 0) assert(!(chunk_info.feerate >> chunking.GetChunk(i - 1).feerate));
            // Chunks must be a subset of what is left of the linearization.
            assert(chunk_info.transactions.IsSubsetOf(todo));
            // Chunks' claimed feerates must match their transactions' aggregate feerate.
            assert(depgraph.FeeRate(chunk_info.transactions) == chunk_info.feerate);
            // Chunks must be the highest-feerate remaining prefix.
            SetInfo<TestBitSet> accumulator, best;
            for (auto j : linearization) {
                if (todo[j] && !combined[j]) {
                    accumulator.Set(depgraph, j);
                    if (best.feerate.IsEmpty() || accumulator.feerate > best.feerate) {
                        best = accumulator;
                    }
                }
            }
            assert(best.transactions == chunk_info.transactions);
            assert(best.feerate == chunk_info.feerate);
            // Chunks cannot overlap.
            assert(!chunk_info.transactions.Overlaps(combined));
            combined |= chunk_info.transactions;
            // Chunks must be topological.
            for (auto idx : chunk_info.transactions) {
                assert((depgraph.Ancestors(idx) & todo).IsSubsetOf(combined));
            }
        }
        assert(combined == todo);

        // Verify the expected properties of LinearizationChunking::IntersectPrefixes:
        auto intersect = chunking.IntersectPrefixes(subset);
        // - Intersecting again doesn't change the result.
        assert(chunking.IntersectPrefixes(intersect) == intersect);
        // - The intersection is topological.
        TestBitSet intersect_anc;
        for (auto idx : intersect.transactions) {
            intersect_anc |= (depgraph.Ancestors(idx) & todo);
        }
        assert(intersect.transactions == intersect_anc);
        // - The claimed intersection feerate matches its transactions.
        assert(intersect.feerate == depgraph.FeeRate(intersect.transactions));
        // - The intersection may only be empty if its input is empty.
        assert(intersect.transactions.Any() == subset.transactions.Any());
        // - The intersection feerate must be as high as the input.
        assert(intersect.feerate >= subset.feerate);
        // - No non-empty intersection between the intersection and a prefix of the chunks of the
        //   remainder of the linearization may be better than the intersection.
        TestBitSet prefix;
        for (DepGraphIndex i = 0; i < chunking.NumChunksLeft(); ++i) {
            prefix |= chunking.GetChunk(i).transactions;
            auto reintersect = SetInfo(depgraph, prefix & intersect.transactions);
            if (!reintersect.feerate.IsEmpty()) {
                assert(reintersect.feerate <= intersect.feerate);
            }
        }

        // Find a non-empty topologically valid subset of transactions to remove from the graph.
        // Using an empty set would mean the next iteration is identical to the current one, and
        // could cause an infinite loop.
        auto done = ReadTopologicalSet(depgraph, todo, reader, /*non_empty=*/true);
        todo -= done;
        chunking.MarkDone(done);
        subset = SetInfo(depgraph, subset.transactions - done);
    }

    assert(chunking.NumChunksLeft() == 0);
}

FUZZ_TARGET(clusterlin_simple_linearize)
{
    // Verify the behavior of SimpleLinearize(). Note that SimpleLinearize is only used in tests;
    // the purpose of this fuzz test is to establish confidence in SimpleLinearize, so that it can
    // be used to test the real Linearize function in the fuzz test below.

    // Retrieve an iteration count and a depgraph from the fuzz input.
    SpanReader reader(buffer);
    uint64_t iter_count{0};
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> VARINT(iter_count) >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}
    iter_count %= MAX_SIMPLE_ITERATIONS;

    // Invoke SimpleLinearize().
    auto [linearization, optimal] = SimpleLinearize(depgraph, iter_count);
    SanityCheck(depgraph, linearization);
    auto simple_chunking = ChunkLinearization(depgraph, linearization);

    // If the iteration count is sufficiently high, an optimal linearization must be found.
    // SimpleLinearize on k transactions can take up to 2^(k-1) iterations (one per non-empty
    // connected topologically valid subset), which sums over k=1..n to (2^n)-1.
    const uint64_t n = depgraph.TxCount();
    if (n <= 63 && (iter_count >> n)) {
        assert(optimal);
    }

    // If SimpleLinearize claims optimal result, and the cluster is sufficiently small (there are
    // n! linearizations), test that the result is as good as every valid linearization.
    if (optimal && depgraph.TxCount() <= 8) {
        auto exh_linearization = ExhaustiveLinearize(depgraph);
        auto exh_chunking = ChunkLinearization(depgraph, exh_linearization);
        auto cmp = CompareChunks(simple_chunking, exh_chunking);
        assert(cmp == 0);
        assert(simple_chunking.size() == exh_chunking.size());
    }

    if (optimal) {
        // Compare with a linearization read from the fuzz input.
        auto read = ReadLinearization(depgraph, reader);
        auto read_chunking = ChunkLinearization(depgraph, read);
        auto cmp = CompareChunks(simple_chunking, read_chunking);
        assert(cmp >= 0);
    }
}

FUZZ_TARGET(clusterlin_linearize)
{
    // Verify the behavior of Linearize().

    // Retrieve an RNG seed, an iteration count, a depgraph, and whether to make it connected from
    // the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    uint64_t rng_seed{0};
    uint64_t iter_count{0};
    uint8_t make_connected{1};
    try {
        reader >> VARINT(iter_count) >> Using<DepGraphFormatter>(depgraph) >> rng_seed >> make_connected;
    } catch (const std::ios_base::failure&) {}
    // The most complicated graphs are connected ones (other ones just split up). Optionally force
    // the graph to be connected.
    if (make_connected) MakeConnected(depgraph);

    // Optionally construct an old linearization for it.
    std::vector<DepGraphIndex> old_linearization;
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
    auto [linearization, optimal, cost] = Linearize(depgraph, iter_count, rng_seed, old_linearization);
    assert(cost <= iter_count);
    SanityCheck(depgraph, linearization);
    auto chunking = ChunkLinearization(depgraph, linearization);

    // Linearization must always be as good as the old one, if provided.
    if (!old_linearization.empty()) {
        auto old_chunking = ChunkLinearization(depgraph, old_linearization);
        auto cmp = CompareChunks(chunking, old_chunking);
        assert(cmp >= 0);
    }

    // If the iteration count is sufficiently high, an optimal linearization must be found.
    if (iter_count >= MaxOptimalLinearizationIters(depgraph.TxCount())) {
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
        // If simple_chunking is diagram-optimal, it cannot have more chunks than chunking (as
        // chunking is claimed to be optimal, which implies minimal chunks).
        if (cmp == 0) assert(chunking.size() >= simple_chunking.size());

        // Compare with a linearization read from the fuzz input.
        auto read = ReadLinearization(depgraph, reader);
        auto read_chunking = ChunkLinearization(depgraph, read);
        auto cmp_read = CompareChunks(chunking, read_chunking);
        assert(cmp_read >= 0);
    }
}

FUZZ_TARGET(clusterlin_postlinearize)
{
    // Verify expected properties of PostLinearize() on arbitrary linearizations.

    // Retrieve a depgraph from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}

    // Retrieve a linearization from the fuzz input.
    std::vector<DepGraphIndex> linearization;
    linearization = ReadLinearization(depgraph, reader);
    SanityCheck(depgraph, linearization);

    // Produce a post-processed version.
    auto post_linearization = linearization;
    PostLinearize(depgraph, post_linearization);
    SanityCheck(depgraph, post_linearization);

    // Compare diagrams: post-linearization cannot worsen anywhere.
    auto chunking = ChunkLinearization(depgraph, linearization);
    auto post_chunking = ChunkLinearization(depgraph, post_linearization);
    auto cmp = CompareChunks(post_chunking, chunking);
    assert(cmp >= 0);

    // Run again, things can keep improving (and never get worse)
    auto post_post_linearization = post_linearization;
    PostLinearize(depgraph, post_post_linearization);
    SanityCheck(depgraph, post_post_linearization);
    auto post_post_chunking = ChunkLinearization(depgraph, post_post_linearization);
    cmp = CompareChunks(post_post_chunking, post_chunking);
    assert(cmp >= 0);

    // The chunks that come out of postlinearizing are always connected.
    LinearizationChunking linchunking(depgraph, post_linearization);
    while (linchunking.NumChunksLeft()) {
        assert(depgraph.IsConnected(linchunking.GetChunk(0).transactions));
        linchunking.MarkDone(linchunking.GetChunk(0).transactions);
    }
}

FUZZ_TARGET(clusterlin_postlinearize_tree)
{
    // Verify expected properties of PostLinearize() on linearizations of graphs that form either
    // an upright or reverse tree structure.

    // Construct a direction, RNG seed, and an arbitrary graph from the fuzz input.
    SpanReader reader(buffer);
    uint64_t rng_seed{0};
    DepGraph<TestBitSet> depgraph_gen;
    uint8_t direction{0};
    try {
        reader >> direction >> rng_seed >> Using<DepGraphFormatter>(depgraph_gen);
    } catch (const std::ios_base::failure&) {}

    // Now construct a new graph, copying the nodes, but leaving only the first parent (even
    // direction) or the first child (odd direction).
    DepGraph<TestBitSet> depgraph_tree;
    for (DepGraphIndex i = 0; i < depgraph_gen.PositionRange(); ++i) {
        if (depgraph_gen.Positions()[i]) {
            depgraph_tree.AddTransaction(depgraph_gen.FeeRate(i));
        } else {
            // For holes, add a dummy transaction which is deleted below, so that non-hole
            // transactions retain their position.
            depgraph_tree.AddTransaction(FeeFrac{});
        }
    }
    depgraph_tree.RemoveTransactions(TestBitSet::Fill(depgraph_gen.PositionRange()) - depgraph_gen.Positions());

    if (direction & 1) {
        for (DepGraphIndex i = 0; i < depgraph_gen.TxCount(); ++i) {
            auto children = depgraph_gen.GetReducedChildren(i);
            if (children.Any()) {
                depgraph_tree.AddDependencies(TestBitSet::Singleton(i), children.First());
            }
         }
    } else {
        for (DepGraphIndex i = 0; i < depgraph_gen.TxCount(); ++i) {
            auto parents = depgraph_gen.GetReducedParents(i);
            if (parents.Any()) {
                depgraph_tree.AddDependencies(TestBitSet::Singleton(parents.First()), i);
            }
        }
    }

    // Retrieve a linearization from the fuzz input.
    std::vector<DepGraphIndex> linearization;
    linearization = ReadLinearization(depgraph_tree, reader);
    SanityCheck(depgraph_tree, linearization);

    // Produce a postlinearized version.
    auto post_linearization = linearization;
    PostLinearize(depgraph_tree, post_linearization);
    SanityCheck(depgraph_tree, post_linearization);

    // Compare diagrams.
    auto chunking = ChunkLinearization(depgraph_tree, linearization);
    auto post_chunking = ChunkLinearization(depgraph_tree, post_linearization);
    auto cmp = CompareChunks(post_chunking, chunking);
    assert(cmp >= 0);

    // Verify that post-linearizing again does not change the diagram. The result must be identical
    // as post_linearization ought to be optimal already with a tree-structured graph.
    auto post_post_linearization = post_linearization;
    PostLinearize(depgraph_tree, post_linearization);
    SanityCheck(depgraph_tree, post_linearization);
    auto post_post_chunking = ChunkLinearization(depgraph_tree, post_post_linearization);
    auto cmp_post = CompareChunks(post_post_chunking, post_chunking);
    assert(cmp_post == 0);

    // Try to find an even better linearization directly. This must not change the diagram for the
    // same reason.
    auto [opt_linearization, _optimal, _cost] = Linearize(depgraph_tree, 100000, rng_seed, post_linearization);
    auto opt_chunking = ChunkLinearization(depgraph_tree, opt_linearization);
    auto cmp_opt = CompareChunks(opt_chunking, post_chunking);
    assert(cmp_opt == 0);
}

FUZZ_TARGET(clusterlin_postlinearize_moved_leaf)
{
    // Verify that taking an existing linearization, and moving a leaf to the back, potentially
    // increasing its fee, and then post-linearizing, results in something as good as the
    // original. This guarantees that in an RBF that replaces a transaction with one of the same
    // size but higher fee, applying the "remove conflicts, append new transaction, postlinearize"
    // process will never worsen linearization quality.

    // Construct an arbitrary graph and a fee from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    int32_t fee_inc{0};
    try {
        uint64_t fee_inc_code;
        reader >> Using<DepGraphFormatter>(depgraph) >> VARINT(fee_inc_code);
        fee_inc = fee_inc_code & 0x3ffff;
    } catch (const std::ios_base::failure&) {}
    if (depgraph.TxCount() == 0) return;

    // Retrieve two linearizations from the fuzz input.
    auto lin = ReadLinearization(depgraph, reader);
    auto lin_leaf = ReadLinearization(depgraph, reader);

    // Construct a linearization identical to lin, but with the tail end of lin_leaf moved to the
    // back.
    std::vector<DepGraphIndex> lin_moved;
    for (auto i : lin) {
        if (i != lin_leaf.back()) lin_moved.push_back(i);
    }
    lin_moved.push_back(lin_leaf.back());

    // Postlinearize lin_moved.
    PostLinearize(depgraph, lin_moved);
    SanityCheck(depgraph, lin_moved);

    // Compare diagrams (applying the fee delta after computing the old one).
    auto old_chunking = ChunkLinearization(depgraph, lin);
    depgraph.FeeRate(lin_leaf.back()).fee += fee_inc;
    auto new_chunking = ChunkLinearization(depgraph, lin_moved);
    auto cmp = CompareChunks(new_chunking, old_chunking);
    assert(cmp >= 0);
}

FUZZ_TARGET(clusterlin_merge)
{
    // Construct an arbitrary graph from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}

    // Retrieve two linearizations from the fuzz input.
    auto lin1 = ReadLinearization(depgraph, reader);
    auto lin2 = ReadLinearization(depgraph, reader);

    // Merge the two.
    auto lin_merged = MergeLinearizations(depgraph, lin1, lin2);

    // Compute chunkings and compare.
    auto chunking1 = ChunkLinearization(depgraph, lin1);
    auto chunking2 = ChunkLinearization(depgraph, lin2);
    auto chunking_merged = ChunkLinearization(depgraph, lin_merged);
    auto cmp1 = CompareChunks(chunking_merged, chunking1);
    assert(cmp1 >= 0);
    auto cmp2 = CompareChunks(chunking_merged, chunking2);
    assert(cmp2 >= 0);
}

FUZZ_TARGET(clusterlin_fix_linearization)
{
    // Verify expected properties of FixLinearization() on arbitrary linearizations.

    // Retrieve a depgraph from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}

    // Construct an arbitrary linearization (not necessarily topological for depgraph).
    std::vector<DepGraphIndex> linearization;
    /** Which transactions of depgraph are yet to be included in linearization. */
    TestBitSet todo = depgraph.Positions();
    while (todo.Any()) {
        // Read a number from the fuzz input in range [0, todo.Count()).
        uint64_t val{0};
        try {
            reader >> VARINT(val);
        } catch (const std::ios_base::failure&) {}
        val %= todo.Count();
        // Find the val'th element in todo, remove it from todo, and append it to linearization.
        for (auto idx : todo) {
            if (val == 0) {
                linearization.push_back(idx);
                todo.Reset(idx);
                break;
            }
            --val;
        }
    }
    assert(linearization.size() == depgraph.TxCount());

    // Determine what prefix of linearization is topological, i.e., the position of the first entry
    // in linearization which corresponds to a transaction that is not preceded by all its
    // ancestors.
    size_t topo_prefix = 0;
    todo = depgraph.Positions();
    while (topo_prefix < linearization.size()) {
        DepGraphIndex idx = linearization[topo_prefix];
        todo.Reset(idx);
        if (todo.Overlaps(depgraph.Ancestors(idx))) break;
        ++topo_prefix;
    }

    // Then make a fixed copy of linearization.
    auto linearization_fixed = linearization;
    FixLinearization(depgraph, linearization_fixed);
    // Sanity check it (which includes testing whether it is topological).
    SanityCheck(depgraph, linearization_fixed);

    // FixLinearization does not modify the topological prefix of linearization.
    assert(std::equal(linearization.begin(), linearization.begin() + topo_prefix,
                      linearization_fixed.begin()));
    // This also means that if linearization was entirely topological, FixLinearization cannot have
    // modified it. This is implied by the assertion above already, but repeat it explicitly.
    if (topo_prefix == linearization.size()) {
        assert(linearization == linearization_fixed);
    }
}
