// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CLUSTER_LINEARIZE_H
#define BITCOIN_CLUSTER_LINEARIZE_H

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include <random.h>
#include <span.h>
#include <util/feefrac.h>
#include <util/vecdeque.h>

namespace cluster_linearize {

/** Data type to represent transaction indices in DepGraphs and the clusters they represent. */
using DepGraphIndex = uint32_t;

/** Data structure that holds a transaction graph's preprocessed data (fee, size, ancestors,
 *  descendants). */
template<typename SetType>
class DepGraph
{
    /** Information about a single transaction. */
    struct Entry
    {
        /** Fee and size of transaction itself. */
        FeeFrac feerate;
        /** All ancestors of the transaction (including itself). */
        SetType ancestors;
        /** All descendants of the transaction (including itself). */
        SetType descendants;

        /** Equality operator (primarily for for testing purposes). */
        friend bool operator==(const Entry&, const Entry&) noexcept = default;

        /** Construct an empty entry. */
        Entry() noexcept = default;
        /** Construct an entry with a given feerate, ancestor set, descendant set. */
        Entry(const FeeFrac& f, const SetType& a, const SetType& d) noexcept : feerate(f), ancestors(a), descendants(d) {}
    };

    /** Data for each transaction. */
    std::vector<Entry> entries;

    /** Which positions are used. */
    SetType m_used;

public:
    /** Equality operator (primarily for testing purposes). */
    friend bool operator==(const DepGraph& a, const DepGraph& b) noexcept
    {
        if (a.m_used != b.m_used) return false;
        // Only compare the used positions within the entries vector.
        for (auto idx : a.m_used) {
            if (a.entries[idx] != b.entries[idx]) return false;
        }
        return true;
    }

    // Default constructors.
    DepGraph() noexcept = default;
    DepGraph(const DepGraph&) noexcept = default;
    DepGraph(DepGraph&&) noexcept = default;
    DepGraph& operator=(const DepGraph&) noexcept = default;
    DepGraph& operator=(DepGraph&&) noexcept = default;

    /** Construct a DepGraph object given another DepGraph and a mapping from old to new.
     *
     * @param depgraph   The original DepGraph that is being remapped.
     *
     * @param mapping    A span such that mapping[i] gives the position in the new DepGraph
     *                   for position i in the old depgraph. Its size must be equal to
     *                   depgraph.PositionRange(). The value of mapping[i] is ignored if
     *                   position i is a hole in depgraph (i.e., if !depgraph.Positions()[i]).
     *
     * @param pos_range  The PositionRange() for the new DepGraph. It must equal the largest
     *                   value in mapping for any used position in depgraph plus 1, or 0 if
     *                   depgraph.TxCount() == 0.
     *
     * Complexity: O(N^2) where N=depgraph.TxCount().
     */
    DepGraph(const DepGraph<SetType>& depgraph, std::span<const DepGraphIndex> mapping, DepGraphIndex pos_range) noexcept : entries(pos_range)
    {
        Assume(mapping.size() == depgraph.PositionRange());
        Assume((pos_range == 0) == (depgraph.TxCount() == 0));
        for (DepGraphIndex i : depgraph.Positions()) {
            auto new_idx = mapping[i];
            Assume(new_idx < pos_range);
            // Add transaction.
            entries[new_idx].ancestors = SetType::Singleton(new_idx);
            entries[new_idx].descendants = SetType::Singleton(new_idx);
            m_used.Set(new_idx);
            // Fill in fee and size.
            entries[new_idx].feerate = depgraph.entries[i].feerate;
        }
        for (DepGraphIndex i : depgraph.Positions()) {
            // Fill in dependencies by mapping direct parents.
            SetType parents;
            for (auto j : depgraph.GetReducedParents(i)) parents.Set(mapping[j]);
            AddDependencies(parents, mapping[i]);
        }
        // Verify that the provided pos_range was correct (no unused positions at the end).
        Assume(m_used.None() ? (pos_range == 0) : (pos_range == m_used.Last() + 1));
    }

    /** Get the set of transactions positions in use. Complexity: O(1). */
    const SetType& Positions() const noexcept { return m_used; }
    /** Get the range of positions in this DepGraph. All entries in Positions() are in [0, PositionRange() - 1]. */
    DepGraphIndex PositionRange() const noexcept { return entries.size(); }
    /** Get the number of transactions in the graph. Complexity: O(1). */
    auto TxCount() const noexcept { return m_used.Count(); }
    /** Get the feerate of a given transaction i. Complexity: O(1). */
    const FeeFrac& FeeRate(DepGraphIndex i) const noexcept { return entries[i].feerate; }
    /** Get the mutable feerate of a given transaction i. Complexity: O(1). */
    FeeFrac& FeeRate(DepGraphIndex i) noexcept { return entries[i].feerate; }
    /** Get the ancestors of a given transaction i. Complexity: O(1). */
    const SetType& Ancestors(DepGraphIndex i) const noexcept { return entries[i].ancestors; }
    /** Get the descendants of a given transaction i. Complexity: O(1). */
    const SetType& Descendants(DepGraphIndex i) const noexcept { return entries[i].descendants; }

    /** Add a new unconnected transaction to this transaction graph (in the first available
     *  position), and return its DepGraphIndex.
     *
     * Complexity: O(1) (amortized, due to resizing of backing vector).
     */
    DepGraphIndex AddTransaction(const FeeFrac& feefrac) noexcept
    {
        static constexpr auto ALL_POSITIONS = SetType::Fill(SetType::Size());
        auto available = ALL_POSITIONS - m_used;
        Assume(available.Any());
        DepGraphIndex new_idx = available.First();
        if (new_idx == entries.size()) {
            entries.emplace_back(feefrac, SetType::Singleton(new_idx), SetType::Singleton(new_idx));
        } else {
            entries[new_idx] = Entry(feefrac, SetType::Singleton(new_idx), SetType::Singleton(new_idx));
        }
        m_used.Set(new_idx);
        return new_idx;
    }

    /** Remove the specified positions from this DepGraph.
     *
     * The specified positions will no longer be part of Positions(), and dependencies with them are
     * removed. Note that due to DepGraph only tracking ancestors/descendants (and not direct
     * dependencies), if a parent is removed while a grandparent remains, the grandparent will
     * remain an ancestor.
     *
     * Complexity: O(N) where N=TxCount().
     */
    void RemoveTransactions(const SetType& del) noexcept
    {
        m_used -= del;
        // Remove now-unused trailing entries.
        while (!entries.empty() && !m_used[entries.size() - 1]) {
            entries.pop_back();
        }
        // Remove the deleted transactions from ancestors/descendants of other transactions. Note
        // that the deleted positions will retain old feerate and dependency information. This does
        // not matter as they will be overwritten by AddTransaction if they get used again.
        for (auto& entry : entries) {
            entry.ancestors &= m_used;
            entry.descendants &= m_used;
        }
    }

    /** Modify this transaction graph, adding multiple parents to a specified child.
     *
     * Complexity: O(N) where N=TxCount().
     */
    void AddDependencies(const SetType& parents, DepGraphIndex child) noexcept
    {
        Assume(m_used[child]);
        Assume(parents.IsSubsetOf(m_used));
        // Compute the ancestors of parents that are not already ancestors of child.
        SetType par_anc;
        for (auto par : parents - Ancestors(child)) {
            par_anc |= Ancestors(par);
        }
        par_anc -= Ancestors(child);
        // Bail out if there are no such ancestors.
        if (par_anc.None()) return;
        // To each such ancestor, add as descendants the descendants of the child.
        const auto& chl_des = entries[child].descendants;
        for (auto anc_of_par : par_anc) {
            entries[anc_of_par].descendants |= chl_des;
        }
        // To each descendant of the child, add those ancestors.
        for (auto dec_of_chl : Descendants(child)) {
            entries[dec_of_chl].ancestors |= par_anc;
        }
    }

    /** Compute the (reduced) set of parents of node i in this graph.
     *
     * This returns the minimal subset of the parents of i whose ancestors together equal all of
     * i's ancestors (unless i is part of a cycle of dependencies). Note that DepGraph does not
     * store the set of parents; this information is inferred from the ancestor sets.
     *
     * Complexity: O(N) where N=Ancestors(i).Count() (which is bounded by TxCount()).
     */
    SetType GetReducedParents(DepGraphIndex i) const noexcept
    {
        SetType parents = Ancestors(i);
        parents.Reset(i);
        for (auto parent : parents) {
            if (parents[parent]) {
                parents -= Ancestors(parent);
                parents.Set(parent);
            }
        }
        return parents;
    }

    /** Compute the (reduced) set of children of node i in this graph.
     *
     * This returns the minimal subset of the children of i whose descendants together equal all of
     * i's descendants (unless i is part of a cycle of dependencies). Note that DepGraph does not
     * store the set of children; this information is inferred from the descendant sets.
     *
     * Complexity: O(N) where N=Descendants(i).Count() (which is bounded by TxCount()).
     */
    SetType GetReducedChildren(DepGraphIndex i) const noexcept
    {
        SetType children = Descendants(i);
        children.Reset(i);
        for (auto child : children) {
            if (children[child]) {
                children -= Descendants(child);
                children.Set(child);
            }
        }
        return children;
    }

    /** Compute the aggregate feerate of a set of nodes in this graph.
     *
     * Complexity: O(N) where N=elems.Count().
     **/
    FeeFrac FeeRate(const SetType& elems) const noexcept
    {
        FeeFrac ret;
        for (auto pos : elems) ret += entries[pos].feerate;
        return ret;
    }

    /** Get the connected component within the subset "todo" that contains tx (which must be in
     *  todo).
     *
     * Two transactions are considered connected if they are both in `todo`, and one is an ancestor
     * of the other in the entire graph (so not just within `todo`), or transitively there is a
     * path of transactions connecting them. This does mean that if `todo` contains a transaction
     * and a grandparent, but misses the parent, they will still be part of the same component.
     *
     * Complexity: O(ret.Count()).
     */
    SetType GetConnectedComponent(const SetType& todo, DepGraphIndex tx) const noexcept
    {
        Assume(todo[tx]);
        Assume(todo.IsSubsetOf(m_used));
        auto to_add = SetType::Singleton(tx);
        SetType ret;
        do {
            SetType old = ret;
            for (auto add : to_add) {
                ret |= Descendants(add);
                ret |= Ancestors(add);
            }
            ret &= todo;
            to_add = ret - old;
        } while (to_add.Any());
        return ret;
    }

    /** Find some connected component within the subset "todo" of this graph.
     *
     * Specifically, this finds the connected component which contains the first transaction of
     * todo (if any).
     *
     * Complexity: O(ret.Count()).
     */
    SetType FindConnectedComponent(const SetType& todo) const noexcept
    {
        if (todo.None()) return todo;
        return GetConnectedComponent(todo, todo.First());
    }

    /** Determine if a subset is connected.
     *
     * Complexity: O(subset.Count()).
     */
    bool IsConnected(const SetType& subset) const noexcept
    {
        return FindConnectedComponent(subset) == subset;
    }

    /** Determine if this entire graph is connected.
     *
     * Complexity: O(TxCount()).
     */
    bool IsConnected() const noexcept { return IsConnected(m_used); }

    /** Append the entries of select to list in a topologically valid order.
     *
     * Complexity: O(select.Count() * log(select.Count())).
     */
    void AppendTopo(std::vector<DepGraphIndex>& list, const SetType& select) const noexcept
    {
        DepGraphIndex old_len = list.size();
        for (auto i : select) list.push_back(i);
        std::sort(list.begin() + old_len, list.end(), [&](DepGraphIndex a, DepGraphIndex b) noexcept {
            const auto a_anc_count = entries[a].ancestors.Count();
            const auto b_anc_count = entries[b].ancestors.Count();
            if (a_anc_count != b_anc_count) return a_anc_count < b_anc_count;
            return a < b;
        });
    }

    /** Check if this graph is acyclic. */
    bool IsAcyclic() const noexcept
    {
        for (auto i : Positions()) {
            if ((Ancestors(i) & Descendants(i)) != SetType::Singleton(i)) {
                return false;
            }
        }
        return true;
    }
};

/** A set of transactions together with their aggregate feerate. */
template<typename SetType>
struct SetInfo
{
    /** The transactions in the set. */
    SetType transactions;
    /** Their combined fee and size. */
    FeeFrac feerate;

    /** Construct a SetInfo for the empty set. */
    SetInfo() noexcept = default;

    /** Construct a SetInfo for a specified set and feerate. */
    SetInfo(const SetType& txn, const FeeFrac& fr) noexcept : transactions(txn), feerate(fr) {}

    /** Construct a SetInfo for a given transaction in a depgraph. */
    explicit SetInfo(const DepGraph<SetType>& depgraph, DepGraphIndex pos) noexcept :
        transactions(SetType::Singleton(pos)), feerate(depgraph.FeeRate(pos)) {}

    /** Construct a SetInfo for a set of transactions in a depgraph. */
    explicit SetInfo(const DepGraph<SetType>& depgraph, const SetType& txn) noexcept :
        transactions(txn), feerate(depgraph.FeeRate(txn)) {}

    /** Add a transaction to this SetInfo (which must not yet be in it). */
    void Set(const DepGraph<SetType>& depgraph, DepGraphIndex pos) noexcept
    {
        Assume(!transactions[pos]);
        transactions.Set(pos);
        feerate += depgraph.FeeRate(pos);
    }

    /** Add the transactions of other to this SetInfo (no overlap allowed). */
    SetInfo& operator|=(const SetInfo& other) noexcept
    {
        Assume(!transactions.Overlaps(other.transactions));
        transactions |= other.transactions;
        feerate += other.feerate;
        return *this;
    }

    /** Remove the transactions of other from this SetInfo (must be subset). */
    SetInfo& operator-=(const SetInfo& other) noexcept
    {
        Assume(other.transactions.IsSubsetOf(transactions));
        transactions -= other.transactions;
        feerate -= other.feerate;
        return *this;
    }

    /** Compute the difference between this and other SetInfo (which must be a subset). */
    SetInfo operator-(const SetInfo& other) noexcept
    {
        Assume(other.transactions.IsSubsetOf(transactions));
        return {transactions - other.transactions, feerate - other.feerate};
    }

    /** Construct a new SetInfo equal to this, with more transactions added (which may overlap
     *  with the existing transactions in the SetInfo). */
    [[nodiscard]] SetInfo Add(const DepGraph<SetType>& depgraph, const SetType& txn) const noexcept
    {
        return {transactions | txn, feerate + depgraph.FeeRate(txn - transactions)};
    }

    /** Swap two SetInfo objects. */
    friend void swap(SetInfo& a, SetInfo& b) noexcept
    {
        swap(a.transactions, b.transactions);
        swap(a.feerate, b.feerate);
    }

    /** Permit equality testing. */
    friend bool operator==(const SetInfo&, const SetInfo&) noexcept = default;
};

/** Compute the feerates of the chunks of linearization. */
template<typename SetType>
std::vector<FeeFrac> ChunkLinearization(const DepGraph<SetType>& depgraph, std::span<const DepGraphIndex> linearization) noexcept
{
    std::vector<FeeFrac> ret;
    for (DepGraphIndex i : linearization) {
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

/** Data structure encapsulating the chunking of a linearization, permitting removal of subsets. */
template<typename SetType>
class LinearizationChunking
{
    /** The depgraph this linearization is for. */
    const DepGraph<SetType>& m_depgraph;

    /** The linearization we started from, possibly with removed prefix stripped. */
    std::span<const DepGraphIndex> m_linearization;

    /** Chunk sets and their feerates, of what remains of the linearization. */
    std::vector<SetInfo<SetType>> m_chunks;

    /** How large a prefix of m_chunks corresponds to removed transactions. */
    DepGraphIndex m_chunks_skip{0};

    /** Which transactions remain in the linearization. */
    SetType m_todo;

    /** Fill the m_chunks variable, and remove the done prefix of m_linearization. */
    void BuildChunks() noexcept
    {
        // Caller must clear m_chunks.
        Assume(m_chunks.empty());

        // Chop off the initial part of m_linearization that is already done.
        while (!m_linearization.empty() && !m_todo[m_linearization.front()]) {
            m_linearization = m_linearization.subspan(1);
        }

        // Iterate over the remaining entries in m_linearization. This is effectively the same
        // algorithm as ChunkLinearization, but supports skipping parts of the linearization and
        // keeps track of the sets themselves instead of just their feerates.
        for (auto idx : m_linearization) {
            if (!m_todo[idx]) continue;
            // Start with an initial chunk containing just element idx.
            SetInfo add(m_depgraph, idx);
            // Absorb existing final chunks into add while they have lower feerate.
            while (!m_chunks.empty() && add.feerate >> m_chunks.back().feerate) {
                add |= m_chunks.back();
                m_chunks.pop_back();
            }
            // Remember new chunk.
            m_chunks.push_back(std::move(add));
        }
    }

public:
    /** Initialize a LinearizationSubset object for a given length of linearization. */
    explicit LinearizationChunking(const DepGraph<SetType>& depgraph LIFETIMEBOUND, std::span<const DepGraphIndex> lin LIFETIMEBOUND) noexcept :
        m_depgraph(depgraph), m_linearization(lin)
    {
        // Mark everything in lin as todo still.
        for (auto i : m_linearization) m_todo.Set(i);
        // Compute the initial chunking.
        m_chunks.reserve(depgraph.TxCount());
        BuildChunks();
    }

    /** Determine how many chunks remain in the linearization. */
    DepGraphIndex NumChunksLeft() const noexcept { return m_chunks.size() - m_chunks_skip; }

    /** Access a chunk. Chunk 0 is the highest-feerate prefix of what remains. */
    const SetInfo<SetType>& GetChunk(DepGraphIndex n) const noexcept
    {
        Assume(n + m_chunks_skip < m_chunks.size());
        return m_chunks[n + m_chunks_skip];
    }

    /** Remove some subset of transactions from the linearization. */
    void MarkDone(SetType subset) noexcept
    {
        Assume(subset.Any());
        Assume(subset.IsSubsetOf(m_todo));
        m_todo -= subset;
        if (GetChunk(0).transactions == subset) {
            // If the newly done transactions exactly match the first chunk of the remainder of
            // the linearization, we do not need to rechunk; just remember to skip one
            // additional chunk.
            ++m_chunks_skip;
            // With subset marked done, some prefix of m_linearization will be done now. How long
            // that prefix is depends on how many done elements were interspersed with subset,
            // but at least as many transactions as there are in subset.
            m_linearization = m_linearization.subspan(subset.Count());
        } else {
            // Otherwise rechunk what remains of m_linearization.
            m_chunks.clear();
            m_chunks_skip = 0;
            BuildChunks();
        }
    }

    /** Find the shortest intersection between subset and the prefixes of remaining chunks
     *  of the linearization that has a feerate not below subset's.
     *
     * This is a crucial operation in guaranteeing improvements to linearizations. If subset has
     * a feerate not below GetChunk(0)'s, then moving IntersectPrefixes(subset) to the front of
     * (what remains of) the linearization is guaranteed not to make it worse at any point.
     *
     * See https://delvingbitcoin.org/t/introduction-to-cluster-linearization/1032 for background.
     */
    SetInfo<SetType> IntersectPrefixes(const SetInfo<SetType>& subset) const noexcept
    {
        Assume(subset.transactions.IsSubsetOf(m_todo));
        SetInfo<SetType> accumulator;
        // Iterate over all chunks of the remaining linearization.
        for (DepGraphIndex i = 0; i < NumChunksLeft(); ++i) {
            // Find what (if any) intersection the chunk has with subset.
            const SetType to_add = GetChunk(i).transactions & subset.transactions;
            if (to_add.Any()) {
                // If adding that to accumulator makes us hit all of subset, we are done as no
                // shorter intersection with higher/equal feerate exists.
                accumulator.transactions |= to_add;
                if (accumulator.transactions == subset.transactions) break;
                // Otherwise update the accumulator feerate.
                accumulator.feerate += m_depgraph.FeeRate(to_add);
                // If that does result in something better, or something with the same feerate but
                // smaller, return that. Even if a longer, higher-feerate intersection exists, it
                // does not hurt to return the shorter one (the remainder of the longer intersection
                // will generally be found in the next call to Intersect, but even if not, it is not
                // required for the improvement guarantee this function makes).
                if (!(accumulator.feerate << subset.feerate)) return accumulator;
            }
        }
        return subset;
    }
};

/** Class to represent the internal state of the spanning-forest linearization algorithm.
 *
 * At all times, each dependency is marked as "active" or "inactive", with the constraint that
 * no cycle of active dependencies may exist when ignoring the direction of those dependencies.
 * So for example, the diamond (C->X->P, C->Y->P) would be considered a cycle, and those 4
 * dependencies cannot all simultaneously be active.
 *
 * The sets of transactions that are internally connected by active dependencies are called chunks.
 * Each chunk of N transactions contains exactly N-1 active dependencies (an additional one would
 * necessarily form a cycle), and thus those active dependencies form a spanning tree for the chunk.
 * The collection of all spanning trees for the entire cluster form a spanning forest. Each
 * transaction may be in its own chunk (and thus 0 dependencies are active), or all transactions
 * may form a single chunk (and thus N-1 dependencies are active).
 *
 * Each chunk has a feerate: the total fee of all transactions in it divided by the total size of
 * all transactions in it. We say the spanning forest is topological whenever no inactive
 * dependencies exist from one chunk to another chunk with lower or equal feerate. The algorithm
 * can be stopped whenever the state is topological. In this case, the output linearization
 * consists of each of the chunks, from high to low feerate, each internally ordered in an
 * arbitrary but topologically-valid way. If the spanning forest is topological, then the output
 * linearization is also topological.
 *
 * At a high level, the algorithm works by performing a sequence of the following operations:
 * - Merging:
 *   - Whenever an inactive dependency exists from a chunk to another chunk which has lower or
 *     equal feerate, that dependency can be made active, merging the two chunks.
 *   - Merging is only possible in non-topological forests, and generally helps making it
 *     topological.
 * - Splitting:
 *   - Whenever an active dependency d exists, making it inactive will result in the chunk it is in
 *     splitting in two: a bottom chunk (which d is from) and a top chunk (which d is to). This is
 *     the case because no other active dependency between the top and bottom can exist; if it did,
 *     it would form a cycle together with d.
 *   - An active dependency can be deactivated whenever its would-be top chunk has strictly higher
 *     feerate than its would-be bottom chunk.
 *   - Splitting generally helps making a forest's output linearization better, but can result in
 *     it becoming non-topological, necessitating merging steps.
 *
 * A forest is said to be optimal when neither of these operations are applicable anymore. It can
 * be shown that the output linearization for an optimal spanning forest is optimal, and that at
 * least one optimal spanning forest exists for every cluster. Note that no proof exists that an
 * optimal state will always be reached.
 *
 * To make sure the algorithm can be interrupted quickly and get a valid linearization out, merging
 * will always be prioritized over splitting:
 *
 * - Construct an initial topological spanning forest for the graph.
 * - Loop until optimal or time runs out:
 *   - Perform a splitting step.
 *   - Loop until the forest is topological:
 *     - Perform a merging step.
 * - Output the chunks from high to low feerate, each internally sorted topologically.
 *
 * Merging is always done by maximal feerate difference. This guarantees that the sequence of a
 * single split followed by merges until topological never makes the output linearization worse.
 * In addition, this allows refining the algorithm flow into:
 *
 * - Construct an initial topological spanning forest for the graph:
 *   - Start with an empty graph.
 *   - Iterate over the transactions X of the cluster in a topological order:
 *     - Add X to the graph, becoming a new singleton chunk.
 *     - Add all dependencies of X to the graph, initially all inactive.
 *     - Make the forest topological by running an upward merging sequence on X.
 * - Loop until optimal or time runs out:
 *   - Pick a dependency D to deactivate among those whose would-be top chunk has strictly higher
 *     feerate than its would-be bottom chunk.
 *   - Deactivate D, causing the chunk it is in to split into top T and bottom B.
 *   - Make the forest topological by running an upward merging sequence on T, and a downward
 *     merging sequence on B.
 * - Output the chunks from high to low feerate, each internally sorted topologically.
 *
 * Where an upward merging sequence on a transaction X is defined as:
 * - Loop:
 *   - Find the chunk C which X is in.
 *   - Iterate over all other chunks which C has a dependency on, and remember O, the
 *     lowest-feerate one among them.
 *   - If O has higher feerate than C, stop.
 *   - Activate a dependency from C to O.
 *
 * Analogously, a downward merging sequence on a transaction X is defined as:
 * - Loop:
 *   - Find the chunk C which X is in.
 *   - Iterate over all other chunks which have a dependency on C, and remember O, the
 *     highest-feerate one among them.
 *   - If O has lower feerate than C, stop.
 *   - Activate a dependency from O to C.
 *
 * What remains to be specified are two heuristics:
 *
 * - How to decide what dependency to deactivate (when splitting chunks):
 *   - Define the gain of an active dependency as
 *     (feerate(top) - feerate(chunk)) * size(top) * size(chunk), also equal to:
 *     fee(top) * size(chunk) - fee(chunk) * size(top).
 *   - The first encountered dependency within a chunk among those with maximal gain is
 *     deactivated, if that gain is strictly positive.
 *
 * - How to decide what dependency to activate (when merging chunks):
 *   - Currently, the first encountered dependency between the two maximum-feerate-difference chunks
 *     is activated. This will be changed in a future commit.
 */
template<typename SetType>
class SpanningForestState
{
private:
    /** Data type to represent indexing into m_tx_data. */
    using TxIdx = uint32_t;
    /** Data type to represent indexing into m_dep_data. */
    using DepIdx = uint32_t;

    /** Structure with information about a single transaction. For transactions that are the
     *  representative for the chunk they are in, this also stores chunk information. */
    struct TxData {
        /** The position in the input/original linearization. Immutable after construction. */
        DepGraphIndex original_pos;
        /** The dependencies to children of this transaction. Immutable after construction. */
        std::vector<DepIdx> child_deps;
        /** The set of parent transactions of this transaction. Immutable after construction. */
        SetType parents;
        /** The set of child transactions of this transaction. Immutable after construction. */
        SetType children;
        /** The set of parent transactions of this transaction reachable through active
         *  dependencies. */
        SetType active_parents;
        /** Which transaction holds the chunk_setinfo for the chunk this transaction is in
         *  (the representative for the chunk). */
        TxIdx chunk_rep;
        /** (Only if this transaction is the representative for the chunk it is in) the total
         *  chunk set and feerate. */
        SetInfo<SetType> chunk_setinfo;
    };

    /** Structure with information about a single dependency. */
    struct DepData {
        /** Whether this dependency is active. */
        bool active;
        /** What the parent and child transactions are. Immutable after construction. */
        TxIdx parent, child;
        /** (Only if this dependency is active) the would-be top chunk and its feerate that would
         *  be formed if this dependency were to be deactivated. */
        SetInfo<SetType> top_setinfo;
    };

    /** The set of all transactions in the cluster. */
    SetType m_transactions;
    /** Information about each transaction (and chunks). Indexed by TxIdx. */
    std::vector<TxData> m_tx_data;
    /** Information about each dependency. Indexed by DepIdx. */
    std::vector<DepData> m_dep_data;

    /** The number of activations/deactivations performed. */
    uint64_t m_operations{0};

    /** Walk a chunk, starting from transaction start. visit_tx(idx) is called for each encountered
     *  transaction. visit_dep_down(dep) is called for each encountered dependency that is traversed
     *  in the parent-to-child (downward) direction. */
    void Walk(TxIdx start, std::invocable<TxData&> auto visit_tx, std::invocable<DepData&> auto visit_dep_down) noexcept
    {
        /** The set of transactions we still have to process. */
        SetType todo = SetType::Singleton(start);
        /** The set of transactions we have already processed. */
        SetType done;
        do {
            for (auto tx_idx : todo) {
                // Mark the transaction as processed, and invoke the visitor for it.
                auto& tx_data = m_tx_data[tx_idx];
                done.Set(tx_idx);
                visit_tx(tx_data);
                // Mark all active parents as to be processed.
                todo |= tx_data.active_parents;
                todo -= done;
                // Iterate over all its active child dependencies.
                auto child_deps = std::span{tx_data.child_deps};
                for (auto dep_idx : child_deps) {
                    auto& dep_entry = m_dep_data[dep_idx];
                    Assume(dep_entry.parent == tx_idx);
                    if (!dep_entry.active) continue;
                    // If this is the first time reaching the child, mark it as todo, and invoke
                    // the downward dependency visitor for it. We do not need to check if it isn't
                    // already in todo here, because there cannot be multiple dependencies that
                    // reach the same transaction; the !done check is purely to prevent travelling
                    // an already-travelled dependency back in reverse direction.
                    if (!done[dep_entry.child]) {
                        Assume(!todo[dep_entry.child]);
                        todo.Set(dep_entry.child);
                        visit_dep_down(dep_entry);
                    }
                }
            }
        } while (todo.Any());
    }

    /** Make a specified inactive dependency active. Returns the merged chunk representative. */
    TxIdx Activate(DepIdx dep_idx) noexcept
    {
        auto& dep_data = m_dep_data[dep_idx];
        Assume(!dep_data.active);
        auto& child_tx_data = m_tx_data[dep_data.child];
        auto& parent_tx_data = m_tx_data[dep_data.parent];

        // Gather information about the parent and child chunks.
        Assume(parent_tx_data.chunk_rep != child_tx_data.chunk_rep);
        auto& par_chunk_data = m_tx_data[parent_tx_data.chunk_rep];
        auto& chl_chunk_data = m_tx_data[child_tx_data.chunk_rep];
        TxIdx top_rep = parent_tx_data.chunk_rep;
        auto top_part = par_chunk_data.chunk_setinfo;
        auto bottom_part = chl_chunk_data.chunk_setinfo;
        // Update the parent chunk to also contain the child.
        par_chunk_data.chunk_setinfo |= bottom_part;
        // Add bottom component to top transactions.
        Walk(dep_data.parent,
             [](TxData&) noexcept {},
             [&](DepData& depdata) noexcept { depdata.top_setinfo |= bottom_part; });
        // Add top component to bottom transactions.
        Walk(dep_data.child,
             [&](TxData& txdata) noexcept { txdata.chunk_rep = top_rep; },
             [&](DepData& depdata) noexcept { depdata.top_setinfo |= top_part; });
        // Make active.
        dep_data.active = true;
        dep_data.top_setinfo = top_part;
        child_tx_data.active_parents.Set(dep_data.parent);
        ++m_operations;
        return top_rep;
    }

    /** Make a specified active dependency inactive. */
    void Deactivate(DepIdx dep_idx) noexcept
    {
        auto& dep_data = m_dep_data[dep_idx];
        Assume(dep_data.active);
        auto& child_tx_data = m_tx_data[dep_data.child];
        auto& parent_tx_data = m_tx_data[dep_data.parent];
        // Make inactive.
        dep_data.active = false;
        child_tx_data.active_parents.Reset(dep_data.parent);
        // Update representatives.
        auto& chunk_data = m_tx_data[parent_tx_data.chunk_rep];
        auto top_part = dep_data.top_setinfo;
        auto bottom_part = chunk_data.chunk_setinfo - top_part;
        chunk_data.chunk_setinfo = top_part;
        TxIdx bottom_rep = dep_data.child;
        auto& bottom_chunk_data = m_tx_data[bottom_rep];
        bottom_chunk_data.chunk_setinfo = bottom_part;
        TxIdx top_rep = dep_data.parent;
        auto& top_chunk_data = m_tx_data[top_rep];
        top_chunk_data.chunk_setinfo = top_part;
        // Remove bottom component from top transactions, and make top_rep the representative for
        // all of them.
        Walk(dep_data.parent,
             [&](TxData& txdata) noexcept { txdata.chunk_rep = top_rep; },
             [&](DepData& depdata) noexcept { depdata.top_setinfo -= bottom_part; });
        // Remove top component from bottom transactions, and make bottom_rep the representative
        // for all of them.
        Walk(dep_data.child,
             [&](TxData& txdata) noexcept { txdata.chunk_rep = bottom_rep; },
             [&](DepData& depdata) noexcept { depdata.top_setinfo -= top_part; });
        ++m_operations;
    }

    /** Activate a dependency from the chunk represented by bottom_rep to the chunk represented by
     *  top_rep. Such a dependency must exist. Return the representative of the merged chunk. */
    TxIdx MergeChunks(TxIdx top_rep, TxIdx bottom_rep) noexcept
    {
        auto& top_chunk = m_tx_data[top_rep];
        Assume(top_chunk.chunk_rep == top_rep);
        auto& bottom_chunk = m_tx_data[bottom_rep];
        Assume(bottom_chunk.chunk_rep == bottom_rep);
        // Activate the first dependency between bottom_chunk and top_chunk.
        for (auto tx : top_chunk.chunk_setinfo.transactions) {
            auto& tx_data = m_tx_data[tx];
            if (tx_data.children.Overlaps(bottom_chunk.chunk_setinfo.transactions)) {
                for (auto dep : tx_data.child_deps) {
                    auto& dep_data = m_dep_data[dep];
                    if (bottom_chunk.chunk_setinfo.transactions[dep_data.child]) {
                        Assume(!dep_data.active);
                        return Activate(dep);
                    }
                }
                break;
            }
        }
        Assume(false);
        return TxIdx(-1);
    }

    /** Perform an upward or downward merge sequence on the specified transaction. */
    template<bool DownWard>
    void MergeSequence(TxIdx tx_idx) noexcept
    {
        auto chunk_rep = m_tx_data[tx_idx].chunk_rep;
        while (true) {
            /** Information about the chunk that tx_idx is currently in. */
            auto& chunk_data = m_tx_data[chunk_rep];
            SetType chunk_txn = chunk_data.chunk_setinfo.transactions;
            // Iterate over all transactions in the chunk, figuring out which other chunk each
            // depends on, but only testing each other chunk once. For those depended-on chunks,
            // remember the highest-feerate (if DownWard) or lowest-feerate (if !DownWard) one.
            // If multiple equal-feerate candidate chunks to merge with exist, pick the first one
            // among them.
            SetType explored = chunk_txn;
            FeeFrac best_other_chunk_feerate;
            TxIdx best_other_chunk_rep = TxIdx(-1);
            for (auto tx : chunk_txn) {
                auto& tx_data = m_tx_data[tx];
                auto unreached = (DownWard ? tx_data.children : tx_data.parents) - explored;
                while (unreached.Any()) {
                    auto chunk_rep = m_tx_data[unreached.First()].chunk_rep;
                    auto& reached = m_tx_data[m_tx_data[unreached.First()].chunk_rep].chunk_setinfo;
                    explored |= reached.transactions;
                    unreached -= explored;
                    auto cmp = DownWard ? FeeRateCompare(best_other_chunk_feerate, reached.feerate)
                                        : FeeRateCompare(reached.feerate, best_other_chunk_feerate);
                    if (cmp > 0) continue;
                    if (cmp < 0 || best_other_chunk_rep == TxIdx(-1)) {
                        best_other_chunk_feerate = reached.feerate;
                        best_other_chunk_rep = chunk_rep;
                    }
                }
            }
            // Stop if all of the depended-on chunks have a lower/higher feerate than the chunk
            // that tx_idx is in.
            if (best_other_chunk_feerate.IsEmpty()) break;
            if (DownWard && best_other_chunk_feerate << chunk_data.chunk_setinfo.feerate) break;
            if (!DownWard && best_other_chunk_feerate >> chunk_data.chunk_setinfo.feerate) break;
            Assume(best_other_chunk_rep != TxIdx(-1));
            if constexpr (DownWard) {
                chunk_rep = MergeChunks(chunk_rep, best_other_chunk_rep);
            } else {
                chunk_rep = MergeChunks(best_other_chunk_rep, chunk_rep);
            }
        }
    }

    /** Split a chunk, and then merge the resulting two chunks to make the graph topological
     *  again. */
    void Improve(DepIdx dep_idx) noexcept
    {
        auto& dep_data = m_dep_data[dep_idx];
        Assume(dep_data.active);
        // Deactivate the specified dependency, splitting it into two new chunks: a top containing
        // the parent, and a bottom containing the child. The top should have a higher feerate.
        Deactivate(dep_idx);
        // Merge the top chunk with lower-feerate chunks it depends on (which may be the bottom it
        // was just split from, or other pre-existing chunks).
        MergeSequence<false>(dep_data.parent);
        // Merge the bottom chunk with higher-feerate chunks that depend on it.
        MergeSequence<true>(dep_data.child);
    }

public:
    /** Construct an initial topological spanning forest for the given DepGraph, and optionally an
     *  existing linearization for it. */
    explicit SpanningForestState(const DepGraph<SetType>& depgraph, std::span<const DepGraphIndex> old_linearization = {}) noexcept
    {
        m_transactions = depgraph.Positions();
        auto num_transactions = m_transactions.Count();
        // If no existing linearization is provided, construct an initial topological ordering.
        std::vector<DepGraphIndex> load_order;
        if (old_linearization.empty()) {
            load_order.reserve(m_transactions.Count());
            for (auto i : m_transactions) load_order.push_back(i);
            std::sort(load_order.begin(), load_order.end(), [&](TxIdx a, TxIdx b) noexcept { return depgraph.Ancestors(a).Count() < depgraph.Ancestors(b).Count(); });
            old_linearization = std::span{load_order};
        }
        // Add transactions one by one, in order of existing linearization.
        m_tx_data.resize(depgraph.PositionRange());
        m_dep_data.reserve(((num_transactions + 1) / 2) * (num_transactions / 2));
        DepGraphIndex num_done = 0;
        for (DepGraphIndex tx : old_linearization) {
            // Fill in transaction data.
            auto& tx_data = m_tx_data[tx];
            tx_data.original_pos = num_done++;
            tx_data.chunk_rep = tx;
            tx_data.chunk_setinfo.transactions = SetType::Singleton(tx);
            tx_data.chunk_setinfo.feerate = depgraph.FeeRate(tx);
            // Add its dependencies.
            SetType parents = depgraph.GetReducedParents(tx);
            for (auto par : parents) {
                auto& par_tx_data = m_tx_data[par];
                auto dep_idx = m_dep_data.size();
                // Construct new dependency.
                auto& dep = m_dep_data.emplace_back();
                dep.active = false;
                dep.parent = par;
                dep.child = tx;
                // Add it as parent of the child.
                tx_data.parents.Set(par);
                // Add it as child of the parent.
                par_tx_data.child_deps.push_back(dep_idx);
                par_tx_data.children.Set(tx);
            }
            // Start a merge sequence on the new transaction to make the graph topological.
            MergeSequence<false>(tx);
        }
    }

    /** Try to improve the forest. Returns false if it is optimal, true otherwise. */
    bool Step() noexcept
    {
        // Iterate over all transactions (only processing those which are chunk representatives).
        for (auto chunk : m_transactions) {
            auto& chunk_data = m_tx_data[chunk];
            // If this is not a chunk representative, skip.
            if (chunk_data.chunk_rep != chunk) continue;
            // Remember the best dependency seen so far, together with its top feerate.
            DepIdx candidate_dep = DepIdx(-1);
            FeeFrac candidate_top_feerate;
            // Iterate over all transactions.
            for (auto tx : chunk_data.chunk_setinfo.transactions) {
                const auto& tx_data = m_tx_data[tx];
                // Iterate over all active child dependencies of the transaction.
                const auto children = std::span{tx_data.child_deps};
                for (DepIdx dep_idx : children) {
                    const auto& dep_data = m_dep_data[dep_idx];
                    if (!dep_data.active) continue;
                    // Define gain(top) = fee(top)*size(chunk) - fee(chunk)*size(top).
                    //                  = (feerate(top) - feerate(chunk)) * size(top) * size(chunk).
                    // Thus:
                    //
                    //     gain(top1) > gain(top2)
                    // <=>   fee(top1)*size(chunk) - fee(chunk)*size(top1)
                    //     > fee(top2)*size(chunk) - fee(chunk)*size(top2)
                    // <=> (fee(top1)-fee(top2))*size(chunk) > fee(chunk)*(size(top1)-size(top2))
                    //
                    // If size(top1)>size(top2), this corresponds to feerate(top1-top2) > feerate(chunk),
                    // so we can use FeeRateCompare to discover if dep_data.top_setinfo has better
                    // gain than best_top_feerate. As FeeRateCompare() is actually implemented by
                    // checking the sign of the cross-product, it even works when
                    // size(top1) <= size(top2). When no candidate exists so far, this is equal
                    // to comparing the feerate with the chunk directly (= the sign of gain(top)).
                    auto cmp = FeeRateCompare(dep_data.top_setinfo.feerate - candidate_top_feerate,
                                              chunk_data.chunk_setinfo.feerate);
                    if (cmp <= 0) continue;
                    // Remember this as our (new) candidate dependency.
                    candidate_dep = dep_idx;
                    candidate_top_feerate = dep_data.top_setinfo.feerate;
                }
            }
            // If a candidate with positive gain was found, activate it.
            if (candidate_dep != DepIdx(-1)) {
                Assume(candidate_dep != DepIdx(-1));
                Improve(candidate_dep);
                return true;
            }
        }
        // No improvable chunk was found, we are done.
        return false;
    }

    /** Construct a topologically-valid linearization from the current forest state. */
    std::vector<DepGraphIndex> GetLinearization() const noexcept
    {
        /** The output linearization. */
        std::vector<DepGraphIndex> ret;
        /** A heap with all chunks (by representative) that can currently be included, sorted by
         *  chunk feerate. */
        std::vector<TxIdx> heap_chunks;
        /** Information about chunks. Only used for chunk representatives, and counts the number
         *  of unmet dependencies this chunk has on other chunks (not including dependencies within
         *  the chunk itself). */
        std::vector<TxIdx> chunk_deps(m_tx_data.size(), 0);
        /** The set of all chunk representatives. */
        SetType chunk_reps;
        // Populate chunk_deps[c] with the number of out-of-chunk dependencies the chunk has.
        for (TxIdx chl_idx : m_transactions) {
            const auto& chl_data = m_tx_data[chl_idx];
            auto chl_chunk_rep = chl_data.chunk_rep;
            chunk_reps.Set(chl_chunk_rep);
            for (auto par_idx : chl_data.parents) {
                auto par_chunk_rep = m_tx_data[par_idx].chunk_rep;
                chunk_deps[chl_chunk_rep] += (par_chunk_rep != chl_chunk_rep);
            }
        }
        // Construct a heap with all chunks that have no out-of-chunk dependencies.
        /** Comparison function for the heap. */
        auto chunk_cmp_fn = [&](TxIdx a, TxIdx b) noexcept {
            auto& chunk_a = m_tx_data[a];
            auto& chunk_b = m_tx_data[b];
            Assume(chunk_a.chunk_rep == a);
            Assume(chunk_b.chunk_rep == b);
            if (chunk_a.chunk_setinfo.feerate != chunk_b.chunk_setinfo.feerate) {
                return chunk_a.chunk_setinfo.feerate < chunk_b.chunk_setinfo.feerate;
            }
            // Compare the chunks' transaction sets lexicographically as tiebreaker.
            auto a_first = chunk_a.chunk_setinfo.transactions.First();
            auto b_first = chunk_b.chunk_setinfo.transactions.First();
            Assume((a == b) == (a_first == b_first));
            return a_first < b_first;
        };
        for (TxIdx chunk_rep : chunk_reps) {
            if (chunk_deps[chunk_rep] == 0) heap_chunks.push_back(chunk_rep);
        }
        std::make_heap(heap_chunks.begin(), heap_chunks.end(), chunk_cmp_fn);
        // Pop chunks off the heap, highest-feerate ones first.
        while (!heap_chunks.empty()) {
            auto chunk_rep = heap_chunks.front();
            std::pop_heap(heap_chunks.begin(), heap_chunks.end(), chunk_cmp_fn);
            heap_chunks.pop_back();
            Assume(m_tx_data[chunk_rep].chunk_rep == chunk_rep);
            Assume(chunk_deps[chunk_rep] == 0);
            const auto& chunk_txn = m_tx_data[chunk_rep].chunk_setinfo.transactions;
            // Append all transactions in the chunk to ret, and decrement chunk dependency counts.
            auto old_len = ret.size();
            for (TxIdx tx_idx : chunk_txn) {
                const auto& tx_data = m_tx_data[tx_idx];
                // Add transaction to output linearization.
                ret.push_back(tx_idx);
                // Decrement chunk dependency counts.
                for (TxIdx chl_idx : tx_data.children) {
                    auto& chl_data = m_tx_data[chl_idx];
                    if (chl_data.chunk_rep != chunk_rep) {
                        Assume(chunk_deps[chl_data.chunk_rep] > 0);
                        if (--chunk_deps[chl_data.chunk_rep] == 0) {
                            // Child chunk has no dependencies left. Add it to the heap.
                            heap_chunks.push_back(chl_data.chunk_rep);
                            std::push_heap(heap_chunks.begin(), heap_chunks.end(), chunk_cmp_fn);
                        }
                    }
                }
            }
            // Sort the transactions within the chunk according to the order they had in the
            // original linearization.
            std::sort(ret.begin() + old_len, ret.end(), [&](TxIdx a, TxIdx b) noexcept {
                return m_tx_data[a].original_pos < m_tx_data[b].original_pos;
            });
        }
        Assume(ret.size() == m_transactions.Count());
        return ret;
    }

    /** Determine how much work was performed so far. */
    uint64_t GetOperations() const noexcept { return m_operations; }
};

/** Find or improve a linearization for a cluster.
 *
 * @param[in] depgraph            Dependency graph of the cluster to be linearized.
 * @param[in] max_iterations      Upper bound on the amount of work that will be done.
 * @param[in] rng_seed            A random number seed to control search order. This prevents peers
 *                                from predicting exactly which clusters would be hard for us to
 *                                linearize.
 * @param[in] old_linearization   An existing linearization for the cluster (which must be
 *                                topologically valid), or empty.
 * @return                        A pair of:
 *                                - The resulting linearization. It is guaranteed to be at least as
 *                                  good (in the feerate diagram sense) as old_linearization.
 *                                - A boolean indicating whether the result is guaranteed to be
 *                                  optimal.
 */
template<typename SetType>
std::pair<std::vector<DepGraphIndex>, bool> Linearize(const DepGraph<SetType>& depgraph, uint64_t max_iterations, uint64_t rng_seed, std::span<const DepGraphIndex> old_linearization = {}) noexcept
{
    (void)rng_seed; // Unused for now.
    /** Initialize a spanning forest data structure for this cluster. */
    SpanningForestState forest(depgraph, old_linearization);
    // Make improvement steps to it until we hit the max_iterations limit, or an optimal result
    // is found.
    while (true) {
        if (forest.GetOperations() >= max_iterations) return {forest.GetLinearization(), false};
        if (!forest.Step()) break;
    }
    return {forest.GetLinearization(), true};
}

/** Improve a given linearization.
 *
 * @param[in]     depgraph       Dependency graph of the cluster being linearized.
 * @param[in,out] linearization  On input, an existing linearization for depgraph. On output, a
 *                               potentially better linearization for the same graph.
 *
 * Postlinearization guarantees:
 * - The resulting chunks are connected.
 * - If the input has a tree shape (either all transactions have at most one child, or all
 *   transactions have at most one parent), the result is optimal.
 * - Given a linearization L1 and a leaf transaction T in it. Let L2 be L1 with T moved to the end,
 *   optionally with its fee increased. Let L3 be the postlinearization of L2. L3 will be at least
 *   as good as L1. This means that replacing transactions with same-size higher-fee transactions
 *   will not worsen linearizations through a "drop conflicts, append new transactions,
 *   postlinearize" process.
 */
template<typename SetType>
void PostLinearize(const DepGraph<SetType>& depgraph, std::span<DepGraphIndex> linearization)
{
    // This algorithm performs a number of passes (currently 2); the even ones operate from back to
    // front, the odd ones from front to back. Each results in an equal-or-better linearization
    // than the one started from.
    // - One pass in either direction guarantees that the resulting chunks are connected.
    // - Each direction corresponds to one shape of tree being linearized optimally (forward passes
    //   guarantee this for graphs where each transaction has at most one child; backward passes
    //   guarantee this for graphs where each transaction has at most one parent).
    // - Starting with a backward pass guarantees the moved-tree property.
    //
    // During an odd (forward) pass, the high-level operation is:
    // - Start with an empty list of groups L=[].
    // - For every transaction i in the old linearization, from front to back:
    //   - Append a new group C=[i], containing just i, to the back of L.
    //   - While L has at least one group before C, and the group immediately before C has feerate
    //     lower than C:
    //     - If C depends on P:
    //       - Merge P into C, making C the concatenation of P+C, continuing with the combined C.
    //     - Otherwise:
    //       - Swap P with C, continuing with the now-moved C.
    // - The output linearization is the concatenation of the groups in L.
    //
    // During even (backward) passes, i iterates from the back to the front of the existing
    // linearization, and new groups are prepended instead of appended to the list L. To enable
    // more code reuse, both passes append groups, but during even passes the meanings of
    // parent/child, and of high/low feerate are reversed, and the final concatenation is reversed
    // on output.
    //
    // In the implementation below, the groups are represented by singly-linked lists (pointing
    // from the back to the front), which are themselves organized in a singly-linked circular
    // list (each group pointing to its predecessor, with a special sentinel group at the front
    // that points back to the last group).
    //
    // Information about transaction t is stored in entries[t + 1], while the sentinel is in
    // entries[0].

    /** Index of the sentinel in the entries array below. */
    static constexpr DepGraphIndex SENTINEL{0};
    /** Indicator that a group has no previous transaction. */
    static constexpr DepGraphIndex NO_PREV_TX{0};


    /** Data structure per transaction entry. */
    struct TxEntry
    {
        /** The index of the previous transaction in this group; NO_PREV_TX if this is the first
         *  entry of a group. */
        DepGraphIndex prev_tx;

        // The fields below are only used for transactions that are the last one in a group
        // (referred to as tail transactions below).

        /** Index of the first transaction in this group, possibly itself. */
        DepGraphIndex first_tx;
        /** Index of the last transaction in the previous group. The first group (the sentinel)
         *  points back to the last group here, making it a singly-linked circular list. */
        DepGraphIndex prev_group;
        /** All transactions in the group. Empty for the sentinel. */
        SetType group;
        /** All dependencies of the group (descendants in even passes; ancestors in odd ones). */
        SetType deps;
        /** The combined fee/size of transactions in the group. Fee is negated in even passes. */
        FeeFrac feerate;
    };

    // As an example, consider the state corresponding to the linearization [1,0,3,2], with
    // groups [1,0,3] and [2], in an odd pass. The linked lists would be:
    //
    //                                        +-----+
    //                                 0<-P-- | 0 S | ---\     Legend:
    //                                        +-----+    |
    //                                           ^       |     - digit in box: entries index
    //             /--------------F---------+    G       |       (note: one more than tx value)
    //             v                         \   |       |     - S: sentinel group
    //          +-----+        +-----+        +-----+    |          (empty feerate)
    //   0<-P-- | 2   | <--P-- | 1   | <--P-- | 4 T |    |     - T: tail transaction, contains
    //          +-----+        +-----+        +-----+    |          fields beyond prev_tv.
    //                                           ^       |     - P: prev_tx reference
    //                                           G       G     - F: first_tx reference
    //                                           |       |     - G: prev_group reference
    //                                        +-----+    |
    //                                 0<-P-- | 3 T | <--/
    //                                        +-----+
    //                                         ^   |
    //                                         \-F-/
    //
    // During an even pass, the diagram above would correspond to linearization [2,3,0,1], with
    // groups [2] and [3,0,1].

    std::vector<TxEntry> entries(depgraph.PositionRange() + 1);

    // Perform two passes over the linearization.
    for (int pass = 0; pass < 2; ++pass) {
        int rev = !(pass & 1);
        // Construct a sentinel group, identifying the start of the list.
        entries[SENTINEL].prev_group = SENTINEL;
        Assume(entries[SENTINEL].feerate.IsEmpty());

        // Iterate over all elements in the existing linearization.
        for (DepGraphIndex i = 0; i < linearization.size(); ++i) {
            // Even passes are from back to front; odd passes from front to back.
            DepGraphIndex idx = linearization[rev ? linearization.size() - 1 - i : i];
            // Construct a new group containing just idx. In even passes, the meaning of
            // parent/child and high/low feerate are swapped.
            DepGraphIndex cur_group = idx + 1;
            entries[cur_group].group = SetType::Singleton(idx);
            entries[cur_group].deps = rev ? depgraph.Descendants(idx): depgraph.Ancestors(idx);
            entries[cur_group].feerate = depgraph.FeeRate(idx);
            if (rev) entries[cur_group].feerate.fee = -entries[cur_group].feerate.fee;
            entries[cur_group].prev_tx = NO_PREV_TX; // No previous transaction in group.
            entries[cur_group].first_tx = cur_group; // Transaction itself is first of group.
            // Insert the new group at the back of the groups linked list.
            entries[cur_group].prev_group = entries[SENTINEL].prev_group;
            entries[SENTINEL].prev_group = cur_group;

            // Start merge/swap cycle.
            DepGraphIndex next_group = SENTINEL; // We inserted at the end, so next group is sentinel.
            DepGraphIndex prev_group = entries[cur_group].prev_group;
            // Continue as long as the current group has higher feerate than the previous one.
            while (entries[cur_group].feerate >> entries[prev_group].feerate) {
                // prev_group/cur_group/next_group refer to (the last transactions of) 3
                // consecutive entries in groups list.
                Assume(cur_group == entries[next_group].prev_group);
                Assume(prev_group == entries[cur_group].prev_group);
                // The sentinel has empty feerate, which is neither higher or lower than other
                // feerates. Thus, the while loop we are in here guarantees that cur_group and
                // prev_group are not the sentinel.
                Assume(cur_group != SENTINEL);
                Assume(prev_group != SENTINEL);
                if (entries[cur_group].deps.Overlaps(entries[prev_group].group)) {
                    // There is a dependency between cur_group and prev_group; merge prev_group
                    // into cur_group. The group/deps/feerate fields of prev_group remain unchanged
                    // but become unused.
                    entries[cur_group].group |= entries[prev_group].group;
                    entries[cur_group].deps |= entries[prev_group].deps;
                    entries[cur_group].feerate += entries[prev_group].feerate;
                    // Make the first of the current group point to the tail of the previous group.
                    entries[entries[cur_group].first_tx].prev_tx = prev_group;
                    // The first of the previous group becomes the first of the newly-merged group.
                    entries[cur_group].first_tx = entries[prev_group].first_tx;
                    // The previous group becomes whatever group was before the former one.
                    prev_group = entries[prev_group].prev_group;
                    entries[cur_group].prev_group = prev_group;
                } else {
                    // There is no dependency between cur_group and prev_group; swap them.
                    DepGraphIndex preprev_group = entries[prev_group].prev_group;
                    // If PP, P, C, N were the old preprev, prev, cur, next groups, then the new
                    // layout becomes [PP, C, P, N]. Update prev_groups to reflect that order.
                    entries[next_group].prev_group = prev_group;
                    entries[prev_group].prev_group = cur_group;
                    entries[cur_group].prev_group = preprev_group;
                    // The current group remains the same, but the groups before/after it have
                    // changed.
                    next_group = prev_group;
                    prev_group = preprev_group;
                }
            }
        }

        // Convert the entries back to linearization (overwriting the existing one).
        DepGraphIndex cur_group = entries[0].prev_group;
        DepGraphIndex done = 0;
        while (cur_group != SENTINEL) {
            DepGraphIndex cur_tx = cur_group;
            // Traverse the transactions of cur_group (from back to front), and write them in the
            // same order during odd passes, and reversed (front to back) in even passes.
            if (rev) {
                do {
                    *(linearization.begin() + (done++)) = cur_tx - 1;
                    cur_tx = entries[cur_tx].prev_tx;
                } while (cur_tx != NO_PREV_TX);
            } else {
                do {
                    *(linearization.end() - (++done)) = cur_tx - 1;
                    cur_tx = entries[cur_tx].prev_tx;
                } while (cur_tx != NO_PREV_TX);
            }
            cur_group = entries[cur_group].prev_group;
        }
        Assume(done == linearization.size());
    }
}

/** Merge two linearizations for the same cluster into one that is as good as both.
 *
 * Complexity: O(N^2) where N=depgraph.TxCount(); O(N) if both inputs are identical.
 */
template<typename SetType>
std::vector<DepGraphIndex> MergeLinearizations(const DepGraph<SetType>& depgraph, std::span<const DepGraphIndex> lin1, std::span<const DepGraphIndex> lin2)
{
    Assume(lin1.size() == depgraph.TxCount());
    Assume(lin2.size() == depgraph.TxCount());

    /** Chunkings of what remains of both input linearizations. */
    LinearizationChunking chunking1(depgraph, lin1), chunking2(depgraph, lin2);
    /** Output linearization. */
    std::vector<DepGraphIndex> ret;
    if (depgraph.TxCount() == 0) return ret;
    ret.reserve(depgraph.TxCount());

    while (true) {
        // As long as we are not done, both linearizations must have chunks left.
        Assume(chunking1.NumChunksLeft() > 0);
        Assume(chunking2.NumChunksLeft() > 0);
        // Find the set to output by taking the best remaining chunk, and then intersecting it with
        // prefixes of remaining chunks of the other linearization.
        SetInfo<SetType> best;
        const auto& lin1_firstchunk = chunking1.GetChunk(0);
        const auto& lin2_firstchunk = chunking2.GetChunk(0);
        if (lin2_firstchunk.feerate >> lin1_firstchunk.feerate) {
            best = chunking1.IntersectPrefixes(lin2_firstchunk);
        } else {
            best = chunking2.IntersectPrefixes(lin1_firstchunk);
        }
        // Append the result to the output and mark it as done.
        depgraph.AppendTopo(ret, best.transactions);
        chunking1.MarkDone(best.transactions);
        if (chunking1.NumChunksLeft() == 0) break;
        chunking2.MarkDone(best.transactions);
    }

    Assume(ret.size() == depgraph.TxCount());
    return ret;
}

/** Make linearization topological, retaining its ordering where possible. */
template<typename SetType>
void FixLinearization(const DepGraph<SetType>& depgraph, std::span<DepGraphIndex> linearization) noexcept
{
    // This algorithm can be summarized as moving every element in the linearization backwards
    // until it is placed after all its ancestors.
    SetType done;
    const auto len = linearization.size();
    // Iterate over the elements of linearization from back to front (i is distance from back).
    for (DepGraphIndex i = 0; i < len; ++i) {
        /** The element at that position. */
        DepGraphIndex elem = linearization[len - 1 - i];
        /** j represents how far from the back of the linearization elem should be placed. */
        DepGraphIndex j = i;
        // Figure out which elements need to be moved before elem.
        SetType place_before = done & depgraph.Ancestors(elem);
        // Find which position to place elem in (updating j), continuously moving the elements
        // in between forward.
        while (place_before.Any()) {
            // j cannot be 0 here; if it was, then there was necessarily nothing earlier which
            // elem needs to be placed before anymore, and place_before would be empty.
            Assume(j > 0);
            auto to_swap = linearization[len - 1 - (j - 1)];
            place_before.Reset(to_swap);
            linearization[len - 1 - (j--)] = to_swap;
        }
        // Put elem in its final position and mark it as done.
        linearization[len - 1 - j] = elem;
        done.Set(elem);
    }
}

} // namespace cluster_linearize

#endif // BITCOIN_CLUSTER_LINEARIZE_H
