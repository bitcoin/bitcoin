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

/** No connected component within TxGraph is allowed to exceed this number of transactions. */
static constexpr unsigned CLUSTER_COUNT_LIMIT{64};

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
    /** Construct a new transaction with the specified feerate, and return a Ref to it. */
    [[nodiscard]] virtual Ref AddTransaction(const FeeFrac& feerate) noexcept = 0;
    /** Remove the specified transaction. This is a no-op if the transaction was already removed. */
    virtual void RemoveTransaction(Ref& arg) noexcept = 0;
    /** Add a dependency between two specified transactions. Parent may not be a descendant of
     *  child already (but may be an ancestor of it already, in which case this is a no-op). If
     *  either transaction is already removed, this is a no-op. */
    virtual void AddDependency(Ref& parent, Ref& child) noexcept = 0;
    /** Modify the feerate of the specified transaction. If the transaction does not exist (or was
     *  removed), this has no effect. */
    virtual void SetTransactionFeerate(Ref& arg, const FeeFrac& feerate) noexcept = 0;
    /** Return a vector of pointers to Ref objects for transactions which have been removed from
     *  the graph, and have not been destroyed yet. Each transaction is only reported once by
     *  Cleanup(). Afterwards, all Refs will be empty. */
    [[nodiscard]] virtual std::vector<Ref*> Cleanup() noexcept = 0;

    /** Determine whether arg exists in this graph (i.e., was not removed). */
    virtual bool Exists(const Ref& arg) noexcept = 0;
    /** Get the feerate of the chunk which transaction arg is in. Returns the empty FeeFrac if arg
     *  does not exist. */
    virtual FeeFrac GetChunkFeerate(const Ref& arg) noexcept = 0;
    /** Get the individual transaction feerate of transaction arg. Returns the empty FeeFrac if
     *  arg does not exist. */
    virtual FeeFrac GetIndividualFeerate(const Ref& arg) noexcept = 0;
    /** Get pointers to all transactions in the connected component ("cluster") which arg is in.
     *  The transactions will be returned in a topologically-valid order of acceptable quality.
     *  Returns {} if arg does not exist in the queried graph. */
    virtual std::vector<Ref*> GetCluster(const Ref& arg) noexcept = 0;
    /** Get pointers to all ancestors of the specified transaction. Returns {} if arg does not
     *  exist. */
    virtual std::vector<Ref*> GetAncestors(const Ref& arg) noexcept = 0;
    /** Get pointers to all descendants of the specified transaction. Returns {} if arg does not
     *  exist in the graph. */
    virtual std::vector<Ref*> GetDescendants(const Ref& arg) noexcept = 0;
    /** Get the total number of transactions in the graph. */
    virtual GraphIndex GetTransactionCount() noexcept = 0;

    /** Perform an internal consistency check on this object. */
    virtual void SanityCheck() const = 0;
};

/** Construct a new TxGraph. */
std::unique_ptr<TxGraph> MakeTxGraph() noexcept;

#endif // BITCOIN_TXGRAPH_H
