// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/txgraph.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

namespace {

bool CheckClusterSizeLimits(TxGraph& txgraph, TxEntry *entry, std::vector<TxEntry::TxEntryRef>& parents, GraphLimits limits)
{
    int64_t total_cluster_count{0};
    int64_t total_cluster_vbytes{0};
    txgraph.GetClusterSize(parents, total_cluster_vbytes, total_cluster_count);
    total_cluster_count += 1;
    total_cluster_vbytes += entry->GetTxSize();
    if (total_cluster_count <= limits.cluster_count && total_cluster_vbytes <= limits.cluster_size_vbytes) {
        return true;
    }
    return false;
}

FUZZ_TARGET(txgraph)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    TxGraph txgraph;

    // Pick random cluster limits.
    GraphLimits limits;
    limits.cluster_count = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(1, 100);
    limits.cluster_size_vbytes = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(1000, 101000);

    // Generate a random number of random fee/size transactions to seed the txgraph.
    std::vector<TxEntry*> all_entries;
    std::set<int64_t> in_txgraph;

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 1000) {
        TxEntry *entry = new
            TxEntry(fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(60, 100'000),
                    fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 100'000*1'000));
        all_entries.emplace_back(entry);

        std::vector<TxEntry::TxEntryRef> parents;
        for (size_t i=0; i<all_entries.size(); ++i) {
            if (fuzzed_data_provider.ConsumeBool() && in_txgraph.count(all_entries[i]->unique_id)) {
                parents.emplace_back(*all_entries[i]);
            }
        }
        if (CheckClusterSizeLimits(txgraph, entry, parents, limits)) {
            txgraph.AddTx(entry, entry->GetTxSize(), entry->GetModifiedFee(), parents);
            in_txgraph.insert(entry->unique_id);
        }
        txgraph.Check(limits);
    }

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100) {
        if (fuzzed_data_provider.ConsumeBool()) {
            // Do an RBF.
            std::vector<TxEntry::TxEntryRef> direct_conflicts;
            for (size_t i=0; i<all_entries.size(); ++i) {
                if (fuzzed_data_provider.ConsumeBool() && in_txgraph.count(all_entries[i]->unique_id)) {
                    direct_conflicts.emplace_back(*all_entries[i]);
                }
            }
            std::vector<TxEntry::TxEntryRef> all_conflicts = txgraph.GetDescendants(direct_conflicts);
            TxGraphChangeSet changeset(&txgraph, limits, all_conflicts);
            TxEntry *entry = new
                TxEntry(fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(60, 100'000),
                        fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 1'000*100'000));
            all_entries.emplace_back(entry);
            std::vector<TxEntry::TxEntryRef> parents;
            for (size_t i=0; i<all_entries.size(); ++i) {
                // Skip anything that is a conflict
                bool conflict{false};
                for (auto &c: all_conflicts) {
                    if (c.get().unique_id == all_entries[i]->unique_id) {
                        conflict = true;
                        break;
                    }
                }
                if (conflict) continue;
                if (fuzzed_data_provider.ConsumeBool() && in_txgraph.count(all_entries[i]->unique_id)) {
                    parents.emplace_back(*all_entries[i]);
                }
            }
            if (changeset.AddTx(*entry, parents)) {
                std::vector<FeeFrac> diagram_dummy;
                changeset.GetFeerateDiagramOld(diagram_dummy);
                changeset.GetFeerateDiagramNew(diagram_dummy);
                if (fuzzed_data_provider.ConsumeBool()) {
                    changeset.Apply();
                    in_txgraph.insert(entry->unique_id);
                    for (auto &c: all_conflicts) {
                        in_txgraph.erase(c.get().unique_id);
                    }
                }
            }
        }
        txgraph.Check(limits);
        if (fuzzed_data_provider.ConsumeBool()) {
            // Remove a random transaction and its descendants
            for (size_t i=0; i < all_entries.size(); ++i) {
                if (in_txgraph.count(all_entries[i]->unique_id) && fuzzed_data_provider.ConsumeBool()) {
                    std::vector<TxEntry::TxEntryRef> to_remove = txgraph.GetDescendants({*all_entries[i]});
                    txgraph.RemoveBatch(to_remove);
                    for (size_t k=0; k<to_remove.size(); ++k) {
                        in_txgraph.erase(to_remove[k].get().unique_id);
                    }
                }
            }
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            // simulate connecting a block
            uint64_t num_transactions_to_select = fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(1, in_txgraph.size()/20+1);
            std::set<TxEntry::TxEntryRef, TxEntry::CompareById> selected_transactions;
            for (size_t i=0; i<all_entries.size(); ++i) {
                if (fuzzed_data_provider.ConsumeBool() && in_txgraph.count(all_entries[i]->unique_id)) {
                    std::vector<TxEntry::TxEntryRef> to_mine = txgraph.GetAncestors({*all_entries[i]});
                    selected_transactions.insert(to_mine.begin(), to_mine.end());
                    if (selected_transactions.size() >= num_transactions_to_select) {
                        break;
                    }
                }
            }
            if (selected_transactions.size() > 0) {
                std::vector<TxEntry::TxEntryRef> selected_transactions_vec(selected_transactions.begin(), selected_transactions.end());
                txgraph.RemoveBatch(selected_transactions_vec);
                for (size_t j=0; j<selected_transactions_vec.size(); ++j) {
                    in_txgraph.erase(selected_transactions_vec[j].get().unique_id);
                }
            }
            txgraph.Check(limits);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            // Test the TxSelector (mining code).
            TxSelector txselector(&txgraph);

            int64_t num_invocations = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(1, in_txgraph.size()/20+1);
            std::vector<TxEntry::TxEntryRef> selected_transactions;
            while (num_invocations > 0) {
                txselector.SelectNextChunk(selected_transactions);
                // TODO: add a check that the feerates never go up as we make
                // further calls.
                if (fuzzed_data_provider.ConsumeBool()) {
                    txselector.Success();
                }
                --num_invocations;
            }
            txgraph.RemoveBatch(selected_transactions);
            for (size_t k=0; k<selected_transactions.size(); ++k) {
                in_txgraph.erase(selected_transactions[k].get().unique_id);
            }
            // Check that the selected transactions are topologically valid.
            // Do this by dumping into a cluster, and running Cluster::Check
            TxGraphCluster dummy(-1, &txgraph);
            for (auto& tx : selected_transactions) {
                tx.get().m_cluster = &dummy;
                dummy.AddTransaction(tx, false);
            }
            dummy.Rechunk();
            assert(dummy.Check());
            txgraph.Check(limits);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            // Run eviction
            Trimmer trimmer(&txgraph);
            std::vector<TxEntry::TxEntryRef> removed;
            auto feerate = trimmer.RemoveWorstChunk(removed);

            // Check that the feerate matches what was removed
            CAmount total_fee{0};
            int32_t total_size{0};

            for (auto &entry : removed) {
                total_fee += entry.get().GetModifiedFee();
                total_size += entry.get().GetTxSize();
                in_txgraph.erase(entry.get().unique_id);
            }
            assert(feerate == CFeeRate(total_fee, total_size));
        }
        txgraph.Check(limits);
        if (fuzzed_data_provider.ConsumeBool()) {
            // do a reorg
            // Generate some random transactions and pick some existing random
            // transactions to have as children.
            int64_t num_to_add = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(1, 100);
            std::map<int64_t, std::vector<TxEntry::TxEntryRef>> children_map;
            std::vector<TxEntry::TxEntryRef> new_transactions;
            std::set<int64_t> new_transaction_ids;
            for (int k=0; k<num_to_add; ++k) {
                all_entries.emplace_back(new
                        TxEntry(fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(60,
                                100'000),
                            fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0,
                                1'000*100'000)));
                new_transactions.emplace_back(*all_entries.back());
                children_map.insert(std::make_pair(all_entries.back()->unique_id, std::vector<TxEntry::TxEntryRef>()));
                for (auto &id : in_txgraph) {
                    if (new_transaction_ids.count(id)) continue;
                    if (fuzzed_data_provider.ConsumeBool()) {
                        // Take advantage of the fact that the unique id's for
                        // each transaction correspond to the index in
                        // all_entries
                        for (auto entry : all_entries) {
                            if (entry->unique_id == id) {
                                children_map[all_entries.back()->unique_id].emplace_back(*entry);
                                break;
                            }
                        }
                    }
                }
                // Pick some random parents, amongst the set of new
                // transactions.
                std::vector<TxEntry::TxEntryRef> parents;
                for (size_t m=0; m<new_transactions.size()-1; ++m) {
                    if (fuzzed_data_provider.ConsumeBool()) {
                        parents.emplace_back(new_transactions[m]);
                    }
                }
                if (CheckClusterSizeLimits(txgraph, all_entries.back(), parents, limits)) {
                    txgraph.AddTx(all_entries.back(), all_entries.back()->GetTxSize(), all_entries.back()->GetModifiedFee(), parents);
                    txgraph.Check(limits);
                    in_txgraph.insert(all_entries.back()->unique_id);
                    new_transaction_ids.insert(all_entries.back()->unique_id);
                } else {
                    new_transactions.pop_back();
                }
            }
            std::vector<TxEntry::TxEntryRef> removed;
            txgraph.AddParentTxs(new_transactions, limits, [&](const TxEntry& tx) { return children_map[tx.unique_id]; }, removed);
            for (auto r : removed) {
                in_txgraph.erase(r.get().unique_id);
            }
            txgraph.Check(limits);
        }
    }
    txgraph.Check(limits);

    // free the memory that was used

    for (auto txentry : all_entries) {
        delete txentry;
    }
    all_entries.clear();
}
} // namespace
