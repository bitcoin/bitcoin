// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_processing.h>
#include <node/txdownloadman_impl.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>

#include <array>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txdownload_tests, TestingSetup)

struct Behaviors {
    bool m_txid_in_rejects;
    bool m_wtxid_in_rejects;
    bool m_txid_in_rejects_recon;
    bool m_wtxid_in_rejects_recon;
    bool m_keep_for_compact;
    bool m_ignore_inv_txid;
    bool m_ignore_inv_wtxid;

    // Constructor. We are passing and casting ints because they are more readable in a table (see all_expected_results).
    Behaviors(bool txid_rejects, bool wtxid_rejects, bool txid_recon, bool wtxid_recon, bool keep, bool txid_inv, bool wtxid_inv) :
        m_txid_in_rejects(txid_rejects),
        m_wtxid_in_rejects(wtxid_rejects),
        m_txid_in_rejects_recon(txid_recon),
        m_wtxid_in_rejects_recon(wtxid_recon),
        m_keep_for_compact(keep),
        m_ignore_inv_txid(txid_inv),
        m_ignore_inv_wtxid(wtxid_inv)
    {}

    void CheckEqual(const Behaviors& other, bool segwit)
    {
        BOOST_CHECK_EQUAL(other.m_wtxid_in_rejects,       m_wtxid_in_rejects);
        BOOST_CHECK_EQUAL(other.m_wtxid_in_rejects_recon, m_wtxid_in_rejects_recon);
        BOOST_CHECK_EQUAL(other.m_keep_for_compact,       m_keep_for_compact);
        BOOST_CHECK_EQUAL(other.m_ignore_inv_wtxid,       m_ignore_inv_wtxid);

        // false negatives for nonsegwit transactions, since txid == wtxid.
        if (segwit) {
            BOOST_CHECK_EQUAL(other.m_txid_in_rejects,        m_txid_in_rejects);
            BOOST_CHECK_EQUAL(other.m_txid_in_rejects_recon,  m_txid_in_rejects_recon);
            BOOST_CHECK_EQUAL(other.m_ignore_inv_txid,        m_ignore_inv_txid);
        }
    }
};

// Map from failure reason to expected behavior for a segwit tx that fails
// Txid and Wtxid are assumed to be different here. For a nonsegwit transaction, use the wtxid results.
static std::map<TxValidationResult, Behaviors> expected_behaviors{
    {TxValidationResult::TX_CONSENSUS,               {/*txid_rejects*/0, /*wtxid_rejects*/1, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
    {TxValidationResult::TX_RECENT_CONSENSUS_CHANGE, {/*txid_rejects*/0, /*wtxid_rejects*/1, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
    {TxValidationResult::TX_INPUTS_NOT_STANDARD,     {/*txid_rejects*/1, /*wtxid_rejects*/1, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/1, /*wtxid_inv*/1}},
    {TxValidationResult::TX_NOT_STANDARD,            {/*txid_rejects*/0, /*wtxid_rejects*/1, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
    {TxValidationResult::TX_MISSING_INPUTS,          {/*txid_rejects*/0, /*wtxid_rejects*/0, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
    {TxValidationResult::TX_PREMATURE_SPEND,         {/*txid_rejects*/0, /*wtxid_rejects*/1, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
    {TxValidationResult::TX_WITNESS_MUTATED,         {/*txid_rejects*/0, /*wtxid_rejects*/1, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
    {TxValidationResult::TX_WITNESS_STRIPPED,        {/*txid_rejects*/0, /*wtxid_rejects*/0, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/0, /*txid_inv*/0, /*wtxid_inv*/0}},
    {TxValidationResult::TX_CONFLICT,                {/*txid_rejects*/0, /*wtxid_rejects*/1, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
    {TxValidationResult::TX_MEMPOOL_POLICY,          {/*txid_rejects*/0, /*wtxid_rejects*/1, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
    {TxValidationResult::TX_NO_MEMPOOL,              {/*txid_rejects*/0, /*wtxid_rejects*/1, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
    {TxValidationResult::TX_RECONSIDERABLE,          {/*txid_rejects*/0, /*wtxid_rejects*/0, /*txid_recon*/0, /*wtxid_recon*/1, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
    {TxValidationResult::TX_UNKNOWN,                 {/*txid_rejects*/0, /*wtxid_rejects*/1, /*txid_recon*/0, /*wtxid_recon*/0, /*keep*/1, /*txid_inv*/0, /*wtxid_inv*/1}},
};

static std::vector<CTransactionRef> CreateTransactionChain(size_t num_txns, bool segwit)
{
    // Ensure every transaction has a different txid by having each one spend the previous one.
    static Txid prevout_hash{};

    std::vector<CTransactionRef> txns;
    txns.reserve(num_txns);
    for (uint32_t i = 0; i < num_txns; ++i) {
        CMutableTransaction mtx;
        mtx.vin.emplace_back(prevout_hash, 0);
        // This makes txid != wtxid
        if (segwit) mtx.vin[0].scriptWitness.stack.push_back({1});
        mtx.vout.emplace_back(CENT, CScript());
        auto ptx{MakeTransactionRef(mtx)};
        txns.emplace_back(ptx);
        prevout_hash = ptx->GetHash();
    }
    return txns;
}

BOOST_FIXTURE_TEST_CASE(tx_rejection_types, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    FastRandomContext det_rand{true};

    // A new TxDownloadManagerImpl is created for each tx so we can just reuse the same one.
    TxValidationState state;
    NodeId nodeid{0};
    std::chrono::microseconds now{GetTime()};
    node::TxDownloadConnectionInfo connection_info{/*m_preferred=*/false, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true};

    for (const auto segwit_parent : {true, false}) {
        for (const auto segwit_child : {true, false}) {
            const auto ptx_parent = CreateTransactionChain(1, segwit_parent).front();
            const auto ptx_child = CreateTransactionChain(1, segwit_child).front();
            const auto& parent_txid = ptx_parent->GetHash().ToUint256();
            const auto& parent_wtxid = ptx_parent->GetWitnessHash().ToUint256();
            const auto& child_txid = ptx_child->GetHash().ToUint256();
            const auto& child_wtxid = ptx_child->GetWitnessHash().ToUint256();

            for (const auto& [result, expected_behavior] : expected_behaviors) {
                node::TxDownloadManagerImpl txdownload_impl{node::TxDownloadOptions{pool, det_rand, DEFAULT_MAX_ORPHAN_TRANSACTIONS, true}};
                txdownload_impl.ConnectedPeer(nodeid, connection_info);
                // Parent failure
                state.Invalid(result, "");
                const auto& [keep, unique_txids, package_to_validate] = txdownload_impl.MempoolRejectedTx(ptx_parent, state, nodeid, /*first_time_failure=*/true);

                // No distinction between txid and wtxid caching for nonsegwit transactions, so only test these specific
                // behaviors for segwit transactions.
                Behaviors actual_behavior{
                    /*txid_rejects=*/txdownload_impl.RecentRejectsFilter().contains(parent_txid),
                    /*wtxid_rejects=*/txdownload_impl.RecentRejectsFilter().contains(parent_wtxid),
                    /*txid_recon=*/txdownload_impl.RecentRejectsReconsiderableFilter().contains(parent_txid),
                    /*wtxid_recon=*/txdownload_impl.RecentRejectsReconsiderableFilter().contains(parent_wtxid),
                    /*keep=*/keep,
                    /*txid_inv=*/txdownload_impl.AddTxAnnouncement(nodeid, GenTxid::Txid(parent_txid), now, /*p2p_inv=*/true),
                    /*wtxid_inv=*/txdownload_impl.AddTxAnnouncement(nodeid, GenTxid::Wtxid(parent_wtxid), now, /*p2p_inv=*/true),
                };
                BOOST_TEST_MESSAGE("Testing behavior for " << result << (segwit_parent ? " segwit " : " nonsegwit"));
                actual_behavior.CheckEqual(expected_behavior, /*segwit=*/segwit_parent);

                // Later, a child of this transaction fails for missing inputs
                state.Invalid(TxValidationResult::TX_MISSING_INPUTS, "");
                txdownload_impl.MempoolRejectedTx(ptx_child, state, nodeid, /*first_time_failure=*/true);

                // If parent (by txid) was rejected, child is too.
                const bool parent_txid_rejected{segwit_parent ? expected_behavior.m_txid_in_rejects : expected_behavior.m_wtxid_in_rejects};
                BOOST_CHECK_EQUAL(parent_txid_rejected, txdownload_impl.RecentRejectsFilter().contains(child_txid));
                BOOST_CHECK_EQUAL(parent_txid_rejected, txdownload_impl.RecentRejectsFilter().contains(child_wtxid));

                // Unless rejected, the child should be in orphanage.
                BOOST_CHECK_EQUAL(!parent_txid_rejected, txdownload_impl.m_orphanage.HaveTx(ptx_child->GetWitnessHash()));
            }
        }
    }

    // More specific examples of orphans with parent(s) that we have seen before.
    TxValidationState state_reconsiderable;
    state_reconsiderable.Invalid(TxValidationResult::TX_RECONSIDERABLE, "");

    TxValidationState state_orphan;
    state_orphan.Invalid(TxValidationResult::TX_MISSING_INPUTS, "");

    const auto ptx_parent_1 = CreateTransactionChain(1, /*segwit=*/false).front();
    const auto ptx_parent_2 = CreateTransactionChain(1, /*segwit=*/false).front();
    CMutableTransaction mtx_child;
    mtx_child.vin.emplace_back(ptx_parent_1->GetHash(), 0);
    mtx_child.vin.emplace_back(ptx_parent_2->GetHash(), 0);
    mtx_child.vout.emplace_back(CENT, CScript());
    auto ptx_child = MakeTransactionRef(mtx_child);
    const std::vector<uint256> vec_parent_txids = {ptx_parent_1->GetHash(), ptx_parent_2->GetHash()};

    // 1 reconsiderable parent
    {
        node::TxDownloadManagerImpl txdownload_impl{node::TxDownloadOptions{pool, det_rand, DEFAULT_MAX_ORPHAN_TRANSACTIONS, true}};
        txdownload_impl.ConnectedPeer(nodeid, connection_info);

        // Parent reconsiderable failure
        const auto& [p1_keep, p1_unique_txids, p1_package] = txdownload_impl.MempoolRejectedTx(ptx_parent_1, state_reconsiderable, nodeid, /*first_time_failure=*/true);
        BOOST_CHECK(p1_keep);
        BOOST_CHECK(p1_unique_txids.empty());
        BOOST_CHECK(!p1_package.has_value());

        // Child missing inputs, should be added to orphanage.
        const auto& [c_keep, c_unique_txids, c_package] = txdownload_impl.MempoolRejectedTx(ptx_child, state_orphan, nodeid, /*first_time_failure=*/true);
        BOOST_CHECK(c_keep);
        BOOST_CHECK(c_unique_txids == vec_parent_txids);
        BOOST_CHECK(!c_package.has_value());
        BOOST_CHECK(txdownload_impl.m_orphanage.HaveTx(ptx_child->GetWitnessHash()));
    }

    // 2 reconsiderable parents
    {
        node::TxDownloadManagerImpl txdownload_impl{node::TxDownloadOptions{pool, det_rand, DEFAULT_MAX_ORPHAN_TRANSACTIONS, true}};
        txdownload_impl.ConnectedPeer(nodeid, connection_info);

        // Parent reconsiderable failure
        const auto& [p1_keep, p1_unique_txids, p1_package] = txdownload_impl.MempoolRejectedTx(ptx_parent_1, state_reconsiderable, nodeid, /*first_time_failure=*/true);
        BOOST_CHECK(p1_keep);
        BOOST_CHECK(p1_unique_txids.empty());
        BOOST_CHECK(!p1_package.has_value());

        const auto& [p2_keep, p2_unique_txids, p2_package] = txdownload_impl.MempoolRejectedTx(ptx_parent_2, state_reconsiderable, nodeid, /*first_time_failure=*/true);
        BOOST_CHECK(p2_keep);
        BOOST_CHECK(p2_unique_txids.empty());
        BOOST_CHECK(!p2_package.has_value());

        // Child missing inputs. Not added to orphanage because only 1 reconsiderable rejected parent is tolerated.
        const auto& [c_keep, c_unique_txids, c_package] = txdownload_impl.MempoolRejectedTx(ptx_child, state_orphan, nodeid, /*first_time_failure=*/true);
        BOOST_CHECK(c_keep);
        BOOST_CHECK(c_unique_txids == vec_parent_txids);
        BOOST_CHECK(!c_package.has_value());
        BOOST_CHECK(!txdownload_impl.m_orphanage.HaveTx(ptx_child->GetWitnessHash()));
    }

    // 1 reconsiderable parent + 1 non-reconsiderable parent
    {
        node::TxDownloadManagerImpl txdownload_impl{node::TxDownloadOptions{pool, det_rand, DEFAULT_MAX_ORPHAN_TRANSACTIONS, true}};
        txdownload_impl.ConnectedPeer(nodeid, connection_info);

        // Parent reconsiderable failure
        const auto& [p1_keep, p1_unique_txids, p1_package] = txdownload_impl.MempoolRejectedTx(ptx_parent_1, state_reconsiderable, nodeid, /*first_time_failure=*/true);
        BOOST_CHECK(p1_keep);
        BOOST_CHECK(p1_unique_txids.empty());
        BOOST_CHECK(!p1_package.has_value());

        TxValidationState state_not_reconsiderable;
        state_not_reconsiderable.Invalid(TxValidationResult::TX_CONSENSUS, "");

        const auto& [p2_keep, p2_unique_txids, p2_package] = txdownload_impl.MempoolRejectedTx(ptx_parent_2, state_not_reconsiderable, nodeid, /*first_time_failure=*/true);
        BOOST_CHECK(p2_keep);
        BOOST_CHECK(p2_unique_txids.empty());
        BOOST_CHECK(!p2_package.has_value());

        // Child missing inputs. Not added to orphanage because only 1 reconsiderable rejected parent is tolerated.
        const auto& [c_keep, c_unique_txids, c_package] = txdownload_impl.MempoolRejectedTx(ptx_child, state_orphan, nodeid, /*first_time_failure=*/true);
        BOOST_CHECK(c_keep);
        BOOST_CHECK(c_unique_txids == vec_parent_txids);
        BOOST_CHECK(!c_package.has_value());
        BOOST_CHECK(!txdownload_impl.m_orphanage.HaveTx(ptx_child->GetWitnessHash()));

    }

    // Reconsiderable parent of a tx already in orphanage: the only time PackageToValidate is returned.
    {
        node::TxDownloadManagerImpl txdownload_impl{node::TxDownloadOptions{pool, det_rand, DEFAULT_MAX_ORPHAN_TRANSACTIONS, true}};
        txdownload_impl.ConnectedPeer(nodeid, connection_info);

        // Child missing inputs, should be added to orphanage.
        const auto& [c_keep, c_unique_txids, c_package] = txdownload_impl.MempoolRejectedTx(ptx_child, state_orphan, nodeid, /*first_time_failure=*/true);
        BOOST_CHECK(c_keep);
        BOOST_CHECK(c_unique_txids == vec_parent_txids);
        BOOST_CHECK(!c_package.has_value());
        BOOST_CHECK(txdownload_impl.m_orphanage.HaveTx(ptx_child->GetWitnessHash()));

        // Parent reconsiderable failure again
        const auto& [p_keep, p_unique_txids, p_package] = txdownload_impl.MempoolRejectedTx(ptx_parent_1, state_reconsiderable, nodeid, /*first_time_failure=*/true);
        BOOST_CHECK(p_keep);
        BOOST_CHECK(p_unique_txids.empty());
        BOOST_CHECK(p_package.has_value());
        Package vec_1p1c{ptx_parent_1, ptx_child};
        BOOST_CHECK(p_package->m_txns == vec_1p1c);
    }
}

BOOST_AUTO_TEST_SUITE_END()
