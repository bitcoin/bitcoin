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
#include <map>
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

/** Position of a ClusterIndex within a Cluster::m_linearization. */
using LinearizationIndex = uint32_t;
/** Position of a Cluster within Graph::ClusterSet::m_clusters. */
using ClusterSetIndex = uint32_t;

/** Quality levels for cached linearizations. */
enum class QualityLevel
{
    /** This cluster may have multiple disconnected components, which are all NEEDS_RELINEARIZE. */
    NEEDS_SPLIT,
    /** This cluster may have multiple disconnected components, which are all ACCEPTABLE. */
    NEEDS_SPLIT_ACCEPTABLE,
    /** This cluster may have multiple disconnected components, which are all OPTIMAL. */
    NEEDS_SPLIT_OPTIMAL,
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
    /** m_mapping[i] gives the GraphIndex for the position i transaction in m_depgraph. */
    std::vector<GraphIndex> m_mapping;
    /** The current linearization of the cluster. Size equals m_mapping.TxCount().
     *  This is always kept topological. */
    std::vector<ClusterIndex> m_linearization;
    /** The quality level of m_linearization. */
    QualityLevel m_quality{QualityLevel::NONE};
    /** Which position this Cluster has in Graph::ClusterSet::m_clusters[m_quality]. */
    ClusterSetIndex m_setindex{ClusterSetIndex(-1)};
    /** Which level this Cluster is at in the graph (-1=not inserted, 0=main, 1=staging). */
    int m_level{-1};

public:
    /** Construct an empty Cluster. */
    Cluster() noexcept = default;
    /** Construct a singleton Cluster. */
    explicit Cluster(TxGraphImpl& graph, const FeeFrac& feerate, GraphIndex graph_index) noexcept;

    // Cannot move or copy (would invalidate Cluster* in Locator and ClusterSet). */
    Cluster(const Cluster&) = delete;
    Cluster& operator=(const Cluster&) = delete;
    Cluster(Cluster&&) = delete;
    Cluster& operator=(Cluster&&) = delete;

    // Generic helper functions.

    /** Get the number of transactions in this Cluster. */
    LinearizationIndex GetTxCount() const noexcept { return m_linearization.size(); }
    /** Given a ClusterIndex into this Cluster, find the corresponding GraphIndex. */
    GraphIndex GetClusterEntry(ClusterIndex index) const noexcept { return m_mapping[index]; }
    /** Only called by Graph::SwapIndexes. */
    void UpdateMapping(ClusterIndex cluster_idx, GraphIndex graph_idx) noexcept { m_mapping[cluster_idx] = graph_idx; }
    /** Push changes to Cluster and its linearization to the TxGraphImpl Entry objects. */
    void Updated(TxGraphImpl& graph) noexcept;
    /** Create a copy of this Cluster, returning a pointer to it (used by PullIn). */
    Cluster* CopyTo(TxGraphImpl& graph, int to_level) const noexcept;
    /** Get the list of Clusters that conflict with this one (at level-1). */
    void GetConflicts(const TxGraphImpl& graph, std::vector<Cluster*>& out) const noexcept;
    /** Mark all the Entry objects belonging to this Cluster as missing. The Cluster must be
     *  deleted immediately after. */
    void MakeTransactionsMissing(TxGraphImpl& graph) noexcept;
    /** Remove all transactions in a Cluster. */
    void Clear(TxGraphImpl& graph) noexcept;
    /** Change a Cluster's level from level to level-1. */
    void LevelDown(TxGraphImpl& graph) noexcept;

    // Functions that implement the Cluster-specific side of internal TxGraphImpl mutations.

    /** Apply any number of removals from the front of to_remove, popping them off. */
    void ApplyRemovals(TxGraphImpl& graph, std::span<GraphIndex>& to_remove) noexcept;
    /** Split this cluster (must have a NEEDS_SPLIT* quality). Returns whether to delete this
     *  Cluster afterwards. */
    [[nodiscard]] bool Split(TxGraphImpl& graph) noexcept;
    /** Move all transactions from cluster to *this (as separate components). */
    void Merge(TxGraphImpl& graph, Cluster& cluster) noexcept;
    /** Given a span of (parent, child) pairs that all belong to this Cluster (or be removed),
        apply them. */
    void ApplyDependencies(TxGraphImpl& graph, std::span<const std::pair<GraphIndex, GraphIndex>> to_apply) noexcept;
    /** Improve the linearization of this Cluster. */
    void Relinearize(TxGraphImpl& graph, uint64_t max_iters) noexcept;
    /** For every chunk in the cluster, append its FeeFrac to ret. */
    void AppendChunkFeerates(std::vector<FeeFrac>& ret) const noexcept;

    // Functions that implement the Cluster-specific side of public TxGraph functions.

    /** Get a vector of Refs for the ancestors of a given Cluster element. */
    std::vector<TxGraph::Ref*> GetAncestorRefs(const TxGraphImpl& graph, ClusterIndex idx) noexcept;
    /** Get a vector of Refs for the descendants of a given Cluster element. */
    std::vector<TxGraph::Ref*> GetDescendantRefs(const TxGraphImpl& graph, ClusterIndex idx) noexcept;
    /** Get a vector of Refs for all elements of this Cluster, in linearization order. Returns
     *  the range ends at the end of the cluster. */
    bool GetClusterRefs(TxGraphImpl& graph, std::span<TxGraph::Ref*> range, LinearizationIndex start_pos) noexcept;
    /** Get the individual transaction feerate of a Cluster element. */
    FeeFrac GetIndividualFeerate(ClusterIndex idx) noexcept;
    /** Modify the feerate of a Cluster element. */
    void SetFeerate(TxGraphImpl& graph, ClusterIndex idx, const FeeFrac& feerate) noexcept;

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
 *   (see TxGraphImpl::Cleanup).
 * - Clusters can be rewritten continuously (removals can cause them to split, new dependencies
 *   can cause them to be merged).
 * - Ref objects can be held outside the class, while permitting them to be moved around, and
 *   inherited from.
 */
class TxGraphImpl final : public TxGraph
{
    friend class Cluster;
    friend class BlockBuilderImpl;
    friend class EvictorImpl;
private:
    /** Internal RNG. */
    FastRandomContext m_rng;
    /** This TxGraphImpl's maximum cluster count limit. */
    const ClusterIndex m_max_cluster_count;

    /** The collection of all Clusters in main or staged. */
    struct ClusterSet
    {
        /** The vectors of clusters, one vector per quality level. */
        std::vector<std::unique_ptr<Cluster>> m_clusters[int(QualityLevel::NONE)];
        /** Which removals have yet to be applied. */
        std::vector<GraphIndex> m_to_remove;
        /** Which dependencies are to be added ((parent,child) pairs). */
        std::vector<std::pair<GraphIndex, GraphIndex>> m_deps_to_add;
        /** Which clusters are to be merged, if known. Each group is followed by a nullptr. */
        std::optional<std::vector<Cluster*>> m_to_merge = std::vector<Cluster*>{};
        /** Which entries were removed in this ClusterSet (so they can be wiped on abort). */
        std::vector<GraphIndex> m_removed;
        /** Total number of transactions in this ClusterSet (explicit + implicit). */
        GraphIndex m_txcount{0};
        /** Whether we know that merging clusters (as determined by m_to_merge) would exceed the max
            cluster size. */
        bool m_oversized{false};
    };

    /** The ClusterSets in this TxGraphImpl. Has exactly 1 (main) or exactly 2 elements (main and staged). */
    std::vector<ClusterSet> m_clustersets;

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

    /** Comparator for ChunkData objects in mining order. */
    class ChunkOrder
    {
        const TxGraphImpl* const m_graph;
    public:
        explicit ChunkOrder(const TxGraphImpl* graph) : m_graph(graph) {}

        bool operator()(const ChunkData& a, const ChunkData& b) const noexcept
        {
            const auto& a_entry = m_graph->m_entries[a.m_graph_index];
            const auto& b_entry = m_graph->m_entries[b.m_graph_index];
            // First sort from high feerate to low feerate.
            auto cmp_feerate = FeeRateCompare(a_entry.m_main_chunk_feerate, b_entry.m_main_chunk_feerate);
            if (cmp_feerate != 0) return cmp_feerate > 0;
            // Then sort by increasing Cluster pointer.
            Assume(a_entry.m_locator[0].IsPresent());
            Assume(b_entry.m_locator[0].IsPresent());
            if (a_entry.m_locator[0].cluster != b_entry.m_locator[0].cluster) {
                return std::less{}(a_entry.m_locator[0].cluster, b_entry.m_locator[0].cluster);
            }
            // Lastly sort by position within the Cluster.
            return a_entry.m_main_lin_index < b_entry.m_main_lin_index;
        }
    };

    /** Boost multi_index definition for the mining index. */
    using ChunkIndex = std::set<ChunkData, ChunkOrder>;

    /** Index of ChunkData objects. */
    ChunkIndex m_chunkindex;
    /** Number of index-observing objects in existence (BlockBuilderImpl, EvictorImpl). */
    size_t m_chunkindex_observers{0};
    /** Cache of discarded ChunkIndex node handles. */
    std::vector<ChunkIndex::node_type> m_chunkindex_discarded;

    /** A Locator that describes whether, where, and in which Cluster an Entry appears.
     *  Every Entry has MAX_LEVELS locators, as it may appear in one Cluster per level. */
    struct Locator
    {
        /** Which Cluster the Entry appears in (nullptr = missing). */
        Cluster* cluster{nullptr};
        /** Where in the Cluster it appears (if cluster == nullptr: 0 = missing, -1 = removed). */
        ClusterIndex index{0};

        /** Mark this Locator as missing (= same as lower level, or non-existing if level 0). */
        void SetMissing() noexcept { cluster = nullptr; index = 0; }
        /** Mark this Locator as removed (not allowed in level 0). */
        void SetRemoved() noexcept { cluster = nullptr; index = ClusterIndex(-1); }
        /** Mark this Locator as present, in the specified Cluster. */
        void SetPresent(Cluster* c, ClusterIndex i) noexcept { cluster = c; index = i; }
        /** Check if this Locator is missing. */
        bool IsMissing() const noexcept { return cluster == nullptr && index == 0; }
        /** Check if this Locator is removed. */
        bool IsRemoved() const noexcept { return cluster == nullptr && index == ClusterIndex(-1); }
        /** Check if this Locator is present (in some Cluster). */
        bool IsPresent() const noexcept { return cluster != nullptr; }
    };

    /** A class of objects held internally in TxGraphImpl, with information about a single
     *  transaction. */
    struct Entry
    {
        /** Pointer to the corresponding Ref object, or nullptr if none. */
        Ref* m_ref;
        /** Iterator to the corresponding ChunkData, if any. */
        ChunkIndex::iterator m_chunkindex_iterator;
        /** Which Cluster and position therein this Entry appears in. ([0] = main, [1] = staged). */
        Locator m_locator[MAX_LEVELS];
        /** The chunk feerate of this transaction in main (if present in m_locator[0]) */
        FeeFrac m_main_chunk_feerate;
        /** The position this transaction in the main linearization (if present). /*/
        LinearizationIndex m_main_lin_index;

        /** Check whether this Entry is not present in any Cluster. */
        bool IsWiped() const noexcept
        {
            for (int level = 0; level < MAX_LEVELS; ++level) {
                if (m_locator[level].IsPresent()) return false;
            }
            return true;
        }
    };

    /** The set of all transactions (in any level). */
    std::vector<Entry> m_entries;

    /** Set of Entries that have no IsPresent locators left, and need to be cleaned up. */
    std::vector<GraphIndex> m_wiped;

public:
    /** Construct a new TxGraphImpl with the specified maximum cluster count. */
    explicit TxGraphImpl(ClusterIndex max_cluster_count) noexcept :
        m_max_cluster_count(max_cluster_count),
        m_chunkindex(ChunkOrder(this))
    {
        Assume(max_cluster_count <= MAX_CLUSTER_COUNT_LIMIT);
        m_clustersets.reserve(MAX_LEVELS);
        m_clustersets.emplace_back();
    }

    // Cannot move or copy (would invalidate TxGraphImpl* in Ref, MiningOrder, EvictionOrder).
    TxGraphImpl(const TxGraphImpl&) = delete;
    TxGraphImpl& operator=(const TxGraphImpl&) = delete;
    TxGraphImpl(TxGraphImpl&&) = delete;
    TxGraphImpl& operator=(TxGraphImpl&&) = delete;

    // Simple helper functions.

    /** Swap the Entrys referred to by a and b. */
    void SwapIndexes(GraphIndex a, GraphIndex b) noexcept;
    /** If idx exists in the specified level ClusterSet (explicitly or implicitly), return the
     *  Cluster it is in. Otherwise, return nullptr. */
    Cluster* FindCluster(GraphIndex idx, int level) const noexcept;
    /** Extract a Cluster from its ClusterSet. */
    std::unique_ptr<Cluster> ExtractCluster(int level, QualityLevel quality, ClusterSetIndex setindex) noexcept;
    /** Delete a Cluster. */
    void DeleteCluster(Cluster& cluster) noexcept;
    /** Insert a Cluster into its ClusterSet. */
    ClusterSetIndex InsertCluster(int level, std::unique_ptr<Cluster>&& cluster, QualityLevel quality) noexcept;
    /** Change the QualityLevel of a Cluster (identified by old_quality and old_index). */
    void SetClusterQuality(int level, QualityLevel old_quality, ClusterSetIndex old_index, QualityLevel new_quality) noexcept;
    /** Make a transaction not exist at a specified level. */
    void ClearLocator(int level, GraphIndex index) noexcept;
    /** Find which Clusters conflict with the top level. */
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

    /** Only called by Ref::~Ref to unlink Refs. */
    void UnlinkRef(GraphIndex idx) noexcept final
    {
        auto& entry = m_entries[idx];
        Assume(entry.m_ref != nullptr);
        entry.m_ref = nullptr;
        if (!entry.IsWiped()) {
            for (size_t level = 0; level < m_clustersets.size(); ++level) {
                m_clustersets[level].m_to_remove.push_back(idx);
            }
        }
    }

    // Functions related to various normalization/application steps.
    /** If cluster is not in to_level, copy it there, and return a pointer to it. */
    Cluster* PullIn(Cluster* cluster, int to_level) noexcept;
    /** Apply all removals queued up in m_to_remove to the relevant Clusters (which get a
     *  NEEDS_SPLIT* QualityLevel). */
    void ApplyRemovals(int up_to_level) noexcept;
    /** Split an individual cluster. */
    void Split(Cluster& cluster) noexcept;
    /** Split all clusters that need splitting in ClusterSets up to the specified level. */
    void SplitAll(int up_to_level) noexcept;
    /** Populate the top ClusterSet's m_to_merge (and m_oversized) based on its m_deps_to_add. */
    void GroupClusters() noexcept;
    /** Merge clusters as needed by m_deps_to_add, and precomputed in m_to_merge. */
    void Merge(std::span<Cluster*> to_merge) noexcept;
    /** Apply all m_deps_to_add to the relevant Clusters. */
    void ApplyDependencies() noexcept;
    /** Make a specified Cluster have quality ACCEPTABLE or OPTIMAL. */
    void MakeAcceptable(Cluster& cluster) noexcept;
    /** Make all Clusters at the specified level have quality ACCEPTABLE or OPTIMAL. */
    void MakeAllAcceptable(int level) noexcept;

    // Implementations for the public TxGraph interface.

    Ref AddTransaction(const FeeFrac& feerate) noexcept final;
    void RemoveTransaction(Ref& arg) noexcept final;
    void AddDependency(Ref& parent, Ref& child) noexcept final;
    void SetTransactionFeerate(Ref&, const FeeFrac& feerate) noexcept final;
    std::vector<Ref*> Cleanup() noexcept final;

    void StartStaging() noexcept final;
    void CommitStaging() noexcept final;
    void AbortStaging() noexcept final;
    bool HaveStaging() const noexcept final { return m_clustersets.size() > 1; }

    bool Exists(const Ref& arg, bool main_only = false) noexcept final;
    FeeFrac GetMainChunkFeerate(const Ref& arg) noexcept final;
    FeeFrac GetIndividualFeerate(const Ref& arg) noexcept final;
    std::vector<Ref*> GetCluster(const Ref& arg, bool main_only = false) noexcept final;
    std::vector<Ref*> GetAncestors(const Ref& arg, bool main_only = false) noexcept final;
    std::vector<Ref*> GetDescendants(const Ref& arg, bool main_only = false) noexcept final;
    GraphIndex GetTransactionCount(bool main_only = false) noexcept final;
    bool IsOversized(bool main_only = false) noexcept final;
    std::strong_ordering CompareMainOrder(const Ref& a, const Ref& b) noexcept final;
    std::pair<std::vector<FeeFrac>, std::vector<FeeFrac>> GetMainStagingDiagrams() noexcept final;

    std::unique_ptr<BlockBuilder> GetBlockBuilder() noexcept final;
    std::unique_ptr<Evictor> GetEvictor() noexcept final;

    void SanityCheck() const final;
};

/** Implementation of the TxGraph::BlockBuilder interface. */
class BlockBuilderImpl final : public TxGraph::BlockBuilder
{
    /** Which TxGraphImpl this object is doing block building for. It will have its
     *  m_chunkindex_observers incremented as long as this BlockBuilderImpl exists. */
    TxGraphImpl* const m_graph;
    /** Vector for actual storage pointed to by TxGraph::BlockBuilder::m_current_chunk. */
    std::vector<TxGraph::Ref*> m_chunkdata;
    /** Which cluster the current chunk belongs to, so we can exclude further transaction from it
     *  when that chunk is skipped, or std::nullopt if we're at the end of the current cluster. */
    std::optional<Cluster*> m_remaining_cluster{nullptr};
    /** Clusters which we're not including further transactions from. */
    std::set<Cluster*> m_excluded_clusters;
    /** Iterator to the next chunk (after the current one) in the chunk index. end() if nothing
     *  further remains. */
    TxGraphImpl::ChunkIndex::const_iterator m_next_iter;

    /** Fill in information about the current chunk in m_current_chunk, m_chunkdata,
     *  m_remaining_cluster, and update m_next_iter. */
    void Next() noexcept;

public:
    /** Construct a new BlockBuilderImpl to build blocks for the provided graph. */
    BlockBuilderImpl(TxGraphImpl& graph) noexcept;

    // Implement the public interface.
    ~BlockBuilderImpl() final;
    void Include() noexcept final;
    void Skip() noexcept final;
};

/** Implementation of the TxGraph::Evictor interface. */
class EvictorImpl final : public TxGraph::Evictor
{
    /** Which TxGraphImpl this object is doing block building for. It will have its
     *  m_chunkindex_observers incremented as long as this BlockBuilderImpl exists. */
    TxGraphImpl* const m_graph;
    /** Vector for actual storage pointed to by TxGraph::BlockBuilder::m_current_chunk. */
    std::vector<TxGraph::Ref*> m_chunkdata;
    /** Iterator to the next chunk (after the current one) in the chunk index. rend() if nothing
     *  further remains. */
    TxGraphImpl::ChunkIndex::const_reverse_iterator m_next_iter;

public:
    /** Construct a new EvictorImpl for the provided graph. */
    EvictorImpl(TxGraphImpl& graph) noexcept;

    // Implement the public interface.
    ~EvictorImpl() final;
    void Next() noexcept final;
};

void TxGraphImpl::ClearChunkData(Entry& entry) noexcept
{
    if (entry.m_chunkindex_iterator != m_chunkindex.end()) {
        // If the Entry has a non-empty m_chunkindex_iterator, extract it, and move the handle
        // to the cache of discarded chunkindex entries.
        m_chunkindex_discarded.emplace_back(m_chunkindex.extract(entry.m_chunkindex_iterator));
        entry.m_chunkindex_iterator = m_chunkindex.end();
    }
}

void TxGraphImpl::CreateChunkData(GraphIndex idx, LinearizationIndex chunk_count) noexcept
{
    auto& entry = m_entries[idx];
    if (!m_chunkindex_discarded.empty()) {
        // Reuse an discarded node handle.
        auto& node = m_chunkindex_discarded.back().value();
        node.m_graph_index = idx;
        node.m_chunk_count = chunk_count;
        auto insert_result = m_chunkindex.insert(std::move(m_chunkindex_discarded.back()));
        Assume(insert_result.inserted);
        entry.m_chunkindex_iterator = insert_result.position;
        m_chunkindex_discarded.pop_back();
    } else {
        // Construct a new entry.
        auto emplace_result = m_chunkindex.emplace(idx, chunk_count);
        Assume(emplace_result.second);
        entry.m_chunkindex_iterator = emplace_result.first;
    }
}

void TxGraphImpl::ClearLocator(int level, GraphIndex idx) noexcept
{
    auto& entry = m_entries[idx];
    Assume(entry.m_locator[level].IsPresent());
    // Change the locator from Present to Missing or Removed.
    if (level == 0 || !entry.m_locator[level - 1].IsPresent()) {
        entry.m_locator[level].SetMissing();
    } else {
        entry.m_locator[level].SetRemoved();
        m_clustersets[level].m_removed.push_back(idx);
    }
    // Update the transaction count.
    --m_clustersets[level].m_txcount;
    // Adjust the status of Locators of this transaction at higher levels.
    for (size_t after_level = level + 1; after_level < m_clustersets.size(); ++after_level) {
        if (entry.m_locator[after_level].IsPresent()) {
            break;
        } else if (entry.m_locator[after_level].IsRemoved()) {
            entry.m_locator[after_level].SetMissing();
            break;
        } else {
            --m_clustersets[after_level].m_txcount;
        }
    }
    // If this was the last level the Locator was Present at, add it to the m_wiped list (which
    // will be processed by Cleanup).
    if (entry.IsWiped()) m_wiped.push_back(idx);
    if (level == 0) ClearChunkData(entry);
}

void Cluster::Updated(TxGraphImpl& graph) noexcept
{
    // Update all the Locators for this Cluster's Entrys.
    for (ClusterIndex idx : m_linearization) {
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
    if (m_level == 0 && (m_quality == QualityLevel::OPTIMAL || m_quality == QualityLevel::ACCEPTABLE)) {
        LinearizationChunking chunking(m_depgraph, m_linearization);
        LinearizationIndex lin_idx{0};
        // Iterate over the chunks.
        for (unsigned chunk_idx = 0; chunk_idx < chunking.NumChunksLeft(); ++chunk_idx) {
            auto chunk = chunking.GetChunk(chunk_idx);
            auto chunk_count = chunk.transactions.Count();
            // Iterate over the transactions in the linearization, which must match those in chunk.
            while (true) {
                ClusterIndex idx = m_linearization[lin_idx];
                GraphIndex graph_idx = m_mapping[idx];
                auto& entry = graph.m_entries[graph_idx];
                entry.m_main_lin_index = lin_idx++;
                entry.m_main_chunk_feerate = chunk.feerate;
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
    for (auto i : m_linearization) {
        auto& entry = graph.m_entries[m_mapping[i]];
        // For every transaction Entry in this Cluster, if it also exists in a lower-level Cluster,
        // then that Cluster conflicts.
        if (entry.m_locator[m_level - 1].IsPresent()) {
            out.push_back(entry.m_locator[m_level - 1].cluster);
        }
    }
}

std::vector<Cluster*> TxGraphImpl::GetConflicts() const noexcept
{
    int level = m_clustersets.size() - 1;
    std::vector<Cluster*> ret;
    // All Clusters at level-1 containing transactions in m_removed are conflicts.
    for (auto i : m_clustersets[level].m_removed) {
        auto& entry = m_entries[i];
        if (entry.IsWiped()) continue;
        Assume(entry.m_locator[level - 1].IsPresent());
        ret.push_back(entry.m_locator[level - 1].cluster);
    }
    // Then go over all Clusters at this level, and find their conflicts.
    for (int quality = 0; quality < int(QualityLevel::NONE); ++quality) {
        auto& clusters = m_clustersets[level].m_clusters[quality];
        for (const auto& cluster : clusters) {
            cluster->GetConflicts(*this, ret);
        }
    }
    // Deduplicate the result (the same Cluster may appear multiple times).
    std::sort(ret.begin(), ret.end());
    ret.erase(std::unique(ret.begin(), ret.end()), ret.end());
    return ret;
}

Cluster* Cluster::CopyTo(TxGraphImpl& graph, int to_level) const noexcept
{
    // Construct an empty Cluster.
    auto ret = std::make_unique<Cluster>();
    auto ptr = ret.get();
    // Copy depgraph, mapping, and linearization/
    ptr->m_depgraph = m_depgraph;
    ptr->m_mapping = m_mapping;
    ptr->m_linearization = m_linearization;
    // Insert the new Cluster into the graph.
    graph.InsertCluster(to_level, std::move(ret), m_quality);
    // Update its Locators (and possibly linearization data in its Entrys).
    ptr->Updated(graph);
    return ptr;
}

void Cluster::ApplyRemovals(TxGraphImpl& graph, std::span<GraphIndex>& to_remove) noexcept
{
    // Iterate over the prefix of to_remove that applies to this cluster.
    SetType todo;
    do {
        GraphIndex idx = to_remove.front();
        auto& entry = graph.m_entries[idx];
        auto& locator = entry.m_locator[m_level];
        // Stop once we hit an entry that applies to another Cluster.
        if (locator.cluster != this) break;
        // - Remember it in a set of to-remove ClusterIndexes.
        todo.Set(locator.index);
        // - Remove from m_mapping.
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
        if (quality == QualityLevel::OPTIMAL || quality == QualityLevel::NEEDS_SPLIT_OPTIMAL) {
            quality = QualityLevel::NEEDS_SPLIT_OPTIMAL;
        } else if (quality == QualityLevel::ACCEPTABLE || quality == QualityLevel::NEEDS_SPLIT_ACCEPTABLE) {
            quality = QualityLevel::NEEDS_SPLIT_ACCEPTABLE;
        } else if (quality == QualityLevel::NEEDS_RELINEARIZE) {
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

void Cluster::LevelDown(TxGraphImpl& graph) noexcept
{
    int level = m_level;
    Assume(level > 0);
    for (auto i : m_linearization) {
        GraphIndex idx = m_mapping[i];
        auto& entry = graph.m_entries[idx];
        entry.m_locator[level].SetMissing();
    }
    auto quality = m_quality;
    auto cluster = graph.ExtractCluster(level, quality, m_setindex);
    graph.InsertCluster(level - 1, std::move(cluster), quality);
    Updated(graph);
}

void Cluster::AppendChunkFeerates(std::vector<FeeFrac>& ret) const noexcept
{
    auto chunk_feerates = ChunkLinearization(m_depgraph, m_linearization);
    ret.insert(ret.end(), chunk_feerates.begin(), chunk_feerates.end());
}

bool Cluster::Split(TxGraphImpl& graph) noexcept
{
    // This function can only be called when the Cluster needs splitting.
    Assume(m_quality == QualityLevel::NEEDS_SPLIT || m_quality == QualityLevel::NEEDS_SPLIT_OPTIMAL ||
           m_quality == QualityLevel::NEEDS_SPLIT_ACCEPTABLE);
    // Determine the new quality the split-off Clusters will have.
    QualityLevel new_quality = m_quality == QualityLevel::NEEDS_SPLIT ? QualityLevel::NEEDS_RELINEARIZE :
                               m_quality == QualityLevel::NEEDS_SPLIT_OPTIMAL ? QualityLevel::OPTIMAL :
                               QualityLevel::ACCEPTABLE;
    // If the cluster was NEEDS_SPLIT_OPTIMAL, and we're thus going to produce OPTIMAL clusters, we
    // need to post-linearize first to make sure the split-out versions are all connected. This is
    // not necessary for other quality levels as they will be subject to LIMO and PostLinearization
    // anyway in MakeAcceptable().
    if (m_quality == QualityLevel::NEEDS_SPLIT_OPTIMAL) {
        PostLinearize(m_depgraph, m_linearization);
    }
    /** Which positions are still left in this Cluster. */
    auto todo = m_depgraph.Positions();
    /** Mapping from transaction positions in this Cluster to the Cluster where it ends up, and
     *  its position therein. */
    std::vector<std::pair<Cluster*, ClusterIndex>> remap(m_depgraph.PositionRange());
    std::vector<Cluster*> new_clusters;
    bool first{true};
    // Iterate over the connected components of this Cluster's m_depgraph.
    while (todo.Any()) {
        auto component = m_depgraph.FindConnectedComponent(todo);
        if (first && component == todo) {
            // The existing Cluster is an entire component. Leave it be, but update its quality.
            graph.SetClusterQuality(m_level, m_quality, m_setindex, new_quality);
            // If this made the quality ACCEPTABLE or OPTIMAL, we need to compute and cache its
            // chunking.
            Updated(graph);
            return false;
        }
        first = false;
        // Construct a new Cluster to hold the found component.
        auto new_cluster = std::make_unique<Cluster>();
        new_clusters.push_back(new_cluster.get());
        // Remember that all the component's transaction go to this new Cluster. The positions
        // will be determined below, so use -1 for now.
        for (auto i : component) {
            remap[i] = {new_cluster.get(), ClusterIndex(-1)};
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
        // ClusterIndex.
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
    std::vector<ClusterIndex> remap(other.m_depgraph.PositionRange());
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
        // Copy the transaction's dependencies, translating them using remap.
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

void Cluster::ApplyDependencies(TxGraphImpl& graph, std::span<const std::pair<GraphIndex, GraphIndex>> to_apply) noexcept
{
    // This function is invoked by TxGraphImpl::ApplyDependencies after merging groups of Clusters
    // between which dependencies are added, which simply concatenates their linearizations. Invoke
    // PostLinearize, which has the effect that the linearization becomes a merge-sort of the
    // constituent linearizations. Do this here rather than in Cluster::Merge, because this
    // function is only invoked once per merged Cluster, rather than once per constituent one.
    // This concatenation + post-linearization could be replaced with an explicit merge-sort.
    PostLinearize(m_depgraph, m_linearization);

    // Iterate over groups of to-be-added dependencies with the same child (note that to_apply
    // was sorted first by Cluster, and then by child, so all dependencies with the same child are
    // grouped together).
    auto it = to_apply.begin();
    while (it != to_apply.end()) {
        auto& first_child = graph.m_entries[it->second].m_locator[m_level];
        ClusterIndex child_idx = first_child.index;
        // Iterate over all to-be-added dependencies within that same child, gather the relevant
        // parents.
        SetType parents;
        while (it != to_apply.end()) {
            auto& child = graph.m_entries[it->second].m_locator[m_level];
            auto& parent = graph.m_entries[it->first].m_locator[m_level];
            Assume(child.cluster == this);
            // Skip parents which have already been removed.
            if (parent.IsPresent()) {
                if (child.index != child_idx) break;
                Assume(parent.cluster == this);
                parents.Set(parent.index);
            }
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

std::unique_ptr<Cluster> TxGraphImpl::ExtractCluster(int level, QualityLevel quality, ClusterSetIndex setindex) noexcept
{
    Assume(quality != QualityLevel::NONE);
    Assume(level >= 0 && size_t(level) < m_clustersets.size());

    auto& clusterset = m_clustersets[level];
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
        quality_clusters.back()->m_quality = quality;
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
    // The specified level must exist.
    Assume(level >= 0 && size_t(level) < m_clustersets.size());

    // Append it at the end of the relevant TxGraphImpl::m_cluster.
    auto& clusterset = m_clustersets[level];
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
    Assume(level >= 0 && size_t(level) < m_clustersets.size());

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
    Assume(level >= 0 && size_t(level) < m_clustersets.size());
    auto& entry = m_entries[idx];
    // Search the entry's locators from top to bottom.
    for (int l = level; l >= 0; --l) {
        // If the locator is missing, dig deeper; it may exist at a lower level.
        if (entry.m_locator[l].IsMissing()) continue;
        // If the locator has the entry marked as explicitly removed, stop.
        if (entry.m_locator[l].IsRemoved()) break;
        // Otherwise, we have found the topmost ClusterSet that contains this entry.
        return entry.m_locator[l].cluster;
    }
    // If no non-empty locator was found, or an explicitly removed was hit, return nothing.
    return nullptr;
}

Cluster* TxGraphImpl::PullIn(Cluster* cluster, int to_level) noexcept
{
    if (to_level == 0) return cluster;
    int level = cluster->m_level;
    Assume(level <= to_level);
    // Copy the Cluster from the level it was found at to higher levels, if any.
    while (level < to_level) {
        // Make the Cluster Acceptable before copying. This isn't strictly necessary, but doing it
        // now avoids doing doable work later.
        MakeAcceptable(*cluster);
        ++level;
        auto new_cluster = cluster->CopyTo(*this, level);
        cluster = new_cluster;
    }
    return cluster;
}

void TxGraphImpl::ApplyRemovals(int up_to_level) noexcept
{
    Assume(up_to_level >= 0 && size_t(up_to_level) < m_clustersets.size());
    for (int level = 0; level <= up_to_level; ++level) {
        auto& clusterset = m_clustersets[level];
        auto& to_remove = clusterset.m_to_remove;
        // Skip this level if there is nothing to remove.
        if (to_remove.empty()) continue;
        // Wipe cached m_to_merge and m_oversized, as they may be invalidated by removals.
        clusterset.m_to_merge = std::nullopt;
        if (size_t(level) == m_clustersets.size() - 1) {
            // Do not wipe the oversized state of a lower level graph (main) if a higher level
            // one (staging) exists. The reason for this is that the alternative would mean that
            // cluster merges may need to be applied to a formerly-oversized main graph while
            // staging exists (to satisfy chunk feerate queries into main, for example), and such
            // merges could conflict with pulls of some of their constituents into staging.
            clusterset.m_oversized = false;
        }
        // Pull in all Clusters that are not in the ClusterSet at level level.
        for (GraphIndex index : clusterset.m_to_remove) {
            auto cluster = FindCluster(index, level);
            if (cluster != nullptr) PullIn(cluster, level);
        }
        // Group the set of to-be-removed entries by Cluster*.
        std::sort(to_remove.begin(), to_remove.end(), [&](GraphIndex a, GraphIndex b) noexcept {
            return std::less{}(m_entries[a].m_locator[level].cluster, m_entries[b].m_locator[level].cluster);
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
                // Otherwise, skip this already-removed entry.
                to_remove_span = to_remove_span.subspan(1);
            }
        }
        clusterset.m_to_remove.clear();
    }
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
        if (entry.m_chunkindex_iterator != m_chunkindex.end()) {
            entry.m_chunkindex_iterator->m_graph_index = idx;
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

std::vector<TxGraph::Ref*> TxGraphImpl::Cleanup() noexcept
{
    // Release memory used by discarded ChunkData index entries.
    ClearShrink(m_chunkindex_discarded);

    // Don't do anything if more than 1 level exists. Cleaning up could invalidate higher levels'
    // m_to_remove, m_removed, and m_deps_to_add.
    if (m_clustersets.size() > 1) return {};

    // Apply dependencies so that this level's m_to_remove, m_removed, and m_deps_to_add are
    // empty, and oversizedness is determined.
    ApplyDependencies();
    std::vector<Ref*> ret;
    if (!m_clustersets[0].m_oversized) {
        // Sort the GraphIndex that need to be cleaned up. This groups them (so duplicates can be
        // processed just once). They are sorted in reverse, so the last ones get processed first.
        // This means earlier-processed GraphIndexes will not move of later-processed ones (which
        // might invalidate them).
        std::sort(m_wiped.begin(), m_wiped.end(), std::greater{});
        GraphIndex last = GraphIndex(-1);
        for (GraphIndex idx : m_wiped) {
            // m_wiped should never contain the same GraphIndex twice (the code below would fail
            // if so, because GraphIndexes get invalidated by removing them).
            Assume(idx != last);
            last = idx;
            Entry& entry = m_entries[idx];
            // Gather Ref pointers that are being unlinked.
            if (entry.m_ref != nullptr) {
                ret.push_back(entry.m_ref);
                GetRefGraph(*entry.m_ref) = nullptr;
                m_entries[idx].m_ref = nullptr;
            }
            // Verify removed entries don't have anything that could hold a reference back.
            Assume(entry.m_chunkindex_iterator == m_chunkindex.end());
            for (int level = 0; level < MAX_LEVELS; ++level) {
                Assume(!entry.m_locator[level].IsPresent());
            }
            if (idx != m_entries.size() - 1) SwapIndexes(idx, m_entries.size() - 1);
            m_entries.pop_back();
        }
        m_wiped.clear();
    }
    return ret;
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
    Assume(up_to_level >= 0 && size_t(up_to_level) < m_clustersets.size());
    // Before splitting all Cluster, first make sure all removals are applied.
    ApplyRemovals(up_to_level);
    for (int level = 0; level <= up_to_level; ++level) {
        for (auto quality : {QualityLevel::NEEDS_SPLIT, QualityLevel::NEEDS_SPLIT_ACCEPTABLE, QualityLevel::NEEDS_SPLIT_OPTIMAL}) {
            auto& queue = m_clustersets[level].m_clusters[int(quality)];
            while (!queue.empty()) {
                Split(*queue.back().get());
            }
        }
    }
}

void TxGraphImpl::GroupClusters() noexcept
{
    int level = m_clustersets.size() - 1;

    // Before computing which Clusters need to be merged together, first apply all removals and
    // split the Clusters into connected components. If we would group first, we might end up
    // with inefficient and/or oversized Clusters which just end up being split again anyway.
    SplitAll(level);

    auto& clusterset = m_clustersets[level];
    // If the groupings have been computed already, nothing is left to be done.
    if (clusterset.m_to_merge.has_value()) return;

    /** Union-find data structure, mapping Clusters to their representative Cluster and rank.
     *  (see https://en.wikipedia.org/wiki/Disjoint-set_data_structure). All Clusters which do not
     *  appear in this map are implicitly treated as singletons. */
    std::map<Cluster*, std::pair<Cluster*, unsigned>> uf;
    /** A simple list of all Clusters affected by added dependencies. */
    std::vector<Cluster*> to_group;

    /** UF Find operation: find (iterator to) root representative of a given Cluster. */
    auto find_uf = [&](Cluster* arg) noexcept {
        auto [it, inserted] = uf.try_emplace(arg, arg, 0);
        if (inserted) {
            // If arg was not found in the table, add it to to_group.
            to_group.push_back(arg);
        } else {
            // Otherwise, walk up towards the root. On the way, replace every parent on the path
            // with the grandparent (path splitting).
            while (it->second.first != it->first) {
                auto it_par = uf.find(it->second.first);
                Assume(it_par != uf.end());
                it->second.first = it_par->second.first;
                it = it_par;
            }
        }
        return it;
    };

    /** UF Union operation: union the partitions arg1 and arg2 are in. */
    auto union_uf = [&](Cluster* arg1, Cluster* arg2) noexcept {
        auto r1 = find_uf(arg1);
        auto r2 = find_uf(arg2);
        // Pick the lower-rank root to become a child of the higher-rank one (union by rank).
        if (r1->second.second < r2->second.second) std::swap(r1, r2);
        r2->second.first = r1->first;
        r1->second.second += (r1->second.second == r2->second.second);
    };

    /** Run through all parent/child pairs in the top ClusterSet's m_deps_to_add, and union their
     *  Clusters. */
    for (const auto& [par, chl] : clusterset.m_deps_to_add) {
        auto par_cluster = FindCluster(par, level);
        auto chl_cluster = FindCluster(chl, level);
        // Skip dependencies for which the parent or child transaction is removed.
        if (par_cluster == nullptr || chl_cluster == nullptr) continue;
        union_uf(par_cluster, chl_cluster);
    }

    // Sort the list of all to-group Clusters by their representative, grouping the ones which have
    // dependencies between them together.
    std::sort(to_group.begin(), to_group.end(), [&](Cluster* a, Cluster* b) noexcept {
        return std::less{}(find_uf(a)->second.first, find_uf(b)->second.first);
    });

    // Translate the result to the m_to_merge structure (sequences of to-group Clusters, each group
    // followed by a nullptr).
    clusterset.m_to_merge = std::vector<Cluster*>{};
    Cluster* last_rep{nullptr};
    ClusterIndex current_cluster_count{0};
    for (Cluster* arg : to_group) {
        Cluster* rep = find_uf(arg)->second.first;
        if (last_rep != nullptr && rep != last_rep) {
            // Switch over to a new group. Insert a nullptr, and reset cluster size counter.
            clusterset.m_to_merge->push_back(nullptr);
            current_cluster_count = 0;
        }
        last_rep = rep;
        // Add arg to the current group. If the result exceeds the cluster count limit, set
        // m_oversized to true.
        clusterset.m_to_merge->push_back(arg);
        current_cluster_count += arg->GetTxCount();
        if (current_cluster_count > m_max_cluster_count) clusterset.m_oversized = true;
    }
    // Terminate the last group with a nullptr.
    if (!clusterset.m_to_merge->empty()) clusterset.m_to_merge->push_back(nullptr);
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
    ClusterIndex max_size = to_merge[max_size_pos]->GetTxCount();
    for (size_t i = 1; i < to_merge.size(); ++i) {
        ClusterIndex size = to_merge[i]->GetTxCount();
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

void TxGraphImpl::ApplyDependencies() noexcept
{
    int level = m_clustersets.size() - 1;

    // Compute the groups of to-be-merged Clusters (which also applies all removals, and splits).
    GroupClusters();
    auto& clusterset = m_clustersets[level];
    Assume(clusterset.m_to_merge.has_value());
    // Nothing to do if there are no dependencies to be added.
    if (clusterset.m_deps_to_add.empty()) return;
    // Dependencies cannot be applied if it would result in oversized clusters.
    if (clusterset.m_oversized) return;

    // Pull in all the Clusters the dependencies are in.
    for (Cluster*& cluster : *clusterset.m_to_merge) {
        if (cluster != nullptr) {
            cluster = PullIn(cluster, level);
        }
    }

    // For each group of to-be-merged Clusters, invoke Merge() to merge them into a single Cluster.
    auto& to_merge = *clusterset.m_to_merge;
    auto it = to_merge.begin();
    while (it != to_merge.end()) {
        auto it_end = it + 1;
        while (it_end != to_merge.end() && *it_end != nullptr) ++it_end;
        Merge({it, it_end});
        it = it_end;
        while (it != to_merge.end() && *it == nullptr) ++it;
    }

    // Apply dependencies.

    // First sort by child Cluster, then by child GraphIndex, so we can dispatch once per child
    // Cluster, but it can process potentially multiple dependencies at once.
    std::sort(clusterset.m_deps_to_add.begin(), clusterset.m_deps_to_add.end(), [&](const auto& a, const auto& b) noexcept {
        const auto& a_loc = m_entries[a.second].m_locator[level];
        const auto& b_loc = m_entries[b.second].m_locator[level];
        bool a_missing = !a_loc.IsPresent();
        bool b_missing = !b_loc.IsPresent();
        // Move all dependencies that apply to missing children to the end.
        if (a_missing != b_missing) return b_missing;
        if (b_missing) return false;
        // Otherwise first sort by child Cluster, then by child GraphIndex.
        if (a_loc.cluster != b_loc.cluster) return std::less{}(a_loc.cluster, b_loc.cluster);
        return a.second < b.second;
    });
    // Then actually apply all to-be-added dependencies (for each, parent and child should now
    // belong to the same Cluster).
    auto app_it = clusterset.m_deps_to_add.begin();
    while (app_it != clusterset.m_deps_to_add.end()) {
        const auto& loc = m_entries[app_it->second].m_locator[level];
        if (!loc.IsPresent()) break;
        // Find the sequence of dependencies that apply to the same Cluster.
        auto app_it_end = std::next(app_it);
        while (app_it_end != clusterset.m_deps_to_add.end()) {
            const auto& loc_enc = m_entries[app_it_end->second].m_locator[level];
            if (loc_enc.cluster != loc.cluster) break;
            ++app_it_end;
        }
        // Apply them all.
        loc.cluster->ApplyDependencies(*this, {app_it, app_it_end});
        app_it = app_it_end;
    }

    // Wipe the list of to-be-added dependencies now that they are applied.
    clusterset.m_deps_to_add.clear();
    // Also no further Cluster mergings are needed (note that we clear, but don't set to
    // std::nullopt, as that would imply the groupings are unknown).
    clusterset.m_to_merge->clear();
}

void Cluster::Relinearize(TxGraphImpl& graph, uint64_t max_iters) noexcept
{
    // We can only relinearize Clusters that do not need splitting.
    Assume(m_quality == QualityLevel::OPTIMAL || m_quality == QualityLevel::ACCEPTABLE ||
           m_quality == QualityLevel::NEEDS_RELINEARIZE);
    // No work is required for Clusters which are already optimally linearized.
    if (m_quality == QualityLevel::OPTIMAL) return;
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
    if (cluster.m_quality == QualityLevel::NEEDS_RELINEARIZE) {
        cluster.Relinearize(*this, 10000);
    }
}

void TxGraphImpl::MakeAllAcceptable(int level) noexcept
{
    if (size_t(level) == m_clustersets.size() - 1) ApplyDependencies();
    auto& queue = m_clustersets[level].m_clusters[int(QualityLevel::NEEDS_RELINEARIZE)];
    while (!queue.empty()) {
        MakeAcceptable(*queue.back().get());
    }
}

Cluster::Cluster(TxGraphImpl& graph, const FeeFrac& feerate, GraphIndex graph_index) noexcept
{
    // Create a new transaction in the DepGraph, and remember its position in m_mapping.
    auto cluster_idx = m_depgraph.AddTransaction(feerate);
    m_mapping.push_back(graph_index);
    m_linearization.push_back(cluster_idx);
}

TxGraph::Ref TxGraphImpl::AddTransaction(const FeeFrac& feerate) noexcept
{
    Assume(m_chunkindex_observers == 0 || m_clustersets.size() > 1);
    // Construct a new Ref.
    Ref ret;
    // Construct a new Entry, and link it with the Ref.
    auto idx = m_entries.size();
    m_entries.emplace_back();
    auto& entry = m_entries.back();
    entry.m_chunkindex_iterator = m_chunkindex.end();
    entry.m_ref = &ret;
    GetRefGraph(ret) = this;
    GetRefIndex(ret) = idx;
    // Construct a new singleton Cluster (which is necessarily optimally linearized).
    auto cluster = std::make_unique<Cluster>(*this, feerate, idx);
    auto cluster_ptr = cluster.get();
    int level = m_clustersets.size() - 1;
    InsertCluster(level, std::move(cluster), QualityLevel::OPTIMAL);
    cluster_ptr->Updated(*this);
    ++m_clustersets[level].m_txcount;
    // Return the Ref.
    return ret;
}

void TxGraphImpl::RemoveTransaction(Ref& arg) noexcept
{
    // Don't do anything if the Ref is empty (which may be indicative of the transaction already
    // having been removed).
    if (GetRefGraph(arg) == nullptr) return;
    Assume(GetRefGraph(arg) == this);
    Assume(m_chunkindex_observers == 0 || m_clustersets.size() > 1);
    // Find the Cluster the transaction is in, and stop if it isn't in any.
    auto cluster = FindCluster(GetRefIndex(arg), m_clustersets.size() - 1);
    if (cluster == nullptr) return;
    // Remember that the transaction is to be removed.
    m_clustersets.back().m_to_remove.push_back(GetRefIndex(arg));
}

void TxGraphImpl::AddDependency(Ref& parent, Ref& child) noexcept
{
    // Don't do anything if either Ref is empty (which may be indicative of it having already been
    // removed).
    if (GetRefGraph(parent) == nullptr || GetRefGraph(child) == nullptr) return;
    Assume(GetRefGraph(parent) == this && GetRefGraph(child) == this);
    Assume(m_chunkindex_observers == 0 || m_clustersets.size() > 1);
    // Find the Cluster the parent and child transaction are in, and stop if either appears to be
    // already removed.
    auto par_cluster = FindCluster(GetRefIndex(parent), m_clustersets.size() - 1);
    if (par_cluster == nullptr) return;
    auto chl_cluster = FindCluster(GetRefIndex(child), m_clustersets.size() - 1);
    if (chl_cluster == nullptr) return;
    // Wipe m_to_merge (as it will need to be recomputed).
    m_clustersets.back().m_to_merge.reset();
    // Remember that this dependency is to be applied.
    m_clustersets.back().m_deps_to_add.emplace_back(GetRefIndex(parent), GetRefIndex(child));
}

bool TxGraphImpl::Exists(const Ref& arg, bool main_only) noexcept
{
    if (GetRefGraph(arg) == nullptr) return false;
    Assume(GetRefGraph(arg) == this);
    size_t level = main_only ? 0 : m_clustersets.size() - 1;
    // Make sure the transaction isn't scheduled for removal.
    ApplyRemovals(level);
    auto cluster = FindCluster(GetRefIndex(arg), level);
    return cluster != nullptr;
}

std::vector<TxGraph::Ref*> Cluster::GetAncestorRefs(const TxGraphImpl& graph, ClusterIndex idx) noexcept
{
    std::vector<TxGraph::Ref*> ret;
    // Translate all ancestors (in arbitrary order) to Refs (if they have any), and return them.
    for (auto idx : m_depgraph.Ancestors(idx)) {
        const auto& entry = graph.m_entries[m_mapping[idx]];
        Assume(entry.m_ref != nullptr);
        ret.push_back(entry.m_ref);
    }
    return ret;
}

std::vector<TxGraph::Ref*> Cluster::GetDescendantRefs(const TxGraphImpl& graph, ClusterIndex idx) noexcept
{
    std::vector<TxGraph::Ref*> ret;
    // Translate all descendants (in arbitrary order) to Refs (if they have any), and return them.
    for (auto idx : m_depgraph.Descendants(idx)) {
        const auto& entry = graph.m_entries[m_mapping[idx]];
        Assume(entry.m_ref != nullptr);
        ret.push_back(entry.m_ref);
    }
    return ret;
}

bool Cluster::GetClusterRefs(TxGraphImpl& graph, std::span<TxGraph::Ref*> range, LinearizationIndex start_pos) noexcept
{
    // Translate the transactions in the Cluster (in linearization order, starting at start_pos in
    // the linearization) to Refs, and fill them in range.
    for (auto& ref : range) {
        const auto& entry = graph.m_entries[m_mapping[m_linearization[start_pos++]]];
        Assume(entry.m_ref != nullptr);
        ref = entry.m_ref;
    }
    // Return whether this was the end of the Cluster.
    return start_pos == m_linearization.size();
}

FeeFrac Cluster::GetIndividualFeerate(ClusterIndex idx) noexcept
{
    return m_depgraph.FeeRate(idx);
}

void Cluster::MakeTransactionsMissing(TxGraphImpl& graph) noexcept
{
    // Mark all transactions of a Cluster missing, needed when aborting staging, so that the
    // corresponding Locators don't retain references into aborted Clusters.
    for (auto ci : m_linearization) {
        GraphIndex idx = m_mapping[ci];
        auto& entry = graph.m_entries[idx];
        entry.m_locator[m_level].SetMissing();
        if (entry.IsWiped()) graph.m_wiped.push_back(idx);
        if (m_level == 0) graph.ClearChunkData(entry);
    }
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetAncestors(const Ref& arg, bool main_only) noexcept
{
    // Return the empty vector if the Ref is empty (which may be indicative of the transaction
    // having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all dependencies, as the result might be incorrect otherwise.
    size_t level = main_only ? 0 : m_clustersets.size() - 1;
    ApplyRemovals(level);
    if (level == m_clustersets.size() - 1) ApplyDependencies();
    // Ancestry cannot be known if unapplied dependencies remain.
    Assume(!m_clustersets[level].m_oversized);
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto cluster = FindCluster(GetRefIndex(arg), level);
    if (cluster == nullptr) return {};
    // Dispatch to the Cluster.
    return cluster->GetAncestorRefs(*this, m_entries[GetRefIndex(arg)].m_locator[cluster->m_level].index);
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetDescendants(const Ref& arg, bool main_only) noexcept
{
    // Return the empty vector if the Ref is empty (which may be indicative of the transaction
    // having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all dependencies, as the result might be incorrect otherwise.
    size_t level = main_only ? 0 : m_clustersets.size() - 1;
    ApplyRemovals(level);
    if (level == m_clustersets.size() - 1) ApplyDependencies();
    // Ancestry cannot be known if unapplied dependencies remain.
    Assume(!m_clustersets[level].m_oversized);
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto cluster = FindCluster(GetRefIndex(arg), level);
    if (cluster == nullptr) return {};
    // Dispatch to the Cluster.
    return cluster->GetDescendantRefs(*this, m_entries[GetRefIndex(arg)].m_locator[cluster->m_level].index);
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetCluster(const Ref& arg, bool main_only) noexcept
{
    // Return the empty vector if the Ref is empty (which may be indicative of the transaction
    // having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all dependencies, as the result might be incorrect otherwise.
    size_t level = main_only ? 0 : m_clustersets.size() - 1;
    SplitAll(level);
    if (level == m_clustersets.size() - 1) ApplyDependencies();
    // Cluster linearization cannot be known if unapplied dependencies remain.
    Assume(!m_clustersets[level].m_oversized);
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
    size_t level = main_only ? 0 : m_clustersets.size() - 1;
    ApplyRemovals(level);
    return m_clustersets[level].m_txcount;
}

FeeFrac TxGraphImpl::GetIndividualFeerate(const Ref& arg) noexcept
{
    // Return the empty FeeFrac if the passed Ref is empty (which may be indicative of the
    // transaction having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Find the cluster the argument is in (the level does not matter as individual feerates will
    // be identical if it occurs in both), and return the empty FeeFrac if it isn't in any.
    Cluster* cluster{nullptr};
    for (int level = 0; size_t(level) < m_clustersets.size(); ++level) {
        // Apply removals, so that we can correctly report FeeFrac{} for non-existing transaction.
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

FeeFrac TxGraphImpl::GetMainChunkFeerate(const Ref& arg) noexcept
{
    // Return the empty FeeFrac if the passed Ref is empty (which may be indicative of the
    // transaction having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all dependencies, as the result might be inaccurate otherwise.
    SplitAll(0);
    if (m_clustersets.size() == 1) ApplyDependencies();
    // Chunk feerates cannot be accurately known if unapplied dependencies remain.
    Assume(!m_clustersets[0].m_oversized);
    // Find the cluster the argument is in, and return the empty FeeFrac if it isn't in any.
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
    size_t level = main_only ? 0 : m_clustersets.size() - 1;
    // Find which Clusters will need to be merged together, as that is where the oversize
    // property is assessed.
    if (level == m_clustersets.size() - 1) GroupClusters();
    return m_clustersets[level].m_oversized;
}

void TxGraphImpl::StartStaging() noexcept
{
    Assume(m_clustersets.size() < MAX_LEVELS);
    // Apply all remaining dependencies in main before creating a staging graph. Once staging
    // exists, we cannot merge Clusters anymore (because of interference with Clusters being
    // pulled into staging), so to make sure all inspectors are available (if not oversized),
    // do all merging work now. This also involves applying all removals.
    ApplyDependencies();
    // Construct a new graph.
    m_clustersets.emplace_back();
    // Copy statistics, precomputed data, and to-be-applied dependencies (only if oversized) to
    // the new graph. To-be-applied removals will always be empty at this point.
    auto& stage = m_clustersets.back();
    auto& main = *(m_clustersets.rbegin() + 1);
    stage.m_txcount = main.m_txcount;
    stage.m_deps_to_add = main.m_deps_to_add;
    stage.m_to_merge = main.m_to_merge;
    stage.m_oversized = main.m_oversized;
}

void TxGraphImpl::AbortStaging() noexcept
{
    Assume(m_clustersets.size() > 1);
    int stage_level = m_clustersets.size() - 1;
    auto& stage = m_clustersets[stage_level];
    // Mark are removed transactions as Missing (so the stage_level locator for these transactions
    // can be reused if another staging is created).
    for (auto idx : stage.m_removed) {
        m_entries[idx].m_locator[stage_level].SetMissing();
    }
    // Do the same with the non-removed transactions in staging Clusters.
    for (int quality = 0; quality < int(QualityLevel::NONE); ++quality) {
        for (auto& cluster : stage.m_clusters[quality]) {
            cluster->MakeTransactionsMissing(*this);
        }
    }
    // Destroy the staging graph data.
    m_clustersets.pop_back();
    if (!m_clustersets.back().m_to_merge.has_value()) {
        // In case m_oversized in main was kept after a Ref destruction while staging exists, we
        // need to re-evaluate m_oversized now.
        m_clustersets.back().m_oversized = false;
    }
}

void TxGraphImpl::CommitStaging() noexcept
{
    Assume(m_clustersets.size() > 1);
    int stage_level = m_clustersets.size() - 1;
    int main_level = stage_level - 1;
    auto& stage = m_clustersets[stage_level];
    auto& main = m_clustersets[main_level];
    Assume(m_chunkindex_observers == 0 || main_level > 0);
    // Delete all conflicting Clusters in main_level, to make place for moving the staging ones
    // there. All of these have been PullIn()'d to stage_level before.
    auto conflicts = GetConflicts();
    for (Cluster* conflict : conflicts) {
        conflict->Clear(*this);
        DeleteCluster(*conflict);
    }
    // Mark the removed transactions as Missing (so the stage_level locator for these transactions
    // can be reused if another staging is created0.
    for (auto idx : stage.m_removed) {
        m_entries[idx].m_locator[stage_level].SetMissing();
    }
    // Then move all Clusters in staging to main.
    for (int quality = 0; quality < int(QualityLevel::NONE); ++quality) {
        auto& stage_sets = stage.m_clusters[quality];
        while (!stage_sets.empty()) {
            stage_sets.back()->LevelDown(*this);
        }
    }
    // Move all statistics, precomputed data, and to-be-applied removals and dependencies.
    main.m_deps_to_add = std::move(stage.m_deps_to_add);
    main.m_to_remove = std::move(stage.m_to_remove);
    main.m_to_merge = std::move(stage.m_to_merge);
    main.m_oversized = std::move(stage.m_oversized);
    main.m_txcount = std::move(stage.m_txcount);
    // Delete the old staging graph, after all its information was moved to main.
    m_clustersets.pop_back();
}

void Cluster::SetFeerate(TxGraphImpl& graph, ClusterIndex idx, const FeeFrac& feerate) noexcept
{
    // Make sure the specified ClusterIndex exists in this Cluster.
    Assume(m_depgraph.Positions()[idx]);
    // Bail out if the feerate isn't actually being changed.
    if (m_depgraph.FeeRate(idx) == feerate) return;
    // Update the feerate, remember that relinearization will be necessary, and update the Entries
    // in the same Cluster.
    m_depgraph.FeeRate(idx) = feerate;
    if (m_quality == QualityLevel::OPTIMAL || m_quality == QualityLevel::ACCEPTABLE) {
        graph.SetClusterQuality(m_level, m_quality, m_setindex, QualityLevel::NEEDS_RELINEARIZE);
    } else if (m_quality == QualityLevel::NEEDS_SPLIT_OPTIMAL || m_quality == QualityLevel::NEEDS_SPLIT_ACCEPTABLE) {
        graph.SetClusterQuality(m_level, m_quality, m_setindex, QualityLevel::NEEDS_SPLIT);
    }
    Updated(graph);
}

void TxGraphImpl::SetTransactionFeerate(Ref& ref, const FeeFrac& feerate) noexcept
{
    // Return the empty FeeFrac if the passed Ref is empty (which may be indicative of the
    // transaction having been removed already.
    if (GetRefGraph(ref) == nullptr) return;
    Assume(GetRefGraph(ref) == this);
    Assume(m_chunkindex_observers == 0);
    // Find the entry, its locator, and inform its Cluster about the new feerate, if any.
    auto& entry = m_entries[GetRefIndex(ref)];
    for (int level = 0; level < MAX_LEVELS; ++level) {
        auto& locator = entry.m_locator[level];
        if (locator.IsPresent()) {
            locator.cluster->SetFeerate(*this, locator.index, feerate);
        }
    }
}

std::strong_ordering TxGraphImpl::CompareMainOrder(const Ref& a, const Ref& b) noexcept
{
    // The references must not be empty.
    Assume(GetRefGraph(a) == this);
    Assume(GetRefGraph(b) == this);
    // Apply dependencies if main is the only level (in every other case, they will have been
    // applied already prior to the creating of staging, or main is oversized).
    SplitAll(0);
    if (m_clustersets.size() == 1) ApplyDependencies();
    Assume(!m_clustersets[0].m_oversized);
    // Make both involved Clusters acceptable, so chunk feerates are relevant.
    const auto& entry_a = m_entries[GetRefIndex(a)];
    const auto& entry_b = m_entries[GetRefIndex(b)];
    const auto& locator_a = entry_a.m_locator[0];
    const auto& locator_b = entry_b.m_locator[0];
    Assume(locator_a.IsPresent());
    Assume(locator_b.IsPresent());
    MakeAcceptable(*locator_a.cluster);
    MakeAcceptable(*locator_b.cluster);
    // Compare chunk feerates, and return result if it differs.
    auto feerate_cmp = FeeRateCompare(entry_b.m_main_chunk_feerate, entry_a.m_main_chunk_feerate);
    if (feerate_cmp < 0) return std::strong_ordering::less;
    if (feerate_cmp > 0) return std::strong_ordering::greater;
    // Compare Cluster* as tie-break for equal chunk feerates.
    if (locator_a.cluster != locator_b.cluster) return locator_a.cluster <=> locator_b.cluster;
    // As final tie-break, compare position within cluster linearization.
    return entry_a.m_main_lin_index <=> entry_b.m_main_lin_index;
}

std::pair<std::vector<FeeFrac>, std::vector<FeeFrac>> TxGraphImpl::GetMainStagingDiagrams() noexcept
{
    Assume(m_clustersets.size() >= 2);
    ApplyDependencies();
    MakeAllAcceptable(m_clustersets.size() - 2);
    MakeAllAcceptable(m_clustersets.size() - 1);
    auto main_clusters = GetConflicts();
    std::vector<FeeFrac> main_feerates, staging_feerates;
    for (Cluster* cluster : main_clusters) {
        cluster->AppendChunkFeerates(main_feerates);
    }
    const auto& staging = m_clustersets.back();
    for (int quality = 0; quality < int(QualityLevel::NONE); ++quality) {
        for (const auto& cluster : staging.m_clusters[quality]) {
            cluster->AppendChunkFeerates(staging_feerates);
        }
    }
    std::sort(main_feerates.begin(), main_feerates.end(), [](auto& a, auto& b) { return FeeRateCompare(a, b) > 0; });
    std::sort(staging_feerates.begin(), staging_feerates.end(), [](auto& a, auto& b) { return FeeRateCompare(a, b) > 0; });
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
    ClusterIndex chunk_pos{0}; //!< position within the current chunk
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
        // For top-level entries, check linearization position and chunk feerate.
        if (level == 0 && (m_quality == QualityLevel::OPTIMAL || m_quality == QualityLevel::ACCEPTABLE)) {
            assert(entry.m_main_lin_index == linindex++);
            if (!linchunking.GetChunk(0).transactions[lin_pos]) {
                linchunking.MarkDone(linchunking.GetChunk(0).transactions);
                chunk_pos = 0;
            }
            assert(entry.m_main_chunk_feerate == linchunking.GetChunk(0).feerate);
            // Verify that an entry in the chunk index exists for every chunk-ending transaction.
            ++chunk_pos;
            bool is_chunk_end = (chunk_pos == linchunking.GetChunk(0).transactions.Count());
            assert((entry.m_chunkindex_iterator != graph.m_chunkindex.end()) == is_chunk_end);
            // If this Cluster has an acceptable quality level, its chunks must be connected.
            assert(m_depgraph.IsConnected(linchunking.GetChunk(0).transactions));
        }
    }
    // Verify that each element of m_depgraph occured in m_linearization.
    assert(m_done == m_depgraph.Positions());
}

void TxGraphImpl::SanityCheck() const
{
    /** Which GraphIndexes ought to occur in m_wiped, based on m_entries. */
    std::set<GraphIndex> expected_wiped;
    /** Which Clusters ought to occur in ClusterSet::m_clusters, based on m_entries. */
    std::set<const Cluster*> expected_clusters[MAX_LEVELS];
    /** Which GraphIndexes ought to occur in ClusterSet::m_removed, based on m_entries. */
    std::set<GraphIndex> expected_removed[MAX_LEVELS];
    /** Which GraphIndexes ought to occur in m_chunkindex, based on m_entries. */
    std::set<GraphIndex> expected_chunkindex;

    // Go over all Entry objects in m_entries.
    for (GraphIndex idx = 0; idx < m_entries.size(); ++idx) {
        const auto& entry = m_entries[idx];
        if (entry.IsWiped()) {
            // If the Entry is not IsPresent anywhere, it should be in m_wiped.
            expected_wiped.insert(idx);
        }
        if (entry.m_ref != nullptr) {
            // If a Ref is pointed to, it must point back to this GraphIndex in this TxGraphImpl.
            assert(GetRefGraph(*entry.m_ref) == this);
            assert(GetRefIndex(*entry.m_ref) == idx);
        }
        if (entry.m_chunkindex_iterator != m_chunkindex.end()) {
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
    for (size_t level = 0; level < m_clustersets.size(); ++level) {
        assert(level < MAX_LEVELS);
        auto& clusterset = m_clustersets[level];
        std::set<const Cluster*> actual_clusters;

        // For all quality levels...
        for (int qual = 0; qual < int(QualityLevel::NONE); ++qual) {
            QualityLevel quality{qual};
            const auto& quality_clusters = clusterset.m_clusters[qual];
            // ... for all clusters in them ...
            for (ClusterSetIndex setindex = 0; setindex < quality_clusters.size(); ++setindex) {
                const auto& cluster = *quality_clusters[setindex];
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

        // Verify that the actually encountered clusters match the ones occurring in Entry vector.
        assert(actual_clusters == expected_clusters[level]);

        // Verify that the contents of m_removed matches what was expected based on the Entry vector.
        std::set<GraphIndex> actual_removed(clusterset.m_removed.begin(), clusterset.m_removed.end());
        for (auto i : expected_wiped) {
            // If a transaction exists in both main and staging, and is removed from staging (adding
            // it to m_removed there), and consequently destroyed (wiping the locator completely),
            // it can remain in m_removed despite not having an IsRemoved() locator. Exclude those
            // transactions from the comparison here.
            actual_removed.erase(i);
            expected_removed[level].erase(i);
        }
        assert(actual_removed == expected_removed[level]);
    }

    // Verify that the contents of m_wiped matches what was expected based on the Entry vector.
    std::set<GraphIndex> actual_wiped(m_wiped.begin(), m_wiped.end());
    assert(actual_wiped == expected_wiped);

    // Finally, check the chunk index.
    std::set<GraphIndex> actual_chunkindex;
    FeeFrac last_chunk_feerate;
    for (const auto& chunk : m_chunkindex) {
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

void BlockBuilderImpl::Next() noexcept
{
    while (m_next_iter != m_graph->m_chunkindex.end()) {
        // Find the cluster pointed to by m_next_iter (and advance it).
        const auto& chunk_data = *(m_next_iter++);
        const auto& chunk_end_entry = m_graph->m_entries[chunk_data.m_graph_index];
        Cluster* cluster = chunk_end_entry.m_locator[0].cluster;
        // If we previously skipped a chunk from this cluster we cannot include more from it.
        if (m_excluded_clusters.contains(cluster)) continue;
        // Populate m_current_chunk.
        if (chunk_data.m_chunk_count == LinearizationIndex(-1)) {
            // Special case in case just a single transaction remains, avoiding the need to
            // dispatch to and dereference Cluster.
            m_chunkdata.resize(1);
            Assume(chunk_end_entry.m_ref != nullptr);
            m_chunkdata[0] = chunk_end_entry.m_ref;
            m_remaining_cluster = std::nullopt;
        } else {
           m_chunkdata.resize(chunk_data.m_chunk_count);
           auto start_pos = chunk_end_entry.m_main_lin_index + 1 - chunk_data.m_chunk_count;
           bool is_end = cluster->GetClusterRefs(*m_graph, m_chunkdata, start_pos);
           if (is_end) {
               m_remaining_cluster = std::nullopt;
           } else {
               m_remaining_cluster = cluster;
           }
        }
        m_current_chunk.emplace(m_chunkdata, chunk_end_entry.m_main_chunk_feerate);
        return;
    }
    // We reached the end of m_chunkindex.
    m_current_chunk = std::nullopt;
}

BlockBuilderImpl::BlockBuilderImpl(TxGraphImpl& graph) noexcept : m_graph(&graph)
{
    // Make sure all clusters in main are up to date, and acceptable.
    m_graph->SplitAll(0);
    if (m_graph->m_clustersets.size() == 1) m_graph->ApplyDependencies();
    m_graph->MakeAllAcceptable(0);
    // The main graph cannot be oversized, as that implies unappliable dependencies.
    Assume(!m_graph->m_clustersets[0].m_oversized);
    // Remember that this object is observing the graph's index, so that we can detect concurrent
    // modifications.
    ++m_graph->m_chunkindex_observers;
    // Find the first chunk.
    m_next_iter = m_graph->m_chunkindex.begin();
    Next();
}

BlockBuilderImpl::~BlockBuilderImpl()
{
    Assume(m_graph->m_chunkindex_observers > 0);
    // Permit modifications to the main graph again after destroying the BlockBuilderImpl.
    --m_graph->m_chunkindex_observers;
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
    // the result topologically invalid.
    if (m_remaining_cluster.has_value()) {
        m_excluded_clusters.insert(*m_remaining_cluster);
    }
    Next();
}

std::unique_ptr<TxGraph::BlockBuilder> TxGraphImpl::GetBlockBuilder() noexcept
{
    return std::make_unique<BlockBuilderImpl>(*this);
}

void EvictorImpl::Next() noexcept
{
    while (m_next_iter != m_graph->m_chunkindex.rend()) {
        // Find the cluster pointed to by m_next_iter (and advance it).
        const auto& chunk_data = *(m_next_iter++);
        const auto& chunk_end_entry = m_graph->m_entries[chunk_data.m_graph_index];
        Cluster* cluster = chunk_end_entry.m_locator[0].cluster;
        // Populate m_current_chunk.
        if (chunk_data.m_chunk_count == LinearizationIndex(-1) || chunk_data.m_chunk_count == 1) {
            // Special case for single-transaction chunks, avoiding the need to dispatch to and
            // dereference Cluster.
            m_chunkdata.resize(1);
            Assume(chunk_end_entry.m_ref != nullptr);
            m_chunkdata[0] = chunk_end_entry.m_ref;
        } else {
            m_chunkdata.resize(chunk_data.m_chunk_count);
            auto start_pos = chunk_end_entry.m_main_lin_index + 1 - chunk_data.m_chunk_count;
            cluster->GetClusterRefs(*m_graph, m_chunkdata, start_pos);
            // GetClusterRefs emits in topological order; Evictor interface expects children before
            // parents, so reverse.
            std::reverse(m_chunkdata.begin(), m_chunkdata.end());
        }
        m_current_chunk.emplace(m_chunkdata, chunk_end_entry.m_main_chunk_feerate);
        return;
    }
    // We reached the end of m_chunkindex.
    m_current_chunk = std::nullopt;
}

EvictorImpl::EvictorImpl(TxGraphImpl& graph) noexcept : m_graph(&graph)
{
    // Make sure all clusters in main are up to date, and acceptable.
    m_graph->SplitAll(0);
    if (m_graph->m_clustersets.size() == 1) m_graph->ApplyDependencies();
    m_graph->MakeAllAcceptable(0);
    // The main graph cannot be oversized, as that implies unappliable dependencies.
    Assume(!m_graph->m_clustersets[0].m_oversized);
    // Remember that this object is observing the graph's index, so that we can detect concurrent
    // modifications.
    ++m_graph->m_chunkindex_observers;
    // Find the first chunk.
    m_next_iter = m_graph->m_chunkindex.rbegin();
    Next();
}

EvictorImpl::~EvictorImpl()
{
    Assume(m_graph->m_chunkindex_observers > 0);
    // Permit modifications to the main graph again after destroying the BlockBuilderImpl.
    --m_graph->m_chunkindex_observers;
}

std::unique_ptr<TxGraph::Evictor> TxGraphImpl::GetEvictor() noexcept
{
    return std::make_unique<EvictorImpl>(*this);
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
    // Inform both TxGraphs about the Refs being swapped.
    if (m_graph) m_graph->UpdateRef(m_index,  other);
    if (other.m_graph) other.m_graph->UpdateRef(other.m_index, *this);
    // Actually swap the contents.
    std::swap(m_graph, other.m_graph);
    std::swap(m_index, other.m_index);
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
