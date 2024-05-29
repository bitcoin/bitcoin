// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cluster_linearize.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <util/bitset.h>
#include <util/feefrac.h>

#include <algorithm>
#include <stdint.h>
#include <vector>
#include <utility>

using namespace cluster_linearize;

namespace {

using TestBitSet = BitSet<32>;

/** Check if a graph is acyclic. */
template<typename SetType>
bool IsAcyclic(const DepGraph<SetType>& depgraph) noexcept
{
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        if ((depgraph.Ancestors(i) & depgraph.Descendants(i)) != SetType::Singleton(i)) {
            return false;
        }
    }
    return true;
}

/** Get the minimal set of parents a transaction has (parents which are not parents
 *  of ancestors). */
template<typename SetType>
SetType GetReducedParents(const DepGraph<SetType>& depgraph, ClusterIndex i) noexcept
{
    SetType ret = depgraph.Ancestors(i);
    ret.Reset(i);
    for (auto a : ret) {
        if (ret[a]) {
            ret -= depgraph.Ancestors(a);
            ret.Set(a);
        }
    }
    return ret;
}

/** Get the minimal set of children a transaction has (children which are not children
 *  of descendants). */
template<typename SetType>
SetType GetReducedChildren(const DepGraph<SetType>& depgraph, ClusterIndex i) noexcept
{
    SetType ret = depgraph.Descendants(i);
    ret.Reset(i);
    for (auto a : ret) {
        if (ret[a]) {
            ret -= depgraph.Descendants(a);
            ret.Set(a);
        }
    }
    return ret;
}

/** Test whether adding a dependency between parent and child is valid and meaningful. */
template<typename SetType>
bool CanAddDependency(const DepGraph<SetType>& depgraph, ClusterIndex parent, ClusterIndex child) noexcept
{
    // If child is already a descendant of parent, the dependency would be redundant.
    if (depgraph.Descendants(parent)[child]) return false;
    // If child is already an ancestor of parent, the dependency would cause a cycle.
    if (depgraph.Ancestors(parent)[child]) return false;
    // If there is an ancestor of parent which is a direct parent of a descendant of child,
    // that dependency will have been redundant if a dependency between parent and child is
    // added.
    const auto& descendants = depgraph.Descendants(child);
    for (auto i : depgraph.Ancestors(parent)) {
        if (descendants.Overlaps(depgraph.Descendants(i))) {
            if (descendants.Overlaps(GetReducedChildren(depgraph, i))) return false;
        }
    }
    return true;
}

/** A formatter for a bespoke serialization for *acyclic* DepGraph objects. */
struct DepGraphFormatter
{
    /** Convert x>=0 to 2x (even), x<0 to -2x-1 (odd). */
    static uint64_t SignedToUnsigned(int64_t x) noexcept
    {
        if (x < 0) {
            return 2 * uint64_t(-(x + 1)) + 1;
        } else {
            return 2 * uint64_t(x);
        }
    }

    /** Convert even x to x/2 (>=0), odd x to -(x/2)-1 (<0). */
    static int64_t UnsignedToSigned(uint64_t x) noexcept
    {
        if (x & 1) {
            return -int64_t(x / 2) - 1;
        } else {
            return int64_t(x / 2);
        }
    }

    template <typename Stream, typename SetType>
    static void Ser(Stream& s, const DepGraph<SetType>& depgraph)
    {
        DepGraph<SetType> rebuild(depgraph.TxCount());
        for (ClusterIndex idx = 0; idx < depgraph.TxCount(); ++idx) {
            // Write size.
            s << VARINT_MODE(depgraph.FeeRate(idx).size, VarIntMode::NONNEGATIVE_SIGNED);
            // Write fee.
            s << VARINT(SignedToUnsigned(depgraph.FeeRate(idx).fee));
            // Write dependency information.
            uint64_t counter = 0; //!< How many potential parent/child relations we've iterated over.
            uint64_t offset = 0; //!< The counter value at the last actually written relation.
            for (unsigned loop = 0; loop < 2; ++loop) {
                // In loop 0 store parents among tx 0..idx-1; in loop 1 store children among those.
                SetType towrite = loop ? GetReducedChildren(depgraph, idx) : GetReducedParents(depgraph, idx);
                for (ClusterIndex i = 0; i < idx; ++i) {
                    ClusterIndex parent = loop ? idx : idx - 1 - i;
                    ClusterIndex child = loop ? idx - 1 - i : idx;
                    if (CanAddDependency(rebuild, parent, child)) {
                        ++counter;
                        if (towrite[idx - 1 - i]) {
                            rebuild.AddDependency(parent, child);
                            // The actually emitted values are differentially encoded (one value
                            // per parent/child relation).
                            s << VARINT(counter - offset);
                            offset = counter;
                        }
                    }
                }
            }
            if (counter > offset) s << uint8_t{0};
        }
        // Output a final 0 to denote the end of the graph.
        s << uint8_t{0};
    }

    template <typename Stream, typename SetType>
    void Unser(Stream& s, DepGraph<SetType>& depgraph)
    {
        depgraph = {};
        while (true) {
            // Read size. Size 0 signifies the end of the DepGraph.
            int32_t size;
            s >> VARINT_MODE(size, VarIntMode::NONNEGATIVE_SIGNED);
            size &= 0x3FFFFF; // Enough for size up to 4M.
            if (size == 0 || depgraph.TxCount() == SetType::Size()) break;
            // Read fee, encoded as a signed varint (odd means negative, even means non-negative).
            uint64_t coded_fee;
            s >> VARINT(coded_fee);
            coded_fee &= 0xFFFFFFFFFFFFF; // Enough for fee between -21M...21M BTC.
            auto fee = UnsignedToSigned(coded_fee);
            // Extend resulting graph with new transaction.
            auto idx = depgraph.AddTransaction({fee, size});
            // Read dependency information.
            uint64_t offset = 0; //!< The next encoded value.
            uint64_t counter = 0; //!< How many potential parent/child relations we've iterated over.
            for (unsigned loop = 0; loop < 2; ++loop) {
                // In loop 0 read parents among tx 0..idx-1; in loop 1 store children among those.
                bool done = false;
                for (ClusterIndex i = 0; i < idx; ++i) {
                    ClusterIndex parent = loop ? idx : idx - 1 - i;
                    ClusterIndex child = loop ? idx - 1 - i : idx;
                    if (CanAddDependency(depgraph, parent, child)) {
                        ++counter;
                        // If counter passes offset, read & decode the next differentially encoded
                        // value. If a 0 is read, this signifies the end of this transaction's
                        // dependency information.
                        if (offset < counter) {
                            uint64_t diff;
                            s >> VARINT(diff);
                            offset += diff;
                            if (diff == 0 || offset < diff) {
                                done = true;
                                break;
                            }
                        }
                        // On a match, actually add the relation.
                        if (offset == counter) depgraph.AddDependency(parent, child);
                    }
                }
                if (done) break;
            }
        }
    }
};

/** A very simple finder class for optimal candidate sets, which tries every subset. */
template<typename SetType>
class ExhaustiveCandidateFinder
{
    /** Internal dependency graph. */
    const DepGraph<SetType>& m_depgraph;
    /** Which transaction are left to include. */
    SetType m_todo;

public:
    /** Construct an SimpleOptimalCandidateFinder for a given graph. */
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

/** A simple finder class for candidate sets. */
template<typename SetType>
class SimpleCandidateFinder
{
    /** Internal dependency graph. */
    const DepGraph<SetType>& m_depgraph;
    /** Which transaction are left to include. */
    SetType m_todo;

public:
    /** Construct an SimpleOptimalCandidateFinder for a given graph. */
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
            for (auto pivot : und) {
                // If inc is empty, consider any pivot. Otherwise only consider transactions
                // that share ancestry with inc so far (which means only connected sets will be
                // considered).
                if (inc_none || inc.Overlaps(m_depgraph.Ancestors(pivot))) {
                    // Add a queue entry with pivot included.
                    SetInfo new_inc(m_depgraph, inc | (m_todo & m_depgraph.Ancestors(pivot)));
                    queue.emplace_back(new_inc.transactions, und - new_inc.transactions);
                    // Add a queue entry with pivot excluded.
                    queue.emplace_back(inc, und - m_depgraph.Descendants(pivot));
                    // Update statistics to account for the candidate new_inc.
                    if (new_inc.feerate > best.feerate) best = new_inc;
                    break;
                }
            }
        }
        return {std::move(best), max_iterations - iterations_left};
    }
};

/** Simple linearization algorithm built on SimpleCandidateFinder. */
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

/** Perform a sanity/consistency check on a DepGraph. */
template<typename SetType>
void SanityCheck(const DepGraph<SetType>& depgraph)
{
    // Consistency check between ancestors internally.
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        // Transactions include themselves as ancestors.
        assert(depgraph.Ancestors(i)[i]);
        // If a is an ancestor of b, then b's ancestors must include all of a's ancestors.
        for (auto a : depgraph.Ancestors(i)) {
            assert(depgraph.Ancestors(i).IsSupersetOf(depgraph.Ancestors(a)));
        }
    }
    // Consistency check between ancestors and descendants.
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        for (ClusterIndex j = 0; j < depgraph.TxCount(); ++j) {
            assert(depgraph.Ancestors(i)[j] == depgraph.Descendants(j)[i]);
        }
    }
    // Consistency check between reduced parents/children and ancestors/descendants.
    for (ClusterIndex i = 0; i < depgraph.TxCount(); ++i) {
        SetType parents = GetReducedParents(depgraph, i);
        SetType combined_anc = SetType::Singleton(i);
        for (auto j : parents) {
            // Transactions cannot be a parent of themselves.
            assert(j != i);
            // Parents cannot have other parents as ancestors.
            assert((depgraph.Ancestors(j) & parents) == SetType::Singleton(j));
            combined_anc |= depgraph.Ancestors(j);
        }
        // The ancestors of all parents combined must equal the ancestors.
        assert(combined_anc == depgraph.Ancestors(i));

        SetType children = GetReducedChildren(depgraph, i);
        SetType combined_desc = SetType::Singleton(i);
        for (auto j : children) {
            // Transactions cannot be a child of themselves.
            assert(j != i);
            // Children cannot have other children as descendants.
            assert((depgraph.Descendants(j) & children) == SetType::Singleton(j));
            combined_desc |= depgraph.Descendants(j);
        }
        // The descendants of all children combined must equal the descendants.
        assert(combined_desc == depgraph.Descendants(i));
    }
    // If DepGraph is acyclic, serialize + deserialize must roundtrip.
    if (IsAcyclic(depgraph)) {
        std::vector<unsigned char> ser;
        VectorWriter writer(ser, 0);
        writer << Using<DepGraphFormatter>(depgraph);
        SpanReader reader(ser);
        DepGraph<TestBitSet> decoded_depgraph;
        reader >> Using<DepGraphFormatter>(decoded_depgraph);
        assert(depgraph == decoded_depgraph);
        assert(reader.empty());
    }
}

/** Perform a sanity check on a linearization. */
template<typename SetType>
void SanityCheck(const DepGraph<SetType>& depgraph, Span<const ClusterIndex> linearization)
{
    // Check completeness.
    assert(linearization.size() == depgraph.TxCount());
    TestBitSet done;
    for (auto i : linearization) {
        // Check topology and lack of duplicates.
        assert((depgraph.Ancestors(i) - done) == TestBitSet::Singleton(i));
        done.Set(i);
    }
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
    Cluster<TestBitSet> cluster;
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    auto num_tx = provider.ConsumeIntegralInRange<ClusterIndex>(2, 32);
    cluster.resize(num_tx);
    for (auto& item : cluster) item.first.size = 1;
    // Construct the corresponding DepGraph object (also no dependencies).
    DepGraph depgraph(cluster);
    SanityCheck(depgraph);
    // Read (parent, child) pairs, and add them to the cluster and txgraph.
    LIMITED_WHILE(provider.remaining_bytes() > 0, 1024) {
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
            auto [simple, simple_iters] = smp_finder.FindCandidateSet(0x3ffff);
            assert(found.feerate >= simple.feerate);
            if (simple_iters < 0x3ffff) assert(found.feerate == simple.feerate);

            // Compare with AncestorCandidateFinder;
            auto anc = anc_finder.FindCandidateSet();
            assert(found.feerate >= anc.feerate);

            // If todo isn't too big, compare with ExhaustiveCandidateFinder.
            if (todo.Count() <= 12) {
                auto exhaustive = exh_finder.FindCandidateSet();
                assert(exhaustive.feerate == found.feerate);
                // Also compare ExhaustiveCandidateFinder with SimpleCandidateFinder (this is more
                // a test for SimpleCandidateFinder's correctness).
                assert(exhaustive.feerate >= simple.feerate);
                if (simple_iters < 0x3ffff) assert(exhaustive.feerate == simple.feerate);
            }
        }

        // Find a topologically valid subset of transactions to remove from the graph.
        auto del_set = ReadTopologicalSet(depgraph, todo, reader);
        // If we did not find anything, use found_set itself, because we should remove something.
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
        auto [simple_linearization, simple_optimal] = SimpleLinearize(depgraph, 0x3ffff);
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

FUZZ_TARGET(clusterlin_postlinearize)
{
    // Verify expected properties of PostLinearize() on arbitrary linearizations.

    // Retrieve a depgraph from the fuzz input.
    SpanReader reader(buffer);
    DepGraph<TestBitSet> depgraph;
    try {
        reader >> Using<DepGraphFormatter>(depgraph);
    } catch (const std::ios_base::failure&) {}
    MakeConnected(depgraph);

    // Retrieve a linearization from the fuzz input.
    std::vector<ClusterIndex> linearization;
    linearization = ReadLinearization(depgraph, reader);
    SanityCheck(depgraph, linearization);

    // Produce a post-processed version.
    auto post_linearization = linearization;
    PostLinearize(depgraph, post_linearization);
    SanityCheck(depgraph, linearization);

    // Compare diagrams.
    auto old_chunking = ChunkLinearization(depgraph, linearization);
    auto new_chunking = ChunkLinearization(depgraph, post_linearization);
    auto cmp = CompareChunks(new_chunking, old_chunking);
    assert(cmp >= 0);

    // The chunks that come out of postlinearizing are always connected.
    ClusterIndex idx = 0;
    for (const auto& chunk_feerate : new_chunking) {
        // Construct chunk from chunk_feerate.
        TestBitSet chunk;
        int32_t size{0};
        while (size < chunk_feerate.size) {
            chunk.Set(post_linearization[idx]);
            size += depgraph.FeeRate(post_linearization[idx]).size;
            ++idx;
        }
        assert(size == chunk_feerate.size);
        assert(idx <= post_linearization.size());
        // Test that it is connected.
        assert(depgraph.IsConnected(chunk));
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
    MakeConnected(depgraph_gen);

    // Now construct a new graph, copying the nodes, but leaving only the first parent (even
    // direction) or the first child (odd direction).
    DepGraph<TestBitSet> depgraph_tree;
    for (ClusterIndex i = 0; i < depgraph_gen.TxCount(); ++i) {
        depgraph_tree.AddTransaction(depgraph_gen.FeeRate(i));
    }
    if (direction & 1) {
        for (ClusterIndex i = 0; i < depgraph_gen.TxCount(); ++i) {
            auto children = GetReducedChildren(depgraph_gen, i);
            if (children.Any()) depgraph_tree.AddDependency(i, children.First());
        }
    } else {
        for (ClusterIndex i = 0; i < depgraph_gen.TxCount(); ++i) {
            auto parents = GetReducedParents(depgraph_gen, i);
            if (parents.Any()) depgraph_tree.AddDependency(parents.First(), i);
        }
    }

    // Retrieve a linearization from the fuzz input.
    std::vector<ClusterIndex> linearization;
    linearization = ReadLinearization(depgraph_tree, reader);
    SanityCheck(depgraph_tree, linearization);

    // Produce a post-processed version.
    auto post_linearization = linearization;
    PostLinearize(depgraph_tree, post_linearization);
    SanityCheck(depgraph_tree, linearization);

    // Compare diagrams.
    auto old_chunking = ChunkLinearization(depgraph_tree, linearization);
    auto new_chunking = ChunkLinearization(depgraph_tree, post_linearization);
    auto cmp = CompareChunks(new_chunking, old_chunking);
    assert(cmp >= 0);

    // Try to find an even better linearization; the result must be identical as post_linearization
    // ought to be optimal already with a tree-structured graph.
    auto [opt_linearization, _optimal] = Linearize(depgraph_tree, 100000, rng_seed, post_linearization);
    auto opt_chunking = ChunkLinearization(depgraph_tree, opt_linearization);
    auto cmp_opt = CompareChunks(opt_chunking, new_chunking);
    assert(cmp_opt == 0);
}

FUZZ_TARGET(clusterlin_postlinearize_moved_leaf)
{
    // Verify that taking an existing linearized, and moving a leaf to the back, potentially
    // increasing its fee, and then post-linearizing, results in something as good as the original.

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
    MakeConnected(depgraph);

    // Retrieve two linearizations from the fuzz input.
    auto lin = ReadLinearization(depgraph, reader);
    auto lin_leaf = ReadLinearization(depgraph, reader);

    // Construct a linearization identical to lin, but with the tail end of lin_leaf moved to the
    // back.
    std::vector<ClusterIndex> lin_moved;
    for (auto i : lin) {
        if (i != lin_leaf.back()) lin_moved.push_back(i);
    }
    lin_moved.push_back(lin_leaf.back());

    // Postlinearize lin_moved.
    PostLinearize(depgraph, lin_moved);

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
    MakeConnected(depgraph);

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
