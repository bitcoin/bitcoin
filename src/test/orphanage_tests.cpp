// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <txorphanage.h>

#include <array>
#include <cstdint>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(orphanage_tests, TestingSetup)

class TxOrphanageTest : public TxOrphanage
{
public:
    inline size_t CountOrphans() const
    {
        return m_orphans.size();
    }

    CTransactionRef RandomOrphan()
    {
        std::map<Wtxid, OrphanTx>::iterator it;
        it = m_orphans.lower_bound(Wtxid::FromUint256(InsecureRand256()));
        if (it == m_orphans.end())
            it = m_orphans.begin();
        return it->second.tx;
    }
};

static void MakeNewKeyWithFastRandomContext(CKey& key, FastRandomContext& rand_ctx = g_insecure_rand_ctx)
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
static bool EqualTxns(const std::set<CTransactionRef>& set_txns,
                      const std::vector<std::pair<CTransactionRef, NodeId>>& vec_txns)
{
    if (vec_txns.size() != set_txns.size()) return false;
    for (const auto& [tx, nodeid] : vec_txns) {
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
    g_insecure_rand_ctx = FastRandomContext{uint256{33}};

    TxOrphanageTest orphanage;
    CKey key;
    MakeNewKeyWithFastRandomContext(key);
    FillableSigningProvider keystore;
    BOOST_CHECK(keystore.AddKey(key));

    // 50 orphan transactions:
    for (int i = 0; i < 50; i++)
    {
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = Txid::FromUint256(InsecureRand256());
        tx.vin[0].scriptSig << OP_1;
        tx.vout.resize(1);
        tx.vout[0].nValue = 1*CENT;
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
        tx.vout[0].nValue = 1*CENT;
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

    // Test EraseOrphansFor:
    for (NodeId i = 0; i < 3; i++)
    {
        size_t sizeBefore = orphanage.CountOrphans();
        orphanage.EraseForPeer(i);
        BOOST_CHECK(orphanage.CountOrphans() < sizeBefore);
    }

    // Test LimitOrphanTxSize() function:
    FastRandomContext rng{/*fDeterministic=*/true};
    orphanage.LimitOrphans(40, rng);
    BOOST_CHECK(orphanage.CountOrphans() <= 40);
    orphanage.LimitOrphans(10, rng);
    BOOST_CHECK(orphanage.CountOrphans() <= 10);
    orphanage.LimitOrphans(0, rng);
    BOOST_CHECK(orphanage.CountOrphans() == 0);
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
    BOOST_CHECK(!orphanage.HaveTx(mutated_wtxid));

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

        BOOST_CHECK(EqualTxns(expected_parent1_children, orphanage.GetChildrenFromDifferentPeer(parent1, node2)));
        BOOST_CHECK(EqualTxns(expected_parent2_children, orphanage.GetChildrenFromDifferentPeer(parent2, node2)));

        // The peer must match
        BOOST_CHECK(orphanage.GetChildrenFromSamePeer(parent1, node2).empty());
        BOOST_CHECK(orphanage.GetChildrenFromSamePeer(parent2, node2).empty());

        // There shouldn't be any children of this tx in the orphanage
        BOOST_CHECK(orphanage.GetChildrenFromSamePeer(child_p1n0_p2n0, node1).empty());
        BOOST_CHECK(orphanage.GetChildrenFromSamePeer(child_p1n0_p2n0, node2).empty());
        BOOST_CHECK(orphanage.GetChildrenFromDifferentPeer(child_p1n0_p2n0, node1).empty());
        BOOST_CHECK(orphanage.GetChildrenFromDifferentPeer(child_p1n0_p2n0, node2).empty());
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
            BOOST_CHECK(EqualTxns(expected_parent1_node1, orphanage.GetChildrenFromDifferentPeer(parent1, node2)));
        }

        // Children of parent2 from node1:
        {
            std::set<CTransactionRef> expected_parent2_node1{child_p2n1};

            BOOST_CHECK(EqualTxns(expected_parent2_node1, orphanage.GetChildrenFromSamePeer(parent2, node1)));
            BOOST_CHECK(EqualTxns(expected_parent2_node1, orphanage.GetChildrenFromDifferentPeer(parent2, node2)));
        }

        // Children of parent1 from node2:
        {
            std::set<CTransactionRef> expected_parent1_node2{child_p1n0_p1n1, child_p1n0_p2n0};

            BOOST_CHECK(EqualTxns(expected_parent1_node2, orphanage.GetChildrenFromSamePeer(parent1, node2)));
            BOOST_CHECK(EqualTxns(expected_parent1_node2, orphanage.GetChildrenFromDifferentPeer(parent1, node1)));
        }

        // Children of parent2 from node2:
        {
            std::set<CTransactionRef> expected_parent2_node2{child_p1n0_p2n0};

            BOOST_CHECK(EqualTxns(expected_parent2_node2, orphanage.GetChildrenFromSamePeer(parent2, node2)));
            BOOST_CHECK(EqualTxns(expected_parent2_node2, orphanage.GetChildrenFromDifferentPeer(parent2, node1)));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
