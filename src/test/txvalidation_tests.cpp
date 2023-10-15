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

    coinbaseTx.version = 1;
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

// Creates a placeholder tx (not valid) with 25 outputs. Specify the version and the inputs.
static inline CTransactionRef make_tx(const std::vector<COutPoint>& inputs, int32_t version)
{
    CMutableTransaction mtx = CMutableTransaction{};
    mtx.version = version;
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
    CTxMemPool::setEntries empty_ancestors;

    auto mempool_tx_v3 = make_tx(random_outpoints(1), /*version=*/3);
    pool.addUnchecked(entry.FromTx(mempool_tx_v3));
    auto mempool_tx_v2 = make_tx(random_outpoints(1), /*version=*/2);
    pool.addUnchecked(entry.FromTx(mempool_tx_v2));

    // Cannot spend from an unconfirmed v3 transaction unless this tx is also v3.
    {
        // mempool_tx_v3
        //      ^
        // tx_v2_from_v3
        auto tx_v2_from_v3 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 0}}, /*version=*/2);
        auto ancestors_v2_from_v3{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v2_from_v3))};
        const auto expected_error_str{strprintf("non-v3 tx %s (wtxid=%s) cannot spend from v3 tx %s (wtxid=%s)",
            tx_v2_from_v3->GetHash().ToString(), tx_v2_from_v3->GetWitnessHash().ToString(),
            mempool_tx_v3->GetHash().ToString(), mempool_tx_v3->GetWitnessHash().ToString())};
        auto result_v2_from_v3{SingleV3Checks(tx_v2_from_v3, *ancestors_v2_from_v3, empty_conflicts_set, GetVirtualTransactionSize(*tx_v2_from_v3))};
        BOOST_CHECK_EQUAL(result_v2_from_v3->first, expected_error_str);
        BOOST_CHECK_EQUAL(result_v2_from_v3->second, nullptr);

        Package package_v3_v2{mempool_tx_v3, tx_v2_from_v3};
        BOOST_CHECK_EQUAL(*PackageV3Checks(tx_v2_from_v3, GetVirtualTransactionSize(*tx_v2_from_v3), package_v3_v2, empty_ancestors), expected_error_str);
        CTxMemPool::setEntries entries_mempool_v3{pool.GetIter(mempool_tx_v3->GetHash().ToUint256()).value()};
        BOOST_CHECK_EQUAL(*PackageV3Checks(tx_v2_from_v3, GetVirtualTransactionSize(*tx_v2_from_v3), {tx_v2_from_v3}, entries_mempool_v3), expected_error_str);

        // mempool_tx_v3  mempool_tx_v2
        //            ^    ^
        //    tx_v2_from_v2_and_v3
        auto tx_v2_from_v2_and_v3 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 0}, COutPoint{mempool_tx_v2->GetHash(), 0}}, /*version=*/2);
        auto ancestors_v2_from_both{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v2_from_v2_and_v3))};
        const auto expected_error_str_2{strprintf("non-v3 tx %s (wtxid=%s) cannot spend from v3 tx %s (wtxid=%s)",
            tx_v2_from_v2_and_v3->GetHash().ToString(), tx_v2_from_v2_and_v3->GetWitnessHash().ToString(),
            mempool_tx_v3->GetHash().ToString(), mempool_tx_v3->GetWitnessHash().ToString())};
        auto result_v2_from_both{SingleV3Checks(tx_v2_from_v2_and_v3, *ancestors_v2_from_both, empty_conflicts_set, GetVirtualTransactionSize(*tx_v2_from_v2_and_v3))};
        BOOST_CHECK_EQUAL(result_v2_from_both->first, expected_error_str_2);
        BOOST_CHECK_EQUAL(result_v2_from_both->second, nullptr);

        Package package_v3_v2_v2{mempool_tx_v3, mempool_tx_v2, tx_v2_from_v2_and_v3};
        BOOST_CHECK_EQUAL(*PackageV3Checks(tx_v2_from_v2_and_v3, GetVirtualTransactionSize(*tx_v2_from_v2_and_v3), package_v3_v2_v2, empty_ancestors), expected_error_str_2);
    }

    // V3 cannot spend from an unconfirmed non-v3 transaction.
    {
        // mempool_tx_v2
        //      ^
        // tx_v3_from_v2
        auto tx_v3_from_v2 = make_tx({COutPoint{mempool_tx_v2->GetHash(), 0}}, /*version=*/3);
        auto ancestors_v3_from_v2{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_from_v2))};
        const auto expected_error_str{strprintf("v3 tx %s (wtxid=%s) cannot spend from non-v3 tx %s (wtxid=%s)",
            tx_v3_from_v2->GetHash().ToString(), tx_v3_from_v2->GetWitnessHash().ToString(),
            mempool_tx_v2->GetHash().ToString(), mempool_tx_v2->GetWitnessHash().ToString())};
        auto result_v3_from_v2{SingleV3Checks(tx_v3_from_v2, *ancestors_v3_from_v2,  empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_from_v2))};
        BOOST_CHECK_EQUAL(result_v3_from_v2->first, expected_error_str);
        BOOST_CHECK_EQUAL(result_v3_from_v2->second, nullptr);

        Package package_v2_v3{mempool_tx_v2, tx_v3_from_v2};
        BOOST_CHECK_EQUAL(*PackageV3Checks(tx_v3_from_v2, GetVirtualTransactionSize(*tx_v3_from_v2), package_v2_v3, empty_ancestors), expected_error_str);
        CTxMemPool::setEntries entries_mempool_v2{pool.GetIter(mempool_tx_v2->GetHash().ToUint256()).value()};
        BOOST_CHECK_EQUAL(*PackageV3Checks(tx_v3_from_v2, GetVirtualTransactionSize(*tx_v3_from_v2), {tx_v3_from_v2}, entries_mempool_v2), expected_error_str);

        // mempool_tx_v3  mempool_tx_v2
        //            ^    ^
        //    tx_v3_from_v2_and_v3
        auto tx_v3_from_v2_and_v3 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 0}, COutPoint{mempool_tx_v2->GetHash(), 0}}, /*version=*/3);
        auto ancestors_v3_from_both{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_from_v2_and_v3))};
        const auto expected_error_str_2{strprintf("v3 tx %s (wtxid=%s) cannot spend from non-v3 tx %s (wtxid=%s)",
            tx_v3_from_v2_and_v3->GetHash().ToString(), tx_v3_from_v2_and_v3->GetWitnessHash().ToString(),
            mempool_tx_v2->GetHash().ToString(), mempool_tx_v2->GetWitnessHash().ToString())};
        auto result_v3_from_both{SingleV3Checks(tx_v3_from_v2_and_v3, *ancestors_v3_from_both, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_from_v2_and_v3))};
        BOOST_CHECK_EQUAL(result_v3_from_both->first, expected_error_str_2);
        BOOST_CHECK_EQUAL(result_v3_from_both->second, nullptr);

        // tx_v3_from_v2_and_v3 also violates V3_ANCESTOR_LIMIT.
        const auto expected_error_str_3{strprintf("tx %s (wtxid=%s) would have too many ancestors",
            tx_v3_from_v2_and_v3->GetHash().ToString(), tx_v3_from_v2_and_v3->GetWitnessHash().ToString())};
        Package package_v3_v2_v3{mempool_tx_v3, mempool_tx_v2, tx_v3_from_v2_and_v3};
        BOOST_CHECK_EQUAL(*PackageV3Checks(tx_v3_from_v2_and_v3, GetVirtualTransactionSize(*tx_v3_from_v2_and_v3), package_v3_v2_v3, empty_ancestors), expected_error_str_3);
    }
    // V3 from V3 is ok, and non-V3 from non-V3 is ok.
    {
        // mempool_tx_v3
        //      ^
        // tx_v3_from_v3
        auto tx_v3_from_v3 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 0}}, /*version=*/3);
        auto ancestors_v3{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_from_v3))};
        BOOST_CHECK(SingleV3Checks(tx_v3_from_v3, *ancestors_v3, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_from_v3))
                    == std::nullopt);

        Package package_v3_v3{mempool_tx_v3, tx_v3_from_v3};
        BOOST_CHECK(PackageV3Checks(tx_v3_from_v3, GetVirtualTransactionSize(*tx_v3_from_v3), package_v3_v3, empty_ancestors) == std::nullopt);

        // mempool_tx_v2
        //      ^
        // tx_v2_from_v2
        auto tx_v2_from_v2 = make_tx({COutPoint{mempool_tx_v2->GetHash(), 0}}, /*version=*/2);
        auto ancestors_v2{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v2_from_v2))};
        BOOST_CHECK(SingleV3Checks(tx_v2_from_v2, *ancestors_v2, empty_conflicts_set, GetVirtualTransactionSize(*tx_v2_from_v2))
                    == std::nullopt);

        Package package_v2_v2{mempool_tx_v2, tx_v2_from_v2};
        BOOST_CHECK(PackageV3Checks(tx_v2_from_v2, GetVirtualTransactionSize(*tx_v2_from_v2), package_v2_v2, empty_ancestors) == std::nullopt);
    }

    // Tx spending v3 cannot have too many mempool ancestors
    // Configuration where the tx has multiple direct parents.
    {
        Package package_multi_parents;
        std::vector<COutPoint> mempool_outpoints;
        mempool_outpoints.emplace_back(mempool_tx_v3->GetHash(), 0);
        package_multi_parents.emplace_back(mempool_tx_v3);
        for (size_t i{0}; i < 2; ++i) {
            auto mempool_tx = make_tx(random_outpoints(i + 1), /*version=*/3);
            pool.addUnchecked(entry.FromTx(mempool_tx));
            mempool_outpoints.emplace_back(mempool_tx->GetHash(), 0);
            package_multi_parents.emplace_back(mempool_tx);
        }
        auto tx_v3_multi_parent = make_tx(mempool_outpoints, /*version=*/3);
        package_multi_parents.emplace_back(tx_v3_multi_parent);
        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_multi_parent))};
        BOOST_CHECK_EQUAL(ancestors->size(), 3);
        const auto expected_error_str{strprintf("tx %s (wtxid=%s) would have too many ancestors",
            tx_v3_multi_parent->GetHash().ToString(), tx_v3_multi_parent->GetWitnessHash().ToString())};
        auto result{SingleV3Checks(tx_v3_multi_parent, *ancestors, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_multi_parent))};
        BOOST_CHECK_EQUAL(result->first, expected_error_str);
        BOOST_CHECK_EQUAL(result->second, nullptr);

        BOOST_CHECK_EQUAL(*PackageV3Checks(tx_v3_multi_parent, GetVirtualTransactionSize(*tx_v3_multi_parent), package_multi_parents, empty_ancestors),
                          expected_error_str);
    }

    // Configuration where the tx is in a multi-generation chain.
    {
        Package package_multi_gen;
        CTransactionRef middle_tx;
        auto last_outpoint{random_outpoints(1)[0]};
        for (size_t i{0}; i < 2; ++i) {
            auto mempool_tx = make_tx({last_outpoint}, /*version=*/3);
            pool.addUnchecked(entry.FromTx(mempool_tx));
            last_outpoint = COutPoint{mempool_tx->GetHash(), 0};
            package_multi_gen.emplace_back(mempool_tx);
            if (i == 1) middle_tx = mempool_tx;
        }
        auto tx_v3_multi_gen = make_tx({last_outpoint}, /*version=*/3);
        package_multi_gen.emplace_back(tx_v3_multi_gen);
        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_multi_gen))};
        const auto expected_error_str{strprintf("tx %s (wtxid=%s) would have too many ancestors",
            tx_v3_multi_gen->GetHash().ToString(), tx_v3_multi_gen->GetWitnessHash().ToString())};
        auto result{SingleV3Checks(tx_v3_multi_gen, *ancestors, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_multi_gen))};
        BOOST_CHECK_EQUAL(result->first, expected_error_str);
        BOOST_CHECK_EQUAL(result->second, nullptr);

        // Middle tx is what triggers a failure for the grandchild:
        BOOST_CHECK_EQUAL(*PackageV3Checks(middle_tx, GetVirtualTransactionSize(*middle_tx), package_multi_gen, empty_ancestors), expected_error_str);
        BOOST_CHECK(PackageV3Checks(tx_v3_multi_gen, GetVirtualTransactionSize(*tx_v3_multi_gen), package_multi_gen, empty_ancestors) == std::nullopt);
    }

    // Tx spending v3 cannot be too large in virtual size.
    auto many_inputs{random_outpoints(100)};
    many_inputs.emplace_back(mempool_tx_v3->GetHash(), 0);
    {
        auto tx_v3_child_big = make_tx(many_inputs, /*version=*/3);
        const auto vsize{GetVirtualTransactionSize(*tx_v3_child_big)};
        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_child_big))};
        const auto expected_error_str{strprintf("v3 child tx %s (wtxid=%s) is too big: %u > %u virtual bytes",
            tx_v3_child_big->GetHash().ToString(), tx_v3_child_big->GetWitnessHash().ToString(), vsize, V3_CHILD_MAX_VSIZE)};
        auto result{SingleV3Checks(tx_v3_child_big, *ancestors, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_child_big))};
        BOOST_CHECK_EQUAL(result->first, expected_error_str);
        BOOST_CHECK_EQUAL(result->second, nullptr);

        Package package_child_big{mempool_tx_v3, tx_v3_child_big};
        BOOST_CHECK_EQUAL(*PackageV3Checks(tx_v3_child_big, GetVirtualTransactionSize(*tx_v3_child_big), package_child_big, empty_ancestors),
                          expected_error_str);
    }

    // Tx spending v3 cannot have too many sigops.
    // This child has 10 P2WSH multisig inputs.
    auto multisig_outpoints{random_outpoints(10)};
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
        mtx_many_sigops.version = TRUC_VERSION;
        for (const auto& outpoint : multisig_outpoints) {
            mtx_many_sigops.vin.emplace_back(outpoint);
            mtx_many_sigops.vin.back().scriptWitness.stack.emplace_back(script_multisig.begin(), script_multisig.end());
        }
        mtx_many_sigops.vout.resize(1);
        mtx_many_sigops.vout.back().scriptPubKey = CScript() << OP_TRUE;
        mtx_many_sigops.vout.back().nValue = 10000;
        auto tx_many_sigops{MakeTransactionRef(mtx_many_sigops)};

        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_many_sigops))};
        // legacy uses fAccurate = false, and the maximum number of multisig keys is used
        const int64_t total_sigops{static_cast<int64_t>(tx_many_sigops->vin.size()) * static_cast<int64_t>(script_multisig.GetSigOpCount(/*fAccurate=*/false))};
        BOOST_CHECK_EQUAL(total_sigops, tx_many_sigops->vin.size() * MAX_PUBKEYS_PER_MULTISIG);
        const int64_t bip141_vsize{GetVirtualTransactionSize(*tx_many_sigops)};
        // Weight limit is not reached...
        BOOST_CHECK(SingleV3Checks(tx_many_sigops, *ancestors, empty_conflicts_set, bip141_vsize) == std::nullopt);
        // ...but sigop limit is.
        const auto expected_error_str{strprintf("v3 child tx %s (wtxid=%s) is too big: %u > %u virtual bytes",
            tx_many_sigops->GetHash().ToString(), tx_many_sigops->GetWitnessHash().ToString(),
            total_sigops * DEFAULT_BYTES_PER_SIGOP / WITNESS_SCALE_FACTOR, V3_CHILD_MAX_VSIZE)};
        auto result{SingleV3Checks(tx_many_sigops, *ancestors, empty_conflicts_set,
                                        GetVirtualTransactionSize(*tx_many_sigops, /*nSigOpCost=*/total_sigops, /*bytes_per_sigop=*/ DEFAULT_BYTES_PER_SIGOP))};
        BOOST_CHECK_EQUAL(result->first, expected_error_str);
        BOOST_CHECK_EQUAL(result->second, nullptr);

        Package package_child_sigops{mempool_tx_v3, tx_many_sigops};
        BOOST_CHECK_EQUAL(*PackageV3Checks(tx_many_sigops, total_sigops * DEFAULT_BYTES_PER_SIGOP / WITNESS_SCALE_FACTOR, package_child_sigops, empty_ancestors),
                          expected_error_str);
    }

    // Parent + child with v3 in the mempool. Child is allowed as long as it is under V3_CHILD_MAX_VSIZE.
    auto tx_mempool_v3_child = make_tx({COutPoint{mempool_tx_v3->GetHash(), 0}}, /*version=*/3);
    {
        BOOST_CHECK(GetTransactionWeight(*tx_mempool_v3_child) <= V3_CHILD_MAX_VSIZE * WITNESS_SCALE_FACTOR);
        auto ancestors{pool.CalculateMemPoolAncestors(entry.FromTx(tx_mempool_v3_child))};
        BOOST_CHECK(SingleV3Checks(tx_mempool_v3_child, *ancestors, empty_conflicts_set, GetVirtualTransactionSize(*tx_mempool_v3_child)) == std::nullopt);
        pool.addUnchecked(entry.FromTx(tx_mempool_v3_child));

        Package package_v3_1p1c{mempool_tx_v3, tx_mempool_v3_child};
        BOOST_CHECK(PackageV3Checks(tx_mempool_v3_child, GetVirtualTransactionSize(*tx_mempool_v3_child), package_v3_1p1c, empty_ancestors) == std::nullopt);
    }

    // A v3 transaction cannot have more than 1 descendant. Sibling is returned when exactly 1 exists.
    {
        auto tx_v3_child2 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 1}}, /*version=*/3);
        // Configuration where parent already has 1 other child in mempool
        auto ancestors_1sibling{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_child2))};
        const auto expected_error_str{strprintf("tx %s (wtxid=%s) would exceed descendant count limit",
            mempool_tx_v3->GetHash().ToString(), mempool_tx_v3->GetWitnessHash().ToString())};
        auto result_with_sibling_eviction{SingleV3Checks(tx_v3_child2, *ancestors_1sibling, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_child2))};
        BOOST_CHECK_EQUAL(result_with_sibling_eviction->first, expected_error_str);
        // The other mempool child is returned to allow for sibling eviction.
        BOOST_CHECK_EQUAL(result_with_sibling_eviction->second, tx_mempool_v3_child);

        // If directly replacing the child, make sure there is no double-counting.
        BOOST_CHECK(SingleV3Checks(tx_v3_child2, *ancestors_1sibling, {tx_mempool_v3_child->GetHash()}, GetVirtualTransactionSize(*tx_v3_child2))
                    == std::nullopt);

        Package package_v3_1p2c{mempool_tx_v3, tx_mempool_v3_child, tx_v3_child2};
        BOOST_CHECK_EQUAL(*PackageV3Checks(tx_v3_child2, GetVirtualTransactionSize(*tx_v3_child2), package_v3_1p2c, empty_ancestors),
                          expected_error_str);

        // Configuration where parent already has 2 other children in mempool (no sibling eviction allowed). This may happen as the result of a reorg.
        pool.addUnchecked(entry.FromTx(tx_v3_child2));
        auto tx_v3_child3 = make_tx({COutPoint{mempool_tx_v3->GetHash(), 24}}, /*version=*/3);
        auto entry_mempool_parent = pool.GetIter(mempool_tx_v3->GetHash().ToUint256()).value();
        BOOST_CHECK_EQUAL(entry_mempool_parent->GetNumChildren(), 2);
        auto ancestors_2siblings{pool.CalculateMemPoolAncestors(entry.FromTx(tx_v3_child3))};

        auto result_2children{SingleV3Checks(tx_v3_child3, *ancestors_2siblings, empty_conflicts_set, GetVirtualTransactionSize(*tx_v3_child3))};
        BOOST_CHECK_EQUAL(result_2children->first, expected_error_str);
        // The other mempool child is not returned because sibling eviction is not allowed.
        BOOST_CHECK_EQUAL(result_2children->second, nullptr);
    }

    // Sibling eviction: parent already has 1 other child, which also has its own child (no sibling eviction allowed). This may happen as the result of a reorg.
    {
        auto tx_mempool_grandparent = make_tx(random_outpoints(1), /*version=*/3);
        auto tx_mempool_sibling = make_tx({COutPoint{tx_mempool_grandparent->GetHash(), 0}}, /*version=*/3);
        auto tx_mempool_nibling = make_tx({COutPoint{tx_mempool_sibling->GetHash(), 0}}, /*version=*/3);
        auto tx_to_submit = make_tx({COutPoint{tx_mempool_grandparent->GetHash(), 1}}, /*version=*/3);

        pool.addUnchecked(entry.FromTx(tx_mempool_grandparent));
        pool.addUnchecked(entry.FromTx(tx_mempool_sibling));
        pool.addUnchecked(entry.FromTx(tx_mempool_nibling));

        auto ancestors_3gen{pool.CalculateMemPoolAncestors(entry.FromTx(tx_to_submit))};
        const auto expected_error_str{strprintf("tx %s (wtxid=%s) would exceed descendant count limit",
            tx_mempool_grandparent->GetHash().ToString(), tx_mempool_grandparent->GetWitnessHash().ToString())};
        auto result_3gen{SingleV3Checks(tx_to_submit, *ancestors_3gen, empty_conflicts_set, GetVirtualTransactionSize(*tx_to_submit))};
        BOOST_CHECK_EQUAL(result_3gen->first, expected_error_str);
        // The other mempool child is not returned because sibling eviction is not allowed.
        BOOST_CHECK_EQUAL(result_3gen->second, nullptr);
    }

    // Configuration where tx has multiple generations of descendants is not tested because that is
    // equivalent to the tx with multiple generations of ancestors.
}

BOOST_AUTO_TEST_SUITE_END()
