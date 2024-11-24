// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txgraph.h>

#include <cluster_linearize.h>
#include <random.h>
#include <util/bitset.h>
#include <util/check.h>
#include <util/feefrac.h>

#include <compare>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <utility>

namespace {

using namespace cluster_linearize;

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

public:
    /** Construct an empty Cluster. */
    Cluster() noexcept = default;
    /** Construct a singleton Cluster. */
    explicit Cluster(TxGraphImpl& graph, const FeeFrac& feerate, GraphIndex graph_index) noexcept;

    // Cannot move or copy (would invalidate Cluster* in Locator and TxGraphImpl::ClusterSet). */
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

    // Functions that implement the Cluster-specific side of public TxGraph functions.

    /** Get a vector of Refs for the ancestors of a given Cluster element. */
    std::vector<TxGraph::Ref*> GetAncestorRefs(const TxGraphImpl& graph, ClusterIndex idx) noexcept;
    /** Get a vector of Refs for the descendants of a given Cluster element. */
    std::vector<TxGraph::Ref*> GetDescendantRefs(const TxGraphImpl& graph, ClusterIndex idx) noexcept;
    /** Get a vector of Refs for all elements of this Cluster, in linearization order. */
    std::vector<TxGraph::Ref*> GetClusterRefs(const TxGraphImpl& graph) noexcept;
    /** Get the individual transaction feerate of a Cluster element. */
    FeeFrac GetIndividualFeerate(ClusterIndex idx) noexcept;
    /** Modify the feerate of a Cluster element. */
    void SetFeerate(TxGraphImpl& graph, ClusterIndex idx, const FeeFrac& feerate) noexcept;

    // Debugging functions.

    void SanityCheck(const TxGraphImpl& graph) const;
};

/** The transaction graph.
 *
 * The overall design of the data structure consists of 3 interlinked representations:
 * - The transactions (held as a vector of TxGraphImpl::Entry inside TxGraphImpl).
 * - The clusters (Cluster objects in per-quality vectors inside TxGraphImpl).
 * - The Refs (TxGraph::Ref objects, held externally by users of the TxGraph class)
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
        /** Total number of transactions in this ClusterSet (explicit + implicit). */
        GraphIndex m_txcount{0};
        /** Whether we know that merging clusters (as determined by m_to_merge) would exceed the max
            cluster size. */
        bool m_oversized{false};
    };

    /** The ClusterSet for this TxGraphImpl. */
    ClusterSet m_clusterset;

    /** A Locator that describes whether, where, and in which Cluster an Entry appears. */
    struct Locator
    {
        /** Which Cluster the Entry appears in (nullptr = missing). */
        Cluster* cluster{nullptr};
        /** Where in the Cluster it appears (only if cluster != nullptr). */
        ClusterIndex index{0};

        /** Mark this Locator as missing. */
        void SetMissing() noexcept { cluster = nullptr; index = 0; }
        /** Mark this Locator as present, in the specified Cluster. */
        void SetPresent(Cluster* c, ClusterIndex i) noexcept { cluster = c; index = i; }
        /** Check if this Locator is missing. */
        bool IsMissing() const noexcept { return cluster == nullptr && index == 0; }
        /** Check if this Locator is present (in some Cluster). */
        bool IsPresent() const noexcept { return cluster != nullptr; }
    };

    /** A class of objects held internally in TxGraphImpl, with information about a single
     *  transaction. */
    struct Entry
    {
        /** Pointer to the corresponding Ref object, if any. */
        Ref* m_ref;
        /** Which Cluster and position therein this Entry appears in. */
        Locator m_locator;
        /** The chunk feerate of this transaction (if not missing) */
        FeeFrac m_chunk_feerate;

        /** Check whether this Entry is not present in any Cluster. */
        bool IsWiped() const noexcept
        {
            return !m_locator.IsPresent();
        }
    };

    /** The set of all transactions. */
    std::vector<Entry> m_entries;

    /** Set of Entries that have no IsPresent locators left, and need to be cleaned up. */
    std::vector<GraphIndex> m_wiped;

public:
    /** Construct a new TxGraphImpl with the specified maximum cluster count. */
    explicit TxGraphImpl(ClusterIndex max_cluster_count) noexcept :
        m_max_cluster_count(max_cluster_count)
    {
        Assume(max_cluster_count <= MAX_CLUSTER_COUNT_LIMIT);
    }

    // Cannot move or copy (would invalidate TxGraphImpl* in Ref, MiningOrder, EvictionOrder).
    TxGraphImpl(const TxGraphImpl&) = delete;
    TxGraphImpl& operator=(const TxGraphImpl&) = delete;
    TxGraphImpl(TxGraphImpl&&) = delete;
    TxGraphImpl& operator=(TxGraphImpl&&) = delete;

    // Simple helper functions.

    /** Swap the Entrys referred to by a and b. */
    void SwapIndexes(GraphIndex a, GraphIndex b) noexcept;
    /** Extract a Cluster. */
    std::unique_ptr<Cluster> ExtractCluster(QualityLevel quality, ClusterSetIndex setindex) noexcept;
    /** Delete a Cluster. */
    void DeleteCluster(Cluster& cluster) noexcept;
    /** Insert a Cluster. */
    ClusterSetIndex InsertCluster(std::unique_ptr<Cluster>&& cluster, QualityLevel quality) noexcept;
    /** Change the QualityLevel of a Cluster (identified by old_quality and old_index). */
    void SetClusterQuality(QualityLevel old_quality, ClusterSetIndex old_index, QualityLevel new_quality) noexcept;
    /** Make a transaction not exist. */
    void ClearLocator(GraphIndex index) noexcept;

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
    }

    // Functions related to various normalization/application steps.
    /** Apply all removals queued up in m_to_remove to the relevant Clusters (which get a
     *  NEEDS_SPLIT* QualityLevel). */
    void ApplyRemovals() noexcept;
    /** Split an individual cluster. */
    void Split(Cluster& cluster) noexcept;
    /** Split all clusters that need splitting. */
    void SplitAll() noexcept;
    /** Populate m_to_merge (and m_oversized) based on m_deps_to_add. */
    void GroupClusters() noexcept;
    /** Merge clusters as needed by m_deps_to_add, and precomputed in m_to_merge. */
    void Merge(std::span<Cluster*> to_merge) noexcept;
    /** Apply all m_deps_to_add to the relevant Clusters. */
    void ApplyDependencies() noexcept;
    /** Make a specified Cluster have quality ACCEPTABLE or OPTIMAL. */
    void MakeAcceptable(Cluster& cluster) noexcept;

    // Implementations for the public TxGraph interface.

    Ref AddTransaction(const FeeFrac& feerate) noexcept final;
    void RemoveTransaction(Ref& arg) noexcept final;
    void AddDependency(Ref& parent, Ref& child) noexcept final;
    void SetTransactionFeerate(Ref&, const FeeFrac& feerate) noexcept final;
    std::vector<Ref*> Cleanup() noexcept final;

    bool Exists(const Ref& arg) noexcept final;
    FeeFrac GetChunkFeerate(const Ref& arg) noexcept final;
    FeeFrac GetIndividualFeerate(const Ref& arg) noexcept final;
    std::vector<Ref*> GetCluster(const Ref& arg) noexcept final;
    std::vector<Ref*> GetAncestors(const Ref& arg) noexcept final;
    std::vector<Ref*> GetDescendants(const Ref& arg) noexcept final;
    GraphIndex GetTransactionCount() noexcept final;
    bool IsOversized() noexcept final;

    void SanityCheck() const final;
};

void TxGraphImpl::ClearLocator(GraphIndex idx) noexcept
{
    auto& entry = m_entries[idx];
    Assume(entry.m_locator.IsPresent());
    // Change the locator from Present to Missing.
    entry.m_locator.SetMissing();
    // Update the transaction count.
    --m_clusterset.m_txcount;
    // Add it to the m_wiped list (which will be processed by Cleanup).
    if (entry.IsWiped()) m_wiped.push_back(idx);
}

void Cluster::Updated(TxGraphImpl& graph) noexcept
{
    // Update all the Locators for this Cluster's Entrys.
    for (ClusterIndex idx : m_linearization) {
        auto& entry = graph.m_entries[m_mapping[idx]];
        entry.m_locator.SetPresent(this, idx);
    }
    // If the Cluster's quality is ACCEPTABLE or OPTIMAL, compute its chunking and store its
    // information in the Entry's m_chunk_feerate. These fields are only accessed after making
    // the entire graph ACCEPTABLE, so it is pointless to compute these if we haven't reached that
    // quality level yet.
    if (m_quality == QualityLevel::OPTIMAL || m_quality == QualityLevel::ACCEPTABLE) {
        LinearizationChunking chunking(m_depgraph, m_linearization);
        LinearizationIndex lin_idx{0};
        // Iterate over the chunks.
        for (unsigned chunk_idx = 0; chunk_idx < chunking.NumChunksLeft(); ++chunk_idx) {
            auto chunk = chunking.GetChunk(chunk_idx);
            // Iterate over the transactions in the linearization, which must match those in chunk.
            while (true) {
                ClusterIndex idx = m_linearization[lin_idx++];
                GraphIndex graph_idx = m_mapping[idx];
                auto& entry = graph.m_entries[graph_idx];
                entry.m_chunk_feerate = chunk.feerate;
                chunk.transactions.Reset(idx);
                if (chunk.transactions.None()) break;
            }
        }
    }
}

void Cluster::ApplyRemovals(TxGraphImpl& graph, std::span<GraphIndex>& to_remove) noexcept
{
    // Iterate over the prefix of to_remove that applies to this cluster.
    SetType todo;
    do {
        GraphIndex idx = to_remove.front();
        auto& entry = graph.m_entries[idx];
        auto& locator = entry.m_locator;
        // Stop once we hit an entry that applies to another Cluster.
        if (locator.cluster != this) break;
        // - Remember it in a set of to-remove ClusterIndexes.
        todo.Set(locator.index);
        // - Remove from m_mapping.
        m_mapping[locator.index] = GraphIndex(-1);
        // - Mark it as removed in the Entry's locator.
        graph.ClearLocator(idx);
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
    graph.SetClusterQuality(m_quality, m_setindex, quality);
    Updated(graph);
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
            graph.SetClusterQuality(m_quality, m_setindex, new_quality);
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
        graph.InsertCluster(std::move(new_cluster), new_quality);
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
        graph.m_entries[idx].m_locator.SetPresent(this, new_pos);
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
        auto& first_child = graph.m_entries[it->second].m_locator;
        ClusterIndex child_idx = first_child.index;
        // Iterate over all to-be-added dependencies within that same child, gather the relevant
        // parents.
        SetType parents;
        while (it != to_apply.end()) {
            auto& child = graph.m_entries[it->second].m_locator;
            auto& parent = graph.m_entries[it->first].m_locator;
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

std::unique_ptr<Cluster> TxGraphImpl::ExtractCluster(QualityLevel quality, ClusterSetIndex setindex) noexcept
{
    Assume(quality != QualityLevel::NONE);

    auto& quality_clusters = m_clusterset.m_clusters[int(quality)];
    Assume(setindex < quality_clusters.size());

    // Extract the Cluster-owning unique_ptr.
    std::unique_ptr<Cluster> ret = std::move(quality_clusters[setindex]);
    ret->m_quality = QualityLevel::NONE;
    ret->m_setindex = ClusterSetIndex(-1);

    // Clean up space in quality_cluster.
    auto max_setindex = quality_clusters.size() - 1;
    if (setindex != max_setindex) {
        // If the cluster was not the last element of quality_clusters, move that to take its place.
        quality_clusters.back()->m_quality = quality;
        quality_clusters.back()->m_setindex = setindex;
        quality_clusters[setindex] = std::move(quality_clusters.back());
    }
    // The last element of quality_clusters is now unused; drop it.
    quality_clusters.pop_back();

    return ret;
}

ClusterSetIndex TxGraphImpl::InsertCluster(std::unique_ptr<Cluster>&& cluster, QualityLevel quality) noexcept
{
    // Cannot insert with quality level NONE (as that would mean not inserted).
    Assume(quality != QualityLevel::NONE);
    // The passed-in Cluster must not currently be in the TxGraphImpl.
    Assume(cluster->m_quality == QualityLevel::NONE);

    // Append it at the end of the relevant TxGraphImpl::m_cluster.
    auto& quality_clusters = m_clusterset.m_clusters[int(quality)];
    ClusterSetIndex ret = quality_clusters.size();
    cluster->m_quality = quality;
    cluster->m_setindex = ret;
    quality_clusters.push_back(std::move(cluster));
    return ret;
}

void TxGraphImpl::SetClusterQuality(QualityLevel old_quality, ClusterSetIndex old_index, QualityLevel new_quality) noexcept
{
    Assume(new_quality != QualityLevel::NONE);

    // Don't do anything if the quality did not change.
    if (old_quality == new_quality) return;
    // Extract the cluster from where it currently resides.
    auto cluster_ptr = ExtractCluster(old_quality, old_index);
    // And re-insert it where it belongs.
    InsertCluster(std::move(cluster_ptr), new_quality);
}

void TxGraphImpl::DeleteCluster(Cluster& cluster) noexcept
{
    // Extract the cluster from where it currently resides.
    auto cluster_ptr = ExtractCluster(cluster.m_quality, cluster.m_setindex);
    // And throw it away.
    cluster_ptr.reset();
}

void TxGraphImpl::ApplyRemovals() noexcept
{
    auto& clusterset = m_clusterset;
    auto& to_remove = clusterset.m_to_remove;
    // Stop if there is nothing to remove.
    if (to_remove.empty()) return;
    // Wipe cached m_to_merge and m_oversized, as they may be invalidated by removals.
    clusterset.m_to_merge = std::nullopt;
    clusterset.m_oversized = false;
    // Group the set of to-be-removed entries by Cluster*.
    std::sort(to_remove.begin(), to_remove.end(), [&](GraphIndex a, GraphIndex b) noexcept {
        return std::less{}(m_entries[a].m_locator.cluster, m_entries[b].m_locator.cluster);
    });
    // Process per Cluster.
    std::span to_remove_span{to_remove};
    while (!to_remove_span.empty()) {
        Cluster* cluster = m_entries[to_remove_span.front()].m_locator.cluster;
        if (cluster != nullptr) {
            // If the first to_remove_span entry's Cluster exists, hand to_remove_span to it, so it
            // can pop off whatever applies to it.
            cluster->ApplyRemovals(*this, to_remove_span);
        } else {
            // Otherwise, skip this already-removed entry.
            to_remove_span = to_remove_span.subspan(1);
        }
    }
    to_remove.clear();
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
        // Update linked Ref.
        if (entry.m_ref) GetRefIndex(*entry.m_ref) = idx;
        // Update the locator. The rest of the Entry information will not change, so no need to
        // invoke Cluster::Updated().
        Locator& locator = entry.m_locator;
        if (locator.IsPresent()) {
            locator.cluster->UpdateMapping(locator.index, idx);
        }
    }
}

std::vector<TxGraph::Ref*> TxGraphImpl::Cleanup() noexcept
{
    ApplyDependencies();
    std::vector<Ref*> ret;
    if (!m_clusterset.m_oversized) {
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
            Assume(!entry.m_locator.IsPresent());
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
    ApplyRemovals();
    bool del = cluster.Split(*this);
    if (del) {
        // Cluster::Split reports whether the Cluster is to be deleted.
        DeleteCluster(cluster);
    }
}

void TxGraphImpl::SplitAll() noexcept
{
    // Before splitting all Cluster, first make sure all removals are applied.
    ApplyRemovals();
    for (auto quality : {QualityLevel::NEEDS_SPLIT, QualityLevel::NEEDS_SPLIT_ACCEPTABLE, QualityLevel::NEEDS_SPLIT_OPTIMAL}) {
        auto& queue = m_clusterset.m_clusters[int(quality)];
        while (!queue.empty()) {
            Split(*queue.back().get());
        }
    }
}

void TxGraphImpl::GroupClusters() noexcept
{
    // Before computing which Clusters need to be merged together, first apply all removals and
    // split the Clusters into connected components. If we would group first, we might end up
    // with inefficient and/or oversized Clusters which just end up being split again anyway.
    SplitAll();

    auto& clusterset = m_clusterset;
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

    /** Run through all parent/child pairs in m_deps_to_add, and union their Clusters. */
    for (const auto& [par, chl] : clusterset.m_deps_to_add) {
        auto par_cluster = m_entries[par].m_locator.cluster;
        auto chl_cluster = m_entries[chl].m_locator.cluster;
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
    // Compute the groups of to-be-merged Clusters (which also applies all removals, and splits).
    GroupClusters();
    auto& clusterset = m_clusterset;
    Assume(clusterset.m_to_merge.has_value());
    // Nothing to do if there are no dependencies to be added.
    if (clusterset.m_deps_to_add.empty()) return;
    // Dependencies cannot be applied if it would result in oversized clusters.
    if (clusterset.m_oversized) return;

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
        const auto& a_loc = m_entries[a.second].m_locator;
        const auto& b_loc = m_entries[b.second].m_locator;
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
        const auto& loc = m_entries[app_it->second].m_locator;
        if (!loc.IsPresent()) break;
        // Find the sequence of dependencies that apply to the same Cluster.
        auto app_it_end = std::next(app_it);
        while (app_it_end != clusterset.m_deps_to_add.end()) {
            const auto& loc_enc = m_entries[app_it_end->second].m_locator;
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
    graph.SetClusterQuality(m_quality, m_setindex, new_quality);
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

Cluster::Cluster(TxGraphImpl& graph, const FeeFrac& feerate, GraphIndex graph_index) noexcept
{
    // Create a new transaction in the DepGraph, and remember its position in m_mapping.
    auto cluster_idx = m_depgraph.AddTransaction(feerate);
    m_mapping.push_back(graph_index);
    m_linearization.push_back(cluster_idx);
}

TxGraph::Ref TxGraphImpl::AddTransaction(const FeeFrac& feerate) noexcept
{
    // Construct a new Ref.
    Ref ret;
    // Construct a new Entry, and link it with the Ref.
    auto idx = m_entries.size();
    m_entries.emplace_back();
    auto& entry = m_entries.back();
    entry.m_ref = &ret;
    GetRefGraph(ret) = this;
    GetRefIndex(ret) = idx;
    // Construct a new singleton Cluster (which is necessarily optimally linearized).
    auto cluster = std::make_unique<Cluster>(*this, feerate, idx);
    auto cluster_ptr = cluster.get();
    InsertCluster(std::move(cluster), QualityLevel::OPTIMAL);
    cluster_ptr->Updated(*this);
    ++m_clusterset.m_txcount;
    // Return the Ref.
    return ret;
}

void TxGraphImpl::RemoveTransaction(Ref& arg) noexcept
{
    // Don't do anything if the Ref is empty (which may be indicative of the transaction already
    // having been removed).
    if (GetRefGraph(arg) == nullptr) return;
    Assume(GetRefGraph(arg) == this);
    // Find the Cluster the transaction is in, and stop if it isn't in any.
    auto cluster = m_entries[GetRefIndex(arg)].m_locator.cluster;
    if (cluster == nullptr) return;
    // Remember that the transaction is to be removed.
    m_clusterset.m_to_remove.push_back(GetRefIndex(arg));
}

void TxGraphImpl::AddDependency(Ref& parent, Ref& child) noexcept
{
    // Don't do anything if either Ref is empty (which may be indicative of it having already been
    // removed).
    if (GetRefGraph(parent) == nullptr || GetRefGraph(child) == nullptr) return;
    Assume(GetRefGraph(parent) == this && GetRefGraph(child) == this);
    // Find the Cluster the parent and child transaction are in, and stop if either appears to be
    // already removed.
    auto par_cluster = m_entries[GetRefIndex(parent)].m_locator.cluster;
    if (par_cluster == nullptr) return;
    auto chl_cluster = m_entries[GetRefIndex(child)].m_locator.cluster;
    if (chl_cluster == nullptr) return;
    // Wipe m_to_merge (as it will need to be recomputed).
    m_clusterset.m_to_merge.reset();
    // Remember that this dependency is to be applied.
    m_clusterset.m_deps_to_add.emplace_back(GetRefIndex(parent), GetRefIndex(child));
}

bool TxGraphImpl::Exists(const Ref& arg) noexcept
{
    if (GetRefGraph(arg) == nullptr) return false;
    Assume(GetRefGraph(arg) == this);
    // Make sure the transaction isn't scheduled for removal.
    ApplyRemovals();
    return m_entries[GetRefIndex(arg)].m_locator.IsPresent();
}

std::vector<TxGraph::Ref*> Cluster::GetAncestorRefs(const TxGraphImpl& graph, ClusterIndex idx) noexcept
{
    std::vector<TxGraph::Ref*> ret;
    // Translate all ancestors (in arbitrary order) to Refs (if they have any), and return them.
    for (auto idx : m_depgraph.Ancestors(idx)) {
        const auto& entry = graph.m_entries[m_mapping[idx]];
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
        ret.push_back(entry.m_ref);
    }
    return ret;
}

std::vector<TxGraph::Ref*> Cluster::GetClusterRefs(const TxGraphImpl& graph) noexcept
{
    std::vector<TxGraph::Ref*> ret;
    // Translate all transactions in the Cluster (in linearization order) to Refs.
    for (auto idx : m_linearization) {
        const auto& entry = graph.m_entries[m_mapping[idx]];
        ret.push_back(entry.m_ref);
    }
    return ret;
}

FeeFrac Cluster::GetIndividualFeerate(ClusterIndex idx) noexcept
{
    return m_depgraph.FeeRate(idx);
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetAncestors(const Ref& arg) noexcept
{
    // Return the empty vector if the Ref is empty (which may be indicative of the transaction
    // having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all dependencies, as the result might be incorrect otherwise.
    ApplyDependencies();
    // Ancestry cannot be known if unapplied dependencies remain.
    Assume(!m_clusterset.m_oversized);
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto cluster = m_entries[GetRefIndex(arg)].m_locator.cluster;
    if (cluster == nullptr) return {};
    // Dispatch to the Cluster.
    return cluster->GetAncestorRefs(*this, m_entries[GetRefIndex(arg)].m_locator.index);
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetDescendants(const Ref& arg) noexcept
{
    // Return the empty vector if the Ref is empty (which may be indicative of the transaction
    // having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all dependencies, as the result might be incorrect otherwise.
    ApplyDependencies();
    // Ancestry cannot be known if unapplied dependencies remain.
    Assume(!m_clusterset.m_oversized);
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto cluster = m_entries[GetRefIndex(arg)].m_locator.cluster;
    if (cluster == nullptr) return {};
    // Dispatch to the Cluster.
    return cluster->GetDescendantRefs(*this, m_entries[GetRefIndex(arg)].m_locator.index);
}

std::vector<TxGraph::Ref*> TxGraphImpl::GetCluster(const Ref& arg) noexcept
{
    // Return the empty vector if the Ref is empty (which may be indicative of the transaction
    // having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all dependencies, as the result might be incorrect otherwise.
    ApplyDependencies();
    // Cluster linearization cannot be known if unapplied dependencies remain.
    Assume(!m_clusterset.m_oversized);
    // Find the Cluster the argument is in, and return the empty vector if it isn't in any.
    auto cluster = m_entries[GetRefIndex(arg)].m_locator.cluster;
    if (cluster == nullptr) return {};
    // Make sure the Cluster has an acceptable quality level, and then dispatch to it.
    MakeAcceptable(*cluster);
    return cluster->GetClusterRefs(*this);
}

TxGraph::GraphIndex TxGraphImpl::GetTransactionCount() noexcept
{
    ApplyRemovals();
    return m_clusterset.m_txcount;
}

FeeFrac TxGraphImpl::GetIndividualFeerate(const Ref& arg) noexcept
{
    // Return the empty FeeFrac if the passed Ref is empty (which may be indicative of the
    // transaction having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply removals, so that we can correctly report FeeFrac{} for non-existing transaction.
    ApplyRemovals();
    // Find the cluster the argument is in, and return the empty FeeFrac if it isn't in any.
    auto cluster = m_entries[GetRefIndex(arg)].m_locator.cluster;
    if (cluster == nullptr) return {};
    // Dispatch to the Cluster.
    return cluster->GetIndividualFeerate(m_entries[GetRefIndex(arg)].m_locator.index);
}

FeeFrac TxGraphImpl::GetChunkFeerate(const Ref& arg) noexcept
{
    // Return the empty FeeFrac if the passed Ref is empty (which may be indicative of the
    // transaction having been removed already.
    if (GetRefGraph(arg) == nullptr) return {};
    Assume(GetRefGraph(arg) == this);
    // Apply all dependencies, as the result might be inaccurate otherwise.
    ApplyDependencies();
    // Chunk feerates cannot be accurately known if unapplied dependencies remain.
    Assume(!m_clusterset.m_oversized);
    // Find the cluster the argument is in, and return the empty FeeFrac if it isn't in any.
    auto cluster = m_entries[GetRefIndex(arg)].m_locator.cluster;
    if (cluster == nullptr) return {};
    // Make sure the Cluster has an acceptable quality level, and then return the transaction's
    // chunk feerate.
    MakeAcceptable(*cluster);
    const auto& entry = m_entries[GetRefIndex(arg)];
    return entry.m_chunk_feerate;
}

bool TxGraphImpl::IsOversized() noexcept
{
    // Find which Clusters will need to be merged together, as that is where the oversize
    // property is assessed.
    GroupClusters();
    return m_clusterset.m_oversized;
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
        graph.SetClusterQuality(m_quality, m_setindex, QualityLevel::NEEDS_RELINEARIZE);
    } else if (m_quality == QualityLevel::NEEDS_SPLIT_OPTIMAL || m_quality == QualityLevel::NEEDS_SPLIT_ACCEPTABLE) {
        graph.SetClusterQuality(m_quality, m_setindex, QualityLevel::NEEDS_SPLIT);
    }
    Updated(graph);
}

void TxGraphImpl::SetTransactionFeerate(Ref& ref, const FeeFrac& feerate) noexcept
{
    // Return the empty FeeFrac if the passed Ref is empty (which may be indicative of the
    // transaction having been removed already.
    if (GetRefGraph(ref) == nullptr) return;
    Assume(GetRefGraph(ref) == this);
    // Find the entry, its locator, and inform its Cluster about the new feerate, if any.
    auto& entry = m_entries[GetRefIndex(ref)];
    auto& locator = entry.m_locator;
    if (locator.IsPresent()) {
        locator.cluster->SetFeerate(*this, locator.index, feerate);
    }
}

void Cluster::SanityCheck(const TxGraphImpl& graph) const
{
    // There must be an m_mapping for each m_depgraph position (including holes).
    assert(m_depgraph.PositionRange() == m_mapping.size());
    // The linearization for this Cluster must contain every transaction once.
    assert(m_depgraph.TxCount() == m_linearization.size());
    // The number of transactions in a Cluster cannot exceed m_max_cluster_count.
    assert(m_linearization.size() <= graph.m_max_cluster_count);
    // m_quality and m_setindex are checked in TxGraphImpl::SanityCheck.

    // Compute the chunking of m_linearization.
    LinearizationChunking linchunking(m_depgraph, m_linearization);

    // Verify m_linearization.
    SetType m_done;
    assert(m_depgraph.IsAcyclic());
    for (auto lin_pos : m_linearization) {
        assert(lin_pos < m_mapping.size());
        const auto& entry = graph.m_entries[m_mapping[lin_pos]];
        // Check that the linearization is topological.
        m_done.Set(lin_pos);
        assert(m_done.IsSupersetOf(m_depgraph.Ancestors(lin_pos)));
        // Check that the Entry has a locator pointing back to this Cluster & position within it.
        assert(entry.m_locator.cluster == this);
        assert(entry.m_locator.index == lin_pos);
        // Check linearization position and chunk feerate.
        if (m_quality == QualityLevel::OPTIMAL || m_quality == QualityLevel::ACCEPTABLE) {
            if (!linchunking.GetChunk(0).transactions[lin_pos]) {
                linchunking.MarkDone(linchunking.GetChunk(0).transactions);
            }
            assert(entry.m_chunk_feerate == linchunking.GetChunk(0).feerate);
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
    /** Which Clusters ought to occur in m_clusters, based on m_entries. */
    std::set<const Cluster*> expected_clusters;

    // Go over all Entry objects in m_entries.
    for (GraphIndex idx = 0; idx < m_entries.size(); ++idx) {
        const auto& entry = m_entries[idx];
        if (entry.IsWiped()) {
            // If the Entry is not IsPresent anywhere, it should be in m_wiped.
            expected_wiped.insert(idx);
        } else {
            // Every non-wiped Entry must have a Ref that points back to it.
            assert(entry.m_ref != nullptr);
            assert(GetRefGraph(*entry.m_ref) == this);
            assert(GetRefIndex(*entry.m_ref) == idx);
        }
        // Verify the Entry m_locator.
        const auto& locator = entry.m_locator;
        // Every Locator must be in exactly one of these 2 states.
        assert(locator.IsMissing() + locator.IsPresent() == 1);
        if (locator.IsPresent()) {
            // Verify that the Cluster agrees with where the Locator claims the transaction is.
            assert(locator.cluster->GetClusterEntry(locator.index) == idx);
            // Remember that we expect said Cluster to appear in the m_clusters.
            expected_clusters.insert(locator.cluster);
        }

    }

    auto& clusterset = m_clusterset;
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
            cluster.SanityCheck(*this);
            // Check that the cluster's quality and setindex matches its position in the quality list.
            assert(cluster.m_quality == quality);
            assert(cluster.m_setindex == setindex);
        }
    }

    // Verify that the actually encountered clusters match the ones occurring in Entry vector.
    assert(actual_clusters == expected_clusters);

    // Verify that the contents of m_wiped matches what was expected based on the Entry vector.
    std::set<GraphIndex> actual_wiped(m_wiped.begin(), m_wiped.end());
    assert(actual_wiped == expected_wiped);
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
