// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <consensus/validation.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <test/util/transaction_utils.h>
#include <txorphanage.h>

#include <array>
#include <cstdint>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(orphanage_tests, TestingSetup)

class TxOrphanageTest : public TxOrphanage
{
public:
    TxOrphanageTest(FastRandomContext& rng) : m_rng{rng} {}

    inline size_t CountOrphans() const
    {
        return m_orphans.size();
    }

    CTransactionRef RandomOrphan()
    {
        std::map<Wtxid, OrphanTx>::iterator it;
        it = m_orphans.lower_bound(Wtxid::FromUint256(m_rng.rand256()));
        if (it == m_orphans.end())
            it = m_orphans.begin();
        return it->second.tx;
    }

    FastRandomContext& m_rng;
};

static void MakeNewKeyWithFastRandomContext(CKey& key, FastRandomContext& rand_ctx)
{
    std::vector<unsigned char> keydata;
    keydata = rand_ctx.randbytes(32);
    key.Set(keydata.data(), keydata.data() + keydata.size(), /*fCompressedIn=*/true);
    assert(key.IsValid());
}

// Creates a transaction with 2 outputs. Spends all outpoints. If outpoints is empty, spends a random one.
static CTransactionRef MakeTransactionSpending(const std::vector<COutPoint>& outpoints, FastRandomContext& det_rand)
{
    CKey key;
    MakeNewKeyWithFastRandomContext(key, det_rand);
    CMutableTransaction tx;
    // If no outpoints are given, create a random one.
    if (outpoints.empty()) {
        tx.vin.emplace_back(Txid::FromUint256(det_rand.rand256()), 0);
    } else {
        for (const auto& outpoint : outpoints) {
            tx.vin.emplace_back(outpoint);
        }
    }
    // Ensure txid != wtxid
    tx.vin[0].scriptWitness.stack.push_back({1});
    tx.vout.resize(2);
    tx.vout[0].nValue = CENT;
    tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
    tx.vout[1].nValue = 3 * CENT;
    tx.vout[1].scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(key.GetPubKey()));
    return MakeTransactionRef(tx);
}

// Make another (not necessarily valid) tx with the same txid but different wtxid.
static CTransactionRef MakeMutation(const CTransactionRef& ptx)
{
    CMutableTransaction tx(*ptx);
    tx.vin[0].scriptWitness.stack.push_back({5});
    auto mutated_tx = MakeTransactionRef(tx);
    assert(ptx->GetHash() == mutated_tx->GetHash());
    return mutated_tx;
}

static bool EqualTxns(const std::set<CTransactionRef>& set_txns, const std::vector<CTransactionRef>& vec_txns)
{
    if (vec_txns.size() != set_txns.size()) return false;
    for (const auto& tx : vec_txns) {
        if (!set_txns.contains(tx)) return false;
    }
    return true;
}

BOOST_AUTO_TEST_CASE(DoS_mapOrphans)
{
    // This test had non-deterministic coverage due to
    // randomly selected seeds.
    // This seed is chosen so that all branches of the function
    // ecdsa_signature_parse_der_lax are executed during this test.
    // Specifically branches that run only when an ECDSA
    // signature's R and S values have leading zeros.
    m_rng.Reseed(uint256{33});

    TxOrphanageTest orphanage{m_rng};
    CKey key;
    MakeNewKeyWithFastRandomContext(key, m_rng);
    FillableSigningProvider keystore;
    BOOST_CHECK(keystore.AddKey(key));

    // Freeze time for length of test
    auto now{GetTime<std::chrono::seconds>()};
    SetMockTime(now);

    // 50 orphan transactions:
    for (int i = 0; i < 50; i++)
    {
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = Txid::FromUint256(m_rng.rand256());
        tx.vin[0].scriptSig << OP_1;
        tx.vout.resize(1);
        tx.vout[0].nValue = i*CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

        orphanage.AddTx(MakeTransactionRef(tx), i);
    }

    // ... and 50 that depend on other orphans:
    for (int i = 0; i < 50; i++)
    {
        CTransactionRef txPrev = orphanage.RandomOrphan();

        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = txPrev->GetHash();
        tx.vout.resize(1);
        tx.vout[0].nValue = i*CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
        SignatureData empty;
        BOOST_CHECK(SignSignature(keystore, *txPrev, tx, 0, SIGHASH_ALL, empty));

        orphanage.AddTx(MakeTransactionRef(tx), i);
    }

    // This really-big orphan should be ignored:
    for (int i = 0; i < 10; i++)
    {
        CTransactionRef txPrev = orphanage.RandomOrphan();

        CMutableTransaction tx;
        tx.vout.resize(1);
        tx.vout[0].nValue = 1*CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
        tx.vin.resize(2777);
        for (unsigned int j = 0; j < tx.vin.size(); j++)
        {
            tx.vin[j].prevout.n = j;
            tx.vin[j].prevout.hash = txPrev->GetHash();
        }
        SignatureData empty;
        BOOST_CHECK(SignSignature(keystore, *txPrev, tx, 0, SIGHASH_ALL, empty));
        // Reuse same signature for other inputs
        // (they don't have to be valid for this test)
        for (unsigned int j = 1; j < tx.vin.size(); j++)
            tx.vin[j].scriptSig = tx.vin[0].scriptSig;

        BOOST_CHECK(!orphanage.AddTx(MakeTransactionRef(tx), i));
    }

    size_t expected_num_orphans = orphanage.CountOrphans();

    // Non-existent peer; nothing should be deleted
    orphanage.EraseForPeer(/*peer=*/-1);
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), expected_num_orphans);

    // Each of first three peers stored
    // two transactions each.
    for (NodeId i = 0; i < 3; i++)
    {
        orphanage.EraseForPeer(i);
        expected_num_orphans -= 2;
        BOOST_CHECK(orphanage.CountOrphans() == expected_num_orphans);
    }

    // Test LimitOrphanTxSize() function, nothing should timeout:
    FastRandomContext rng{/*fDeterministic=*/true};
    orphanage.LimitOrphans(/*max_orphans=*/expected_num_orphans, rng);
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), expected_num_orphans);
    expected_num_orphans -= 1;
    orphanage.LimitOrphans(/*max_orphans=*/expected_num_orphans, rng);
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), expected_num_orphans);
    assert(expected_num_orphans > 40);
    orphanage.LimitOrphans(40, rng);
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), 40);
    orphanage.LimitOrphans(10, rng);
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), 10);
    orphanage.LimitOrphans(0, rng);
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), 0);

    // Add one more orphan, check timeout logic
    auto timeout_tx = MakeTransactionSpending(/*outpoints=*/{}, rng);
    orphanage.AddTx(timeout_tx, 0);
    orphanage.LimitOrphans(1, rng);
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), 1);

    // One second shy of expiration
    SetMockTime(now + ORPHAN_TX_EXPIRE_TIME - 1s);
    orphanage.LimitOrphans(1, rng);
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), 1);

    // Jump one more second, orphan should be timed out on limiting
    SetMockTime(now + ORPHAN_TX_EXPIRE_TIME);
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), 1);
    orphanage.LimitOrphans(1, rng);
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), 0);
}

BOOST_AUTO_TEST_CASE(same_txid_diff_witness)
{
    FastRandomContext det_rand{true};
    TxOrphanage orphanage;
    NodeId peer{0};

    std::vector<COutPoint> empty_outpoints;
    auto parent = MakeTransactionSpending(empty_outpoints, det_rand);

    // Create children to go into orphanage.
    auto child_normal = MakeTransactionSpending({{parent->GetHash(), 0}}, det_rand);
    auto child_mutated = MakeMutation(child_normal);

    const auto& normal_wtxid = child_normal->GetWitnessHash();
    const auto& mutated_wtxid = child_mutated->GetWitnessHash();
    BOOST_CHECK(normal_wtxid != mutated_wtxid);

    BOOST_CHECK(orphanage.AddTx(child_normal, peer));
    // EraseTx fails as transaction by this wtxid doesn't exist.
    BOOST_CHECK_EQUAL(orphanage.EraseTx(mutated_wtxid), 0);
    BOOST_CHECK(orphanage.HaveTx(normal_wtxid));
    BOOST_CHECK(orphanage.GetTx(normal_wtxid) == child_normal);
    BOOST_CHECK(!orphanage.HaveTx(mutated_wtxid));
    BOOST_CHECK(orphanage.GetTx(mutated_wtxid) == nullptr);

    // Must succeed. Both transactions should be present in orphanage.
    BOOST_CHECK(orphanage.AddTx(child_mutated, peer));
    BOOST_CHECK(orphanage.HaveTx(normal_wtxid));
    BOOST_CHECK(orphanage.HaveTx(mutated_wtxid));

    // Outpoints map should track all entries: check that both are returned as children of the parent.
    std::set<CTransactionRef> expected_children{child_normal, child_mutated};
    BOOST_CHECK(EqualTxns(expected_children, orphanage.GetChildrenFromSamePeer(parent, peer)));

    // Erase by wtxid: mutated first
    BOOST_CHECK_EQUAL(orphanage.EraseTx(mutated_wtxid), 1);
    BOOST_CHECK(orphanage.HaveTx(normal_wtxid));
    BOOST_CHECK(!orphanage.HaveTx(mutated_wtxid));

    BOOST_CHECK_EQUAL(orphanage.EraseTx(normal_wtxid), 1);
    BOOST_CHECK(!orphanage.HaveTx(normal_wtxid));
    BOOST_CHECK(!orphanage.HaveTx(mutated_wtxid));
}


BOOST_AUTO_TEST_CASE(get_children)
{
    FastRandomContext det_rand{true};
    std::vector<COutPoint> empty_outpoints;

    auto parent1 = MakeTransactionSpending(empty_outpoints, det_rand);
    auto parent2 = MakeTransactionSpending(empty_outpoints, det_rand);

    // Make sure these parents have different txids otherwise this test won't make sense.
    while (parent1->GetHash() == parent2->GetHash()) {
        parent2 = MakeTransactionSpending(empty_outpoints, det_rand);
    }

    // Create children to go into orphanage.
    auto child_p1n0 = MakeTransactionSpending({{parent1->GetHash(), 0}}, det_rand);
    auto child_p2n1 = MakeTransactionSpending({{parent2->GetHash(), 1}}, det_rand);
    // Spends the same tx twice. Should not cause duplicates.
    auto child_p1n0_p1n1 = MakeTransactionSpending({{parent1->GetHash(), 0}, {parent1->GetHash(), 1}}, det_rand);
    // Spends the same outpoint as previous tx. Should still be returned; don't assume outpoints are unique.
    auto child_p1n0_p2n0 = MakeTransactionSpending({{parent1->GetHash(), 0}, {parent2->GetHash(), 0}}, det_rand);

    const NodeId node1{1};
    const NodeId node2{2};

    // All orphans provided by node1
    {
        TxOrphanage orphanage;
        BOOST_CHECK(orphanage.AddTx(child_p1n0, node1));
        BOOST_CHECK(orphanage.AddTx(child_p2n1, node1));
        BOOST_CHECK(orphanage.AddTx(child_p1n0_p1n1, node1));
        BOOST_CHECK(orphanage.AddTx(child_p1n0_p2n0, node1));

        std::set<CTransactionRef> expected_parent1_children{child_p1n0, child_p1n0_p2n0, child_p1n0_p1n1};
        std::set<CTransactionRef> expected_parent2_children{child_p2n1, child_p1n0_p2n0};

        BOOST_CHECK(EqualTxns(expected_parent1_children, orphanage.GetChildrenFromSamePeer(parent1, node1)));
        BOOST_CHECK(EqualTxns(expected_parent2_children, orphanage.GetChildrenFromSamePeer(parent2, node1)));

        // The peer must match
        BOOST_CHECK(orphanage.GetChildrenFromSamePeer(parent1, node2).empty());
        BOOST_CHECK(orphanage.GetChildrenFromSamePeer(parent2, node2).empty());

        // There shouldn't be any children of this tx in the orphanage
        BOOST_CHECK(orphanage.GetChildrenFromSamePeer(child_p1n0_p2n0, node1).empty());
        BOOST_CHECK(orphanage.GetChildrenFromSamePeer(child_p1n0_p2n0, node2).empty());
    }

    // Orphans provided by node1 and node2
    {
        TxOrphanage orphanage;
        BOOST_CHECK(orphanage.AddTx(child_p1n0, node1));
        BOOST_CHECK(orphanage.AddTx(child_p2n1, node1));
        BOOST_CHECK(orphanage.AddTx(child_p1n0_p1n1, node2));
        BOOST_CHECK(orphanage.AddTx(child_p1n0_p2n0, node2));

        // +----------------+---------------+----------------------------------+
        // |                | sender=node1  |           sender=node2           |
        // +----------------+---------------+----------------------------------+
        // | spends parent1 | child_p1n0    | child_p1n0_p1n1, child_p1n0_p2n0 |
        // | spends parent2 | child_p2n1    | child_p1n0_p2n0                  |
        // +----------------+---------------+----------------------------------+

        // Children of parent1 from node1:
        {
            std::set<CTransactionRef> expected_parent1_node1{child_p1n0};

            BOOST_CHECK(EqualTxns(expected_parent1_node1, orphanage.GetChildrenFromSamePeer(parent1, node1)));
        }

        // Children of parent2 from node1:
        {
            std::set<CTransactionRef> expected_parent2_node1{child_p2n1};

            BOOST_CHECK(EqualTxns(expected_parent2_node1, orphanage.GetChildrenFromSamePeer(parent2, node1)));
        }

        // Children of parent1 from node2:
        {
            std::set<CTransactionRef> expected_parent1_node2{child_p1n0_p1n1, child_p1n0_p2n0};

            BOOST_CHECK(EqualTxns(expected_parent1_node2, orphanage.GetChildrenFromSamePeer(parent1, node2)));
        }

        // Children of parent2 from node2:
        {
            std::set<CTransactionRef> expected_parent2_node2{child_p1n0_p2n0};

            BOOST_CHECK(EqualTxns(expected_parent2_node2, orphanage.GetChildrenFromSamePeer(parent2, node2)));
        }
    }
}

BOOST_AUTO_TEST_CASE(too_large_orphan_tx)
{
    TxOrphanage orphanage;
    CMutableTransaction tx;
    tx.vin.resize(1);

    // check that txs larger than MAX_STANDARD_TX_WEIGHT are not added to the orphanage
    BulkTransaction(tx, MAX_STANDARD_TX_WEIGHT + 4);
    BOOST_CHECK_EQUAL(GetTransactionWeight(CTransaction(tx)), MAX_STANDARD_TX_WEIGHT + 4);
    BOOST_CHECK(!orphanage.AddTx(MakeTransactionRef(tx), 0));

    tx.vout.clear();
    BulkTransaction(tx, MAX_STANDARD_TX_WEIGHT);
    BOOST_CHECK_EQUAL(GetTransactionWeight(CTransaction(tx)), MAX_STANDARD_TX_WEIGHT);
    BOOST_CHECK(orphanage.AddTx(MakeTransactionRef(tx), 0));
}

BOOST_AUTO_TEST_CASE(process_block)
{
    FastRandomContext det_rand{true};
    TxOrphanageTest orphanage{det_rand};

    // Create outpoints that will be spent by transactions in the block
    std::vector<COutPoint> outpoints;
    const uint32_t num_outpoints{6};
    outpoints.reserve(num_outpoints);
    for (uint32_t i{0}; i < num_outpoints; ++i) {
        // All the hashes should be different, but change the n just in case.
        outpoints.emplace_back(Txid::FromUint256(det_rand.rand256()), i);
    }

    CBlock block;
    const NodeId node{0};

    auto control_tx = MakeTransactionSpending({}, det_rand);
    BOOST_CHECK(orphanage.AddTx(control_tx, node));

    auto bo_tx_same_txid = MakeTransactionSpending({outpoints.at(0)}, det_rand);
    BOOST_CHECK(orphanage.AddTx(bo_tx_same_txid, node));
    block.vtx.emplace_back(bo_tx_same_txid);

    // 2 transactions with the same txid but different witness
    auto b_tx_same_txid_diff_witness = MakeTransactionSpending({outpoints.at(1)}, det_rand);
    block.vtx.emplace_back(b_tx_same_txid_diff_witness);

    auto o_tx_same_txid_diff_witness = MakeMutation(b_tx_same_txid_diff_witness);
    BOOST_CHECK(orphanage.AddTx(o_tx_same_txid_diff_witness, node));

    // 2 different transactions that spend the same input.
    auto b_tx_conflict = MakeTransactionSpending({outpoints.at(2)}, det_rand);
    block.vtx.emplace_back(b_tx_conflict);

    auto o_tx_conflict = MakeTransactionSpending({outpoints.at(2)}, det_rand);
    BOOST_CHECK(orphanage.AddTx(o_tx_conflict, node));

    // 2 different transactions that have 1 overlapping input.
    auto b_tx_conflict_partial = MakeTransactionSpending({outpoints.at(3), outpoints.at(4)}, det_rand);
    block.vtx.emplace_back(b_tx_conflict_partial);

    auto o_tx_conflict_partial_2 = MakeTransactionSpending({outpoints.at(4), outpoints.at(5)}, det_rand);
    BOOST_CHECK(orphanage.AddTx(o_tx_conflict_partial_2, node));

    orphanage.EraseForBlock(block);
    for (const auto& expected_removed : {bo_tx_same_txid, o_tx_same_txid_diff_witness, o_tx_conflict, o_tx_conflict_partial_2}) {
        const auto& expected_removed_wtxid = expected_removed->GetWitnessHash();
        BOOST_CHECK(!orphanage.HaveTx(expected_removed_wtxid));
    }
    // Only remaining tx is control_tx
    BOOST_CHECK_EQUAL(orphanage.Size(), 1);
    BOOST_CHECK(orphanage.HaveTx(control_tx->GetWitnessHash()));
}

BOOST_AUTO_TEST_CASE(multiple_announcers)
{
    const NodeId node0{0};
    const NodeId node1{1};
    const NodeId node2{2};
    size_t expected_total_count{0};
    FastRandomContext det_rand{true};
    TxOrphanageTest orphanage{det_rand};

    // Check accounting per peer.
    // Check that EraseForPeer works with multiple announcers.
    {
        auto ptx = MakeTransactionSpending({}, det_rand);
        const auto& wtxid = ptx->GetWitnessHash();
        BOOST_CHECK(orphanage.AddTx(ptx, node0));
        BOOST_CHECK(orphanage.HaveTx(wtxid));
        expected_total_count += 1;
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);

        // Adding again should do nothing.
        BOOST_CHECK(!orphanage.AddTx(ptx, node0));
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);

        // We can add another tx with the same txid but different witness.
        auto ptx_mutated{MakeMutation(ptx)};
        BOOST_CHECK(orphanage.AddTx(ptx_mutated, node0));
        BOOST_CHECK(orphanage.HaveTx(ptx_mutated->GetWitnessHash()));
        expected_total_count += 1;

        BOOST_CHECK(!orphanage.AddTx(ptx, node0));

        // Adding a new announcer should not change overall accounting.
        BOOST_CHECK(orphanage.AddAnnouncer(ptx->GetWitnessHash(), node2));
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);

        // If we already have this announcer, AddAnnouncer returns false.
        BOOST_CHECK(orphanage.HaveTxFromPeer(ptx->GetWitnessHash(), node2));
        BOOST_CHECK(!orphanage.AddAnnouncer(ptx->GetWitnessHash(), node2));

        // Same with using AddTx for an existing tx, which is equivalent to using AddAnnouncer
        BOOST_CHECK(!orphanage.AddTx(ptx, node1));
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);

        // if EraseForPeer is called for an orphan with multiple announcers, the orphanage should only
        // erase that peer from the announcers set.
        orphanage.EraseForPeer(node0);
        BOOST_CHECK(orphanage.HaveTx(ptx->GetWitnessHash()));
        BOOST_CHECK(!orphanage.HaveTxFromPeer(ptx->GetWitnessHash(), node0));
        // node0 is the only one that announced ptx_mutated
        BOOST_CHECK(!orphanage.HaveTx(ptx_mutated->GetWitnessHash()));
        expected_total_count -= 1;
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);

        // EraseForPeer should delete the orphan if it's the only announcer left.
        orphanage.EraseForPeer(node1);
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK(orphanage.HaveTx(ptx->GetWitnessHash()));
        orphanage.EraseForPeer(node2);
        expected_total_count -= 1;
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK(!orphanage.HaveTx(ptx->GetWitnessHash()));
    }

    // Check that erasure for blocks removes for all peers.
    {
        CBlock block;
        auto tx_block = MakeTransactionSpending({}, det_rand);
        block.vtx.emplace_back(tx_block);
        BOOST_CHECK(orphanage.AddTx(tx_block, node0));
        BOOST_CHECK(!orphanage.AddTx(tx_block, node1));

        expected_total_count += 1;

        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);

        orphanage.EraseForBlock(block);

        expected_total_count -= 1;

        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
    }
}
BOOST_AUTO_TEST_CASE(peer_worksets)
{
    const NodeId node0{0};
    const NodeId node1{1};
    const NodeId node2{2};
    FastRandomContext det_rand{true};
    TxOrphanageTest orphanage{det_rand};
    // AddChildrenToWorkSet should pick an announcer randomly
    {
        auto tx_missing_parent = MakeTransactionSpending({}, det_rand);
        auto tx_orphan = MakeTransactionSpending({COutPoint{tx_missing_parent->GetHash(), 0}}, det_rand);
        const auto& orphan_wtxid = tx_orphan->GetWitnessHash();

        // All 3 peers are announcers.
        BOOST_CHECK(orphanage.AddTx(tx_orphan, node0));
        BOOST_CHECK(!orphanage.AddTx(tx_orphan, node1));
        BOOST_CHECK(orphanage.AddAnnouncer(orphan_wtxid, node2));
        for (NodeId node = node0; node <= node2; ++node) {
            BOOST_CHECK(orphanage.HaveTxFromPeer(orphan_wtxid, node));
        }

        // Parent accepted: child is added to 1 of 3 worksets.
        orphanage.AddChildrenToWorkSet(*tx_missing_parent, det_rand);
        int node0_reconsider = orphanage.HaveTxToReconsider(node0);
        int node1_reconsider = orphanage.HaveTxToReconsider(node1);
        int node2_reconsider = orphanage.HaveTxToReconsider(node2);
        BOOST_CHECK_EQUAL(node0_reconsider + node1_reconsider + node2_reconsider, 1);

        NodeId assigned_peer;
        if (node0_reconsider) {
            assigned_peer = node0;
        } else if (node1_reconsider) {
            assigned_peer = node1;
        } else {
            BOOST_CHECK(node2_reconsider);
            assigned_peer = node2;
        }

        // EraseForPeer also removes that tx from the workset.
        orphanage.EraseForPeer(assigned_peer);
        BOOST_CHECK_EQUAL(orphanage.GetTxToReconsider(node0), nullptr);

        // Delete this tx, clearing the orphanage.
        BOOST_CHECK_EQUAL(orphanage.EraseTx(orphan_wtxid), 1);
        BOOST_CHECK_EQUAL(orphanage.Size(), 0);
        for (NodeId node = node0; node <= node2; ++node) {
            BOOST_CHECK_EQUAL(orphanage.GetTxToReconsider(node), nullptr);
            BOOST_CHECK(!orphanage.HaveTxFromPeer(orphan_wtxid, node));
        }
    }
}
BOOST_AUTO_TEST_SUITE_END()
