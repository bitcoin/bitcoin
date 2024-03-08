// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <common/system.h>
#include <policy/rbf.h>
#include <random.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/time.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <optional>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(rbf_tests, TestingSetup)

static inline CTransactionRef make_tx(const std::vector<CTransactionRef>& inputs,
                                      const std::vector<CAmount>& output_values)
{
    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(inputs.size());
    tx.vout.resize(output_values.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
        tx.vin[i].prevout.hash = inputs[i]->GetHash();
        tx.vin[i].prevout.n = 0;
        // Add a witness so wtxid != txid
        CScriptWitness witness;
        witness.stack.emplace_back(i + 10);
        tx.vin[i].scriptWitness = witness;
    }
    for (size_t i = 0; i < output_values.size(); ++i) {
        tx.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx.vout[i].nValue = output_values[i];
    }
    return MakeTransactionRef(tx);
}

static void add_descendants(const CTransactionRef& tx, int32_t num_descendants, CTxMemPool& pool)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main, pool.cs)
{
    AssertLockHeld(::cs_main);
    AssertLockHeld(pool.cs);
    TestMemPoolEntryHelper entry;
    // Assumes this isn't already spent in mempool
    auto tx_to_spend = tx;
    for (int32_t i{0}; i < num_descendants; ++i) {
        auto next_tx = make_tx(/*inputs=*/{tx_to_spend}, /*output_values=*/{(50 - i) * CENT});
        pool.addUnchecked(entry.FromTx(next_tx));
        tx_to_spend = next_tx;
    }
}

BOOST_FIXTURE_TEST_CASE(rbf_helper_functions, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    const CAmount low_fee{CENT/100};
    const CAmount normal_fee{CENT/10};
    const CAmount high_fee{CENT};

    // Create a parent tx1 and child tx2 with normal fees:
    const auto tx1 = make_tx(/*inputs=*/ {m_coinbase_txns[0]}, /*output_values=*/ {10 * COIN});
    pool.addUnchecked(entry.Fee(normal_fee).FromTx(tx1));
    const auto tx2 = make_tx(/*inputs=*/ {tx1}, /*output_values=*/ {995 * CENT});
    pool.addUnchecked(entry.Fee(normal_fee).FromTx(tx2));

    // Create a low-feerate parent tx3 and high-feerate child tx4 (cpfp)
    const auto tx3 = make_tx(/*inputs=*/ {m_coinbase_txns[1]}, /*output_values=*/ {1099 * CENT});
    pool.addUnchecked(entry.Fee(low_fee).FromTx(tx3));
    const auto tx4 = make_tx(/*inputs=*/ {tx3}, /*output_values=*/ {999 * CENT});
    pool.addUnchecked(entry.Fee(high_fee).FromTx(tx4));

    // Create a parent tx5 and child tx6 where both have very low fees
    const auto tx5 = make_tx(/*inputs=*/ {m_coinbase_txns[2]}, /*output_values=*/ {1099 * CENT});
    pool.addUnchecked(entry.Fee(low_fee).FromTx(tx5));
    const auto tx6 = make_tx(/*inputs=*/ {tx5}, /*output_values=*/ {1098 * CENT});
    pool.addUnchecked(entry.Fee(low_fee).FromTx(tx6));
    // Make tx6's modified fee much higher than its base fee. This should cause it to pass
    // the fee-related checks despite being low-feerate.
    pool.PrioritiseTransaction(tx6->GetHash(), 1 * COIN);

    // Two independent high-feerate transactions, tx7 and tx8
    const auto tx7 = make_tx(/*inputs=*/ {m_coinbase_txns[3]}, /*output_values=*/ {999 * CENT});
    pool.addUnchecked(entry.Fee(high_fee).FromTx(tx7));
    const auto tx8 = make_tx(/*inputs=*/ {m_coinbase_txns[4]}, /*output_values=*/ {999 * CENT});
    pool.addUnchecked(entry.Fee(high_fee).FromTx(tx8));

    const auto entry1 = pool.GetIter(tx1->GetHash()).value();
    const auto entry2 = pool.GetIter(tx2->GetHash()).value();
    const auto entry3 = pool.GetIter(tx3->GetHash()).value();
    const auto entry4 = pool.GetIter(tx4->GetHash()).value();
    const auto entry5 = pool.GetIter(tx5->GetHash()).value();
    const auto entry6 = pool.GetIter(tx6->GetHash()).value();
    const auto entry7 = pool.GetIter(tx7->GetHash()).value();
    const auto entry8 = pool.GetIter(tx8->GetHash()).value();

    BOOST_CHECK_EQUAL(entry1->GetFee(), normal_fee);
    BOOST_CHECK_EQUAL(entry2->GetFee(), normal_fee);
    BOOST_CHECK_EQUAL(entry3->GetFee(), low_fee);
    BOOST_CHECK_EQUAL(entry4->GetFee(), high_fee);
    BOOST_CHECK_EQUAL(entry5->GetFee(), low_fee);
    BOOST_CHECK_EQUAL(entry6->GetFee(), low_fee);
    BOOST_CHECK_EQUAL(entry7->GetFee(), high_fee);
    BOOST_CHECK_EQUAL(entry8->GetFee(), high_fee);

    CTxMemPool::setEntries set_12_normal{entry1, entry2};
    CTxMemPool::setEntries set_34_cpfp{entry3, entry4};
    CTxMemPool::setEntries set_56_low{entry5, entry6};
    CTxMemPool::setEntries all_entries{entry1, entry2, entry3, entry4, entry5, entry6, entry7, entry8};
    CTxMemPool::setEntries empty_set;

    const auto unused_txid{GetRandHash()};

    // Tests for EntriesAndTxidsDisjoint
    BOOST_CHECK(EntriesAndTxidsDisjoint(empty_set, {tx1->GetHash()}, unused_txid) == std::nullopt);
    BOOST_CHECK(EntriesAndTxidsDisjoint(set_12_normal, {tx3->GetHash()}, unused_txid) == std::nullopt);
    BOOST_CHECK(EntriesAndTxidsDisjoint({entry2}, {tx2->GetHash()}, unused_txid).has_value());
    BOOST_CHECK(EntriesAndTxidsDisjoint(set_12_normal, {tx1->GetHash()}, unused_txid).has_value());
    BOOST_CHECK(EntriesAndTxidsDisjoint(set_12_normal, {tx2->GetHash()}, unused_txid).has_value());
    // EntriesAndTxidsDisjoint does not calculate descendants of iters_conflicting; it uses whatever
    // the caller passed in. As such, no error is returned even though entry2 is a descendant of tx1.
    BOOST_CHECK(EntriesAndTxidsDisjoint({entry2}, {tx1->GetHash()}, unused_txid) == std::nullopt);

    // Tests for PaysForRBF
    const CFeeRate incremental_relay_feerate{DEFAULT_INCREMENTAL_RELAY_FEE};
    const CFeeRate higher_relay_feerate{2 * DEFAULT_INCREMENTAL_RELAY_FEE};
    // Must pay at least as much as the original.
    BOOST_CHECK(PaysForRBF(/*original_fees=*/high_fee,
                           /*replacement_fees=*/high_fee,
                           /*replacement_vsize=*/1,
                           /*relay_fee=*/CFeeRate(0),
                           /*txid=*/unused_txid)
                           == std::nullopt);
    BOOST_CHECK(PaysForRBF(high_fee, high_fee - 1, 1, CFeeRate(0), unused_txid).has_value());
    BOOST_CHECK(PaysForRBF(high_fee + 1, high_fee, 1, CFeeRate(0), unused_txid).has_value());
    // Additional fees must cover the replacement's vsize at incremental relay fee
    BOOST_CHECK(PaysForRBF(high_fee, high_fee + 1, 2, incremental_relay_feerate, unused_txid).has_value());
    BOOST_CHECK(PaysForRBF(high_fee, high_fee + 2, 2, incremental_relay_feerate, unused_txid) == std::nullopt);
    BOOST_CHECK(PaysForRBF(high_fee, high_fee + 2, 2, higher_relay_feerate, unused_txid).has_value());
    BOOST_CHECK(PaysForRBF(high_fee, high_fee + 4, 2, higher_relay_feerate, unused_txid) == std::nullopt);
    BOOST_CHECK(PaysForRBF(low_fee, high_fee, 99999999, incremental_relay_feerate, unused_txid).has_value());
    BOOST_CHECK(PaysForRBF(low_fee, high_fee + 99999999, 99999999, incremental_relay_feerate, unused_txid) == std::nullopt);

    // Tests for GetEntriesForConflicts
    CTxMemPool::setEntries all_parents{entry1, entry3, entry5, entry7, entry8};
    CTxMemPool::setEntries all_children{entry2, entry4, entry6};
    const std::vector<CTransactionRef> parent_inputs({m_coinbase_txns[0], m_coinbase_txns[1], m_coinbase_txns[2],
                                                m_coinbase_txns[3], m_coinbase_txns[4]});
    const auto conflicts_with_parents = make_tx(parent_inputs, {50 * CENT});
    CTxMemPool::setEntries all_conflicts;
    BOOST_CHECK(GetEntriesForConflicts(/*tx=*/ *conflicts_with_parents.get(),
                                       /*pool=*/ pool,
                                       /*iters_conflicting=*/ all_parents,
                                       /*all_conflicts=*/ all_conflicts) == std::nullopt);
    BOOST_CHECK(all_conflicts == all_entries);
    auto conflicts_size = all_conflicts.size();
    all_conflicts.clear();

    add_descendants(tx2, 23, pool);
    BOOST_CHECK(GetEntriesForConflicts(*conflicts_with_parents.get(), pool, all_parents, all_conflicts) == std::nullopt);
    conflicts_size += 23;
    BOOST_CHECK_EQUAL(all_conflicts.size(), conflicts_size);
    all_conflicts.clear();

    add_descendants(tx4, 23, pool);
    BOOST_CHECK(GetEntriesForConflicts(*conflicts_with_parents.get(), pool, all_parents, all_conflicts) == std::nullopt);
    conflicts_size += 23;
    BOOST_CHECK_EQUAL(all_conflicts.size(), conflicts_size);
    all_conflicts.clear();

    add_descendants(tx6, 23, pool);
    BOOST_CHECK(GetEntriesForConflicts(*conflicts_with_parents.get(), pool, all_parents, all_conflicts) == std::nullopt);
    conflicts_size += 23;
    BOOST_CHECK_EQUAL(all_conflicts.size(), conflicts_size);
    all_conflicts.clear();

    add_descendants(tx7, 23, pool);
    BOOST_CHECK(GetEntriesForConflicts(*conflicts_with_parents.get(), pool, all_parents, all_conflicts) == std::nullopt);
    conflicts_size += 23;
    BOOST_CHECK_EQUAL(all_conflicts.size(), conflicts_size);
    BOOST_CHECK_EQUAL(all_conflicts.size(), 100);
    all_conflicts.clear();

    // If we treat all_conflicts as being direct conflicts, then we should exceed the replacement limit.
    add_descendants(tx8, 1, pool);
    BOOST_CHECK(GetEntriesForConflicts(*conflicts_with_parents.get(), pool, all_parents, all_conflicts) == std::nullopt);
    BOOST_CHECK_EQUAL(all_conflicts.size(), 101);

    CTxMemPool::setEntries dummy;
    BOOST_CHECK(GetEntriesForConflicts(*conflicts_with_parents.get(), pool, all_conflicts, dummy).has_value());
}

BOOST_AUTO_TEST_CASE(feerate_diagram_utilities)
{
    // Sanity check the correctness of the feerate diagram comparison.

    // A strictly better case.
    std::vector<FeeFrac> old_diagram{{FeeFrac{0, 0}, FeeFrac{950, 300}, FeeFrac{1050, 400}}};
    std::vector<FeeFrac> new_diagram{{FeeFrac{0, 0}, FeeFrac{1000, 300}, FeeFrac{1050, 400}}};

    BOOST_CHECK(CompareFeeSizeDiagram(old_diagram, new_diagram));
    BOOST_CHECK(!CompareFeeSizeDiagram(new_diagram, old_diagram));

    // Incomparable diagrams
    old_diagram = {FeeFrac{0, 0}, FeeFrac{950, 300}, FeeFrac{1050, 400}};
    new_diagram = {FeeFrac{0, 0}, FeeFrac{1000, 300}, FeeFrac{1000, 400}};

    BOOST_CHECK(!CompareFeeSizeDiagram(old_diagram, new_diagram));
    BOOST_CHECK(!CompareFeeSizeDiagram(new_diagram, old_diagram));

    // Strictly better but smaller size.
    old_diagram = {FeeFrac{0, 0}, FeeFrac{950, 300}, FeeFrac{1050, 400}};
    new_diagram = {FeeFrac{0, 0}, FeeFrac{1100, 300}};

    BOOST_CHECK(CompareFeeSizeDiagram(old_diagram, new_diagram));
    BOOST_CHECK(!CompareFeeSizeDiagram(new_diagram, old_diagram));

    // Feerate of first chunk is sufficiently better, but second chunk is worse.
    old_diagram = {FeeFrac{0, 0}, FeeFrac{950, 300}, FeeFrac{1050, 400}};
    new_diagram = {FeeFrac{0, 0}, FeeFrac{1100, 100}, FeeFrac{1100, 200}};

    BOOST_CHECK(CompareFeeSizeDiagram(old_diagram, new_diagram));
    BOOST_CHECK(!CompareFeeSizeDiagram(new_diagram, old_diagram));

    // Feerate of first chunk is better, but second chunk is worse
    old_diagram = {FeeFrac{0, 0}, FeeFrac{950, 300}, FeeFrac{1050, 400}};
    new_diagram = {FeeFrac{0, 0}, FeeFrac{750, 100}, FeeFrac{999, 350}, FeeFrac{1150, 500}};

    BOOST_CHECK(!CompareFeeSizeDiagram(old_diagram, new_diagram));
    BOOST_CHECK(!CompareFeeSizeDiagram(new_diagram, old_diagram));

    // If we make the second chunk slightly better, the new diagram now wins.
    old_diagram = {FeeFrac{0, 0}, FeeFrac{950, 300}, FeeFrac{1050, 400}};
    new_diagram = {FeeFrac{0, 0}, FeeFrac{750, 100}, FeeFrac{1000, 350}, FeeFrac{1150, 500}};

    BOOST_CHECK(CompareFeeSizeDiagram(old_diagram, new_diagram));
    BOOST_CHECK(!CompareFeeSizeDiagram(new_diagram, old_diagram));
}

BOOST_AUTO_TEST_SUITE_END()
