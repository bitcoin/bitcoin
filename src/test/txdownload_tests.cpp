// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <consensus/validation.h>
#include <kernel/mempool_removal_reason.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <node/txdownloadman_impl.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <script/script.h>
#include <test/util/common.h>
#include <test/util/net.h>
#include <test/util/random.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <validation.h>

#include <array>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txdownload_tests)

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
    {TxValidationResult::TX_CONFLICT,                {                0,                 0,              0,               0,        1,            0,             0}},
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

    if (expect_orphan != txdownload_impl.m_orphanage->HaveTx(tx->GetWitnessHash())) {
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
    node::TxDownloadOptions DEFAULT_OPTS{.m_mempool = pool, .m_deterministic_txrequest = true};

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
                BOOST_CHECK_EQUAL(!parent_txid_rejected, txdownload_impl.m_orphanage->HaveTx(ptx_child->GetWitnessHash()));
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(handle_missing_inputs, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    node::TxDownloadOptions DEFAULT_OPTS{.m_mempool = pool, .m_deterministic_txrequest = true};
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

// A witnessed parent in the mempool, a stripped copy of it rejected as TX_CONFLICT, then the parent is
// gone: a later child must not be rejected because the known parent's txid is in the reject filter.
BOOST_FIXTURE_TEST_CASE(known_mempool_parent_replay_does_not_poison_after_eviction, TestChain100Setup)
{
    const auto p_input_mtx = CreateValidMempoolTransaction(m_coinbase_txns[0], /*input_vout=*/0, /*input_height=*/1, coinbaseKey, P2WSH_OP_TRUE, 49 * COIN, /*submit=*/false);
    CreateAndProcessBlock({p_input_mtx}, CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG);
    CMutableTransaction parent_mtx;
    parent_mtx.vin.emplace_back(p_input_mtx.GetHash(), 0);
    parent_mtx.vin[0].scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    parent_mtx.vout.emplace_back(49 * COIN - 10000, P2WSH_OP_TRUE); // pays its own fee: accepted alone
    const auto parent = MakeTransactionRef(parent_mtx);
    CMutableTransaction stripped_mtx{parent_mtx};
    stripped_mtx.vin[0].scriptWitness.SetNull();
    const auto parent_stripped = MakeTransactionRef(stripped_mtx);
    CMutableTransaction child_mtx;
    child_mtx.vin.emplace_back(parent->GetHash(), 0);
    child_mtx.vin[0].scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    child_mtx.vout.emplace_back(49 * COIN - 20000, P2WSH_OP_TRUE);
    const auto child = MakeTransactionRef(child_mtx);

    LOCK(cs_main);
    CTxMemPool& pool = *Assert(m_node.mempool);
    node::TxDownloadManagerImpl txdownload_impl{node::TxDownloadOptions{.m_mempool = pool, .m_deterministic_txrequest = true}};
    constexpr NodeId honest{1}, other{2};
    txdownload_impl.ConnectedPeer(honest, {/*m_preferred=*/true, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true});
    txdownload_impl.ConnectedPeer(other, {/*m_preferred=*/false, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true});

    // Parent accepted into the mempool.
    BOOST_REQUIRE(m_node.chainman->ProcessTransaction(parent).m_result_type == MempoolAcceptResult::ResultType::VALID);
    txdownload_impl.MempoolAcceptedTx(parent);
    // Another peer replays a stripped copy: same non-witness data as the mempool entry.
    BOOST_REQUIRE(txdownload_impl.ReceivedTx(other, parent_stripped).first);
    const auto stripped_result = m_node.chainman->ProcessTransaction(parent_stripped);
    BOOST_REQUIRE_MESSAGE(stripped_result.m_state.GetResult() == TxValidationResult::TX_CONFLICT, stripped_result.m_state.ToString());
    txdownload_impl.MempoolRejectedTx(parent_stripped, stripped_result.m_state, other, /*first_time_failure=*/true);
    // The parent leaves the mempool (evicted or replaced), without a tip change.
    WITH_LOCK(pool.cs, pool.removeRecursive(*parent, MemPoolRemovalReason::SIZELIMIT));
    BOOST_REQUIRE(!pool.exists(parent->GetHash()));
    // A child arrives: the parent is genuinely missing now. It must be stored, not rejected.
    BOOST_REQUIRE(txdownload_impl.ReceivedTx(honest, child).first);
    const auto child_result = m_node.chainman->ProcessTransaction(child);
    BOOST_REQUIRE(child_result.m_state.GetResult() == TxValidationResult::TX_MISSING_INPUTS);
    txdownload_impl.MempoolRejectedTx(child, child_result.m_state, honest, /*first_time_failure=*/true);
    BOOST_CHECK_MESSAGE(txdownload_impl.m_orphanage->HaveTxFromPeer(child->GetWitnessHash(), honest),
                        "child rejected because the evicted parent's txid was poisoned by a stripped replay");
}

// A parent rejected for its feerate is delivered again while another witness version of it is in the
// mempool, and is evaluated as a 1p1c package with a child from the orphanage. Its wtxid must not enter
// the reject filter: once the other version leaves the mempool, the honest parent is needed again.
BOOST_FIXTURE_TEST_CASE(known_parent_in_package_does_not_poison_after_eviction, TestChain100Setup)
{
    const CScript drop_true{CScript() << OP_DROP << OP_TRUE};
    const CScript p2wsh_drop_true{GetScriptForDestination(WitnessV0ScriptHash(drop_true))};
    const std::vector<unsigned char> drop_true_bytes{drop_true.begin(), drop_true.end()};
    const auto funding_mtx = CreateValidMempoolTransaction(m_coinbase_txns[0], /*input_vout=*/0, /*input_height=*/1, coinbaseKey, p2wsh_drop_true, 49 * COIN, /*submit=*/false);
    CreateAndProcessBlock({funding_mtx}, CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG);

    // Two versions of the parent: OP_DROP accepts any dummy, and the larger one is below the minimum
    // relay feerate on its own while the smaller one is not.
    auto make_parent = [&](size_t dummy_size, CAmount fee) {
        CMutableTransaction mtx;
        mtx.vin.emplace_back(funding_mtx.GetHash(), 0);
        mtx.vin[0].scriptWitness.stack = {std::vector<unsigned char>(dummy_size, 1), drop_true_bytes};
        mtx.vout.emplace_back(49 * COIN - fee, p2wsh_drop_true);
        return MakeTransactionRef(mtx);
    };
    const CFeeRate min_relay_feerate{m_node.mempool->m_opts.min_relay_feerate};
    const CAmount parent_fee{min_relay_feerate.GetFee(GetVirtualTransactionSize(*make_parent(0, 0)))};
    const auto parent{make_parent(MAX_STANDARD_P2WSH_STACK_ITEM_SIZE, parent_fee)};
    const auto parent_other{make_parent(0, parent_fee)};
    BOOST_REQUIRE(min_relay_feerate.GetFee(GetVirtualTransactionSize(*parent)) > parent_fee);
    BOOST_REQUIRE(parent->GetHash() == parent_other->GetHash());

    // The honest child pays for the pair. The fake child also spends an unknown outpoint, so it stays
    // in the orphanage.
    CMutableTransaction child_mtx;
    child_mtx.vin.emplace_back(parent->GetHash(), 0);
    child_mtx.vin[0].scriptWitness.stack = {{}, drop_true_bytes};
    child_mtx.vout.emplace_back(49 * COIN - parent_fee - 10000, p2wsh_drop_true);
    const auto child{MakeTransactionRef(child_mtx)};
    CMutableTransaction fake_child_mtx{child_mtx};
    fake_child_mtx.vin.emplace_back(Txid::FromUint256(uint256::ONE), 0);
    fake_child_mtx.vin[1].scriptWitness.stack = {{}, drop_true_bytes};
    const auto fake_child{MakeTransactionRef(fake_child_mtx)};

    ConnmanTestMsg connman{0x1337, 0x1337, *m_node.addrman, *m_node.netgroupman, Params()};
    auto peerman{PeerManager::make(connman, *m_node.addrman, /*banman=*/nullptr, *m_node.chainman, *m_node.mempool, *m_node.warnings,
                                   PeerManager::Options{.deterministic_rng = true})};
    CConnman::Options connman_options;
    connman_options.m_msgproc = peerman.get();
    connman.Init(connman_options);

    LOCK(NetEventsInterface::g_msgproc_mutex);
    constexpr ServiceFlags services{NODE_NETWORK | NODE_WITNESS};
    std::vector<CNode*> peers;
    for (NodeId id : {0, 1}) {
        in_addr address{};
        address.s_addr = htonl(0x0a000001U + static_cast<uint32_t>(id));
        peers.push_back(new CNode{id, /*sock=*/nullptr, CAddress{CService{address, 8333}, NODE_NETWORK},
                                  /*nKeyedNetGroupIn=*/static_cast<uint64_t>(id), /*nLocalHostNonceIn=*/0, CAddress{},
                                  /*addrNameIn=*/"", ConnectionType::INBOUND, /*inbound_onion=*/false, /*network_key=*/static_cast<uint64_t>(id + 1)});
        connman.AddTestNode(*peers.back());
        connman.Handshake(*peers.back(), /*successfully_connected=*/true, services, services, PROTOCOL_VERSION, /*relay_txs=*/true);
    }
    CNode& attacker{*peers[0]};
    CNode& honest{*peers[1]};
    auto deliver = [&](CNode& peer, const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex) {
        connman.FlushSendBuffer(peer);
        BOOST_REQUIRE(connman.ReceiveMsgFrom(peer, NetMsg::Make(NetMsgType::TX, TX_WITH_WITNESS(*tx))));
        peer.fPauseSend = false;
        while (connman.ProcessMessagesOnce(peer)) {}
    };
    auto orphaned = [&](const CTransactionRef& tx) {
        return std::ranges::any_of(peerman->GetOrphanTransactions(), [&](const auto& o) { return o.tx->GetWitnessHash() == tx->GetWitnessHash(); });
    };

    deliver(attacker, parent);
    BOOST_REQUIRE(!m_node.mempool->exists(parent->GetHash()));
    deliver(attacker, fake_child);
    BOOST_REQUIRE(orphaned(fake_child));
    deliver(attacker, parent_other);
    BOOST_REQUIRE(m_node.mempool->exists(parent->GetHash()));
    // Evaluated with the fake child: the parent's result is DIFFERENT_WITNESS.
    deliver(attacker, parent);
    BOOST_REQUIRE(orphaned(fake_child));
    // The other version leaves the mempool (evicted or replaced), without a tip change.
    WITH_LOCK(m_node.mempool->cs, m_node.mempool->removeRecursive(*parent_other, MemPoolRemovalReason::SIZELIMIT));

    deliver(honest, child);
    BOOST_REQUIRE(orphaned(child));
    deliver(honest, parent);
    BOOST_CHECK(m_node.mempool->exists(child->GetHash()));

    for (CNode* peer : peers) peerman->FinalizeNode(*peer);
    connman.ClearTestNodes();
    connman.SetMsgProc(nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
