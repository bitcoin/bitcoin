// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_processing.h>
#include <node/txdownload_impl.h>
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

    void CheckEqual(const Behaviors& other)
    {
        BOOST_CHECK_EQUAL(other.m_txid_in_rejects, m_txid_in_rejects);
        BOOST_CHECK_EQUAL(other.m_wtxid_in_rejects, m_wtxid_in_rejects);
        BOOST_CHECK_EQUAL(other.m_txid_in_rejects_recon, m_txid_in_rejects_recon);
        BOOST_CHECK_EQUAL(other.m_wtxid_in_rejects_recon, m_wtxid_in_rejects_recon);
        BOOST_CHECK_EQUAL(other.m_keep_for_compact, m_keep_for_compact);
        BOOST_CHECK_EQUAL(other.m_ignore_inv_txid, m_ignore_inv_txid);
        BOOST_CHECK_EQUAL(other.m_ignore_inv_wtxid, m_ignore_inv_wtxid);
    }
};
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

static std::vector<CTransactionRef> CreateTransactions(size_t num_txns)
{
    // Ensure every transaction has a different txid by having each one spend the previous one.
    static Txid prevout_hash{};

    std::vector<CTransactionRef> txns;
    txns.reserve(num_txns);
    // Simplest spk for every tx
    CScript spk = CScript() << OP_TRUE;
    for (uint32_t i = 0; i < num_txns; ++i) {
        CMutableTransaction tx;
        tx.vin.emplace_back(prevout_hash, 0);
        // Ensure txid != wtxid
        tx.vin[0].scriptWitness.stack.push_back({1});
        tx.vout.emplace_back(CENT, spk);
        auto ptx{MakeTransactionRef(tx)};
        txns.emplace_back(ptx);
        prevout_hash = ptx->GetHash();
    }
    return txns;
}
static CTransactionRef GetNewTransaction()
{
    // Create 2 transactions and return the second one. Transactions are created as a chain to
    // ensure uniqueness. Here, we want to avoid direct parent/child relationships between transactions.
    const auto two_transactions{CreateTransactions(/*num_txns=*/2)};
    return two_transactions.back();
}

BOOST_FIXTURE_TEST_CASE(tx_rejection_types, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    FastRandomContext det_rand{true};

    TxValidationState state;

    for (const auto& [result, expected_behavior] : expected_behaviors) {
        node::TxDownloadImpl txdownload_impl{node::TxDownloadOptions{pool, det_rand, DEFAULT_MAX_ORPHAN_TRANSACTIONS}};
        const auto ptx = GetNewTransaction();
        const auto& txid = ptx->GetHash();
        const auto& wtxid = ptx->GetWitnessHash();

        state.Invalid(result, "");
        NodeId nodeid{0};
        std::chrono::microseconds now{GetTime()};

        const auto& [keep, unique_txids, package_to_validate] = txdownload_impl.MempoolRejectedTx(ptx, state, 0, true);
        Behaviors actual_behavior{
            /*txid_rejects=*/txdownload_impl.m_recent_rejects.contains(txid.ToUint256()),
            /*wtxid_rejects=*/txdownload_impl.m_recent_rejects.contains(wtxid.ToUint256()),
            /*txid_recon=*/txdownload_impl.m_recent_rejects_reconsiderable.contains(txid.ToUint256()),
            /*wtxid_recon=*/txdownload_impl.m_recent_rejects_reconsiderable.contains(wtxid.ToUint256()),
            /*keep=*/keep,
            /*txid_inv=*/txdownload_impl.AddTxAnnouncement(nodeid, GenTxid::Txid(txid), now, /*p2p_inv=*/true),
            /*wtxid_inv=*/txdownload_impl.AddTxAnnouncement(nodeid, GenTxid::Wtxid(wtxid), now, /*p2p_inv=*/true),
        };
        BOOST_TEST_MESSAGE("Testing behavior for " << result);
        actual_behavior.CheckEqual(expected_behavior);
    }
}

BOOST_AUTO_TEST_SUITE_END()
