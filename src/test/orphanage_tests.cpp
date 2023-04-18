// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <consensus/validation.h>
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
    inline size_t CountOrphans() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        return m_orphans.size();
    }

    CTransactionRef RandomOrphan() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        std::map<uint256, OrphanTx>::iterator it;
        it = m_orphans.lower_bound(InsecureRand256());
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

static CTransactionRef MakeLargeOrphan()
{
    CKey key;
    MakeNewKeyWithFastRandomContext(key);
    CMutableTransaction tx;
    tx.vout.resize(1);
    tx.vout[0].nValue = CENT;
    tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
    tx.vin.resize(80);
    for (unsigned int j = 0; j < tx.vin.size(); j++) {
        tx.vin[j].prevout.n = j;
        tx.vin[j].prevout.hash = GetRandHash();
        tx.vin[j].scriptWitness.stack.reserve(100);
        for (int i = 0; i < 100; ++i) {
            tx.vin[j].scriptWitness.stack.push_back(std::vector<unsigned char>(j));
        }
    }
    return MakeTransactionRef(tx);
}

static CTransactionRef MakeTransactionSpending(const std::vector<COutPoint>& outpoints, FastRandomContext& det_rand)
{
    CKey key;
    MakeNewKeyWithFastRandomContext(key, det_rand);
    CMutableTransaction tx;
    // If no outpoints are given, create a random one.
    if (outpoints.empty()) {
        tx.vin.emplace_back(CTxIn{COutPoint{det_rand.rand256(), 0}});
    } else {
        for (const auto& outpoint : outpoints) {
            tx.vin.emplace_back(CTxIn(outpoint));
        }
    }
    // Ensure txid != wtxid
    tx.vin[0].scriptWitness.stack.push_back({1});
    tx.vout.resize(1);
    tx.vout[0].nValue = CENT;
    tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
    return MakeTransactionRef(tx);
}

// Make another (not necessarily valid) tx with the same txid but different wtxid.
static CTransactionRef MakeMutation(const CTransactionRef& ptx)
{
    CMutableTransaction tx(*ptx);
    tx.vin[0].scriptWitness.stack.push_back({1});
    auto mutated_tx = MakeTransactionRef(tx);
    assert(ptx->GetHash() == mutated_tx->GetHash());
    assert(ptx->GetWitnessHash() != mutated_tx->GetWitnessHash());
    return mutated_tx;
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

    size_t expected_count{0};
    size_t expected_total_size{0};

    // 50 orphan transactions:
    for (int i = 0; i < 50; i++)
    {
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = InsecureRand256();
        tx.vin[0].scriptSig << OP_1;
        tx.vout.resize(1);
        tx.vout[0].nValue = 1*CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

        auto ptx{MakeTransactionRef(tx)};
        if (orphanage.AddTx(ptx, i, {})) {
            ++expected_count;
            expected_total_size += ptx->GetTotalSize();
        }
    }
    BOOST_CHECK_EQUAL(orphanage.Size(), expected_count);
    BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);

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

        auto ptx{MakeTransactionRef(tx)};
        if (orphanage.AddTx(ptx, i, {})) {
            ++expected_count;
            expected_total_size += ptx->GetTotalSize();
        }
    }
    BOOST_CHECK_EQUAL(orphanage.Size(), expected_count);
    BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);

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
        // Re-use same signature for other inputs
        // (they don't have to be valid for this test)
        for (unsigned int j = 1; j < tx.vin.size(); j++)
            tx.vin[j].scriptSig = tx.vin[0].scriptSig;

        BOOST_CHECK(!orphanage.AddTx(MakeTransactionRef(tx), i, {}));
    }
    BOOST_CHECK_EQUAL(orphanage.Size(), expected_count);
    BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);

    // Test EraseOrphansFor:
    for (NodeId i = 0; i < 3; i++)
    {
        size_t sizeBefore = orphanage.CountOrphans();
        orphanage.EraseForPeer(i);
        BOOST_CHECK(orphanage.CountOrphans() < sizeBefore);
    }

    orphanage.LimitOrphans(40);
    BOOST_CHECK(orphanage.CountOrphans() <= 40);
    orphanage.LimitOrphans(10);
    BOOST_CHECK(orphanage.CountOrphans() <= 10);
    orphanage.LimitOrphans(0);
    BOOST_CHECK(orphanage.CountOrphans() == 0);

    // Limits on total size of orphans
    const size_t modified_max_count{150};
    unsigned int size_just_under{0};
    const unsigned int size_per_tx{MakeLargeOrphan()->GetTotalSize()};
    // The transaction size needs to be small enough to not be rejected, but big enough so
    // that we hit the size limit instead of the count limit.
    BOOST_CHECK(size_per_tx <= MAX_STANDARD_TX_WEIGHT);
    BOOST_CHECK(size_per_tx * modified_max_count > DEFAULT_MAX_ORPHAN_TOTAL_SIZE);

    expected_count = 0;
    expected_total_size = 0;
    BOOST_CHECK_EQUAL(orphanage.CountOrphans(), expected_count);
    BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);

    while (expected_total_size <= DEFAULT_MAX_ORPHAN_TOTAL_SIZE) {
        // Create really large orphan tx.
        auto huge_orphan{MakeLargeOrphan()};
        BOOST_CHECK_EQUAL(huge_orphan->GetTotalSize(), size_per_tx);

        // Check if this is going to be the orphan that pushes us over DEFAULT_MAX_ORPHAN_TOTAL_SIZE.
        if (expected_total_size + huge_orphan->GetTotalSize()) size_just_under = expected_total_size;

        // Add tx to orphanage
        BOOST_CHECK(orphanage.AddTx(huge_orphan, /*peer=*/0, {}));
        expected_total_size += huge_orphan->GetTotalSize();
        expected_count += 1;
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_count);
    }

    // LimitOrphans should trim until within the maximum size
    orphanage.LimitOrphans(modified_max_count);
    // Both weight and count limits are enforced
    BOOST_CHECK(orphanage.Size() <= modified_max_count);
    BOOST_CHECK(orphanage.TotalOrphanBytes() <= DEFAULT_MAX_ORPHAN_TOTAL_SIZE);
    BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), size_just_under);

    // Now trim to a very small maximum size...
    orphanage.LimitOrphans(/*max_orphans=*/10, DEFAULT_MAX_ORPHAN_TOTAL_SIZE);
    BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), 10 * size_per_tx);
}

BOOST_AUTO_TEST_CASE(process_block)
{
    FastRandomContext det_rand{true};
    TxOrphanageTest orphanage;

    // Create outpoints that will be spent by transactions in the block
    std::vector<COutPoint> outpoints;
    const uint32_t num_outpoints{6};
    outpoints.reserve(num_outpoints);
    for (uint32_t i{0}; i < num_outpoints; ++i) {
        // All the hashes should be different, but change the n just in case.
        outpoints.emplace_back(COutPoint{det_rand.rand256(), i});
    }

    CBlock block;
    const NodeId node{0};

    auto bo_tx_same_txid = MakeTransactionSpending({outpoints.at(0)}, det_rand);
    BOOST_CHECK(orphanage.AddTx(bo_tx_same_txid, node, {}));
    block.vtx.emplace_back(bo_tx_same_txid);

    // 2 transactions with the same txid but different witness
    auto b_tx_same_txid_diff_witness = MakeTransactionSpending({outpoints.at(1)}, det_rand);
    block.vtx.emplace_back(b_tx_same_txid_diff_witness);

    auto o_tx_same_txid_diff_witness = MakeMutation(b_tx_same_txid_diff_witness);
    BOOST_CHECK(orphanage.AddTx(o_tx_same_txid_diff_witness, node, {}));

    // 2 different transactions that spend the same input.
    auto b_tx_conflict = MakeTransactionSpending({outpoints.at(2)}, det_rand);
    block.vtx.emplace_back(b_tx_conflict);

    auto o_tx_conflict = MakeTransactionSpending({outpoints.at(2)}, det_rand);
    BOOST_CHECK(orphanage.AddTx(o_tx_conflict, node, {}));

    // 2 different transactions that have 1 overlapping input.
    auto b_tx_conflict_partial = MakeTransactionSpending({outpoints.at(3), outpoints.at(4)}, det_rand);
    block.vtx.emplace_back(b_tx_conflict_partial);

    auto o_tx_conflict_partial_2 = MakeTransactionSpending({outpoints.at(4), outpoints.at(5)}, det_rand);
    BOOST_CHECK(orphanage.AddTx(o_tx_conflict_partial_2, node, {}));

    const auto removed = orphanage.EraseForBlock(block);
    BOOST_CHECK_EQUAL(orphanage.Size(), 0);
    for (const auto& expected_removed : {bo_tx_same_txid, o_tx_same_txid_diff_witness, o_tx_conflict, o_tx_conflict_partial_2}) {
        const auto& expected_removed_wtxid = expected_removed->GetWitnessHash();
        BOOST_CHECK(std::find_if(removed.begin(), removed.end(), [&](const auto& wtxid) { return wtxid == expected_removed_wtxid; }) != removed.end());
    }
}

BOOST_AUTO_TEST_CASE(multiple_announcers)
{
    const NodeId node0{0};
    const NodeId node1{1};
    const NodeId node2{2};
    size_t expected_total_count{0};
    size_t expected_total_size{0};
    size_t expected_node0_size{0};
    size_t expected_node1_size{0};
    TxOrphanageTest orphanage;
    FastRandomContext det_rand{true};

    // Check accounting per peer.
    // Check that EraseForPeer works with multiple announcers.
    {
        auto ptx{MakeLargeOrphan()};
        const auto& wtxid = ptx->GetWitnessHash();
        const auto tx_size = ptx->GetTotalSize();
        BOOST_CHECK(orphanage.AddTx(ptx, node0, {}));
        BOOST_CHECK(orphanage.HaveTx(GenTxid::Txid(ptx->GetHash())));
        BOOST_CHECK(orphanage.HaveTx(GenTxid::Wtxid(wtxid)));
        expected_total_size += tx_size;
        expected_total_count += 1;
        expected_node0_size += tx_size;
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);

        // Adding again should do nothing.
        BOOST_CHECK(!orphanage.AddTx(ptx, node0, {}));
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);

        // Adding another tx with the same txid but different witness should not work.
        auto ptx_mutated{MakeMutation(ptx)};
        BOOST_CHECK(!orphanage.AddTx(ptx_mutated, node0, {}));
        BOOST_CHECK(!orphanage.HaveTx(GenTxid::Wtxid(ptx_mutated->GetWitnessHash())));

        // It's too late to add parent_txids through AddTx.
        BOOST_CHECK(!orphanage.AddTx(ptx, node0, {ptx->vin.at(0).prevout.hash}));
        // Parent txids is empty because the tx exists but no parent_txids were provided.
        BOOST_CHECK(orphanage.GetParentTxids(wtxid)->empty());
        // Parent txids is std::nullopt because the tx doesn't exist.
        BOOST_CHECK(!orphanage.GetParentTxids(ptx_mutated->GetWitnessHash()).has_value());

        // Adding existing tx for another peer should change that peer's bytes, but not total bytes.
        BOOST_CHECK(!orphanage.AddTx(ptx, node1, {}));
        expected_node1_size += tx_size;
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);

        // Adding new announcer for an existing tx should change that peer's bytes, but not total bytes.
        orphanage.AddAnnouncer(ptx->GetWitnessHash(), node2);
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node2), tx_size);
        orphanage.EraseOrphanOfPeer(ptx->GetWitnessHash(), node2);
        // AddAnnouncer is by wtxid, not txid.
        orphanage.AddAnnouncer(ptx->GetHash(), node2);
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node2), 0);

        // if EraseForPeer is called for an orphan with multiple announcers, the orphanage should only
        // decrement the number of bytes for that peer.
        orphanage.EraseForPeer(node0);
        expected_node0_size -= tx_size;
        BOOST_CHECK(orphanage.HaveTx(GenTxid::Txid(ptx->GetHash())));
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);

        // EraseForPeer should delete the orphan if it's the only announcer left.
        orphanage.EraseForPeer(node1);
        expected_total_count -= 1;
        expected_total_size -= tx_size;
        expected_node1_size -= tx_size;
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);
        BOOST_CHECK(!orphanage.HaveTx(GenTxid::Txid(ptx->GetHash())));
    }

    // EraseOrphanOfPeer only erases the tx for 1 peer
    {
        auto ptx = MakeTransactionSpending({}, det_rand);
        const auto& wtxid = ptx->GetWitnessHash();
        const auto tx_size = ptx->GetTotalSize();

        // Add from node0
        BOOST_CHECK(orphanage.AddTx(ptx, node0, {}));
        expected_total_size += tx_size;
        expected_total_count += 1;
        expected_node0_size += tx_size;
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);
        BOOST_CHECK(orphanage.HaveTxAndPeer(GenTxid::Wtxid(wtxid), node0));

        // Add from node1
        BOOST_CHECK(!orphanage.AddTx(ptx, node1, {}));
        expected_node1_size += tx_size;
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);
        BOOST_CHECK(orphanage.HaveTxAndPeer(GenTxid::Wtxid(wtxid), node1));

        // EraseOrphanOfPeer shouldn't work with txid, only with wtxid
        orphanage.EraseOrphanOfPeer(ptx->GetHash(), node1);
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);

        // Erase just for node1
        orphanage.EraseOrphanOfPeer(wtxid, node1);
        expected_node1_size -= tx_size;
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);
        BOOST_CHECK(orphanage.HaveTxAndPeer(GenTxid::Wtxid(wtxid), node0));
        BOOST_CHECK(!orphanage.HaveTxAndPeer(GenTxid::Wtxid(wtxid), node1));

        // Now erase for node0
        orphanage.EraseOrphanOfPeer(wtxid, node0);
        expected_total_count -= 1;
        expected_total_size -= tx_size;
        expected_node0_size -= tx_size;
        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);
    }

    // Check that erasure for blocks removes for all peers.
    {
        CBlock block;
        auto tx_block = MakeTransactionSpending({}, det_rand);
        const auto tx_block_size = tx_block->GetTotalSize();
        block.vtx.emplace_back(tx_block);
        orphanage.AddTx(tx_block, node0, {});
        orphanage.AddTx(tx_block, node1, {});

        expected_total_count += 1;
        expected_total_size += tx_block_size;
        expected_node0_size += tx_block_size;
        expected_node1_size += tx_block_size;

        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);

        const auto removed = orphanage.EraseForBlock(block);
        BOOST_CHECK_EQUAL(removed.size(), 1);
        BOOST_CHECK_EQUAL(removed.front(), tx_block->GetWitnessHash());

        expected_total_count -= 1;
        expected_total_size -= tx_block_size;
        expected_node0_size -= tx_block_size;
        expected_node1_size -= tx_block_size;

        BOOST_CHECK_EQUAL(orphanage.Size(), expected_total_count);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), expected_total_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node0), expected_node0_size);
        BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node1), expected_node1_size);
    }
}

BOOST_AUTO_TEST_CASE(peer_worksets)
{
    const NodeId node0{0};
    const NodeId node1{1};
    const NodeId node2{2};
    TxOrphanageTest orphanage;
    FastRandomContext det_rand{true};
    // AddChildrenToWorkSet should pick an announcer randomly
    {
        auto tx_missing_parent = MakeTransactionSpending({}, det_rand);
        auto tx_orphan = MakeTransactionSpending({COutPoint{tx_missing_parent->GetHash(), 0}}, det_rand);
        const auto orphan_size = tx_orphan->GetTotalSize();

        // All 3 peers are announcers.
        BOOST_CHECK(orphanage.AddTx(tx_orphan, node0, {tx_missing_parent->GetHash()}));
        BOOST_CHECK(!orphanage.AddTx(tx_orphan, node1, {tx_missing_parent->GetHash()}));
        orphanage.AddAnnouncer(tx_orphan->GetWitnessHash(), node2);
        for (NodeId node = node0; node <= node2; ++node) {
            BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node), orphan_size);
        }

        // Parent accepted: add child to all 3 worksets.
        orphanage.AddChildrenToWorkSet(*tx_missing_parent);
        BOOST_CHECK_EQUAL(orphanage.GetTxToReconsider(node0), tx_orphan);
        BOOST_CHECK_EQUAL(orphanage.GetTxToReconsider(node1), tx_orphan);
        // Don't call GetTxToReconsider(node2) yet because it mutates the workset.

        // EraseOrphanOfPeer also removes that tx from the workset.
        orphanage.EraseOrphanOfPeer(tx_orphan->GetWitnessHash(), node0);
        BOOST_CHECK_EQUAL(orphanage.GetTxToReconsider(node0), nullptr);

        // However, the other peers' worksets are not touched.
        BOOST_CHECK_EQUAL(orphanage.GetTxToReconsider(node2), tx_orphan);

        // Delete this tx, clearing the orphanage.
        BOOST_CHECK_EQUAL(orphanage.EraseTx(tx_orphan->GetWitnessHash()), 1);
        BOOST_CHECK_EQUAL(orphanage.TotalOrphanBytes(), 0);
        BOOST_CHECK_EQUAL(orphanage.Size(), 0);
        for (NodeId node = node0; node <= node2; ++node) {
            BOOST_CHECK_EQUAL(orphanage.GetTxToReconsider(node), nullptr);
            BOOST_CHECK_EQUAL(orphanage.BytesFromPeer(node), 0);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
