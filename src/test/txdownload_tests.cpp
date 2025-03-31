// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <consensus/validation.h>
#include <net_processing.h>
#include <node/txdownloadman_impl.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <validation.h>

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

    // Constructor. We are passing and casting ints because they are more readable in a table (see expected_behaviors).
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
    {TxValidationResult::TX_CONSENSUS,               {/*txid_rejects*/0,/*wtxid_rejects*/1,/*txid_recon*/0,/*wtxid_recon*/0,/*keep*/1,/*txid_inv*/0,/*wtxid_inv*/1}},
    {TxValidationResult::TX_INPUTS_NOT_STANDARD,     {                1,                 1,              0,               0,        1,            1,             1}},
    {TxValidationResult::TX_NOT_STANDARD,            {                0,                 1,              0,               0,        1,            0,             1}},
    {TxValidationResult::TX_MISSING_INPUTS,          {                0,                 0,              0,               0,        1,            0,             1}},
    {TxValidationResult::TX_PREMATURE_SPEND,         {                0,                 1,              0,               0,        1,            0,             1}},
    {TxValidationResult::TX_WITNESS_MUTATED,         {                0,                 1,              0,               0,        1,            0,             1}},
    {TxValidationResult::TX_WITNESS_STRIPPED,        {                0,                 0,              0,               0,        0,            0,             0}},
    {TxValidationResult::TX_CONFLICT,                {                0,                 1,              0,               0,        1,            0,             1}},
    {TxValidationResult::TX_MEMPOOL_POLICY,          {                0,                 1,              0,               0,        1,            0,             1}},
    {TxValidationResult::TX_NO_MEMPOOL,              {                0,                 1,              0,               0,        1,            0,             1}},
    {TxValidationResult::TX_RECONSIDERABLE,          {                0,                 0,              0,               1,        1,            0,             1}},
    {TxValidationResult::TX_UNKNOWN,                 {                0,                 1,              0,               0,        1,            0,             1}},
};

static bool CheckOrphanBehavior(node::TxDownloadManagerImpl& txdownload_impl, const CTransactionRef& tx, const node::RejectedTxTodo& ret, std::string& err_msg,
                                bool expect_orphan, bool expect_keep, unsigned int expected_parents)
{
    // Missing inputs can never result in a PackageToValidate.
    if (ret.m_package_to_validate.has_value()) {
        err_msg = strprintf("returned a PackageToValidate on missing inputs");
        return false;
    }

    if (expect_orphan != txdownload_impl.m_orphanage.HaveTx(tx->GetWitnessHash())) {
        err_msg = strprintf("unexpectedly %s tx in orphanage", expect_orphan ? "did not find" : "found");
        return false;
    }
    if (expect_keep != ret.m_should_add_extra_compact_tx) {
        err_msg = strprintf("unexpectedly returned %s add to vExtraTxnForCompact", expect_keep ? "should not" : "should");
        return false;
    }
    if (expected_parents != ret.m_unique_parents.size()) {
        err_msg = strprintf("expected %u unique_parents, got %u", expected_parents, ret.m_unique_parents.size());
        return false;
    }
    return true;
}

static CTransactionRef CreatePlaceholderTx(bool segwit)
{
    // Each tx returned from here spends the previous one.
    static Txid prevout_hash{};

    CMutableTransaction mtx;
    mtx.vin.emplace_back(prevout_hash, 0);
    // This makes txid != wtxid
    if (segwit) mtx.vin[0].scriptWitness.stack.push_back({1});
    mtx.vout.emplace_back(CENT, CScript());
    auto ptx = MakeTransactionRef(mtx);
    prevout_hash = ptx->GetHash();
    return ptx;
}

BOOST_FIXTURE_TEST_CASE(tx_rejection_types, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    FastRandomContext det_rand{true};
    node::TxDownloadOptions DEFAULT_OPTS{pool, det_rand, DEFAULT_MAX_ORPHAN_TRANSACTIONS, true};

    // A new TxDownloadManagerImpl is created for each tx so we can just reuse the same one.
    TxValidationState state;
    NodeId nodeid{0};
    std::chrono::microseconds now{GetTime()};
    node::TxDownloadConnectionInfo connection_info{/*m_preferred=*/false, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true};

    for (const auto segwit_parent : {true, false}) {
        for (const auto segwit_child : {true, false}) {
            const auto ptx_parent = CreatePlaceholderTx(segwit_parent);
            const auto ptx_child = CreatePlaceholderTx(segwit_child);
            const auto& parent_txid = ptx_parent->GetHash();
            const auto& parent_wtxid = ptx_parent->GetWitnessHash();
            const auto& child_txid = ptx_child->GetHash();
            const auto& child_wtxid = ptx_child->GetWitnessHash();

            for (const auto& [result, expected_behavior] : expected_behaviors) {
                node::TxDownloadManagerImpl txdownload_impl{DEFAULT_OPTS};
                txdownload_impl.ConnectedPeer(nodeid, connection_info);
                // Parent failure
                state.Invalid(result, "");
                const auto& [keep, unique_txids, package_to_validate] = txdownload_impl.MempoolRejectedTx(ptx_parent, state, nodeid, /*first_time_failure=*/true);

                // No distinction between txid and wtxid caching for nonsegwit transactions, so only test these specific
                // behaviors for segwit transactions.
                Behaviors actual_behavior{
                    /*txid_rejects=*/txdownload_impl.RecentRejectsFilter().contains(parent_txid.ToUint256()),
                    /*wtxid_rejects=*/txdownload_impl.RecentRejectsFilter().contains(parent_wtxid.ToUint256()),
                    /*txid_recon=*/txdownload_impl.RecentRejectsReconsiderableFilter().contains(parent_txid.ToUint256()),
                    /*wtxid_recon=*/txdownload_impl.RecentRejectsReconsiderableFilter().contains(parent_wtxid.ToUint256()),
                    /*keep=*/keep,
                    /*txid_inv=*/txdownload_impl.AddTxAnnouncement(nodeid, parent_txid, now),
                    /*wtxid_inv=*/txdownload_impl.AddTxAnnouncement(nodeid, parent_wtxid, now),
                };
                BOOST_TEST_MESSAGE("Testing behavior for " << result << (segwit_parent ? " segwit " : " nonsegwit"));
                actual_behavior.CheckEqual(expected_behavior, /*segwit=*/segwit_parent);

                // Later, a child of this transaction fails for missing inputs
                state.Invalid(TxValidationResult::TX_MISSING_INPUTS, "");
                txdownload_impl.MempoolRejectedTx(ptx_child, state, nodeid, /*first_time_failure=*/true);

                // If parent (by txid) was rejected, child is too.
                const bool parent_txid_rejected{segwit_parent ? expected_behavior.m_txid_in_rejects : expected_behavior.m_wtxid_in_rejects};
                BOOST_CHECK_EQUAL(parent_txid_rejected, txdownload_impl.RecentRejectsFilter().contains(child_txid.ToUint256()));
                BOOST_CHECK_EQUAL(parent_txid_rejected, txdownload_impl.RecentRejectsFilter().contains(child_wtxid.ToUint256()));

                // Unless rejected, the child should be in orphanage.
                BOOST_CHECK_EQUAL(!parent_txid_rejected, txdownload_impl.m_orphanage.HaveTx(ptx_child->GetWitnessHash()));
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(handle_missing_inputs, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    FastRandomContext det_rand{true};
    node::TxDownloadOptions DEFAULT_OPTS{pool, det_rand, DEFAULT_MAX_ORPHAN_TRANSACTIONS, true};
    NodeId nodeid{1};
    node::TxDownloadConnectionInfo DEFAULT_CONN{/*m_preferred=*/false, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true};

    // We need mature coinbases
    mineBlocks(20);

    // Transactions with missing inputs are treated differently depending on how much we know about
    // their parents.
    CKey wallet_key = GenerateRandomKey();
    CScript destination = GetScriptForDestination(PKHash(wallet_key.GetPubKey()));
    // Amount for spending coinbase in a 1-in-1-out tx, at depth n, each time deducting 1000 from the amount as fees.
    CAmount amount_depth_1{50 * COIN - 1000};
    CAmount amount_depth_2{amount_depth_1 - 1000};
    // Amount for spending coinbase in a 1-in-2-out tx, deducting 1000 in fees
    CAmount amount_split_half{25 * COIN - 500};
    int test_chain_height{100};

    TxValidationState state_orphan;
    state_orphan.Invalid(TxValidationResult::TX_MISSING_INPUTS, "");

    // Transactions are not all submitted to mempool. Conserve the number of m_coinbase_txns we
    // consume, and only increment this index number when we would conflict with an existing
    // mempool transaction.
    size_t coinbase_idx{0};

    for (int decisions = 0; decisions < (1 << 4); ++decisions) {
        auto mtx_single_parent = CreateValidMempoolTransaction(m_coinbase_txns[coinbase_idx], /*input_vout=*/0, test_chain_height, coinbaseKey, destination, amount_depth_1, /*submit=*/false);
        auto single_parent = MakeTransactionRef(mtx_single_parent);

        auto mtx_orphan = CreateValidMempoolTransaction(single_parent, /*input_vout=*/0, test_chain_height, wallet_key, destination, amount_depth_2, /*submit=*/false);
        auto orphan = MakeTransactionRef(mtx_orphan);

        node::TxDownloadManagerImpl txdownload_impl{DEFAULT_OPTS};
        txdownload_impl.ConnectedPeer(nodeid, DEFAULT_CONN);

        // Each bit of decisions tells us whether the parent is in a particular cache.
        // It is definitely possible for a transaction to be in multiple caches. For example, it
        // may have both a low feerate and found to violate some mempool policy when validated
        // in a 1p1c.
        const bool parent_recent_rej(decisions & 1);
        const bool parent_recent_rej_recon((decisions >> 1) & 1);
        const bool parent_recent_conf((decisions >> 2) & 1);
        const bool parent_in_mempool((decisions >> 3) & 1);

        if (parent_recent_rej) txdownload_impl.RecentRejectsFilter().insert(single_parent->GetHash().ToUint256());
        if (parent_recent_rej_recon) txdownload_impl.RecentRejectsReconsiderableFilter().insert(single_parent->GetHash().ToUint256());
        if (parent_recent_conf) txdownload_impl.RecentConfirmedTransactionsFilter().insert(single_parent->GetHash().ToUint256());
        if (parent_in_mempool) {
            const auto mempool_result = WITH_LOCK(::cs_main, return m_node.chainman->ProcessTransaction(single_parent));
            BOOST_CHECK(mempool_result.m_result_type == MempoolAcceptResult::ResultType::VALID);
            coinbase_idx += 1;
            assert(coinbase_idx < m_coinbase_txns.size());
        }

        // Whether or not the transaction is added as an orphan depends solely on whether or not
        // it's in RecentRejectsFilter. Specifically, the parent is allowed to be in
        // RecentRejectsReconsiderableFilter, but it cannot be in RecentRejectsFilter.
        const bool expect_keep_orphan = !parent_recent_rej;
        const unsigned int expected_parents = parent_recent_rej || parent_recent_conf || parent_in_mempool ? 0 : 1;
        // If we don't expect to keep the orphan then expected_parents is 0.
        // !expect_keep_orphan => (expected_parents == 0)
        BOOST_CHECK(expect_keep_orphan || expected_parents == 0);
        const auto ret_1p1c = txdownload_impl.MempoolRejectedTx(orphan, state_orphan, nodeid, /*first_time_failure=*/true);
        std::string err_msg;
        const bool ok = CheckOrphanBehavior(txdownload_impl, orphan, ret_1p1c, err_msg,
                                            /*expect_orphan=*/expect_keep_orphan, /*expect_keep=*/true, /*expected_parents=*/expected_parents);
        BOOST_CHECK_MESSAGE(ok, err_msg);
    }

    // Orphan with multiple parents
    {
        std::vector<CTransactionRef> parents;
        std::vector<COutPoint> outpoints;
        int32_t num_parents{24};
        for (int32_t i = 0; i < num_parents; ++i) {
            assert(coinbase_idx < m_coinbase_txns.size());
            auto mtx_parent = CreateValidMempoolTransaction(m_coinbase_txns[coinbase_idx++], /*input_vout=*/0, test_chain_height,
                                                            coinbaseKey, destination, amount_depth_1 + i, /*submit=*/false);
            auto ptx_parent = MakeTransactionRef(mtx_parent);
            parents.emplace_back(ptx_parent);
            outpoints.emplace_back(ptx_parent->GetHash(), 0);
        }

        // Send all coins to 1 output.
        auto mtx_orphan = CreateValidMempoolTransaction(parents, outpoints, test_chain_height, {wallet_key}, {{amount_depth_2 * num_parents, destination}}, /*submit=*/false);
        auto orphan = MakeTransactionRef(mtx_orphan);

        // 1 parent in RecentRejectsReconsiderableFilter, the rest are unknown
        {
            node::TxDownloadManagerImpl txdownload_impl{DEFAULT_OPTS};
            txdownload_impl.ConnectedPeer(nodeid, DEFAULT_CONN);

            txdownload_impl.RecentRejectsReconsiderableFilter().insert(parents[0]->GetHash().ToUint256());
            const auto ret_1p1c_parent_reconsiderable = txdownload_impl.MempoolRejectedTx(orphan, state_orphan, nodeid, /*first_time_failure=*/true);
            std::string err_msg;
            const bool ok = CheckOrphanBehavior(txdownload_impl, orphan, ret_1p1c_parent_reconsiderable, err_msg,
                                                /*expect_orphan=*/true, /*expect_keep=*/true, /*expected_parents=*/num_parents);
            BOOST_CHECK_MESSAGE(ok, err_msg);
        }

        // 1 parent in RecentRejectsReconsiderableFilter, the rest are confirmed
        {
            node::TxDownloadManagerImpl txdownload_impl{DEFAULT_OPTS};
            txdownload_impl.ConnectedPeer(nodeid, DEFAULT_CONN);

            txdownload_impl.RecentRejectsReconsiderableFilter().insert(parents[0]->GetHash().ToUint256());
            for (int32_t i = 1; i < num_parents; ++i) {
                txdownload_impl.RecentConfirmedTransactionsFilter().insert(parents[i]->GetHash().ToUint256());
            }
            const unsigned int expected_parents = 1;

            const auto ret_1recon_conf = txdownload_impl.MempoolRejectedTx(orphan, state_orphan, nodeid, /*first_time_failure=*/true);
            std::string err_msg;
            const bool ok = CheckOrphanBehavior(txdownload_impl, orphan, ret_1recon_conf, err_msg,
                                                /*expect_orphan=*/true, /*expect_keep=*/true, /*expected_parents=*/expected_parents);
            BOOST_CHECK_MESSAGE(ok, err_msg);
        }

        // 1 parent in RecentRejectsReconsiderableFilter, 1 other in {RecentRejectsReconsiderableFilter, RecentRejectsFilter}
        for (int i = 0; i < 2; ++i) {
            node::TxDownloadManagerImpl txdownload_impl{DEFAULT_OPTS};
            txdownload_impl.ConnectedPeer(nodeid, DEFAULT_CONN);

            txdownload_impl.RecentRejectsReconsiderableFilter().insert(parents[1]->GetHash().ToUint256());

            // Doesn't really matter which parent
            auto& alreadyhave_parent = parents[0];
            if (i == 0) {
                txdownload_impl.RecentRejectsReconsiderableFilter().insert(alreadyhave_parent->GetHash().ToUint256());
            } else if (i == 1) {
                txdownload_impl.RecentRejectsFilter().insert(alreadyhave_parent->GetHash().ToUint256());
            }

            const auto ret_2_problems = txdownload_impl.MempoolRejectedTx(orphan, state_orphan, nodeid, /*first_time_failure=*/true);
            std::string err_msg;
            const bool ok = CheckOrphanBehavior(txdownload_impl, orphan, ret_2_problems, err_msg,
                                                /*expect_orphan=*/false, /*expect_keep=*/true, /*expected_parents=*/0);
            BOOST_CHECK_MESSAGE(ok, err_msg);
        }
    }

    // Orphan with multiple inputs spending from a single parent
    {
        assert(coinbase_idx < m_coinbase_txns.size());
        auto parent_2outputs = MakeTransactionRef(CreateValidMempoolTransaction({m_coinbase_txns[coinbase_idx]}, {{m_coinbase_txns[coinbase_idx]->GetHash(), 0}}, test_chain_height, {coinbaseKey},
                                                             {{amount_split_half, destination}, {amount_split_half, destination}}, /*submit=*/false));

        auto orphan = MakeTransactionRef(CreateValidMempoolTransaction({parent_2outputs}, {{parent_2outputs->GetHash(), 0}, {parent_2outputs->GetHash(), 1}},
                                                                       test_chain_height, {wallet_key}, {{amount_depth_2, destination}}, /*submit=*/false));
        // Parent is in RecentRejectsReconsiderableFilter. Inputs will find it twice, but this
        // should only counts as 1 parent in the filter.
        {
            node::TxDownloadManagerImpl txdownload_impl{DEFAULT_OPTS};
            txdownload_impl.ConnectedPeer(nodeid, DEFAULT_CONN);

            txdownload_impl.RecentRejectsReconsiderableFilter().insert(parent_2outputs->GetHash().ToUint256());
            const auto ret_1p1c_2reconsiderable = txdownload_impl.MempoolRejectedTx(orphan, state_orphan, nodeid, /*first_time_failure=*/true);
            std::string err_msg;
            const bool ok = CheckOrphanBehavior(txdownload_impl, orphan, ret_1p1c_2reconsiderable, err_msg,
                                                /*expect_orphan=*/true, /*expect_keep=*/true, /*expected_parents=*/1);
            BOOST_CHECK_MESSAGE(ok, err_msg);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
