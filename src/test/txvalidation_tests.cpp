// Copyright (c) 2017-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <key_io.h>
#include <policy/v3_policy.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(txvalidation_tests)

/**
 * Ensure that the mempool won't accept coinbase transactions.
 */
BOOST_FIXTURE_TEST_CASE(tx_mempool_reject_coinbase, TestChain100Setup)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CMutableTransaction coinbaseTx;

    coinbaseTx.nVersion = 1;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vout.resize(1);
    coinbaseTx.vin[0].scriptSig = CScript() << OP_11 << OP_EQUAL;
    coinbaseTx.vout[0].nValue = 1 * CENT;
    coinbaseTx.vout[0].scriptPubKey = scriptPubKey;

    BOOST_CHECK(CTransaction(coinbaseTx).IsCoinBase());

    LOCK(cs_main);

    unsigned int initialPoolSize = m_node.mempool->size();
    const MempoolAcceptResult result = m_node.chainman->ProcessTransaction(MakeTransactionRef(coinbaseTx));

    BOOST_CHECK(result.m_result_type == MempoolAcceptResult::ResultType::INVALID);

    // Check that the transaction hasn't been added to mempool.
    BOOST_CHECK_EQUAL(m_node.mempool->size(), initialPoolSize);

    // Check that the validation state reflects the unsuccessful attempt.
    BOOST_CHECK(result.m_state.IsInvalid());
    BOOST_CHECK_EQUAL(result.m_state.GetRejectReason(), "coinbase");
    BOOST_CHECK(result.m_state.GetResult() == TxValidationResult::TX_CONSENSUS);
}

// Generate a number of random, nonexistent outpoints.
static inline std::vector<COutPoint> random_outpoints(size_t num_outpoints) {
    std::vector<COutPoint> outpoints;
    for (size_t i{0}; i < num_outpoints; ++i) {
        outpoints.emplace_back(Txid::FromUint256(GetRandHash()), 0);
    }
    return outpoints;
}

static inline std::vector<CPubKey> random_keys(size_t num_keys) {
    std::vector<CPubKey> keys;
    keys.reserve(num_keys);
    for (size_t i{0}; i < num_keys; ++i) {
        CKey key;
        key.MakeNewKey(true);
        keys.emplace_back(key.GetPubKey());
    }
    return keys;
}

// Creates a placeholder tx (not valid) with 25 outputs. Specify the nVersion and the inputs.
static inline CTransactionRef make_tx(const std::vector<COutPoint>& inputs, int32_t version)
{
    CMutableTransaction mtx = CMutableTransaction{};
    mtx.nVersion = version;
    mtx.vin.resize(inputs.size());
    mtx.vout.resize(25);
    for (size_t i{0}; i < inputs.size(); ++i) {
        mtx.vin[i].prevout = inputs[i];
    }
    for (auto i{0}; i < 25; ++i) {
        mtx.vout[i].scriptPubKey = CScript() << OP_TRUE;
        mtx.vout[i].nValue = 10000;
    }
    return MakeTransactionRef(mtx);
}

BOOST_FIXTURE_TEST_CASE(version3_tests, RegTestingSetup)
{
    // Test V3 policy helper functions
    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;
    std::set<Txid> empty_conflicts_set;

    auto mempool_tx_v3 = make_tx(random_outpoints(1), /*version=*/3);
    pool.addUnchecked(entry.FromTx(mempool_tx_v3));
    auto mempool_tx_v2 = make_tx(random_outpoints(1), /*version=*/2);
    pool.addUnchecked(entry.FromTx(mempool_tx_v2));
    // These two transactions are unrelated, so CheckV3Inheritance should pass.
    BOOST_CHECK(CheckV3Inheritance({mempool_tx_v2, mempool_tx_v3}) == std::nullopt);
    // Default values.
    CTxMemPool::Limits m_limits{};

    // Cannot spend from an unconfirmed v3 transaction unless this tx is also v3.
    {
        auto tx_v2_from_v3 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 0}}, /*version=*/2);
        auto ancestors_v2_from_v3{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v2_from_v3), m_limits)};
        BOOST_CHECK(CheckV3Inheritance(tx_v2_from_v3, *ancestors_v2_from_v3).has_value());
        BOOST_CHECK(CheckV3Inheritance({mempool_tx_v3, tx_v2_from_v3}).has_value());
        auto tx_v2_from_v2_and_v3 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 0}, COutPoint{mempool_tx_v2->GetHash(), 0}}, /*version=*/2);
        auto ancestors_v2_from_both{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v2_from_v2_and_v3), m_limits)};
        BOOST_CHECK(CheckV3Inheritance(tx_v2_from_v2_and_v3, *ancestors_v2_from_both).has_value());
        BOOST_CHECK(CheckV3Inheritance({mempool_tx_v2, mempool_tx_v3, tx_v2_from_v2_and_v3}).value() ==
                    std::make_tuple(mempool_tx_v3->GetWitnessHash(), tx_v2_from_v2_and_v3->GetWitnessHash(), false));
    }

    // V3 cannot spend from an unconfirmed non-v3 transaction.
    {
        auto tx_v3_from_v2 = make_tx({COutPoint{mempool_tx_v2->GetHash(), 0}}, /*version=*/3);
        auto ancestors_v3_from_v2{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_from_v2), m_limits)};
        BOOST_CHECK(CheckV3Inheritance(tx_v3_from_v2, *ancestors_v3_from_v2).has_value());
        BOOST_CHECK(CheckV3Inheritance({mempool_tx_v2, tx_v3_from_v2}).value() ==
                    std::make_tuple(mempool_tx_v2->GetWitnessHash(), tx_v3_from_v2->GetWitnessHash(), true));
        auto tx_v3_from_v2_and_v3 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 0}, COutPoint{mempool_tx_v2->GetHash(), 0}}, /*version=*/3);
        auto ancestors_v3_from_both{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_from_v2_and_v3), m_limits)};
        BOOST_CHECK(CheckV3Inheritance(tx_v3_from_v2_and_v3, *ancestors_v3_from_both).has_value());
        BOOST_CHECK(CheckV3Inheritance({mempool_tx_v2, mempool_tx_v3, tx_v3_from_v2_and_v3}).value() ==
                    std::make_tuple(mempool_tx_v2->GetWitnessHash(), tx_v3_from_v2_and_v3->GetWitnessHash(), true));
    }
    // V3 from V3 is ok, and non-V3 from non-V3 is ok.
    {
        auto tx_v3_from_v3 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 0}}, /*version=*/3);
        auto ancestors_v3{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_from_v3), m_limits)};
        BOOST_CHECK(CheckV3Inheritance({tx_v3_from_v3, mempool_tx_v3}) == std::nullopt);
        BOOST_CHECK(CheckV3Inheritance({mempool_tx_v3, tx_v3_from_v3}) == std::nullopt);
        BOOST_CHECK(CheckV3Inheritance(tx_v3_from_v3, *ancestors_v3) == std::nullopt);

        auto tx_v2_from_v2 = make_tx({COutPoint{mempool_tx_v2->GetHash(), 0}}, /*version=*/2);
        auto ancestors_v2{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v2_from_v2), m_limits)};
        BOOST_CHECK(CheckV3Inheritance({tx_v2_from_v2, mempool_tx_v2}) == std::nullopt);
        BOOST_CHECK(CheckV3Inheritance({mempool_tx_v2, tx_v2_from_v2}) == std::nullopt);
        BOOST_CHECK(CheckV3Inheritance(tx_v2_from_v2, *ancestors_v2) == std::nullopt);
    }

    // Tx spending v3 cannot have too many mempool ancestors
    // Configuration where the tx has multiple direct parents.
    {
        std::vector<COutPoint> mempool_outpoints;
        mempool_outpoints.emplace_back(mempool_tx_v3->GetHash(), 0);
        mempool_outpoints.resize(2);
        for (size_t i{0}; i < 2; ++i) {
            auto mempool_tx = make_tx(random_outpoints(1), /*version=*/2);
            pool.addUnchecked(entry.FromTx(mempool_tx));
            mempool_outpoints.emplace_back(mempool_tx->GetHash(), 0);
        }
        auto tx_v3_multi_parent = make_tx(mempool_outpoints, /*version=*/3);
        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_multi_parent), m_limits)};
        BOOST_CHECK(ApplyV3Rules(tx_v3_multi_parent, *ancestors, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_multi_parent)).has_value());
    }

    // Configuration where the tx is in a multi-generation chain.
    auto last_outpoint{random_outpoints(1)[0]};
    for (size_t i{0}; i < 2; ++i) {
        auto mempool_tx = make_tx({last_outpoint}, /*version=*/2);
        pool.addUnchecked(entry.FromTx(mempool_tx));
        last_outpoint = COutPoint{mempool_tx->GetHash(), 0};
    }
    {
        auto tx_v3_multi_gen = make_tx({last_outpoint}, /*version=*/3);
        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_multi_gen), m_limits)};
        BOOST_CHECK(ApplyV3Rules(tx_v3_multi_gen, *ancestors, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_multi_gen)).has_value());
    }

    // Tx spending v3 cannot be too large in weight.
    auto many_inputs{random_outpoints(100)};
    many_inputs.emplace_back(mempool_tx_v3->GetHash(), 0);
    {
        auto tx_v3_child_big = make_tx(many_inputs, /*version=*/3);
        BOOST_CHECK(GetTransactionWeight(*tx_v3_child_big) > V3_CHILD_MAX_VSIZE * WITNESS_SCALE_FACTOR);
        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_child_big), m_limits)};
        BOOST_CHECK(ApplyV3Rules(tx_v3_child_big, *ancestors, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_child_big)).has_value());
    }

    // Tx spending v3 cannot have too many sigops.
    auto multisig_outpoints{random_outpoints(3)};
    multisig_outpoints.emplace_back(mempool_tx_v3->GetHash(), 0);
    auto keys{random_keys(2)};
    CScript script_multisig;
    script_multisig << OP_1;
    for (const auto& key : keys) {
        script_multisig << ToByteVector(key);
    }
    script_multisig << OP_2 << OP_CHECKMULTISIG;
    {
        CMutableTransaction mtx_many_sigops = CMutableTransaction{};
        mtx_many_sigops.nVersion = 3;
        for (const auto& outpoint : multisig_outpoints) {
            mtx_many_sigops.vin.emplace_back(outpoint, script_multisig);
        }
        mtx_many_sigops.vout.resize(1);
        mtx_many_sigops.vout.back().scriptPubKey = CScript() << OP_TRUE;
        mtx_many_sigops.vout.back().nValue = 10000;
        auto tx_many_sigops{MakeTransactionRef(mtx_many_sigops)};

        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_many_sigops), m_limits)};
        // legacy uses fAccurate = false, and the maximum number of multisig keys is used
        const int64_t total_sigops{static_cast<int64_t>(tx_many_sigops->vin.size()) * static_cast<int64_t>(script_multisig.GetSigOpCount(/*fAccurate=*/false))};
        BOOST_CHECK_EQUAL(total_sigops, tx_many_sigops->vin.size() * MAX_PUBKEYS_PER_MULTISIG);
        const int64_t bip141_vsize{GetVirtualTransactionSize(*tx_many_sigops)};
        // Weight limit is not reached...
        BOOST_CHECK(ApplyV3Rules(tx_many_sigops, *ancestors, empty_conflicts_set, bip141_vsize) == std::nullopt);
        // ...but sigop limit is.
        BOOST_CHECK(ApplyV3Rules(tx_many_sigops, *ancestors, empty_conflicts_set, std::max(total_sigops * DEFAULT_BYTES_PER_SIGOP, bip141_vsize)).has_value());
    }

    // Parent + child with v3 in the mempool. Child is allowed as long as it is under V3_CHILD_MAX_VSIZE.
    auto tx_mempool_v3_child = make_tx({COutPoint{mempool_tx_v3->GetHash(), 0}}, /*version=*/3);
    {
        BOOST_CHECK(GetTransactionWeight(*tx_mempool_v3_child) <= V3_CHILD_MAX_VSIZE * WITNESS_SCALE_FACTOR);
        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_mempool_v3_child), m_limits)};
        BOOST_CHECK(ApplyV3Rules(tx_mempool_v3_child, *ancestors, empty_conflicts_set, GetVirtualTransactionSize(*tx_mempool_v3_child)) == std::nullopt);
        pool.addUnchecked(entry.FromTx(tx_mempool_v3_child));
    }

    // A v3 transaction cannot have more than 1 descendant.
    {
        auto tx_v3_child2 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 1}}, /*version=*/3);
        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_child2), m_limits)};
        BOOST_CHECK(ApplyV3Rules(tx_v3_child2, *ancestors, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_child2)).has_value());
        // If replacing the child, make sure there is no double-counting.
        BOOST_CHECK(ApplyV3Rules(tx_v3_child2, *ancestors, {tx_mempool_v3_child->GetHash()}, GetVirtualTransactionSize(*tx_v3_child2)) == std::nullopt);
    }

    {
        auto tx_v3_grandchild = make_tx({COutPoint{tx_mempool_v3_child->GetHash(), 0}}, /*version=*/3);
        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_grandchild), m_limits)};
        BOOST_CHECK(ApplyV3Rules(tx_v3_grandchild, *ancestors, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_grandchild)).has_value());
    }
}

BOOST_AUTO_TEST_SUITE_END()
