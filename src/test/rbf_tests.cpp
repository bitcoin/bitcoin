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

// Make two child transactions from parent (which must have at least 2 outputs).
// Each tx will have the same outputs, using the amounts specified in output_values.
static inline std::pair<CTransactionRef, CTransactionRef> make_two_siblings(const CTransactionRef parent,
                                      const std::vector<CAmount>& output_values)
{
    assert(parent->vout.size() >= 2);

    // First tx takes first parent output
    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vin.resize(1);
    tx1.vout.resize(output_values.size());

    tx1.vin[0].prevout.hash = parent->GetHash();
    tx1.vin[0].prevout.n = 0;
    // Add a witness so wtxid != txid
    CScriptWitness witness;
    witness.stack.emplace_back(10);
    tx1.vin[0].scriptWitness = witness;

    for (size_t i = 0; i < output_values.size(); ++i) {
        tx1.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx1.vout[i].nValue = output_values[i];
    }

    // Second tx takes second parent output
    CMutableTransaction tx2 = tx1;
    tx2.vin[0].prevout.n = 1;

    return std::make_pair(MakeTransactionRef(tx1), MakeTransactionRef(tx2));
}

static CTransactionRef add_descendants(const CTransactionRef& tx, int32_t num_descendants, CTxMemPool& pool)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main, pool.cs)
{
    AssertLockHeld(::cs_main);
    AssertLockHeld(pool.cs);
    TestMemPoolEntryHelper entry;
    // Assumes this isn't already spent in mempool
    auto tx_to_spend = tx;
    for (int32_t i{0}; i < num_descendants; ++i) {
        auto next_tx = make_tx(/*inputs=*/{tx_to_spend}, /*output_values=*/{(50 - i) * CENT});
        AddToMempool(pool, entry.FromTx(next_tx));
        tx_to_spend = next_tx;
    }
    // Return last created tx
    return tx_to_spend;
}

static CTransactionRef add_descendant_to_parents(const std::vector<CTransactionRef>& parents, CTxMemPool& pool)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main, pool.cs)
{
    AssertLockHeld(::cs_main);
    AssertLockHeld(pool.cs);
    TestMemPoolEntryHelper entry;
    // Assumes this isn't already spent in mempool
    auto child_tx = make_tx(/*inputs=*/parents, /*output_values=*/{50 * CENT});
    AddToMempool(pool, entry.FromTx(child_tx));
    // Return last created tx
    return child_tx;
}

// Makes two children for a single parent
static std::pair<CTransactionRef, CTransactionRef> add_children_to_parent(const CTransactionRef parent, CTxMemPool& pool)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main, pool.cs)
{
    AssertLockHeld(::cs_main);
    AssertLockHeld(pool.cs);
    TestMemPoolEntryHelper entry;
    // Assumes this isn't already spent in mempool
    auto children_tx = make_two_siblings(/*parent=*/parent, /*output_values=*/{50 * CENT});
    AddToMempool(pool, entry.FromTx(children_tx.first));
    AddToMempool(pool, entry.FromTx(children_tx.second));
    return children_tx;
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
    AddToMempool(pool, entry.Fee(normal_fee).FromTx(tx1));
    const auto tx2 = make_tx(/*inputs=*/ {tx1}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(normal_fee).FromTx(tx2));

    // Create a low-feerate parent tx3 and high-feerate child tx4 (cpfp)
    const auto tx3 = make_tx(/*inputs=*/ {m_coinbase_txns[1]}, /*output_values=*/ {1099 * CENT});
    AddToMempool(pool, entry.Fee(low_fee).FromTx(tx3));
    const auto tx4 = make_tx(/*inputs=*/ {tx3}, /*output_values=*/ {999 * CENT});
    AddToMempool(pool, entry.Fee(high_fee).FromTx(tx4));

    // Create a parent tx5 and child tx6 where both have very low fees
    const auto tx5 = make_tx(/*inputs=*/ {m_coinbase_txns[2]}, /*output_values=*/ {1099 * CENT});
    AddToMempool(pool, entry.Fee(low_fee).FromTx(tx5));
    const auto tx6 = make_tx(/*inputs=*/ {tx5}, /*output_values=*/ {1098 * CENT});
    AddToMempool(pool, entry.Fee(low_fee).FromTx(tx6));
    // Make tx6's modified fee much higher than its base fee. This should cause it to pass
    // the fee-related checks despite being low-feerate.
    pool.PrioritiseTransaction(tx6->GetHash(), 1 * COIN);

    // Two independent high-feerate transactions, tx7 and tx8
    const auto tx7 = make_tx(/*inputs=*/ {m_coinbase_txns[3]}, /*output_values=*/ {999 * CENT});
    AddToMempool(pool, entry.Fee(high_fee).FromTx(tx7));
    const auto tx8 = make_tx(/*inputs=*/ {m_coinbase_txns[4]}, /*output_values=*/ {999 * CENT});
    AddToMempool(pool, entry.Fee(high_fee).FromTx(tx8));

    // Normal txs, will chain txns right before CheckConflictTopology test
    const auto tx9 = make_tx(/*inputs=*/ {m_coinbase_txns[5]}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(normal_fee).FromTx(tx9));
    const auto tx10 = make_tx(/*inputs=*/ {m_coinbase_txns[6]}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(normal_fee).FromTx(tx10));

    // Will make these two parents of single child
    const auto tx11 = make_tx(/*inputs=*/ {m_coinbase_txns[7]}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(normal_fee).FromTx(tx11));
    const auto tx12 = make_tx(/*inputs=*/ {m_coinbase_txns[8]}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(normal_fee).FromTx(tx12));

    // Will make two children of this single parent
    const auto tx13 = make_tx(/*inputs=*/ {m_coinbase_txns[9]}, /*output_values=*/ {995 * CENT, 995 * CENT});
    AddToMempool(pool, entry.Fee(normal_fee).FromTx(tx13));

    const auto entry1_normal = pool.GetIter(tx1->GetHash()).value();
    const auto entry2_normal = pool.GetIter(tx2->GetHash()).value();
    const auto entry3_low = pool.GetIter(tx3->GetHash()).value();
    const auto entry4_high = pool.GetIter(tx4->GetHash()).value();
    const auto entry5_low = pool.GetIter(tx5->GetHash()).value();
    const auto entry6_low_prioritised = pool.GetIter(tx6->GetHash()).value();
    const auto entry7_high = pool.GetIter(tx7->GetHash()).value();
    const auto entry8_high = pool.GetIter(tx8->GetHash()).value();
    const auto entry9_unchained = pool.GetIter(tx9->GetHash()).value();
    const auto entry10_unchained = pool.GetIter(tx10->GetHash()).value();
    const auto entry11_unchained = pool.GetIter(tx11->GetHash()).value();
    const auto entry12_unchained = pool.GetIter(tx12->GetHash()).value();
    const auto entry13_unchained = pool.GetIter(tx13->GetHash()).value();

    BOOST_CHECK_EQUAL(entry1_normal->GetFee(), normal_fee);
    BOOST_CHECK_EQUAL(entry2_normal->GetFee(), normal_fee);
    BOOST_CHECK_EQUAL(entry3_low->GetFee(), low_fee);
    BOOST_CHECK_EQUAL(entry4_high->GetFee(), high_fee);
    BOOST_CHECK_EQUAL(entry5_low->GetFee(), low_fee);
    BOOST_CHECK_EQUAL(entry6_low_prioritised->GetFee(), low_fee);
    BOOST_CHECK_EQUAL(entry7_high->GetFee(), high_fee);
    BOOST_CHECK_EQUAL(entry8_high->GetFee(), high_fee);

    CTxMemPool::setEntries set_12_normal{entry1_normal, entry2_normal};
    CTxMemPool::setEntries set_34_cpfp{entry3_low, entry4_high};
    CTxMemPool::setEntries set_56_low{entry5_low, entry6_low_prioritised};
    CTxMemPool::setEntries set_78_high{entry7_high, entry8_high};
    CTxMemPool::setEntries all_entries{entry1_normal, entry2_normal, entry3_low, entry4_high,
                                       entry5_low, entry6_low_prioritised, entry7_high, entry8_high};
    CTxMemPool::setEntries empty_set;

    const auto unused_txid{GetRandHash()};

    // Tests for PaysMoreThanConflicts
    // These tests use feerate, not absolute fee.
    BOOST_CHECK(PaysMoreThanConflicts(/*iters_conflicting=*/set_12_normal,
                                      /*replacement_feerate=*/CFeeRate(entry1_normal->GetModifiedFee() + 1, entry1_normal->GetTxSize() + 2),
                                      /*txid=*/unused_txid).has_value());
    // Replacement must be strictly greater than the originals.
    BOOST_CHECK(PaysMoreThanConflicts(set_12_normal, CFeeRate(entry1_normal->GetModifiedFee(), entry1_normal->GetTxSize()), unused_txid).has_value());
    BOOST_CHECK(PaysMoreThanConflicts(set_12_normal, CFeeRate(entry1_normal->GetModifiedFee() + 1, entry1_normal->GetTxSize()), unused_txid) == std::nullopt);
    // These tests use modified fees (including prioritisation), not base fees.
    BOOST_CHECK(PaysMoreThanConflicts({entry5_low}, CFeeRate(entry5_low->GetModifiedFee() + 1, entry5_low->GetTxSize()), unused_txid) == std::nullopt);
    BOOST_CHECK(PaysMoreThanConflicts({entry6_low_prioritised}, CFeeRate(entry6_low_prioritised->GetFee() + 1, entry6_low_prioritised->GetTxSize()), unused_txid).has_value());
    BOOST_CHECK(PaysMoreThanConflicts({entry6_low_prioritised}, CFeeRate(entry6_low_prioritised->GetModifiedFee() + 1, entry6_low_prioritised->GetTxSize()), unused_txid) == std::nullopt);
    // PaysMoreThanConflicts checks individual feerate, not ancestor feerate. This test compares
    // replacement_feerate and entry4_high's feerate, which are the same. The replacement_feerate is
    // considered too low even though entry4_high has a low ancestor feerate.
    BOOST_CHECK(PaysMoreThanConflicts(set_34_cpfp, CFeeRate(entry4_high->GetModifiedFee(), entry4_high->GetTxSize()), unused_txid).has_value());

    // Tests for EntriesAndTxidsDisjoint
    BOOST_CHECK(EntriesAndTxidsDisjoint(empty_set, {tx1->GetHash()}, unused_txid) == std::nullopt);
    BOOST_CHECK(EntriesAndTxidsDisjoint(set_12_normal, {tx3->GetHash()}, unused_txid) == std::nullopt);
    BOOST_CHECK(EntriesAndTxidsDisjoint({entry2_normal}, {tx2->GetHash()}, unused_txid).has_value());
    BOOST_CHECK(EntriesAndTxidsDisjoint(set_12_normal, {tx1->GetHash()}, unused_txid).has_value());
    BOOST_CHECK(EntriesAndTxidsDisjoint(set_12_normal, {tx2->GetHash()}, unused_txid).has_value());
    // EntriesAndTxidsDisjoint does not calculate descendants of iters_conflicting; it uses whatever
    // the caller passed in. As such, no error is returned even though entry2_normal is a descendant of tx1.
    BOOST_CHECK(EntriesAndTxidsDisjoint({entry2_normal}, {tx1->GetHash()}, unused_txid) == std::nullopt);

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
    CTxMemPool::setEntries all_parents{entry1_normal, entry3_low, entry5_low, entry7_high, entry8_high};
    CTxMemPool::setEntries all_children{entry2_normal, entry4_high, entry6_low_prioritised};
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

    // Exceeds maximum number of conflicts.
    add_descendants(tx8, 1, pool);
    BOOST_CHECK(GetEntriesForConflicts(*conflicts_with_parents.get(), pool, all_parents, all_conflicts).has_value());

    // Tests for HasNoNewUnconfirmed
    const auto spends_unconfirmed = make_tx({tx1}, {36 * CENT});
    for (const auto& input : spends_unconfirmed->vin) {
        // Spends unconfirmed inputs.
        BOOST_CHECK(pool.exists(GenTxid::Txid(input.prevout.hash)));
    }
    BOOST_CHECK(HasNoNewUnconfirmed(/*tx=*/ *spends_unconfirmed.get(),
                                    /*pool=*/ pool,
                                    /*iters_conflicting=*/ all_entries) == std::nullopt);
    BOOST_CHECK(HasNoNewUnconfirmed(*spends_unconfirmed.get(), pool, {entry2_normal}) == std::nullopt);
    BOOST_CHECK(HasNoNewUnconfirmed(*spends_unconfirmed.get(), pool, empty_set).has_value());

    const auto spends_new_unconfirmed = make_tx({tx1, tx8}, {36 * CENT});
    BOOST_CHECK(HasNoNewUnconfirmed(*spends_new_unconfirmed.get(), pool, {entry2_normal}).has_value());
    BOOST_CHECK(HasNoNewUnconfirmed(*spends_new_unconfirmed.get(), pool, all_entries).has_value());

    const auto spends_conflicting_confirmed = make_tx({m_coinbase_txns[0], m_coinbase_txns[1]}, {45 * CENT});
    BOOST_CHECK(HasNoNewUnconfirmed(*spends_conflicting_confirmed.get(), pool, {entry1_normal, entry3_low}) == std::nullopt);

    // Tests for CheckConflictTopology

    // Tx4 has 23 descendants
    BOOST_CHECK_EQUAL(pool.CheckConflictTopology(set_34_cpfp).value(), strprintf("%s has 24 descendants, max 1 allowed", entry3_low->GetSharedTx()->GetHash().ToString()));

    // No descendants yet
    BOOST_CHECK(pool.CheckConflictTopology({entry9_unchained}) == std::nullopt);

    // Add 1 descendant, still ok
    add_descendants(tx9, 1, pool);
    BOOST_CHECK(pool.CheckConflictTopology({entry9_unchained}) == std::nullopt);

    // N direct conflicts; ok
    BOOST_CHECK(pool.CheckConflictTopology({entry9_unchained, entry10_unchained, entry11_unchained}) == std::nullopt);

    // Add 1 descendant, still ok, even if it's considered a direct conflict as well
    const auto child_tx = add_descendants(tx10, 1, pool);
    const auto entry10_child = pool.GetIter(child_tx->GetHash()).value();
    BOOST_CHECK(pool.CheckConflictTopology({entry9_unchained, entry10_unchained, entry11_unchained}) == std::nullopt);
    BOOST_CHECK(pool.CheckConflictTopology({entry9_unchained, entry10_unchained, entry11_unchained, entry10_child}) == std::nullopt);

    // One more, size 3 cluster too much
    const auto grand_child_tx = add_descendants(child_tx, 1, pool);
    const auto entry10_grand_child = pool.GetIter(grand_child_tx->GetHash()).value();
    BOOST_CHECK_EQUAL(pool.CheckConflictTopology({entry9_unchained, entry10_unchained, entry11_unchained}).value(), strprintf("%s has 2 descendants, max 1 allowed", entry10_unchained->GetSharedTx()->GetHash().ToString()));
    // even if direct conflict is descendent itself
    BOOST_CHECK_EQUAL(pool.CheckConflictTopology({entry9_unchained, entry10_grand_child, entry11_unchained}).value(), strprintf("%s has 2 ancestors, max 1 allowed", entry10_grand_child->GetSharedTx()->GetHash().ToString()));

    // Make a single child from two singleton parents
    const auto two_parent_child_tx = add_descendant_to_parents({tx11, tx12}, pool);
    const auto entry_two_parent_child = pool.GetIter(two_parent_child_tx->GetHash()).value();
    BOOST_CHECK_EQUAL(pool.CheckConflictTopology({entry11_unchained}).value(), strprintf("%s is not the only parent of child %s", entry11_unchained->GetSharedTx()->GetHash().ToString(), entry_two_parent_child->GetSharedTx()->GetHash().ToString()));
    BOOST_CHECK_EQUAL(pool.CheckConflictTopology({entry12_unchained}).value(), strprintf("%s is not the only parent of child %s", entry12_unchained->GetSharedTx()->GetHash().ToString(), entry_two_parent_child->GetSharedTx()->GetHash().ToString()));
    BOOST_CHECK_EQUAL(pool.CheckConflictTopology({entry_two_parent_child}).value(), strprintf("%s has 2 ancestors, max 1 allowed", entry_two_parent_child->GetSharedTx()->GetHash().ToString()));

    // Single parent with two children, we will conflict with the siblings directly only
    const auto two_siblings = add_children_to_parent(tx13, pool);
    const auto entry_sibling_1 = pool.GetIter(two_siblings.first->GetHash()).value();
    const auto entry_sibling_2 = pool.GetIter(two_siblings.second->GetHash()).value();
    BOOST_CHECK_EQUAL(pool.CheckConflictTopology({entry_sibling_1}).value(), strprintf("%s is not the only child of parent %s", entry_sibling_1->GetSharedTx()->GetHash().ToString(), entry13_unchained->GetSharedTx()->GetHash().ToString()));
    BOOST_CHECK_EQUAL(pool.CheckConflictTopology({entry_sibling_2}).value(), strprintf("%s is not the only child of parent %s", entry_sibling_2->GetSharedTx()->GetHash().ToString(), entry13_unchained->GetSharedTx()->GetHash().ToString()));

}

BOOST_FIXTURE_TEST_CASE(improves_feerate, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    const CAmount low_fee{CENT/100};
    const CAmount normal_fee{CENT/10};

    // low feerate parent with normal feerate child
    const auto tx1 = make_tx(/*inputs=*/ {m_coinbase_txns[0], m_coinbase_txns[1]}, /*output_values=*/ {10 * COIN});
    AddToMempool(pool, entry.Fee(low_fee).FromTx(tx1));
    const auto tx2 = make_tx(/*inputs=*/ {tx1}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(normal_fee).FromTx(tx2));

    const auto entry1 = pool.GetIter(tx1->GetHash()).value();
    const auto tx1_fee = entry1->GetModifiedFee();
    const auto entry2 = pool.GetIter(tx2->GetHash()).value();
    const auto tx2_fee = entry2->GetModifiedFee();

    // conflicting transactions
    const auto tx1_conflict = make_tx(/*inputs=*/ {m_coinbase_txns[0], m_coinbase_txns[2]}, /*output_values=*/ {10 * COIN});
    const auto tx3 = make_tx(/*inputs=*/ {tx1_conflict}, /*output_values=*/ {995 * CENT});
    auto entry3 = entry.FromTx(tx3);

    // Now test ImprovesFeerateDiagram with various levels of "package rbf" feerates

    // It doesn't improve itself
    auto changeset = pool.GetChangeSet();
    changeset->StageRemoval(entry1);
    changeset->StageRemoval(entry2);
    changeset->StageAddition(tx1_conflict, tx1_fee, 0, 1, 0, false, 4, LockPoints());
    changeset->StageAddition(tx3, tx2_fee, 0, 1, 0, false, 4, LockPoints());
    const auto res1 = ImprovesFeerateDiagram(*changeset);
    BOOST_CHECK(res1.has_value());
    BOOST_CHECK(res1.value().first == DiagramCheckError::FAILURE);
    BOOST_CHECK(res1.value().second == "insufficient feerate: does not improve feerate diagram");

    // With one more satoshi it does
    changeset.reset();
    changeset = pool.GetChangeSet();
    changeset->StageRemoval(entry1);
    changeset->StageRemoval(entry2);
    changeset->StageAddition(tx1_conflict, tx1_fee+1, 0, 1, 0, false, 4, LockPoints());
    changeset->StageAddition(tx3, tx2_fee, 0, 1, 0, false, 4, LockPoints());
    BOOST_CHECK(ImprovesFeerateDiagram(*changeset) == std::nullopt);

    changeset.reset();
    // With prioritisation of in-mempool conflicts, it affects the results of the comparison using the same args as just above
    pool.PrioritiseTransaction(entry1->GetSharedTx()->GetHash(), /*nFeeDelta=*/1);
    changeset = pool.GetChangeSet();
    changeset->StageRemoval(entry1);
    changeset->StageRemoval(entry2);
    changeset->StageAddition(tx1_conflict, tx1_fee+1, 0, 1, 0, false, 4, LockPoints());
    changeset->StageAddition(tx3, tx2_fee, 0, 1, 0, false, 4, LockPoints());
    const auto res2 = ImprovesFeerateDiagram(*changeset);
    BOOST_CHECK(res2.has_value());
    BOOST_CHECK(res2.value().first == DiagramCheckError::FAILURE);
    BOOST_CHECK(res2.value().second == "insufficient feerate: does not improve feerate diagram");
    changeset.reset();

    pool.PrioritiseTransaction(entry1->GetSharedTx()->GetHash(), /*nFeeDelta=*/-1);

    // With fewer vbytes it does
    CMutableTransaction tx4{entry3.GetTx()};
    tx4.vin[0].scriptWitness = CScriptWitness(); // Clear out the witness, to reduce size
    auto entry4 = entry.FromTx(MakeTransactionRef(tx4));
    changeset = pool.GetChangeSet();
    changeset->StageRemoval(entry1);
    changeset->StageRemoval(entry2);
    changeset->StageAddition(tx1_conflict, tx1_fee, 0, 1, 0, false, 4, LockPoints());
    changeset->StageAddition(entry4.GetSharedTx(), tx2_fee, 0, 1, 0, false, 4, LockPoints());
    BOOST_CHECK(ImprovesFeerateDiagram(*changeset) == std::nullopt);
    changeset.reset();

    // Adding a grandchild makes the cluster size 3, which is uncalculable
    const auto tx5 = make_tx(/*inputs=*/ {tx2}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(normal_fee).FromTx(tx5));
    const auto entry5 = pool.GetIter(tx5->GetHash()).value();

    changeset = pool.GetChangeSet();
    changeset->StageRemoval(entry1);
    changeset->StageRemoval(entry2);
    changeset->StageRemoval(entry5);
    changeset->StageAddition(tx1_conflict, tx1_fee, 0, 1, 0, false, 4, LockPoints());
    changeset->StageAddition(entry4.GetSharedTx(), tx2_fee + entry5->GetModifiedFee() + 1, 0, 1, 0, false, 4, LockPoints());
    const auto res3 = ImprovesFeerateDiagram(*changeset);
    BOOST_CHECK(res3.has_value());
    BOOST_CHECK(res3.value().first == DiagramCheckError::UNCALCULABLE);
    BOOST_CHECK_MESSAGE(res3.value().second == strprintf("%s has 2 descendants, max 1 allowed", tx1->GetHash().GetHex()), res3.value().second);
}

BOOST_FIXTURE_TEST_CASE(calc_feerate_diagram_rbf, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    const CAmount low_fee{CENT/100};
    const CAmount normal_fee{CENT/10};
    const CAmount high_fee{CENT};

    // low -> high -> medium fee transactions that would result in two chunks together since they
    // are all same size
    const auto low_tx = make_tx(/*inputs=*/ {m_coinbase_txns[0]}, /*output_values=*/ {10 * COIN});
    AddToMempool(pool, entry.Fee(low_fee).FromTx(low_tx));

    const auto entry_low = pool.GetIter(low_tx->GetHash()).value();
    const auto low_size = entry_low->GetTxSize();

    const auto replacement_tx = make_tx(/*inputs=*/ {m_coinbase_txns[0]}, /*output_values=*/ {9 * COIN});
    auto entry_replacement = entry.FromTx(replacement_tx);

    // Replacement of size 1
    {
        auto changeset = pool.GetChangeSet();
        changeset->StageRemoval(entry_low);
        changeset->StageAddition(replacement_tx, 0, 0, 1, 0, false, 4, LockPoints());
        const auto replace_one{changeset->CalculateChunksForRBF()};
        BOOST_CHECK(replace_one.has_value());
        std::vector<FeeFrac> expected_old_chunks{{low_fee, low_size}};
        BOOST_CHECK(replace_one->first == expected_old_chunks);
        std::vector<FeeFrac> expected_new_chunks{{0, int32_t(entry_replacement.GetTxSize())}};
        BOOST_CHECK(replace_one->second == expected_new_chunks);
    }

    // Non-zero replacement fee/size
    {
        auto changeset = pool.GetChangeSet();
        changeset->StageRemoval(entry_low);
        changeset->StageAddition(replacement_tx, high_fee, 0, 1, 0, false, 4, LockPoints());
        const auto replace_one_fee{changeset->CalculateChunksForRBF()};
        BOOST_CHECK(replace_one_fee.has_value());
        std::vector<FeeFrac> expected_old_diagram{{low_fee, low_size}};
        BOOST_CHECK(replace_one_fee->first == expected_old_diagram);
        std::vector<FeeFrac> expected_new_diagram{{high_fee, low_size}};
        BOOST_CHECK(replace_one_fee->second == expected_new_diagram);
    }

    // Add a second transaction to the cluster that will make a single chunk, to be evicted in the RBF
    const auto high_tx = make_tx(/*inputs=*/ {low_tx}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(high_fee).FromTx(high_tx));
    const auto entry_high = pool.GetIter(high_tx->GetHash()).value();
    const auto high_size = entry_high->GetTxSize();

    {
        auto changeset = pool.GetChangeSet();
        changeset->StageRemoval(entry_low);
        changeset->StageRemoval(entry_high);
        changeset->StageAddition(replacement_tx, high_fee, 0, 1, 0, false, 4, LockPoints());
        const auto replace_single_chunk{changeset->CalculateChunksForRBF()};
        BOOST_CHECK(replace_single_chunk.has_value());
        std::vector<FeeFrac> expected_old_chunks{{low_fee + high_fee, low_size + high_size}};
        BOOST_CHECK(replace_single_chunk->first == expected_old_chunks);
        std::vector<FeeFrac> expected_new_chunks{{high_fee, low_size}};
        BOOST_CHECK(replace_single_chunk->second == expected_new_chunks);
    }

    // Conflict with the 2nd tx, resulting in new diagram with three entries
    {
        auto changeset = pool.GetChangeSet();
        changeset->StageRemoval(entry_high);
        changeset->StageAddition(replacement_tx, high_fee, 0, 1, 0, false, 4, LockPoints());
        const auto replace_cpfp_child{changeset->CalculateChunksForRBF()};
        BOOST_CHECK(replace_cpfp_child.has_value());
        std::vector<FeeFrac> expected_old_chunks{{low_fee + high_fee, low_size + high_size}};
        BOOST_CHECK(replace_cpfp_child->first == expected_old_chunks);
        std::vector<FeeFrac> expected_new_chunks{{high_fee, low_size}, {low_fee, low_size}};
        BOOST_CHECK(replace_cpfp_child->second == expected_new_chunks);
    }

    // third transaction causes the topology check to fail
    const auto normal_tx = make_tx(/*inputs=*/ {high_tx}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(normal_fee).FromTx(normal_tx));
    const auto entry_normal = pool.GetIter(normal_tx->GetHash()).value();

    {
        auto changeset = pool.GetChangeSet();
        changeset->StageRemoval(entry_low);
        changeset->StageRemoval(entry_high);
        changeset->StageRemoval(entry_normal);
        changeset->StageAddition(replacement_tx, high_fee, 0, 1, 0, false, 4, LockPoints());
        const auto replace_too_large{changeset->CalculateChunksForRBF()};
        BOOST_CHECK(!replace_too_large.has_value());
        BOOST_CHECK_EQUAL(util::ErrorString(replace_too_large).original, strprintf("%s has both ancestor and descendant, exceeding cluster limit of 2", high_tx->GetHash().GetHex()));
    }

    // Make a size 2 cluster that is itself two chunks; evict both txns
    const auto high_tx_2 = make_tx(/*inputs=*/ {m_coinbase_txns[1]}, /*output_values=*/ {10 * COIN});
    AddToMempool(pool, entry.Fee(high_fee).FromTx(high_tx_2));
    const auto entry_high_2 = pool.GetIter(high_tx_2->GetHash()).value();
    const auto high_size_2 = entry_high_2->GetTxSize();

    const auto low_tx_2 = make_tx(/*inputs=*/ {high_tx_2}, /*output_values=*/ {9 * COIN});
    AddToMempool(pool, entry.Fee(low_fee).FromTx(low_tx_2));
    const auto entry_low_2 = pool.GetIter(low_tx_2->GetHash()).value();
    const auto low_size_2 = entry_low_2->GetTxSize();

    {
        auto changeset = pool.GetChangeSet();
        changeset->StageRemoval(entry_high_2);
        changeset->StageRemoval(entry_low_2);
        changeset->StageAddition(replacement_tx, high_fee, 0, 1, 0, false, 4, LockPoints());
        const auto replace_two_chunks_single_cluster{changeset->CalculateChunksForRBF()};
        BOOST_CHECK(replace_two_chunks_single_cluster.has_value());
        std::vector<FeeFrac> expected_old_chunks{{high_fee, high_size_2}, {low_fee, low_size_2}};
        BOOST_CHECK(replace_two_chunks_single_cluster->first == expected_old_chunks);
        std::vector<FeeFrac> expected_new_chunks{{high_fee, low_size_2}};
        BOOST_CHECK(replace_two_chunks_single_cluster->second == expected_new_chunks);
    }

    // You can have more than two direct conflicts if there are multiple affected clusters, all of size 2 or less
    const auto conflict_1 = make_tx(/*inputs=*/ {m_coinbase_txns[2]}, /*output_values=*/ {10 * COIN});
    AddToMempool(pool, entry.Fee(low_fee).FromTx(conflict_1));
    const auto conflict_1_entry = pool.GetIter(conflict_1->GetHash()).value();

    const auto conflict_2 = make_tx(/*inputs=*/ {m_coinbase_txns[3]}, /*output_values=*/ {10 * COIN});
    AddToMempool(pool, entry.Fee(low_fee).FromTx(conflict_2));
    const auto conflict_2_entry = pool.GetIter(conflict_2->GetHash()).value();

    const auto conflict_3 = make_tx(/*inputs=*/ {m_coinbase_txns[4]}, /*output_values=*/ {10 * COIN});
    AddToMempool(pool, entry.Fee(low_fee).FromTx(conflict_3));
    const auto conflict_3_entry = pool.GetIter(conflict_3->GetHash()).value();

    {
        auto changeset = pool.GetChangeSet();
        changeset->StageRemoval(conflict_1_entry);
        changeset->StageRemoval(conflict_2_entry);
        changeset->StageRemoval(conflict_3_entry);
        changeset->StageAddition(replacement_tx, high_fee, 0, 1, 0, false, 4, LockPoints());
        const auto replace_multiple_clusters{changeset->CalculateChunksForRBF()};
        BOOST_CHECK(replace_multiple_clusters.has_value());
        BOOST_CHECK(replace_multiple_clusters->first.size() == 3);
        BOOST_CHECK(replace_multiple_clusters->second.size() == 1);
    }

    // Add a child transaction to conflict_1 and make it cluster size 2, two chunks due to same feerate
    const auto conflict_1_child = make_tx(/*inputs=*/{conflict_1}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(low_fee).FromTx(conflict_1_child));
    const auto conflict_1_child_entry = pool.GetIter(conflict_1_child->GetHash()).value();

    {
        auto changeset = pool.GetChangeSet();
        changeset->StageRemoval(conflict_1_entry);
        changeset->StageRemoval(conflict_2_entry);
        changeset->StageRemoval(conflict_3_entry);
        changeset->StageRemoval(conflict_1_child_entry);
        changeset->StageAddition(replacement_tx, high_fee, 0, 1, 0, false, 4, LockPoints());
        const auto replace_multiple_clusters_2{changeset->CalculateChunksForRBF()};

        BOOST_CHECK(replace_multiple_clusters_2.has_value());
        BOOST_CHECK(replace_multiple_clusters_2->first.size() == 4);
        BOOST_CHECK(replace_multiple_clusters_2->second.size() == 1);
    }

    // Add another descendant to conflict_1, making the cluster size > 2 should fail at this point.
    const auto conflict_1_grand_child = make_tx(/*inputs=*/{conflict_1_child}, /*output_values=*/ {995 * CENT});
    AddToMempool(pool, entry.Fee(high_fee).FromTx(conflict_1_grand_child));
    const auto conflict_1_grand_child_entry = pool.GetIter(conflict_1_child->GetHash()).value();

    {
        auto changeset = pool.GetChangeSet();
        changeset->StageRemoval(conflict_1_entry);
        changeset->StageRemoval(conflict_2_entry);
        changeset->StageRemoval(conflict_3_entry);
        changeset->StageRemoval(conflict_1_child_entry);
        changeset->StageRemoval(conflict_1_grand_child_entry);
        changeset->StageAddition(replacement_tx, high_fee, 0, 1, 0, false, 4, LockPoints());
        const auto replace_cluster_size_3{changeset->CalculateChunksForRBF()};

        BOOST_CHECK(!replace_cluster_size_3.has_value());
        BOOST_CHECK_EQUAL(util::ErrorString(replace_cluster_size_3).original, strprintf("%s has 2 descendants, max 1 allowed", conflict_1->GetHash().GetHex()));
    }
}

BOOST_AUTO_TEST_CASE(feerate_chunks_utilities)
{
    // Sanity check the correctness of the feerate chunks comparison.

    // A strictly better case.
    std::vector<FeeFrac> old_chunks{{{950, 300}, {100, 100}}};
    std::vector<FeeFrac> new_chunks{{{1000, 300}, {50, 100}}};

    BOOST_CHECK(std::is_lt(CompareChunks(old_chunks, new_chunks)));
    BOOST_CHECK(std::is_gt(CompareChunks(new_chunks, old_chunks)));

    // Incomparable diagrams
    old_chunks = {{950, 300}, {100, 100}};
    new_chunks = {{1000, 300}, {0, 100}};

    BOOST_CHECK(CompareChunks(old_chunks, new_chunks) == std::partial_ordering::unordered);
    BOOST_CHECK(CompareChunks(new_chunks, old_chunks) == std::partial_ordering::unordered);

    // Strictly better but smaller size.
    old_chunks = {{950, 300}, {100, 100}};
    new_chunks = {{1100, 300}};

    BOOST_CHECK(std::is_lt(CompareChunks(old_chunks, new_chunks)));
    BOOST_CHECK(std::is_gt(CompareChunks(new_chunks, old_chunks)));

    // New diagram is strictly better due to the first chunk, even though
    // second chunk contributes no fees
    old_chunks = {{950, 300}, {100, 100}};
    new_chunks = {{1100, 100}, {0, 100}};

    BOOST_CHECK(std::is_lt(CompareChunks(old_chunks, new_chunks)));
    BOOST_CHECK(std::is_gt(CompareChunks(new_chunks, old_chunks)));

    // Feerate of first new chunk is better with, but second chunk is worse
    old_chunks = {{950, 300}, {100, 100}};
    new_chunks = {{750, 100}, {249, 250}, {151, 650}};

    BOOST_CHECK(CompareChunks(old_chunks, new_chunks) == std::partial_ordering::unordered);
    BOOST_CHECK(CompareChunks(new_chunks, old_chunks) == std::partial_ordering::unordered);

    // If we make the second chunk slightly better, the new diagram now wins.
    old_chunks = {{950, 300}, {100, 100}};
    new_chunks = {{750, 100}, {250, 250}, {150, 150}};

    BOOST_CHECK(std::is_lt(CompareChunks(old_chunks, new_chunks)));
    BOOST_CHECK(std::is_gt(CompareChunks(new_chunks, old_chunks)));

    // Identical diagrams, cannot be strictly better
    old_chunks = {{950, 300}, {100, 100}};
    new_chunks = {{950, 300}, {100, 100}};

    BOOST_CHECK(std::is_eq(CompareChunks(old_chunks, new_chunks)));
    BOOST_CHECK(std::is_eq(CompareChunks(new_chunks, old_chunks)));

    // Same aggregate fee, but different total size (trigger single tail fee check step)
    old_chunks = {{950, 300}, {100, 99}};
    new_chunks = {{950, 300}, {100, 100}};

    // No change in evaluation when tail check needed.
    BOOST_CHECK(std::is_gt(CompareChunks(old_chunks, new_chunks)));
    BOOST_CHECK(std::is_lt(CompareChunks(new_chunks, old_chunks)));

    // Trigger multiple tail fee check steps
    old_chunks = {{950, 300}, {100, 99}};
    new_chunks = {{950, 300}, {100, 100}, {0, 1}, {0, 1}};

    BOOST_CHECK(std::is_gt(CompareChunks(old_chunks, new_chunks)));
    BOOST_CHECK(std::is_lt(CompareChunks(new_chunks, old_chunks)));

    // Multiple tail fee check steps, unordered result
    new_chunks = {{950, 300}, {100, 100}, {0, 1}, {0, 1}, {1, 1}};
    BOOST_CHECK(CompareChunks(old_chunks, new_chunks) == std::partial_ordering::unordered);
    BOOST_CHECK(CompareChunks(new_chunks, old_chunks) == std::partial_ordering::unordered);
}

BOOST_AUTO_TEST_SUITE_END()
