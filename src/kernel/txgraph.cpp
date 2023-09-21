#include <kernel/txgraph.h>
#include <cluster_linearize.h>
#include <util/time.h>
#include <logging.h>
#include <util/strencodings.h>
#include <util/bitset.h>

const uint64_t MAX_ITERATIONS = 1000;

void TxGraphCluster::AddTransaction(const TxEntry& entry, bool sort)
{
    m_chunks.emplace_back(entry.GetModifiedFee(), entry.GetTxSize());
    m_chunks.back().txs.emplace_back(entry);
    //entry.m_cluster = this; XXX do we need this? it breaks the rbf logic.
    ++m_tx_count;
    m_tx_size += entry.GetTxSize();
    if (sort) Sort();
    return;
}

void TxGraphCluster::RemoveTransaction(const TxEntry& entry)
{
    m_chunks[entry.m_loc.first].txs.erase(entry.m_loc.second);

    // Chunk (or cluster) may now be empty, but this will get cleaned up
    // when the cluster is re-sorted (or when the cluster is deleted) Note:
    // if we cleaned up empty chunks here, then this would break the
    // locations of other entries in the cluster. Since we would like to be
    // able to do multiple removals in a row and then clean up the sort, we
    // can't clean up empty chunks here.
    --m_tx_count;
    m_tx_size -= entry.m_virtual_size;
    return;
}

void TxGraphCluster::RechunkFromLinearization(std::vector<TxEntry::TxEntryRef>& txs, bool reassign_locations)
{
    m_chunks.clear();
    m_tx_size = 0;

    for (auto txentry : txs) {
        m_chunks.emplace_back(txentry.get().GetModifiedFee(), txentry.get().GetTxSize());
        m_chunks.back().txs.emplace_back(txentry);
        while (m_chunks.size() >= 2) {
            auto cur_iter = std::prev(m_chunks.end());
            auto prev_iter = std::prev(cur_iter);
            // We only combine chunks if the feerate would go up; if two
            // chunks have equal feerate, we prefer to keep the smaller
            // chunksize (which is generally better for both mining and
            // eviction).
            if (prev_iter->feerate < cur_iter->feerate) {
                prev_iter->feerate += cur_iter->feerate;
                prev_iter->txs.splice(prev_iter->txs.end(), cur_iter->txs, cur_iter->txs.begin(), cur_iter->txs.end());
                m_chunks.erase(cur_iter);
            } else {
                break;
            }
        }
        m_tx_size += txentry.get().GetTxSize();
    }

    if (reassign_locations) {
        AssignTransactions();
    }
}

void TxGraphCluster::AssignTransactions()
{
    // Update locations of all transactions
    for (size_t i=0; i<m_chunks.size(); ++i) {
        for (auto it = m_chunks[i].txs.begin(); it != m_chunks[i].txs.end(); ++it) {
            it->get().m_cluster = this;
            it->get().m_loc = {i, it};
        }
    }
}

TxEntry::TxEntryRef TxGraphCluster::GetLastTransaction()
{
    assert(m_tx_count > 0);
    for (auto chunkit = m_chunks.rbegin(); chunkit != m_chunks.rend(); ++chunkit) {
        if (!chunkit->txs.empty()) return chunkit->txs.back();
    }
    // Unreachable
    assert(false);
}

bool TxGraphCluster::Check() const
{
    // First check that the metadata is correct.
    int64_t tx_count = 0;
    int64_t tx_size = 0;
    for (auto &chunk : m_chunks) {
        for (auto &tx : chunk.txs) {
            ++tx_count;
            tx_size += tx.get().GetTxSize();
        }
    }
    if (tx_count != m_tx_count) return false;
    if (tx_size != m_tx_size) return false;

    // Check topology.
    std::set<TxEntry::TxEntryRef, TxEntry::CompareById> seen_elements;
    for (auto &chunk : m_chunks) {
        for (auto tx : chunk.txs) {
            for (auto parent : tx.get().GetTxEntryParents()) {
                if (seen_elements.count(parent) == 0) return false;
            }
            seen_elements.insert(tx);
            if (tx.get().m_cluster != this) return false;
            if (&m_chunks[tx.get().m_loc.first] != &chunk) return false;
            if (tx.get().m_loc.second->get().unique_id != tx.get().unique_id) return false;
        }
    }
    return true;
}

bool TxGraphCluster::CheckTopo() const
{
    // Check topology.
    std::set<TxEntry::TxEntryRef, TxEntry::CompareById> seen_elements;
    for (auto &chunk : m_chunks) {
        for (auto tx : chunk.txs) {
            for (auto parent : tx.get().GetTxEntryParents()) {
                if (seen_elements.count(parent) == 0) return false;
            }
            seen_elements.insert(tx);
        }
    }
    return true;
}

namespace {

template <typename SetType>
std::vector<TxEntry::TxEntryRef> InvokeSort(size_t tx_count, const std::vector<TxGraphCluster::Chunk>& chunks)
{
    std::vector<TxEntry::TxEntryRef> txs;
    cluster_linearize::DepGraph<SetType> dep_graph;
    const auto time_1{SteadyClock::now()};

    std::vector<TxEntry::TxEntryRef> orig_txs;
    std::vector<std::pair<const TxEntry*, cluster_linearize::ClusterIndex>> entry_to_index;

    entry_to_index.reserve(tx_count);
    for (auto &chunk : chunks) {
        for (auto tx : chunk.txs) {
            orig_txs.emplace_back(tx);
            entry_to_index.emplace_back(&(tx.get()), dep_graph.AddTransaction(FeeFrac(tx.get().GetModifiedFee(), tx.get().GetTxSize())));
        }
    }
    std::sort(entry_to_index.begin(), entry_to_index.end());
    for (size_t i=0; i<orig_txs.size(); ++i) {
        SetType parents;
        auto tx_location = std::lower_bound(entry_to_index.begin(), entry_to_index.end(), &(orig_txs[i].get()),
                [&](const auto& a, const auto& b) { return std::less<const TxEntry*>()(a.first, b); });
        for (auto& parent : orig_txs[i].get().GetTxEntryParents()) {
            auto it = std::lower_bound(entry_to_index.begin(), entry_to_index.end(), &(parent.get()),
                    [&](const auto& a, const auto& b) { return std::less<const TxEntry*>()(a.first, b); });
            assert(it != entry_to_index.end());
            assert(it->first == &(parent.get()));
            parents.Set(it->second);
        }
        dep_graph.AddDependencies(parents, tx_location->second);
    }
    uint64_t iterations = MAX_ITERATIONS;
    uint64_t iterations_done = 0;

    std::vector<unsigned int> orig_linearization;
    orig_linearization.reserve(tx_count);
    for (unsigned int i=0; i<tx_count; ++i) {
        orig_linearization.push_back(i);
    }
    auto result = cluster_linearize::Linearize(dep_graph, iterations, 0, orig_linearization, &iterations_done);
    if (!result.second) {
        // We want to postlinearize non-optimal clusters, to ensure that chunks
        // are connected and to perform "easy" improvements.
        cluster_linearize::PostLinearize(dep_graph, result.first);
    }
    txs.clear();
    for (auto index : result.first) {
        txs.push_back(orig_txs[index]);
    }

    const auto time_2{SteadyClock::now()};
    if (tx_count >= 10) {
        double time_millis = Ticks<MillisecondsDouble>(time_2-time_1);

        LogDebug(BCLog::BENCH, "InvokeSort linearize cluster: %zu txs, %.4fms, %u iter, %.1fns/iter\n",
                tx_count,
                time_millis,
                iterations_done,
                time_millis * 1000000.0 / (iterations_done > 0 ? iterations_done : iterations_done+1));
    }
    return txs;
}

} // namespace

void TxGraphCluster::Sort(bool reassign_locations)
{
    std::vector<TxEntry::TxEntryRef> txs;
    if (m_tx_count <= 32) {
        txs = InvokeSort<BitSet<32>>(m_tx_count, m_chunks);
    } else if (m_tx_count <= 64) {
        txs = InvokeSort<BitSet<64>>(m_tx_count, m_chunks);
    } else if (m_tx_count <= 128) {
        txs = InvokeSort<BitSet<128>>(m_tx_count, m_chunks);
    } else if (m_tx_count <= 192) {
        txs = InvokeSort<BitSet<192>>(m_tx_count, m_chunks);
    } else if (m_tx_count <= 256) {
        txs = InvokeSort<BitSet<256>>(m_tx_count, m_chunks);
    } else if (m_tx_count <= 320) {
        txs = InvokeSort<BitSet<320>>(m_tx_count, m_chunks);
    } else if (m_tx_count <= 384) {
        txs = InvokeSort<BitSet<384>>(m_tx_count, m_chunks);
    } else if (m_tx_count <= 1280) {
        txs = InvokeSort<BitSet<1280>>(m_tx_count, m_chunks);
    } else {
        // Only do the topological sort for big clusters
        for (auto &chunk : m_chunks) {
            for (auto chunk_tx : chunk.txs) {
                txs.emplace_back(chunk_tx.get());
            }
        }
        m_tx_graph->TopoSort(txs);
    }
    RechunkFromLinearization(txs, reassign_locations);
}

void TxGraphCluster::Rechunk()
{
    std::vector<TxEntry::TxEntryRef> txs;

    // Insert all transactions from the cluster into txs
    for (auto &chunk : m_chunks) {
        for (auto chunk_tx : chunk.txs) {
            txs.push_back(chunk_tx);
        }
    }

    RechunkFromLinearization(txs, true);
}

void TxGraphCluster::MergeCopy(std::vector<TxGraphCluster*>::const_iterator first, std::vector<TxGraphCluster*>::const_iterator last)
{
    if (first == last) return;

    for (auto it=first; it != last; ++it) {
        for (auto &chunk : (*it)->m_chunks) {
            for (auto &tx: chunk.txs) {
                AddTransaction(tx, false);
            }
        }
    }
}

// Merge the clusters from [first, last) into this cluster.
void TxGraphCluster::Merge(std::vector<TxGraphCluster*>::iterator first, std::vector<TxGraphCluster*>::iterator last, bool this_cluster_first)
{
    // Check to see if we have anything to do.
    if (first == last) return;

    std::vector<Chunk> new_chunks;
    std::vector<TxGraphCluster::HeapEntry> heap_chunks;

    int64_t total_txs = m_tx_count;

    // Make a heap of all the best chunks.
    for (auto it = first; it != last; ++it) {
        if ((*it)->m_chunks.size() > 0) {
            heap_chunks.emplace_back((*it)->m_chunks.begin(), *it);
        }
        total_txs += (*it)->m_tx_count;
    }

    // During a reorg, we want to merge clusters corresponding to descendants
    // so that they appear after the cluster with their parent. This allows us
    // to trim megaclusters down to our cluster size limit in a way that
    // respects topology but still preferences higher feerate chunks over lower
    // feerate chunks.
    if (this_cluster_first) {
        new_chunks = std::move(m_chunks);
        m_chunks.clear();
    } else if (m_tx_count > 0) {
        heap_chunks.emplace_back(m_chunks.begin(), this);
    }
    // Define comparison operator on our heap entries (using feerate of chunks).
    auto cmp = [](const TxGraphCluster::HeapEntry& a, const TxGraphCluster::HeapEntry& b) {
        return a.first->feerate < b.first->feerate;
    };

    std::make_heap(heap_chunks.begin(), heap_chunks.end(), cmp);

    while (!heap_chunks.empty()) {
        // Take the best chunk from the heap.
        auto best_chunk = heap_chunks.front();
        new_chunks.emplace_back(std::move(*(best_chunk.first)));
        // Remove the best chunk from the heap.
        std::pop_heap(heap_chunks.begin(), heap_chunks.end(), cmp);
        heap_chunks.pop_back();
        // If the cluster has more chunks, add the next best chunk to the heap.
        ++best_chunk.first;
        if (best_chunk.first != best_chunk.second->m_chunks.end()) {
            heap_chunks.emplace_back(best_chunk.first, best_chunk.second);
            std::push_heap(heap_chunks.begin(), heap_chunks.end(), cmp);
        }
    }

    // At this point we've merged the clusters into new_chunks.
    m_chunks = std::move(new_chunks);

    m_tx_count=0;
    m_tx_size=0;

    for (size_t i=0; i<m_chunks.size(); ++i) {
        m_tx_count += m_chunks[i].txs.size();
        m_tx_size += m_chunks[i].feerate.size;
    }

    // Update the cluster and location information for each transaction.
    AssignTransactions();

    assert(m_tx_count == total_txs);
}

void TxGraph::UpdateClusterIndex(TxGraphCluster* c)
{
    auto it = m_cluster_index.get<id>().find(c->m_id);
    if (it != m_cluster_index.get<id>().end()) {
        m_cluster_index.get<id>().erase(it);
    }
    m_cluster_index.insert(c);
}

void TxGraph::EraseCluster(TxGraphCluster* c)
{
    LOCK(cs);
    auto it = m_cluster_index.get<id>().find(c->m_id);
    if (it != m_cluster_index.get<id>().end()) {
        m_cluster_index.get<id>().erase(it);
    }
    m_cluster_map.erase(c->m_id);
}

void TxGraph::AddTx(TxEntry *new_tx, int32_t vsize, CAmount modified_fee, const std::vector<TxEntry::TxEntryRef>& parents)
{
    LOCK(cs);
    new_tx->m_virtual_size = vsize;
    new_tx->m_modified_fee = modified_fee;
    new_tx->GetTxEntryParents().clear();
    // Figure out which cluster this transaction belongs to.
    std::vector<TxGraphCluster*> clusters_to_merge;
    {
        WITH_FRESH_EPOCH(m_epoch);
        for (auto p : parents) {
            UpdateParent(*new_tx, p, true);
            UpdateChild(p, *new_tx, true);
            if (!visited(p.get().m_cluster)) {
                clusters_to_merge.push_back(p.get().m_cluster);
            }
        }
    }

    // Merge all the clusters together.
    if (clusters_to_merge.size() == 0) {
        // No parents, make a new cluster.
        new_tx->m_cluster = AssignTxGraphCluster();
        new_tx->m_cluster->AddTransaction(*new_tx, true);
        cachedInnerUsage += new_tx->m_cluster->GetMemoryUsage();
        UpdateClusterIndex(new_tx->m_cluster);
    } else if (clusters_to_merge.size() == 1) {
        cachedInnerUsage -= clusters_to_merge[0]->GetMemoryUsage();
        // Only one parent cluster: add to it.
        new_tx->m_cluster = clusters_to_merge[0];
        clusters_to_merge[0]->AddTransaction(*new_tx, true);
        cachedInnerUsage += clusters_to_merge[0]->GetMemoryUsage();
        UpdateClusterIndex(new_tx->m_cluster);
    } else {
        cachedInnerUsage -= clusters_to_merge[0]->GetMemoryUsage();
        clusters_to_merge[0]->Merge(clusters_to_merge.begin()+1, clusters_to_merge.end(), false);
        // Add this transaction to the cluster.
        new_tx->m_cluster = clusters_to_merge[0];
        clusters_to_merge[0]->AddTransaction(*new_tx, true);
        UpdateClusterIndex(new_tx->m_cluster);
        // Need to delete the other clusters.
        for (auto it=clusters_to_merge.begin()+1; it != clusters_to_merge.end(); ++it) {
            cachedInnerUsage -= (*it)->GetMemoryUsage();
            EraseCluster(*it);
        }
        cachedInnerUsage += clusters_to_merge[0]->GetMemoryUsage();
    }
}

void TxGraph::RemoveTx(TxEntry::TxEntryRef remove_tx)
{
    cachedInnerUsage -= memusage::DynamicUsage(remove_tx.get().GetTxEntryParents()) + memusage::DynamicUsage(remove_tx.get().GetTxEntryChildren());
    // Update the parent/child state
    for (const TxEntry::TxEntryRef& parent: remove_tx.get().GetTxEntryParents()) {
        parent.get().GetTxEntryChildren().erase(remove_tx);
        cachedInnerUsage -= memusage::IncrementalDynamicUsage(parent.get().GetTxEntryChildren());
    }
    for (const TxEntry::TxEntryRef& child: remove_tx.get().GetTxEntryChildren()) {
        child.get().GetTxEntryParents().erase(remove_tx);
        cachedInnerUsage -= memusage::IncrementalDynamicUsage(child.get().GetTxEntryParents());
    }
    // Update the cluster
    remove_tx.get().m_cluster->RemoveTransaction(remove_tx.get());
}


std::vector<TxEntry::TxEntryRef> TxGraph::GetDescendants(const std::vector<TxEntry::TxEntryRef>& txs) const
{
    std::vector<TxEntry::TxEntryRef> result{}, work_queue{};

    LOCK(cs);

    WITH_FRESH_EPOCH(m_epoch);
    for (auto tx : txs) {
        if (!visited(tx)) {
            work_queue.push_back(tx);
        }
    }

    while (!work_queue.empty()) {
        auto it = work_queue.back();
        work_queue.pop_back();
        result.push_back(it);
        for (auto& child: it.get().GetTxEntryChildren()) {
            if (!visited(child.get())) {
                work_queue.push_back(child);
            }
        }
    }
    return result;
}

size_t TxGraph::GetUniqueClusterCount(const std::vector<TxEntry::TxEntryRef>& txs) const
{
    size_t result = 0;
    LOCK(cs);
    WITH_FRESH_EPOCH(m_epoch);
    for (auto tx : txs) {
        if (!visited(tx.get().m_cluster)) {
            ++result;
        }
    }
    return result;
}

std::vector<TxEntry::TxEntryRef> TxGraph::GetAncestors(const std::vector<TxEntry::TxEntryRef>& parents) const
{
    std::vector<TxEntry::TxEntryRef> result{}, work_queue{};

    LOCK(cs);

    WITH_FRESH_EPOCH(m_epoch);
    for (auto p : parents) {
        visited(p);
        work_queue.push_back(p);
    }

    while (!work_queue.empty()) {
        auto it = work_queue.back();
        work_queue.pop_back();
        result.push_back(it);
        for (auto& parent: it.get().parents) {
            if (!visited(parent.get())) {
                work_queue.push_back(parent);
            }
        }
    }
    return result;
}

std::vector<TxEntry::TxEntryRef> TxGraph::GatherAllClusterTransactions(const std::vector<TxEntry::TxEntryRef> &txs) const
{
    LOCK(cs);

    WITH_FRESH_EPOCH(m_epoch);
    std::vector<TxEntry::TxEntryRef> result;
    for (auto tx : txs) {
        if (!visited(tx.get().m_cluster)) {
            for (auto &chunk : tx.get().m_cluster->m_chunks) {
                for (auto &txentry : chunk.txs) {
                    result.push_back(txentry);
                }
            }
        }
    }
    return result;
}

TxGraphCluster* TxGraph::AssignTxGraphCluster()
{
    auto new_cluster = std::make_unique<TxGraphCluster>(m_next_cluster_id++, this);
    TxGraphCluster* ret = new_cluster.get(); // XXX No one is going to like this.
    m_cluster_map[new_cluster->m_id] = std::move(new_cluster);
    return ret;
}

// When transactions are removed from a cluster, the cluster might get split
// into smaller clusters.
void TxGraph::RecalculateTxGraphClusterAndMaybeSort(TxGraphCluster *cluster, bool sort)
{
    // TODO: if the common case involves no cluster splitting, can we short
    // circuit the work here somehow?

    // Wipe cluster assignments.
    std::vector<TxEntry::TxEntryRef> txs;
    for (auto& chunk : cluster->m_chunks) {
        for (auto& txentry: chunk.txs) {
            txentry.get().m_cluster = nullptr;
            txs.push_back(txentry);
        }
    }
    cluster->Clear();

    // The first transaction gets to stay in the existing cluster.
    bool first = true;
    for (auto& txentry : txs) {
        if (txentry.get().m_cluster == nullptr) {
            if (first) {
                txentry.get().m_cluster = cluster;
                first = false;
            } else {
                txentry.get().m_cluster = AssignTxGraphCluster();
            }
            txentry.get().m_cluster->AddTransaction(txentry.get(), false);
            // We need to label all transactions connected to this one as
            // being in the same cluster.
            {
                WITH_FRESH_EPOCH(m_epoch);
                std::vector<TxEntry::TxEntryRef> work_queue;
                for (auto entry : txentry.get().GetTxEntryChildren()) {
                    work_queue.push_back(entry);
                    visited(entry.get());
                }

                while (!work_queue.empty()) {
                    auto next_entry = work_queue.back();
                    work_queue.pop_back();
                    next_entry.get().m_cluster = txentry.get().m_cluster;

                    for (auto& descendant : next_entry.get().GetTxEntryChildren()) {
                        if (!visited(descendant.get())) {
                            work_queue.push_back(descendant);
                        }
                    }
                    for (auto& ancestor : next_entry.get().parents) {
                        if (!visited(ancestor.get())) {
                            work_queue.push_back(ancestor);
                        }
                    }
                }
            }
        } else {
            // If we already have a cluster assignment, we need to just add
            // ourselves to the cluster. Doing the addition here preserves
            // the topology and sort order from the original cluster.
            txentry.get().m_cluster->AddTransaction(txentry.get(), false);
        }
    }

    // After all assignments are made, either re-sort or re-chunk each cluster.
    std::vector<TxGraphCluster *> clusters_to_fix;
    {
        WITH_FRESH_EPOCH(m_epoch);
        for (auto it : txs) {
            if (!visited(it.get().m_cluster)) {
                clusters_to_fix.push_back(it.get().m_cluster);
            }
        }
    }
    for (auto cluster : clusters_to_fix) {
        if (sort) {
            cluster->Sort(true);
        } else {
            cluster->Rechunk();
        }
        UpdateClusterIndex(cluster);
        cachedInnerUsage += cluster->GetMemoryUsage();
    }

    // Sanity check that all transactions are where they should be.
    for (auto it : txs) {
        assert(it.get().unique_id == it.get().m_loc.second->get().unique_id);
    }
}

void TxGraph::RemoveBatch(std::vector<TxEntry::TxEntryRef> &txs_removed)
{
    LOCK(cs);
    static std::vector<TxGraphCluster *> cluster_clean_up;
    cluster_clean_up.clear();
    {
        WITH_FRESH_EPOCH(m_epoch);
        for (auto t : txs_removed) {
            bool delete_now{false};
            TxGraphCluster *cluster = t.get().m_cluster;

            // Single transaction clusters can be deleted immediately without
            // any additional work.  TxGraphClusters with more than one transaction
            // need to be cleaned up later, even if they are ultimately fully
            // cleared, since we've left the pointer to the cluster
            // in the cluster_clean_up structure (so don't want to delete it and
            // invalidate the pointer).
            if (!visited(cluster)) {
                if (cluster->m_tx_count > 1) {
                    cluster_clean_up.push_back(cluster);
                } else {
                    delete_now = true;
                }
                cachedInnerUsage -= cluster->GetMemoryUsage();
            }
            RemoveTx(t);
            if (delete_now) {
                EraseCluster(cluster);
            }
        }
    }
    // After all transactions have been removed, delete the empty clusters and
    // repartition/re-sort the remaining clusters (which could have split).
    for (auto c : cluster_clean_up) {
        if (c->m_tx_count == 0) {
            EraseCluster(c);
        } else {
            RecalculateTxGraphClusterAndMaybeSort(c, true);
        }
    }
}

// Remove the last chunk from the cluster.
void TxGraph::RemoveChunkForEviction(TxGraphCluster *cluster)
{
    AssertLockHeld(cs);

    cachedInnerUsage -= cluster->GetMemoryUsage();

    std::vector<TxEntry::TxEntryRef> txs;
    for (auto& tx : cluster->m_chunks.back().txs) {
        txs.emplace_back(tx);
    }

    for (auto& tx : txs) {
        RemoveTx(tx);
    }

    cluster->m_chunks.pop_back();

    cachedInnerUsage += cluster->GetMemoryUsage();
    // Note: at this point the clusters will still be sorted, but they may need
    // to be split.
}

void TxGraph::UpdateForPrioritisedTransaction(const TxEntry& tx)
{
    LOCK(cs);
    cachedInnerUsage -= tx.m_cluster->GetMemoryUsage();
    tx.m_cluster->Sort();
    cachedInnerUsage += tx.m_cluster->GetMemoryUsage();
    UpdateClusterIndex(tx.m_cluster);
}

// Note: it's currently unsafe to look at children in this function, because
// during RBF processing we only have the parents in a consistent state.
void TxGraph::TopoSort(std::vector<TxEntry::TxEntryRef>& to_be_sorted) const
{
    LOCK(cs);
    std::vector<TxEntry::TxEntryRef> sorted;
    sorted.reserve(to_be_sorted.size());
    std::vector<bool> already_added(to_be_sorted.size(), false);

    WITH_FRESH_EPOCH(m_epoch);
    for (size_t i=0; i<to_be_sorted.size(); ++i) {
        auto tx = to_be_sorted[i];
        // Check to see if this is already in the list.
        if (m_epoch.is_visited(tx.get().m_epoch_marker)) continue;

        // Gather the parents for traversal.
        std::vector<TxEntry::TxEntryRef> work_queue;
        for (auto parent : tx.get().GetTxEntryParents()) {
            if (!m_epoch.is_visited(parent.get().m_epoch_marker)) {
                work_queue.push_back(parent);

                while (!work_queue.empty()) {
                    auto next_entry = work_queue.back();
                    // Check to see if this entry is already added.
                    if (m_epoch.is_visited(next_entry.get().m_epoch_marker)) {
                        work_queue.pop_back();
                        continue;
                    }
                    // Otherwise, check to see if all parents have been walked.
                    bool parents_visited_already = true;
                    for (auto p : next_entry.get().GetTxEntryParents()) {
                        if (!m_epoch.is_visited(p.get().m_epoch_marker)) {
                            parents_visited_already = false;
                            work_queue.push_back(p);
                        }
                    }
                    // If parents have all been walked, we can remove this entry and
                    // add it to the list
                    if (parents_visited_already) {
                        work_queue.pop_back();
                        sorted.push_back(next_entry);
                        visited(next_entry);
                    }
                }
            }
        }
        // Now that the ancestors are added, we can add this entry.
        sorted.push_back(tx);
        visited(tx);
    }
    to_be_sorted.swap(sorted);
}

void TxGraph::UpdateChild(TxEntry::TxEntryRef entry, TxEntry::TxEntryRef child, bool add)
{
    AssertLockHeld(cs);
    TxEntry::TxEntryChildren s;
    if (add && entry.get().GetTxEntryChildren().insert(child).second) {
        cachedInnerUsage += memusage::IncrementalDynamicUsage(s);
    } else if (!add && entry.get().GetTxEntryChildren().erase(child)) {
        cachedInnerUsage -= memusage::IncrementalDynamicUsage(s);
    }
}

void TxGraph::UpdateParent(TxEntry::TxEntryRef entry, TxEntry::TxEntryRef parent, bool add)
{
    AssertLockHeld(cs);
    TxEntry::TxEntryParents s;
    if (add && entry.get().parents.insert(parent).second) {
        cachedInnerUsage += memusage::IncrementalDynamicUsage(s);
    } else if (!add && entry.get().parents.erase(parent)) {
        cachedInnerUsage -= memusage::IncrementalDynamicUsage(s);
    }
}

// This is a helper function for dealing with reorgs (see AddParentTxs).
// "target" is a cluster that consists only of transactions from a reorged
// block. Transactions in target may have children that are in clusters that
// only consist of transactions that were in the mempool prior to the reorg
// occurring.
// This function returns a pair of vectors, consisting of clusters that are
// also in the reorged block which are connected to the target cluster, and
// consisting of clusters that are disjoint from the reorged block that are
// connected to the target cluster.
// See AddParentTxs for more about how this is used to help stitch the mempool
// back together after a reorg.
std::pair<std::vector<TxGraphCluster*>, std::vector<TxGraphCluster*>> TxGraph::GetAllConnectedClusters(TxGraphCluster *target)
{
    std::vector<TxGraphCluster*> ret_block, ret_mempool;

    WITH_FRESH_EPOCH(m_epoch);

    std::vector<TxGraphCluster*> in_mempool_clusters;
    std::vector<TxGraphCluster*> in_block_clusters;

    // We don't return the target cluster in ret_block.
    visited(target);
    in_block_clusters.push_back(target);

    while (!in_mempool_clusters.empty() || !in_block_clusters.empty()) {
        while (!in_block_clusters.empty()) {
            auto cluster = in_block_clusters.back();
            in_block_clusters.pop_back();
            for (auto& chunk : cluster->m_chunks) {
                for (auto& tx : chunk.txs) {
                    for (auto& child : tx.get().GetTxEntryChildren()) {
                        if (!visited(child.get().m_cluster)) {
                            ret_mempool.push_back(child.get().m_cluster);
                            in_mempool_clusters.push_back(child.get().m_cluster);
                        }
                    }
                }
            }
        }
        while (!in_mempool_clusters.empty()) {
            auto cluster = in_mempool_clusters.back();
            in_mempool_clusters.pop_back();
            for (auto& chunk : cluster->m_chunks) {
                for (auto& tx : chunk.txs) {
                    for (auto& parent : tx.get().GetTxEntryParents()) {
                        if (!visited(parent.get().m_cluster)) {
                            ret_block.push_back(parent.get().m_cluster);
                            in_block_clusters.push_back(parent.get().m_cluster);
                        }
                    }
                }
            }
        }
    }
    return {ret_block, ret_mempool};
}

void TxGraph::AddParentTxs(std::vector<TxEntry::TxEntryRef> parent_txs, GraphLimits limits,
        std::function<std::vector<TxEntry::TxEntryRef>(TxEntry::TxEntryRef)> func,
        std::vector<TxEntry::TxEntryRef> &txs_removed)
{
    LOCK(cs);
    // Start by filling in the parent/child maps so that the in-block
    // transactions are connected to their in-mempool children.
    for (auto tx : parent_txs) {
        auto children = func(tx);
        for (auto child : children) {
            UpdateChild(tx, child, true);
            UpdateParent(child, tx, true);
        }
    }

    // Now we need to fix the clusters -- the transactions that came from the
    // disconnected blocks will be correctly clustered with each other, but not
    // with any children that were in the mempool when the reorg started.
    // Fix this as follows:
    // - For each cluster belonging to a transaction from the reorged block
    //   * Find all connected clusters, and classify such clusters as either
    //     belonging to in-block transactions or in-mempool transactions.
    //   * Merge the in-block clusters together first, so that none of the
    //     clusters that were from the mempool prior to the reorg appear in
    //     the merged cluster before the in-block transactions.
    //   * Then merge in the in-mempool clusters, keeping the in-block cluster
    //     transactions first. (At this point, the cluster will be
    //     topologically sorted.)
    // - Then limit the size of the resulting cluster if it exceeds cluster
    //   size limits, by removing transactions from the end.
    // - Then sort the cluster (and re-cluster if any transactions were
    //   removed).
    // - Repeat for every (remaining) cluster from the reorged block.
    std::set<TxGraphCluster *> unique_clusters_from_block;
    {
        WITH_FRESH_EPOCH(m_epoch);
        for (const auto& tx : parent_txs) {
            if (!visited(tx.get().m_cluster)) {
                unique_clusters_from_block.insert(tx.get().m_cluster);
            }
        }
    }
    while (!unique_clusters_from_block.empty()) {
        TxGraphCluster *cluster = *unique_clusters_from_block.begin();
        unique_clusters_from_block.erase(cluster);
        auto [from_block, from_mempool] = GetAllConnectedClusters(cluster);
        for (auto c : from_block) {
            unique_clusters_from_block.erase(c);
        }
        cachedInnerUsage -= cluster->GetMemoryUsage();
        cluster->Merge(from_block.begin(), from_block.end(), false);
        cluster->Merge(from_mempool.begin(), from_mempool.end(), true);
        UpdateClusterIndex(cluster);

        // Now delete the other clusters.
        for (auto it=from_block.begin(); it != from_block.end(); ++it) {
            cachedInnerUsage -= (*it)->GetMemoryUsage();
            EraseCluster(*it);
        }
        for (auto it=from_mempool.begin(); it != from_mempool.end(); ++it) {
            cachedInnerUsage -= (*it)->GetMemoryUsage();
            EraseCluster(*it);
        }
        // Limit the size of the resulting cluster.
        bool removed{false};
        while (cluster->m_tx_count > limits.cluster_count ||
                cluster->m_tx_size > limits.cluster_size_vbytes) {
            TxEntry::TxEntryRef last_tx = cluster->GetLastTransaction();
            RemoveTx(last_tx);
            txs_removed.emplace_back(last_tx);
            removed = true;
        }
        if (removed) {
            RecalculateTxGraphClusterAndMaybeSort(cluster, true);
        } else {
            cluster->Sort(true);
            cachedInnerUsage += cluster->GetMemoryUsage();
        }
        UpdateClusterIndex(cluster);
    }
}

void TxGraph::CheckMemory() const
{
    LOCK(cs);
    // Check that cachedInnerUsage is correct.
    uint64_t expected_memory_usage{0};
    TxEntry::TxEntryParents p;
    TxEntry::TxEntryChildren c;
    for (const auto & [cluster_id, cluster] : m_cluster_map) {
        expected_memory_usage += cluster->GetMemoryUsage();
        for (auto &chunk : cluster->m_chunks) {
            for (auto& tx : chunk.txs) {
                expected_memory_usage += tx.get().parents.size() * memusage::IncrementalDynamicUsage(p);
                expected_memory_usage += tx.get().GetTxEntryChildren().size() * memusage::IncrementalDynamicUsage(c);
            }
        }
    }
    assert(expected_memory_usage == cachedInnerUsage);
}

void TxGraph::Check(GraphLimits limits) const
{
    LOCK(cs);
    // Run sanity checks on each cluster.
    for (const auto & [cluster_id, cluster] : m_cluster_map) {
        assert(cluster->Check());
        assert(cluster->m_tx_count <= limits.cluster_count);
        assert(cluster->m_tx_size  <= limits.cluster_size_vbytes);

        // Ensure that every cluster in the cluster map corresponds to a cluster in the cluster index
        auto it = m_cluster_index.get<id>().find(cluster_id);
        assert(it != m_cluster_index.get<id>().end());
        assert((*it) == cluster.get());
    }

    // Check that every cluster in the multi_index is in the cluster_map
    assert (m_cluster_index.size() == m_cluster_map.size());

    CheckMemory();

    // Check that each cluster is connected.
    for (const auto & [id, cluster] : m_cluster_map) {
        // We'll check that if we walk to every transaction reachable from the
        // first one, that we get every tx in the cluster.
        auto first_tx = cluster->m_chunks.front().txs.front();
        std::vector<TxEntry::TxEntryRef> work_queue;
        int reachable_txs = 1; // we'll count the transactions we reach.

        WITH_FRESH_EPOCH(m_epoch);
        visited(first_tx.get());
        assert(first_tx.get().parents.size() == 0); // first tx can never have parents.
        for (auto child : first_tx.get().GetTxEntryChildren()) {
            work_queue.push_back(child);
            visited(child.get());
            ++reachable_txs;
        }
        while (work_queue.size() > 0) {
            auto next_tx = work_queue.back();
            work_queue.pop_back();
            for (auto parent : next_tx.get().parents) {
                if (!visited(parent.get())) {
                    ++reachable_txs;
                    work_queue.push_back(parent);
                }
            }
            for (auto child : next_tx.get().GetTxEntryChildren()) {
                if (!visited(child.get())) {
                    ++reachable_txs;
                    work_queue.push_back(child);
                }
            }
        }
        assert (reachable_txs == cluster->m_tx_count);
    }
}

void TxGraph::GetClusterSize(const std::vector<TxEntry::TxEntryRef>& parents, int64_t &cluster_size, int64_t &cluster_count) const
{
    LOCK(cs);
    WITH_FRESH_EPOCH(m_epoch);

    cluster_size = cluster_count = 0;
    for (auto tx: parents) {
        if (!visited(tx.get().m_cluster)) {
            cluster_size += tx.get().m_cluster->m_tx_size;
            cluster_count += tx.get().m_cluster->m_tx_count;
        }
    }
    return;
}

Trimmer::Trimmer(TxGraph* tx_graph)
    : m_tx_graph(tx_graph)
{
}

CFeeRate Trimmer::RemoveWorstChunk(std::vector<TxEntry::TxEntryRef>& txs_to_remove)
{
    LOCK(m_tx_graph->cs);

    TxGraph::indexed_cluster_set::index<TxGraph::worst_chunk>::type::iterator it = m_tx_graph->m_cluster_index.get<TxGraph::worst_chunk>().begin();

    if (it != m_tx_graph->m_cluster_index.get<TxGraph::worst_chunk>().end()) {
        auto cluster = *it;
        for (auto& tx : cluster->m_chunks.back().txs) {
            txs_to_remove.emplace_back(tx);
        }

        CFeeRate removed(cluster->m_chunks.back().feerate.fee, cluster->m_chunks.back().feerate.size);

        m_tx_graph->RemoveChunkForEviction(cluster);
        if (cluster->m_tx_count > 0) {
            m_tx_graph->UpdateClusterIndex(cluster);
        } else {
            m_tx_graph->m_cluster_index.get<TxGraph::worst_chunk>().erase(it);
        }
        clusters_with_evictions.insert(cluster);

        return removed;
    }

    return CFeeRate{};
}

Trimmer::~Trimmer()
{
    // Before we can return, we have to clean up the clusters that saw
    // evictions, because they will have stray chunks and may need to be
    // re-partitioned.
    // However, these clusters do not need to be re-sorted, because evicted
    // chunks at the end can never change the relative ordering of transactions
    // that come before them.
    for (TxGraphCluster* cluster : clusters_with_evictions) {
        m_tx_graph->cachedInnerUsage -= cluster->GetMemoryUsage();
        if (cluster->m_tx_count == 0) {
            m_tx_graph->EraseCluster(cluster);
        } else {
            m_tx_graph->RecalculateTxGraphClusterAndMaybeSort(cluster, false);
        }
    }
}

TxSelector::TxSelector(const TxGraph* tx_graph)
    : m_tx_graph(tx_graph)
{
    LOCK(m_tx_graph->cs);
    for (const auto & [id, cluster] : tx_graph->m_cluster_map) {
        if (!cluster->m_chunks.empty()) {
            heap_chunks.emplace_back(cluster->m_chunks.begin(), cluster.get());
        }
    }

    std::make_heap(heap_chunks.begin(), heap_chunks.end(), ChunkCompare);
}

FeeFrac TxSelector::SelectNextChunk(std::vector<TxEntry::TxEntryRef>& txs)
{
    LOCK(m_tx_graph->cs);
    // Remove the top element (highest feerate) that matches the maximum vsize.

    if (heap_chunks.size() > 0) {
        m_last_entry_selected = heap_chunks.front();
        std::pop_heap(heap_chunks.begin(), heap_chunks.end(), TxSelector::ChunkCompare);
        heap_chunks.pop_back();

        // Copy the txs being selected.
        for (auto& tx : m_last_entry_selected.first->txs) {
            txs.emplace_back(tx);
        }
        return m_last_entry_selected.first->feerate;
    }
    return FeeFrac(0, 0);
}

void TxSelector::Success()
{
    if (m_last_entry_selected.second != nullptr) {
        ++m_last_entry_selected.first;
        if (m_last_entry_selected.first != m_last_entry_selected.second->m_chunks.end()) {
            // Add the next chunk from this cluster back to the heap.
            heap_chunks.emplace_back(m_last_entry_selected);
            std::push_heap(heap_chunks.begin(), heap_chunks.end(), TxSelector::ChunkCompare);
        }
        m_last_entry_selected.second = nullptr;
    }
}

TxGraphChangeSet::TxGraphChangeSet(TxGraph *tx_graph, GraphLimits limits, const std::vector<TxEntry::TxEntryRef>& to_remove)
  : m_tx_graph(tx_graph), m_limits(limits), m_txs_to_remove(to_remove)
{
    // For each transaction that will be removed, copy the cluster into the
    // changeset.
    std::set<int64_t> removed_set;
    LOCK(m_tx_graph->cs);
    {
        WITH_FRESH_EPOCH(m_tx_graph->m_epoch);
        for (auto &tx : m_txs_to_remove) {
            removed_set.insert(tx.get().unique_id);
            if (!m_tx_graph->visited(tx.get().m_cluster)) {
                m_clusters_to_delete.emplace_back(tx.get().m_cluster);
            }
            // Trigger a nullptr deref if we try to add a transaction to this
            // changeset that spends a removed transaction.
            m_tx_to_cluster_map[tx.get().unique_id] = nullptr;
        }
    }

    {
        for (auto &cluster : m_clusters_to_delete) {
            for (auto& chunk : cluster->m_chunks) {
                for (auto &tx: chunk.txs) {
                    if (removed_set.count(tx.get().unique_id)) {
                        // Skip the txs that will be removed
                        continue;
                    }
                    if (!m_tx_to_cluster_map.count(tx.get().unique_id)) {
                        // No cluster assignment, so create a new one.
                        auto cluster = m_tx_graph->AssignTxGraphCluster();
                        m_new_clusters.insert(cluster->m_id);
                        m_tx_to_cluster_map[tx.get().unique_id] = cluster;
                    }
                    TxGraphCluster* c = m_tx_to_cluster_map[tx.get().unique_id];
                    c->AddTransaction(tx.get(), false);
                    // Now we have to mark all the connected transactions as being
                    // in the same cluster.

                    {
                        WITH_FRESH_EPOCH(m_tx_graph->m_epoch);
                        std::vector<TxEntry::TxEntryRef> work_queue;
                        for (auto& entry : tx.get().GetTxEntryChildren()) {
                            m_tx_graph->visited(entry.get());
                            if (removed_set.count(entry.get().unique_id)) {
                                continue;
                            }
                            work_queue.push_back(entry);
                        }
                        while (!work_queue.empty()) {
                            auto next_entry = work_queue.back();
                            work_queue.pop_back();
                            m_tx_to_cluster_map[next_entry.get().unique_id] = c;

                            for (auto& descendant : next_entry.get().GetTxEntryChildren()) {
                                if (!m_tx_graph->visited(descendant.get())) {
                                    if (removed_set.count(descendant.get().unique_id)) {
                                        continue;
                                    }
                                    work_queue.push_back(descendant);
                                }
                            }
                            for (auto& ancestor : next_entry.get().GetTxEntryParents()) {
                                if (!m_tx_graph->visited(ancestor.get())) {
                                    if (removed_set.count(ancestor.get().unique_id)) {
                                        continue;
                                    }
                                    work_queue.push_back(ancestor);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

TxGraphChangeSet::~TxGraphChangeSet()
{
    LOCK(m_tx_graph->cs);
    // Delete any allocated memory here.
    // If we didn't apply the changeset, eg because validation failed, then we
    // should delete our allocated clusters.
    for (auto &cluster_id : m_new_clusters) {
        m_tx_graph->m_cluster_map.erase(cluster_id);
    }
    m_new_clusters.clear();
}

void TxGraphChangeSet::Apply()
{
    LOCK(m_tx_graph->cs);

    if (m_sort_new_clusters) SortNewClusters();

    for (auto &cluster : m_clusters_to_delete) {
        m_tx_graph->cachedInnerUsage -= cluster->GetMemoryUsage();
    }

    // Remove the transactions from the graph.
    {
        for (auto &tx : m_txs_to_remove) {
            // Invoking RemoveTx() will cleanup the parent/child maps, and
            // update memory usage.
            m_tx_graph->RemoveTx(tx);
        }
    }
    m_txs_to_remove.clear();

    // Delete any clusters that are no longer needed.
    for (auto &cluster : m_clusters_to_delete) {
        m_tx_graph->EraseCluster(cluster);
    }
    m_clusters_to_delete.clear();

    // Add in the clusters that were created.
    for (auto &cluster_id : m_new_clusters) {
        auto cluster = m_tx_graph->m_cluster_map[cluster_id].get();
        m_tx_graph->cachedInnerUsage += cluster->GetMemoryUsage();
        cluster->AssignTransactions();
        m_tx_graph->UpdateClusterIndex(cluster);
    }
    m_new_clusters.clear();

    // XXX: since memory isn't tracked properly for parents/children, fix that
    // here, and also add the new transactions as children of the existing
    // transactions.
    TxEntry::TxEntryParents p;
    for (auto &tx : m_txs_to_add) {
        m_tx_graph->cachedInnerUsage += memusage::DynamicUsage(tx.get().GetTxEntryParents());
        for (auto& parent : tx.get().GetTxEntryParents()) {
            m_tx_graph->UpdateChild(parent, tx, true);
        }
    }
    m_txs_to_add.clear();
}

static bool HasSameParents(const TxEntry& a, const TxEntry& b) {
    if (a.GetTxEntryParents().size() != b.GetTxEntryParents().size()) return false;
    for (auto& p : a.GetTxEntryParents()) {
        if (b.GetTxEntryParents().count(p) == 0) return false;
    }
    return true;
}


bool TxGraphChangeSet::AddTx(TxEntry::TxEntryRef tx, const std::vector<TxEntry::TxEntryRef> parents)
{
    // Register a new cluster for this transaction that merges its parents.
    LOCK(m_tx_graph->cs);

    for (auto& parent : parents) {
        tx.get().GetTxEntryParents().insert(parent);
    }

    // Go through all the parents, and merge them into a new cluster, checking
    // the limits.
    {
        // Check to see if the new cluster is within our limits.
        int64_t new_cluster_vsize{tx.get().m_virtual_size};
        int64_t new_cluster_count{1};

        std::vector<TxGraphCluster *> clusters_to_merge;
        WITH_FRESH_EPOCH(m_tx_graph->m_epoch);
        for (auto& parent : parents) {
            TxGraphCluster* parent_cluster{nullptr};
            if (m_tx_to_cluster_map.find(parent.get().unique_id) != m_tx_to_cluster_map.end()) {
                parent_cluster = m_tx_to_cluster_map[parent.get().unique_id];
            } else {
                // If it's not in the cluster map, it must be an existing
                // transaction, so we can just look at the cluster assignment.
                parent_cluster = parent.get().m_cluster;
            }
            assert(parent_cluster != nullptr);
            if (!m_tx_graph->visited(parent_cluster)) {
                new_cluster_vsize += parent_cluster->m_tx_size;
                new_cluster_count += parent_cluster->m_tx_count;
                clusters_to_merge.emplace_back(parent_cluster);
            }
        }
        if (new_cluster_vsize > m_limits.cluster_size_vbytes || new_cluster_count > m_limits.cluster_count) {
            return false;
        }

        // Otherwise, we're good to combine the needed clusters.
        TxGraphCluster *new_cluster = m_tx_graph->AssignTxGraphCluster();
        m_new_clusters.insert(new_cluster->m_id);

        // Special case the situation where a single new transaction is
        // replacing a single old transaction with the exact same parents.
        // TODO: verify that we no longer need this hack.
        if (m_txs_to_remove.size() == 1 && HasSameParents(m_txs_to_remove.front().get(), tx.get()) && m_txs_to_add.size() == 0) {
            const TxEntry &removetx = m_txs_to_remove.front().get();
            assert(m_clusters_to_delete.size() == 1);
            TxGraphCluster* old_cluster = m_clusters_to_delete[0];
            for (auto& chunk : old_cluster->m_chunks) {
                for (auto& entry : chunk.txs) {
                    if (&(entry.get()) != &removetx) {
                        new_cluster->AddTransaction(entry.get(), false);
                    } else {
                        new_cluster->AddTransaction(tx.get(), false);
                    }
                }
            }
        } else {
            new_cluster->MergeCopy(clusters_to_merge.begin(), clusters_to_merge.end());
            new_cluster->AddTransaction(tx.get(), false);
        }
        m_sort_new_clusters = true;

        // Update our index of tx -> cluster assignment.
        for (auto& chunk : new_cluster->m_chunks) {
            for (auto& tx : chunk.txs) {
                m_tx_to_cluster_map[tx.get().unique_id] = new_cluster;
            }
        }

        // If we merged any clusters that had been temporary creations in our
        // changeset, we can just get rid of them now.
        for (auto& c : clusters_to_merge) {
            if (m_new_clusters.count(c->m_id) > 0) {
                m_new_clusters.erase(c->m_id);
                m_tx_graph->m_cluster_map.erase(c->m_id);
            }
        }
    }
    {
        WITH_FRESH_EPOCH(m_tx_graph->m_epoch);
        for (auto& c : m_clusters_to_delete) {
            m_tx_graph->visited(c);
        }
        for (auto& p : parents) {
            // If we're adding transactions with in-mempool parents, we must
            // add those in-mempool parents' clusters to our "old" set.
            if (p.get().m_cluster != nullptr && !m_tx_graph->visited(p.get().m_cluster)) {
                m_clusters_to_delete.emplace_back(p.get().m_cluster);
            }
        }
    }
    m_txs_to_add.emplace_back(tx);

    return true;
}

void TxGraphChangeSet::SortNewClusters()
{
    LOCK(m_tx_graph->cs);
    // Sort every new cluster.
    for (auto cluster_id : m_new_clusters) {
        m_tx_graph->m_cluster_map[cluster_id]->Sort(false);
    }
    m_sort_new_clusters = false;
}

void TxGraphChangeSet::GetFeerateDiagramOld(std::vector<FeeFrac>& diagram)
{
    GetFeerateDiagram(diagram, m_clusters_to_delete);
    return;
}

void TxGraphChangeSet::GetFeerateDiagramNew(std::vector<FeeFrac>& diagram)
{
    LOCK(m_tx_graph->cs);
    std::vector<TxGraphCluster*> new_clusters;
    new_clusters.reserve(m_new_clusters.size());
    for (auto cluster_id : m_new_clusters) {
        new_clusters.emplace_back(m_tx_graph->m_cluster_map[cluster_id].get());
    }
    if (m_sort_new_clusters) SortNewClusters();
    GetFeerateDiagram(diagram, new_clusters);
    return;
}

void TxGraphChangeSet::GetFeerateDiagram(std::vector<FeeFrac>& diagram, const std::vector<TxGraphCluster*>& clusters)
{
    LOCK(m_tx_graph->cs);
    diagram.clear();

    std::vector<TxGraphCluster::HeapEntry> heap_chunks;

    for (auto& cluster : clusters) {
        if (!cluster->m_chunks.empty()) {
            heap_chunks.emplace_back(cluster->m_chunks.begin(), cluster);
        }
    }
    std::make_heap(heap_chunks.begin(), heap_chunks.end(), TxSelector::ChunkCompare);
    while (!heap_chunks.empty()) {
        auto best_chunk = heap_chunks.front();
        std::pop_heap(heap_chunks.begin(), heap_chunks.end(), TxSelector::ChunkCompare);
        heap_chunks.pop_back();

        diagram.emplace_back(best_chunk.first->feerate);

        ++best_chunk.first;
        if (best_chunk.first != best_chunk.second->m_chunks.end()) {
            heap_chunks.emplace_back(best_chunk);
            std::push_heap(heap_chunks.begin(), heap_chunks.end(), TxSelector::ChunkCompare);
        }
    }
}
