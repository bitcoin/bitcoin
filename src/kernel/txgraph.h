#ifndef BITCOIN_KERNEL_TXGRAPH_H
#define BITCOIN_KERNEL_TXGRAPH_H

#include <util/feefrac.h>
#include <policy/feerate.h>
#include <consensus/amount.h>
#include <policy/policy.h>
#include <util/epochguard.h>
#include <memusage.h>
#include <sync.h>
#include <util/check.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/member.hpp>

#include <atomic>
#include <functional>
#include <list>
#include <set>
#include <utility>

using namespace std;

class TxGraphCluster;
class TxGraph;

static std::atomic<int64_t> unique_id_counter{0};

class TxEntry {
public:
    typedef std::reference_wrapper<const TxEntry> TxEntryRef;
    typedef std::pair<size_t, std::list<TxEntryRef>::iterator> Location;

    struct CompareById {
        bool operator()(const TxEntryRef& a, const TxEntryRef& b) const {
            return a.get().unique_id < b.get().unique_id;
        }
    };

    typedef std::set<TxEntryRef, CompareById> TxEntryParents;
    typedef std::set<TxEntryRef, CompareById> TxEntryChildren;

    TxEntry(int32_t vsize, CAmount modified_fee)
        : m_virtual_size(vsize), m_modified_fee(modified_fee) {}
    virtual ~TxEntry() = default;
    int64_t unique_id{++unique_id_counter};
    int32_t m_virtual_size;
    CAmount m_modified_fee;              //!< Tx fee (including prioritisetransaction effects)
    int32_t GetTxSize() const { return m_virtual_size; }
    CAmount GetModifiedFee() const { return m_modified_fee; }

    TxEntryParents& GetTxEntryParents() const { return parents; }
    TxEntryChildren& GetTxEntryChildren() const { return children; }

    mutable Location m_loc;              //!< Location within a cluster
    mutable TxGraphCluster *m_cluster{nullptr}; //! The cluster this entry belongs to
private:
    // Note: It's a little weird to store parent and children information in
    // the TxEntry, because the notion of which transactions are connected is
    // one that exists at the cluster/graph level, rather than the transaction
    // level. In particular, if a transaction is being evaluated for RBF, then
    // it's possible that some other transaction (eg a common parent) might
    // have two different possible descendants, depending on which transaction
    // ultimately is accepted to the mempool.
    // Fortunately, for now this implementation doesn't relay on child
    // information, only parent information, for being able to invoke the
    // cluster_linearize sorting algorithm. Since parent information is correct
    // and currently unambiguous for RBF evaluation, this implementation should
    // work, but this could break in the future if (eg) we wanted to implement
    // RBF'ing a transaction with some other transaction that had the same txid
    // (eg smaller witness replacement, where child transactions would not need
    // to be evicted).
    // Maybe sipa's implementation will move this information from the
    // transaction to the cluster, and eliminate this confusion?
    mutable TxEntryParents parents;
    mutable TxEntryChildren children;
    mutable Epoch::Marker m_epoch_marker; //!< epoch when last touched

    friend class TxGraph;
};

class TxGraphCluster {
public:
    TxGraphCluster(int64_t id, TxGraph *tx_graph) : m_id(id), m_tx_graph(tx_graph) {}

    void Clear() {
        m_chunks.clear();
        m_tx_count = 0;
    }

    // Add a transaction and update the sort.
    void AddTransaction(const TxEntry& entry, bool sort);
    void RemoveTransaction(const TxEntry& entry);

    // Sort the cluster and partition into chunks.
    void Sort(bool reassign_locations = true);

    // Just rechunk the cluster using its existing linearization.
    void Rechunk();

    // Permanently assign transactions to this cluster
    void AssignTransactions();

    // Sanity checks -- verify metadata matches and clusters are topo-sorted.
    bool Check() const;
    bool CheckTopo() const;

private:
    // Helper function
    void RechunkFromLinearization(std::vector<TxEntry::TxEntryRef>& txs, bool reassign_locations);

public:
    void Merge(std::vector<TxGraphCluster *>::iterator first, std::vector<TxGraphCluster*>::iterator last, bool this_cluster_first);
    void MergeCopy(std::vector<TxGraphCluster *>::const_iterator first, std::vector<TxGraphCluster*>::const_iterator last);

    uint64_t GetMemoryUsage() const {
        return memusage::DynamicUsage(m_chunks) + m_tx_count * sizeof(void*) * 3;
    }

    TxEntry::TxEntryRef GetLastTransaction();

    // The chunks of transactions which will be added to blocks or
    // evicted from the mempool.
    struct Chunk {
        Chunk(CAmount _fee, int64_t _size) : feerate(_fee, _size) {}
        Chunk(Chunk&& other) = default;
        Chunk& operator=(TxGraphCluster::Chunk&& other) = default;
        Chunk& operator=(const TxGraphCluster::Chunk& other) = delete;

        FeeFrac feerate; // The fee/size of this chunk
        std::list<TxEntry::TxEntryRef> txs;
    };

    typedef std::vector<Chunk>::iterator ChunkIter;
    typedef std::pair<ChunkIter, TxGraphCluster*> HeapEntry;

    std::vector<Chunk> m_chunks;
    int64_t m_tx_count{0};
    int64_t m_tx_size{0};

    const int64_t m_id;
    mutable Epoch::Marker m_epoch_marker; //!< epoch when last touched

    TxGraph *m_tx_graph{nullptr};
};

class Trimmer {
public:
    Trimmer(TxGraph *tx_graph);
    ~Trimmer();
    CFeeRate RemoveWorstChunk(std::vector<TxEntry::TxEntryRef>& txs_to_remove);

private:
    std::set<TxGraphCluster*> clusters_with_evictions;

    TxGraph *m_tx_graph{nullptr};
};

class TxSelector {
public:
    TxSelector(const TxGraph *tx_graph);
    ~TxSelector() = default;
    // Return the next chunk in the mempool that is at most max_vsize in size.
    FeeFrac SelectNextChunk(std::vector<TxEntry::TxEntryRef>& txs);
    // If the transactions were successfully used, then notify the TxSelector
    // to keep selecting transactions from the same cluster.
    void Success();

    static bool ChunkCompare(const TxGraphCluster::HeapEntry& a, const TxGraphCluster::HeapEntry& b) {
        return a.first->feerate < b.first->feerate;
    }
private:
    std::vector<TxGraphCluster::HeapEntry> heap_chunks;

    const TxGraph *m_tx_graph{nullptr};
    TxGraphCluster::HeapEntry m_last_entry_selected{TxGraphCluster::ChunkIter(), nullptr};
};

struct GraphLimits {
    //! The maximum number of transactions in a cluster.
    int64_t cluster_count{DEFAULT_CLUSTER_LIMIT};
    //! The maximum allowed size in virtual bytes of a cluster.
    int64_t cluster_size_vbytes{DEFAULT_CLUSTER_SIZE_LIMIT_KVB*1'000};
};

class TxGraphChangeSet {
public:
    TxGraphChangeSet(TxGraph *tx_graph, GraphLimits limits, const std::vector<TxEntry::TxEntryRef>& to_remove);
    ~TxGraphChangeSet();

    // Returns failure if a cluster size limit would be hit.
    bool AddTx(TxEntry::TxEntryRef tx, const std::vector<TxEntry::TxEntryRef> parents);

    void GetFeerateDiagramOld(std::vector<FeeFrac> &diagram);
    void GetFeerateDiagramNew(std::vector<FeeFrac> &diagram);

    void Apply(); // Apply this changeset to the txgraph, adding/removing
                  // transactions and clusters as needed.
private:
    void GetFeerateDiagram(std::vector<FeeFrac> &diagram, const std::vector<TxGraphCluster*>& clusters);
    void SortNewClusters();
    TxGraph *m_tx_graph{nullptr};
    GraphLimits m_limits;
    std::map<int64_t, TxGraphCluster *> m_tx_to_cluster_map; // map entries to their new clusters
    std::set<int64_t> m_new_clusters; // cluster id's of the new clusters
    std::vector<TxEntry::TxEntryRef> m_txs_to_add;
    std::vector<TxEntry::TxEntryRef> m_txs_to_remove;
    std::vector<TxGraphCluster *> m_clusters_to_delete;

    bool m_sort_new_clusters{true};
};

class TxGraph {
    public:
    TxGraph() = default;

    // (lazily?) add a transaction to the graph (assume no in-mempool children?)
    void AddTx(TxEntry *new_tx, int32_t vsize, CAmount modified_fee, const std::vector<TxEntry::TxEntryRef>& parents);

    // Lazily remove a transaction from the graph
    void RemoveTx(TxEntry::TxEntryRef remove_tx) EXCLUSIVE_LOCKS_REQUIRED(cs);

    void RemoveBatch(std::vector<TxEntry::TxEntryRef> &txs_removed);

    // add a group of parent transactions, but limit resulting cluster sizes.
    void AddParentTxs(std::vector<TxEntry::TxEntryRef> parent_txs, GraphLimits limits, std::function<std::vector<TxEntry::TxEntryRef>(TxEntry::TxEntryRef)> func, std::vector<TxEntry::TxEntryRef> &txs_removed);
private:
    std::pair<std::vector<TxGraphCluster*>, std::vector<TxGraphCluster*>> GetAllConnectedClusters(TxGraphCluster *target) EXCLUSIVE_LOCKS_REQUIRED(cs);
public:
    // Evict the last chunk from the given cluster.
    // We need to do this iteratively, so lazy updating of state would be better.
    void RemoveChunkForEviction(TxGraphCluster *cluster) EXCLUSIVE_LOCKS_REQUIRED(cs);

    void UpdateForPrioritisedTransaction(const TxEntry& tx);

    std::vector<TxEntry::TxEntryRef> GetAncestors(const std::vector<TxEntry::TxEntryRef>& txs) const;

    std::vector<TxEntry::TxEntryRef> GetDescendants(const std::vector<TxEntry::TxEntryRef>& txs) const;

    size_t GetUniqueClusterCount(const std::vector<TxEntry::TxEntryRef>& txs) const;

    // Return all transactions in the clusters that the given transactions are part of.
    std::vector<TxEntry::TxEntryRef> GatherAllClusterTransactions(const std::vector<TxEntry::TxEntryRef> &txs) const;

    void GetClusterSize(const std::vector<TxEntry::TxEntryRef>& parents, int64_t &cluster_size, int64_t &cluster_count) const;
    TxGraphCluster* GetClusterById(int64_t id) const EXCLUSIVE_LOCKS_REQUIRED(cs) {
        auto it = m_cluster_map.find(id);
        if (it != m_cluster_map.end()) return it->second.get();
        return nullptr;
    }
    uint64_t GetClusterCount() const { LOCK(cs); return m_cluster_map.size(); }
    uint64_t GetClusterCount(const TxEntry& tx) const { return tx.m_cluster->m_tx_count; }

    void Check(GraphLimits limits) const; // sanity checks
    void CheckMemory() const;

    bool HasDescendants(const TxEntry& tx) const {
        return tx.GetTxEntryChildren().size() > 0;
    }

    // TODO: add test coverage
    bool CompareMiningScore(const TxEntry& a, const TxEntry& b) const {
        if (&a == &b) return false;

        FeeFrac a_frac = a.m_cluster->m_chunks[a.m_loc.first].feerate;
        FeeFrac b_frac = b.m_cluster->m_chunks[b.m_loc.first].feerate;
        if (a_frac != b_frac) {
            return a_frac > b_frac;
        } else if (a.m_cluster != b.m_cluster) {
            // Equal scores in different clusters; sort by cluster id.
            return a.m_cluster->m_id < b.m_cluster->m_id;
            //return a->GetTx().GetHash() < b->GetTx().GetHash();
        } else if (a.m_loc.first != b.m_loc.first) {
            // Equal scores in same cluster; sort by chunk index.
            return a.m_loc.first < b.m_loc.first;
        } else {
            // Equal scores in same cluster and chunk; sort by position in chunk.
            for (auto it = a.m_cluster->m_chunks[a.m_loc.first].txs.begin();
                    it != a.m_cluster->m_chunks[a.m_loc.first].txs.end(); ++it) {
                if (&(it->get()) == &a) return true;
                if (&(it->get()) == &b) return false;
            }
        }
        Assume(false); // this should not be reachable.
        return true;
    }

private:
    struct worst_chunk {};
    struct best_chunk {};
    struct id {};

    class CompareTxGraphClusterByWorstChunk {
    public:
        bool operator()(const TxGraphCluster& a, const TxGraphCluster& b) const
        {
            return operator()(&a, &b);
        }
        bool operator()(const TxGraphCluster* a, const TxGraphCluster* b) const
        {
            return a->m_chunks.back().feerate < b->m_chunks.back().feerate;
        }
    };

    class CompareTxGraphClusterByBestChunk {
    public:
        bool operator()(const TxGraphCluster& a, const TxGraphCluster& b) const
        {
            return operator()(&a, &b);
        }
        bool operator()(const TxGraphCluster* a, const TxGraphCluster* b) const
        {
            return a->m_chunks.back().feerate > b->m_chunks.back().feerate;
        }
    };
    typedef boost::multi_index_container<
        TxGraphCluster*,
        boost::multi_index::indexed_by<
            // sorted by lowest chunk feerate
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<worst_chunk>,
                boost::multi_index::identity<TxGraphCluster>,
                CompareTxGraphClusterByWorstChunk
            >,
            // sorted by highest chunk feerate
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<best_chunk>,
                boost::multi_index::identity<TxGraphCluster>,
                CompareTxGraphClusterByBestChunk
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<id>,
                boost::multi_index::member<TxGraphCluster, const int64_t, &TxGraphCluster::m_id>
            >
        >
    > indexed_cluster_set;

    indexed_cluster_set m_cluster_index;

    void EraseCluster(TxGraphCluster* c);
    void UpdateClusterIndex(TxGraphCluster* c);

    // Create a new (empty) cluster in the cluster map, and return a pointer to it.
    TxGraphCluster* AssignTxGraphCluster() EXCLUSIVE_LOCKS_REQUIRED(cs);

    bool visited(const TxEntry& entry) const EXCLUSIVE_LOCKS_REQUIRED(cs, m_epoch)
    {
        return m_epoch.visited(entry.m_epoch_marker);
    }

    bool visited(TxGraphCluster *cluster) const EXCLUSIVE_LOCKS_REQUIRED(cs, m_epoch)
    {
        return m_epoch.visited(cluster->m_epoch_marker);
    }

    void RecalculateTxGraphClusterAndMaybeSort(TxGraphCluster *cluster, bool sort) EXCLUSIVE_LOCKS_REQUIRED(cs);

    void TopoSort(std::vector<TxEntry::TxEntryRef>& to_be_sorted) const;

    void UpdateParent(TxEntry::TxEntryRef entry, TxEntry::TxEntryRef parent, bool add) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void UpdateChild(TxEntry::TxEntryRef entry, TxEntry::TxEntryRef child, bool add) EXCLUSIVE_LOCKS_REQUIRED(cs);

public:
    uint64_t GetInnerUsage() const { LOCK(cs); return cachedInnerUsage; }

    mutable RecursiveMutex cs; // TODO: figure out how this could be private? used by rpc code, bleh

    const std::unordered_map<int64_t, std::unique_ptr<TxGraphCluster>>& GetClusterMap() const EXCLUSIVE_LOCKS_REQUIRED(cs) { return m_cluster_map; }
private:

    // TxGraphClusters
    std::unordered_map<int64_t, std::unique_ptr<TxGraphCluster>> m_cluster_map GUARDED_BY(cs);
    int64_t m_next_cluster_id GUARDED_BY(cs){0};

    mutable Epoch m_epoch GUARDED_BY(cs){};
    uint64_t cachedInnerUsage GUARDED_BY(cs){0};

    friend class Trimmer;
    friend class TxGraphCluster;
    friend class TxSelector;
    friend class TxGraphChangeSet;
};

#endif // BITCOIN_KERNEL_TXGRAPH_H
