// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compare>
#include <stdint.h>
#include <memory>
#include <vector>

#include <util/feefrac.h>

#ifndef BITCOIN_TXGRAPH_H
#define BITCOIN_TXGRAPH_H

static constexpr unsigned MAX_CLUSTER_COUNT_LIMIT{64};

/** Data structure to encapsulate fees, sizes, and dependencies for a set of transactions. */
class TxGraph
{
public:
    /** Internal identifier for a transaction within a TxGraph. */
    using GraphIndex = uint32_t;

    /** Data type used to reference transactions within a TxGraph.
     *
     * Every transaction within a TxGraph has exactly one corresponding TxGraph::Ref, held by users
     * of the class. Destroying the TxGraph::Ref removes the corresponding transaction.
     *
     * Users of the class can inherit from TxGraph::Ref. If all Refs are inherited this way, the
     * Ref* pointers returned by TxGraph functions can be used as this inherited type.
     */
    class Ref
    {
        // Allow TxGraph's GetRefGraph and GetRefIndex to access internals.
        friend class TxGraph;
        /** Which Graph the Entry lives in. nullptr if this Ref is empty. */
        TxGraph* m_graph = nullptr;
        /** Index into the Graph's m_entries. Only used if m_graph != nullptr. */
        GraphIndex m_index = GraphIndex(-1);
    public:
        /** Construct an empty Ref (not pointing to any Entry). */
        Ref() noexcept = default;
        /** Test if this Ref is not empty. */
        explicit operator bool() const noexcept { return m_graph != nullptr; }
        /** Destroy this Ref. */
        virtual ~Ref();
        // Support moving a Ref.
        Ref& operator=(Ref&& other) noexcept;
        Ref(Ref&& other) noexcept;
        // Do not permit copy constructing or copy assignment. A TxGraph entry can have at most one
        // Ref pointing to it.
        Ref& operator=(const Ref&) = delete;
        Ref(const Ref&) = delete;
    };

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
    /** Virtual destructor, so inheriting is safe. */
    virtual ~TxGraph() = default;
    /** Construct a new transaction with the specified feerate, and return a Ref to it.
     *  If a staging graph exists, the new transaction is only created there. */
    [[nodiscard]] virtual Ref AddTransaction(const FeeFrac& feerate) noexcept = 0;
    /** Remove the specified transaction. If a staging graph exists, the removal only happens
     *  there. This is a no-op if the transaction was already removed. */
    virtual void RemoveTransaction(Ref& arg) noexcept = 0;
    /** Add a dependency between two specified transactions. If a staging graph exists, the
     *  dependency is only added there. Parent may not be a descendant of child already (but may
     *  be an ancestor of it already, in which case this is a no-op). If either transaction is
     *  already removed, this is a no-op. */
    virtual void AddDependency(Ref& parent, Ref& child) noexcept = 0;
    /** Modify the feerate of the specified transaction, in both the main graph and the staging
     *  graph if it exists. Wherever the transaction does not exist (or was removed), this has no
     *  effect. */
    virtual void SetTransactionFeerate(Ref& arg, const FeeFrac& feerate) noexcept = 0;
    /** Return a vector of pointers to Ref objects for transactions which have been removed from
     *  the graph, and have not been destroyed yet. This has no effect if a staging graph exists,
     *  or if the graph is oversized (see below). Each transaction is only reported once by
     *  Cleanup(). Afterwards, all Refs will be empty. */
    [[nodiscard]] virtual std::vector<Ref*> Cleanup() noexcept = 0;

    /** Create a staging graph (which cannot exist already). This acts as if a full copy of
     *  the transaction graph is made, upon which further modifications are made. This copy can
     *  be inspected, and then either discarded, or the main graph can be replaced by it by
     *  commiting it. */
    virtual void StartStaging() noexcept = 0;
    /** Discard the existing active staging graph (which must exist). */
    virtual void AbortStaging() noexcept = 0;
    /** Replace the main graph with the staging graph (which must exist). */
    virtual void CommitStaging() noexcept = 0;
    /** Check whether a staging graph exists. */
    virtual bool HaveStaging() const noexcept = 0;

    /** Determine whether arg exists in the graph (i.e., was not removed). If main_only is false
     *  and a staging graph exists, it is queried; otherwise the main graph is queried. */
    virtual bool Exists(const Ref& arg, bool main_only = false) noexcept = 0;
    /** Determine whether the graph is oversized (contains a connected component of more than the
     *  configured maximum cluster count). If main_only is false and a staging graph exists, it is
     *  queried; otherwise the main graph is queried. Some of the functions below are not available
     *  for oversized graphs. The mutators above are always available. */
    virtual bool IsOversized(bool main_only = false) noexcept = 0;
    /** Get the feerate of the chunk which transaction arg is in in the main graph. Returns the
     *  empty FeeFrac if arg does not exist in the main graph. The main graph must not be
     *  oversized. */
    virtual FeeFrac GetMainChunkFeerate(const Ref& arg) noexcept = 0;
    /** Get the individual transaction feerate of transaction arg. Returns the empty FeeFrac if
     *  arg does not exist in either main or staging. This is available even for oversized
     *  graphs. */
    virtual FeeFrac GetIndividualFeerate(const Ref& arg) noexcept = 0;
    /** Get pointers to all transactions in the connected component ("cluster") which arg is in.
     *  The transactions will be returned in a topologically-valid order of acceptable quality.
     *  If main_only is false and a staging graph exists, it is queried; otherwise the main graph
     *  is queried. The queried graph must not be oversized. Returns {} if arg does not exist in
     *  the queried graph. */
    virtual std::vector<Ref*> GetCluster(const Ref& arg, bool main_only = false) noexcept = 0;
    /** Get pointers to all ancestors of the specified transaction. If main_only is false and a
     *  staging graph exists, it is queried; otherwise the main graph is queried. The queried
     *  graph must not be oversized. Returns {} if arg does not exist in the queried graph. */
    virtual std::vector<Ref*> GetAncestors(const Ref& arg, bool main_only = false) noexcept = 0;
    /** Get pointers to all descendants of the specified transaction. If main_only is false and a
     *  staging graph exists, it is queried; otherwise the main graph is queried. The queried
     *  graph must not be oversized. Returns {} if arg does not exist in the queried graph. */
    virtual std::vector<Ref*> GetDescendants(const Ref& arg, bool main_only = false) noexcept = 0;
    /** Get the total number of transactions in the graph. If main_only is false and a staging
     *  graph exists, it is queried; otherwise the main graph is queried. This is available even
     *  for oversized graphs. */
    virtual GraphIndex GetTransactionCount(bool main_only = false) noexcept = 0;

    /** Perform an internal consistency check on this object. */
    virtual void SanityCheck() const = 0;
};

/** Construct a new TxGraph with the specified limit on transactions within a cluster. That
 *  number cannot exceed MAX_CLUSTER_COUNT_LIMIT. */
std::unique_ptr<TxGraph> MakeTxGraph(unsigned max_cluster_count) noexcept;

#endif // BITCOIN_TXGRAPH_H
