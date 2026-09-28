// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <consensus/validation.h>
#include <kernel/mempool_removal_reason.h>
#include <net_processing.h>
#include <node/txdownloadman_impl.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/common.h>
#include <test/util/random.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
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

/** All parent txids of tx, sorted and unique: what validation reports when every input is missing. */
static std::vector<Txid> AllParents(const CTransaction& tx)
{
    std::vector<Txid> parents;
    parents.reserve(tx.vin.size());
    for (const auto& input : tx.vin) parents.push_back(input.prevout.hash);
    std::sort(parents.begin(), parents.end());
    parents.erase(std::unique(parents.begin(), parents.end()), parents.end());
    return parents;
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
                const auto& [keep, unique_txids, package_to_validate] = txdownload_impl.MempoolRejectedTx(ptx_parent, state, nodeid, /*first_time_failure=*/true, AllParents(*ptx_parent));

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
                txdownload_impl.MempoolRejectedTx(ptx_child, state, nodeid, /*first_time_failure=*/true, AllParents(*ptx_child));

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
        const auto ret_1p1c = txdownload_impl.MempoolRejectedTx(orphan, state_orphan, nodeid, /*first_time_failure=*/true, AllParents(*orphan));
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
            const auto ret_1p1c_parent_reconsiderable = txdownload_impl.MempoolRejectedTx(orphan, state_orphan, nodeid, /*first_time_failure=*/true, AllParents(*orphan));
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

            const auto ret_1recon_conf = txdownload_impl.MempoolRejectedTx(orphan, state_orphan, nodeid, /*first_time_failure=*/true, AllParents(*orphan));
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

            const auto ret_2_problems = txdownload_impl.MempoolRejectedTx(orphan, state_orphan, nodeid, /*first_time_failure=*/true, AllParents(*orphan));
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
            const auto ret_1p1c_2reconsiderable = txdownload_impl.MempoolRejectedTx(orphan, state_orphan, nodeid, /*first_time_failure=*/true, AllParents(*orphan));
            std::string err_msg;
            const bool ok = CheckOrphanBehavior(txdownload_impl, orphan, ret_1p1c_2reconsiderable, err_msg,
                                                /*expect_orphan=*/true, /*expect_keep=*/true, /*expected_parents=*/1);
            BOOST_CHECK_MESSAGE(ok, err_msg);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(orphan_parent_request_survives_reject_from_other_peer, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    node::TxDownloadOptions DEFAULT_OPTS{.m_mempool = pool, .m_deterministic_txrequest = true};
    constexpr NodeId honest{1}, other{2};
    const std::chrono::microseconds now{GetTime<std::chrono::microseconds>()};
    TxValidationState state_orphan, state_reconsiderable;
    state_orphan.Invalid(TxValidationResult::TX_MISSING_INPUTS, "");
    state_reconsiderable.Invalid(TxValidationResult::TX_RECONSIDERABLE, "");

    // A parent without witness data has wtxid == txid: either a nonsegwit tx or a segwit tx whose
    // witness was stripped by the sender (indistinguishable here). Rejecting it must not cancel the
    // orphan resolution requests (by txid) that other peers are candidates for. The witness-bearing
    // parent is the control: its rejection never touched those requests.
    for (const auto segwit_parent : {false, true}) {
        node::TxDownloadManagerImpl txdownload_impl{DEFAULT_OPTS};
        txdownload_impl.ConnectedPeer(honest, {/*m_preferred=*/true, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true});
        txdownload_impl.ConnectedPeer(other, {/*m_preferred=*/false, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true});

        const auto parent = CreatePlaceholderTx(segwit_parent);
        const auto child = CreatePlaceholderTx(/*segwit=*/true);
        BOOST_REQUIRE(child->vin[0].prevout.hash == parent->GetHash());

        // Honest peer delivers the child. It is an orphan; the parent will be requested by txid.
        BOOST_REQUIRE(txdownload_impl.ReceivedTx(honest, child).first);
        txdownload_impl.MempoolRejectedTx(child, state_orphan, honest, /*first_time_failure=*/true, AllParents(*child));
        BOOST_REQUIRE(txdownload_impl.m_orphanage->HaveTxFromPeer(child->GetWitnessHash(), honest));

        // Before that request is sent, another peer (not an announcer of the child) delivers the
        // parent unsolicited and it fails for low feerate.
        BOOST_REQUIRE(txdownload_impl.ReceivedTx(other, parent).first);
        const auto ret = txdownload_impl.MempoolRejectedTx(parent, state_reconsiderable, other, /*first_time_failure=*/true, AllParents(*parent));
        BOOST_CHECK(!ret.m_package_to_validate.has_value());
        BOOST_CHECK(txdownload_impl.m_orphanage->HaveTxFromPeer(child->GetWitnessHash(), honest));

        // The honest peer must still be asked for the parent: its version may carry a witness that
        // makes the package acceptable.
        const auto requests = txdownload_impl.GetRequestsToSend(honest, now + std::chrono::seconds{10});
        BOOST_REQUIRE_EQUAL(requests.size(), 1);
        BOOST_CHECK(!requests[0].IsWtxid());
        BOOST_CHECK(requests[0].ToUint256() == parent->GetHash().ToUint256());
    }
}

BOOST_FIXTURE_TEST_CASE(missing_parents_only_actually_missing, TestChain100Setup)
{
    // A confirmed transaction FUND with an unspent output in the coins cache, and a confirmed coin for
    // the parent. Neither block is fed to the download manager, so FUND is not in its
    // recently-confirmed filter (as any transaction confirmed more than a few blocks ago).
    const auto fund_input_mtx = CreateValidMempoolTransaction(m_coinbase_txns[0], /*input_vout=*/0, /*input_height=*/1, coinbaseKey, P2WSH_OP_TRUE, 49 * COIN, /*submit=*/false);
    CreateAndProcessBlock({fund_input_mtx}, CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG);
    // The second coinbase is mature from height 102.
    const auto parent_input_mtx = CreateValidMempoolTransaction(m_coinbase_txns[1], /*input_vout=*/0, /*input_height=*/2, coinbaseKey, P2WSH_OP_TRUE, 49 * COIN, /*submit=*/false);
    CMutableTransaction fund_mtx;
    fund_mtx.vin.emplace_back(fund_input_mtx.GetHash(), 0);
    fund_mtx.vin[0].scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    fund_mtx.vout.emplace_back(24 * COIN, P2WSH_OP_TRUE);
    fund_mtx.vout.emplace_back(24 * COIN, P2WSH_OP_TRUE);
    const auto fund = MakeTransactionRef(fund_mtx);
    const auto fund_block = CreateAndProcessBlock({fund_mtx, parent_input_mtx}, CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG);
    BOOST_REQUIRE(WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()->GetBlockHash()) == fund_block.GetHash());

    // Zero-fee parent, and a child paying for the pair from FUND's second output.
    CMutableTransaction parent_mtx;
    parent_mtx.vin.emplace_back(parent_input_mtx.GetHash(), 0);
    parent_mtx.vin[0].scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    parent_mtx.vout.emplace_back(49 * COIN, P2WSH_OP_TRUE);
    const auto parent = MakeTransactionRef(parent_mtx);
    CMutableTransaction child_mtx;
    child_mtx.vin.emplace_back(parent->GetHash(), 0);
    child_mtx.vin.emplace_back(fund->GetHash(), 1);
    for (auto& input : child_mtx.vin) input.scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    child_mtx.vout.emplace_back(49 * COIN + 24 * COIN - 10000, P2WSH_OP_TRUE);
    const auto child = MakeTransactionRef(child_mtx);

    // A witness-stripped copy of FUND: same txid, and wtxid == txid.
    CMutableTransaction fund_stripped_mtx{fund_mtx};
    fund_stripped_mtx.vin[0].scriptWitness.SetNull();
    const auto fund_stripped = MakeTransactionRef(fund_stripped_mtx);
    BOOST_REQUIRE(fund_stripped->GetWitnessHash().ToUint256() == fund->GetHash().ToUint256());

    LOCK(cs_main);
    CTxMemPool& pool = *Assert(m_node.mempool);
    node::TxDownloadManagerImpl txdownload_impl{node::TxDownloadOptions{.m_mempool = pool, .m_deterministic_txrequest = true}};
    constexpr NodeId honest{1}, other{2};
    txdownload_impl.ConnectedPeer(honest, {/*m_preferred=*/true, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true});
    txdownload_impl.ConnectedPeer(other, {/*m_preferred=*/false, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true});

    // Validation reports only the parent as missing: FUND's output is in the UTXO set.
    const auto child_result = m_node.chainman->ProcessTransaction(child);
    BOOST_REQUIRE(child_result.m_state.GetResult() == TxValidationResult::TX_MISSING_INPUTS);
    BOOST_REQUIRE(child_result.m_missing_parents.has_value());
    BOOST_CHECK(*child_result.m_missing_parents == std::vector<Txid>{parent->GetHash()});

    // One of FUND's outputs is in the coins cache, as after the block creating it was connected or after
    // any transaction spending it was validated. Another peer sends the stripped copy of FUND: its inputs
    // are spent and an output is cached, so it is rejected as already known before its missing witness
    // could be noticed, and its wtxid (== txid) lands in the reject filter.
    auto& coins_tip = m_node.chainman->ActiveChainstate().CoinsTip();
    coins_tip.AccessCoin(COutPoint{fund->GetHash(), 0});
    BOOST_REQUIRE(coins_tip.HaveCoinInCache(COutPoint{fund->GetHash(), 0}));
    BOOST_REQUIRE(txdownload_impl.ReceivedTx(other, fund_stripped).first);
    const auto fund_result = m_node.chainman->ProcessTransaction(fund_stripped);
    BOOST_REQUIRE_MESSAGE(fund_result.m_state.GetResult() == TxValidationResult::TX_CONFLICT, fund_result.m_state.ToString());
    txdownload_impl.MempoolRejectedTx(fund_stripped, fund_result.m_state, other, /*first_time_failure=*/true, AllParents(*fund_stripped));
    // Whether or not that rejection put FUND's txid in the reject filter, a stripped copy of a confirmed
    // transaction failing a check before the known-transaction check (a coinbase, or one no longer
    // meeting current standardness) would have: make sure it is there, to show it does not matter.
    txdownload_impl.RecentRejectsFilter().insert(fund->GetHash().ToUint256());
    BOOST_REQUIRE(txdownload_impl.RecentRejectsFilter().contains(fund->GetHash().ToUint256()));

    // The honest child arrives. FUND's txid being in the reject filter is irrelevant: FUND is not a
    // missing parent. The child is stored, only the parent is requested, and the child is not rejected.
    BOOST_REQUIRE(txdownload_impl.ReceivedTx(honest, child).first);
    const auto todo = txdownload_impl.MempoolRejectedTx(child, child_result.m_state, honest, /*first_time_failure=*/true, *child_result.m_missing_parents);
    BOOST_CHECK(txdownload_impl.m_orphanage->HaveTxFromPeer(child->GetWitnessHash(), honest));
    BOOST_CHECK(!txdownload_impl.RecentRejectsFilter().contains(child->GetHash().ToUint256()));
    BOOST_CHECK(todo.m_unique_parents == std::vector<Txid>{parent->GetHash()});

    // The honest parent then finds its child, and the pair is acceptable.
    BOOST_REQUIRE(txdownload_impl.ReceivedTx(honest, parent).first);
    const auto parent_result = m_node.chainman->ProcessTransaction(parent);
    BOOST_REQUIRE(parent_result.m_state.GetResult() == TxValidationResult::TX_RECONSIDERABLE);
    const auto parent_todo = txdownload_impl.MempoolRejectedTx(parent, parent_result.m_state, honest, /*first_time_failure=*/true, AllParents(*parent));
    BOOST_CHECK(parent_todo.m_package_to_validate.has_value());
    const auto package_result = ProcessNewPackage(m_node.chainman->ActiveChainstate(), pool, {parent, child}, /*test_accept=*/false, std::nullopt);
    BOOST_CHECK(package_result.m_state.IsValid());
}

BOOST_FIXTURE_TEST_CASE(rejected_parent_check_ignores_present_parents, TestChain100Setup)
{
    // Whatever put a present parent's txid into the reject filter (any hard rejection of a witnessless
    // copy of it: already-known, coinbase, currently nonstandard, ...), a child is only judged by the
    // parents that are actually missing.
    CTxMemPool& pool = *Assert(m_node.mempool);
    node::TxDownloadManagerImpl txdownload_impl{node::TxDownloadOptions{.m_mempool = pool, .m_deterministic_txrequest = true}};
    constexpr NodeId honest{1}, other{2};
    txdownload_impl.ConnectedPeer(honest, {/*m_preferred=*/true, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true});
    txdownload_impl.ConnectedPeer(other, {/*m_preferred=*/false, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true});
    TxValidationState state_orphan, state_consensus;
    state_orphan.Invalid(TxValidationResult::TX_MISSING_INPUTS, "");
    state_consensus.Invalid(TxValidationResult::TX_CONSENSUS, "");

    const auto present_parent = CreatePlaceholderTx(/*segwit=*/false); // witnessless: wtxid == txid
    const auto missing_parent = CreatePlaceholderTx(/*segwit=*/true);
    CMutableTransaction child_mtx;
    child_mtx.vin.emplace_back(missing_parent->GetHash(), 0);
    child_mtx.vin.emplace_back(present_parent->GetHash(), 0);
    child_mtx.vin[0].scriptWitness.stack.push_back({1});
    child_mtx.vout.emplace_back(CENT, CScript());
    const auto child = MakeTransactionRef(child_mtx);

    // The present parent's txid is hard-rejected.
    txdownload_impl.MempoolRejectedTx(present_parent, state_consensus, other, /*first_time_failure=*/true, AllParents(*present_parent));
    BOOST_REQUIRE(txdownload_impl.RecentRejectsFilter().contains(present_parent->GetHash().ToUint256()));

    // Without knowing which parents are missing, the child is treated as having a rejected parent...
    {
        node::TxDownloadManagerImpl control{node::TxDownloadOptions{.m_mempool = pool, .m_deterministic_txrequest = true}};
        control.ConnectedPeer(honest, {/*m_preferred=*/true, /*m_relay_permissions=*/false, /*m_wtxid_relay=*/true});
        control.MempoolRejectedTx(present_parent, state_consensus, honest, /*first_time_failure=*/true, AllParents(*present_parent));
        control.MempoolRejectedTx(child, state_orphan, honest, /*first_time_failure=*/true, AllParents(*child));
        BOOST_CHECK(!control.m_orphanage->HaveTx(child->GetWitnessHash()));
        BOOST_CHECK(control.RecentRejectsFilter().contains(child->GetHash().ToUint256()));
    }
    // ...but with validation's set of actually missing parents, it is stored and only they are requested.
    const auto todo = txdownload_impl.MempoolRejectedTx(child, state_orphan, honest, /*first_time_failure=*/true, std::vector<Txid>{missing_parent->GetHash()});
    BOOST_CHECK(txdownload_impl.m_orphanage->HaveTxFromPeer(child->GetWitnessHash(), honest));
    BOOST_CHECK(!txdownload_impl.RecentRejectsFilter().contains(child->GetHash().ToUint256()));
    BOOST_CHECK(todo.m_unique_parents == std::vector<Txid>{missing_parent->GetHash()});
}

BOOST_FIXTURE_TEST_CASE(missing_parents_reports_all_missing_once, TestChain100Setup)
{
    // Confirmed FUND with two outputs; two zero-fee parents spending confirmed coins.
    const auto fund_input_mtx = CreateValidMempoolTransaction(m_coinbase_txns[0], /*input_vout=*/0, /*input_height=*/1, coinbaseKey, P2WSH_OP_TRUE, 49 * COIN, /*submit=*/false);
    CreateAndProcessBlock({fund_input_mtx}, CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG);
    const auto p1_input_mtx = CreateValidMempoolTransaction(m_coinbase_txns[1], /*input_vout=*/0, /*input_height=*/2, coinbaseKey, P2WSH_OP_TRUE, 49 * COIN, /*submit=*/false);
    CMutableTransaction fund_mtx;
    fund_mtx.vin.emplace_back(fund_input_mtx.GetHash(), 0);
    fund_mtx.vin[0].scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    fund_mtx.vout.emplace_back(24 * COIN, P2WSH_OP_TRUE);
    fund_mtx.vout.emplace_back(24 * COIN, P2WSH_OP_TRUE);
    const auto fund = MakeTransactionRef(fund_mtx);
    const auto block = CreateAndProcessBlock({fund_mtx, p1_input_mtx}, CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG);
    BOOST_REQUIRE(WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()->GetBlockHash()) == block.GetHash());

    auto spend = [](const Txid& txid, uint32_t n, std::vector<CAmount> outs) {
        CMutableTransaction mtx;
        mtx.vin.emplace_back(txid, n);
        mtx.vin[0].scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
        for (const auto out : outs) mtx.vout.emplace_back(out, P2WSH_OP_TRUE);
        return MakeTransactionRef(mtx);
    };
    const auto p1 = spend(p1_input_mtx.GetHash(), 0, {24 * COIN, 24 * COIN}); // missing, two outputs
    const auto p2 = spend(fund->GetHash(), 0, {24 * COIN});                    // missing
    // Child spends two outputs of the missing p1 (parent reported once), the missing p2, and the
    // present FUND:1.
    CMutableTransaction child_mtx;
    child_mtx.vin.emplace_back(p1->GetHash(), 0);
    child_mtx.vin.emplace_back(fund->GetHash(), 1);
    child_mtx.vin.emplace_back(p2->GetHash(), 0);
    child_mtx.vin.emplace_back(p1->GetHash(), 1);
    for (auto& in : child_mtx.vin) in.scriptWitness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);
    child_mtx.vout.emplace_back(10 * COIN, P2WSH_OP_TRUE);
    const auto child = MakeTransactionRef(child_mtx);

    LOCK(cs_main);
    const auto result = m_node.chainman->ProcessTransaction(child);
    BOOST_REQUIRE(result.m_state.GetResult() == TxValidationResult::TX_MISSING_INPUTS);
    BOOST_REQUIRE(result.m_missing_parents.has_value());
    std::vector<Txid> expected{p1->GetHash(), p2->GetHash()};
    std::sort(expected.begin(), expected.end());
    BOOST_CHECK(*result.m_missing_parents == expected);
}

BOOST_AUTO_TEST_SUITE_END()
