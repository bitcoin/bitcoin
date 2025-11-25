// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txgraph.h>

#include <cluster_linearize.h>
#include <random.h>
#include <util/bitset.h>
#include <util/check.h>
#include <util/feefrac.h>
#include <util/vector.h>

#include <compare>
#include <functional>
#include <memory>
#include <set>
#include <span>
#include <unordered_set>
#include <utility>

namespace {

using namespace cluster_linearize;

/** The maximum number of levels a TxGraph can have (0 = main, 1 = staging). */
static constexpr int MAX_LEVELS{2};

// Forward declare the TxGraph implementation class.
class TxGraphImpl;

/** Position of a DepGraphIndex within a Cluster::m_linearization. */
using LinearizationIndex = uint32_t;
/** Position of a Cluster within TxGraphImpl::ClusterSet::m_clusters. */
using ClusterSetIndex = uint32_t;

/** Quality levels for cached cluster linearizations. */
enum class QualityLevel
{
    /** This is a singleton cluster consisting of a transaction that individually exceeds the
     *  cluster size limit. It cannot be merged with anything. */
    OVERSIZED_SINGLETON,
    /** This cluster may have multiple disconnected components, which are all NEEDS_RELINEARIZE. */
    NEEDS_SPLIT,
    /** This cluster may have multiple disconnected components, which are all ACCEPTABLE. */
    NEEDS_SPLIT_ACCEPTABLE,
    /** This cluster has undergone changes that warrant re-linearization. */
    NEEDS_RELINEARIZE,
    /** The minimal level of linearization has been performed, but it is not known to be optimal. */
    ACCEPTABLE,
    /** The linearization is known to be optimal. */
    OPTIMAL,
    /** This cluster is not registered in any ClusterSet::m_clusters.
     *  This must be the last entry in QualityLevel as ClusterSet::m_clusters is sized using it. */
    NONE,
};

/** Information about a transaction inside TxGraphImpl::Trim. */
struct TrimTxData
{
    // Fields populated by Cluster::AppendTrimData(). These are immutable after TrimTxData
    // construction.
    /** Chunk feerate for this transaction. */
    FeePerWeight m_chunk_feerate;
    /** GraphIndex of the transaction. */
    TxGraph::GraphIndex m_index;
    /** Size of the transaction. */
    uint32_t m_tx_size;

    // Fields only used internally by TxGraphImpl::Trim():
    /** Number of unmet dependencies this transaction has. -1 if the transaction is included. */
    uint32_t m_deps_left;
    /** Number of dependencies that apply to this transaction as child. */
    uint32_t m_parent_count;
    /** Where in deps_by_child those dependencies begin. */
    uint32_t m_parent_offset;
    /** Number of dependencies that apply to this transaction as parent. */
    uint32_t m_children_count;
    /** Where in deps_by_parent those dependencies begin. */
    uint32_t m_children_offset;

    // Fields only used internally by TxGraphImpl::Trim()'s union-find implementation, and only for
    // transactions that are definitely included or definitely rejected.
    //
    // As transactions get processed, they get organized into trees which form partitions
    // representing the would-be clusters up to that point. The root of each tree is a
    // representative for that partition. See
    // https://en.wikipedia.org/wiki/Disjoint-set_data_structure.
    //
    /** Pointer to another TrimTxData, towards the root of the tree. If this is a root, m_uf_parent
     *  is equal to this itself. */
    TrimTxData* m_uf_parent;
    /** If this is a root, the total number of transactions in the partition. */
    uint32_t m_uf_count;
    /** If this is a root, the total size of transactions in the partition. */
    uint64_t m_uf_size;
};

/** A grouping of connected transactions inside a TxGraphImpl::ClusterSet. */
class Cluster
{
    friend class TxGraphImpl;
    friend class BlockBuilderImpl;

protected:
    using GraphIndex = TxGraph::GraphIndex;
    using SetType = BitSet<MAX_CLUSTER_COUNT_LIMIT>;
    /** The quality level of m_linearization. */
    QualityLevel m_quality{QualityLevel::NONE};
    /** Which position this Cluster has in TxGraphImpl::ClusterSet::m_clusters[m_quality]. */
    ClusterSetIndex m_setindex{ClusterSetIndex(-1)};
    /** Sequence number for this Cluster (for tie-breaking comparison between equal-chunk-feerate
        transactions in distinct clusters). */
    uint64_t m_sequence;

    explicit Cluster(uint64_t sequence) noexcept : m_sequence(sequence) {}

public:
    // Provide virtual destructor, for safe polymorphic usage inside std::unique_ptr.
    virtual ~Cluster() = default;

    // Cannot move or copy (would invalidate Cluster* in Locator and ClusterSet). */
    Cluster(const Cluster&) = delete;
    Cluster& operator=(const Cluster&) = delete;
    Cluster(Cluster&&) = delete;
    Cluster& operator=(Cluster&&) = delete;

    // Generic helper functions.

    /** Whether the linearization of this Cluster can be exposed. */
    bool IsAcceptable(bool after_split = false) const noexcept
    {
        return m_quality == QualityLevel::ACCEPTABLE || m_quality == QualityLevel::OPTIMAL ||
               (after_split && m_quality == QualityLevel::NEEDS_SPLIT_ACCEPTABLE);
    }
    /** Whether the linearization of this Cluster is optimal. */
    bool IsOptimal() const noexcept
    {
        return m_quality == QualityLevel::OPTIMAL;
    }
    /** Whether this cluster is oversized. Note that no changes that can cause oversizedness are
     *  ever applied, so the only way a materialized Cluster object can be oversized is by being
     *  an individually oversized transaction singleton. */
    bool IsOversized() const noexcept { return m_quality == QualityLevel::OVERSIZED_SINGLETON; }
    /** Whether this cluster requires splitting. */
    bool NeedsSplitting() const noexcept
    {
        return m_quality == QualityLevel::NEEDS_SPLIT ||
               m_quality == QualityLevel::NEEDS_SPLIT_ACCEPTABLE;
    }

    /** Get the smallest number of transactions this Cluster is intended for. */
    virtual DepGraphIndex GetMinIntendedTxCount() const noexcept = 0;
    /** Get the maximum number of transactions this Cluster supports. */
    virtual DepGraphIndex GetMaxTxCount() const noexcept = 0;
    /** Total memory usage currently for this Cluster, including all its dynamic memory, plus Cluster
     *  structure itself, and ClusterSet::m_clusters entry. */
    virtual size_t TotalMemoryUsage() const noexcept = 0;
    /** Determine the range of DepGraphIndexes used by this Cluster. */
    virtual DepGraphIndex GetDepGraphIndexRange() const noexcept = 0;
    /** Get the number of transactions in this Cluster. */
    virtual LinearizationIndex GetTxCount() const noexcept = 0;
    /** Get the total size of the transactions in this Cluster. */
    virtual uint64_t GetTotalTxSize() const noexcept = 0;
    /** Given a DepGraphIndex into this Cluster, find the corresponding GraphIndex. */
    virtual GraphIndex GetClusterEntry(DepGraphIndex index) const noexcept = 0;
    /** Append a transaction with given GraphIndex at the end of this Cluster and its
     *  linearization. Return the DepGraphIndex it was placed at. */
    virtual DepGraphIndex AppendTransaction(GraphIndex graph_idx, FeePerWeight feerate) noexcept = 0;
    /** Add dependencies to a given child in this cluster. */
    virtual void AddDependencies(SetType parents, DepGraphIndex child) noexcept = 0;
    /** Invoke visitor_fn for each transaction in the cluster, in linearization order, then wipe this Cluster. */
    virtual void ExtractTransactions(const std::function<void (DepGraphIndex, GraphIndex, FeePerWeight, SetType)>& visit_fn) noexcept = 0;
    /** Figure out what level this Cluster exists at in the graph. In most cases this is known by
     *  the caller already (see all "int level" arguments below), but not always. */
    virtual int GetLevel(const TxGraphImpl& graph) const noexcept = 0;
    /** Only called by TxGraphImpl::SwapIndexes. */
    virtual void UpdateMapping(DepGraphIndex cluster_idx, GraphIndex graph_idx) noexcept = 0;
    /** Push changes to Cluster and its linearization to the TxGraphImpl Entry objects. */
    virtual void Updated(TxGraphImpl& graph, int level) noexcept = 0;
    /** Create a copy of this Cluster in staging, returning a pointer to it (used by PullIn). */
    virtual Cluster* CopyToStaging(TxGraphImpl& graph) const noexcept = 0;
    /** Get the list of Clusters in main that conflict with this one (which is assumed to be in staging). */
    virtual void GetConflicts(const TxGraphImpl& graph, std::vector<Cluster*>& out) const noexcept = 0;
    /** Mark all the Entry objects belonging to this staging Cluster as missing. The Cluster must be
     *  deleted immediately after. */
    virtual void MakeStagingTransactionsMissing(TxGraphImpl& graph) noexcept = 0;
    /** Remove all transactions from a (non-empty) Cluster. */
    virtual void Clear(TxGraphImpl& graph, int level) noexcept = 0;
    /** Change a Cluster's level from 1 (staging) to 0 (main). */
    virtual void MoveToMain(TxGraphImpl& graph) noexcept = 0;
    /** Minimize this Cluster's memory usage. */
    virtual void Compact() noexcept = 0;

    // Functions that implement the Cluster-specific side of internal TxGraphImpl mutations.

    /** Apply all removals from the front of to_remove that apply to this Cluster, popping them
     *  off. There must be at least one such entry. */
    virtual void ApplyRemovals(TxGraphImpl& graph, int level, std::span<GraphIndex>& to_remove) noexcept = 0;
    /** Split this cluster (must have a NEEDS_SPLIT* quality). Returns whether to delete this
     *  Cluster afterwards. */
    [[nodiscard]] virtual bool Split(TxGraphImpl& graph, int level) noexcept = 0;
    /** Move all transactions from cluster to *this (as separate components). */
    virtual void Merge(TxGraphImpl& graph, int level, Cluster& cluster) noexcept = 0;
    /** Given a span of (parent, child) pairs that all belong to this Cluster, apply them. */
    virtual void ApplyDependencies(TxGraphImpl& graph, int level, std::span<std::pair<GraphIndex, GraphIndex>> to_apply) noexcept = 0;
    /** Improve the linearization of this Cluster. Returns how much work was performed and whether
     *  the Cluster's QualityLevel improved as a result. */
    virtual std::pair<uint64_t, bool> Relinearize(TxGraphImpl& graph, int level, uint64_t max_iters) noexcept = 0;
    /** For every chunk in the cluster, append its FeeFrac to ret. */
    virtual void AppendChunkFeerates(std::vector<FeeFrac>& ret) const noexcept = 0;
    /** Add a TrimTxData entry (filling m_chunk_feerate, m_index, m_tx_size) for every
     *  transaction in the Cluster to ret. Implicit dependencies between consecutive transactions
     *  in the linearization are added to deps. Return the Cluster's total transaction size. */
    virtual uint64_t AppendTrimData(std::vector<TrimTxData>& ret, std::vector<std::pair<GraphIndex, GraphIndex>>& deps) const noexcept = 0;

    // Functions that implement the Cluster-specific side of public TxGraph functions.

    /** Process elements from the front of args that apply to this cluster, and append Refs for the
     *  union of their ancestors to output. */
    virtual void GetAncestorRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept = 0;
    /** Process elements from the front of args that apply to this cluster, and append Refs for the
     *  union of their descendants to output. */
    virtual void GetDescendantRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept = 0;
    /** Populate range with refs for the transactions in this Cluster's linearization, from
     *  position start_pos until start_pos+range.size()-1, inclusive. Returns whether that
     *  range includes the last transaction in the linearization. */
    virtual bool GetClusterRefs(TxGraphImpl& graph, std::span<TxGraph::Ref*> range, LinearizationIndex start_pos) noexcept = 0;
    /** Get the individual transaction feerate of a Cluster element. */
    virtual FeePerWeight GetIndividualFeerate(DepGraphIndex idx) noexcept = 0;
    /** Modify the fee of a Cluster element. */
    virtual void SetFee(TxGraphImpl& graph, int level, DepGraphIndex idx, int64_t fee) noexcept = 0;

    // Debugging functions.

    virtual void SanityCheck(const TxGraphImpl& graph, int level) const = 0;
};

/** An implementation of Cluster that uses a DepGraph and vectors, to support arbitrary numbers of
 *  transactions up to MAX_CLUSTER_COUNT_LIMIT. */
class GenericClusterImpl final : public Cluster
{
    friend class TxGraphImpl;
    /** The DepGraph for this cluster, holding all feerates, and ancestors/descendants. */
    DepGraph<SetType> m_depgraph;
    /** m_mapping[i] gives the GraphIndex for the position i transaction in m_depgraph. Values for
     *  positions i that do not exist in m_depgraph shouldn't ever be accessed and thus don't
     *  matter. m_mapping.size() equals m_depgraph.PositionRange(). */
    std::vector<GraphIndex> m_mapping;
    /** The current linearization of the cluster. m_linearization.size() equals
     *  m_depgraph.TxCount(). This is always kept topological. */
    std::vector<DepGraphIndex> m_linearization;

public:
    /** The smallest number of transactions this Cluster implementation is intended for. */
    static constexpr DepGraphIndex MIN_INTENDED_TX_COUNT{2};
    /** The largest number of transactions this Cluster implementation supports. */
    static constexpr DepGraphIndex MAX_TX_COUNT{SetType::Size()};

    GenericClusterImpl() noexcept = delete;
    /** Construct an empty GenericClusterImpl. */
    explicit GenericClusterImpl(uint64_t sequence) noexcept;

    size_t TotalMemoryUsage() const noexcept final;
    constexpr DepGraphIndex GetMinIntendedTxCount() const noexcept final { return MIN_INTENDED_TX_COUNT; }
    constexpr DepGraphIndex GetMaxTxCount() const noexcept final { return MAX_TX_COUNT; }
    DepGraphIndex GetDepGraphIndexRange() const noexcept final { return m_depgraph.PositionRange(); }
    LinearizationIndex GetTxCount() const noexcept final { return m_linearization.size(); }
    uint64_t GetTotalTxSize() const noexcept final;
    GraphIndex GetClusterEntry(DepGraphIndex index) const noexcept final { return m_mapping[index]; }
    DepGraphIndex AppendTransaction(GraphIndex graph_idx, FeePerWeight feerate) noexcept final;
    void AddDependencies(SetType parents, DepGraphIndex child) noexcept final;
    void ExtractTransactions(const std::function<void (DepGraphIndex, GraphIndex, FeePerWeight, SetType)>& visit_fn) noexcept final;
    int GetLevel(const TxGraphImpl& graph) const noexcept final;
    void UpdateMapping(DepGraphIndex cluster_idx, GraphIndex graph_idx) noexcept final { m_mapping[cluster_idx] = graph_idx; }
    void Updated(TxGraphImpl& graph, int level) noexcept final;
    Cluster* CopyToStaging(TxGraphImpl& graph) const noexcept final;
    void GetConflicts(const TxGraphImpl& graph, std::vector<Cluster*>& out) const noexcept final;
    void MakeStagingTransactionsMissing(TxGraphImpl& graph) noexcept final;
    void Clear(TxGraphImpl& graph, int level) noexcept final;
    void MoveToMain(TxGraphImpl& graph) noexcept final;
    void Compact() noexcept final;
    void ApplyRemovals(TxGraphImpl& graph, int level, std::span<GraphIndex>& to_remove) noexcept final;
    [[nodiscard]] bool Split(TxGraphImpl& graph, int level) noexcept final;
    void Merge(TxGraphImpl& graph, int level, Cluster& cluster) noexcept final;
    void ApplyDependencies(TxGraphImpl& graph, int level, std::span<std::pair<GraphIndex, GraphIndex>> to_apply) noexcept final;
    std::pair<uint64_t, bool> Relinearize(TxGraphImpl& graph, int level, uint64_t max_iters) noexcept final;
    void AppendChunkFeerates(std::vector<FeeFrac>& ret) const noexcept final;
    uint64_t AppendTrimData(std::vector<TrimTxData>& ret, std::vector<std::pair<GraphIndex, GraphIndex>>& deps) const noexcept final;
    void GetAncestorRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept final;
    void GetDescendantRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept final;
    bool GetClusterRefs(TxGraphImpl& graph, std::span<TxGraph::Ref*> range, LinearizationIndex start_pos) noexcept final;
    FeePerWeight GetIndividualFeerate(DepGraphIndex idx) noexcept final;
    void SetFee(TxGraphImpl& graph, int level, DepGraphIndex idx, int64_t fee) noexcept final;
    void SanityCheck(const TxGraphImpl& graph, int level) const final;
};

/** An implementation of Cluster that only supports 1 transaction. */
class SingletonClusterImpl final : public Cluster
{
    friend class TxGraphImpl;

    /** The feerate of the (singular) transaction in this Cluster. */
    FeePerWeight m_feerate;
    /** Constant to indicate that this Cluster is empty. */
    static constexpr auto NO_GRAPH_INDEX = GraphIndex(-1);
    /** The GraphIndex of the transaction. NO_GRAPH_INDEX if this Cluster is empty. */
    GraphIndex m_graph_index = NO_GRAPH_INDEX;

public:
    /** The smallest number of transactions this Cluster implementation is intended for. */
    static constexpr DepGraphIndex MIN_INTENDED_TX_COUNT{1};
    /** The largest number of transactions this Cluster implementation supports. */
    static constexpr DepGraphIndex MAX_TX_COUNT{1};

    SingletonClusterImpl() noexcept = delete;
    /** Construct an empty SingletonClusterImpl. */
    explicit SingletonClusterImpl(uint64_t sequence) noexcept : Cluster(sequence) {}

    size_t TotalMemoryUsage() const noexcept final;
    constexpr DepGraphIndex GetMinIntendedTxCount() const noexcept final { return MIN_INTENDED_TX_COUNT; }
    constexpr DepGraphIndex GetMaxTxCount() const noexcept final { return MAX_TX_COUNT; }
    LinearizationIndex GetTxCount() const noexcept final { return m_graph_index != NO_GRAPH_INDEX; }
    DepGraphIndex GetDepGraphIndexRange() const noexcept final { return GetTxCount(); }
    uint64_t GetTotalTxSize() const noexcept final { return GetTxCount() ? m_feerate.size : 0; }
    GraphIndex GetClusterEntry(DepGraphIndex index) const noexcept final { Assume(index == 0); Assume(GetTxCount()); return m_graph_index; }
    DepGraphIndex AppendTransaction(GraphIndex graph_idx, FeePerWeight feerate) noexcept final;
    void AddDependencies(SetType parents, DepGraphIndex child) noexcept final;
    void ExtractTransactions(const std::function<void (DepGraphIndex, GraphIndex, FeePerWeight, SetType)>& visit_fn) noexcept final;
    int GetLevel(const TxGraphImpl& graph) const noexcept final;
    void UpdateMapping(DepGraphIndex cluster_idx, GraphIndex graph_idx) noexcept final { Assume(cluster_idx == 0); m_graph_index = graph_idx; }
    void Updated(TxGraphImpl& graph, int level) noexcept final;
    Cluster* CopyToStaging(TxGraphImpl& graph) const noexcept final;
    void GetConflicts(const TxGraphImpl& graph, std::vector<Cluster*>& out) const noexcept final;
    void MakeStagingTransactionsMissing(TxGraphImpl& graph) noexcept final;
    void Clear(TxGraphImpl& graph, int level) noexcept final;
    void MoveToMain(TxGraphImpl& graph) noexcept final;
    void Compact() noexcept final;
    void ApplyRemovals(TxGraphImpl& graph, int level, std::span<GraphIndex>& to_remove) noexcept final;
    [[nodiscard]] bool Split(TxGraphImpl& graph, int level) noexcept final;
    void Merge(TxGraphImpl& graph, int level, Cluster& cluster) noexcept final;
    void ApplyDependencies(TxGraphImpl& graph, int level, std::span<std::pair<GraphIndex, GraphIndex>> to_apply) noexcept final;
    std::pair<uint64_t, bool> Relinearize(TxGraphImpl& graph, int level, uint64_t max_iters) noexcept final;
    void AppendChunkFeerates(std::vector<FeeFrac>& ret) const noexcept final;
    uint64_t AppendTrimData(std::vector<TrimTxData>& ret, std::vector<std::pair<GraphIndex, GraphIndex>>& deps) const noexcept final;
    void GetAncestorRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept final;
    void GetDescendantRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept final;
    bool GetClusterRefs(TxGraphImpl& graph, std::span<TxGraph::Ref*> range, LinearizationIndex start_pos) noexcept final;
    FeePerWeight GetIndividualFeerate(DepGraphIndex idx) noexcept final;
    void SetFee(TxGraphImpl& graph, int level, DepGraphIndex idx, int64_t fee) noexcept final;
    void SanityCheck(const TxGraphImpl& graph, int level) const final;
};

/** The transaction graph, including staged changes.
 *
 * The overall design of the data structure consists of 3 interlinked representations:
 * - The transactions (held as a vector of TxGraphImpl::Entry inside TxGraphImpl).
 * - The clusters (Cluster objects in per-quality vectors inside TxGraphImpl::ClusterSet).
 * - The Refs (TxGraph::Ref objects, held externally by users of the TxGraph class)
 *
 * The Clusters are kept in one or two ClusterSet objects, one for the "main" graph, and one for
 * the proposed changes ("staging"). If a transaction occurs in both, they share the same Entry,
 * but there will be a separate Cluster per graph.
 *
 * Clusters and Refs contain the index of the Entry objects they refer to, and the Entry objects
 * refer back to the Clusters and Refs the corresponding transaction is contained in.
 *
 * While redundant, this permits moving all of them independently, without invalidating things
 * or costly iteration to fix up everything:
 * - Entry objects can be moved to fill holes left by removed transactions in the Entry vector
 *   (see TxGraphImpl::Compact).
 * - Clusters can be rewritten continuously (removals can cause them to split, new dependencies
 *   can cause them to be merged).
 * - Ref objects can be held outside the class, while permitting them to be moved around, and
 *   inherited from.
 */
class TxGraphImpl final : public TxGraph
{
    friend class Cluster;
    friend class SingletonClusterImpl;
    friend class GenericClusterImpl;
    friend class BlockBuilderImpl;
private:
    /** Internal RNG. */
    FastRandomContext m_rng;
    /** This TxGraphImpl's maximum cluster count limit. */
    const DepGraphIndex m_max_cluster_count;
    /** This TxGraphImpl's maximum cluster size limit. */
    const uint64_t m_max_cluster_size;
    /** The number of linearization improvement steps needed per cluster to be considered
     *  acceptable. */
    const uint64_t m_acceptable_iters;

    /** Information about one group of Clusters to be merged. */
    struct GroupEntry
    {
        /** Where the clusters to be merged start in m_group_clusters. */
        uint32_t m_cluster_offset;
        /** How many clusters to merge. */
        uint32_t m_cluster_count;
        /** Where the dependencies for this cluster group in m_deps_to_add start. */
        uint32_t m_deps_offset;
        /** How many dependencies to add. */
        uint32_t m_deps_count;
    };

    /** Information about all groups of Clusters to be merged. */
    struct GroupData
    {
        /** The groups of Clusters to be merged. */
        std::vector<GroupEntry> m_groups;
        /** Which clusters are to be merged. GroupEntry::m_cluster_offset indexes into this. */
        std::vector<Cluster*> m_group_clusters;
    };

    /** The collection of all Clusters in main or staged. */
    struct ClusterSet
    {
        /** The vectors of clusters, one vector per quality level. ClusterSetIndex indexes into each. */
        std::array<std::vector<std::unique_ptr<Cluster>>, int(QualityLevel::NONE)> m_clusters;
        /** Which removals have yet to be applied. */
        std::vector<GraphIndex> m_to_remove;
        /** Which dependencies are to be added ((parent,child) pairs). GroupData::m_deps_offset indexes
         *  into this. */
        std::vector<std::pair<GraphIndex, GraphIndex>> m_deps_to_add;
        /** Information about the merges to be performed, if known. */
        std::optional<GroupData> m_group_data = GroupData{};
        /** Which entries were removed in this ClusterSet (so they can be wiped on abort). This
         *  includes all entries which have an (R) removed locator at this level (staging only),
         *  plus optionally any transaction in m_unlinked. */
        std::vector<GraphIndex> m_removed;
        /** Total number of transactions in this graph (sum of all transaction counts in all
         *  Clusters, and for staging also those inherited from the main ClusterSet). */
        GraphIndex m_txcount{0};
        /** Total number of individually oversized transactions in the graph. */
        GraphIndex m_txcount_oversized{0};
        /** Whether this graph is oversized (if known). */
        std::optional<bool> m_oversized{false};
        /** The combined TotalMemoryUsage of all clusters in this level (only Clusters that
         *  are materialized; in staging, implicit Clusters from main are not counted), */
        size_t m_cluster_usage{0};

        ClusterSet() noexcept = default;
    };

    /** The main ClusterSet. */
    ClusterSet m_main_clusterset;
    /** The staging ClusterSet, if any. */
    std::optional<ClusterSet> m_staging_clusterset;
    /** Next sequence number to assign to created Clusters. */
    uint64_t m_next_sequence_counter{0};

    /** Information about a chunk in the main graph. */
    struct ChunkData
    {
        /** The Entry which is the last transaction of the chunk. */
        mutable GraphIndex m_graph_index;
        /** How many transactions the chunk contains (-1 = singleton tail of cluster). */
        LinearizationIndex m_chunk_count;

        ChunkData(GraphIndex graph_index, LinearizationIndex chunk_count) noexcept :
            m_graph_index{graph_index}, m_chunk_count{chunk_count} {}
    };

    /** Compare two Cluster* by their m_sequence value (while supporting nullptr). */
    static std::strong_ordering CompareClusters(Cluster* a, Cluster* b) noexcept
    {
        // The nullptr pointer compares before everything else.
        if (a == nullptr || b == nullptr) {
            return (a != nullptr) <=> (b != nullptr);
        }
        // If neither pointer is nullptr, compare the Clusters' sequence numbers.
        Assume(a == b || a->m_sequence != b->m_sequence);
        return a->m_sequence <=> b->m_sequence;
    }

    /** Compare two entries (which must both exist within the main graph). */
    std::strong_ordering CompareMainTransactions(GraphIndex a, GraphIndex b) const noexcept
    {
        Assume(a < m_entries.size() && b < m_entries.size());
        const auto& entry_a = m_entries[a];
        const auto& entry_b = m_entries[b];
        // Compare chunk feerates, and return result if it differs.
        auto feerate_cmp = FeeRateCompare(entry_b.m_main_chunk_feerate, entry_a.m_main_chunk_feerate);
        if (feerate_cmp < 0) return std::strong_ordering::less;
        if (feerate_cmp > 0) return std::strong_ordering::greater;
        // Compare Cluster m_sequence as tie-break for equal chunk feerates.
        const auto& locator_a = entry_a.m_locator[0];
        const auto& locator_b = entry_b.m_locator[0];
        Assume(locator_a.IsPresent() && locator_b.IsPresent());
        if (locator_a.cluster != locator_b.cluster) {
            return CompareClusters(locator_a.cluster, locator_b.cluster);
        }
        // As final tie-break, compare position within cluster linearization.
        return entry_a.m_main_lin_index <=> entry_b.m_main_lin_index;
    }

    /** Comparator for ChunkData objects in mining order. */
    class ChunkOrder
    {
        const TxGraphImpl* const m_graph;
    public:
        explicit ChunkOrder(const TxGraphImpl* graph) : m_graph(graph) {}

        bool operator()(const ChunkData& a, const ChunkData& b) const noexcept
        {
            return m_graph->CompareMainTransactions(a.m_graph_index, b.m_graph_index) < 0;
        }
    };

    /** Definition for the mining index type. */
    using ChunkIndex = std::set<ChunkData, ChunkOrder>;

    /** Index of ChunkData objects, indexing the last transaction in each chunk in the main
     *  graph. */
    ChunkIndex m_main_chunkindex;
    /** Number of index-observing objects in existence (BlockBuilderImpls). */
    size_t m_main_chunkindex_observers{0};
    /** Cache of discarded ChunkIndex node handles to reuse, avoiding additional allocation. */
    std::vector<ChunkIndex::node_type> m_main_chunkindex_discarded;

    /** A Locator that describes whether, where, and in which Cluster an Entry appears.
     *  Every Entry has MAX_LEVELS locators, as it may appear in one Cluster per level.
     *
     *  Each level of a Locator is in one of three states:
     *
     *  - (P)resent: actually occurs in a Cluster at that level.
     *
     *  - (M)issing:
     *    - In the main graph:    the transaction does not exist in main.
     *    - In the staging graph: the transaction's existence is the same as in main. If it doesn't
     *                            exist in main, (M) in staging means it does not exist there
     *                            either. If it does exist in main, (M) in staging means the
     *                            cluster it is in has not been modified in staging, and thus the
     *                            transaction implicitly exists in staging too (without explicit
     *                            Cluster object; see PullIn() to create it in staging too).
     *
     *  - (R)emoved: only possible in staging; it means the transaction exists in main, but is
     *               removed in staging.
     *
     * The following combinations are possible:
     * - (M,M): the transaction doesn't exist in either graph.
     * - (P,M): the transaction exists in both, but only exists explicitly in a Cluster object in
     *          main. Its existence in staging is inherited from main.
     * - (P,P): the transaction exists in both, and is materialized in both. Thus, the clusters
     *          and/or their linearizations may be different in main and staging.
     * - (M,P): the transaction is added in staging, and does not exist in main.
     * - (P,R): the transaction exists in main, but is removed in staging.
     *
     * When staging does not exist, only (M,M) and (P,M) are possible.
     */
    struct Locator
    {
        /** Which Cluster the Entry appears in (nullptr = missing). */
        Cluster* cluster{nullptr};
        /** Where in the Cluster it appears (if cluster == nullptr: 0 = missing, -1 = removed). */
        DepGraphIndex index{0};

        /** Mark this Locator as missing (= same as lower level, or non-existing if level 0). */
        void SetMissing() noexcept { cluster = nullptr; index = 0; }
        /** Mark this Locator as removed (not allowed in level 0). */
        void SetRemoved() noexcept { cluster = nullptr; index = DepGraphIndex(-1); }
        /** Mark this Locator as present, in the specified Cluster. */
        void SetPresent(Cluster* c, DepGraphIndex i) noexcept { cluster = c; index = i; }
        /** Check if this Locator is missing. */
        bool IsMissing() const noexcept { return cluster == nullptr && index == 0; }
        /** Check if this Locator is removed. */
        bool IsRemoved() const noexcept { return cluster == nullptr && index == DepGraphIndex(-1); }
        /** Check if this Locator is present (in some Cluster). */
        bool IsPresent() const noexcept { return cluster != nullptr; }
    };

    /** Internal information about each transaction in a TxGraphImpl. */
    struct Entry
    {
        /** Pointer to the corresponding Ref object if any, or nullptr if unlinked. */
        Ref* m_ref{nullptr};
        /** Iterator to the corresponding ChunkData, if any, and m_main_chunkindex.end() otherwise.
         *  This is initialized on construction of the Entry, in AddTransaction. */
        ChunkIndex::iterator m_main_chunkindex_iterator;
        /** Which Cluster and position therein this Entry appears in. ([0] = main, [1] = staged). */
        Locator m_locator[MAX_LEVELS];
        /** The chunk feerate of this transaction in main (if present in m_locator[0]). */
        FeePerWeight m_main_chunk_feerate;
        /** The position this transaction has in the main linearization (if present). */
        LinearizationIndex m_main_lin_index;
    };

    /** The set of all transactions (in all levels combined). GraphIndex values index into this. */
    std::vector<Entry> m_entries;

    /** Set of Entries which have no linked Ref anymore. */
    std::vector<GraphIndex> m_unlinked;

public:
    /** Construct a new TxGraphImpl with the specified limits. */
    explicit TxGraphImpl(DepGraphIndex max_cluster_count, uint64_t max_cluster_size, uint64_t acceptable_iters) noexcept :
        m_max_cluster_count(max_cluster_count),
        m_max_cluster_size(max_cluster_size),
        m_acceptable_iters(acceptable_iters),
        m_main_chunkindex(ChunkOrder(this))
    {
        Assume(max_cluster_count >= 1);
        Assume(max_cluster_count <= MAX_CLUSTER_COUNT_LIMIT);
    }

    /** Destructor. */
    ~TxGraphImpl() noexcept;

    // Cannot move or copy (would invalidate TxGraphImpl* in Ref, MiningOrder, EvictionOrder).
    TxGraphImpl(const TxGraphImpl&) = delete;
    TxGraphImpl& operator=(const TxGraphImpl&) = delete;
    TxGraphImpl(TxGraphImpl&&) = delete;
    TxGraphImpl& operator=(TxGraphImpl&&) = delete;

    // Simple helper functions.

    /** Swap the Entry referred to by a and the one referred to by b. */
    void SwapIndexes(GraphIndex a, GraphIndex b) noexcept;
    /** If idx exists in the specified level ClusterSet (explicitly, or in the level below and not
    *   removed), return the Cluster it is in. Otherwise, return nullptr. */
    Cluster* FindCluster(GraphIndex idx, int level) const noexcept { return FindClusterAndLevel(idx, level).first; }
    /** Like FindCluster, but also return what level the match was found in (-1 if not found). */
    std::pair<Cluster*, int> FindClusterAndLevel(GraphIndex idx, int level) const noexcept;
    /** Extract a Cluster from its ClusterSet, and set its quality to QualityLevel::NONE. */
    std::unique_ptr<Cluster> ExtractCluster(int level, QualityLevel quality, ClusterSetIndex setindex) noexcept;
    /** Delete a Cluster. */
    void DeleteCluster(Cluster& cluster, int level) noexcept;
    /** Insert a Cluster into its ClusterSet. */
    ClusterSetIndex InsertCluster(int level, std::unique_ptr<Cluster>&& cluster, QualityLevel quality) noexcept;
    /** Change the QualityLevel of a Cluster (identified by old_quality and old_index). */
    void SetClusterQuality(int level, QualityLevel old_quality, ClusterSetIndex old_index, QualityLevel new_quality) noexcept;
    /** Get the index of the top level ClusterSet (staging if it exists, main otherwise). */
    int GetTopLevel() const noexcept { return m_staging_clusterset.has_value(); }
    /** Get the specified level (staging if it exists and level is TOP, main otherwise). */
    int GetSpecifiedLevel(Level level) const noexcept { return level == Level::TOP && m_staging_clusterset.has_value(); }
    /** Get a reference to the ClusterSet at the specified level (which must exist). */
    ClusterSet& GetClusterSet(int level) noexcept;
    const ClusterSet& GetClusterSet(int level) const noexcept;
    /** Make a transaction not exist at a specified level. It must currently exist there.
     *  oversized_tx indicates whether the transaction is an individually-oversized one
     *  (OVERSIZED_SINGLETON). */
    void ClearLocator(int level, GraphIndex index, bool oversized_tx) noexcept;
    /** Find which Clusters in main conflict with ones in staging. */
    std::vector<Cluster*> GetConflicts() const noexcept;
    /** Clear an Entry's ChunkData. */
    void ClearChunkData(Entry& entry) noexcept;
    /** Give an Entry a ChunkData object. */
    void CreateChunkData(GraphIndex idx, LinearizationIndex chunk_count) noexcept;
    /** Create an empty GenericClusterImpl object. */
    std::unique_ptr<GenericClusterImpl> CreateEmptyGenericCluster() noexcept
    {
        return std::make_unique<GenericClusterImpl>(m_next_sequence_counter++);
    }
    /** Create an empty SingletonClusterImpl object. */
    std::unique_ptr<SingletonClusterImpl> CreateEmptySingletonCluster() noexcept
    {
        return std::make_unique<SingletonClusterImpl>(m_next_sequence_counter++);
    }
    /** Create an empty Cluster of the appropriate implementation for the specified (maximum) tx
     *  count. */
    std::unique_ptr<Cluster> CreateEmptyCluster(DepGraphIndex tx_count) noexcept
    {
        if (tx_count >= SingletonClusterImpl::MIN_INTENDED_TX_COUNT && tx_count <= SingletonClusterImpl::MAX_TX_COUNT) {
            return CreateEmptySingletonCluster();
        }
        if (tx_count >= GenericClusterImpl::MIN_INTENDED_TX_COUNT && tx_count <= GenericClusterImpl::MAX_TX_COUNT) {
            return CreateEmptyGenericCluster();
        }
        assert(false);
        return {};
    }

    // Functions for handling Refs.

    /** Only called by Ref's move constructor/assignment to update Ref locations. */
    void UpdateRef(GraphIndex idx, Ref& new_location) noexcept final
    {
        auto& entry = m_entries[idx];
        Assume(entry.m_ref != nullptr);
        entry.m_ref = &new_location;
    }

    /** Only called by Ref::~Ref to unlink Refs, and Ref's move assignment. */
    void UnlinkRef(GraphIndex idx) noexcept final
    {
        auto& entry = m_entries[idx];
        Assume(entry.m_ref != nullptr);
        Assume(m_main_chunkindex_observers == 0 || !entry.m_locator[0].IsPresent());
        entry.m_ref = nullptr;
        // Mark the transaction as to be removed in all levels where it explicitly or implicitly
        // exists.
        bool exists_anywhere{false};
        bool exists{false};
        for (int level = 0; level <= GetTopLevel(); ++level) {
            if (entry.m_locator[level].IsPresent()) {
                exists_anywhere = true;
                exists = true;
            } else if (entry.m_locator[level].IsRemoved()) {
                exists = false;
            }
            if (exists) {
                auto& clusterset = GetClusterSet(level);
                clusterset.m_to_remove.push_back(idx);
                // Force recomputation of grouping data.
                clusterset.m_group_data = std::nullopt;
                // Do not wipe the oversized state of main if staging exists. The reason for this
                // is that the alternative would mean that cluster merges may need to be applied to
                // a formerly-oversized main graph while staging exists (to satisfy chunk feerate
                // queries into main, for example), and such merges could conflict with pulls of
                // some of their constituents into staging.
                if (level == GetTopLevel() && clusterset.m_oversized == true) {
                    clusterset.m_oversized = std::nullopt;
                }
            }
        }
        m_unlinked.push_back(idx);
        if (!exists_anywhere) Compact();
    }

    // Functions related to various normalization/application steps.
    /** Get rid of unlinked Entry objects in m_entries, if possible (this changes the GraphIndex
     *  values for remaining Entry objects, so this only does something when no to-be-applied
     *  operations or staged removals referring to GraphIndexes remain). */
    void Compact() noexcept;
    /** If cluster is not in staging, copy it there, and return a pointer to it.
    *   Staging must exist, and this modifies the locators of its
    *   transactions from inherited (P,M) to explicit (P,P). */
    Cluster* PullIn(Cluster* cluster, int level) noexcept;
    /** Apply all removals queued up in m_to_remove to the relevant Clusters (which get a
     *  NEEDS_SPLIT* QualityLevel) up to the specified level. */
    void ApplyRemovals(int up_to_level) noexcept;
    /** Split an individual cluster. */
    void Split(Cluster& cluster, int level) noexcept;
    /** Split all clusters that need splitting up to the specified level. */
    void SplitAll(int up_to_level) noexcept;
    /** Populate m_group_data based on m_deps_to_add in the specified level. */
    void GroupClusters(int level) noexcept;
    /** Merge the specified clusters. */
    void Merge(std::span<Cluster*> to_merge, int level) noexcept;
    /** Apply all m_deps_to_add to the relevant Clusters in the specified level. */
    void ApplyDependencies(int level) noexcept;
    /** Make a specified Cluster have quality ACCEPTABLE or OPTIMAL. */
    void MakeAcceptable(Cluster& cluster, int level) noexcept;
    /** Make all Clusters at the specified level have quality ACCEPTABLE or OPTIMAL. */
    void MakeAllAcceptable(int level) noexcept;

    // Implementations for the public TxGraph interface.

    Ref AddTransaction(const FeePerWeight& feerate) noexcept final;
    void RemoveTransaction(const Ref& arg) noexcept final;
    void AddDependency(const Ref& parent, const Ref& child) noexcept final;
    void SetTransactionFee(const Ref&, int64_t fee) noexcept final;

    bool DoWork(uint64_t iters) noexcept final;

    void StartStaging() noexcept final;
    void CommitStaging() noexcept final;
    void AbortStaging() noexcept final;
    bool HaveStaging() const noexcept final { return m_staging_clusterset.has_value(); }

    bool Exists(const Ref& arg, Level level) noexcept final;
    FeePerWeight GetMainChunkFeerate(const Ref& arg) noexcept final;
    FeePerWeight GetIndividualFeerate(const Ref& arg) noexcept final;
    std::vector<Ref*> GetCluster(const Ref& arg, Level level) noexcept final;
    std::vector<Ref*> GetAncestors(const Ref& arg, Level level) noexcept final;
    std::vector<Ref*> GetDescendants(const Ref& arg, Level level) noexcept final;
    std::vector<Ref*> GetAncestorsUnion(std::span<const Ref* const> args, Level level) noexcept final;
    std::vector<Ref*> GetDescendantsUnion(std::span<const Ref* const> args, Level level) noexcept final;
    GraphIndex GetTransactionCount(Level level) noexcept final;
    bool IsOversized(Level level) noexcept final;
    std::strong_ordering CompareMainOrder(const Ref& a, const Ref& b) noexcept final;
    GraphIndex CountDistinctClusters(std::span<const Ref* const> refs, Level level) noexcept final;
    std::pair<std::vector<FeeFrac>, std::vector<FeeFrac>> GetMainStagingDiagrams() noexcept final;
    std::vector<Ref*> Trim() noexcept final;

    std::unique_ptr<BlockBuilder> GetBlockBuilder() noexcept final;
    std::pair<std::vector<Ref*>, FeePerWeight> GetWorstMainChunk() noexcept final;

    size_t GetMainMemoryUsage() noexcept final;

    void SanityCheck() const final;
};

TxGraphImpl::ClusterSet& TxGraphImpl::GetClusterSet(int level) noexcept
{
    if (level == 0) return m_main_clusterset;
    Assume(level == 1);
    Assume(m_staging_clusterset.has_value());
    return *m_staging_clusterset;
}

const TxGraphImpl::ClusterSet& TxGraphImpl::GetClusterSet(int level) const noexcept
{
    if (level == 0) return m_main_clusterset;
    Assume(level == 1);
    Assume(m_staging_clusterset.has_value());
    return *m_staging_clusterset;
}

/** Implementation of the TxGraph::BlockBuilder interface. */
class BlockBuilderImpl final : public TxGraph::BlockBuilder
{
    /** Which TxGraphImpl this object is doing block building for. It will have its
     *  m_main_chunkindex_observers incremented as long as this BlockBuilderImpl exists. */
    TxGraphImpl* const m_graph;
    /** Cluster sequence numbers which we're not including further transactions from. */
    std::unordered_set<uint64_t> m_excluded_clusters;
    /** Iterator to the current chunk in the chunk index. end() if nothing further remains. */
    TxGraphImpl::ChunkIndex::const_iterator m_cur_iter;
    /** Which cluster the current chunk belongs to, so we can exclude further transactions from it
     *  when that chunk is skipped. */
    Cluster* m_cur_cluster;
    /** Whether we know that m_cur_iter points to the last chunk of m_cur_cluster. */
    bool m_known_end_of_cluster;

    // Move m_cur_iter / m_cur_cluster to the next acceptable chunk.
    void Next() noexcept;

public:
    /** Construct a new BlockBuilderImpl to build blocks for the provided graph. */
    BlockBuilderImpl(TxGraphImpl& graph) noexcept;

    // Implement the public interface.
    ~BlockBuilderImpl() final;
    std::optional<std::pair<std::vector<TxGraph::Ref*>, FeePerWeight>> GetCurrentChunk() noexcept final;
    void Include() noexcept final;
    void Skip() noexcept final;
};

void TxGraphImpl::ClearChunkData(Entry& entry) noexcept
{
    if (entry.m_main_chunkindex_iterator != m_main_chunkindex.end()) {
        Assume(m_main_chunkindex_observers == 0);
        // If the Entry has a non-empty m_main_chunkindex_iterator, extract it, and move the handle
        // to the cache of discarded chunkindex entries.
        m_main_chunkindex_discarded.emplace_back(m_main_chunkindex.extract(entry.m_main_chunkindex_iterator));
        entry.m_main_chunkindex_iterator = m_main_chunkindex.end();
    }
}

void TxGraphImpl::CreateChunkData(GraphIndex idx, LinearizationIndex chunk_count) noexcept
{
    auto& entry = m_entries[idx];
    if (!m_main_chunkindex_discarded.empty()) {
        // Reuse an discarded node handle.
        auto& node = m_main_chunkindex_discarded.back().value();
        node.m_graph_index = idx;
        node.m_chunk_count = chunk_count;
        auto insert_result = m_main_chunkindex.insert(std::move(m_main_chunkindex_discarded.back()));
        Assume(insert_result.inserted);
        entry.m_main_chunkindex_iterator = insert_result.position;
        m_main_chunkindex_discarded.pop_back();
    } else {
        // Construct a new entry.
        auto emplace_result = m_main_chunkindex.emplace(idx, chunk_count);
        Assume(emplace_result.second);
        entry.m_main_chunkindex_iterator = emplace_result.first;
    }
}

size_t GenericClusterImpl::TotalMemoryUsage() const noexcept
{
    return // Dynamic memory allocated in this Cluster.
           memusage::DynamicUsage(m_mapping) + memusage::DynamicUsage(m_linearization) +
           // Dynamic memory usage inside m_depgraph.
           m_depgraph.DynamicMemoryUsage() +
           // Memory usage of the allocated Cluster itself.
           memusage::MallocUsage(sizeof(GenericClusterImpl)) +
           // Memory usage of the ClusterSet::m_clusters entry.
           sizeof(std::unique_ptr<Cluster>);
}

size_t SingletonClusterImpl::TotalMemoryUsage() const noexcept
{
    return // Memory usage of the allocated SingletonClusterImpl itself.
           memusage::MallocUsage(sizeof(SingletonClusterImpl)) +
           // Memory usage of the ClusterSet::m_clusters entry.
           sizeof(std::unique_ptr<Cluster>);
}

uint64_t GenericClusterImpl::GetTotalTxSize() const noexcept
{
    uint64_t ret{0};
    for (auto i : m_linearization) {
        ret += m_depgraph.FeeRate(i).size;
    }
    return ret;
}

DepGraphIndex GenericClusterImpl::AppendTransaction(GraphIndex graph_idx, FeePerWeight feerate) noexcept
{
    Assume(graph_idx != GraphIndex(-1));
    auto ret = m_depgraph.AddTransaction(feerate);
    m_mapping.push_back(graph_idx);
    m_linearization.push_back(ret);
    return ret;
}

DepGraphIndex SingletonClusterImpl::AppendTransaction(GraphIndex graph_idx, FeePerWeight feerate) noexcept
{
    Assume(!GetTxCount());
    m_graph_index = graph_idx;
    m_feerate = feerate;
    return 0;
}

void GenericClusterImpl::AddDependencies(SetType parents, DepGraphIndex child) noexcept
{
    m_depgraph.AddDependencies(parents, child);
}

void SingletonClusterImpl::AddDependencies(SetType parents, DepGraphIndex child) noexcept
{
    // Singletons cannot have any dependencies.
    Assume(child == 0);
    Assume(parents == SetType{} || parents == SetType::Fill(0));
}

void GenericClusterImpl::ExtractTransactions(const std::function<void (DepGraphIndex, GraphIndex, FeePerWeight, SetType)>& visit_fn) noexcept
{
    for (auto pos : m_linearization) {
        visit_fn(pos, m_mapping[pos], FeePerWeight::FromFeeFrac(m_depgraph.FeeRate(pos)), m_depgraph.GetReducedParents(pos));
    }
    // Purge this Cluster, now that everything has been moved.
    m_depgraph = DepGraph<SetType>{};
    m_linearization.clear();
    m_mapping.clear();
}

void SingletonClusterImpl::ExtractTransactions(const std::function<void (DepGraphIndex, GraphIndex, FeePerWeight, SetType)>& visit_fn) noexcept
{
    if (GetTxCount()) {
        visit_fn(0, m_graph_index, m_feerate, SetType{});
        m_graph_index = NO_GRAPH_INDEX;
    }
}

int GenericClusterImpl::GetLevel(const TxGraphImpl& graph) const noexcept
{
    // GetLevel() does not work for empty Clusters.
    if (!Assume(!m_linearization.empty())) return -1;

    // Pick an arbitrary Entry that occurs in this Cluster.
    const auto& entry = graph.m_entries[m_mapping[m_linearization.front()]];
    // See if there is a level whose Locator matches this Cluster, if so return that level.
    for (int level = 0; level < MAX_LEVELS; ++level) {
        if (entry.m_locator[level].cluster == this) return level;
    }
    // Given that we started with an Entry that occurs in this Cluster, one of its Locators must
    // point back to it.
    assert(false);
    return -1;
}

int SingletonClusterImpl::GetLevel(const TxGraphImpl& graph) const noexcept
{
    // GetLevel() does not work for empty Clusters.
    if (!Assume(GetTxCount())) return -1;

    // Get the Entry in this Cluster.
    const auto& entry = graph.m_entries[m_graph_index];
    // See if there is a level whose Locator matches this Cluster, if so return that level.
    for (int level = 0; level < MAX_LEVELS; ++level) {
        if (entry.m_locator[level].cluster == this) return level;
    }
    // Given that we started with an Entry that occurs in this Cluster, one of its Locators must
    // point back to it.
    assert(false);
    return -1;
}

void TxGraphImpl::ClearLocator(int level, GraphIndex idx, bool oversized_tx) noexcept
{
    auto& entry = m_entries[idx];
    auto& clusterset = GetClusterSet(level);
    Assume(entry.m_locator[level].IsPresent());
    // Change the locator from Present to Missing or Removed.
    if (level == 0 || !entry.m_locator[level - 1].IsPresent()) {
        entry.m_locator[level].SetMissing();
    } else {
        entry.m_locator[level].SetRemoved();
        clusterset.m_removed.push_back(idx);
    }
    // Update the transaction count.
    --clusterset.m_txcount;
    clusterset.m_txcount_oversized -= oversized_tx;
    // If clearing main, adjust the status of Locators of this transaction in staging, if it exists.
    if (level == 0 && GetTopLevel() == 1) {
        if (entry.m_locator[1].IsRemoved()) {
            entry.m_locator[1].SetMissing();
        } else if (!entry.m_locator[1].IsPresent()) {
            --m_staging_clusterset->m_txcount;
            m_staging_clusterset->m_txcount_oversized -= oversized_tx;
        }
    }
    if (level == 0) ClearChunkData(entry);
}

void GenericClusterImpl::Updated(TxGraphImpl& graph, int level) noexcept
{
    // Update all the Locators for this Cluster's Entry objects.
    for (DepGraphIndex idx : m_linearization) {
        auto& entry = graph.m_entries[m_mapping[idx]];
        // Discard any potential ChunkData prior to modifying the Cluster (as that could
        // invalidate its ordering).
        if (level == 0) graph.ClearChunkData(entry);
        entry.m_locator[level].SetPresent(this, idx);
    }
    // If this is for the main graph (level = 0), and the Cluster's quality is ACCEPTABLE or
    // OPTIMAL, compute its chunking and store its information in the Entry's m_main_lin_index
    // and m_main_chunk_feerate. These fields are only accessed after making the entire graph
    // ACCEPTABLE, so it is pointless to compute these if we haven't reached that quality level
    // yet.
    if (level == 0 && IsAcceptable()) {
        const LinearizationChunking chunking(m_depgraph, m_linearization);
        LinearizationIndex lin_idx{0};
        // Iterate over the chunks.
        for (unsigned chunk_idx = 0; chunk_idx < chunking.NumChunksLeft(); ++chunk_idx) {
            auto chunk = chunking.GetChunk(chunk_idx);
            auto chunk_count = chunk.transactions.Count();
            Assume(chunk_count > 0);
            // Iterate over the transactions in the linearization, which must match those in chunk.
            while (true) {
                DepGraphIndex idx = m_linearization[lin_idx];
                GraphIndex graph_idx = m_mapping[idx];
                auto& entry = graph.m_entries[graph_idx];
                entry.m_main_lin_index = lin_idx++;
                entry.m_main_chunk_feerate = FeePerWeight::FromFeeFrac(chunk.feerate);
                Assume(chunk.transactions[idx]);
                chunk.transactions.Reset(idx);
                if (chunk.transactions.None()) {
                    // Last transaction in the chunk.
                    if (chunk_count == 1 && chunk_idx + 1 == chunking.NumChunksLeft()) {
                        // If this is the final chunk of the cluster, and it contains just a single
                        // transaction (which will always be true for the very common singleton
                        // clusters), store the special value -1 as chunk count.
                        chunk_count = LinearizationIndex(-1);
                    }
                    graph.CreateChunkData(graph_idx, chunk_count);
                    break;
                }
            }
        }
    }
}

void SingletonClusterImpl::Updated(TxGraphImpl& graph, int level) noexcept
{
    // Don't do anything if this is empty.
    if (GetTxCount() == 0) return;

    auto& entry = graph.m_entries[m_graph_index];
    // Discard any potential ChunkData prior to modifying the Cluster (as that could
    // invalidate its ordering).
    if (level == 0) graph.ClearChunkData(entry);
    entry.m_locator[level].SetPresent(this, 0);
    // If this is for the main graph (level = 0), compute its chunking and store its information in
    // the Entry's m_main_lin_index and m_main_chunk_feerate.
    if (level == 0 && IsAcceptable()) {
        entry.m_main_lin_index = 0;
        entry.m_main_chunk_feerate = m_feerate;
        // Always use the special LinearizationIndex(-1), indicating singleton chunk at end of
        // Cluster, here.
        graph.CreateChunkData(m_graph_index, LinearizationIndex(-1));
    }
}

void GenericClusterImpl::GetConflicts(const TxGraphImpl& graph, std::vector<Cluster*>& out) const noexcept
{
    for (auto i : m_linearization) {
        auto& entry = graph.m_entries[m_mapping[i]];
        // For every transaction Entry in this Cluster, if it also exists in a lower-level Cluster,
        // then that Cluster conflicts.
        if (entry.m_locator[0].IsPresent()) {
            out.push_back(entry.m_locator[0].cluster);
        }
    }
}

void SingletonClusterImpl::GetConflicts(const TxGraphImpl& graph, std::vector<Cluster*>& out) const noexcept
{
    // Empty clusters have no conflicts.
    if (GetTxCount() == 0) return;

    auto& entry = graph.m_entries[m_graph_index];
    // If the transaction in this Cluster also exists in a lower-level Cluster, then that Cluster
    // conflicts.
    if (entry.m_locator[0].IsPresent()) {
        out.push_back(entry.m_locator[0].cluster);
    }
}

std::vector<Cluster*> TxGraphImpl::GetConflicts() const noexcept
{
    Assume(GetTopLevel() == 1);
    auto& clusterset = GetClusterSet(1);
    std::vector<Cluster*> ret;
    // All main Clusters containing transactions in m_removed (so (P,R) ones) are conflicts.
    for (auto i : clusterset.m_removed) {
        auto& entry = m_entries[i];
        if (entry.m_locator[0].IsPresent()) {
            ret.push_back(entry.m_locator[0].cluster);
        }
    }
    // Then go over all Clusters at this level, and find their conflicts (the (P,P) ones).
    for (int quality = 0; quality < int(QualityLevel::NONE); ++quality) {
        auto& clusters = clusterset.m_clusters[quality];
        for (const auto& cluster : clusters) {
            cluster->GetConflicts(*this, ret);
        }
    }
    // Deduplicate the result (the same Cluster may appear multiple times).
    std::sort(ret.begin(), ret.end(), [](Cluster* a, Cluster* b) noexcept { return CompareClusters(a, b) < 0; });
    ret.erase(std::unique(ret.begin(), ret.end()), ret.end());
    return ret;
}

Cluster* GenericClusterImpl::CopyToStaging(TxGraphImpl& graph) const noexcept
{
    // Construct an empty Cluster.
    auto ret = graph.CreateEmptyGenericCluster();
    auto ptr = ret.get();
    // Copy depgraph, mapping, and linearization.
    ptr->m_depgraph = m_depgraph;
    ptr->m_mapping = m_mapping;
    ptr->m_linearization = m_linearization;
    // Insert the new Cluster into the graph.
    graph.InsertCluster(/*level=*/1, std::move(ret), m_quality);
    // Update its Locators.
    ptr->Updated(graph, /*level=*/1);
    // Update memory usage.
    graph.GetClusterSet(/*level=*/1).m_cluster_usage += ptr->TotalMemoryUsage();
    return ptr;
}

Cluster* SingletonClusterImpl::CopyToStaging(TxGraphImpl& graph) const noexcept
{
    // Construct an empty Cluster.
    auto ret = graph.CreateEmptySingletonCluster();
    auto ptr = ret.get();
    // Copy data.
    ptr->m_graph_index = m_graph_index;
    ptr->m_feerate = m_feerate;
    // Insert the new Cluster into the graph.
    graph.InsertCluster(/*level=*/1, std::move(ret), m_quality);
    // Update its Locators.
    ptr->Updated(graph, /*level=*/1);
    // Update memory usage.
    graph.GetClusterSet(/*level=*/1).m_cluster_usage += ptr->TotalMemoryUsage();
    return ptr;
}

void GenericClusterImpl::ApplyRemovals(TxGraphImpl& graph, int level, std::span<GraphIndex>& to_remove) noexcept
{
    // Iterate over the prefix of to_remove that applies to this cluster.
    Assume(!to_remove.empty());
    SetType todo;
    graph.GetClusterSet(level).m_cluster_usage -= TotalMemoryUsage();
    do {
        GraphIndex idx = to_remove.front();
        Assume(idx < graph.m_entries.size());
        auto& entry = graph.m_entries[idx];
        auto& locator = entry.m_locator[level];
        // Stop once we hit an entry that applies to another Cluster.
        if (locator.cluster != this) break;
        // - Remember it in a set of to-remove DepGraphIndexes.
        todo.Set(locator.index);
        // - Remove from m_mapping. This isn't strictly necessary as unused positions in m_mapping
        //   are just never accessed, but set it to -1 here to increase the ability to detect a bug
        //   that causes it to be accessed regardless.
        m_mapping[locator.index] = GraphIndex(-1);
        // - Remove its linearization index from the Entry (if in main).
        if (level == 0) {
            entry.m_main_lin_index = LinearizationIndex(-1);
        }
        // - Mark it as missing/removed in the Entry's locator.
        graph.ClearLocator(level, idx, m_quality == QualityLevel::OVERSIZED_SINGLETON);
        to_remove = to_remove.subspan(1);
    } while(!to_remove.empty());

    auto quality = m_quality;
    Assume(todo.Any());
    // Wipe from the Cluster's DepGraph (this is O(n) regardless of the number of entries
    // removed, so we benefit from batching all the removals).
    m_depgraph.RemoveTransactions(todo);
    m_mapping.resize(m_depgraph.PositionRange());

    // First remove all removals at the end of the linearization.
    while (!m_linearization.empty() && todo[m_linearization.back()]) {
        todo.Reset(m_linearization.back());
        m_linearization.pop_back();
    }
    if (todo.None()) {
        // If no further removals remain, and thus all removals were at the end, we may be able
        // to leave the cluster at a better quality level.
        if (IsAcceptable(/*after_split=*/true)) {
            quality = QualityLevel::NEEDS_SPLIT_ACCEPTABLE;
        } else {
            quality = QualityLevel::NEEDS_SPLIT;
        }
    } else {
        // If more removals remain, filter those out of m_linearization.
        m_linearization.erase(std::remove_if(
            m_linearization.begin(),
            m_linearization.end(),
            [&](auto pos) { return todo[pos]; }), m_linearization.end());
        quality = QualityLevel::NEEDS_SPLIT;
    }
    Compact();
    graph.GetClusterSet(level).m_cluster_usage += TotalMemoryUsage();
    graph.SetClusterQuality(level, m_quality, m_setindex, quality);
    Updated(graph, level);
}

void SingletonClusterImpl::ApplyRemovals(TxGraphImpl& graph, int level, std::span<GraphIndex>& to_remove) noexcept
{
    // We can only remove the one transaction this Cluster has.
    Assume(!to_remove.empty());
    Assume(GetTxCount());
    Assume(to_remove.front() == m_graph_index);
    // Pop all copies of m_graph_index from the front of to_remove (at least one, but there may be
    // multiple).
    do {
        to_remove = to_remove.subspan(1);
    } while (!to_remove.empty() && to_remove.front() == m_graph_index);
    // Clear this cluster.
    graph.ClearLocator(level, m_graph_index, m_quality == QualityLevel::OVERSIZED_SINGLETON);
    m_graph_index = NO_GRAPH_INDEX;
    graph.SetClusterQuality(level, m_quality, m_setindex, QualityLevel::NEEDS_SPLIT);
    // No need to account for m_cluster_usage changes here, as SingletonClusterImpl has constant
    // memory usage.
}

void GenericClusterImpl::Clear(TxGraphImpl& graph, int level) noexcept
{
    Assume(GetTxCount());
    graph.GetClusterSet(level).m_cluster_usage -= TotalMemoryUsage();
    for (auto i : m_linearization) {
        graph.ClearLocator(level, m_mapping[i], m_quality == QualityLevel::OVERSIZED_SINGLETON);
    }
    m_depgraph = {};
    m_linearization.clear();
    m_mapping.clear();
}

void SingletonClusterImpl::Clear(TxGraphImpl& graph, int level) noexcept
{
    Assume(GetTxCount());
    graph.GetClusterSet(level).m_cluster_usage -= TotalMemoryUsage();
    graph.ClearLocator(level, m_graph_index, m_quality == QualityLevel::OVERSIZED_SINGLETON);
    m_graph_index = NO_GRAPH_INDEX;
}

void GenericClusterImpl::MoveToMain(TxGraphImpl& graph) noexcept
{
    for (auto i : m_linearization) {
        GraphIndex idx = m_mapping[i];
        auto& entry = graph.m_entries[idx];
        entry.m_locator[1].SetMissing();
    }
    auto quality = m_quality;
    // Subtract memory usage from staging and add it to main.
    graph.GetClusterSet(/*level=*/1).m_cluster_usage -= TotalMemoryUsage();
    graph.GetClusterSet(/*level=*/0).m_cluster_usage += TotalMemoryUsage();
    // Remove cluster itself from staging and add it to main.
    auto cluster = graph.ExtractCluster(1, quality, m_setindex);
    graph.InsertCluster(/*level=*/0, std::move(cluster), quality);
    Updated(graph, /*level=*/0);
}

void SingletonClusterImpl::MoveToMain(TxGraphImpl& graph) noexcept
{
    if (GetTxCount()) {
        auto& entry = graph.m_entries[m_graph_index];
        entry.m_locator[1].SetMissing();
    }
    auto quality = m_quality;
    graph.GetClusterSet(/*level=*/1).m_cluster_usage -= TotalMemoryUsage();
    auto cluster = graph.ExtractCluster(/*level=*/1, quality, m_setindex);
    graph.InsertCluster(/*level=*/0, std::move(cluster), quality);
    graph.GetClusterSet(/*level=*/0).m_cluster_usage += TotalMemoryUsage();
    Updated(graph, /*level=*/0);
}

void GenericClusterImpl::Compact() noexcept
{
    m_linearization.shrink_to_fit();
    m_mapping.shrink_to_fit();
    m_depgraph.Compact();
}

void SingletonClusterImpl::Compact() noexcept
{
    // Nothing to compact; SingletonClusterImpl is constant size.
}

void GenericClusterImpl::AppendChunkFeerates(std::vector<FeeFrac>& ret) const noexcept
{
    auto chunk_feerates = ChunkLinearization(m_depgraph, m_linearization);
    ret.reserve(ret.size() + chunk_feerates.size());
    ret.insert(ret.end(), chunk_feerates.begin(), chunk_feerates.end());
}

void SingletonClusterImpl::AppendChunkFeerates(std::vector<FeeFrac>& ret) const noexcept
{
    if (GetTxCount()) {
        ret.push_back(m_feerate);
    }
}

uint64_t GenericClusterImpl::AppendTrimData(std::vector<TrimTxData>& ret, std::vector<std::pair<GraphIndex, GraphIndex>>& deps) const noexcept
{
    const LinearizationChunking linchunking(m_depgraph, m_linearization);
    LinearizationIndex pos{0};
    uint64_t size{0};
    auto prev_index = GraphIndex(-1);
    // Iterate over the chunks of this cluster's linearization.
    for (unsigned i = 0; i < linchunking.NumChunksLeft(); ++i) {
        const auto& [chunk, chunk_feerate] = linchunking.GetChunk(i);
        // Iterate over the transactions of that chunk, in linearization order.
        auto chunk_tx_count = chunk.Count();
        for (unsigned j = 0; j < chunk_tx_count; ++j) {
            auto cluster_idx = m_linearization[pos];
            // The transaction must appear in the chunk.
            Assume(chunk[cluster_idx]);
            // Construct a new element in ret.
            auto& entry = ret.emplace_back();
            entry.m_chunk_feerate = FeePerWeight::FromFeeFrac(chunk_feerate);
            entry.m_index = m_mapping[cluster_idx];
            // If this is not the first transaction of the cluster linearization, it has an
            // implicit dependency on its predecessor.
            if (pos != 0) deps.emplace_back(prev_index, entry.m_index);
            prev_index = entry.m_index;
            entry.m_tx_size = m_depgraph.FeeRate(cluster_idx).size;
            size += entry.m_tx_size;
            ++pos;
        }
    }
    return size;
}

uint64_t SingletonClusterImpl::AppendTrimData(std::vector<TrimTxData>& ret, std::vector<std::pair<GraphIndex, GraphIndex>>& deps) const noexcept
{
    if (!GetTxCount()) return 0;
    auto& entry = ret.emplace_back();
    entry.m_chunk_feerate = m_feerate;
    entry.m_index = m_graph_index;
    entry.m_tx_size = m_feerate.size;
    return m_feerate.size;
}

bool GenericClusterImpl::Split(TxGraphImpl& graph, int level) noexcept
{
    // This function can only be called when the Cluster needs splitting.
    Assume(NeedsSplitting());
    // Determine the new quality the split-off Clusters will have.
    QualityLevel new_quality = IsAcceptable(/*after_split=*/true) ? QualityLevel::ACCEPTABLE
                                                                  : QualityLevel::NEEDS_RELINEARIZE;
    // If we're going to produce ACCEPTABLE clusters (i.e., when in NEEDS_SPLIT_ACCEPTABLE), we
    // need to post-linearize to make sure the split-out versions are all connected (as
    // connectivity may have changed by removing part of the cluster). This could be done on each
    // resulting split-out cluster separately, but it is simpler to do it once up front before
    // splitting. This step is not necessary if the resulting clusters are NEEDS_RELINEARIZE, as
    // they will be post-linearized anyway in MakeAcceptable().
    if (new_quality == QualityLevel::ACCEPTABLE) {
        PostLinearize(m_depgraph, m_linearization);
    }
    /** Which positions are still left in this Cluster. */
    auto todo = m_depgraph.Positions();
    /** Mapping from transaction positions in this Cluster to the Cluster where it ends up, and
     *  its position therein. */
    std::vector<std::pair<Cluster*, DepGraphIndex>> remap(m_depgraph.PositionRange());
    std::vector<Cluster*> new_clusters;
    bool first{true};
    // Iterate over the connected components of this Cluster's m_depgraph.
    while (todo.Any()) {
        auto component = m_depgraph.FindConnectedComponent(todo);
        auto component_size = component.Count();
        auto split_quality = component_size <= 2 ? QualityLevel::OPTIMAL : new_quality;
        if (first && component == todo && SetType::Fill(component_size) == component && component_size >= MIN_INTENDED_TX_COUNT) {
            // The existing Cluster is an entire component, without holes. Leave it be, but update
            // its quality. If there are holes, we continue, so that the Cluster is reconstructed
            // without holes, reducing memory usage. If the component's size is below the intended
            // transaction count for this Cluster implementation, continue so that it can get
            // converted.
            Assume(todo == m_depgraph.Positions());
            graph.SetClusterQuality(level, m_quality, m_setindex, split_quality);
            // If this made the quality ACCEPTABLE or OPTIMAL, we need to compute and cache its
            // chunking.
            Updated(graph, level);
            return false;
        }
        first = false;
        // Construct a new Cluster to hold the found component.
        auto new_cluster = graph.CreateEmptyCluster(component_size);
        new_clusters.push_back(new_cluster.get());
        // Remember that all the component's transactions go to this new Cluster. The positions
        // will be determined below, so use -1 for now.
        for (auto i : component) {
            remap[i] = {new_cluster.get(), DepGraphIndex(-1)};
        }
        graph.InsertCluster(level, std::move(new_cluster), split_quality);
        todo -= component;
    }
    // We have to split the Cluster up. Remove accounting for the existing one first.
    graph.GetClusterSet(level).m_cluster_usage -= TotalMemoryUsage();
    // Redistribute the transactions.
    for (auto i : m_linearization) {
        /** The cluster which transaction originally in position i is moved to. */
        Cluster* new_cluster = remap[i].first;
        // Copy the transaction to the new cluster's depgraph, and remember the position.
        remap[i].second = new_cluster->AppendTransaction(m_mapping[i], FeePerWeight::FromFeeFrac(m_depgraph.FeeRate(i)));
    }
    // Redistribute the dependencies.
    for (auto i : m_linearization) {
        /** The cluster transaction in position i is moved to. */
        Cluster* new_cluster = remap[i].first;
        // Copy its parents, translating positions.
        SetType new_parents;
        for (auto par : m_depgraph.GetReducedParents(i)) new_parents.Set(remap[par].second);
        new_cluster->AddDependencies(new_parents, remap[i].second);
    }
    // Update all the Locators of moved transactions, and memory usage.
    for (Cluster* new_cluster : new_clusters) {
        new_cluster->Updated(graph, level);
        new_cluster->Compact();
        graph.GetClusterSet(level).m_cluster_usage += new_cluster->TotalMemoryUsage();
    }
    // Wipe this Cluster, and return that it needs to be deleted.
    m_depgraph = DepGraph<SetType>{};
    m_mapping.clear();
    m_linearization.clear();
    return true;
}

bool SingletonClusterImpl::Split(TxGraphImpl& graph, int level) noexcept
{
    Assume(NeedsSplitting());
    Assume(!GetTxCount());
    graph.GetClusterSet(level).m_cluster_usage -= TotalMemoryUsage();
    return true;
}

void GenericClusterImpl::Merge(TxGraphImpl& graph, int level, Cluster& other) noexcept
{
    /** Vector to store the positions in this Cluster for each position in other. */
    std::vector<DepGraphIndex> remap(other.GetDepGraphIndexRange());
    // Iterate over all transactions in the other Cluster (the one being absorbed).
    other.ExtractTransactions([&](DepGraphIndex pos, GraphIndex idx, FeePerWeight feerate, SetType other_parents) noexcept {
        // Copy the transaction into this Cluster, and remember its position.
        auto new_pos = m_depgraph.AddTransaction(feerate);
        // Since this cluster must have been made hole-free before being merged into, all added
        // transactions should appear at the end.
        Assume(new_pos == m_mapping.size());
        remap[pos] = new_pos;
        m_mapping.push_back(idx);
        m_linearization.push_back(new_pos);
        // Copy the transaction's dependencies, translating them using remap. Note that since
        // pos iterates in linearization order, which is topological, all parents of pos should
        // already be in remap.
        SetType parents;
        for (auto par : other_parents) {
            parents.Set(remap[par]);
        }
        m_depgraph.AddDependencies(parents, remap[pos]);
        // Update the transaction's Locator. There is no need to call Updated() to update chunk
        // feerates, as Updated() will be invoked by Cluster::ApplyDependencies on the resulting
        // merged Cluster later anyway.
        auto& entry = graph.m_entries[idx];
        // Discard any potential ChunkData prior to modifying the Cluster (as that could
        // invalidate its ordering).
        if (level == 0) graph.ClearChunkData(entry);
        entry.m_locator[level].SetPresent(this, new_pos);
    });
}

void SingletonClusterImpl::Merge(TxGraphImpl&, int, Cluster&) noexcept
{
    // Nothing can be merged into a singleton; it should have been converted to GenericClusterImpl first.
    Assume(false);
}

void GenericClusterImpl::ApplyDependencies(TxGraphImpl& graph, int level, std::span<std::pair<GraphIndex, GraphIndex>> to_apply) noexcept
{
    // This function is invoked by TxGraphImpl::ApplyDependencies after merging groups of Clusters
    // between which dependencies are added, which simply concatenates their linearizations. Invoke
    // PostLinearize, which has the effect that the linearization becomes a merge-sort of the
    // constituent linearizations. Do this here rather than in Cluster::Merge, because this
    // function is only invoked once per merged Cluster, rather than once per constituent one.
    // This concatenation + post-linearization could be replaced with an explicit merge-sort.
    PostLinearize(m_depgraph, m_linearization);

    // Sort the list of dependencies to apply by child, so those can be applied in batch.
    std::sort(to_apply.begin(), to_apply.end(), [](auto& a, auto& b) { return a.second < b.second; });
    // Iterate over groups of to-be-added dependencies with the same child.
    auto it = to_apply.begin();
    while (it != to_apply.end()) {
        auto& first_child = graph.m_entries[it->second].m_locator[level];
        const auto child_idx = first_child.index;
        // Iterate over all to-be-added dependencies within that same child, gather the relevant
        // parents.
        SetType parents;
        while (it != to_apply.end()) {
            auto& child = graph.m_entries[it->second].m_locator[level];
            auto& parent = graph.m_entries[it->first].m_locator[level];
            Assume(child.cluster == this && parent.cluster == this);
            if (child.index != child_idx) break;
            parents.Set(parent.index);
            ++it;
        }
        // Push all dependencies to the underlying DepGraph. Note that this is O(N) in the size of
        // the cluster, regardless of the number of parents being added, so batching them together
        // has a performance benefit.
        m_depgraph.AddDependencies(parents, child_idx);
    }

    // Finally fix the linearization, as the new dependencies may have invalidated the
    // linearization, and post-linearize it to fix up the worst problems with it.
    FixLinearization(m_depgraph, m_linearization);
    PostLinearize(m_depgraph, m_linearization);
    Assume(!NeedsSplitting());
    Assume(!IsOversized());
    if (IsAcceptable()) {
        graph.SetClusterQuality(level, m_quality, m_setindex, QualityLevel::NEEDS_RELINEARIZE);
    }

    // Finally push the changes to graph.m_entries.
    Updated(graph, level);
}

void SingletonClusterImpl::ApplyDependencies(TxGraphImpl&, int, std::span<std::pair<GraphIndex, GraphIndex>>) noexcept
{
    // Nothing can actually be applied.
    Assume(false);
}

TxGraphImpl::~TxGraphImpl() noexcept
{
    // If Refs outlive the TxGraphImpl they refer to, unlink them, so that their destructor does not
    // try to reach into a non-existing TxGraphImpl anymore.
    for (auto& entry : m_entries) {
        if (entry.m_ref != nullptr) {
            GetRefGraph(*entry.m_ref) = nullptr;
        }
    }
}

std::unique_ptr<Cluster> TxGraphImpl::ExtractCluster(int level, QualityLevel quality, ClusterSetIndex setindex) noexcept
{
    Assume(quality != QualityLevel::NONE);

    auto& clusterset = GetClusterSet(level);
    auto& quality_clusters = clusterset.m_clusters[int(quality)];
    Assume(setindex < quality_clusters.size());

    // Extract the Cluster-owning unique_ptr.
    std::unique_ptr<Cluster> ret = std::move(quality_clusters[setindex]);
    ret->m_quality = QualityLevel::NONE;
    ret->m_setindex = ClusterSetIndex(-1);

    // Clean up space in quality_cluster.
    auto max_setindex = quality_clusters.size() - 1;
    if (setindex != max_setindex) {
        // If the cluster was not the last element of quality_clusters, move that to take its place.
        quality_clusters.back()->m_setindex = setindex;
        quality_clusters[setindex] = std::move(quality_clusters.back());
    }
    // The last element of quality_clusters is now unused; drop it.
    quality_clusters.pop_back();

    return ret;
}

ClusterSetIndex TxGraphImpl::InsertCluster(int level, std::unique_ptr<Cluster>&& cluster, QualityLevel quality) noexcept
{
    // Cannot insert with quality level NONE (as that would mean not inserted).
    Assume(quality != QualityLevel::NONE);
    // The passed-in Cluster must not currently be in the TxGraphImpl.
    Assume(cluster->m_quality == QualityLevel::NONE);

    // Append it at the end of the relevant TxGraphImpl::m_cluster.
    auto& clusterset = GetClusterSet(level);
    auto& quality_clusters = clusterset.m_clusters[int(quality)];
    ClusterSetIndex ret = quality_clusters.size();
    cluster->m_quality = quality;
    cluster->m_setindex = ret;
    quality_clusters.push_back(std::move(cluster));
    return ret;
}

void TxGraphImpl::SetClusterQuality(int level, QualityLevel old_quality, ClusterSetIndex old_index, QualityLevel new_quality) noexcept
{
    Assume(new_quality != QualityLevel::NONE);

    // Don't do anything if the quality did not change.
    if (old_quality == new_quality) return;
    // Extract the cluster from where it currently resides.
    auto cluster_ptr = ExtractCluster(level, old_quality, old_index);
    // And re-insert it where it belongs.
    InsertCluster(level, std::move(cluster_ptr), new_quality);
}

void TxGraphImpl::DeleteCluster(Cluster& cluster, int level) noexcept
{
    // Extract the cluster from where it currently resides.
    auto cluster_ptr = ExtractCluster(level, cluster.m_quality, cluster.m_setindex);
    // And throw it away.
    cluster_ptr.reset();
}

std::pair<Cluster*, int> TxGraphImpl::FindClusterAndLevel(GraphIndex idx, int level) const noexcept
{
    Assume(level >= 0 && level <= GetTopLevel());
    auto& entry = m_entries[idx];
    // Search the entry's locators from top to bottom.
    for (int l = level; l >= 0; --l) {
        // If the locator is missing, dig deeper; it may exist at a lower level and therefore be
        // implicitly existing at this level too.
        if (entry.m_locator[l].IsMissing()) continue;
        // If the locator has the entry marked as explicitly removed, stop.
        if (entry.m_locator[l].IsRemoved()) break;
        // Otherwise, we have found the topmost ClusterSet that contains this entry.
        return {entry.m_locator[l].cluster, l};
    }
    // If no non-empty locator was found, or an explicitly removed was hit, return nothing.
    return {nullptr, -1};
}

Cluster* TxGraphImpl::PullIn(Cluster* cluster, int level) noexcept
{
    int to_level = GetTopLevel();
    Assume(to_level == 1);
    Assume(level <= to_level);
    // Copy the Cluster from main to staging, if it's not already there.
    if (level == 0) {
        // Make the Cluster Acceptable before copying. This isn't strictly necessary, but doing it
        // now avoids doing double work later.
        MakeAcceptable(*cluster, level);
        cluster = cluster->CopyToStaging(*this);
    }
    return cluster;
}

void TxGraphImpl::ApplyRemovals(int up_to_level) noexcept
{
    Assume(up_to_level >= 0 && up_to_level <= GetTopLevel());
    for (int level = 0; level <= up_to_level; ++level) {
        auto& clusterset = GetClusterSet(level);
        auto& to_remove = clusterset.m_to_remove;
        // Skip if there is nothing to remove in this level.
        if (to_remove.empty()) continue;
        // Pull in all Clusters that are not in staging.
        if (level == 1) {
            for (GraphIndex index : to_remove) {
                auto [cluster, cluster_level] = FindClusterAndLevel(index, level);
                if (cluster != nullptr) PullIn(cluster, cluster_level);
            }
        }
        // Group the set of to-be-removed entries by Cluster::m_sequence.
        std::sort(to_remove.begin(), to_remove.end(), [&](GraphIndex a, GraphIndex b) noexcept {
            Cluster* cluster_a = m_entries[a].m_locator[level].cluster;
            Cluster* cluster_b = m_entries[b].m_locator[level].cluster;
            return CompareClusters(cluster_a, cluster_b) < 0;
        });
        // Process per Cluster.
        std::span to_remove_span{to_remove};
        while (!to_remove_span.empty()) {
            Cluster* cluster = m_entries[to_remove_span.front()].m_locator[level].cluster;
            if (cluster != nullptr) {
                // If the first to_remove_span entry's Cluster exists, hand to_remove_span to it, so it
                // can pop off whatever applies to it.
                cluster->ApplyRemovals(*this, level, to_remove_span);
            } else {
                // Otherwise, skip this already-removed entry. This may happen when
                // RemoveTransaction was called twice on the same Ref, for example.
                to_remove_span = to_remove_span.subspan(1);
            }
        }
        to_remove.clear();
    }
    Compact();
}

void TxGraphImpl::SwapIndexes(GraphIndex a, GraphIndex b) noexcept
{
    Assume(a < m_entries.size());
    Assume(b < m_entries.size());
    // Swap the Entry objects.
    std::swap(m_entries[a], m_entries[b]);
    // Iterate over both objects.
    for (int i = 0; i < 2; ++i) {
        GraphIndex idx = i ? b : a;
        Entry& entry = m_entries[idx];
        // Update linked Ref, if any exists.
        if (entry.m_ref) GetRefIndex(*entry.m_ref) = idx;
        // Update linked chunk index entries, if any exist.
        if (entry.m_main_chunkindex_iterator != m_main_chunkindex.end()) {
            entry.m_main_chunkindex_iterator->m_graph_index = idx;
        }
        // Update the locators for both levels. The rest of the Entry information will not change,
        // so no need to invoke Cluster::Updated().
        for (int level = 0; level < MAX_LEVELS; ++level) {
            Locator& locator = entry.m_locator[level];
            if (locator.IsPresent()) {
                locator.cluster->UpdateMapping(locator.index, idx);
            }
        }
    }
}

void TxGraphImpl::Compact() noexcept
{
    // We cannot compact while any to-be-applied operations or staged removals remain as we'd need
    // to rewrite them. It is easier to delay the compaction until they have been applied.
    if (!m_main_clusterset.m_deps_to_add.empty()) return;
    if (!m_main_clusterset.m_to_remove.empty()) return;
    Assume(m_main_clusterset.m_removed.empty()); // non-staging m_removed is always empty
    if (m_staging_clusterset.has_value()) {
        if (!m_staging_clusterset->m_deps_to_add.empty()) return;
        if (!m_staging_clusterset->m_to_remove.empty()) return;
        if (!m_staging_clusterset->m_removed.empty()) return;
    }

    // Release memory used by discarded ChunkData index entries.
    ClearShrink(m_main_chunkindex_discarded);

    // Sort the GraphIndexes that need to be cleaned up. They are sorted in reverse, so the last
    // ones get processed first. This means earlier-processed GraphIndexes will not cause moving of
    // later-processed ones during the "swap with end of m_entries" step below (which might
    // invalidate them).
    std::sort(m_unlinked.begin(), m_unlinked.end(), std::greater{});

    auto last = GraphIndex(-1);
    for (GraphIndex idx : m_unlinked) {
        // m_unlinked should never contain the same GraphIndex twice (the code below would fail
        // if so, because GraphIndexes get invalidated by removing them).
        Assume(idx != last);
        last = idx;

        // Make sure the entry is unlinked.
        Entry& entry = m_entries[idx];
        Assume(entry.m_ref == nullptr);
        // Make sure the entry does not occur in the graph.
        for (int level = 0; level < MAX_LEVELS; ++level) {
            Assume(!entry.m_locator[level].IsPresent());
        }

        // Move the entry to the end.
        if (idx != m_entries.size() - 1) SwapIndexes(idx, m_entries.size() - 1);
        // Drop the entry for idx, now that it is at the end.
        m_entries.pop_back();
    }
    m_unlinked.clear();
}

void TxGraphImpl::Split(Cluster& cluster, int level) noexcept
{
    // To split a Cluster, first make sure all removals are applied (as we might need to split
    // again afterwards otherwise).
    ApplyRemovals(level);
    bool del = cluster.Split(*this, level);
    if (del) {
        // Cluster::Split reports whether the Cluster is to be deleted.
        DeleteCluster(cluster, level);
    }
}

void TxGraphImpl::SplitAll(int up_to_level) noexcept
{
    Assume(up_to_level >= 0 && up_to_level <= GetTopLevel());
    // Before splitting all Cluster, first make sure all removals are applied.
    ApplyRemovals(up_to_level);
    for (int level = 0; level <= up_to_level; ++level) {
        for (auto quality : {QualityLevel::NEEDS_SPLIT, QualityLevel::NEEDS_SPLIT_ACCEPTABLE}) {
            auto& queue = GetClusterSet(level).m_clusters[int(quality)];
            while (!queue.empty()) {
                Split(*queue.back().get(), level);
            }
        }
    }
}

void TxGraphImpl::GroupClusters(int level) noexcept
{
    auto& clusterset = GetClusterSet(level);
    // If the groupings have been computed already, nothing is left to be done.
    if (clusterset.m_group_data.has_value()) return;

    // Before computing which Clusters need to be merged together, first apply all removals and
    // split the Clusters into connected components. If we would group first, we might end up
    // with inefficient and/or oversized Clusters which just end up being split again anyway.
    SplitAll(level);

    /** Annotated clusters: an entry for each Cluster, together with the sequence number for the
     *  representative for the partition it is in (initially its own, later that of the
     *  to-be-merged group). */
    std::vector<std::pair<Cluster*, uint64_t>> an_clusters;
    /** Annotated dependencies: an entry for each m_deps_to_add entry (excluding ones that apply
     *  to removed transactions), together with the sequence number of the representative root of
     *  Clusters it applies to (initially that of the child Cluster, later that of the
     *  to-be-merged group). */
    std::vector<std::pair<std::pair<GraphIndex, GraphIndex>, uint64_t>> an_deps;

    // Construct an an_clusters entry for every oversized cluster, including ones from levels below,
    // as they may be inherited in this one.
    for (int level_iter = 0; level_iter <= level; ++level_iter) {
        for (auto& cluster : GetClusterSet(level_iter).m_clusters[int(QualityLevel::OVERSIZED_SINGLETON)]) {
            auto graph_idx = cluster->GetClusterEntry(0);
            auto cur_cluster = FindCluster(graph_idx, level);
            if (cur_cluster == nullptr) continue;
            an_clusters.emplace_back(cur_cluster, cur_cluster->m_sequence);
        }
    }

    // Construct a an_clusters entry for every parent and child in the to-be-applied dependencies,
    // and an an_deps entry for each dependency to be applied.
    an_deps.reserve(clusterset.m_deps_to_add.size());
    for (const auto& [par, chl] : clusterset.m_deps_to_add) {
        auto par_cluster = FindCluster(par, level);
        auto chl_cluster = FindCluster(chl, level);
        // Skip dependencies for which the parent or child transaction is removed.
        if (par_cluster == nullptr || chl_cluster == nullptr) continue;
        an_clusters.emplace_back(par_cluster, par_cluster->m_sequence);
        // Do not include a duplicate when parent and child are identical, as it'll be removed
        // below anyway.
        if (chl_cluster != par_cluster) an_clusters.emplace_back(chl_cluster, chl_cluster->m_sequence);
        // Add entry to an_deps, using the child sequence number.
        an_deps.emplace_back(std::pair{par, chl}, chl_cluster->m_sequence);
    }
    // Sort and deduplicate an_clusters, so we end up with a sorted list of all involved Clusters
    // to which dependencies apply, or which are oversized.
    std::sort(an_clusters.begin(), an_clusters.end(), [](auto& a, auto& b) noexcept { return a.second < b.second; });
    an_clusters.erase(std::unique(an_clusters.begin(), an_clusters.end()), an_clusters.end());
    // Sort an_deps by applying the same order to the involved child cluster.
    std::sort(an_deps.begin(), an_deps.end(), [&](auto& a, auto& b) noexcept { return a.second < b.second; });

    // Run the union-find algorithm to to find partitions of the input Clusters which need to be
    // grouped together. See https://en.wikipedia.org/wiki/Disjoint-set_data_structure.
    {
        /** Each PartitionData entry contains information about a single input Cluster. */
        struct PartitionData
        {
            /** The sequence number of the cluster this holds information for. */
            uint64_t sequence;
            /** All PartitionData entries belonging to the same partition are organized in a tree.
             *  Each element points to its parent, or to itself if it is the root. The root is then
             *  a representative for the entire tree, and can be found by walking upwards from any
             *  element. */
            PartitionData* parent;
            /** (only if this is a root, so when parent == this) An upper bound on the height of
             *  tree for this partition. */
            unsigned rank;
        };
        /** Information about each input Cluster. Sorted by Cluster::m_sequence. */
        std::vector<PartitionData> partition_data;

        /** Given a Cluster, find its corresponding PartitionData. */
        auto locate_fn = [&](uint64_t sequence) noexcept -> PartitionData* {
            auto it = std::lower_bound(partition_data.begin(), partition_data.end(), sequence,
                                       [](auto& a, uint64_t seq) noexcept { return a.sequence < seq; });
            Assume(it != partition_data.end());
            Assume(it->sequence == sequence);
            return &*it;
        };

        /** Given a PartitionData, find the root of the tree it is in (its representative). */
        static constexpr auto find_root_fn = [](PartitionData* data) noexcept -> PartitionData* {
            while (data->parent != data) {
                // Replace pointers to parents with pointers to grandparents.
                // See https://en.wikipedia.org/wiki/Disjoint-set_data_structure#Finding_set_representatives.
                auto par = data->parent;
                data->parent = par->parent;
                data = par;
            }
            return data;
        };

        /** Given two PartitionDatas, union the partitions they are in, and return their
         *  representative. */
        static constexpr auto union_fn = [](PartitionData* arg1, PartitionData* arg2) noexcept {
            // Find the roots of the trees, and bail out if they are already equal (which would
            // mean they are in the same partition already).
            auto rep1 = find_root_fn(arg1);
            auto rep2 = find_root_fn(arg2);
            if (rep1 == rep2) return rep1;
            // Pick the lower-rank root to become a child of the higher-rank one.
            // See https://en.wikipedia.org/wiki/Disjoint-set_data_structure#Union_by_rank.
            if (rep1->rank < rep2->rank) std::swap(rep1, rep2);
            rep2->parent = rep1;
            rep1->rank += (rep1->rank == rep2->rank);
            return rep1;
        };

        // Start by initializing every Cluster as its own singleton partition.
        partition_data.resize(an_clusters.size());
        for (size_t i = 0; i < an_clusters.size(); ++i) {
            partition_data[i].sequence = an_clusters[i].first->m_sequence;
            partition_data[i].parent = &partition_data[i];
            partition_data[i].rank = 0;
        }

        // Run through all parent/child pairs in an_deps, and union the partitions their Clusters
        // are in.
        Cluster* last_chl_cluster{nullptr};
        PartitionData* last_partition{nullptr};
        for (const auto& [dep, _] : an_deps) {
            auto [par, chl] = dep;
            auto par_cluster = FindCluster(par, level);
            auto chl_cluster = FindCluster(chl, level);
            Assume(chl_cluster != nullptr && par_cluster != nullptr);
            // Nothing to do if parent and child are in the same Cluster.
            if (par_cluster == chl_cluster) continue;
            Assume(par != chl);
            if (chl_cluster == last_chl_cluster) {
                // If the child Clusters is the same as the previous iteration, union with the
                // tree they were in, avoiding the need for another lookup. Note that an_deps
                // is sorted by child Cluster, so batches with the same child are expected.
                last_partition = union_fn(locate_fn(par_cluster->m_sequence), last_partition);
            } else {
                last_chl_cluster = chl_cluster;
                last_partition = union_fn(locate_fn(par_cluster->m_sequence), locate_fn(chl_cluster->m_sequence));
            }
        }

        // Update the sequence numbers in an_clusters and an_deps to be those of the partition
        // representative.
        auto deps_it = an_deps.begin();
        for (size_t i = 0; i < partition_data.size(); ++i) {
            auto& data = partition_data[i];
            // Find the sequence of the representative of the partition Cluster i is in, and store
            // it with the Cluster.
            auto rep_seq = find_root_fn(&data)->sequence;
            an_clusters[i].second = rep_seq;
            // Find all dependencies whose child Cluster is Cluster i, and annotate them with rep.
            while (deps_it != an_deps.end()) {
                auto [par, chl] = deps_it->first;
                auto chl_cluster = FindCluster(chl, level);
                Assume(chl_cluster != nullptr);
                if (chl_cluster->m_sequence > data.sequence) break;
                deps_it->second = rep_seq;
                ++deps_it;
            }
        }
    }

    // Sort both an_clusters and an_deps by sequence number of the representative of the
    // partition they are in, grouping all those applying to the same partition together.
    std::sort(an_deps.begin(), an_deps.end(), [](auto& a, auto& b) noexcept { return a.second < b.second; });
    std::sort(an_clusters.begin(), an_clusters.end(), [](auto& a, auto& b) noexcept { return a.second < b.second; });

    // Translate the resulting cluster groups to the m_group_data structure, and the dependencies
    // back to m_deps_to_add.
    clusterset.m_group_data = GroupData{};
    clusterset.m_group_data->m_group_clusters.reserve(an_clusters.size());
    clusterset.m_deps_to_add.clear();
    clusterset.m_deps_to_add.reserve(an_deps.size());
    clusterset.m_oversized = false;
    auto an_deps_it = an_deps.begin();
    auto an_clusters_it = an_clusters.begin();
    while (an_clusters_it != an_clusters.end()) {
        // Process all clusters/dependencies belonging to the partition with representative rep.
        auto rep = an_clusters_it->second;
        // Create and initialize a new GroupData entry for the partition.
        auto& new_entry = clusterset.m_group_data->m_groups.emplace_back();
        new_entry.m_cluster_offset = clusterset.m_group_data->m_group_clusters.size();
        new_entry.m_cluster_count = 0;
        new_entry.m_deps_offset = clusterset.m_deps_to_add.size();
        new_entry.m_deps_count = 0;
        uint32_t total_count{0};
        uint64_t total_size{0};
        // Add all its clusters to it (copying those from an_clusters to m_group_clusters).
        while (an_clusters_it != an_clusters.end() && an_clusters_it->second == rep) {
            clusterset.m_group_data->m_group_clusters.push_back(an_clusters_it->first);
            total_count += an_clusters_it->first->GetTxCount();
            total_size += an_clusters_it->first->GetTotalTxSize();
            ++an_clusters_it;
            ++new_entry.m_cluster_count;
        }
        // Add all its dependencies to it (copying those back from an_deps to m_deps_to_add).
        while (an_deps_it != an_deps.end() && an_deps_it->second == rep) {
            clusterset.m_deps_to_add.push_back(an_deps_it->first);
            ++an_deps_it;
            ++new_entry.m_deps_count;
        }
        // Detect oversizedness.
        if (total_count > m_max_cluster_count || total_size > m_max_cluster_size) {
            clusterset.m_oversized = true;
        }
    }
    Assume(an_deps_it == an_deps.end());
    Assume(an_clusters_it == an_clusters.end());
    Compact();
}

void TxGraphImpl::Merge(std::span<Cluster*> to_merge, int level) noexcept
{
    Assume(!to_merge.empty());
    // Nothing to do if a group consists of just a single Cluster.
    if (to_merge.size() == 1) return;

    // Move the largest Cluster to the front of to_merge. As all transactions in other to-be-merged
    // Clusters will be moved to that one, putting the largest one first minimizes the number of
    // moves.
    size_t max_size_pos{0};
    DepGraphIndex max_size = to_merge[max_size_pos]->GetTxCount();
    GetClusterSet(level).m_cluster_usage -= to_merge[max_size_pos]->TotalMemoryUsage();
    DepGraphIndex total_size = max_size;
    for (size_t i = 1; i < to_merge.size(); ++i) {
        GetClusterSet(level).m_cluster_usage -= to_merge[i]->TotalMemoryUsage();
        DepGraphIndex size = to_merge[i]->GetTxCount();
        total_size += size;
        if (size > max_size) {
            max_size_pos = i;
            max_size = size;
        }
    }
    if (max_size_pos != 0) std::swap(to_merge[0], to_merge[max_size_pos]);

    size_t start_idx = 1;
    Cluster* into_cluster = to_merge[0];
    if (total_size > into_cluster->GetMaxTxCount()) {
        // The into_merge cluster is too small to fit all transactions being merged. Construct a
        // a new Cluster using an implementation that matches the total size, and merge everything
        // in there.
        auto new_cluster = CreateEmptyCluster(total_size);
        into_cluster = new_cluster.get();
        InsertCluster(level, std::move(new_cluster), QualityLevel::OPTIMAL);
        start_idx = 0;
    }

    // Merge all further Clusters in the group into the result (first one, or new one), and delete
    // them.
    for (size_t i = start_idx; i < to_merge.size(); ++i) {
        into_cluster->Merge(*this, level, *to_merge[i]);
        DeleteCluster(*to_merge[i], level);
    }
    into_cluster->Compact();
    GetClusterSet(level).m_cluster_usage += into_cluster->TotalMemoryUsage();
}

void TxGraphImpl::ApplyDependencies(int level) noexcept
{
    auto& clusterset = GetClusterSet(level);
    // Do not bother computing groups if we already know the result will be oversized.
    if (clusterset.m_oversized == true) return;
    // Compute the groups of to-be-merged Clusters (which also applies all removals, and splits).
    GroupClusters(level);
    Assume(clusterset.m_group_data.has_value());
    // Nothing to do if there are no dependencies to be added.
    if (clusterset.m_deps_to_add.empty()) return;
    // Dependencies cannot be applied if it would result in oversized clusters.
    if (clusterset.m_oversized == true) return;

    // For each group of to-be-merged Clusters.
    for (const auto& group_entry : clusterset.m_group_data->m_groups) {
        auto cluster_span = std::span{clusterset.m_group_data->m_group_clusters}
                                .subspan(group_entry.m_cluster_offset, group_entry.m_cluster_count);
        // Pull in all the Clusters that contain dependencies.
        if (level == 1) {
            for (Cluster*& cluster : cluster_span) {
                cluster = PullIn(cluster, cluster->GetLevel(*this));
            }
        }
        // Invoke Merge() to merge them into a single Cluster.
        Merge(cluster_span, level);
        // Actually apply all to-be-added dependencies (all parents and children from this grouping
        // belong to the same Cluster at this point because of the merging above).
        auto deps_span = std::span{clusterset.m_deps_to_add}
                             .subspan(group_entry.m_deps_offset, group_entry.m_deps_count);
        Assume(!deps_span.empty());
        const auto& loc = m_entries[deps_span[0].second].m_locator[level];
        Assume(loc.IsPresent());
        loc.cluster->ApplyDependencies(*this, level, deps_span);
    }

    // Wipe the list of to-be-added dependencies now that they are applied.
    clusterset.m_deps_to_add.clear();
    Compact();
    // Also no further Cluster mergings are needed (note that we clear, but don't set to
    // std::nullopt, as that would imply the groupings are unknown).
    clusterset.m_group_data = GroupData{};
}

std::pair<uint64_t, bool> GenericClusterImpl::Relinearize(TxGraphImpl& graph, int level, uint64_t max_iters) noexcept
{
    // We can only relinearize Clusters that do not need splitting.
    Assume(!NeedsSplitting());
    // No work is required for Clusters which are already optimally linearized.
    if (IsOptimal()) return {0, false};
    // Invoke the actual linearization algorithm (passing in the existing one).
    uint64_t rng_seed = graph.m_rng.rand64();
    auto [linearization, optimal, cost] = Linearize(m_depgraph, max_iters, rng_seed, m_linearization);
    // Postlinearize if the result isn't optimal already. This guarantees (among other things)
    // that the chunks of the resulting linearization are all connected.
    if (!optimal) PostLinearize(m_depgraph, linearization);
    // Update the linearization.
    m_linearization = std::move(linearization);
    // Update the Cluster's quality.
    bool improved = false;
    if (optimal) {
        graph.SetClusterQuality(level, m_quality, m_setindex, QualityLevel::OPTIMAL);
        improved = true;
    } else if (max_iters >= graph.m_acceptable_iters && !IsAcceptable()) {
        graph.SetClusterQuality(level, m_quality, m_setindex, QualityLevel::ACCEPTABLE);
        improved = true;
    }
    // Update the Entry objects.
    Updated(graph, level);
    return {cost, improved};
}

std::pair<uint64_t, bool> SingletonClusterImpl::Relinearize(TxGraphImpl& graph, int level, uint64_t max_iters) noexcept
{
    // All singletons are optimal, oversized, or need splitting. Each of these precludes
    // Relinearize from being called.
    assert(false);
    return {0, false};
}

void TxGraphImpl::MakeAcceptable(Cluster& cluster, int level) noexcept
{
    // Relinearize the Cluster if needed.
    if (!cluster.NeedsSplitting() && !cluster.IsAcceptable() && !cluster.IsOversized()) {
        cluster.Relinearize(*this, level, m_acceptable_iters);
    }
}

void TxGraphImpl::MakeAllAcceptable(int level) noexcept
{
    ApplyDependencies(level);
    auto& clusterset = GetClusterSet(level);
    if (clusterset.m_oversized == true) return;
    auto& queue = clusterset.m_clusters[int(QualityLevel::NEEDS_RELINEARIZE)];
    while (!queue.empty()) {
        MakeAcceptable(*queue.back().get(), level);
    }
}

GenericClusterImpl::GenericClusterImpl(uint64_t sequence) noexcept : Cluster{sequence} {}

TxGraph::Ref TxGraphImpl::AddTransaction(const FeePerWeight& feerate) noexcept
{
    Assume(m_main_chunkindex_observers == 0 || GetTopLevel() != 0);
    Assume(feerate.size > 0);
    // Construct a new Ref.
    Ref ret;
    // Construct a new Entry, and link it with the Ref.
    auto idx = m_entries.size();
    m_entries.emplace_back();
    auto& entry = m_entries.back();
    entry.m_main_chunkindex_iterator = m_main_chunkindex.end();
    entry.m_ref = &ret;
    GetRefGraph(ret) = this;
    GetRefIndex(ret) = idx;
    // Construct a new singleton Cluster (which is necessarily optimally linearized).
    bool oversized = uint64_t(feerate.size) > m_max_cluster_size;
    auto cluster = CreateEmptyCluster(1);
    cluster->AppendTransaction(idx, feerate);
    auto cluster_ptr = cluster.get();
    int level = GetTopLevel();
    auto& clusterset = GetClusterSet(level);
    InsertCluster(level, std::move(cluster), oversized ? QualityLevel::OVERSIZED_SINGLETON : QualityLevel::OPTIMAL);
    cluster_ptr->Updated(*this, level);
    clusterset.m_cluster_usage += cluster_ptr->TotalMemoryUsage();
    ++clusterset.m_txcount;
    // Deal with individually oversized transactions.
    if (oversized) {
        ++clusterset.m_txcount_oversized;
        clusterset.m_oversized = true;
        clusterset.m_group_data = std::nullopt;
    }
    // Return the Ref.
    return ret;
}

void TxGraphImpl::RemoveTransaction(const Ref& arg) noexcept
{
    // Don't do anything if the Ref is empty (which may be indicative of the transaction already
    // having been removed).
    if (GetRefGraph(arg) == nullptr) return;
    Assume(GetRefGraph(arg) == this);
    Assume(m_main_chunkindex_observers == 0 || GetTopLevel() != 0);
    // Find the Cluster the transaction is in, and stop if it isn't in any.
    int level = GetTopLevel();
    auto cluster = FindCluster(GetRefIndex(arg), level);
    if (cluster == nullptr) return;
    // Remember that the transaction is to be removed.
    auto& clusterset = GetClusterSet(level);
    clusterset.m_to_remove.push_back(GetRefIndex(arg));
    // Wipe m_group_data (as it will need to be recomputed).
    clusterset.m_group_data.reset();
    if (clusterset.m_oversized == true) clusterset.m_oversized = std::nullopt;
}

void TxGraphImpl::AddDependency(const Ref& parent, const Ref& child) noexcept
{
    // Don't do anything if either Ref is empty (which may be indicative of it having already been
    // removed).
    if (GetRefGraph(parent) == nullptr || GetRefGraph(child) == nullptr) return;
    Assume(GetRefGraph(parent) == this && GetRefGraph(child) == this);
    Assume(m_main_chunkindex_observers == 0 || GetTopLevel() != 0);
    // Don't do anything if this is a dependency on self.
    if (GetRefIndex(parent) == GetRefIndex(child)) return;
    // Find the Cluster the parent and child transaction are in, and stop if either appears to be
    // already removed.
    int level = GetTopLevel();
    auto par_cluster = FindCluster(GetRefIndex(parent), level);
    if (par_cluster == nullptr) return;
    auto chl_cluster = FindCluster(GetRefIndex(child), level);
    if (chl_cluster == nullptr) return;
    // Remember that this dependency is to be applied.
    auto& clusterset = GetClusterSet(level);
    clusterset.m_deps_to_add.emplace_back(GetRefIndex(parent), GetRefIndex(child));
    // Wipe m_group_data (as it will need to be recomputed).
    clusterset.m_group_data.reset();
    if (clusterset.m_oversized == false) clusterset.m_oversized = std::nullopt;
}

bool TxGraphImpl::Exists(const Ref& arg, Level level_select) noexcept
{
    if (GetRefGraph(arg) == nullptr) return false;
    Assume(GetRefGraph(arg) == this);
    size_t level = GetSpecifiedLevel(level_select);
    // Make sure the transaction isn't scheduled for removal.
    ApplyRemovals(level);
    auto cluster = FindCluster(GetRefIndex(arg), level);
    return cluster != nullptr;
}

void GenericClusterImpl::GetAncestorRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept
{
    /** The union of all ancestors to be returned. */
    SetType ancestors_union;
    // Process elements from the front of args, as long as they apply.
    while (!args.empty()) {
        if (args.front().first != this) break;
        ancestors_union |= m_depgraph.Ancestors(args.front().second);
        args = args.subspan(1);
    }
    Assume(ancestors_union.Any());
    // Translate all ancestors (in arbitrary order) to Refs (if they have any), and return them.
    for (auto idx : ancestors_union) {
        const auto& entry = graph.m_entries[m_mapping[idx]];
        Assume(entry.m_ref != nullptr);
        output.push_back(entry.m_ref);
    }
}

void SingletonClusterImpl::GetAncestorRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept
{
    Assume(GetTxCount());
    while (!args.empty()) {
        if (args.front().first != this) break;
        args = args.subspan(1);
    }
    const auto& entry = graph.m_entries[m_graph_index];
    Assume(entry.m_ref != nullptr);
    output.push_back(entry.m_ref);
}

void GenericClusterImpl::GetDescendantRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept
{
    /** The union of all descendants to be returned. */
    SetType descendants_union;
    // Process elements from the front of args, as long as they apply.
    while (!args.empty()) {
        if (args.front().first != this) break;
        descendants_union |= m_depgraph.Descendants(args.front().second);
        args = args.subspan(1);
    }
    Assume(descendants_union.Any());
    // Translate all descendants (in arbitrary order) to Refs (if they have any), and return them.
    for (auto idx : descendants_union) {
        const auto& entry = graph.m_entries[m_mapping[idx]];
        Assume(entry.m_ref != nullptr);
        output.push_back(entry.m_ref);
    }
}

void SingletonClusterImpl::GetDescendantRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept
{
    // In a singleton cluster, the ancestors or descendants are always just the entire cluster.
    GetAncestorRefs(graph, args, output);
}

bool GenericClusterImpl::GetClusterRefs(TxGraphImpl& graph, std::span<TxGraph::Ref*> range, LinearizationIndex start_pos) noexcept
{
    // Translate the transactions in the Cluster (in linearization order, starting at start_pos in
    // the linearization) to Refs, and fill them in range.
    for (auto& ref : range) {
        Assume(start_pos < m_linearization.size());
        const auto& entry = graph.m_entries[m_mapping[m_linearization[start_pos++]]];
        Assume(entry.m_ref != nullptr);
        ref = entry.m_ref;
    }
    // Return whether start_pos has advanced to the end of the Cluster.
    return start_pos == m_linearization.size();
}

bool SingletonClusterImpl::GetClusterRefs(TxGraphImpl& graph, std::span<TxGraph::Ref*> range, LinearizationIndex start_pos) noexcept
{
    Assume(!range.empty());
    Assume(GetTxCount());
    Assume(start_pos == 0);
    const auto& entry = graph.m_entries[m_graph_index];
    Assume(entry.m_ref != nullptr);
    range[0] = entry.m_ref;
    return true;
}

FeePerWeight GenericClusterImpl::GetIndividualFeerate(DepGraphIndex idx) noexcept
{
    return FeePerWeight::FromFeeFrac(m_depgraph.FeeRate(idx));
}

FeePerWeight SingletonClusterImpl::GetIndividualFeerate(DepGraphIndex idx) noexcept
{
    Assume(GetTxCount());
    Assume(idx == 0);
    return m_feerate;
}

void GenericClusterImpl::MakeStagingTransactionsMissing(TxGraphImpl& graph) noexcept
{
    // Mark all transactions of a Cluster missing, needed when aborting staging, so that the
    // corresponding Locators don't retain references into aborted Clusters.
    for (auto ci : m_linearization) {
        GraphIndex idx = m_mapping[ci];
        auto& entry = graph.m_entries[idx];
        entry.m_locator[1].SetMissing();
    }
}

void SingletonClusterImpl::MakeStagingTransactionsMissing(TxGraphImpl& graph) noexcept
{
    if (GetTxCount()) {
        auto& entry = graph.m_entries[m_graph_index];
        entry.m_locator[1].SetMissing();
    }
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetAncestors(const Ref& arg, Level level_select) noexcept
{
    // Return the empty vector if the Ref is empty.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all removals and dependencies, as the result might be incorrect otherwise.
    size_t level = GetSpecifiedLevel(level_select);
    ApplyDependencies(level);
    // Ancestry cannot be known if unapplied dependencies remain.
    Assume(GetClusterSet(level).m_deps_to_add.empty());
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto [cluster, cluster_level] = FindClusterAndLevel(GetRefIndex(arg), level);
    if (cluster == nullptr) return {};
    // Dispatch to the Cluster.
    std::pair<Cluster*, DepGraphIndex> match = {cluster, m_entries[GetRefIndex(arg)].m_locator[cluster_level].index};
    auto matches = std::span(&match, 1);
    std::vector<TxGraph::Ref*> ret;
    cluster->GetAncestorRefs(*this, matches, ret);
    return ret;
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetDescendants(const Ref& arg, Level level_select) noexcept
{
    // Return the empty vector if the Ref is empty.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all removals and dependencies, as the result might be incorrect otherwise.
    size_t level = GetSpecifiedLevel(level_select);
    ApplyDependencies(level);
    // Ancestry cannot be known if unapplied dependencies remain.
    Assume(GetClusterSet(level).m_deps_to_add.empty());
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto [cluster, cluster_level] = FindClusterAndLevel(GetRefIndex(arg), level);
    if (cluster == nullptr) return {};
    // Dispatch to the Cluster.
    std::pair<Cluster*, DepGraphIndex> match = {cluster, m_entries[GetRefIndex(arg)].m_locator[cluster_level].index};
    auto matches = std::span(&match, 1);
    std::vector<TxGraph::Ref*> ret;
    cluster->GetDescendantRefs(*this, matches, ret);
    return ret;
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetAncestorsUnion(std::span<const Ref* const> args, Level level_select) noexcept
{
    // Apply all dependencies, as the result might be incorrect otherwise.
    size_t level = GetSpecifiedLevel(level_select);
    ApplyDependencies(level);
    // Ancestry cannot be known if unapplied dependencies remain.
    Assume(GetClusterSet(level).m_deps_to_add.empty());

    // Translate args to matches.
    std::vector<std::pair<Cluster*, DepGraphIndex>> matches;
    matches.reserve(args.size());
    for (auto arg : args) {
        Assume(arg);
        // Skip empty Refs.
        if (GetRefGraph(*arg) == nullptr) continue;
        Assume(GetRefGraph(*arg) == this);
        // Find the Cluster the argument is in, and skip if none is found.
        auto [cluster, cluster_level] = FindClusterAndLevel(GetRefIndex(*arg), level);
        if (cluster == nullptr) continue;
        // Append to matches.
        matches.emplace_back(cluster, m_entries[GetRefIndex(*arg)].m_locator[cluster_level].index);
    }
    // Group by Cluster.
    std::sort(matches.begin(), matches.end(), [](auto& a, auto& b) noexcept { return CompareClusters(a.first, b.first) < 0; });
    // Dispatch to the Clusters.
    std::span match_span(matches);
    std::vector<TxGraph::Ref*> ret;
    while (!match_span.empty()) {
        match_span.front().first->GetAncestorRefs(*this, match_span, ret);
    }
    return ret;
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetDescendantsUnion(std::span<const Ref* const> args, Level level_select) noexcept
{
    // Apply all dependencies, as the result might be incorrect otherwise.
    size_t level = GetSpecifiedLevel(level_select);
    ApplyDependencies(level);
    // Ancestry cannot be known if unapplied dependencies remain.
    Assume(GetClusterSet(level).m_deps_to_add.empty());

    // Translate args to matches.
    std::vector<std::pair<Cluster*, DepGraphIndex>> matches;
    matches.reserve(args.size());
    for (auto arg : args) {
        Assume(arg);
        // Skip empty Refs.
        if (GetRefGraph(*arg) == nullptr) continue;
        Assume(GetRefGraph(*arg) == this);
        // Find the Cluster the argument is in, and skip if none is found.
        auto [cluster, cluster_level] = FindClusterAndLevel(GetRefIndex(*arg), level);
        if (cluster == nullptr) continue;
        // Append to matches.
        matches.emplace_back(cluster, m_entries[GetRefIndex(*arg)].m_locator[cluster_level].index);
    }
    // Group by Cluster.
    std::sort(matches.begin(), matches.end(), [](auto& a, auto& b) noexcept { return CompareClusters(a.first, b.first) < 0; });
    // Dispatch to the Clusters.
    std::span match_span(matches);
    std::vector<TxGraph::Ref*> ret;
    while (!match_span.empty()) {
        match_span.front().first->GetDescendantRefs(*this, match_span, ret);
    }
    return ret;
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetCluster(const Ref& arg, Level level_select) noexcept
{
    // Return the empty vector if the Ref is empty (which may be indicative of the transaction
    // having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all removals and dependencies, as the result might be incorrect otherwise.
    size_t level = GetSpecifiedLevel(level_select);
    ApplyDependencies(level);
    // Cluster linearization cannot be known if unapplied dependencies remain.
    Assume(GetClusterSet(level).m_deps_to_add.empty());
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto [cluster, cluster_level] = FindClusterAndLevel(GetRefIndex(arg), level);
    if (cluster == nullptr) return {};
    // Make sure the Cluster has an acceptable quality level, and then dispatch to it.
    MakeAcceptable(*cluster, cluster_level);
    std::vector<TxGraph::Ref*> ret(cluster->GetTxCount());
    cluster->GetClusterRefs(*this, ret, 0);
    return ret;
}

TxGraph::GraphIndex TxGraphImpl::GetTransactionCount(Level level_select) noexcept
{
    size_t level = GetSpecifiedLevel(level_select);
    ApplyRemovals(level);
    return GetClusterSet(level).m_txcount;
}

FeePerWeight TxGraphImpl::GetIndividualFeerate(const Ref& arg) noexcept
{
    // Return the empty FeePerWeight if the passed Ref is empty.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Find the cluster the argument is in (the level does not matter as individual feerates will
    // be identical if it occurs in both), and return the empty FeePerWeight if it isn't in any.
    Cluster* cluster{nullptr};
    int level;
    for (level = 0; level <= GetTopLevel(); ++level) {
        // Apply removals, so that we can correctly report FeePerWeight{} for non-existing
        // transactions.
        ApplyRemovals(level);
        if (m_entries[GetRefIndex(arg)].m_locator[level].IsPresent()) {
            cluster = m_entries[GetRefIndex(arg)].m_locator[level].cluster;
            break;
        }
    }
    if (cluster == nullptr) return {};
    // Dispatch to the Cluster.
    return cluster->GetIndividualFeerate(m_entries[GetRefIndex(arg)].m_locator[level].index);
}

FeePerWeight TxGraphImpl::GetMainChunkFeerate(const Ref& arg) noexcept
{
    // Return the empty FeePerWeight if the passed Ref is empty.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all removals and dependencies, as the result might be inaccurate otherwise.
    ApplyDependencies(/*level=*/0);
    // Chunk feerates cannot be accurately known if unapplied dependencies remain.
    Assume(m_main_clusterset.m_deps_to_add.empty());
    // Find the cluster the argument is in, and return the empty FeePerWeight if it isn't in any.
    auto [cluster, cluster_level] = FindClusterAndLevel(GetRefIndex(arg), /*level=*/0);
    if (cluster == nullptr) return {};
    // Make sure the Cluster has an acceptable quality level, and then return the transaction's
    // chunk feerate.
    MakeAcceptable(*cluster, cluster_level);
    const auto& entry = m_entries[GetRefIndex(arg)];
    return entry.m_main_chunk_feerate;
}

bool TxGraphImpl::IsOversized(Level level_select) noexcept
{
    size_t level = GetSpecifiedLevel(level_select);
    auto& clusterset = GetClusterSet(level);
    if (clusterset.m_oversized.has_value()) {
        // Return cached value if known.
        return *clusterset.m_oversized;
    }
    ApplyRemovals(level);
    if (clusterset.m_txcount_oversized > 0) {
        clusterset.m_oversized = true;
    } else {
        // Find which Clusters will need to be merged together, as that is where the oversize
        // property is assessed.
        GroupClusters(level);
    }
    Assume(clusterset.m_oversized.has_value());
    return *clusterset.m_oversized;
}

void TxGraphImpl::StartStaging() noexcept
{
    // Staging cannot already exist.
    Assume(!m_staging_clusterset.has_value());
    // Apply all remaining dependencies in main before creating a staging graph. Once staging
    // exists, we cannot merge Clusters anymore (because of interference with Clusters being
    // pulled into staging), so to make sure all inspectors are available (if not oversized), do
    // all merging work now. Call SplitAll() first, so that even if ApplyDependencies does not do
    // any thing due to knowing the result is oversized, splitting is still performed.
    SplitAll(0);
    ApplyDependencies(0);
    // Construct the staging ClusterSet.
    m_staging_clusterset.emplace();
    // Copy statistics, precomputed data, and to-be-applied dependencies (only if oversized) to
    // the new graph. To-be-applied removals will always be empty at this point.
    m_staging_clusterset->m_txcount = m_main_clusterset.m_txcount;
    m_staging_clusterset->m_txcount_oversized = m_main_clusterset.m_txcount_oversized;
    m_staging_clusterset->m_deps_to_add = m_main_clusterset.m_deps_to_add;
    m_staging_clusterset->m_group_data = m_main_clusterset.m_group_data;
    m_staging_clusterset->m_oversized = m_main_clusterset.m_oversized;
    Assume(m_staging_clusterset->m_oversized.has_value());
    m_staging_clusterset->m_cluster_usage = 0;
}

void TxGraphImpl::AbortStaging() noexcept
{
    // Staging must exist.
    Assume(m_staging_clusterset.has_value());
    // Mark all removed transactions as Missing (so the staging locator for these transactions
    // can be reused if another staging is created).
    for (auto idx : m_staging_clusterset->m_removed) {
        m_entries[idx].m_locator[1].SetMissing();
    }
    // Do the same with the non-removed transactions in staging Clusters.
    for (int quality = 0; quality < int(QualityLevel::NONE); ++quality) {
        for (auto& cluster : m_staging_clusterset->m_clusters[quality]) {
            cluster->MakeStagingTransactionsMissing(*this);
        }
    }
    // Destroy the staging ClusterSet.
    m_staging_clusterset.reset();
    Compact();
    if (!m_main_clusterset.m_group_data.has_value()) {
        // In case m_oversized in main was kept after a Ref destruction while staging exists, we
        // need to re-evaluate m_oversized now.
        if (m_main_clusterset.m_to_remove.empty() && m_main_clusterset.m_txcount_oversized > 0) {
            // It is possible that a Ref destruction caused a removal in main while staging existed.
            // In this case, m_txcount_oversized may be inaccurate.
            m_main_clusterset.m_oversized = true;
        } else {
            m_main_clusterset.m_oversized = std::nullopt;
        }
    }
}

void TxGraphImpl::CommitStaging() noexcept
{
    // Staging must exist.
    Assume(m_staging_clusterset.has_value());
    Assume(m_main_chunkindex_observers == 0);
    // Delete all conflicting Clusters in main, to make place for moving the staging ones
    // there. All of these have been copied to staging in PullIn().
    auto conflicts = GetConflicts();
    for (Cluster* conflict : conflicts) {
        conflict->Clear(*this, /*level=*/0);
        DeleteCluster(*conflict, /*level=*/0);
    }
    // Mark the removed transactions as Missing (so the staging locator for these transactions
    // can be reused if another staging is created).
    for (auto idx : m_staging_clusterset->m_removed) {
        m_entries[idx].m_locator[1].SetMissing();
    }
    // Then move all Clusters in staging to main.
    for (int quality = 0; quality < int(QualityLevel::NONE); ++quality) {
        auto& stage_sets = m_staging_clusterset->m_clusters[quality];
        while (!stage_sets.empty()) {
            stage_sets.back()->MoveToMain(*this);
        }
    }
    // Move all statistics, precomputed data, and to-be-applied removals and dependencies.
    m_main_clusterset.m_deps_to_add = std::move(m_staging_clusterset->m_deps_to_add);
    m_main_clusterset.m_to_remove = std::move(m_staging_clusterset->m_to_remove);
    m_main_clusterset.m_group_data = std::move(m_staging_clusterset->m_group_data);
    m_main_clusterset.m_oversized = std::move(m_staging_clusterset->m_oversized);
    m_main_clusterset.m_txcount = std::move(m_staging_clusterset->m_txcount);
    m_main_clusterset.m_txcount_oversized = std::move(m_staging_clusterset->m_txcount_oversized);
    // Delete the old staging graph, after all its information was moved to main.
    m_staging_clusterset.reset();
    Compact();
}

void GenericClusterImpl::SetFee(TxGraphImpl& graph, int level, DepGraphIndex idx, int64_t fee) noexcept
{
    // Make sure the specified DepGraphIndex exists in this Cluster.
    Assume(m_depgraph.Positions()[idx]);
    // Bail out if the fee isn't actually being changed.
    if (m_depgraph.FeeRate(idx).fee == fee) return;
    // Update the fee, remember that relinearization will be necessary, and update the Entries
    // in the same Cluster.
    m_depgraph.FeeRate(idx).fee = fee;
    if (m_quality == QualityLevel::OVERSIZED_SINGLETON) {
        // Nothing to do.
    } else if (!NeedsSplitting()) {
        graph.SetClusterQuality(level, m_quality, m_setindex, QualityLevel::NEEDS_RELINEARIZE);
    } else {
        graph.SetClusterQuality(level, m_quality, m_setindex, QualityLevel::NEEDS_SPLIT);
    }
    Updated(graph, level);
}

void SingletonClusterImpl::SetFee(TxGraphImpl& graph, int level, DepGraphIndex idx, int64_t fee) noexcept
{
    Assume(GetTxCount());
    Assume(idx == 0);
    m_feerate.fee = fee;
    Updated(graph, level);
}

void TxGraphImpl::SetTransactionFee(const Ref& ref, int64_t fee) noexcept
{
    // Don't do anything if the passed Ref is empty.
    if (GetRefGraph(ref) == nullptr) return;
    Assume(GetRefGraph(ref) == this);
    Assume(m_main_chunkindex_observers == 0);
    // Find the entry, its locator, and inform its Cluster about the new feerate, if any.
    auto& entry = m_entries[GetRefIndex(ref)];
    for (int level = 0; level < MAX_LEVELS; ++level) {
        auto& locator = entry.m_locator[level];
        if (locator.IsPresent()) {
            locator.cluster->SetFee(*this, level, locator.index, fee);
        }
    }
}

std::strong_ordering TxGraphImpl::CompareMainOrder(const Ref& a, const Ref& b) noexcept
{
    // The references must not be empty.
    Assume(GetRefGraph(a) == this);
    Assume(GetRefGraph(b) == this);
    // Apply dependencies in main.
    ApplyDependencies(0);
    Assume(m_main_clusterset.m_deps_to_add.empty());
    // Make both involved Clusters acceptable, so chunk feerates are relevant.
    const auto& entry_a = m_entries[GetRefIndex(a)];
    const auto& entry_b = m_entries[GetRefIndex(b)];
    const auto& locator_a = entry_a.m_locator[0];
    const auto& locator_b = entry_b.m_locator[0];
    Assume(locator_a.IsPresent());
    Assume(locator_b.IsPresent());
    MakeAcceptable(*locator_a.cluster, /*level=*/0);
    MakeAcceptable(*locator_b.cluster, /*level=*/0);
    // Invoke comparison logic.
    return CompareMainTransactions(GetRefIndex(a), GetRefIndex(b));
}

TxGraph::GraphIndex TxGraphImpl::CountDistinctClusters(std::span<const Ref* const> refs, Level level_select) noexcept
{
    size_t level = GetSpecifiedLevel(level_select);
    ApplyDependencies(level);
    auto& clusterset = GetClusterSet(level);
    Assume(clusterset.m_deps_to_add.empty());
    // Build a vector of Clusters that the specified Refs occur in.
    std::vector<Cluster*> clusters;
    clusters.reserve(refs.size());
    for (const Ref* ref : refs) {
        Assume(ref);
        if (GetRefGraph(*ref) == nullptr) continue;
        Assume(GetRefGraph(*ref) == this);
        auto cluster = FindCluster(GetRefIndex(*ref), level);
        if (cluster != nullptr) clusters.push_back(cluster);
    }
    // Count the number of distinct elements in clusters.
    std::sort(clusters.begin(), clusters.end(), [](Cluster* a, Cluster* b) noexcept { return CompareClusters(a, b) < 0; });
    Cluster* last{nullptr};
    GraphIndex ret{0};
    for (Cluster* cluster : clusters) {
        ret += (cluster != last);
        last = cluster;
    }
    return ret;
}

std::pair<std::vector<FeeFrac>, std::vector<FeeFrac>> TxGraphImpl::GetMainStagingDiagrams() noexcept
{
    Assume(m_staging_clusterset.has_value());
    MakeAllAcceptable(0);
    Assume(m_main_clusterset.m_deps_to_add.empty()); // can only fail if main is oversized
    MakeAllAcceptable(1);
    Assume(m_staging_clusterset->m_deps_to_add.empty()); // can only fail if staging is oversized
    // For all Clusters in main which conflict with Clusters in staging (i.e., all that are removed
    // by, or replaced in, staging), gather their chunk feerates.
    auto main_clusters = GetConflicts();
    std::vector<FeeFrac> main_feerates, staging_feerates;
    for (Cluster* cluster : main_clusters) {
        cluster->AppendChunkFeerates(main_feerates);
    }
    // Do the same for the Clusters in staging themselves.
    for (int quality = 0; quality < int(QualityLevel::NONE); ++quality) {
        for (const auto& cluster : m_staging_clusterset->m_clusters[quality]) {
            cluster->AppendChunkFeerates(staging_feerates);
        }
    }
    // Sort both by decreasing feerate to obtain diagrams, and return them.
    std::sort(main_feerates.begin(), main_feerates.end(), [](auto& a, auto& b) { return a > b; });
    std::sort(staging_feerates.begin(), staging_feerates.end(), [](auto& a, auto& b) { return a > b; });
    return std::make_pair(std::move(main_feerates), std::move(staging_feerates));
}

void GenericClusterImpl::SanityCheck(const TxGraphImpl& graph, int level) const
{
    // There must be an m_mapping for each m_depgraph position (including holes).
    assert(m_depgraph.PositionRange() == m_mapping.size());
    // The linearization for this Cluster must contain every transaction once.
    assert(m_depgraph.TxCount() == m_linearization.size());
    // Unless a split is to be applied, the cluster cannot have any holes.
    if (!NeedsSplitting()) {
        assert(m_depgraph.Positions() == SetType::Fill(m_depgraph.TxCount()));
    }

    // Compute the chunking of m_linearization.
    LinearizationChunking linchunking(m_depgraph, m_linearization);

    // Verify m_linearization.
    SetType m_done;
    LinearizationIndex linindex{0};
    DepGraphIndex chunk_pos{0}; //!< position within the current chunk
    assert(m_depgraph.IsAcyclic());
    for (auto lin_pos : m_linearization) {
        assert(lin_pos < m_mapping.size());
        const auto& entry = graph.m_entries[m_mapping[lin_pos]];
        // Check that the linearization is topological.
        m_done.Set(lin_pos);
        assert(m_done.IsSupersetOf(m_depgraph.Ancestors(lin_pos)));
        // Check that the Entry has a locator pointing back to this Cluster & position within it.
        assert(entry.m_locator[level].cluster == this);
        assert(entry.m_locator[level].index == lin_pos);
        // For main-level entries, check linearization position and chunk feerate.
        if (level == 0 && IsAcceptable()) {
            assert(entry.m_main_lin_index == linindex);
            ++linindex;
            if (!linchunking.GetChunk(0).transactions[lin_pos]) {
                linchunking.MarkDone(linchunking.GetChunk(0).transactions);
                chunk_pos = 0;
            }
            assert(entry.m_main_chunk_feerate == linchunking.GetChunk(0).feerate);
            // Verify that an entry in the chunk index exists for every chunk-ending transaction.
            ++chunk_pos;
            bool is_chunk_end = (chunk_pos == linchunking.GetChunk(0).transactions.Count());
            assert((entry.m_main_chunkindex_iterator != graph.m_main_chunkindex.end()) == is_chunk_end);
            if (is_chunk_end) {
                auto& chunk_data = *entry.m_main_chunkindex_iterator;
                if (m_done == m_depgraph.Positions() && chunk_pos == 1) {
                    assert(chunk_data.m_chunk_count == LinearizationIndex(-1));
                } else {
                    assert(chunk_data.m_chunk_count == chunk_pos);
                }
            }
            // If this Cluster has an acceptable quality level, its chunks must be connected.
            assert(m_depgraph.IsConnected(linchunking.GetChunk(0).transactions));
        }
    }
    // Verify that each element of m_depgraph occurred in m_linearization.
    assert(m_done == m_depgraph.Positions());
}

void SingletonClusterImpl::SanityCheck(const TxGraphImpl& graph, int level) const
{
    // All singletons are optimal, oversized, or need splitting.
    Assume(IsOptimal() || IsOversized() || NeedsSplitting());
    if (GetTxCount()) {
        const auto& entry = graph.m_entries[m_graph_index];
        // Check that the Entry has a locator pointing back to this Cluster & position within it.
        assert(entry.m_locator[level].cluster == this);
        assert(entry.m_locator[level].index == 0);
        // For main-level entries, check linearization position and chunk feerate.
        if (level == 0 && IsAcceptable()) {
            assert(entry.m_main_lin_index == 0);
            assert(entry.m_main_chunk_feerate == m_feerate);
            assert(entry.m_main_chunkindex_iterator != graph.m_main_chunkindex.end());
            auto& chunk_data = *entry.m_main_chunkindex_iterator;
            assert(chunk_data.m_chunk_count == LinearizationIndex(-1));
        }
    }
}

void TxGraphImpl::SanityCheck() const
{
    /** Which GraphIndexes ought to occur in m_unlinked, based on m_entries. */
    std::set<GraphIndex> expected_unlinked;
    /** Which Clusters ought to occur in ClusterSet::m_clusters, based on m_entries. */
    std::set<const Cluster*> expected_clusters[MAX_LEVELS];
    /** Which GraphIndexes ought to occur in ClusterSet::m_removed, based on m_entries. */
    std::set<GraphIndex> expected_removed[MAX_LEVELS];
    /** Which Cluster::m_sequence values have been encountered. */
    std::set<uint64_t> sequences;
    /** Which GraphIndexes ought to occur in m_main_chunkindex, based on m_entries. */
    std::set<GraphIndex> expected_chunkindex;
    /** Whether compaction is possible in the current state. */
    bool compact_possible{true};

    // Go over all Entry objects in m_entries.
    for (GraphIndex idx = 0; idx < m_entries.size(); ++idx) {
        const auto& entry = m_entries[idx];
        if (entry.m_ref == nullptr) {
            // Unlinked Entry must have indexes appear in m_unlinked.
            expected_unlinked.insert(idx);
        } else {
            // Every non-unlinked Entry must have a Ref that points back to it.
            assert(GetRefGraph(*entry.m_ref) == this);
            assert(GetRefIndex(*entry.m_ref) == idx);
        }
        if (entry.m_main_chunkindex_iterator != m_main_chunkindex.end()) {
            // Remember which entries we see a chunkindex entry for.
            assert(entry.m_locator[0].IsPresent());
            expected_chunkindex.insert(idx);
        }
        // Verify the Entry m_locators.
        bool was_present{false}, was_removed{false};
        for (int level = 0; level < MAX_LEVELS; ++level) {
            const auto& locator = entry.m_locator[level];
            // Every Locator must be in exactly one of these 3 states.
            assert(locator.IsMissing() + locator.IsRemoved() + locator.IsPresent() == 1);
            if (locator.IsPresent()) {
                // Once removed, a transaction cannot be revived.
                assert(!was_removed);
                // Verify that the Cluster agrees with where the Locator claims the transaction is.
                assert(locator.cluster->GetClusterEntry(locator.index) == idx);
                // Remember that we expect said Cluster to appear in the ClusterSet::m_clusters.
                expected_clusters[level].insert(locator.cluster);
                was_present = true;
            } else if (locator.IsRemoved()) {
                // Level 0 (main) cannot have IsRemoved locators (IsMissing there means non-existing).
                assert(level > 0);
                // A Locator can only be IsRemoved if it was IsPresent before, and only once.
                assert(was_present && !was_removed);
                // Remember that we expect this GraphIndex to occur in the ClusterSet::m_removed.
                expected_removed[level].insert(idx);
                was_removed = true;
            }
        }
    }

    // For all levels (0 = main, 1 = staged)...
    for (int level = 0; level <= GetTopLevel(); ++level) {
        assert(level < MAX_LEVELS);
        auto& clusterset = GetClusterSet(level);
        std::set<const Cluster*> actual_clusters;
        size_t recomputed_cluster_usage{0};

        // For all quality levels...
        for (int qual = 0; qual < int(QualityLevel::NONE); ++qual) {
            QualityLevel quality{qual};
            const auto& quality_clusters = clusterset.m_clusters[qual];
            // ... for all clusters in them ...
            for (ClusterSetIndex setindex = 0; setindex < quality_clusters.size(); ++setindex) {
                const auto& cluster = *quality_clusters[setindex];
                // The number of transactions in a Cluster cannot exceed m_max_cluster_count.
                assert(cluster.GetTxCount() <= m_max_cluster_count);
                // The level must match the Cluster's own idea of what level it is in (but GetLevel
                // can only be called for non-empty Clusters).
                assert(cluster.GetTxCount() == 0 || level == cluster.GetLevel(*this));
                // The sum of their sizes cannot exceed m_max_cluster_size, unless it is an
                // individually oversized transaction singleton. Note that groups of to-be-merged
                // clusters which would exceed this limit are marked oversized, which means they
                // are never applied.
                assert(cluster.IsOversized() || cluster.GetTotalTxSize() <= m_max_cluster_size);
                // OVERSIZED clusters are singletons.
                assert(!cluster.IsOversized() || cluster.GetTxCount() == 1);
                // Transaction counts cannot exceed the Cluster implementation's maximum
                // supported transactions count.
                assert(cluster.GetTxCount() <= cluster.GetMaxTxCount());
                // Unless a Split is yet to be applied, the number of transactions must not be
                // below the Cluster implementation's intended transaction count.
                if (!cluster.NeedsSplitting()) {
                    assert(cluster.GetTxCount() >= cluster.GetMinIntendedTxCount());
                }

                // Check the sequence number.
                assert(cluster.m_sequence < m_next_sequence_counter);
                assert(sequences.count(cluster.m_sequence) == 0);
                sequences.insert(cluster.m_sequence);
                // Remember we saw this Cluster (only if it is non-empty; empty Clusters aren't
                // expected to be referenced by the Entry vector).
                if (cluster.GetTxCount() != 0) {
                    actual_clusters.insert(&cluster);
                }
                // Sanity check the cluster, according to the Cluster's internal rules.
                cluster.SanityCheck(*this, level);
                // Check that the cluster's quality and setindex matches its position in the quality list.
                assert(cluster.m_quality == quality);
                assert(cluster.m_setindex == setindex);
                // Count memory usage.
                recomputed_cluster_usage += cluster.TotalMemoryUsage();
            }
        }

        // Verify memory usage.
        assert(clusterset.m_cluster_usage == recomputed_cluster_usage);

        // Verify that all to-be-removed transactions have valid identifiers.
        for (GraphIndex idx : clusterset.m_to_remove) {
            assert(idx < m_entries.size());
            // We cannot assert that all m_to_remove transactions are still present: ~Ref on a
            // (P,M) transaction (present in main, inherited in staging) will cause an m_to_remove
            // addition in both main and staging, but a subsequence ApplyRemovals in main will
            // cause it to disappear from staging too, leaving the m_to_remove in place.
        }

        // Verify that all to-be-added dependencies have valid identifiers.
        for (auto [par_idx, chl_idx] : clusterset.m_deps_to_add) {
            assert(par_idx != chl_idx);
            assert(par_idx < m_entries.size());
            assert(chl_idx < m_entries.size());
        }

        // Verify that the actually encountered clusters match the ones occurring in Entry vector.
        assert(actual_clusters == expected_clusters[level]);

        // Verify that the contents of m_removed matches what was expected based on the Entry vector.
        std::set<GraphIndex> actual_removed(clusterset.m_removed.begin(), clusterset.m_removed.end());
        for (auto i : expected_unlinked) {
            // If a transaction exists in both main and staging, and is removed from staging (adding
            // it to m_removed there), and consequently destroyed (wiping the locator completely),
            // it can remain in m_removed despite not having an IsRemoved() locator. Exclude those
            // transactions from the comparison here.
            actual_removed.erase(i);
            expected_removed[level].erase(i);
        }

        assert(actual_removed == expected_removed[level]);

        // If any GraphIndex entries remain in this ClusterSet, compact is not possible.
        if (!clusterset.m_deps_to_add.empty()) compact_possible = false;
        if (!clusterset.m_to_remove.empty()) compact_possible = false;
        if (!clusterset.m_removed.empty()) compact_possible = false;

        // For non-top levels, m_oversized must be known (as it cannot change until the level
        // on top is gone).
        if (level < GetTopLevel()) assert(clusterset.m_oversized.has_value());
    }

    // Verify that the contents of m_unlinked matches what was expected based on the Entry vector.
    std::set<GraphIndex> actual_unlinked(m_unlinked.begin(), m_unlinked.end());
    assert(actual_unlinked == expected_unlinked);

    // If compaction was possible, it should have been performed already, and m_unlinked must be
    // empty (to prevent memory leaks due to an ever-growing m_entries vector).
    if (compact_possible) {
        assert(actual_unlinked.empty());
    }

    // Finally, check the chunk index.
    std::set<GraphIndex> actual_chunkindex;
    FeeFrac last_chunk_feerate;
    for (const auto& chunk : m_main_chunkindex) {
        GraphIndex idx = chunk.m_graph_index;
        actual_chunkindex.insert(idx);
        auto chunk_feerate = m_entries[idx].m_main_chunk_feerate;
        if (!last_chunk_feerate.IsEmpty()) {
            assert(FeeRateCompare(last_chunk_feerate, chunk_feerate) >= 0);
        }
        last_chunk_feerate = chunk_feerate;
    }
    assert(actual_chunkindex == expected_chunkindex);
}

bool TxGraphImpl::DoWork(uint64_t iters) noexcept
{
    uint64_t iters_done{0};
    // First linearize everything in NEEDS_RELINEARIZE to an acceptable level. If more budget
    // remains after that, try to make everything optimal.
    for (QualityLevel quality : {QualityLevel::NEEDS_RELINEARIZE, QualityLevel::ACCEPTABLE}) {
        // First linearize staging, if it exists, then main.
        for (int level = GetTopLevel(); level >= 0; --level) {
            // Do not modify main if it has any observers.
            if (level == 0 && m_main_chunkindex_observers != 0) continue;
            ApplyDependencies(level);
            auto& clusterset = GetClusterSet(level);
            // Do not modify oversized levels.
            if (clusterset.m_oversized == true) continue;
            auto& queue = clusterset.m_clusters[int(quality)];
            while (!queue.empty()) {
                if (iters_done >= iters) return false;
                // Randomize the order in which we process, so that if the first cluster somehow
                // needs more work than what iters allows, we don't keep spending it on the same
                // one.
                auto pos = m_rng.randrange<size_t>(queue.size());
                auto iters_now = iters - iters_done;
                if (quality == QualityLevel::NEEDS_RELINEARIZE) {
                    // If we're working with clusters that need relinearization still, only perform
                    // up to m_acceptable_iters iterations. If they become ACCEPTABLE, and we still
                    // have budget after all other clusters are ACCEPTABLE too, we'll spend the
                    // remaining budget on trying to make them OPTIMAL.
                    iters_now = std::min(iters_now, m_acceptable_iters);
                }
                auto [cost, improved] = queue[pos].get()->Relinearize(*this, level, iters_now);
                iters_done += cost;
                // If no improvement was made to the Cluster, it means we've essentially run out of
                // budget. Even though it may be the case that iters_done < iters still, the
                // linearizer decided there wasn't enough budget left to attempt anything with.
                // To avoid an infinite loop that keeps trying clusters with minuscule budgets,
                // stop here too.
                if (!improved) return false;
            }
        }
    }
    // All possible work has been performed, so we can return true. Note that this does *not* mean
    // that all clusters are optimally linearized now. It may be that there is nothing to do left
    // because all non-optimal clusters are in oversized and/or observer-bearing levels.
    return true;
}

void BlockBuilderImpl::Next() noexcept
{
    // Don't do anything if we're already done.
    if (m_cur_iter == m_graph->m_main_chunkindex.end()) return;
    while (true) {
        // Advance the pointer, and stop if we reach the end.
        ++m_cur_iter;
        m_cur_cluster = nullptr;
        if (m_cur_iter == m_graph->m_main_chunkindex.end()) break;
        // Find the cluster pointed to by m_cur_iter.
        const auto& chunk_data = *m_cur_iter;
        const auto& chunk_end_entry = m_graph->m_entries[chunk_data.m_graph_index];
        m_cur_cluster = chunk_end_entry.m_locator[0].cluster;
        m_known_end_of_cluster = false;
        // If we previously skipped a chunk from this cluster we cannot include more from it.
        if (!m_excluded_clusters.contains(m_cur_cluster->m_sequence)) break;
    }
}

std::optional<std::pair<std::vector<TxGraph::Ref*>, FeePerWeight>> BlockBuilderImpl::GetCurrentChunk() noexcept
{
    std::optional<std::pair<std::vector<TxGraph::Ref*>, FeePerWeight>> ret;
    // Populate the return value if we are not done.
    if (m_cur_iter != m_graph->m_main_chunkindex.end()) {
        ret.emplace();
        const auto& chunk_data = *m_cur_iter;
        const auto& chunk_end_entry = m_graph->m_entries[chunk_data.m_graph_index];
        if (chunk_data.m_chunk_count == LinearizationIndex(-1)) {
            // Special case in case just a single transaction remains, avoiding the need to
            // dispatch to and dereference Cluster.
            ret->first.resize(1);
            Assume(chunk_end_entry.m_ref != nullptr);
            ret->first[0] = chunk_end_entry.m_ref;
            m_known_end_of_cluster = true;
        } else {
            Assume(m_cur_cluster);
            ret->first.resize(chunk_data.m_chunk_count);
            auto start_pos = chunk_end_entry.m_main_lin_index + 1 - chunk_data.m_chunk_count;
            m_known_end_of_cluster = m_cur_cluster->GetClusterRefs(*m_graph, ret->first, start_pos);
            // If the chunk size was 1 and at end of cluster, then the special case above should
            // have been used.
            Assume(!m_known_end_of_cluster || chunk_data.m_chunk_count > 1);
        }
        ret->second = chunk_end_entry.m_main_chunk_feerate;
    }
    return ret;
}

BlockBuilderImpl::BlockBuilderImpl(TxGraphImpl& graph) noexcept : m_graph(&graph)
{
    // Make sure all clusters in main are up to date, and acceptable.
    m_graph->MakeAllAcceptable(0);
    // There cannot remain any inapplicable dependencies (only possible if main is oversized).
    Assume(m_graph->m_main_clusterset.m_deps_to_add.empty());
    // Remember that this object is observing the graph's index, so that we can detect concurrent
    // modifications.
    ++m_graph->m_main_chunkindex_observers;
    // Find the first chunk.
    m_cur_iter = m_graph->m_main_chunkindex.begin();
    m_cur_cluster = nullptr;
    if (m_cur_iter != m_graph->m_main_chunkindex.end()) {
        // Find the cluster pointed to by m_cur_iter.
        const auto& chunk_data = *m_cur_iter;
        const auto& chunk_end_entry = m_graph->m_entries[chunk_data.m_graph_index];
        m_cur_cluster = chunk_end_entry.m_locator[0].cluster;
    }
}

BlockBuilderImpl::~BlockBuilderImpl()
{
    Assume(m_graph->m_main_chunkindex_observers > 0);
    // Permit modifications to the main graph again after destroying the BlockBuilderImpl.
    --m_graph->m_main_chunkindex_observers;
}

void BlockBuilderImpl::Include() noexcept
{
    // The actual inclusion of the chunk is done by the calling code. All we have to do is switch
    // to the next chunk.
    Next();
}

void BlockBuilderImpl::Skip() noexcept
{
    // When skipping a chunk we need to not include anything more of the cluster, as that could make
    // the result topologically invalid. However, don't do this if the chunk is known to be the last
    // chunk of the cluster. This may significantly reduce the size of m_excluded_clusters,
    // especially when many singleton clusters are ignored.
    if (m_cur_cluster != nullptr && !m_known_end_of_cluster) {
        m_excluded_clusters.insert(m_cur_cluster->m_sequence);
    }
    Next();
}

std::unique_ptr<TxGraph::BlockBuilder> TxGraphImpl::GetBlockBuilder() noexcept
{
    return std::make_unique<BlockBuilderImpl>(*this);
}

std::pair<std::vector<TxGraph::Ref*>, FeePerWeight> TxGraphImpl::GetWorstMainChunk() noexcept
{
    std::pair<std::vector<Ref*>, FeePerWeight> ret;
    // Make sure all clusters in main are up to date, and acceptable.
    MakeAllAcceptable(0);
    Assume(m_main_clusterset.m_deps_to_add.empty());
    // If the graph is not empty, populate ret.
    if (!m_main_chunkindex.empty()) {
        const auto& chunk_data = *m_main_chunkindex.rbegin();
        const auto& chunk_end_entry = m_entries[chunk_data.m_graph_index];
        Cluster* cluster = chunk_end_entry.m_locator[0].cluster;
        if (chunk_data.m_chunk_count == LinearizationIndex(-1) || chunk_data.m_chunk_count == 1)  {
            // Special case for singletons.
            ret.first.resize(1);
            Assume(chunk_end_entry.m_ref != nullptr);
            ret.first[0] = chunk_end_entry.m_ref;
        } else {
            ret.first.resize(chunk_data.m_chunk_count);
            auto start_pos = chunk_end_entry.m_main_lin_index + 1 - chunk_data.m_chunk_count;
            cluster->GetClusterRefs(*this, ret.first, start_pos);
            std::reverse(ret.first.begin(), ret.first.end());
        }
        ret.second = chunk_end_entry.m_main_chunk_feerate;
    }
    return ret;
}

std::vector<TxGraph::Ref*> TxGraphImpl::Trim() noexcept
{
    int level = GetTopLevel();
    Assume(m_main_chunkindex_observers == 0 || level != 0);
    std::vector<TxGraph::Ref*> ret;

    // Compute the groups of to-be-merged Clusters (which also applies all removals, and splits).
    auto& clusterset = GetClusterSet(level);
    if (clusterset.m_oversized == false) return ret;
    GroupClusters(level);
    Assume(clusterset.m_group_data.has_value());
    // Nothing to do if not oversized.
    Assume(clusterset.m_oversized.has_value());
    if (clusterset.m_oversized == false) return ret;

    // In this function, would-be clusters (as precomputed in m_group_data by GroupClusters) are
    // trimmed by removing transactions in them such that the resulting clusters satisfy the size
    // and count limits.
    //
    // It works by defining for each would-be cluster a rudimentary linearization: at every point
    // the highest-chunk-feerate remaining transaction is picked among those with no unmet
    // dependencies. "Dependency" here means either a to-be-added dependency (m_deps_to_add), or
    // an implicit dependency added between any two consecutive transaction in their current
    // cluster linearization. So it can be seen as a "merge sort" of the chunks of the clusters,
    // but respecting the dependencies being added.
    //
    // This rudimentary linearization is computed lazily, by putting all eligible (no unmet
    // dependencies) transactions in a heap, and popping the highest-feerate one from it. Along the
    // way, the counts and sizes of the would-be clusters up to that point are tracked (by
    // partitioning the involved transactions using a union-find structure). Any transaction whose
    // addition would cause a violation is removed, along with all their descendants.
    //
    // A next invocation of GroupClusters (after applying the removals) will compute the new
    // resulting clusters, and none of them will violate the limits.

    /** All dependencies (both to be added ones, and implicit ones between consecutive transactions
     *  in existing cluster linearizations), sorted by parent. */
    std::vector<std::pair<GraphIndex, GraphIndex>> deps_by_parent;
    /** Same, but sorted by child. */
    std::vector<std::pair<GraphIndex, GraphIndex>> deps_by_child;
    /** Information about all transactions involved in a Cluster group to be trimmed, sorted by
     *  GraphIndex. It contains entries both for transactions that have already been included,
     *  and ones that have not yet been. */
    std::vector<TrimTxData> trim_data;
    /** Iterators into trim_data, treated as a max heap according to cmp_fn below. Each entry is
     *  a transaction that has not yet been included yet, but all its ancestors have. */
    std::vector<std::vector<TrimTxData>::iterator> trim_heap;
    /** The list of representatives of the partitions a given transaction depends on. */
    std::vector<TrimTxData*> current_deps;

    /** Function to define the ordering of trim_heap. */
    static constexpr auto cmp_fn = [](auto a, auto b) noexcept {
        // Sort by increasing chunk feerate, and then by decreasing size.
        // We do not need to sort by cluster or within clusters, because due to the implicit
        // dependency between consecutive linearization elements, no two transactions from the
        // same Cluster will ever simultaneously be in the heap.
        return a->m_chunk_feerate < b->m_chunk_feerate;
    };

    /** Given a TrimTxData entry, find the representative of the partition it is in. */
    static constexpr auto find_fn = [](TrimTxData* arg) noexcept {
        while (arg != arg->m_uf_parent) {
            // Replace pointer to parent with pointer to grandparent (path splitting).
            // See https://en.wikipedia.org/wiki/Disjoint-set_data_structure#Finding_set_representatives.
            auto par = arg->m_uf_parent;
            arg->m_uf_parent = par->m_uf_parent;
            arg = par;
        }
        return arg;
    };

    /** Given two TrimTxData entries, union the partitions they are in, and return the
     *  representative. */
    static constexpr auto union_fn = [](TrimTxData* arg1, TrimTxData* arg2) noexcept {
        // Replace arg1 and arg2 by their representatives.
        auto rep1 = find_fn(arg1);
        auto rep2 = find_fn(arg2);
        // Bail out if both representatives are the same, because that means arg1 and arg2 are in
        // the same partition already.
        if (rep1 == rep2) return rep1;
        // Pick the lower-count root to become a child of the higher-count one.
        // See https://en.wikipedia.org/wiki/Disjoint-set_data_structure#Union_by_size.
        if (rep1->m_uf_count < rep2->m_uf_count) std::swap(rep1, rep2);
        rep2->m_uf_parent = rep1;
        // Add the statistics of arg2 (which is no longer a representative) to those of arg1 (which
        // is now the representative for both).
        rep1->m_uf_size += rep2->m_uf_size;
        rep1->m_uf_count += rep2->m_uf_count;
        return rep1;
    };

    /** Get iterator to TrimTxData entry for a given index. */
    auto locate_fn = [&](GraphIndex index) noexcept {
        auto it = std::lower_bound(trim_data.begin(), trim_data.end(), index, [](TrimTxData& elem, GraphIndex idx) noexcept {
            return elem.m_index < idx;
        });
        Assume(it != trim_data.end() && it->m_index == index);
        return it;
    };

    // For each group of to-be-merged Clusters.
    for (const auto& group_data : clusterset.m_group_data->m_groups) {
        trim_data.clear();
        trim_heap.clear();
        deps_by_child.clear();
        deps_by_parent.clear();

        // Gather trim data and implicit dependency data from all involved Clusters.
        auto cluster_span = std::span{clusterset.m_group_data->m_group_clusters}
                                .subspan(group_data.m_cluster_offset, group_data.m_cluster_count);
        uint64_t size{0};
        for (Cluster* cluster : cluster_span) {
            size += cluster->AppendTrimData(trim_data, deps_by_child);
        }
        // If this group of Clusters does not violate any limits, continue to the next group.
        if (trim_data.size() <= m_max_cluster_count && size <= m_max_cluster_size) continue;
        // Sort the trim data by GraphIndex. In what follows, we will treat this sorted vector as
        // a map from GraphIndex to TrimTxData via locate_fn, and its ordering will not change
        // anymore.
        std::sort(trim_data.begin(), trim_data.end(), [](auto& a, auto& b) noexcept { return a.m_index < b.m_index; });

        // Add the explicitly added dependencies to deps_by_child.
        deps_by_child.insert(deps_by_child.end(),
                             clusterset.m_deps_to_add.begin() + group_data.m_deps_offset,
                             clusterset.m_deps_to_add.begin() + group_data.m_deps_offset + group_data.m_deps_count);

        // Sort deps_by_child by child transaction GraphIndex. The order will not be changed
        // anymore after this.
        std::sort(deps_by_child.begin(), deps_by_child.end(), [](auto& a, auto& b) noexcept { return a.second < b.second; });
        // Fill m_parents_count and m_parents_offset in trim_data, as well as m_deps_left, and
        // initially populate trim_heap. Because of the sort above, all dependencies involving the
        // same child are grouped together, so a single linear scan suffices.
        auto deps_it = deps_by_child.begin();
        for (auto trim_it = trim_data.begin(); trim_it != trim_data.end(); ++trim_it) {
            trim_it->m_parent_offset = deps_it - deps_by_child.begin();
            trim_it->m_deps_left = 0;
            while (deps_it != deps_by_child.end() && deps_it->second == trim_it->m_index) {
                ++trim_it->m_deps_left;
                ++deps_it;
            }
            trim_it->m_parent_count = trim_it->m_deps_left;
            // If this transaction has no unmet dependencies, and is not oversized, add it to the
            // heap (just append for now, the heapification happens below).
            if (trim_it->m_deps_left == 0 && trim_it->m_tx_size <= m_max_cluster_size) {
                trim_heap.push_back(trim_it);
            }
        }
        Assume(deps_it == deps_by_child.end());

        // Construct deps_by_parent, sorted by parent transaction GraphIndex. The order will not be
        // changed anymore after this.
        deps_by_parent = deps_by_child;
        std::sort(deps_by_parent.begin(), deps_by_parent.end(), [](auto& a, auto& b) noexcept { return a.first < b.first; });
        // Fill m_children_offset and m_children_count in trim_data. Because of the sort above, all
        // dependencies involving the same parent are grouped together, so a single linear scan
        // suffices.
        deps_it = deps_by_parent.begin();
        for (auto& trim_entry : trim_data) {
            trim_entry.m_children_count = 0;
            trim_entry.m_children_offset = deps_it - deps_by_parent.begin();
            while (deps_it != deps_by_parent.end() && deps_it->first == trim_entry.m_index) {
                ++trim_entry.m_children_count;
                ++deps_it;
            }
        }
        Assume(deps_it == deps_by_parent.end());

        // Build a heap of all transactions with 0 unmet dependencies.
        std::make_heap(trim_heap.begin(), trim_heap.end(), cmp_fn);

        // Iterate over to-be-included transactions, and convert them to included transactions, or
        // skip them if adding them would violate resource limits of the would-be cluster.
        //
        // It is possible that the heap empties without ever hitting either cluster limit, in case
        // the implied graph (to be added dependencies plus implicit dependency between each
        // original transaction and its predecessor in the linearization it came from) contains
        // cycles. Such cycles will be removed entirely, because each of the transactions in the
        // cycle permanently have unmet dependencies. However, this cannot occur in real scenarios
        // where Trim() is called to deal with reorganizations that would violate cluster limits,
        // as all added dependencies are in the same direction (from old mempool transactions to
        // new from-block transactions); cycles require dependencies in both directions to be
        // added.
        while (!trim_heap.empty()) {
            // Move the best remaining transaction to the end of trim_heap.
            std::pop_heap(trim_heap.begin(), trim_heap.end(), cmp_fn);
            // Pop it, and find its TrimTxData.
            auto& entry = *trim_heap.back();
            trim_heap.pop_back();

            // Initialize it as a singleton partition.
            entry.m_uf_parent = &entry;
            entry.m_uf_count = 1;
            entry.m_uf_size = entry.m_tx_size;

            // Find the distinct transaction partitions this entry depends on.
            current_deps.clear();
            for (auto& [par, chl] : std::span{deps_by_child}.subspan(entry.m_parent_offset, entry.m_parent_count)) {
                Assume(chl == entry.m_index);
                current_deps.push_back(find_fn(&*locate_fn(par)));
            }
            std::sort(current_deps.begin(), current_deps.end());
            current_deps.erase(std::unique(current_deps.begin(), current_deps.end()), current_deps.end());

            // Compute resource counts.
            uint32_t new_count = 1;
            uint64_t new_size = entry.m_tx_size;
            for (TrimTxData* ptr : current_deps) {
                new_count += ptr->m_uf_count;
                new_size += ptr->m_uf_size;
            }
            // Skip the entry if this would violate any limit.
            if (new_count > m_max_cluster_count || new_size > m_max_cluster_size) continue;

            // Union the partitions this transaction and all its dependencies are in together.
            auto rep = &entry;
            for (TrimTxData* ptr : current_deps) rep = union_fn(ptr, rep);
            // Mark the entry as included (so the loop below will not remove the transaction).
            entry.m_deps_left = uint32_t(-1);
            // Mark each to-be-added dependency involving this transaction as parent satisfied.
            for (auto& [par, chl] : std::span{deps_by_parent}.subspan(entry.m_children_offset, entry.m_children_count)) {
                Assume(par == entry.m_index);
                auto chl_it = locate_fn(chl);
                // Reduce the number of unmet dependencies of chl_it, and if that brings the number
                // to zero, add it to the heap of includable transactions.
                Assume(chl_it->m_deps_left > 0);
                if (--chl_it->m_deps_left == 0) {
                    trim_heap.push_back(chl_it);
                    std::push_heap(trim_heap.begin(), trim_heap.end(), cmp_fn);
                }
            }
        }

        // Remove all the transactions that were not processed above. Because nothing gets
        // processed until/unless all its dependencies are met, this automatically guarantees
        // that if a transaction is removed, all its descendants, or would-be descendants, are
        // removed as well.
        for (const auto& trim_entry : trim_data) {
            if (trim_entry.m_deps_left != uint32_t(-1)) {
                ret.push_back(m_entries[trim_entry.m_index].m_ref);
                clusterset.m_to_remove.push_back(trim_entry.m_index);
            }
        }
    }
    clusterset.m_group_data.reset();
    clusterset.m_oversized = false;
    Assume(!ret.empty());
    return ret;
}

size_t TxGraphImpl::GetMainMemoryUsage() noexcept
{
    // Make sure splits/merges are applied, as memory usage may not be representative otherwise.
    SplitAll(/*up_to_level=*/0);
    ApplyDependencies(/*level=*/0);
    // Compute memory usage
    size_t usage = /* From clusters */
                   m_main_clusterset.m_cluster_usage +
                   /* From Entry objects. */
                   sizeof(Entry) * m_main_clusterset.m_txcount +
                   /* From the chunk index. */
                   memusage::DynamicUsage(m_main_chunkindex);
    return usage;
}

} // namespace

TxGraph::Ref::~Ref()
{
    if (m_graph) {
        // Inform the TxGraph about the Ref being destroyed.
        m_graph->UnlinkRef(m_index);
        m_graph = nullptr;
    }
}

TxGraph::Ref::Ref(Ref&& other) noexcept
{
    // Inform the TxGraph of other that its Ref is being moved.
    if (other.m_graph) other.m_graph->UpdateRef(other.m_index, *this);
    // Actually move the contents.
    std::swap(m_graph, other.m_graph);
    std::swap(m_index, other.m_index);
}

std::unique_ptr<TxGraph> MakeTxGraph(unsigned max_cluster_count, uint64_t max_cluster_size, uint64_t acceptable_iters) noexcept
{
    return std::make_unique<TxGraphImpl>(max_cluster_count, max_cluster_size, acceptable_iters);
}
