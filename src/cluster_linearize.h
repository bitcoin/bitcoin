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

#include <memusage.h>
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

        /** Equality operator (primarily for testing purposes). */
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

    unsigned CountDependencies() const noexcept
    {
        unsigned ret = 0;
        for (auto i : Positions()) {
            ret += GetReducedParents(i).Count();
        }
        return ret;
    }

    /** Reduce memory usage if possible. No observable effect. */
    void Compact() noexcept
    {
        entries.shrink_to_fit();
    }

    size_t DynamicMemoryUsage() const noexcept
    {
        return memusage::DynamicUsage(entries);
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

    /** Remove the transactions of other from this SetInfo (which must be a subset). */
    SetInfo& operator-=(const SetInfo& other) noexcept
    {
        Assume(other.transactions.IsSubsetOf(transactions));
        transactions -= other.transactions;
        feerate -= other.feerate;
        return *this;
    }

    /** Compute the difference between this and other SetInfo (which must be a subset). */
    SetInfo operator-(const SetInfo& other) const noexcept
    {
        Assume(other.transactions.IsSubsetOf(transactions));
        return {transactions - other.transactions, feerate - other.feerate};
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

/** Compute the chunks of linearization as SetInfos. */
template<typename SetType>
std::vector<SetInfo<SetType>> ChunkLinearizationInfo(const DepGraph<SetType>& depgraph, std::span<const DepGraphIndex> linearization) noexcept
{
    std::vector<SetInfo<SetType>> ret;
    for (DepGraphIndex i : linearization) {
        /** The new chunk to be added, initially a singleton. */
        SetInfo<SetType> new_chunk(depgraph, i);
        // As long as the new chunk has a higher feerate than the last chunk so far, absorb it.
        while (!ret.empty() && new_chunk.feerate >> ret.back().feerate) {
            new_chunk |= ret.back();
            ret.pop_back();
        }
        // Actually move that new chunk into the chunking.
        ret.emplace_back(std::move(new_chunk));
    }
    return ret;
}

/** Compute the feerates of the chunks of linearization. Identical to ChunkLinearizationInfo, but
 *  only returns the chunk feerates, not the corresponding transaction sets. */
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

/** Class to represent the internal state of the spanning-forest linearization (SFL) algorithm.
 *
 * At all times, each dependency is marked as either "active" or "inactive". The subset of active
 * dependencies is the state of the SFL algorithm. The implementation maintains several other
 * values to speed up operations, but everything is ultimately a function of what that subset of
 * active dependencies is.
 *
 * Given such a subset, define a chunk as the set of transactions that are connected through active
 * dependencies (ignoring their parent/child direction). Thus, every state implies a particular
 * partitioning of the graph into chunks (including potential singletons). In the extreme, each
 * transaction may be in its own chunk, or in the other extreme all transactions may form a single
 * chunk. A chunk's feerate is its total fee divided by its total size.
 *
 * The algorithm consists of switching dependencies between active and inactive. The final
 * linearization that is produced at the end consists of these chunks, sorted from high to low
 * feerate, each individually sorted in an arbitrary but topological (= no child before parent)
 * way.
 *
 * We define four quality properties the state can have:
 *
 * - acyclic: The state is acyclic whenever no cycle of active dependencies exists within the
 *            graph, ignoring the parent/child direction. This is equivalent to saying that within
 *            each chunk the set of active dependencies form a tree, and thus the overall set of
 *            active dependencies in the graph form a spanning forest, giving the algorithm its
 *            name. Being acyclic is also equivalent to every chunk of N transactions having
 *            exactly N-1 active dependencies.
 *
 *            For example in a diamond graph, D->{B,C}->A, the 4 dependencies cannot be
 *            simultaneously active. If at least one is inactive, the state is acyclic.
 *
 *            The algorithm maintains an acyclic state at *all* times as an invariant. This implies
 *            that activating a dependency always corresponds to merging two chunks, and that
 *            deactivating one always corresponds to splitting two chunks.
 *
 * - topological: We say the state is topological whenever it is acyclic and no inactive dependency
 *                exists between two distinct chunks such that the child chunk has higher or equal
 *                feerate than the parent chunk.
 *
 *                The relevance is that whenever the state is topological, the produced output
 *                linearization will be topological too (i.e., not have children before parents).
 *                Note that the "or equal" part of the definition matters: if not, one can end up
 *                in a situation with mutually-dependent equal-feerate chunks that cannot be
 *                linearized. For example C->{A,B} and D->{A,B}, with C->A and D->B active. The AC
 *                chunk depends on DB through C->B, and the BD chunk depends on AC through D->A.
 *                Merging them into a single ABCD chunk fixes this.
 *
 *                The algorithm attempts to keep the state topological as much as possible, so it
 *                can be interrupted to produce an output whenever, but will sometimes need to
 *                temporarily deviate from it when improving the state.
 *
 * - optimal: For every active dependency, define its top and bottom set as the set of transactions
 *            in the chunks that would result if the dependency were deactivated; the top being the
 *            one with the dependency's parent, and the bottom being the one with the child. Note
 *            that due to acyclicity, every deactivation splits a chunk exactly in two.
 *
 *            We say the state is optimal whenever it is topological and it has no active
 *            dependency whose top feerate is strictly higher than its bottom feerate. The
 *            relevance is that it can be proven that whenever the state is optimal, the produced
 *            linearization will also be optimal (in the convexified feerate diagram sense). It can
 *            also be proven that for every graph at least one optimal state exists.
 *
 *            Note that it is possible for the SFL state to not be optimal, but the produced
 *            linearization to still be optimal. This happens when the chunks of a state are
 *            identical to those of an optimal state, but the exact set of active dependencies
 *            within a chunk differ in such a way that the state optimality condition is not
 *            satisfied. Thus, the state being optimal is more a "the eventual output is *known*
 *            to be optimal".
 *
 * - minimal: We say the state is minimal when it is:
 *            - acyclic
 *            - topological, except that inactive dependencies between equal-feerate chunks are
 *              allowed as long as they do not form a loop.
 *            - like optimal, no active dependencies whose top feerate is strictly higher than
 *              the bottom feerate are allowed.
 *            - no chunk contains a proper non-empty subset which includes all its own in-chunk
 *              dependencies of the same feerate as the chunk itself.
 *
 *            A minimal state effectively corresponds to an optimal state, where every chunk has
 *            been split into its minimal equal-feerate components.
 *
 *            The algorithm terminates whenever a minimal state is reached.
 *
 *
 * This leads to the following high-level algorithm:
 * - Start with all dependencies inactive, and thus all transactions in their own chunk. This is
 *   definitely acyclic.
 * - Activate dependencies (merging chunks) until the state is topological.
 * - Loop until optimal (no dependencies with higher-feerate top than bottom), or time runs out:
 *   - Deactivate a violating dependency, potentially making the state non-topological.
 *   - Activate other dependencies to make the state topological again.
 * - If there is time left and the state is optimal:
 *   - Attempt to split chunks into equal-feerate parts without mutual dependencies between them.
 *     When this succeeds, recurse into them.
 *   - If no such chunks can be found, the state is minimal.
 * - Output the chunks from high to low feerate, each internally sorted topologically.
 *
 * When merging, we always either:
 * - Merge upwards: merge a chunk with the lowest-feerate other chunk it depends on, among those
 *                  with lower or equal feerate than itself.
 * - Merge downwards: merge a chunk with the highest-feerate other chunk that depends on it, among
 *                    those with higher or equal feerate than itself.
 *
 * Using these strategies in the improvement loop above guarantees that the output linearization
 * after a deactivate + merge step is never worse or incomparable (in the convexified feerate
 * diagram sense) than the output linearization that would be produced before the step. With that,
 * we can refine the high-level algorithm to:
 * - Start with all dependencies inactive.
 * - Perform merges as described until none are possible anymore, making the state topological.
 * - Loop until optimal or time runs out:
 *   - Pick a dependency D to deactivate among those with higher feerate top than bottom.
 *   - Deactivate D, causing the chunk it is in to split into top T and bottom B.
 *   - Do an upwards merge of T, if possible. If so, repeat the same with the merged result.
 *   - Do a downwards merge of B, if possible. If so, repeat the same with the merged result.
 * - Split chunks further to obtain a minimal state, see below.
 * - Output the chunks from high to low feerate, each internally sorted topologically.
 *
 * Instead of performing merges arbitrarily to make the initial state topological, it is possible
 * to do so guided by an existing linearization. This has the advantage that the state's would-be
 * output linearization is immediately as good as the existing linearization it was based on:
 * - Start with all dependencies inactive.
 * - For each transaction t in the existing linearization:
 *   - Find the chunk C that transaction is in (which will be singleton).
 *   - Do an upwards merge of C, if possible. If so, repeat the same with the merged result.
 * No downwards merges are needed in this case.
 *
 * After reaching an optimal state, it can be transformed into a minimal state by attempting to
 * split chunks further into equal-feerate parts. To do so, pick a specific transaction in each
 * chunk (the pivot), and rerun the above split-then-merge procedure again:
 * - first, while pretending the pivot transaction has an infinitesimally higher (or lower) fee
 *   than it really has. If a split exists with the pivot in the top part (or bottom part), this
 *   will find it.
 * - if that fails to split, repeat while pretending the pivot transaction has an infinitesimally
 *   lower (or higher) fee. If a split exists with the pivot in the bottom part (or top part), this
 *   will find it.
 * - if either succeeds, repeat the procedure for the newly found chunks to split them further.
 *   If not, the chunk is already minimal.
 * If the chunk can be split into equal-feerate parts, then the pivot must exist in either the top
 * or bottom part of that potential split. By trying both with the same pivot, if a split exists,
 * it will be found.
 *
 * What remains to be specified are a number of heuristics:
 *
 * - How to decide which chunks to merge:
 *   - The merge upwards and downward rules specify that the lowest-feerate respectively
 *     highest-feerate candidate chunk is merged with, but if there are multiple equal-feerate
 *     candidates, a uniformly random one among them is picked.
 *
 * - How to decide what dependency to activate (when merging chunks):
 *   - After picking two chunks to be merged (see above), a uniformly random dependency between the
 *     two chunks is activated.
 *
 * - How to decide which chunk to find a dependency to split in:
 *   - A round-robin queue of chunks to improve is maintained. The initial ordering of this queue
 *     is uniformly randomly permuted.
 *
 * - How to decide what dependency to deactivate (when splitting chunks):
 *   - Define the gain of an active dependency as either:
 *
 *         gain = (feerate(top) - feerate(bottom)) * size(top) * size(bottom)
 *              = fee(top) * size(chunk) - fee(chunk) * size(top)
 *
 *   - After every split, it is possible that the top and the bottom chunk merge with each other
 *     again in the merge sequence (through a top->bottom dependency, not through the deactivated
 *     one, which was bottom->top). Call this a self-merge. If a self-merge does not occur after
 *     a split, the resulting linearization is strictly improved (the area under the convexified
 *     feerate diagram increases by at least gain/2), while self-merges do not change it.
 *   - Inside the selected chunk (see above), among the dependencies whose gain is maximal, if any
 *     with strictly positive gain exist, a uniformly random one is deactivated.
 *
 * - How to decide the exact output linearization:
 *   - When there are multiple equal-feerate chunks with no dependencies between them, output a
 *     uniformly random one among the ones with no missing dependent chunks first.
 *   - Within chunks, repeatedly pick a uniformly random transaction among those with no missing
 *     dependencies.
 */
template<typename SetType>
class SpanningForestState
{
private:
    /** Internal RNG. */
    InsecureRandomContext m_rng;

    /** Data type to represent indexing into m_tx_data. */
    using TxIdx = uint32_t;
    /** Data type to represent indexing into m_set_info. Use the smallest type possible to improve
     *  cache locality. */
    using SetIdx = std::conditional_t<(SetType::Size() <= 0xff),
                                      uint8_t,
                                      std::conditional_t<(SetType::Size() <= 0xffff),
                                                         uint16_t,
                                                         uint32_t>>;
    /** An invalid SetIdx. */
    static constexpr SetIdx INVALID_SET_IDX = SetIdx(-1);

    /** Structure with information about a single transaction. */
    struct TxData {
        /** The top set for every active child dependency this transaction has, indexed by child
         *  TxIdx. INVALID_SET_IDX if there is no active dependency with the corresponding child.
         */
        std::array<SetIdx, SetType::Size()> dep_top_idx;
        /** The set of parent transactions of this transaction. Immutable after construction. */
        SetType parents;
        /** The set of child transactions of this transaction. Immutable after construction. */
        SetType children;
        /** Which chunk this transaction belongs to. */
        SetIdx chunk_idx;
    };

    /** The set of all TxIdx's of transactions in the cluster indexing into m_tx_data. */
    SetType m_transaction_idxs;
    /** The set of all chunk SetIdx's. This excludes the SetIdxs that refer to active
     *  dependencies' tops. */
    SetType m_chunk_idxs;
    /** Information about each transaction (and chunks). Keeps the "holes" from DepGraph during
     *  construction. Indexed by TxIdx. */
    std::vector<TxData> m_tx_data;
    /** Information about each set (chunk, or active dependency top set). Indexed by SetIdx. */
    std::vector<SetInfo<SetType>> m_set_info;
    /** For each chunk, indexed by SetIdx, the set of out-of-chunk reachable transactions, in the
     *  upwards (.first) and downwards (.second) direction. */
    std::vector<std::pair<SetType, SetType>> m_reachable;
    /** A FIFO of chunk SetIdxs for chunks that may be improved still. */
    VecDeque<SetIdx> m_suboptimal_chunks;
    /** A FIFO of chunk indexes with a pivot transaction in them, and a flag to indicate their
     *  status:
     *  - bit 1: currently attempting to move the pivot down, rather than up.
     *  - bit 2: this is the second stage, so we have already tried moving the pivot in the other
     *           direction.
     */
    VecDeque<std::tuple<SetIdx, TxIdx, unsigned>> m_nonminimal_chunks;

    /** The number of updated transactions in activations/deactivations. */
    uint64_t m_cost{0};

    /** Pick a random transaction within a set (which must be non-empty). */
    TxIdx PickRandomTx(const SetType& tx_idxs) noexcept
    {
        Assume(tx_idxs.Any());
        unsigned pos = m_rng.randrange<unsigned>(tx_idxs.Count());
        for (auto tx_idx : tx_idxs) {
            if (pos == 0) return tx_idx;
            --pos;
        }
        Assume(false);
        return TxIdx(-1);
    }

    /** Update a chunk:
     *  - All transactions have their chunk index set to `chunk_idx`.
     *  - All dependencies which have `query` in their top set get `dep_change` added to it
     *    (if `!Subtract`) or removed from it (if `Subtract`).
     */
    template<bool Subtract>
    void UpdateChunk(const SetType& tx_idxs, TxIdx query, SetIdx chunk_idx, const SetInfo<SetType>& dep_change) noexcept
    {
        // Iterate over all the chunk's transactions.
        for (auto tx_idx : tx_idxs) {
            auto& tx_data = m_tx_data[tx_idx];
            // Update the chunk index for this transaction.
            tx_data.chunk_idx = chunk_idx;
            // Iterate over all active dependencies with tx_idx as parent. Combined with the outer
            // loop this iterates over all internal active dependencies of the chunk.
            for (auto child_idx : tx_data.children) {
                // Skip inactive dependencies.
                if (tx_data.dep_top_idx[child_idx] == INVALID_SET_IDX) continue;
                auto& top_set_info = m_set_info[tx_data.dep_top_idx[child_idx]];
                // If this dependency's top set contains query, update it to add/remove
                // dep_change.
                if (top_set_info.transactions[query]) {
                    if constexpr (Subtract) {
                        top_set_info -= dep_change;
                    } else {
                        top_set_info |= dep_change;
                    }
                }
            }
        }
    }

    /** Find the set of out-of-chunk transactions reachable from tx_idxs, both in upwards and
     *  downwards direction. */
    std::pair<SetType, SetType> GetReachable(const SetType& tx_idxs) const noexcept
    {
        SetType parents, children;
        for (auto tx_idx : tx_idxs) {
            const auto& tx_data = m_tx_data[tx_idx];
            parents |= tx_data.parents;
            children |= tx_data.children;
        }
        return {parents - tx_idxs, children - tx_idxs};
    }

    /** Make the inactive dependency from child to parent, which must not be in the same chunk
     *  already, active. Returns the merged chunk idx. */
    SetIdx Activate(TxIdx parent_idx, TxIdx child_idx) noexcept
    {
        // Gather and check information about the parent and child transactions.
        auto& parent_data = m_tx_data[parent_idx];
        auto& child_data = m_tx_data[child_idx];
        Assume(parent_data.children[child_idx]);
        Assume(parent_data.dep_top_idx[child_idx] == INVALID_SET_IDX);
        // Get the set index of the chunks the parent and child are currently in. The parent chunk
        // will become the top set of the newly activated dependency, while the child chunk will be
        // grown to become the merged chunk.
        auto parent_chunk_idx = parent_data.chunk_idx;
        auto child_chunk_idx = child_data.chunk_idx;
        Assume(parent_chunk_idx != child_chunk_idx);
        auto& top_info = m_set_info[parent_chunk_idx];
        auto& bottom_info = m_set_info[child_chunk_idx];

        // Consider the following example:
        //
        //    A           A     There are two chunks, ABC and DEF, and the inactive E->C dependency
        //   / \         / \    is activated, resulting in a single chunk ABCDEF.
        //  B   C       B   C
        //      :  ==>      |   Dependency | top set before | top set after | change
        //  D   E       D   E   B->A       | AC             | ACDEF         | +DEF
        //   \ /         \ /    C->A       | AB             | AB            |
        //    F           F     F->D       | D              | D             |
        //                      F->E       | E              | ABCE          | +ABC
        //
        // The common pattern here is that any dependency which has the parent or child of the
        // dependency being activated (E->C here) in its top set, will have the opposite part added
        // to it. This is true for B->A and F->E, but not for C->A and F->D.
        //
        // Let UpdateChunk traverse the old parent chunk top_info (ABC in example), and add
        // bottom_info (DEF) to every dependency's top set which has the parent (C) in it. At the
        // same time, change the chunk_idx for each to be child_chunk_idx, which becomes the set for
        // the merged chunk.
        UpdateChunk<false>(/*tx_idxs=*/top_info.transactions, /*query=*/parent_idx,
                           /*chunk_idx=*/child_chunk_idx, /*dep_change=*/bottom_info);
        // Let UpdateChunk traverse the old child chunk bottom_info (DEF in example), and add
        // top_info (ABC) to every dependency's top set which has the child (E) in it. The chunk
        // these are part of isn't being changed here (already child_chunk_idx for each).
        UpdateChunk<false>(/*tx_idxs=*/bottom_info.transactions, /*query=*/child_idx,
                           /*chunk_idx=*/child_chunk_idx, /*dep_change=*/top_info);
        // Merge top_info into bottom_info, which becomes the merged chunk.
        bottom_info |= top_info;
        m_cost += bottom_info.transactions.Count();
        // Compute merged sets of reachable transactions from the new chunk.
        m_reachable[child_chunk_idx].first |= m_reachable[parent_chunk_idx].first;
        m_reachable[child_chunk_idx].second |= m_reachable[parent_chunk_idx].second;
        m_reachable[child_chunk_idx].first -= bottom_info.transactions;
        m_reachable[child_chunk_idx].second -= bottom_info.transactions;
        // Make parent chunk the set for the new active dependency.
        parent_data.dep_top_idx[child_idx] = parent_chunk_idx;
        m_chunk_idxs.Reset(parent_chunk_idx);
        // Return the newly merged chunk.
        return child_chunk_idx;
    }

    /** Make a specified active dependency inactive. Returns the created parent and child chunk
     *  indexes. */
    std::pair<SetIdx, SetIdx> Deactivate(TxIdx parent_idx, TxIdx child_idx) noexcept
    {
        // Gather and check information about the parent transactions.
        auto& parent_data = m_tx_data[parent_idx];
        Assume(parent_data.children[child_idx]);
        Assume(parent_data.dep_top_idx[child_idx] != INVALID_SET_IDX);
        // Get the top set of the active dependency (which will become the parent chunk) and the
        // chunk set the transactions are currently in (which will become the bottom chunk).
        auto parent_chunk_idx = parent_data.dep_top_idx[child_idx];
        auto child_chunk_idx = parent_data.chunk_idx;
        Assume(parent_chunk_idx != child_chunk_idx);
        auto& top_info = m_set_info[parent_chunk_idx];
        auto& bottom_info = m_set_info[child_chunk_idx];

        // Remove the active dependency.
        parent_data.dep_top_idx[child_idx] = INVALID_SET_IDX;
        m_chunk_idxs.Set(parent_chunk_idx);
        m_cost += bottom_info.transactions.Count();
        // Subtract the top_info from the bottom_info, as it will become the child chunk.
        bottom_info -= top_info;
        // See the comment above in Activate(). We perform the opposite operations here, removing
        // instead of adding.
        UpdateChunk<true>(/*tx_idxs=*/top_info.transactions, /*query=*/parent_idx,
                          /*chunk_idx=*/parent_chunk_idx, /*dep_change=*/bottom_info);
        UpdateChunk<true>(/*tx_idxs=*/bottom_info.transactions, /*query=*/child_idx,
                          /*chunk_idx=*/child_chunk_idx, /*dep_change=*/top_info);
        // Compute the new sets of reachable transactions for each new chunk.
        m_reachable[child_chunk_idx] = GetReachable(bottom_info.transactions);
        m_reachable[parent_chunk_idx] = GetReachable(top_info.transactions);
        // Return the two new set idxs.
        return {parent_chunk_idx, child_chunk_idx};
    }

    /** Activate a dependency from the bottom set to the top set, which must exist. Return the
     *  index of the merged chunk. */
    SetIdx MergeChunks(SetIdx top_idx, SetIdx bottom_idx) noexcept
    {
        auto& top_chunk_info = m_set_info[top_idx];
        Assume(m_chunk_idxs[top_idx]);
        auto& bottom_chunk_info = m_set_info[bottom_idx];
        Assume(m_chunk_idxs[bottom_idx]);
        // Count the number of dependencies between bottom_chunk and top_chunk.
        TxIdx num_deps{0};
        for (auto tx_idx : top_chunk_info.transactions) {
            auto& tx_data = m_tx_data[tx_idx];
            num_deps += (tx_data.children & bottom_chunk_info.transactions).Count();
        }
        Assume(num_deps > 0);
        // Uniformly randomly pick one of them and activate it.
        TxIdx pick = m_rng.randrange(num_deps);
        for (auto tx_idx : top_chunk_info.transactions) {
            auto& tx_data = m_tx_data[tx_idx];
            auto intersect = tx_data.children & bottom_chunk_info.transactions;
            auto count = intersect.Count();
            if (pick < count) {
                for (auto child_idx : intersect) {
                    if (pick == 0) return Activate(tx_idx, child_idx);
                    --pick;
                }
                Assume(false);
            }
            pick -= count;
        }
        Assume(false);
        return INVALID_SET_IDX;
    }

    /** Activate a dependency from chunk_idx to merge_chunk_idx (if !DownWard), or a dependency
     *  from merge_chunk_idx to chunk_idx (if DownWard). Return the index of the merged chunk. */
    template<bool DownWard>
    SetIdx MergeChunksDirected(SetIdx chunk_idx, SetIdx merge_chunk_idx) noexcept
    {
        if constexpr (DownWard) {
            return MergeChunks(chunk_idx, merge_chunk_idx);
        } else {
            return MergeChunks(merge_chunk_idx, chunk_idx);
        }
    }

    /** Determine which chunk to merge chunk_idx with, or INVALID_SET_IDX if none. */
    template<bool DownWard>
    SetIdx PickMergeCandidate(SetIdx chunk_idx) noexcept
    {
        /** Information about the chunk. */
        auto& chunk_info = m_set_info[chunk_idx];
        // Iterate over all chunks reachable from this one. For those depended-on chunks,
        // remember the highest-feerate (if DownWard) or lowest-feerate (if !DownWard) one.
        // If multiple equal-feerate candidate chunks to merge with exist, pick a random one
        // among them.

        /** The minimum feerate (if downward) or maximum feerate (if upward) to consider when
         *  looking for candidate chunks to merge with. Initially, this is the original chunk's
         *  feerate, but is updated to be the current best candidate whenever one is found. */
        FeeFrac best_other_chunk_feerate = chunk_info.feerate;
        /** The index for the best candidate chunk to merge with. INVALID_SET_INDEX if none. */
        SetIdx best_other_chunk_idx = INVALID_SET_IDX;
        /** We generate random tiebreak values to pick between equal-feerate candidate chunks.
         *  This variable stores the tiebreak of the current best candidate. */
        uint64_t best_other_chunk_tiebreak{0};

        /** Which parent/child transactions we still need to process the chunks for. */
        auto todo = DownWard ? m_reachable[chunk_idx].second : m_reachable[chunk_idx].first;
        while (todo.Any()) {
            // Find a chunk for a transaction in todo, and remove all its transactions from todo.
            auto reached_chunk_idx = m_tx_data[todo.First()].chunk_idx;
            auto& reached_chunk_info = m_set_info[reached_chunk_idx];
            todo -= reached_chunk_info.transactions;
            // See if it has an acceptable feerate.
            auto cmp = DownWard ? FeeRateCompare(best_other_chunk_feerate, reached_chunk_info.feerate)
                                : FeeRateCompare(reached_chunk_info.feerate, best_other_chunk_feerate);
            if (cmp > 0) continue;
            uint64_t tiebreak = m_rng.rand64();
            if (cmp < 0 || tiebreak >= best_other_chunk_tiebreak) {
                best_other_chunk_feerate = reached_chunk_info.feerate;
                best_other_chunk_idx = reached_chunk_idx;
                best_other_chunk_tiebreak = tiebreak;
            }
        }

        return best_other_chunk_idx;
    }

    /** Perform an upward or downward merge step, on the specified chunk. Returns the merged chunk,
     *  or INVALID_SET_IDX if no merge took place. */
    template<bool DownWard>
    SetIdx MergeStep(SetIdx chunk_idx) noexcept
    {
        auto merge_chunk_idx = PickMergeCandidate<DownWard>(chunk_idx);
        if (merge_chunk_idx == INVALID_SET_IDX) return INVALID_SET_IDX;
        chunk_idx = MergeChunksDirected<DownWard>(chunk_idx, merge_chunk_idx);
        Assume(chunk_idx != INVALID_SET_IDX);
        return chunk_idx;
    }

    /** Perform an upward or downward merge sequence on the specified chunk. */
    template<bool DownWard>
    void MergeSequence(SetIdx chunk_idx) noexcept
    {
        Assume(m_chunk_idxs[chunk_idx]);
        while (true) {
            auto merged_chunk_idx = MergeStep<DownWard>(chunk_idx);
            if (merged_chunk_idx == INVALID_SET_IDX) break;
            chunk_idx = merged_chunk_idx;
        }
        // Add the chunk to the queue of improvable chunks.
        m_suboptimal_chunks.push_back(chunk_idx);
    }

    /** Split a chunk, and then merge the resulting two chunks to make the graph topological
     *  again. */
    void Improve(TxIdx parent_idx, TxIdx child_idx) noexcept
    {
        // Deactivate the specified dependency, splitting it into two new chunks: a top containing
        // the parent, and a bottom containing the child. The top should have a higher feerate.
        auto [parent_chunk_idx, child_chunk_idx] = Deactivate(parent_idx, child_idx);

        // At this point we have exactly two chunks which may violate topology constraints (the
        // parent chunk and child chunk that were produced by deactivating dep_idx). We can fix
        // these using just merge sequences, one upwards and one downwards, avoiding the need for a
        // full MakeTopological.
        const auto& parent_reachable = m_reachable[parent_chunk_idx].first;
        const auto& child_chunk_txn = m_set_info[child_chunk_idx].transactions;
        if (parent_reachable.Overlaps(child_chunk_txn)) {
            // The parent chunk has a dependency on a transaction in the child chunk. In this case,
            // the parent needs to merge back with the child chunk (a self-merge), and no other
            // merges are needed. Special-case this, so the overhead of PickMergeCandidate and
            // MergeSequence can be avoided.
            auto merged_chunk_idx = MergeChunks(child_chunk_idx, parent_chunk_idx);
            m_suboptimal_chunks.push_back(merged_chunk_idx);
        } else {
            // Merge the top chunk with lower-feerate chunks it depends on.
            MergeSequence<false>(parent_chunk_idx);
            // Merge the bottom chunk with higher-feerate chunks that depend on it.
            MergeSequence<true>(child_chunk_idx);
        }
    }

public:
    /** Construct a spanning forest for the given DepGraph, with every transaction in its own chunk
     *  (not topological). */
    explicit SpanningForestState(const DepGraph<SetType>& depgraph, uint64_t rng_seed) noexcept : m_rng(rng_seed)
    {
        m_transaction_idxs = depgraph.Positions();
        auto num_transactions = m_transaction_idxs.Count();
        m_tx_data.resize(depgraph.PositionRange());
        m_set_info.resize(num_transactions);
        m_reachable.resize(num_transactions);
        size_t num_chunks = 0;
        for (auto tx_idx : m_transaction_idxs) {
            // Fill in transaction data.
            auto& tx_data = m_tx_data[tx_idx];
            tx_data.parents = depgraph.GetReducedParents(tx_idx);
            for (auto parent_idx : tx_data.parents) {
                m_tx_data[parent_idx].children.Set(tx_idx);
            }
            // Create a singleton chunk for it.
            tx_data.chunk_idx = num_chunks;
            m_set_info[num_chunks++] = SetInfo(depgraph, tx_idx);
            // Mark all its dependencies inactive.
            tx_data.dep_top_idx.fill(INVALID_SET_IDX);
        }
        // Set the reachable transactions for each chunk to the transactions' parents and children.
        for (SetIdx chunk_idx = 0; chunk_idx < num_transactions; ++chunk_idx) {
            auto& tx_data = m_tx_data[m_set_info[chunk_idx].transactions.First()];
            m_reachable[chunk_idx].first = tx_data.parents;
            m_reachable[chunk_idx].second = tx_data.children;
        }
        Assume(num_chunks == num_transactions);
        // Mark all chunk sets as chunks.
        m_chunk_idxs = SetType::Fill(num_chunks);
    }

    /** Load an existing linearization. Must be called immediately after constructor. The result is
     *  topological if the linearization is valid. Otherwise, MakeTopological still needs to be
     *  called. */
    void LoadLinearization(std::span<const DepGraphIndex> old_linearization) noexcept
    {
        // Add transactions one by one, in order of existing linearization.
        for (DepGraphIndex tx_idx : old_linearization) {
            auto chunk_idx = m_tx_data[tx_idx].chunk_idx;
            // Merge the chunk upwards, as long as merging succeeds.
            while (true) {
                chunk_idx = MergeStep<false>(chunk_idx);
                if (chunk_idx == INVALID_SET_IDX) break;
            }
        }
    }

    /** Make state topological. Can be called after constructing, or after LoadLinearization. */
    void MakeTopological() noexcept
    {
        for (auto chunk_idx : m_chunk_idxs) {
            m_suboptimal_chunks.emplace_back(chunk_idx);
            // Randomize the initial order of suboptimal chunks in the queue.
            SetIdx j = m_rng.randrange<SetIdx>(m_suboptimal_chunks.size());
            if (j != m_suboptimal_chunks.size() - 1) {
                std::swap(m_suboptimal_chunks.back(), m_suboptimal_chunks[j]);
            }
        }
        while (!m_suboptimal_chunks.empty()) {
            // Pop an entry from the potentially-suboptimal chunk queue.
            SetIdx chunk_idx = m_suboptimal_chunks.front();
            m_suboptimal_chunks.pop_front();
            // If what was popped is not currently a chunk, continue. This may
            // happen when it was merged with something else since being added.
            if (!m_chunk_idxs[chunk_idx]) continue;
            int flip = m_rng.randbool();
            for (int i = 0; i < 2; ++i) {
                if (i ^ flip) {
                    // Attempt to merge the chunk upwards.
                    auto result_up = MergeStep<false>(chunk_idx);
                    if (result_up != INVALID_SET_IDX) {
                        m_suboptimal_chunks.push_back(result_up);
                        break;
                    }
                } else {
                    // Attempt to merge the chunk downwards.
                    auto result_down = MergeStep<true>(chunk_idx);
                    if (result_down != INVALID_SET_IDX) {
                        m_suboptimal_chunks.push_back(result_down);
                        break;
                    }
                }
            }
        }
    }

    /** Initialize the data structure for optimization. It must be topological already. */
    void StartOptimizing() noexcept
    {
        // Mark chunks suboptimal.
        for (auto chunk_idx : m_chunk_idxs) {
            m_suboptimal_chunks.push_back(chunk_idx);
            // Randomize the initial order of suboptimal chunks in the queue.
            SetIdx j = m_rng.randrange<TxIdx>(m_suboptimal_chunks.size());
            if (j != m_suboptimal_chunks.size() - 1) {
                std::swap(m_suboptimal_chunks.back(), m_suboptimal_chunks[j]);
            }
        }
    }

    /** Try to improve the forest. Returns false if it is optimal, true otherwise. */
    bool OptimizeStep() noexcept
    {
        while (!m_suboptimal_chunks.empty()) {
            // Pop an entry from the potentially-suboptimal chunk queue.
            SetIdx chunk_idx = m_suboptimal_chunks.front();
            m_suboptimal_chunks.pop_front();
            auto& chunk_info = m_set_info[chunk_idx];
            // If what was popped is not currently a chunk, continue. This may
            // happen when a split chunk merges in Improve() with one or more existing chunks that
            // are themselves on the suboptimal queue already.
            if (!m_chunk_idxs[chunk_idx]) continue;
            // Remember the best dependency {par, chl} seen so far, together with its top feerate.
            std::pair<TxIdx, TxIdx> candidate_dep;
            FeeFrac candidate_top_feerate;
            uint64_t candidate_tiebreak = std::numeric_limits<uint64_t>::max();
            // Iterate over all transactions.
            for (auto tx_idx : chunk_info.transactions) {
                const auto& tx_data = m_tx_data[tx_idx];
                // Iterate over all active child dependencies of the transaction.
                for (auto child_idx : tx_data.children) {
                    // Skip inactive child dependencies.
                    if (tx_data.dep_top_idx[child_idx] == INVALID_SET_IDX) continue;
                    auto& dep_top_info = m_set_info[tx_data.dep_top_idx[child_idx]];
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
                    // so we can use FeeRateCompare to discover if dep_top_info.feerate has better
                    // gain than candidate_top_feerate. As FeeRateCompare() is actually implemented by
                    // checking the sign of the cross-product, it even works when
                    // size(top1) <= size(top2). When no candidate exists so far, this is equal
                    // to comparing the feerate with the chunk directly (= the sign of gain(top)).
                    auto cmp = FeeRateCompare(dep_top_info.feerate - candidate_top_feerate,
                                              chunk_info.feerate);
                    if (cmp < 0) continue;
                    // Generate a random tiebreak for this dependency, and reject it if its gain is
                    // equal to the candidate so far, but has worse tiebreak. This means that among
                    // equal-gain dependencies, a uniformly random one (the one with the highest
                    // tiebreak) will be chosen.
                    uint64_t tiebreak = m_rng.rand64() >> 1;
                    if (cmp == 0 && tiebreak <= candidate_tiebreak) continue;
                    // Remember this as our (new) candidate dependency.
                    candidate_dep = {tx_idx, child_idx};
                    candidate_top_feerate = dep_top_info.feerate;
                    candidate_tiebreak = tiebreak;
                }
            }
            // If a candidate with positive gain was found, deactivate it and then make the state
            // topological again with a sequence of merges.
            if (candidate_tiebreak != std::numeric_limits<uint64_t>::max()) {
                Improve(candidate_dep.first, candidate_dep.second);
            }
            // Stop processing for now, even if nothing was activated, as the loop above may have
            // had a nontrivial cost.
            return !m_suboptimal_chunks.empty();
        }
        // No improvable chunk was found, we are done.
        return false;
    }

    /** Initialize data structure for minimizing the chunks. Can only be called if state is known
     *  to be optimal. OptimizeStep() cannot be called anymore afterwards. */
    void StartMinimizing() noexcept
    {
        m_nonminimal_chunks.clear();
        m_nonminimal_chunks.reserve(m_transaction_idxs.Count());
        // Gather all chunks, and for each, add it with a random pivot in it, and a random initial
        // direction, to m_nonminimal_chunks.
        for (auto chunk_idx : m_chunk_idxs) {
            TxIdx pivot_idx = PickRandomTx(m_set_info[chunk_idx].transactions);
            m_nonminimal_chunks.emplace_back(chunk_idx, pivot_idx, m_rng.randbits<1>());
            // Randomize the initial order of nonminimal chunks in the queue.
            SetIdx j = m_rng.randrange<SetIdx>(m_nonminimal_chunks.size());
            if (j != m_nonminimal_chunks.size() - 1) {
                std::swap(m_nonminimal_chunks.back(), m_nonminimal_chunks[j]);
            }
        }
    }

    /** Try to reduce a chunk's size. Returns false if all chunks are minimal, true otherwise. */
    bool MinimizeStep() noexcept
    {
        // If the queue of potentially-non-minimal chunks is empty, we are done.
        if (m_nonminimal_chunks.empty()) return false;
        // Pop an entry from the potentially-non-minimal chunk queue.
        auto [chunk_idx, pivot_idx, flags] = m_nonminimal_chunks.front();
        m_nonminimal_chunks.pop_front();
        Assume(m_chunk_idxs[chunk_idx]);
        auto& chunk_info = m_set_info[chunk_idx];
        /** Whether to move the pivot down rather than up. */
        bool move_pivot_down = flags & 1;
        /** Whether this is already the second stage. */
        bool second_stage = flags & 2;

        // Find a random dependency whose top and bottom set feerates are equal, and which has
        // pivot in bottom set (if move_pivot_down) or in top set (if !move_pivot_down).
        std::pair<TxIdx, TxIdx> candidate_dep;
        uint64_t candidate_tiebreak{0};
        bool have_any = false;
        // Iterate over all transactions.
        for (auto tx_idx : chunk_info.transactions) {
            const auto& tx_data = m_tx_data[tx_idx];
            // Iterate over all active child dependencies of the transaction.
            for (auto child_idx : tx_data.children) {
                // Skip inactive child dependencies.
                if (tx_data.dep_top_idx[child_idx] == INVALID_SET_IDX) continue;
                const auto& dep_top_info = m_set_info[tx_data.dep_top_idx[child_idx]];
                // Skip if this dependency does not have equal top and bottom set feerates. Note
                // that the top cannot have higher feerate than the bottom, or OptimizeSteps would
                // have dealt with it.
                if (dep_top_info.feerate << chunk_info.feerate) continue;
                have_any = true;
                // Skip if this dependency does not have pivot in the right place.
                if (move_pivot_down == dep_top_info.transactions[pivot_idx]) continue;
                // Remember this as our chosen dependency if it has a better tiebreak.
                uint64_t tiebreak = m_rng.rand64() | 1;
                if (tiebreak > candidate_tiebreak) {
                    candidate_tiebreak = tiebreak;
                    candidate_dep = {tx_idx, child_idx};
                }
            }
        }
        // If no dependencies have equal top and bottom set feerate, this chunk is minimal.
        if (!have_any) return true;
        // If all found dependencies have the pivot in the wrong place, try moving it in the other
        // direction. If this was the second stage already, we are done.
        if (candidate_tiebreak == 0) {
            // Switch to other direction, and to second phase.
            flags ^= 3;
            if (!second_stage) m_nonminimal_chunks.emplace_back(chunk_idx, pivot_idx, flags);
            return true;
        }

        // Otherwise, deactivate the dependency that was found.
        auto [parent_chunk_idx, child_chunk_idx] = Deactivate(candidate_dep.first, candidate_dep.second);
        // Determine if there is a dependency from the new bottom to the new top (opposite from the
        // dependency that was just deactivated).
        auto& parent_reachable = m_reachable[parent_chunk_idx].first;
        auto& child_chunk_txn = m_set_info[child_chunk_idx].transactions;
        if (parent_reachable.Overlaps(child_chunk_txn)) {
            // A self-merge is needed.
            auto merged_chunk_idx = MergeChunks(child_chunk_idx, parent_chunk_idx);
            // Re-insert the chunk into the queue, in the same direction. Note that the chunk_idx
            // will have changed.
            m_nonminimal_chunks.emplace_back(merged_chunk_idx, pivot_idx, flags);
        } else {
            // No self-merge happens, and thus we have found a way to split the chunk. Create two
            // smaller chunks, and add them to the queue. The one that contains the current pivot
            // gets to continue with it in the same direction, to minimize the number of times we
            // alternate direction. If we were in the second phase already, the newly created chunk
            // inherits that too, because we know no split with the pivot on the other side is
            // possible already. The new chunk without the current pivot gets a new randomly-chosen
            // one.
            if (move_pivot_down) {
                auto parent_pivot_idx = PickRandomTx(m_set_info[parent_chunk_idx].transactions);
                m_nonminimal_chunks.emplace_back(parent_chunk_idx, parent_pivot_idx, m_rng.randbits<1>());
                m_nonminimal_chunks.emplace_back(child_chunk_idx, pivot_idx, flags);
            } else {
                auto child_pivot_idx = PickRandomTx(m_set_info[child_chunk_idx].transactions);
                m_nonminimal_chunks.emplace_back(parent_chunk_idx, pivot_idx, flags);
                m_nonminimal_chunks.emplace_back(child_chunk_idx, child_pivot_idx, m_rng.randbits<1>());
            }
            if (m_rng.randbool()) {
                std::swap(m_nonminimal_chunks.back(), m_nonminimal_chunks[m_nonminimal_chunks.size() - 2]);
            }
        }
        return true;
    }

    /** Construct a topologically-valid linearization from the current forest state. Must be
     *  topological. */
    std::vector<DepGraphIndex> GetLinearization() noexcept
    {
        /** The output linearization. */
        std::vector<DepGraphIndex> ret;
        ret.reserve(m_set_info.size());
        /** A heap with all chunks (by SetIdx) that can currently be included, sorted by
         *  chunk feerate and a random tie-breaker. */
        std::vector<std::pair<TxIdx, uint64_t>> ready_chunks;
        /** For every chunk, indexed by SetIdx, the number of unmet dependencies the chunk has on
         *  other chunks (not including dependencies within the chunk itself). */
        std::vector<TxIdx> chunk_deps(m_set_info.size(), 0);
        /** For every transaction, indexed by TxIdx, the number of unmet dependencies the
         *  transaction has. */
        std::vector<TxIdx> tx_deps(m_tx_data.size(), 0);
        /** A list with all transactions within the current chunk that can be included. */
        std::vector<TxIdx> ready_tx;
        // Populate chunk_deps and tx_deps.
        for (TxIdx chl_idx : m_transaction_idxs) {
            const auto& chl_data = m_tx_data[chl_idx];
            tx_deps[chl_idx] = chl_data.parents.Count();
            auto chl_chunk_idx = chl_data.chunk_idx;
            auto& chl_chunk_info = m_set_info[chl_chunk_idx];
            chunk_deps[chl_chunk_idx] += (chl_data.parents - chl_chunk_info.transactions).Count();
        }
        // Construct a heap with all chunks that have no out-of-chunk dependencies.
        /** Comparison function for the heap. */
        auto chunk_cmp_fn = [&](const auto& a, const auto& b) noexcept {
            auto& chunk_a = m_set_info[a.first];
            auto& chunk_b = m_set_info[b.first];
            // First sort by chunk feerate.
            if (chunk_a.feerate != chunk_b.feerate) {
                return chunk_a.feerate < chunk_b.feerate;
            }
            // Tie-break randomly.
            if (a.second != b.second) return a.second < b.second;
            // Lastly, tie-break by chunk index.
            return a.first < b.first;
        };
        for (SetIdx chunk_idx : m_chunk_idxs) {
            if (chunk_deps[chunk_idx] == 0) ready_chunks.emplace_back(chunk_idx, m_rng.rand64());
        }
        std::make_heap(ready_chunks.begin(), ready_chunks.end(), chunk_cmp_fn);
        // Pop chunks off the heap, highest-feerate ones first.
        while (!ready_chunks.empty()) {
            auto [chunk_idx, _rnd] = ready_chunks.front();
            std::pop_heap(ready_chunks.begin(), ready_chunks.end(), chunk_cmp_fn);
            ready_chunks.pop_back();
            Assume(m_chunk_idxs[chunk_idx]);
            Assume(chunk_deps[chunk_idx] == 0);
            const auto& chunk_txn = m_set_info[chunk_idx].transactions;
            // Build list of all includable transactions in chunk.
            for (TxIdx tx_idx : chunk_txn) {
                if (tx_deps[tx_idx] == 0) {
                    ready_tx.push_back(tx_idx);
                }
            }
            Assume(!ready_tx.empty());
            // Pick transactions from the ready queue, append them to linearization, and decrement
            // dependency counts.
            while (!ready_tx.empty()) {
                // Move a random queue element to the back.
                auto pos = m_rng.randrange(ready_tx.size());
                if (pos != ready_tx.size() - 1) std::swap(ready_tx.back(), ready_tx[pos]);
                // Pop from the back.
                auto tx_idx = ready_tx.back();
                Assume(chunk_txn[tx_idx]);
                ready_tx.pop_back();
                // Append to linearization.
                ret.push_back(tx_idx);
                // Decrement dependency counts.
                auto& tx_data = m_tx_data[tx_idx];
                for (TxIdx chl_idx : tx_data.children) {
                    auto& chl_data = m_tx_data[chl_idx];
                    // Decrement tx dependency count.
                    Assume(tx_deps[chl_idx] > 0);
                    if (--tx_deps[chl_idx] == 0 && chunk_txn[chl_idx]) {
                        // Child tx has no dependencies left, and is in this chunk. Add it to the tx queue.
                        ready_tx.push_back(chl_idx);
                    }
                    // Decrement chunk dependency count if this is out-of-chunk dependency.
                    if (chl_data.chunk_idx != chunk_idx) {
                        Assume(chunk_deps[chl_data.chunk_idx] > 0);
                        if (--chunk_deps[chl_data.chunk_idx] == 0) {
                            // Child chunk has no dependencies left. Add it to the chunk heap.
                            ready_chunks.emplace_back(chl_data.chunk_idx, m_rng.rand64());
                            std::push_heap(ready_chunks.begin(), ready_chunks.end(), chunk_cmp_fn);
                        }
                    }
                }
            }
        }
        Assume(ret.size() == m_set_info.size());
        return ret;
    }

    /** Get the diagram for the current state, which must be topological. Test-only.
     *
     * The linearization produced by GetLinearization() is always at least as good (in the
     * CompareChunks() sense) as this diagram, but may be better.
     *
     * After an OptimizeStep(), the diagram will always be at least as good as before. Once
     * OptimizeStep() returns false, the diagram will be equivalent to that produced by
     * GetLinearization(), and optimal.
     *
     * After a MinimizeStep(), the diagram cannot change anymore (in the CompareChunks() sense),
     * but its number of segments can increase still. Once MinimizeStep() returns false, the number
     * of chunks of the produced linearization will match the number of segments in the diagram.
     */
    std::vector<FeeFrac> GetDiagram() const noexcept
    {
        std::vector<FeeFrac> ret;
        for (auto chunk_idx : m_chunk_idxs) {
            ret.push_back(m_set_info[chunk_idx].feerate);
        }
        std::sort(ret.begin(), ret.end(), std::greater{});
        return ret;
    }

    /** Determine how much work was performed so far. */
    uint64_t GetCost() const noexcept { return m_cost; }

    /** Verify internal consistency of the data structure. */
    void SanityCheck(const DepGraph<SetType>& depgraph) const
    {
        //
        // Verify dependency parent/child information, and build list of (active) dependencies.
        //
        std::vector<std::pair<TxIdx, TxIdx>> expected_dependencies;
        std::vector<std::pair<TxIdx, TxIdx>> all_dependencies;
        std::vector<std::pair<TxIdx, TxIdx>> active_dependencies;
        for (auto parent_idx : depgraph.Positions()) {
            for (auto child_idx : depgraph.GetReducedChildren(parent_idx)) {
                expected_dependencies.emplace_back(parent_idx, child_idx);
            }
        }
        for (auto tx_idx : m_transaction_idxs) {
            for (auto child_idx : m_tx_data[tx_idx].children) {
                all_dependencies.emplace_back(tx_idx, child_idx);
                if (m_tx_data[tx_idx].dep_top_idx[child_idx] != INVALID_SET_IDX) {
                    active_dependencies.emplace_back(tx_idx, child_idx);
                }
            }
        }
        std::sort(expected_dependencies.begin(), expected_dependencies.end());
        std::sort(all_dependencies.begin(), all_dependencies.end());
        assert(expected_dependencies == all_dependencies);

        //
        // Verify the chunks against the list of active dependencies
        //
        SetType chunk_cover;
        for (auto chunk_idx : m_chunk_idxs) {
            const auto& chunk_info = m_set_info[chunk_idx];
            // Verify that transactions in the chunk point back to it. This guarantees
            // that chunks are non-overlapping.
            for (auto tx_idx : chunk_info.transactions) {
                assert(m_tx_data[tx_idx].chunk_idx == chunk_idx);
            }
            assert(chunk_info.transactions.Any());
            assert(!chunk_cover.Overlaps(chunk_info.transactions));
            chunk_cover |= chunk_info.transactions;
            // Verify the chunk's transaction set: starting from an arbitrary chunk transaction,
            // and for every active dependency, if it contains the parent or child, adding the
            // other. It must have exactly N-1 active dependencies in it, guaranteeing it is
            // acyclic.
            SetType expected_chunk = SetType::Singleton(chunk_info.transactions.First());
            while (true) {
                auto old = expected_chunk;
                size_t active_dep_count{0};
                for (const auto& [par, chl] : active_dependencies) {
                    if (expected_chunk[par] || expected_chunk[chl]) {
                        expected_chunk.Set(par);
                        expected_chunk.Set(chl);
                        ++active_dep_count;
                    }
                }
                if (old == expected_chunk) {
                    assert(expected_chunk.Count() == active_dep_count + 1);
                    break;
                }
            }
            assert(chunk_info.transactions == expected_chunk);
            // Verify the chunk's feerate.
            assert(chunk_info.feerate == depgraph.FeeRate(expected_chunk));
            // Verify the chunk's reachable transactions.
            assert(m_reachable[chunk_idx] == GetReachable(expected_chunk));
        }
        // Verify that together, the chunks cover all transactions.
        assert(chunk_cover == depgraph.Positions());

        //
        // Verify transaction data.
        //
        assert(m_transaction_idxs == depgraph.Positions());
        for (auto tx_idx : m_transaction_idxs) {
            const auto& tx_data = m_tx_data[tx_idx];
            // Verify it has a valid chunk index, and that chunk includes this
            // transaction.
            assert(m_chunk_idxs[tx_data.chunk_idx]);
            assert(m_set_info[tx_data.chunk_idx].transactions[tx_idx]);
            // Verify parents/children.
            assert(tx_data.parents == depgraph.GetReducedParents(tx_idx));
            assert(tx_data.children == depgraph.GetReducedChildren(tx_idx));
        }

        //
        // Verify active dependencies' top sets.
        //
        for (const auto& [par_idx, chl_idx] : active_dependencies) {
            auto& top_dep_set = m_set_info[m_tx_data[par_idx].dep_top_idx[chl_idx]];
            // Verify the top_info's transactions: it must contain the parent, and for every
            // active dependency, except dep_idx itself, if it contains the parent or child, it
            // must contain both.
            SetType expected_top = SetType::Singleton(par_idx);
            while (true) {
                auto old = expected_top;
                for (const auto& [par2_idx, chl2_idx] : active_dependencies) {
                    if ((par2_idx != par_idx || chl2_idx != chl_idx) && (expected_top[par2_idx] || expected_top[chl2_idx])) {
                        expected_top.Set(par2_idx);
                        expected_top.Set(chl2_idx);
                    }
                }
                if (old == expected_top) break;
            }
            assert(!expected_top[chl_idx]);
            assert(top_dep_set.transactions == expected_top);
            // Verify the top_info's feerate.
            assert(top_dep_set.feerate == depgraph.FeeRate(expected_top));
        }

        //
        // Verify m_suboptimal_chunks.
        //
        for (size_t i = 0; i < m_suboptimal_chunks.size(); ++i) {
            auto chunk_idx = m_suboptimal_chunks[i];
            assert(chunk_idx < m_set_info.size());
        }

        //
        // Verify m_nonminimal_chunks.
        //
        SetType nonminimal_idxs;
        for (size_t i = 0; i < m_nonminimal_chunks.size(); ++i) {
            auto [chunk_idx, pivot, flags] = m_nonminimal_chunks[i];
            assert(m_tx_data[pivot].chunk_idx == chunk_idx);
            assert(!nonminimal_idxs[chunk_idx]);
            nonminimal_idxs.Set(chunk_idx);
        }
        assert(nonminimal_idxs.IsSubsetOf(m_chunk_idxs));
    }
};

/** Find or improve a linearization for a cluster.
 *
 * @param[in] depgraph            Dependency graph of the cluster to be linearized.
 * @param[in] max_iterations      Upper bound on the amount of work that will be done.
 * @param[in] rng_seed            A random number seed to control search order. This prevents peers
 *                                from predicting exactly which clusters would be hard for us to
 *                                linearize.
 * @param[in] old_linearization   An existing linearization for the cluster, or empty.
 * @param[in] is_topological      (Only relevant if old_linearization is not empty) Whether
 *                                old_linearization is topologically valid.
 * @return                        A tuple of:
 *                                - The resulting linearization. It is guaranteed to be at least as
 *                                  good (in the feerate diagram sense) as old_linearization.
 *                                - A boolean indicating whether the result is guaranteed to be
 *                                  optimal with minimal chunks.
 *                                - How many optimization steps were actually performed.
 */
template<typename SetType>
std::tuple<std::vector<DepGraphIndex>, bool, uint64_t> Linearize(const DepGraph<SetType>& depgraph, uint64_t max_iterations, uint64_t rng_seed, std::span<const DepGraphIndex> old_linearization = {}, bool is_topological = true) noexcept
{
    /** Initialize a spanning forest data structure for this cluster. */
    SpanningForestState forest(depgraph, rng_seed);
    if (!old_linearization.empty()) {
        forest.LoadLinearization(old_linearization);
        if (!is_topological) forest.MakeTopological();
    } else {
        forest.MakeTopological();
    }
    // Make improvement steps to it until we hit the max_iterations limit, or an optimal result
    // is found.
    if (forest.GetCost() < max_iterations) {
        forest.StartOptimizing();
        do {
            if (!forest.OptimizeStep()) break;
        } while (forest.GetCost() < max_iterations);
    }
    // Make chunk minimization steps until we hit the max_iterations limit, or all chunks are
    // minimal.
    bool optimal = false;
    if (forest.GetCost() < max_iterations) {
        forest.StartMinimizing();
        do {
            if (!forest.MinimizeStep()) {
                optimal = true;
                break;
            }
        } while (forest.GetCost() < max_iterations);
    }
    return {forest.GetLinearization(), optimal, forest.GetCost()};
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

} // namespace cluster_linearize

#endif // BITCOIN_CLUSTER_LINEARIZE_H
