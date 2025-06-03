// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compare>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <util/feefrac.h>

#ifndef BITCOIN_TXGRAPH_H
#define BITCOIN_TXGRAPH_H

static constexpr unsigned MAX_CLUSTER_COUNT_LIMIT{64};

/** Data structure to encapsulate fees, sizes, and dependencies for a set of transactions.
 *
 * Each TxGraph represents one or two such graphs ("main", and optionally "staging"), to allow for
 * working with batches of changes that may still be discarded.
 *
 * The connected components within each transaction graph are called clusters: whenever one
 * transaction is reachable from another, through any sequence of is-parent-of or is-child-of
 * relations, they belong to the same cluster (so clusters include parents, children, but also
 * grandparents, siblings, cousins twice removed, ...).
 *
 * For each graph, TxGraph implicitly defines an associated total ordering on its transactions
 * (its linearization) that respects topology (parents go before their children), aiming for it to
 * be close to the optimal order those transactions should be mined in if the goal is fee
 * maximization, though this is a best effort only, not a strong guarantee.
 *
 * For more explanation, see https://delvingbitcoin.org/t/introduction-to-cluster-linearization/1032
 *
 * This linearization is partitioned into chunks: groups of transactions that according to this
 * order would be mined together. Each chunk consists of the highest-feerate prefix of what remains
 * of the linearization after removing previous chunks. TxGraph guarantees that the maintained
 * linearization always results in chunks consisting of transactions that are connected. A chunk's
 * transactions always belong to the same cluster.
 *
 * The interface is designed to accommodate an implementation that only stores the transitive
 * closure of dependencies, so if B spends C, it does not distinguish between "A spending B" and
 * "A spending both B and C".
 */
class TxGraph
{
public:
    /** Internal identifier for a transaction within a TxGraph. */
    using GraphIndex = uint32_t;

    /** Data type used to reference transactions within a TxGraph.
     *
     * Every transaction within a TxGraph has exactly one corresponding TxGraph::Ref, held by users
     * of the class. Destroying the TxGraph::Ref removes the corresponding transaction (in both the
     * main and staging graphs).
     *
     * Users of the class can inherit from TxGraph::Ref. If all Refs are inherited this way, the
     * Ref* pointers returned by TxGraph functions can be cast to, and used as, this inherited type.
     */
    class Ref;

    /** Virtual destructor, so inheriting is safe. */
    virtual ~TxGraph() = default;
    /** Construct a new transaction with the specified feerate, and return a Ref to it.
     *  If a staging graph exists, the new transaction is only created there. In all
     *  further calls, only Refs created by AddTransaction() are allowed to be passed to this
     *  TxGraph object (or empty Ref objects). Ref objects may outlive the TxGraph they were
     *  created for. */
    [[nodiscard]] virtual Ref AddTransaction(const FeePerWeight& feerate) noexcept = 0;
    /** Remove the specified transaction. If a staging graph exists, the removal only happens
     *  there. This is a no-op if the transaction was already removed.
     *
     * TxGraph may internally reorder transaction removals with dependency additions for
     * performance reasons. If together with any transaction removal all its descendants, or all
     * its ancestors, are removed as well (which is what always happens in realistic scenarios),
     * this reordering will not affect the behavior of TxGraph.
     *
     * As an example, imagine 3 transactions A,B,C where B depends on A. If a dependency of C on B
     * is added, and then B is deleted, C will still depend on A. If the deletion of B is reordered
     * before the C->B dependency is added, the dependency adding has no effect. If, together with
     * the deletion of B also either A or C is deleted, there is no distinction between the
     * original order case and the reordered case.
     */
    virtual void RemoveTransaction(const Ref& arg) noexcept = 0;
    /** Add a dependency between two specified transactions. If a staging graph exists, the
     *  dependency is only added there. Parent may not be a descendant of child already (but may
     *  be an ancestor of it already, in which case this is a no-op). If either transaction is
     *  already removed, this is a no-op. */
    virtual void AddDependency(const Ref& parent, const Ref& child) noexcept = 0;
    /** Modify the fee of the specified transaction, in both the main graph and the staging
     *  graph if it exists. Wherever the transaction does not exist (or was removed), this has no
     *  effect. */
    virtual void SetTransactionFee(const Ref& arg, int64_t fee) noexcept = 0;

    /** TxGraph is internally lazy, and will not compute many things until they are needed.
     *  Calling DoWork will compute everything now, so that future operations are fast. This can be
     *  invoked while oversized. */
    virtual void DoWork() noexcept = 0;

    /** Create a staging graph (which cannot exist already). This acts as if a full copy of
     *  the transaction graph is made, upon which further modifications are made. This copy can
     *  be inspected, and then either discarded, or the main graph can be replaced by it by
     *  committing it. */
    virtual void StartStaging() noexcept = 0;
    /** Discard the existing active staging graph (which must exist). */
    virtual void AbortStaging() noexcept = 0;
    /** Replace the main graph with the staging graph (which must exist). */
    virtual void CommitStaging() noexcept = 0;
    /** Check whether a staging graph exists. */
    virtual bool HaveStaging() const noexcept = 0;

    /** Determine whether the graph is oversized (contains a connected component of more than the
     *  configured maximum cluster count). If main_only is false and a staging graph exists, it is
     *  queried; otherwise the main graph is queried. Some of the functions below are not available
     *  for oversized graphs. The mutators above are always available. Removing a transaction by
     *  destroying its Ref while staging exists will not clear main's oversizedness until staging
     *  is aborted or committed. */
    virtual bool IsOversized(bool main_only = false) noexcept = 0;
    /** Determine whether arg exists in the graph (i.e., was not removed). If main_only is false
     *  and a staging graph exists, it is queried; otherwise the main graph is queried. This is
     *  available even for oversized graphs. */
    virtual bool Exists(const Ref& arg, bool main_only = false) noexcept = 0;
    /** Get the individual transaction feerate of transaction arg. Returns the empty FeePerWeight
     *  if arg does not exist in either main or staging. This is available even for oversized
     *  graphs. */
    virtual FeePerWeight GetIndividualFeerate(const Ref& arg) noexcept = 0;
    /** Get the feerate of the chunk which transaction arg is in, in the main graph. Returns the
     *  empty FeePerWeight if arg does not exist in the main graph. The main graph must not be
     *  oversized. */
    virtual FeePerWeight GetMainChunkFeerate(const Ref& arg) noexcept = 0;
    /** Get pointers to all transactions in the cluster which arg is in. The transactions are
     *  returned in graph order. If main_only is false and a staging graph exists, it is queried;
     *  otherwise the main graph is queried. The queried graph must not be oversized. Returns {} if
     *  arg does not exist in the queried graph. */
    virtual std::vector<Ref*> GetCluster(const Ref& arg, bool main_only = false) noexcept = 0;
    /** Get pointers to all ancestors of the specified transaction (including the transaction
     *  itself), in unspecified order. If main_only is false and a staging graph exists, it is
     *  queried; otherwise the main graph is queried. The queried graph must not be oversized.
     *  Returns {} if arg does not exist in the graph. */
    virtual std::vector<Ref*> GetAncestors(const Ref& arg, bool main_only = false) noexcept = 0;
    /** Get pointers to all descendants of the specified transaction (including the transaction
     *  itself), in unspecified order. If main_only is false and a staging graph exists, it is
     *  queried; otherwise the main graph is queried. The queried graph must not be oversized.
     *  Returns {} if arg does not exist in the graph. */
    virtual std::vector<Ref*> GetDescendants(const Ref& arg, bool main_only = false) noexcept = 0;
    /** Like GetAncestors, but return the Refs for all transactions in the union of the provided
     *  arguments' ancestors (each transaction is only reported once). Refs that do not exist in
     *  the queried graph are ignored. Null refs are not allowed. */
    virtual std::vector<Ref*> GetAncestorsUnion(std::span<const Ref* const> args, bool main_only = false) noexcept = 0;
    /** Like GetDescendants, but return the Refs for all transactions in the union of the provided
     *  arguments' descendants (each transaction is only reported once). Refs that do not exist in
     *  the queried graph are ignored. Null refs are not allowed. */
    virtual std::vector<Ref*> GetDescendantsUnion(std::span<const Ref* const> args, bool main_only = false) noexcept = 0;
    /** Get the total number of transactions in the graph. If main_only is false and a staging
     *  graph exists, it is queried; otherwise the main graph is queried. This is available even
     *  for oversized graphs. */
    virtual GraphIndex GetTransactionCount(bool main_only = false) noexcept = 0;
    /** Compare two transactions according to their order in the main graph. Both transactions must
     *  be in the main graph. The main graph must not be oversized. */
    virtual std::strong_ordering CompareMainOrder(const Ref& a, const Ref& b) noexcept = 0;
    /** Count the number of distinct clusters that the specified transactions belong to. If
     *  main_only is false and a staging graph exists, staging clusters are counted. Otherwise,
     *  main clusters are counted. Refs that do not exist in the queried graph are ignored. Refs
     *  can not be null. The queried graph must not be oversized. */
    virtual GraphIndex CountDistinctClusters(std::span<const Ref* const>, bool main_only = false) noexcept = 0;
    /** For both main and staging (which must both exist and not be oversized), return the combined
     *  respective feerate diagrams, including chunks from all clusters, but excluding clusters
     *  that appear identically in both. Use FeeFrac rather than FeePerWeight so CompareChunks is
     *  usable without type-conversion. */
    virtual std::pair<std::vector<FeeFrac>, std::vector<FeeFrac>> GetMainStagingDiagrams() noexcept = 0;

    /** Interface returned by GetBlockBuilder. */
    class BlockBuilder
    {
    protected:
        /** Make constructor non-public (use TxGraph::GetBlockBuilder()). */
        BlockBuilder() noexcept = default;
    public:
        /** Support safe inheritance. */
        virtual ~BlockBuilder() = default;
        /** Get the chunk that is currently suggested to be included, plus its feerate, if any. */
        virtual std::optional<std::pair<std::vector<Ref*>, FeePerWeight>> GetCurrentChunk() noexcept = 0;
        /** Mark the current chunk as included, and progress to the next one. */
        virtual void Include() noexcept = 0;
        /** Mark the current chunk as skipped, and progress to the next one. Further chunks from
         *  the same cluster as the current one will not be reported anymore. */
        virtual void Skip() noexcept = 0;
    };

    /** Construct a block builder, drawing chunks in order, from the main graph, which cannot be
     *  oversized. While the returned object exists, no mutators on the main graph are allowed.
     *  The BlockBuilder object must not outlive the TxGraph it was created with. */
    virtual std::unique_ptr<BlockBuilder> GetBlockBuilder() noexcept = 0;
    /** Get the last chunk in the main graph, i.e., the last chunk that would be returned by a
     *  BlockBuilder created now, together with its feerate. The chunk is returned in
     *  reverse-topological order, so every element is preceded by all its descendants. The main
     *  graph must not be oversized. If the graph is empty, {{}, FeePerWeight{}} is returned. */
    virtual std::pair<std::vector<Ref*>, FeePerWeight> GetWorstMainChunk() noexcept = 0;

    /** Perform an internal consistency check on this object. */
    virtual void SanityCheck() const = 0;

protected:
    // Allow TxGraph::Ref to call UpdateRef and UnlinkRef.
    friend class TxGraph::Ref;
    /** Inform the TxGraph implementation that a TxGraph::Ref has moved. */
    virtual void UpdateRef(GraphIndex index, Ref& new_location) noexcept = 0;
    /** Inform the TxGraph implementation that a TxGraph::Ref was destroyed. */
    virtual void UnlinkRef(GraphIndex index) noexcept = 0;
    // Allow TxGraph implementations (inheriting from it) to access Ref internals.
    static TxGraph*& GetRefGraph(Ref& arg) noexcept { return arg.m_graph; }
    static TxGraph* GetRefGraph(const Ref& arg) noexcept { return arg.m_graph; }
    static GraphIndex& GetRefIndex(Ref& arg) noexcept { return arg.m_index; }
    static GraphIndex GetRefIndex(const Ref& arg) noexcept { return arg.m_index; }

public:
    class Ref
    {
        // Allow TxGraph's GetRefGraph and GetRefIndex to access internals.
        friend class TxGraph;
        /** Which Graph the Entry lives in. nullptr if this Ref is empty. */
        TxGraph* m_graph = nullptr;
        /** Index into the Graph's m_entries. Only used if m_graph != nullptr. */
        GraphIndex m_index = GraphIndex(-1);
    public:
        /** Construct an empty Ref. Non-empty Refs can only be created using
         *  TxGraph::AddTransaction. */
        Ref() noexcept = default;
        /** Destroy this Ref. If it is not empty, the corresponding transaction is removed (in both
         *  main and staging, if it exists). */
        virtual ~Ref();
        // Support moving a Ref.
        Ref& operator=(Ref&& other) noexcept;
        Ref(Ref&& other) noexcept;
        // Do not permit copy constructing or copy assignment. A TxGraph entry can have at most one
        // Ref pointing to it.
        Ref& operator=(const Ref&) = delete;
        Ref(const Ref&) = delete;
    };
};

/** Construct a new TxGraph with the specified limit on transactions within a cluster. That
 *  number cannot exceed MAX_CLUSTER_COUNT_LIMIT. */
std::unique_ptr<TxGraph> MakeTxGraph(unsigned max_cluster_count) noexcept;

#endif // BITCOIN_TXGRAPH_H
