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
#include <memory>
#include <set>
#include <span>
#include <utility>

namespace {

using namespace cluster_linearize;

/** The maximum number of levels a TxGraph can have (0 = main, 1 = staging). */
static constexpr int MAX_LEVELS{2};

// Forward declare the TxGraph implementation class.
class TxGraphImpl;

/** Position of a DepGraphIndex within a Cluster::m_linearization. */
using LinearizationIndex = uint32_t;
/** Position of a Cluster within Graph::ClusterSet::m_clusters. */
using ClusterSetIndex = uint32_t;

/** Quality levels for cached cluster linearizations. */
enum class QualityLevel
{
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

/** A grouping of connected transactions inside a TxGraphImpl::ClusterSet. */
class Cluster
{
    friend class TxGraphImpl;
    using GraphIndex = TxGraph::GraphIndex;
    using SetType = BitSet<MAX_CLUSTER_COUNT_LIMIT>;
    /** The DepGraph for this cluster, holding all feerates, and ancestors/descendants. */
    DepGraph<SetType> m_depgraph;
    /** m_mapping[i] gives the GraphIndex for the position i transaction in m_depgraph. Values for
     *  positions i that do not exist in m_depgraph shouldn't ever be accessed and thus don't
     *  matter. m_mapping.size() equals m_depgraph.PositionRange(). */
    std::vector<GraphIndex> m_mapping;
    /** The current linearization of the cluster. m_linearization.size() equals
     *  m_depgraph.TxCount(). This is always kept topological. */
    std::vector<DepGraphIndex> m_linearization;
    /** The quality level of m_linearization. */
    QualityLevel m_quality{QualityLevel::NONE};
    /** Which position this Cluster has in Graph::ClusterSet::m_clusters[m_quality]. */
    ClusterSetIndex m_setindex{ClusterSetIndex(-1)};
    /** Which level this Cluster is at in the graph (-1=not inserted, 0=main, 1=staging). */
    int m_level{-1};
    /** Sequence number for this Cluster (for tie-breaking comparison between equal-chunk-feerate
        transactions in distinct clusters). */
    uint64_t m_sequence;

public:
    Cluster() noexcept = delete;
    /** Construct an empty Cluster. */
    explicit Cluster(uint64_t sequence) noexcept;
    /** Construct a singleton Cluster. */
    explicit Cluster(uint64_t sequence, TxGraphImpl& graph, const FeePerWeight& feerate, GraphIndex graph_index) noexcept;

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
    /** Whether this cluster requires splitting. */
    bool NeedsSplitting() const noexcept
    {
        return m_quality == QualityLevel::NEEDS_SPLIT ||
               m_quality == QualityLevel::NEEDS_SPLIT_ACCEPTABLE;
    }
    /** Get the number of transactions in this Cluster. */
    LinearizationIndex GetTxCount() const noexcept { return m_linearization.size(); }
    /** Given a DepGraphIndex into this Cluster, find the corresponding GraphIndex. */
    GraphIndex GetClusterEntry(DepGraphIndex index) const noexcept { return m_mapping[index]; }
    /** Only called by Graph::SwapIndexes. */
    void UpdateMapping(DepGraphIndex cluster_idx, GraphIndex graph_idx) noexcept { m_mapping[cluster_idx] = graph_idx; }
    /** Push changes to Cluster and its linearization to the TxGraphImpl Entry objects. */
    void Updated(TxGraphImpl& graph) noexcept;
    /** Create a copy of this Cluster in staging, returning a pointer to it (used by PullIn). */
    Cluster* CopyToStaging(TxGraphImpl& graph) const noexcept;
    /** Get the list of Clusters in main that conflict with this one (which is assumed to be in staging). */
    void GetConflicts(const TxGraphImpl& graph, std::vector<Cluster*>& out) const noexcept;
    /** Mark all the Entry objects belonging to this staging Cluster as missing. The Cluster must be
     *  deleted immediately after. */
    void MakeStagingTransactionsMissing(TxGraphImpl& graph) noexcept;
    /** Remove all transactions from a Cluster. */
    void Clear(TxGraphImpl& graph) noexcept;
    /** Change a Cluster's level from 1 (staging) to 0 (main). */
    void MoveToMain(TxGraphImpl& graph) noexcept;

    // Functions that implement the Cluster-specific side of internal TxGraphImpl mutations.

    /** Apply all removals from the front of to_remove that apply to this Cluster, popping them
     *  off. These must be at least one such entry. */
    void ApplyRemovals(TxGraphImpl& graph, std::span<GraphIndex>& to_remove) noexcept;
    /** Split this cluster (must have a NEEDS_SPLIT* quality). Returns whether to delete this
     *  Cluster afterwards. */
    [[nodiscard]] bool Split(TxGraphImpl& graph) noexcept;
    /** Move all transactions from cluster to *this (as separate components). */
    void Merge(TxGraphImpl& graph, Cluster& cluster) noexcept;
    /** Given a span of (parent, child) pairs that all belong to this Cluster, apply them. */
    void ApplyDependencies(TxGraphImpl& graph, std::span<std::pair<GraphIndex, GraphIndex>> to_apply) noexcept;
    /** Improve the linearization of this Cluster. */
    void Relinearize(TxGraphImpl& graph, uint64_t max_iters) noexcept;
    /** For every chunk in the cluster, append its FeeFrac to ret. */
    void AppendChunkFeerates(std::vector<FeeFrac>& ret) const noexcept;

    // Functions that implement the Cluster-specific side of public TxGraph functions.

    /** Process elements from the front of args that apply to this cluster, and append Refs for the
     *  union of their ancestors to output. */
    void GetAncestorRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept;
    /** Process elements from the front of args that apply to this cluster, and append Refs for the
     *  union of their descendants to output. */
    void GetDescendantRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept;
    /** Populate range with refs for the transactions in this Cluster's linearization, from
     *  position start_pos until start_pos+range.size()-1, inclusive. Returns whether that
     *  range includes the last transaction in the linearization. */
    bool GetClusterRefs(TxGraphImpl& graph, std::span<TxGraph::Ref*> range, LinearizationIndex start_pos) noexcept;
    /** Get the individual transaction feerate of a Cluster element. */
    FeePerWeight GetIndividualFeerate(DepGraphIndex idx) noexcept;
    /** Modify the fee of a Cluster element. */
    void SetFee(TxGraphImpl& graph, DepGraphIndex idx, int64_t fee) noexcept;

    // Debugging functions.

    void SanityCheck(const TxGraphImpl& graph, int level) const;
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
    friend class BlockBuilderImpl;
private:
    /** Internal RNG. */
    FastRandomContext m_rng;
    /** This TxGraphImpl's maximum cluster count limit. */
    const DepGraphIndex m_max_cluster_count;

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
        /** Whether at least one of the groups cannot be applied because it would result in a
         *  Cluster that violates the cluster count limit. */
        bool m_group_oversized;
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
        /** Whether this graph is oversized (if known). This roughly matches
         *  m_group_data->m_group_oversized, but may be known even if m_group_data is not. */
        std::optional<bool> m_oversized{false};

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
    /** Cache of discarded ChunkIndex node handles to re-use, avoiding additional allocation. */
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
    /** Construct a new TxGraphImpl with the specified maximum cluster count. */
    explicit TxGraphImpl(DepGraphIndex max_cluster_count) noexcept :
        m_max_cluster_count(max_cluster_count),
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
    Cluster* FindCluster(GraphIndex idx, int level) const noexcept;
    /** Extract a Cluster from its ClusterSet. */
    std::unique_ptr<Cluster> ExtractCluster(int level, QualityLevel quality, ClusterSetIndex setindex) noexcept;
    /** Delete a Cluster. */
    void DeleteCluster(Cluster& cluster) noexcept;
    /** Insert a Cluster into its ClusterSet. */
    ClusterSetIndex InsertCluster(int level, std::unique_ptr<Cluster>&& cluster, QualityLevel quality) noexcept;
    /** Change the QualityLevel of a Cluster (identified by old_quality and old_index). */
    void SetClusterQuality(int level, QualityLevel old_quality, ClusterSetIndex old_index, QualityLevel new_quality) noexcept;
    /** Get the index of the top level ClusterSet (staging if it exists, main otherwise). */
    int GetTopLevel() const noexcept { return m_staging_clusterset.has_value(); }
    /** Get the specified level (staging if it exists and main_only is not specified, main otherwise). */
    int GetSpecifiedLevel(bool main_only) const noexcept { return m_staging_clusterset.has_value() && !main_only; }
    /** Get a reference to the ClusterSet at the specified level (which must exist). */
    ClusterSet& GetClusterSet(int level) noexcept;
    const ClusterSet& GetClusterSet(int level) const noexcept;
    /** Make a transaction not exist at a specified level. It must currently exist there. */
    void ClearLocator(int level, GraphIndex index) noexcept;
    /** Find which Clusters in main conflict with ones in staging. */
    std::vector<Cluster*> GetConflicts() const noexcept;
    /** Clear an Entry's ChunkData. */
    void ClearChunkData(Entry& entry) noexcept;
    /** Give an Entry a ChunkData object. */
    void CreateChunkData(GraphIndex idx, LinearizationIndex chunk_count) noexcept;

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
    Cluster* PullIn(Cluster* cluster) noexcept;
    /** Apply all removals queued up in m_to_remove to the relevant Clusters (which get a
     *  NEEDS_SPLIT* QualityLevel) up to the specified level. */
    void ApplyRemovals(int up_to_level) noexcept;
    /** Split an individual cluster. */
    void Split(Cluster& cluster) noexcept;
    /** Split all clusters that need splitting up to the specified level. */
    void SplitAll(int up_to_level) noexcept;
    /** Populate m_group_data based on m_deps_to_add in the specified level. */
    void GroupClusters(int level) noexcept;
    /** Merge the specified clusters. */
    void Merge(std::span<Cluster*> to_merge) noexcept;
    /** Apply all m_deps_to_add to the relevant Clusters in the specified level. */
    void ApplyDependencies(int level) noexcept;
    /** Make a specified Cluster have quality ACCEPTABLE or OPTIMAL. */
    void MakeAcceptable(Cluster& cluster) noexcept;
    /** Make all Clusters at the specified level have quality ACCEPTABLE or OPTIMAL. */
    void MakeAllAcceptable(int level) noexcept;

    // Implementations for the public TxGraph interface.

    Ref AddTransaction(const FeePerWeight& feerate) noexcept final;
    void RemoveTransaction(const Ref& arg) noexcept final;
    void AddDependency(const Ref& parent, const Ref& child) noexcept final;
    void SetTransactionFee(const Ref&, int64_t fee) noexcept final;

    void DoWork() noexcept final;

    void StartStaging() noexcept final;
    void CommitStaging() noexcept final;
    void AbortStaging() noexcept final;
    bool HaveStaging() const noexcept final { return m_staging_clusterset.has_value(); }

    bool Exists(const Ref& arg, bool main_only = false) noexcept final;
    FeePerWeight GetMainChunkFeerate(const Ref& arg) noexcept final;
    FeePerWeight GetIndividualFeerate(const Ref& arg) noexcept final;
    std::vector<Ref*> GetCluster(const Ref& arg, bool main_only = false) noexcept final;
    std::vector<Ref*> GetAncestors(const Ref& arg, bool main_only = false) noexcept final;
    std::vector<Ref*> GetDescendants(const Ref& arg, bool main_only = false) noexcept final;
    std::vector<Ref*> GetAncestorsUnion(std::span<const Ref* const> args, bool main_only = false) noexcept final;
    std::vector<Ref*> GetDescendantsUnion(std::span<const Ref* const> args, bool main_only = false) noexcept final;
    GraphIndex GetTransactionCount(bool main_only = false) noexcept final;
    bool IsOversized(bool main_only = false) noexcept final;
    std::strong_ordering CompareMainOrder(const Ref& a, const Ref& b) noexcept final;
    GraphIndex CountDistinctClusters(std::span<const Ref* const> refs, bool main_only = false) noexcept final;
    std::pair<std::vector<FeeFrac>, std::vector<FeeFrac>> GetMainStagingDiagrams() noexcept final;

    std::unique_ptr<BlockBuilder> GetBlockBuilder() noexcept final;
    std::pair<std::vector<Ref*>, FeePerWeight> GetWorstMainChunk() noexcept final;

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
    /** Clusters which we're not including further transactions from. */
    std::set<Cluster*> m_excluded_clusters;
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

void TxGraphImpl::ClearLocator(int level, GraphIndex idx) noexcept
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
    // If clearing main, adjust the status of Locators of this transaction in staging, if it exists.
    if (level == 0 && GetTopLevel() == 1) {
        if (entry.m_locator[1].IsRemoved()) {
            entry.m_locator[1].SetMissing();
        } else if (!entry.m_locator[1].IsPresent()) {
            --m_staging_clusterset->m_txcount;
        }
    }
    if (level == 0) ClearChunkData(entry);
}

void Cluster::Updated(TxGraphImpl& graph) noexcept
{
    // Update all the Locators for this Cluster's Entry objects.
    for (DepGraphIndex idx : m_linearization) {
        auto& entry = graph.m_entries[m_mapping[idx]];
        // Discard any potential ChunkData prior to modifying the Cluster (as that could
        // invalidate its ordering).
        if (m_level == 0) graph.ClearChunkData(entry);
        entry.m_locator[m_level].SetPresent(this, idx);
    }
    // If this is for the main graph (level = 0), and the Cluster's quality is ACCEPTABLE or
    // OPTIMAL, compute its chunking and store its information in the Entry's m_main_lin_index
    // and m_main_chunk_feerate. These fields are only accessed after making the entire graph
    // ACCEPTABLE, so it is pointless to compute these if we haven't reached that quality level
    // yet.
    if (m_level == 0 && IsAcceptable()) {
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

void Cluster::GetConflicts(const TxGraphImpl& graph, std::vector<Cluster*>& out) const noexcept
{
    Assume(m_level == 1);
    for (auto i : m_linearization) {
        auto& entry = graph.m_entries[m_mapping[i]];
        // For every transaction Entry in this Cluster, if it also exists in a lower-level Cluster,
        // then that Cluster conflicts.
        if (entry.m_locator[0].IsPresent()) {
            out.push_back(entry.m_locator[0].cluster);
        }
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

Cluster* Cluster::CopyToStaging(TxGraphImpl& graph) const noexcept
{
    // Construct an empty Cluster.
    auto ret = std::make_unique<Cluster>(graph.m_next_sequence_counter++);
    auto ptr = ret.get();
    // Copy depgraph, mapping, and linearization/
    ptr->m_depgraph = m_depgraph;
    ptr->m_mapping = m_mapping;
    ptr->m_linearization = m_linearization;
    // Insert the new Cluster into the graph.
    graph.InsertCluster(1, std::move(ret), m_quality);
    // Update its Locators.
    ptr->Updated(graph);
    return ptr;
}

void Cluster::ApplyRemovals(TxGraphImpl& graph, std::span<GraphIndex>& to_remove) noexcept
{
    // Iterate over the prefix of to_remove that applies to this cluster.
    Assume(!to_remove.empty());
    SetType todo;
    do {
        GraphIndex idx = to_remove.front();
        Assume(idx < graph.m_entries.size());
        auto& entry = graph.m_entries[idx];
        auto& locator = entry.m_locator[m_level];
        // Stop once we hit an entry that applies to another Cluster.
        if (locator.cluster != this) break;
        // - Remember it in a set of to-remove DepGraphIndexes.
        todo.Set(locator.index);
        // - Remove from m_mapping. This isn't strictly necessary as unused positions in m_mapping
        //   are just never accessed, but set it to -1 here to increase the ability to detect a bug
        //   that causes it to be accessed regardless.
        m_mapping[locator.index] = GraphIndex(-1);
        // - Remove its linearization index from the Entry (if in main).
        if (m_level == 0) {
            entry.m_main_lin_index = LinearizationIndex(-1);
        }
        // - Mark it as missing/removed in the Entry's locator.
        graph.ClearLocator(m_level, idx);
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
    graph.SetClusterQuality(m_level, m_quality, m_setindex, quality);
    Updated(graph);
}

void Cluster::Clear(TxGraphImpl& graph) noexcept
{
    for (auto i : m_linearization) {
        graph.ClearLocator(m_level, m_mapping[i]);
    }
    m_depgraph = {};
    m_linearization.clear();
    m_mapping.clear();
}

void Cluster::MoveToMain(TxGraphImpl& graph) noexcept
{
    Assume(m_level == 1);
    for (auto i : m_linearization) {
        GraphIndex idx = m_mapping[i];
        auto& entry = graph.m_entries[idx];
        entry.m_locator[1].SetMissing();
    }
    auto quality = m_quality;
    auto cluster = graph.ExtractCluster(1, quality, m_setindex);
    graph.InsertCluster(0, std::move(cluster), quality);
    Updated(graph);
}

void Cluster::AppendChunkFeerates(std::vector<FeeFrac>& ret) const noexcept
{
    auto chunk_feerates = ChunkLinearization(m_depgraph, m_linearization);
    ret.reserve(ret.size() + chunk_feerates.size());
    ret.insert(ret.end(), chunk_feerates.begin(), chunk_feerates.end());
}

bool Cluster::Split(TxGraphImpl& graph) noexcept
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
        if (first && component == todo) {
            // The existing Cluster is an entire component. Leave it be, but update its quality.
            Assume(todo == m_depgraph.Positions());
            graph.SetClusterQuality(m_level, m_quality, m_setindex, new_quality);
            // If this made the quality ACCEPTABLE or OPTIMAL, we need to compute and cache its
            // chunking.
            Updated(graph);
            return false;
        }
        first = false;
        // Construct a new Cluster to hold the found component.
        auto new_cluster = std::make_unique<Cluster>(graph.m_next_sequence_counter++);
        new_clusters.push_back(new_cluster.get());
        // Remember that all the component's transactions go to this new Cluster. The positions
        // will be determined below, so use -1 for now.
        for (auto i : component) {
            remap[i] = {new_cluster.get(), DepGraphIndex(-1)};
        }
        graph.InsertCluster(m_level, std::move(new_cluster), new_quality);
        todo -= component;
    }
    // Redistribute the transactions.
    for (auto i : m_linearization) {
        /** The cluster which transaction originally in position i is moved to. */
        Cluster* new_cluster = remap[i].first;
        // Copy the transaction to the new cluster's depgraph, and remember the position.
        remap[i].second = new_cluster->m_depgraph.AddTransaction(m_depgraph.FeeRate(i));
        // Create new mapping entry.
        new_cluster->m_mapping.push_back(m_mapping[i]);
        // Create a new linearization entry. As we're only appending transactions, they equal the
        // DepGraphIndex.
        new_cluster->m_linearization.push_back(remap[i].second);
    }
    // Redistribute the dependencies.
    for (auto i : m_linearization) {
        /** The cluster transaction in position i is moved to. */
        Cluster* new_cluster = remap[i].first;
        // Copy its parents, translating positions.
        SetType new_parents;
        for (auto par : m_depgraph.GetReducedParents(i)) new_parents.Set(remap[par].second);
        new_cluster->m_depgraph.AddDependencies(new_parents, remap[i].second);
    }
    // Update all the Locators of moved transactions.
    for (Cluster* new_cluster : new_clusters) {
        new_cluster->Updated(graph);
    }
    // Wipe this Cluster, and return that it needs to be deleted.
    m_depgraph = DepGraph<SetType>{};
    m_mapping.clear();
    m_linearization.clear();
    return true;
}

void Cluster::Merge(TxGraphImpl& graph, Cluster& other) noexcept
{
    /** Vector to store the positions in this Cluster for each position in other. */
    std::vector<DepGraphIndex> remap(other.m_depgraph.PositionRange());
    // Iterate over all transactions in the other Cluster (the one being absorbed).
    for (auto pos : other.m_linearization) {
        auto idx = other.m_mapping[pos];
        // Copy the transaction into this Cluster, and remember its position.
        auto new_pos = m_depgraph.AddTransaction(other.m_depgraph.FeeRate(pos));
        remap[pos] = new_pos;
        if (new_pos == m_mapping.size()) {
            m_mapping.push_back(idx);
        } else {
            m_mapping[new_pos] = idx;
        }
        m_linearization.push_back(new_pos);
        // Copy the transaction's dependencies, translating them using remap. Note that since
        // pos iterates over other.m_linearization, which is in topological order, all parents
        // of pos should already be in remap.
        SetType parents;
        for (auto par : other.m_depgraph.GetReducedParents(pos)) {
            parents.Set(remap[par]);
        }
        m_depgraph.AddDependencies(parents, remap[pos]);
        // Update the transaction's Locator. There is no need to call Updated() to update chunk
        // feerates, as Updated() will be invoked by Cluster::ApplyDependencies on the resulting
        // merged Cluster later anyway).
        auto& entry = graph.m_entries[idx];
        // Discard any potential ChunkData prior to modifying the Cluster (as that could
        // invalidate its ordering).
        if (m_level == 0) graph.ClearChunkData(entry);
        entry.m_locator[m_level].SetPresent(this, new_pos);
    }
    // Purge the other Cluster, now that everything has been moved.
    other.m_depgraph = DepGraph<SetType>{};
    other.m_linearization.clear();
    other.m_mapping.clear();
}

void Cluster::ApplyDependencies(TxGraphImpl& graph, std::span<std::pair<GraphIndex, GraphIndex>> to_apply) noexcept
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
        auto& first_child = graph.m_entries[it->second].m_locator[m_level];
        const auto child_idx = first_child.index;
        // Iterate over all to-be-added dependencies within that same child, gather the relevant
        // parents.
        SetType parents;
        while (it != to_apply.end()) {
            auto& child = graph.m_entries[it->second].m_locator[m_level];
            auto& parent = graph.m_entries[it->first].m_locator[m_level];
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

    // Finally push the changes to graph.m_entries.
    Updated(graph);
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
    ret->m_level = -1;

    // Clean up space in quality_cluster.
    auto max_setindex = quality_clusters.size() - 1;
    if (setindex != max_setindex) {
        // If the cluster was not the last element of quality_clusters, move that to take its place.
        quality_clusters.back()->m_setindex = setindex;
        quality_clusters.back()->m_level = level;
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
    cluster->m_level = level;
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

void TxGraphImpl::DeleteCluster(Cluster& cluster) noexcept
{
    // Extract the cluster from where it currently resides.
    auto cluster_ptr = ExtractCluster(cluster.m_level, cluster.m_quality, cluster.m_setindex);
    // And throw it away.
    cluster_ptr.reset();
}

Cluster* TxGraphImpl::FindCluster(GraphIndex idx, int level) const noexcept
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
        return entry.m_locator[l].cluster;
    }
    // If no non-empty locator was found, or an explicitly removed was hit, return nothing.
    return nullptr;
}

Cluster* TxGraphImpl::PullIn(Cluster* cluster) noexcept
{
    int to_level = GetTopLevel();
    Assume(to_level == 1);
    int level = cluster->m_level;
    Assume(level <= to_level);
    // Copy the Cluster from main to staging, if it's not already there.
    if (level == 0) {
        // Make the Cluster Acceptable before copying. This isn't strictly necessary, but doing it
        // now avoids doing double work later.
        MakeAcceptable(*cluster);
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
                auto cluster = FindCluster(index, level);
                if (cluster != nullptr) PullIn(cluster);
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
                cluster->ApplyRemovals(*this, to_remove_span);
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

void TxGraphImpl::Split(Cluster& cluster) noexcept
{
    // To split a Cluster, first make sure all removals are applied (as we might need to split
    // again afterwards otherwise).
    ApplyRemovals(cluster.m_level);
    bool del = cluster.Split(*this);
    if (del) {
        // Cluster::Split reports whether the Cluster is to be deleted.
        DeleteCluster(cluster);
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
                Split(*queue.back().get());
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
    // to which dependencies apply.
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
    clusterset.m_group_data->m_group_oversized = false;
    clusterset.m_deps_to_add.clear();
    clusterset.m_deps_to_add.reserve(an_deps.size());
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
        // Add all its clusters to it (copying those from an_clusters to m_group_clusters).
        while (an_clusters_it != an_clusters.end() && an_clusters_it->second == rep) {
            clusterset.m_group_data->m_group_clusters.push_back(an_clusters_it->first);
            total_count += an_clusters_it->first->GetTxCount();
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
        if (total_count > m_max_cluster_count) {
            clusterset.m_group_data->m_group_oversized = true;
        }
    }
    Assume(an_deps_it == an_deps.end());
    Assume(an_clusters_it == an_clusters.end());
    clusterset.m_oversized = clusterset.m_group_data->m_group_oversized;
    Compact();
}

void TxGraphImpl::Merge(std::span<Cluster*> to_merge) noexcept
{
    Assume(!to_merge.empty());
    // Nothing to do if a group consists of just a single Cluster.
    if (to_merge.size() == 1) return;

    // Move the largest Cluster to the front of to_merge. As all transactions in other to-be-merged
    // Clusters will be moved to that one, putting the largest one first minimizes the number of
    // moves.
    size_t max_size_pos{0};
    DepGraphIndex max_size = to_merge[max_size_pos]->GetTxCount();
    for (size_t i = 1; i < to_merge.size(); ++i) {
        DepGraphIndex size = to_merge[i]->GetTxCount();
        if (size > max_size) {
            max_size_pos = i;
            max_size = size;
        }
    }
    if (max_size_pos != 0) std::swap(to_merge[0], to_merge[max_size_pos]);

    // Merge all further Clusters in the group into the first one, and delete them.
    for (size_t i = 1; i < to_merge.size(); ++i) {
        to_merge[0]->Merge(*this, *to_merge[i]);
        DeleteCluster(*to_merge[i]);
    }
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
    if (clusterset.m_group_data->m_group_oversized) return;

    // For each group of to-be-merged Clusters.
    for (const auto& group_entry : clusterset.m_group_data->m_groups) {
        auto cluster_span = std::span{clusterset.m_group_data->m_group_clusters}
                                .subspan(group_entry.m_cluster_offset, group_entry.m_cluster_count);
        // Pull in all the Clusters that contain dependencies.
        if (level == 1) {
            for (Cluster*& cluster : cluster_span) {
                cluster = PullIn(cluster);
            }
        }
        // Invoke Merge() to merge them into a single Cluster.
        Merge(cluster_span);
        // Actually apply all to-be-added dependencies (all parents and children from this grouping
        // belong to the same Cluster at this point because of the merging above).
        auto deps_span = std::span{clusterset.m_deps_to_add}
                             .subspan(group_entry.m_deps_offset, group_entry.m_deps_count);
        Assume(!deps_span.empty());
        const auto& loc = m_entries[deps_span[0].second].m_locator[level];
        Assume(loc.IsPresent());
        loc.cluster->ApplyDependencies(*this, deps_span);
    }

    // Wipe the list of to-be-added dependencies now that they are applied.
    clusterset.m_deps_to_add.clear();
    Compact();
    // Also no further Cluster mergings are needed (note that we clear, but don't set to
    // std::nullopt, as that would imply the groupings are unknown).
    clusterset.m_group_data = GroupData{};
}

void Cluster::Relinearize(TxGraphImpl& graph, uint64_t max_iters) noexcept
{
    // We can only relinearize Clusters that do not need splitting.
    Assume(!NeedsSplitting());
    // No work is required for Clusters which are already optimally linearized.
    if (IsOptimal()) return;
    // Invoke the actual linearization algorithm (passing in the existing one).
    uint64_t rng_seed = graph.m_rng.rand64();
    auto [linearization, optimal] = Linearize(m_depgraph, max_iters, rng_seed, m_linearization);
    // Postlinearize if the result isn't optimal already. This guarantees (among other things)
    // that the chunks of the resulting linearization are all connected.
    if (!optimal) PostLinearize(m_depgraph, linearization);
    // Update the linearization.
    m_linearization = std::move(linearization);
    // Update the Cluster's quality.
    auto new_quality = optimal ? QualityLevel::OPTIMAL : QualityLevel::ACCEPTABLE;
    graph.SetClusterQuality(m_level, m_quality, m_setindex, new_quality);
    // Update the Entry objects.
    Updated(graph);
}

void TxGraphImpl::MakeAcceptable(Cluster& cluster) noexcept
{
    // Relinearize the Cluster if needed.
    if (!cluster.NeedsSplitting() && !cluster.IsAcceptable()) {
        cluster.Relinearize(*this, 10000);
    }
}

void TxGraphImpl::MakeAllAcceptable(int level) noexcept
{
    ApplyDependencies(level);
    auto& clusterset = GetClusterSet(level);
    if (clusterset.m_oversized == true) return;
    auto& queue = clusterset.m_clusters[int(QualityLevel::NEEDS_RELINEARIZE)];
    while (!queue.empty()) {
        MakeAcceptable(*queue.back().get());
    }
}

Cluster::Cluster(uint64_t sequence) noexcept : m_sequence{sequence} {}

Cluster::Cluster(uint64_t sequence, TxGraphImpl& graph, const FeePerWeight& feerate, GraphIndex graph_index) noexcept :
    m_sequence{sequence}
{
    // Create a new transaction in the DepGraph, and remember its position in m_mapping.
    auto cluster_idx = m_depgraph.AddTransaction(feerate);
    m_mapping.push_back(graph_index);
    m_linearization.push_back(cluster_idx);
}

TxGraph::Ref TxGraphImpl::AddTransaction(const FeePerWeight& feerate) noexcept
{
    Assume(m_main_chunkindex_observers == 0 || GetTopLevel() != 0);
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
    auto cluster = std::make_unique<Cluster>(m_next_sequence_counter++, *this, feerate, idx);
    auto cluster_ptr = cluster.get();
    int level = GetTopLevel();
    auto& clusterset = GetClusterSet(level);
    InsertCluster(level, std::move(cluster), QualityLevel::OPTIMAL);
    cluster_ptr->Updated(*this);
    ++clusterset.m_txcount;
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

bool TxGraphImpl::Exists(const Ref& arg, bool main_only) noexcept
{
    if (GetRefGraph(arg) == nullptr) return false;
    Assume(GetRefGraph(arg) == this);
    size_t level = GetSpecifiedLevel(main_only);
    // Make sure the transaction isn't scheduled for removal.
    ApplyRemovals(level);
    auto cluster = FindCluster(GetRefIndex(arg), level);
    return cluster != nullptr;
}

void Cluster::GetAncestorRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept
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

void Cluster::GetDescendantRefs(const TxGraphImpl& graph, std::span<std::pair<Cluster*, DepGraphIndex>>& args, std::vector<TxGraph::Ref*>& output) noexcept
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

bool Cluster::GetClusterRefs(TxGraphImpl& graph, std::span<TxGraph::Ref*> range, LinearizationIndex start_pos) noexcept
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

FeePerWeight Cluster::GetIndividualFeerate(DepGraphIndex idx) noexcept
{
    return FeePerWeight::FromFeeFrac(m_depgraph.FeeRate(idx));
}

void Cluster::MakeStagingTransactionsMissing(TxGraphImpl& graph) noexcept
{
    Assume(m_level == 1);
    // Mark all transactions of a Cluster missing, needed when aborting staging, so that the
    // corresponding Locators don't retain references into aborted Clusters.
    for (auto ci : m_linearization) {
        GraphIndex idx = m_mapping[ci];
        auto& entry = graph.m_entries[idx];
        entry.m_locator[1].SetMissing();
    }
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetAncestors(const Ref& arg, bool main_only) noexcept
{
    // Return the empty vector if the Ref is empty.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all removals and dependencies, as the result might be incorrect otherwise.
    size_t level = GetSpecifiedLevel(main_only);
    ApplyDependencies(level);
    // Ancestry cannot be known if unapplied dependencies remain.
    Assume(GetClusterSet(level).m_deps_to_add.empty());
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto cluster = FindCluster(GetRefIndex(arg), level);
    if (cluster == nullptr) return {};
    // Dispatch to the Cluster.
    std::pair<Cluster*, DepGraphIndex> match = {cluster, m_entries[GetRefIndex(arg)].m_locator[cluster->m_level].index};
    auto matches = std::span(&match, 1);
    std::vector<TxGraph::Ref*> ret;
    cluster->GetAncestorRefs(*this, matches, ret);
    return ret;
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetDescendants(const Ref& arg, bool main_only) noexcept
{
    // Return the empty vector if the Ref is empty.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all removals and dependencies, as the result might be incorrect otherwise.
    size_t level = GetSpecifiedLevel(main_only);
    ApplyDependencies(level);
    // Ancestry cannot be known if unapplied dependencies remain.
    Assume(GetClusterSet(level).m_deps_to_add.empty());
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto cluster = FindCluster(GetRefIndex(arg), level);
    if (cluster == nullptr) return {};
    // Dispatch to the Cluster.
    std::pair<Cluster*, DepGraphIndex> match = {cluster, m_entries[GetRefIndex(arg)].m_locator[cluster->m_level].index};
    auto matches = std::span(&match, 1);
    std::vector<TxGraph::Ref*> ret;
    cluster->GetDescendantRefs(*this, matches, ret);
    return ret;
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetAncestorsUnion(std::span<const Ref* const> args, bool main_only) noexcept
{
    // Apply all dependencies, as the result might be incorrect otherwise.
    size_t level = GetSpecifiedLevel(main_only);
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
        auto cluster = FindCluster(GetRefIndex(*arg), level);
        if (cluster == nullptr) continue;
        // Append to matches.
        matches.emplace_back(cluster, m_entries[GetRefIndex(*arg)].m_locator[cluster->m_level].index);
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

std::vector<TxGraph::Ref*> TxGraphImpl::GetDescendantsUnion(std::span<const Ref* const> args, bool main_only) noexcept
{
    // Apply all dependencies, as the result might be incorrect otherwise.
    size_t level = GetSpecifiedLevel(main_only);
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
        auto cluster = FindCluster(GetRefIndex(*arg), level);
        if (cluster == nullptr) continue;
        // Append to matches.
        matches.emplace_back(cluster, m_entries[GetRefIndex(*arg)].m_locator[cluster->m_level].index);
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

std::vector<TxGraph::Ref*> TxGraphImpl::GetCluster(const Ref& arg, bool main_only) noexcept
{
    // Return the empty vector if the Ref is empty (which may be indicative of the transaction
    // having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all removals and dependencies, as the result might be incorrect otherwise.
    size_t level = GetSpecifiedLevel(main_only);
    ApplyDependencies(level);
    // Cluster linearization cannot be known if unapplied dependencies remain.
    Assume(GetClusterSet(level).m_deps_to_add.empty());
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto cluster = FindCluster(GetRefIndex(arg), level);
    if (cluster == nullptr) return {};
    // Make sure the Cluster has an acceptable quality level, and then dispatch to it.
    MakeAcceptable(*cluster);
    std::vector<TxGraph::Ref*> ret(cluster->GetTxCount());
    cluster->GetClusterRefs(*this, ret, 0);
    return ret;
}

TxGraph::GraphIndex TxGraphImpl::GetTransactionCount(bool main_only) noexcept
{
    size_t level = GetSpecifiedLevel(main_only);
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
    for (int level = 0; level <= GetTopLevel(); ++level) {
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
    return cluster->GetIndividualFeerate(m_entries[GetRefIndex(arg)].m_locator[cluster->m_level].index);
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
    auto cluster = FindCluster(GetRefIndex(arg), 0);
    if (cluster == nullptr) return {};
    // Make sure the Cluster has an acceptable quality level, and then return the transaction's
    // chunk feerate.
    MakeAcceptable(*cluster);
    const auto& entry = m_entries[GetRefIndex(arg)];
    return entry.m_main_chunk_feerate;
}

bool TxGraphImpl::IsOversized(bool main_only) noexcept
{
    size_t level = GetSpecifiedLevel(main_only);
    auto& clusterset = GetClusterSet(level);
    if (clusterset.m_oversized.has_value()) {
        // Return cached value if known.
        return *clusterset.m_oversized;
    }
    // Find which Clusters will need to be merged together, as that is where the oversize
    // property is assessed.
    GroupClusters(level);
    Assume(clusterset.m_group_data.has_value());
    clusterset.m_oversized = clusterset.m_group_data->m_group_oversized;
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
    m_staging_clusterset->m_deps_to_add = m_main_clusterset.m_deps_to_add;
    m_staging_clusterset->m_group_data = m_main_clusterset.m_group_data;
    m_staging_clusterset->m_oversized = m_main_clusterset.m_oversized;
    Assume(m_staging_clusterset->m_oversized.has_value());
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
        m_main_clusterset.m_oversized = std::nullopt;
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
        conflict->Clear(*this);
        DeleteCluster(*conflict);
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
    // Delete the old staging graph, after all its information was moved to main.
    m_staging_clusterset.reset();
    Compact();
}

void Cluster::SetFee(TxGraphImpl& graph, DepGraphIndex idx, int64_t fee) noexcept
{
    // Make sure the specified DepGraphIndex exists in this Cluster.
    Assume(m_depgraph.Positions()[idx]);
    // Bail out if the fee isn't actually being changed.
    if (m_depgraph.FeeRate(idx).fee == fee) return;
    // Update the fee, remember that relinearization will be necessary, and update the Entries
    // in the same Cluster.
    m_depgraph.FeeRate(idx).fee = fee;
    if (!NeedsSplitting()) {
        graph.SetClusterQuality(m_level, m_quality, m_setindex, QualityLevel::NEEDS_RELINEARIZE);
    } else {
        graph.SetClusterQuality(m_level, m_quality, m_setindex, QualityLevel::NEEDS_SPLIT);
    }
    Updated(graph);
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
            locator.cluster->SetFee(*this, locator.index, fee);
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
    MakeAcceptable(*locator_a.cluster);
    MakeAcceptable(*locator_b.cluster);
    // Invoke comparison logic.
    return CompareMainTransactions(GetRefIndex(a), GetRefIndex(b));
}

TxGraph::GraphIndex TxGraphImpl::CountDistinctClusters(std::span<const Ref* const> refs, bool main_only) noexcept
{
    size_t level = GetSpecifiedLevel(main_only);
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

void Cluster::SanityCheck(const TxGraphImpl& graph, int level) const
{
    // There must be an m_mapping for each m_depgraph position (including holes).
    assert(m_depgraph.PositionRange() == m_mapping.size());
    // The linearization for this Cluster must contain every transaction once.
    assert(m_depgraph.TxCount() == m_linearization.size());
    // The number of transactions in a Cluster cannot exceed m_max_cluster_count.
    assert(m_linearization.size() <= graph.m_max_cluster_count);
    // The level must match the level the Cluster occurs in.
    assert(m_level == level);
    // m_quality and m_setindex are checked in TxGraphImpl::SanityCheck.

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

        // For all quality levels...
        for (int qual = 0; qual < int(QualityLevel::NONE); ++qual) {
            QualityLevel quality{qual};
            const auto& quality_clusters = clusterset.m_clusters[qual];
            // ... for all clusters in them ...
            for (ClusterSetIndex setindex = 0; setindex < quality_clusters.size(); ++setindex) {
                const auto& cluster = *quality_clusters[setindex];
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
            }
        }

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

        // If m_group_data exists, its m_group_oversized must match m_oversized.
        if (clusterset.m_group_data.has_value()) {
            assert(clusterset.m_oversized == clusterset.m_group_data->m_group_oversized);
        }

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

void TxGraphImpl::DoWork() noexcept
{
    for (int level = 0; level <= GetTopLevel(); ++level) {
        if (level > 0 || m_main_chunkindex_observers == 0) {
            MakeAllAcceptable(level);
        }
    }
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
        if (!m_excluded_clusters.contains(m_cur_cluster)) break;
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
        m_excluded_clusters.insert(m_cur_cluster);
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

} // namespace

TxGraph::Ref::~Ref()
{
    if (m_graph) {
        // Inform the TxGraph about the Ref being destroyed.
        m_graph->UnlinkRef(m_index);
        m_graph = nullptr;
    }
}

TxGraph::Ref& TxGraph::Ref::operator=(Ref&& other) noexcept
{
    // Unlink the current graph, if any.
    if (m_graph) m_graph->UnlinkRef(m_index);
    // Inform the other's graph about the move, if any.
    if (other.m_graph) other.m_graph->UpdateRef(other.m_index, *this);
    // Actually update the contents.
    m_graph = other.m_graph;
    m_index = other.m_index;
    other.m_graph = nullptr;
    other.m_index = GraphIndex(-1);
    return *this;
}

TxGraph::Ref::Ref(Ref&& other) noexcept
{
    // Inform the TxGraph of other that its Ref is being moved.
    if (other.m_graph) other.m_graph->UpdateRef(other.m_index, *this);
    // Actually move the contents.
    std::swap(m_graph, other.m_graph);
    std::swap(m_index, other.m_index);
}

std::unique_ptr<TxGraph> MakeTxGraph(unsigned max_cluster_count) noexcept
{
    return std::make_unique<TxGraphImpl>(max_cluster_count);
}
