// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/txgraph.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <random.h>

BOOST_AUTO_TEST_SUITE(txgraph_tests)

BOOST_AUTO_TEST_CASE(TxEntryTest)
{
    TxEntry entry(100, 100);
    BOOST_CHECK_EQUAL(entry.GetTxEntryParents().size(), 0);
    BOOST_CHECK_EQUAL(entry.GetTxEntryChildren().size(), 0);
    BOOST_CHECK_EQUAL(entry.GetTxSize(), 100);
    BOOST_CHECK_EQUAL(entry.GetModifiedFee(), 100);
    BOOST_CHECK(entry.m_cluster == nullptr);

    TxEntry entry2(200, 200);
    BOOST_CHECK_EQUAL(entry2.GetTxSize(), 200);
    BOOST_CHECK_EQUAL(entry2.GetModifiedFee(), 200);

    // If CompareById changes, then these tests may need to be changed as well.
    BOOST_CHECK(TxEntry::CompareById()(entry, entry2));
    BOOST_CHECK(!TxEntry::CompareById()(entry2, entry));

    entry2.GetTxEntryParents().insert(entry);
    entry.GetTxEntryChildren().insert(entry2);
    BOOST_CHECK_EQUAL(entry.GetTxEntryChildren().size(), 1);
    BOOST_CHECK_EQUAL(entry2.GetTxEntryParents().size(), 1);
}

BOOST_AUTO_TEST_CASE(TxGraphClusterAddRemoveTest)
{
    TxGraph dummy;
    TxGraphCluster cluster(1, &dummy);

    TxEntry entry1(100, 100);
    entry1.m_cluster = &cluster;
    cluster.AddTransaction(entry1, true);

    BOOST_CHECK_EQUAL(entry1.m_cluster, &cluster);
    BOOST_CHECK_EQUAL(entry1.m_loc.first, 0); // must be first chunk
    BOOST_CHECK_EQUAL(entry1.m_loc.second->get().unique_id, entry1.unique_id); // must be first chunk

    BOOST_CHECK_EQUAL(cluster.m_chunks.size(), 1);
    BOOST_CHECK_EQUAL(cluster.m_tx_count, 1);
    BOOST_CHECK_EQUAL(cluster.m_tx_size, 100);

    BOOST_CHECK(cluster.GetMemoryUsage() > 0);

    TxEntry entry2(1, 500);
    entry1.GetTxEntryChildren().insert(entry2);
    entry2.GetTxEntryParents().insert(entry1);
    entry2.m_cluster = &cluster;
    cluster.AddTransaction(entry2, true);

    BOOST_CHECK_EQUAL(entry2.m_cluster, &cluster);
    BOOST_CHECK_EQUAL(cluster.m_tx_count, 2);
    BOOST_CHECK_EQUAL(cluster.m_tx_size, 101);

    // Check that the topology is respected and entry1 is before entry2
    BOOST_CHECK(entry1.m_loc.first == 0);
    BOOST_CHECK(entry2.m_loc.first >= entry1.m_loc.first);
    if (entry2.m_loc.first == entry1.m_loc.first) {
        // If they are in the same chunk, then entry1 must be before entry2
        BOOST_CHECK(entry1.m_loc.second == cluster.m_chunks[0].txs.begin());
    }

    // Removing tx1 should make tx2 best.
    entry2.GetTxEntryParents().erase(entry1);
    entry1.GetTxEntryChildren().erase(entry2);
    cluster.RemoveTransaction(entry1);
    cluster.Sort();

    BOOST_CHECK_EQUAL(entry2.m_cluster, &cluster);
    BOOST_CHECK_EQUAL(entry2.m_loc.first, 0);
    BOOST_CHECK(entry2.m_loc.second == cluster.m_chunks[0].txs.begin());
    BOOST_CHECK_EQUAL(cluster.m_tx_count, 1);
    BOOST_CHECK_EQUAL(cluster.m_tx_size, 1);
    BOOST_CHECK(cluster.GetMemoryUsage() > 0);
}

BOOST_AUTO_TEST_CASE(TxGraphClusterRechunkTest)
{
    // Test that rechunking doesn't change the transaction order.
    TxGraph dummy;
    TxGraphCluster cluster(1, &dummy);

    // Add some transactions in a silly order.
    TxEntry entry1(100, 100);
    entry1.m_cluster = &cluster;
    cluster.AddTransaction(entry1, true);

    TxEntry entry2(1, 500);
    entry2.m_cluster = &cluster;
    cluster.AddTransaction(entry2, false);

    TxEntry entry3(200, 100);
    entry3.m_cluster = &cluster;
    cluster.AddTransaction(entry3, false);

    std::vector<TxEntry::TxEntryRef> expected;
    expected.emplace_back(entry1);
    expected.emplace_back(entry2);
    expected.emplace_back(entry3);

    // Check that the current order is entry1, entry2, entry3
    std::vector<TxEntry::TxEntryRef> linearized;
    for (auto &chunk : cluster.m_chunks) {
        for (auto tx : chunk.txs) {
            linearized.push_back(tx);
        }
    }
    BOOST_CHECK(linearized.size() == 3);
    for (size_t i=0; i <linearized.size(); ++i) {
        BOOST_CHECK(&linearized[i].get() == &expected[i].get());
    }

    cluster.Rechunk();

    std::vector<TxEntry::TxEntryRef> linearized2;
    for (auto &chunk : cluster.m_chunks) {
        for (auto tx : chunk.txs) {
            linearized2.push_back(tx);
        }
    }
    BOOST_CHECK(linearized2.size() == 3);
    for (size_t i=0; i <linearized2.size(); ++i) {
        BOOST_CHECK(&linearized2[i].get() == &expected[i].get());
    }
}

BOOST_AUTO_TEST_CASE(TxGraphClusterMergeTest)
{
    TxGraph dummy;

    TxGraphCluster cluster1(1, &dummy);
    TxGraphCluster cluster2(2, &dummy);
    TxGraphCluster cluster3(3, &dummy);

    std::vector<TxEntry> all_entries;
    all_entries.reserve(30);

    // Add some random transactions to each cluster.
    for (int i=0; i < 30; ++i) {
        all_entries.emplace_back(GetRand(1000)+1, GetRand(1000)+1);
        if (i < 10) {
            all_entries.back().m_cluster = &cluster1;
            cluster1.AddTransaction(all_entries.back(), true);
        } else if (i < 20) {
            all_entries.back().m_cluster = &cluster2;
            cluster2.AddTransaction(all_entries.back(), true);
        } else {
            all_entries.back().m_cluster = &cluster3;
            cluster3.AddTransaction(all_entries.back(), true);
        }
    }

    std::vector<TxEntry::TxEntryRef> linearized;
    for (auto &chunk : cluster1.m_chunks) {
        for (auto tx : chunk.txs) {
            linearized.push_back(tx);
        }
    }
    // Check that the ordering of transactions within each cluster is preserved
    // under the merge.
    std::vector<TxGraphCluster*> clusters = {&cluster2, &cluster3};
    cluster1.Merge(clusters.begin(), clusters.end(), false);
    std::vector<TxEntry::TxEntryRef> linearized2;
    for (auto &chunk : cluster1.m_chunks) {
        for (auto tx : chunk.txs) {
            linearized2.push_back(tx);
        }
    }
    std::vector<TxEntry::TxEntryRef>::iterator it1, it2;
    it1 = linearized.begin();
    it2 = linearized2.begin();
    while (it1 != linearized.end()) {
        BOOST_CHECK(it2 != linearized2.end());
        if (&(it1->get()) == &(it2->get())) {
            ++it1;
            ++it2;
        } else {
            ++it2;
        }
    }

    // Check that GetLastTransaction returns the correct item
    BOOST_CHECK(&(cluster1.GetLastTransaction().get()) == &linearized2.back().get());
}

BOOST_AUTO_TEST_CASE(TxGraphClusterSortTest)
{
    // Verify that parents always come before children, no matter what we stick
    // in a cluster.
    TxGraph dummy;

    TxGraphCluster cluster(1, &dummy);

    std::vector<TxEntry> all_entries;
    all_entries.reserve(30);

    // Create some random transactions.
    for (int i=0; i < 30; ++i) {
        all_entries.emplace_back(GetRand(1000)+1, GetRand(1000)+1);
        for (int j=0; j<i; ++j) {
            if (GetRand(4) == 0) {
                all_entries[i].GetTxEntryParents().insert(all_entries[j]);
                all_entries[j].GetTxEntryChildren().insert(all_entries[i]);
            }
        }
        all_entries.back().m_cluster = &cluster;
        cluster.AddTransaction(all_entries.back(), false);
    }

    cluster.Sort(true); // Ensure that we're invoking the Sort() call.

    BOOST_CHECK(cluster.Check());
}

BOOST_AUTO_TEST_CASE(TxGraphTest)
{
    TxGraph txgraph;
    GraphLimits limits{1'000'000'000, 1'000'000'000};

    std::vector<TxEntry*> all_entries;
    std::set<int64_t> in_mempool;

    // Test that the TxGraph stays consistent as we mix and match operations.
    for (int i=0; i<1000; ++i) {
        // Randomly choose an operation to perform.
        // 1. Add a loose transaction
        // 2. Remove a transaction and all its descendants.
        // 3. Remove a set of transactions for a block.
        // 4. Trim the worst chunk.
        // 5. Add back a set of confirmed transactions.
        int rand_val = GetRand(100);
        if (rand_val < 85) {
            // Add a random transaction, 85% of the time.
            TxEntry *entry = new TxEntry(GetRand(1000)+1, GetRand(1000)+1);
            all_entries.emplace_back(entry);
            std::vector<TxEntry::TxEntryRef> parents;
            for (size_t j=0; j<all_entries.size(); ++j) {
                if (GetRand(100) < 1 && in_mempool.count(all_entries[j]->unique_id)) {
                    parents.emplace_back(*all_entries[j]);
                }
            }
            txgraph.AddTx(entry, entry->GetTxSize(), entry->GetModifiedFee(), parents);
            in_mempool.insert(entry->unique_id);
        } else if (rand_val < 90 && in_mempool.size() > 0) {
            // RBF a transaction, 5% of the time.
            int64_t random_index = GetRand(all_entries.size());
            if (in_mempool.count(all_entries[random_index]->unique_id)) {
                TxEntry *entry = all_entries[random_index];
                std::vector<TxEntry::TxEntryRef> all_conflicts = txgraph.GetDescendants({*entry});
                GraphLimits limits{100, 1000000};
                TxGraphChangeSet changeset(&txgraph, limits, all_conflicts);
                TxEntry *new_entry = new TxEntry(GetRand(1000)+1, GetRand(1000)+1);
                all_entries.emplace_back(new_entry);
                std::vector<TxEntry::TxEntryRef> parents;
                for (size_t j=0; j<all_entries.size(); ++j) {
                    bool conflict{false};
                    for (auto &c: all_conflicts) {
                        if (c.get().unique_id == all_entries[j]->unique_id) {
                            conflict = true;
                            break;
                        }
                    }
                    if (conflict) continue;
                    if (GetRand(100) < 1 && in_mempool.count(all_entries[j]->unique_id)) {
                        parents.emplace_back(*all_entries[j]);
                    }
                }
                if (changeset.AddTx(*new_entry, parents)) {
                    std::vector<FeeFrac> diagram_dummy;
                    changeset.GetFeerateDiagramOld(diagram_dummy);
                    changeset.GetFeerateDiagramNew(diagram_dummy);
                    // Should do a diagram comparison here, but just apply 1/2 the time.
                    if (GetRand(100) < 50) {
                        in_mempool.insert(new_entry->unique_id);
                        for (auto &c: all_conflicts) {
                            in_mempool.erase(c.get().unique_id);
                        }
                        changeset.Apply();
                    }
                }
            }
        } else if (rand_val < 95 && in_mempool.size() > 0) {
            // Remove a random transaction and its descendants, 5% of the time.
            int64_t random_index = GetRand(all_entries.size());
            if (in_mempool.count(all_entries[random_index]->unique_id)) {
                std::vector<TxEntry::TxEntryRef> to_remove = txgraph.GetDescendants({*all_entries[random_index]});
                txgraph.RemoveBatch(to_remove);
                for (size_t k=0; k<to_remove.size(); ++k) {
                    in_mempool.erase(to_remove[k].get().unique_id);
                }
            }
        } else if (rand_val < 96 && in_mempool.size() > 0) {
            // Mine a "block", of 5% of the transactions.
            uint64_t num_to_remove = GetRand(in_mempool.size()+1) / 20;
            std::set<TxEntry::TxEntryRef, TxEntry::CompareById> selected_transactions;
            while (selected_transactions.size() < num_to_remove) {
                int64_t random_index = GetRand(all_entries.size());
                if (in_mempool.count(all_entries[random_index]->unique_id)) {
                    std::vector<TxEntry::TxEntryRef> to_mine = txgraph.GetAncestors({*all_entries[random_index]});
                    selected_transactions.insert(to_mine.begin(), to_mine.end());
                }
            }
            if (selected_transactions.size() > 0) {
                std::vector<TxEntry::TxEntryRef> selected_transactions_vec(selected_transactions.begin(), selected_transactions.end());
                txgraph.RemoveBatch(selected_transactions_vec);
                for (size_t k=0; k<selected_transactions_vec.size(); ++k) {
                    in_mempool.erase(selected_transactions_vec[k].get().unique_id);
                }
            }
        } else if (rand_val < 98 && in_mempool.size() > 0) {
            // Test the TxSelector (mining code).
            TxSelector txselector(&txgraph);

            int64_t num_invocations = GetRand(in_mempool.size()+1) / 20;
            std::vector<TxEntry::TxEntryRef> selected_transactions;
            while (num_invocations > 0) {
                txselector.SelectNextChunk(selected_transactions);
                // TODO: add a check that the feerates never go up as we make
                // further calls.
                txselector.Success();
                --num_invocations;
            }
            txgraph.RemoveBatch(selected_transactions);
            for (size_t k=0; k<selected_transactions.size(); ++k) {
                in_mempool.erase(selected_transactions[k].get().unique_id);
            }
            // Check that the selected transactions are topologically valid.
            // Do this by dumping into a cluster, and running Cluster::Check
            TxGraphCluster dummy(-1, &txgraph);
            for (auto& tx : selected_transactions) {
                tx.get().m_cluster = &dummy;
                dummy.AddTransaction(tx, false);
            }
            dummy.Rechunk();
            dummy.Check();
        } else if (rand_val < 99 && in_mempool.size() > 0) {
            // Reorg a block with probability 1%
            // Generate some random transactions and pick some existing random
            // transactions to have as children.
            int64_t num_to_add = GetRand(20)+1;
            std::map<int64_t, std::vector<TxEntry::TxEntryRef>> children_map;
            std::vector<TxEntry::TxEntryRef> new_transactions;
            for (int k=0; k<num_to_add; ++k) {
                all_entries.emplace_back(new TxEntry(GetRand(1000)+1, GetRand(1000)+1));
                new_transactions.emplace_back(*all_entries.back());
                children_map.insert(std::make_pair(all_entries.back()->unique_id, std::vector<TxEntry::TxEntryRef>()));
                for (auto &id : in_mempool) {
                    if (GetRand(100) < 10) {
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
                for (int m=0; m<k; ++m) {
                    if (GetRand(100) < 30) {
                        parents.emplace_back(new_transactions[m]);
                    }
                }
                txgraph.AddTx(all_entries.back(), all_entries.back()->GetTxSize(), all_entries.back()->GetModifiedFee(), parents);
            }
            std::vector<TxEntry::TxEntryRef> removed;
            txgraph.AddParentTxs(new_transactions, limits, [&](const TxEntry& tx) { return children_map[tx.unique_id]; }, removed);
            BOOST_CHECK(removed.size() == 0); // no limits should be hit
        } else if (in_mempool.size() > 0) {
            // Trim the worst chunk with probability 1%
            Trimmer trimmer(&txgraph);
            std::vector<TxEntry::TxEntryRef> removed;
            auto feerate = trimmer.RemoveWorstChunk(removed);
            // Check that the feerate matches what was removed
            CAmount total_fee{0};
            int32_t total_size{0};

            for (auto &entry : removed) {
                total_fee += entry.get().GetModifiedFee();
                total_size += entry.get().GetTxSize();
                in_mempool.erase(entry.get().unique_id);
            }
            BOOST_CHECK(feerate == CFeeRate(total_fee, total_size));
        }
        txgraph.Check(limits);
    }

    for (auto txentry : all_entries) {
        delete txentry;
    }
    all_entries.clear();
}

BOOST_AUTO_TEST_SUITE_END()
