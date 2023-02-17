// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <key_io.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/standard.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txpackage_tests)

// Create placeholder transactions that have no meaning.
inline CTransactionRef create_placeholder_tx(size_t num_inputs, size_t num_outputs)
{
    CMutableTransaction mtx = CMutableTransaction();
    mtx.vin.resize(num_inputs);
    mtx.vout.resize(num_outputs);
    auto random_script = CScript() << ToByteVector(InsecureRand256()) << ToByteVector(InsecureRand256());
    for (size_t i{0}; i < num_inputs; ++i) {
        mtx.vin[i].prevout.hash = InsecureRand256();
        mtx.vin[i].prevout.n = 0;
        mtx.vin[i].scriptSig = random_script;
    }
    for (size_t o{0}; o < num_outputs; ++o) {
        mtx.vout[o].nValue = 1 * CENT;
        mtx.vout[o].scriptPubKey = random_script;
    }
    return MakeTransactionRef(mtx);
}

BOOST_FIXTURE_TEST_CASE(package_sanitization_tests, TestChain100Setup)
{
    // Packages can't have more than 25 transactions.
    Package package_too_many;
    package_too_many.reserve(MAX_PACKAGE_COUNT + 1);
    for (size_t i{0}; i < MAX_PACKAGE_COUNT + 1; ++i) {
        package_too_many.emplace_back(create_placeholder_tx(1, 1));
    }
    PackageValidationState state_too_many;
    BOOST_CHECK(!CheckPackage(package_too_many, state_too_many));
    BOOST_CHECK_EQUAL(state_too_many.GetResult(), PackageValidationResult::PCKG_POLICY);
    BOOST_CHECK_EQUAL(state_too_many.GetRejectReason(), "package-too-many-transactions");

    // Packages can't have a total size of more than 101KvB.
    CTransactionRef large_ptx = create_placeholder_tx(150, 150);
    Package package_too_large;
    auto size_large = GetVirtualTransactionSize(*large_ptx);
    size_t total_size{0};
    while (total_size <= MAX_PACKAGE_SIZE * 1000) {
        package_too_large.push_back(large_ptx);
        total_size += size_large;
    }
    BOOST_CHECK(package_too_large.size() <= MAX_PACKAGE_COUNT);
    PackageValidationState state_too_large;
    BOOST_CHECK(!CheckPackage(package_too_large, state_too_large));
    BOOST_CHECK_EQUAL(state_too_large.GetResult(), PackageValidationResult::PCKG_POLICY);
    BOOST_CHECK_EQUAL(state_too_large.GetRejectReason(), "package-too-large");
}

BOOST_FIXTURE_TEST_CASE(package_validation_tests, TestChain100Setup)
{
    LOCK(cs_main);
    unsigned int initialPoolSize = m_node.mempool->size();

    // Parent and Child Package
    CKey parent_key;
    parent_key.MakeNewKey(true);
    CScript parent_locking_script = GetScriptForDestination(PKHash(parent_key.GetPubKey()));
    auto mtx_parent = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[0], /*input_vout=*/0,
                                                    /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                    /*output_destination=*/parent_locking_script,
                                                    /*output_amount=*/CAmount(49 * COIN), /*submit=*/false);
    CTransactionRef tx_parent = MakeTransactionRef(mtx_parent);

    CKey child_key;
    child_key.MakeNewKey(true);
    CScript child_locking_script = GetScriptForDestination(PKHash(child_key.GetPubKey()));
    auto mtx_child = CreateValidMempoolTransaction(/*input_transaction=*/tx_parent, /*input_vout=*/0,
                                                   /*input_height=*/101, /*input_signing_key=*/parent_key,
                                                   /*output_destination=*/child_locking_script,
                                                   /*output_amount=*/CAmount(48 * COIN), /*submit=*/false);
    CTransactionRef tx_child = MakeTransactionRef(mtx_child);
    const auto result_parent_child = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool, {tx_parent, tx_child}, /*test_accept=*/true);
    BOOST_CHECK_MESSAGE(result_parent_child.m_state.IsValid(),
                        "Package validation unexpectedly failed: " << result_parent_child.m_state.GetRejectReason());
    BOOST_CHECK(result_parent_child.m_tx_results.size() == 2);
    auto it_parent = result_parent_child.m_tx_results.find(tx_parent->GetWitnessHash());
    auto it_child = result_parent_child.m_tx_results.find(tx_child->GetWitnessHash());
    BOOST_CHECK(it_parent != result_parent_child.m_tx_results.end());
    BOOST_CHECK_MESSAGE(it_parent->second.m_state.IsValid(),
                        "Package validation unexpectedly failed: " << it_parent->second.m_state.GetRejectReason());
    BOOST_CHECK(it_parent->second.m_effective_feerate.value().GetFee(GetVirtualTransactionSize(*tx_parent)) == COIN);
    BOOST_CHECK_EQUAL(it_parent->second.m_wtxids_fee_calculations.value().size(), 1);
    BOOST_CHECK_EQUAL(it_parent->second.m_wtxids_fee_calculations.value().front(), tx_parent->GetWitnessHash());
    BOOST_CHECK(it_child != result_parent_child.m_tx_results.end());
    BOOST_CHECK_MESSAGE(it_child->second.m_state.IsValid(),
                        "Package validation unexpectedly failed: " << it_child->second.m_state.GetRejectReason());
    BOOST_CHECK(it_child->second.m_effective_feerate.value().GetFee(GetVirtualTransactionSize(*tx_child)) == COIN);
    BOOST_CHECK_EQUAL(it_child->second.m_wtxids_fee_calculations.value().size(), 1);
    BOOST_CHECK_EQUAL(it_child->second.m_wtxids_fee_calculations.value().front(), tx_child->GetWitnessHash());

    // A single, giant transaction submitted through ProcessNewPackage fails on single tx policy.
    CTransactionRef giant_ptx = create_placeholder_tx(999, 999);
    BOOST_CHECK(GetVirtualTransactionSize(*giant_ptx) > MAX_PACKAGE_SIZE * 1000);
    auto result_single_large = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool, {giant_ptx}, /*test_accept=*/true);
    BOOST_CHECK(result_single_large.m_state.IsInvalid());
    BOOST_CHECK_EQUAL(result_single_large.m_state.GetResult(), PackageValidationResult::PCKG_TX);
    BOOST_CHECK_EQUAL(result_single_large.m_state.GetRejectReason(), "transaction failed");
    BOOST_CHECK(result_single_large.m_tx_results.size() == 1);
    auto it_giant_tx = result_single_large.m_tx_results.find(giant_ptx->GetWitnessHash());
    BOOST_CHECK(it_giant_tx != result_single_large.m_tx_results.end());
    BOOST_CHECK_EQUAL(it_giant_tx->second.m_state.GetRejectReason(), "tx-size");

    // Check that mempool size hasn't changed.
    BOOST_CHECK_EQUAL(m_node.mempool->size(), initialPoolSize);
}

BOOST_FIXTURE_TEST_CASE(noncontextual_package_tests, TestChain100Setup)
{
    // The signatures won't be verified so we can just use a placeholder
    CKey placeholder_key;
    placeholder_key.MakeNewKey(true);
    CScript spk = GetScriptForDestination(PKHash(placeholder_key.GetPubKey()));
    CKey placeholder_key_2;
    placeholder_key_2.MakeNewKey(true);
    CScript spk2 = GetScriptForDestination(PKHash(placeholder_key_2.GetPubKey()));

    // Parent and Child Package
    {
        auto mtx_parent = CreateValidMempoolTransaction(m_coinbase_txns[0], 0, 0, coinbaseKey, spk,
                                                        CAmount(49 * COIN), /*submit=*/false);
        CTransactionRef tx_parent = MakeTransactionRef(mtx_parent);

        auto mtx_child = CreateValidMempoolTransaction(tx_parent, 0, 101, placeholder_key, spk2,
                                                       CAmount(48 * COIN), /*submit=*/false);
        CTransactionRef tx_child = MakeTransactionRef(mtx_child);

        PackageValidationState state;
        BOOST_CHECK(CheckPackage({tx_parent, tx_child}, state));
        BOOST_CHECK(!CheckPackage({tx_child, tx_parent}, state));
        BOOST_CHECK_EQUAL(state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "package-not-sorted");
        BOOST_CHECK(IsChildWithParents({tx_parent, tx_child}));
    }

    // 24 Parents and 1 Child
    {
        Package package;
        CMutableTransaction child;
        for (int i{0}; i < 24; ++i) {
            auto parent = MakeTransactionRef(CreateValidMempoolTransaction(m_coinbase_txns[i + 1],
                                             0, 0, coinbaseKey, spk, CAmount(48 * COIN), false));
            package.emplace_back(parent);
            child.vin.push_back(CTxIn(COutPoint(parent->GetHash(), 0)));
        }
        child.vout.push_back(CTxOut(47 * COIN, spk2));

        // The child must be in the package.
        BOOST_CHECK(!IsChildWithParents(package));

        // The parents can be in any order.
        FastRandomContext rng;
        Shuffle(package.begin(), package.end(), rng);
        package.push_back(MakeTransactionRef(child));

        PackageValidationState state;
        BOOST_CHECK(CheckPackage(package, state));
        BOOST_CHECK(IsChildWithParents(package));

        package.erase(package.begin());
        BOOST_CHECK(IsChildWithParents(package));

        // The package cannot have unrelated transactions.
        package.insert(package.begin(), m_coinbase_txns[0]);
        BOOST_CHECK(!IsChildWithParents(package));
    }

    // 2 Parents and 1 Child where one parent depends on the other.
    {
        CMutableTransaction mtx_parent;
        mtx_parent.vin.push_back(CTxIn(COutPoint(m_coinbase_txns[0]->GetHash(), 0)));
        mtx_parent.vout.push_back(CTxOut(20 * COIN, spk));
        mtx_parent.vout.push_back(CTxOut(20 * COIN, spk2));
        CTransactionRef tx_parent = MakeTransactionRef(mtx_parent);

        CMutableTransaction mtx_parent_also_child;
        mtx_parent_also_child.vin.push_back(CTxIn(COutPoint(tx_parent->GetHash(), 0)));
        mtx_parent_also_child.vout.push_back(CTxOut(20 * COIN, spk));
        CTransactionRef tx_parent_also_child = MakeTransactionRef(mtx_parent_also_child);

        CMutableTransaction mtx_child;
        mtx_child.vin.push_back(CTxIn(COutPoint(tx_parent->GetHash(), 1)));
        mtx_child.vin.push_back(CTxIn(COutPoint(tx_parent_also_child->GetHash(), 0)));
        mtx_child.vout.push_back(CTxOut(39 * COIN, spk));
        CTransactionRef tx_child = MakeTransactionRef(mtx_child);

        PackageValidationState state;
        BOOST_CHECK(IsChildWithParents({tx_parent, tx_parent_also_child}));
        BOOST_CHECK(IsChildWithParents({tx_parent, tx_child}));
        BOOST_CHECK(IsChildWithParents({tx_parent, tx_parent_also_child, tx_child}));
        // IsChildWithParents does not detect unsorted parents.
        BOOST_CHECK(IsChildWithParents({tx_parent_also_child, tx_parent, tx_child}));
        BOOST_CHECK(CheckPackage({tx_parent, tx_parent_also_child, tx_child}, state));
        BOOST_CHECK(!CheckPackage({tx_parent_also_child, tx_parent, tx_child}, state));
        BOOST_CHECK_EQUAL(state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "package-not-sorted");
    }
}

BOOST_FIXTURE_TEST_CASE(package_submission_tests, TestChain100Setup)
{
    LOCK(cs_main);
    unsigned int expected_pool_size = m_node.mempool->size();
    CKey parent_key;
    parent_key.MakeNewKey(true);
    CScript parent_locking_script = GetScriptForDestination(PKHash(parent_key.GetPubKey()));

    // Unrelated transactions are not allowed in package submission.
    Package package_unrelated;
    for (size_t i{0}; i < 10; ++i) {
        auto mtx = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[i + 25], /*input_vout=*/0,
                                                 /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                 /*output_destination=*/parent_locking_script,
                                                 /*output_amount=*/CAmount(49 * COIN), /*submit=*/false);
        package_unrelated.emplace_back(MakeTransactionRef(mtx));
    }
    auto result_unrelated_submit = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                     package_unrelated, /*test_accept=*/false);
    BOOST_CHECK(result_unrelated_submit.m_state.IsInvalid());
    BOOST_CHECK_EQUAL(result_unrelated_submit.m_state.GetResult(), PackageValidationResult::PCKG_POLICY);
    BOOST_CHECK_EQUAL(result_unrelated_submit.m_state.GetRejectReason(), "package-not-child-with-parents");
    BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);

    // Parent and Child (and Grandchild) Package
    Package package_parent_child;
    Package package_3gen;
    auto mtx_parent = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[0], /*input_vout=*/0,
                                                    /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                    /*output_destination=*/parent_locking_script,
                                                    /*output_amount=*/CAmount(49 * COIN), /*submit=*/false);
    CTransactionRef tx_parent = MakeTransactionRef(mtx_parent);
    package_parent_child.push_back(tx_parent);
    package_3gen.push_back(tx_parent);

    CKey child_key;
    child_key.MakeNewKey(true);
    CScript child_locking_script = GetScriptForDestination(PKHash(child_key.GetPubKey()));
    auto mtx_child = CreateValidMempoolTransaction(/*input_transaction=*/tx_parent, /*input_vout=*/0,
                                                   /*input_height=*/101, /*input_signing_key=*/parent_key,
                                                   /*output_destination=*/child_locking_script,
                                                   /*output_amount=*/CAmount(48 * COIN), /*submit=*/false);
    CTransactionRef tx_child = MakeTransactionRef(mtx_child);
    package_parent_child.push_back(tx_child);
    package_3gen.push_back(tx_child);

    CKey grandchild_key;
    grandchild_key.MakeNewKey(true);
    CScript grandchild_locking_script = GetScriptForDestination(PKHash(grandchild_key.GetPubKey()));
    auto mtx_grandchild = CreateValidMempoolTransaction(/*input_transaction=*/tx_child, /*input_vout=*/0,
                                                       /*input_height=*/101, /*input_signing_key=*/child_key,
                                                       /*output_destination=*/grandchild_locking_script,
                                                       /*output_amount=*/CAmount(47 * COIN), /*submit=*/false);
    CTransactionRef tx_grandchild = MakeTransactionRef(mtx_grandchild);
    package_3gen.push_back(tx_grandchild);

    // 3 Generations is not allowed.
    {
        auto result_3gen_submit = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                    package_3gen, /*test_accept=*/false);
        BOOST_CHECK(result_3gen_submit.m_state.IsInvalid());
        BOOST_CHECK_EQUAL(result_3gen_submit.m_state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_EQUAL(result_3gen_submit.m_state.GetRejectReason(), "package-not-child-with-parents");
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
    }

    // Parent and child package where transactions are invalid for reasons other than fee and
    // missing inputs, so the package validation isn't expected to happen.
    {
        CScriptWitness bad_witness;
        bad_witness.stack.push_back(std::vector<unsigned char>(1));
        CMutableTransaction mtx_parent_invalid{mtx_parent};
        mtx_parent_invalid.vin[0].scriptWitness = bad_witness;
        CTransactionRef tx_parent_invalid = MakeTransactionRef(mtx_parent_invalid);
        auto result_quit_early = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                   {tx_parent_invalid, tx_child}, /*test_accept=*/ false);
        BOOST_CHECK(result_quit_early.m_state.IsInvalid());
        BOOST_CHECK_EQUAL(result_quit_early.m_state.GetResult(), PackageValidationResult::PCKG_TX);
        BOOST_CHECK(!result_quit_early.m_tx_results.empty());
        BOOST_CHECK_EQUAL(result_quit_early.m_tx_results.size(), 2);
        auto it_parent = result_quit_early.m_tx_results.find(tx_parent_invalid->GetWitnessHash());
        auto it_child = result_quit_early.m_tx_results.find(tx_child->GetWitnessHash());
        BOOST_CHECK(it_parent != result_quit_early.m_tx_results.end());
        BOOST_CHECK(it_child != result_quit_early.m_tx_results.end());
        BOOST_CHECK_EQUAL(it_parent->second.m_state.GetResult(), TxValidationResult::TX_WITNESS_MUTATED);
        BOOST_CHECK_EQUAL(it_parent->second.m_state.GetRejectReason(), "bad-witness-nonstandard");
        BOOST_CHECK_EQUAL(it_child->second.m_state.GetResult(), TxValidationResult::TX_MISSING_INPUTS);
        BOOST_CHECK_EQUAL(it_child->second.m_state.GetRejectReason(), "bad-txns-inputs-missingorspent");
    }

    // Child with missing parent.
    mtx_child.vin.push_back(CTxIn(COutPoint(package_unrelated[0]->GetHash(), 0)));
    Package package_missing_parent;
    package_missing_parent.push_back(tx_parent);
    package_missing_parent.push_back(MakeTransactionRef(mtx_child));
    {
        const auto result_missing_parent = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                             package_missing_parent, /*test_accept=*/false);
        BOOST_CHECK(result_missing_parent.m_state.IsInvalid());
        BOOST_CHECK_EQUAL(result_missing_parent.m_state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_EQUAL(result_missing_parent.m_state.GetRejectReason(), "package-not-child-with-unconfirmed-parents");
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
    }

    // Submit package with parent + child.
    {
        const auto submit_parent_child = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                           package_parent_child, /*test_accept=*/false);
        expected_pool_size += 2;
        BOOST_CHECK_MESSAGE(submit_parent_child.m_state.IsValid(),
                            "Package validation unexpectedly failed: " << submit_parent_child.m_state.GetRejectReason());
        BOOST_CHECK_EQUAL(submit_parent_child.m_tx_results.size(), package_parent_child.size());
        auto it_parent = submit_parent_child.m_tx_results.find(tx_parent->GetWitnessHash());
        auto it_child = submit_parent_child.m_tx_results.find(tx_child->GetWitnessHash());
        BOOST_CHECK(it_parent != submit_parent_child.m_tx_results.end());
        BOOST_CHECK(it_parent->second.m_state.IsValid());
        BOOST_CHECK(it_parent->second.m_effective_feerate == CFeeRate(1 * COIN, GetVirtualTransactionSize(*tx_parent)));
        BOOST_CHECK_EQUAL(it_parent->second.m_wtxids_fee_calculations.value().size(), 1);
        BOOST_CHECK_EQUAL(it_parent->second.m_wtxids_fee_calculations.value().front(), tx_parent->GetWitnessHash());
        BOOST_CHECK(it_child != submit_parent_child.m_tx_results.end());
        BOOST_CHECK(it_child->second.m_state.IsValid());
        BOOST_CHECK(it_child->second.m_effective_feerate == CFeeRate(1 * COIN, GetVirtualTransactionSize(*tx_child)));
        BOOST_CHECK_EQUAL(it_child->second.m_wtxids_fee_calculations.value().size(), 1);
        BOOST_CHECK_EQUAL(it_child->second.m_wtxids_fee_calculations.value().front(), tx_child->GetWitnessHash());

        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(tx_parent->GetHash())));
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(tx_child->GetHash())));
    }

    // Already-in-mempool transactions should be detected and de-duplicated.
    {
        const auto submit_deduped = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                      package_parent_child, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(submit_deduped.m_state.IsValid(),
                            "Package validation unexpectedly failed: " << submit_deduped.m_state.GetRejectReason());
        BOOST_CHECK_EQUAL(submit_deduped.m_tx_results.size(), package_parent_child.size());
        auto it_parent_deduped = submit_deduped.m_tx_results.find(tx_parent->GetWitnessHash());
        auto it_child_deduped = submit_deduped.m_tx_results.find(tx_child->GetWitnessHash());
        BOOST_CHECK(it_parent_deduped != submit_deduped.m_tx_results.end());
        BOOST_CHECK(it_parent_deduped->second.m_state.IsValid());
        BOOST_CHECK(it_parent_deduped->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
        BOOST_CHECK(it_child_deduped != submit_deduped.m_tx_results.end());
        BOOST_CHECK(it_child_deduped->second.m_state.IsValid());
        BOOST_CHECK(it_child_deduped->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);

        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(tx_parent->GetHash())));
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(tx_child->GetHash())));
    }
}

// Tests for packages containing transactions that have same-txid-different-witness equivalents in
// the mempool.
BOOST_FIXTURE_TEST_CASE(package_witness_swap_tests, TestChain100Setup)
{
    // Mine blocks to mature coinbases.
    mineBlocks(5);
    LOCK(cs_main);

    // Transactions with a same-txid-different-witness transaction in the mempool should be ignored,
    // and the mempool entry's wtxid returned.
    CScript witnessScript = CScript() << OP_DROP << OP_TRUE;
    CScript scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(witnessScript));
    auto mtx_parent = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[0], /*input_vout=*/0,
                                                    /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                    /*output_destination=*/scriptPubKey,
                                                    /*output_amount=*/CAmount(49 * COIN), /*submit=*/false);
    CTransactionRef ptx_parent = MakeTransactionRef(mtx_parent);

    // Make two children with the same txid but different witnesses.
    CScriptWitness witness1;
    witness1.stack.push_back(std::vector<unsigned char>(1));
    witness1.stack.push_back(std::vector<unsigned char>(witnessScript.begin(), witnessScript.end()));

    CScriptWitness witness2(witness1);
    witness2.stack.push_back(std::vector<unsigned char>(2));
    witness2.stack.push_back(std::vector<unsigned char>(witnessScript.begin(), witnessScript.end()));

    CKey child_key;
    child_key.MakeNewKey(true);
    CScript child_locking_script = GetScriptForDestination(WitnessV0KeyHash(child_key.GetPubKey()));
    CMutableTransaction mtx_child1;
    mtx_child1.nVersion = 1;
    mtx_child1.vin.resize(1);
    mtx_child1.vin[0].prevout.hash = ptx_parent->GetHash();
    mtx_child1.vin[0].prevout.n = 0;
    mtx_child1.vin[0].scriptSig = CScript();
    mtx_child1.vin[0].scriptWitness = witness1;
    mtx_child1.vout.resize(1);
    mtx_child1.vout[0].nValue = CAmount(48 * COIN);
    mtx_child1.vout[0].scriptPubKey = child_locking_script;

    CMutableTransaction mtx_child2{mtx_child1};
    mtx_child2.vin[0].scriptWitness = witness2;

    CTransactionRef ptx_child1 = MakeTransactionRef(mtx_child1);
    CTransactionRef ptx_child2 = MakeTransactionRef(mtx_child2);

    // child1 and child2 have the same txid
    BOOST_CHECK_EQUAL(ptx_child1->GetHash(), ptx_child2->GetHash());
    // child1 and child2 have different wtxids
    BOOST_CHECK(ptx_child1->GetWitnessHash() != ptx_child2->GetWitnessHash());

    // Try submitting Package1{parent, child1} and Package2{parent, child2} where the children are
    // same-txid-different-witness.
    {
        const auto submit_witness1 = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                       {ptx_parent, ptx_child1}, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(submit_witness1.m_state.IsValid(),
                            "Package validation unexpectedly failed: " << submit_witness1.m_state.GetRejectReason());
        BOOST_CHECK_EQUAL(submit_witness1.m_tx_results.size(), 2);
        auto it_parent1 = submit_witness1.m_tx_results.find(ptx_parent->GetWitnessHash());
        auto it_child1 = submit_witness1.m_tx_results.find(ptx_child1->GetWitnessHash());
        BOOST_CHECK(it_parent1 != submit_witness1.m_tx_results.end());
        BOOST_CHECK_MESSAGE(it_parent1->second.m_state.IsValid(),
                            "Transaction unexpectedly failed: " << it_parent1->second.m_state.GetRejectReason());
        BOOST_CHECK(it_child1 != submit_witness1.m_tx_results.end());
        BOOST_CHECK_MESSAGE(it_child1->second.m_state.IsValid(),
                            "Transaction unexpectedly failed: " << it_child1->second.m_state.GetRejectReason());

        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(ptx_parent->GetHash())));
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(ptx_child1->GetHash())));

        // Child2 would have been validated individually.
        const auto submit_witness2 = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                       {ptx_parent, ptx_child2}, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(submit_witness2.m_state.IsValid(),
                            "Package validation unexpectedly failed: " << submit_witness2.m_state.GetRejectReason());
        BOOST_CHECK_EQUAL(submit_witness2.m_tx_results.size(), 2);
        auto it_parent2_deduped = submit_witness2.m_tx_results.find(ptx_parent->GetWitnessHash());
        auto it_child2 = submit_witness2.m_tx_results.find(ptx_child2->GetWitnessHash());
        BOOST_CHECK(it_parent2_deduped != submit_witness2.m_tx_results.end());
        BOOST_CHECK(it_parent2_deduped->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
        BOOST_CHECK(it_child2 != submit_witness2.m_tx_results.end());
        BOOST_CHECK(it_child2->second.m_result_type == MempoolAcceptResult::ResultType::DIFFERENT_WITNESS);
        BOOST_CHECK_EQUAL(ptx_child1->GetWitnessHash(), it_child2->second.m_other_wtxid.value());

        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(ptx_child2->GetHash())));
        BOOST_CHECK(!m_node.mempool->exists(GenTxid::Wtxid(ptx_child2->GetWitnessHash())));

        // Deduplication should work when wtxid != txid. Submit package with the already-in-mempool
        // transactions again, which should not fail.
        const auto submit_segwit_dedup = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                           {ptx_parent, ptx_child1}, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(submit_segwit_dedup.m_state.IsValid(),
                            "Package validation unexpectedly failed: " << submit_segwit_dedup.m_state.GetRejectReason());
        BOOST_CHECK_EQUAL(submit_segwit_dedup.m_tx_results.size(), 2);
        auto it_parent_dup = submit_segwit_dedup.m_tx_results.find(ptx_parent->GetWitnessHash());
        auto it_child_dup = submit_segwit_dedup.m_tx_results.find(ptx_child1->GetWitnessHash());
        BOOST_CHECK(it_parent_dup->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
        BOOST_CHECK(it_child_dup->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
    }

    // Try submitting Package1{child2, grandchild} where child2 is same-txid-different-witness as
    // the in-mempool transaction, child1. Since child1 exists in the mempool and its outputs are
    // available, child2 should be ignored and grandchild should be accepted.
    //
    // This tests a potential censorship vector in which an attacker broadcasts a competing package
    // where a parent's witness is mutated. The honest package should be accepted despite the fact
    // that we don't allow witness replacement.
    CKey grandchild_key;
    grandchild_key.MakeNewKey(true);
    CScript grandchild_locking_script = GetScriptForDestination(WitnessV0KeyHash(grandchild_key.GetPubKey()));
    auto mtx_grandchild = CreateValidMempoolTransaction(/*input_transaction=*/ptx_child2, /*input_vout=*/0,
                                                        /*input_height=*/0, /*input_signing_key=*/child_key,
                                                        /*output_destination=*/grandchild_locking_script,
                                                        /*output_amount=*/CAmount(47 * COIN), /*submit=*/false);
    CTransactionRef ptx_grandchild = MakeTransactionRef(mtx_grandchild);

    // We already submitted child1 above.
    {
        const auto submit_spend_ignored = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                            {ptx_child2, ptx_grandchild}, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(submit_spend_ignored.m_state.IsValid(),
                            "Package validation unexpectedly failed: " << submit_spend_ignored.m_state.GetRejectReason());
        BOOST_CHECK_EQUAL(submit_spend_ignored.m_tx_results.size(), 2);
        auto it_child2_ignored = submit_spend_ignored.m_tx_results.find(ptx_child2->GetWitnessHash());
        auto it_grandchild = submit_spend_ignored.m_tx_results.find(ptx_grandchild->GetWitnessHash());
        BOOST_CHECK(it_child2_ignored != submit_spend_ignored.m_tx_results.end());
        BOOST_CHECK(it_child2_ignored->second.m_result_type == MempoolAcceptResult::ResultType::DIFFERENT_WITNESS);
        BOOST_CHECK(it_grandchild != submit_spend_ignored.m_tx_results.end());
        BOOST_CHECK(it_grandchild->second.m_result_type == MempoolAcceptResult::ResultType::VALID);

        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(ptx_child2->GetHash())));
        BOOST_CHECK(!m_node.mempool->exists(GenTxid::Wtxid(ptx_child2->GetWitnessHash())));
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Wtxid(ptx_grandchild->GetWitnessHash())));
    }

    // A package Package{parent1, parent2, parent3, child} where the parents are a mixture of
    // identical-tx-in-mempool, same-txid-different-witness-in-mempool, and new transactions.
    Package package_mixed;

    // Give all the parents anyone-can-spend scripts so we don't have to deal with signing the child.
    CScript acs_script = CScript() << OP_TRUE;
    CScript acs_spk = GetScriptForDestination(WitnessV0ScriptHash(acs_script));
    CScriptWitness acs_witness;
    acs_witness.stack.push_back(std::vector<unsigned char>(acs_script.begin(), acs_script.end()));

    // parent1 will already be in the mempool
    auto mtx_parent1 = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[1], /*input_vout=*/0,
                                                     /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                     /*output_destination=*/acs_spk,
                                                     /*output_amount=*/CAmount(49 * COIN), /*submit=*/true);
    CTransactionRef ptx_parent1 = MakeTransactionRef(mtx_parent1);
    package_mixed.push_back(ptx_parent1);

    // parent2 will have a same-txid-different-witness tx already in the mempool
    CScript grandparent2_script = CScript() << OP_DROP << OP_TRUE;
    CScript grandparent2_spk = GetScriptForDestination(WitnessV0ScriptHash(grandparent2_script));
    CScriptWitness parent2_witness1;
    parent2_witness1.stack.push_back(std::vector<unsigned char>(1));
    parent2_witness1.stack.push_back(std::vector<unsigned char>(grandparent2_script.begin(), grandparent2_script.end()));
    CScriptWitness parent2_witness2;
    parent2_witness2.stack.push_back(std::vector<unsigned char>(2));
    parent2_witness2.stack.push_back(std::vector<unsigned char>(grandparent2_script.begin(), grandparent2_script.end()));

    // Create grandparent2 creating an output with multiple spending paths. Submit to mempool.
    auto mtx_grandparent2 = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[2], /*input_vout=*/0,
                                                          /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                          /*output_destination=*/grandparent2_spk,
                                                          /*output_amount=*/CAmount(49 * COIN), /*submit=*/true);
    CTransactionRef ptx_grandparent2 = MakeTransactionRef(mtx_grandparent2);

    CMutableTransaction mtx_parent2_v1;
    mtx_parent2_v1.nVersion = 1;
    mtx_parent2_v1.vin.resize(1);
    mtx_parent2_v1.vin[0].prevout.hash = ptx_grandparent2->GetHash();
    mtx_parent2_v1.vin[0].prevout.n = 0;
    mtx_parent2_v1.vin[0].scriptSig = CScript();
    mtx_parent2_v1.vin[0].scriptWitness = parent2_witness1;
    mtx_parent2_v1.vout.resize(1);
    mtx_parent2_v1.vout[0].nValue = CAmount(48 * COIN);
    mtx_parent2_v1.vout[0].scriptPubKey = acs_spk;

    CMutableTransaction mtx_parent2_v2{mtx_parent2_v1};
    mtx_parent2_v2.vin[0].scriptWitness = parent2_witness2;

    CTransactionRef ptx_parent2_v1 = MakeTransactionRef(mtx_parent2_v1);
    CTransactionRef ptx_parent2_v2 = MakeTransactionRef(mtx_parent2_v2);
    // Put parent2_v1 in the package, submit parent2_v2 to the mempool.
    const MempoolAcceptResult parent2_v2_result = m_node.chainman->ProcessTransaction(ptx_parent2_v2);
    BOOST_CHECK(parent2_v2_result.m_result_type == MempoolAcceptResult::ResultType::VALID);
    package_mixed.push_back(ptx_parent2_v1);

    // parent3 will be a new transaction. Put 0 fees on it to make it invalid on its own.
    auto mtx_parent3 = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[3], /*input_vout=*/0,
                                                     /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                     /*output_destination=*/acs_spk,
                                                     /*output_amount=*/CAmount(50 * COIN), /*submit=*/false);
    CTransactionRef ptx_parent3 = MakeTransactionRef(mtx_parent3);
    package_mixed.push_back(ptx_parent3);

    // child spends parent1, parent2, and parent3
    CKey mixed_grandchild_key;
    mixed_grandchild_key.MakeNewKey(true);
    CScript mixed_child_spk = GetScriptForDestination(WitnessV0KeyHash(mixed_grandchild_key.GetPubKey()));

    CMutableTransaction mtx_mixed_child;
    mtx_mixed_child.vin.push_back(CTxIn(COutPoint(ptx_parent1->GetHash(), 0)));
    mtx_mixed_child.vin.push_back(CTxIn(COutPoint(ptx_parent2_v1->GetHash(), 0)));
    mtx_mixed_child.vin.push_back(CTxIn(COutPoint(ptx_parent3->GetHash(), 0)));
    mtx_mixed_child.vin[0].scriptWitness = acs_witness;
    mtx_mixed_child.vin[1].scriptWitness = acs_witness;
    mtx_mixed_child.vin[2].scriptWitness = acs_witness;
    mtx_mixed_child.vout.push_back(CTxOut((48 + 49 + 50 - 1) * COIN, mixed_child_spk));
    CTransactionRef ptx_mixed_child = MakeTransactionRef(mtx_mixed_child);
    package_mixed.push_back(ptx_mixed_child);

    // Submit package:
    // parent1 should be ignored
    // parent2_v1 should be ignored (and v2 wtxid returned)
    // parent3 should be accepted
    // child should be accepted
    {
        const auto mixed_result = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool, package_mixed, false);
        BOOST_CHECK_MESSAGE(mixed_result.m_state.IsValid(), mixed_result.m_state.GetRejectReason());
        BOOST_CHECK_EQUAL(mixed_result.m_tx_results.size(), package_mixed.size());
        auto it_parent1 = mixed_result.m_tx_results.find(ptx_parent1->GetWitnessHash());
        auto it_parent2 = mixed_result.m_tx_results.find(ptx_parent2_v1->GetWitnessHash());
        auto it_parent3 = mixed_result.m_tx_results.find(ptx_parent3->GetWitnessHash());
        auto it_child = mixed_result.m_tx_results.find(ptx_mixed_child->GetWitnessHash());
        BOOST_CHECK(it_parent1 != mixed_result.m_tx_results.end());
        BOOST_CHECK(it_parent2 != mixed_result.m_tx_results.end());
        BOOST_CHECK(it_parent3 != mixed_result.m_tx_results.end());
        BOOST_CHECK(it_child != mixed_result.m_tx_results.end());

        BOOST_CHECK(it_parent1->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
        BOOST_CHECK(it_parent2->second.m_result_type == MempoolAcceptResult::ResultType::DIFFERENT_WITNESS);
        BOOST_CHECK(it_parent3->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
        BOOST_CHECK(it_child->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
        BOOST_CHECK_EQUAL(ptx_parent2_v2->GetWitnessHash(), it_parent2->second.m_other_wtxid.value());

        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(ptx_parent1->GetHash())));
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(ptx_parent2_v1->GetHash())));
        BOOST_CHECK(!m_node.mempool->exists(GenTxid::Wtxid(ptx_parent2_v1->GetWitnessHash())));
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(ptx_parent3->GetHash())));
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(ptx_mixed_child->GetHash())));

        // package feerate should include parent3 and child. It should not include parent1 or parent2_v1.
        const CFeeRate expected_feerate(1 * COIN, GetVirtualTransactionSize(*ptx_parent3) + GetVirtualTransactionSize(*ptx_mixed_child));
        BOOST_CHECK(it_parent3->second.m_effective_feerate.value() == expected_feerate);
        BOOST_CHECK(it_child->second.m_effective_feerate.value() == expected_feerate);
        std::vector<uint256> expected_wtxids({ptx_parent3->GetWitnessHash(), ptx_mixed_child->GetWitnessHash()});
        BOOST_CHECK(it_parent3->second.m_wtxids_fee_calculations.value() == expected_wtxids);
        BOOST_CHECK(it_child->second.m_wtxids_fee_calculations.value() == expected_wtxids);
    }
}

BOOST_FIXTURE_TEST_CASE(package_cpfp_tests, TestChain100Setup)
{
    mineBlocks(5);
    LOCK(::cs_main);
    size_t expected_pool_size = m_node.mempool->size();
    CKey child_key;
    child_key.MakeNewKey(true);
    CScript parent_spk = GetScriptForDestination(WitnessV0KeyHash(child_key.GetPubKey()));
    CKey grandchild_key;
    grandchild_key.MakeNewKey(true);
    CScript child_spk = GetScriptForDestination(WitnessV0KeyHash(grandchild_key.GetPubKey()));

    // zero-fee parent and high-fee child package
    const CAmount coinbase_value{50 * COIN};
    const CAmount parent_value{coinbase_value - 0};
    const CAmount child_value{parent_value - COIN};

    Package package_cpfp;
    auto mtx_parent = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[0], /*input_vout=*/0,
                                                    /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                    /*output_destination=*/parent_spk,
                                                    /*output_amount=*/parent_value, /*submit=*/false);
    CTransactionRef tx_parent = MakeTransactionRef(mtx_parent);
    package_cpfp.push_back(tx_parent);

    auto mtx_child = CreateValidMempoolTransaction(/*input_transaction=*/tx_parent, /*input_vout=*/0,
                                                   /*input_height=*/101, /*input_signing_key=*/child_key,
                                                   /*output_destination=*/child_spk,
                                                   /*output_amount=*/child_value, /*submit=*/false);
    CTransactionRef tx_child = MakeTransactionRef(mtx_child);
    package_cpfp.push_back(tx_child);

    // Package feerate is calculated using modified fees, and prioritisetransaction accepts negative
    // fee deltas. This should be taken into account. De-prioritise the parent transaction by -1BTC,
    // bringing the package feerate to 0.
    m_node.mempool->PrioritiseTransaction(tx_parent->GetHash(), -1 * COIN);
    {
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        const auto submit_cpfp_deprio = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                   package_cpfp, /*test_accept=*/ false);
        BOOST_CHECK_EQUAL(submit_cpfp_deprio.m_state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_MESSAGE(submit_cpfp_deprio.m_state.IsInvalid(),
                            "Package validation unexpectedly succeeded: " << submit_cpfp_deprio.m_state.GetRejectReason());
        BOOST_CHECK(submit_cpfp_deprio.m_tx_results.empty());
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        const CFeeRate expected_feerate(0, GetVirtualTransactionSize(*tx_parent) + GetVirtualTransactionSize(*tx_child));
    }

    // Clear the prioritisation of the parent transaction.
    WITH_LOCK(m_node.mempool->cs, m_node.mempool->ClearPrioritisation(tx_parent->GetHash()));

    // Package CPFP: Even though the parent pays 0 absolute fees, the child pays 1 BTC which is
    // enough for the package feerate to meet the threshold.
    {
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        const auto submit_cpfp = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                   package_cpfp, /*test_accept=*/ false);
        expected_pool_size += 2;
        BOOST_CHECK_MESSAGE(submit_cpfp.m_state.IsValid(),
                            "Package validation unexpectedly failed: " << submit_cpfp.m_state.GetRejectReason());
        BOOST_CHECK_EQUAL(submit_cpfp.m_tx_results.size(), package_cpfp.size());
        auto it_parent = submit_cpfp.m_tx_results.find(tx_parent->GetWitnessHash());
        auto it_child = submit_cpfp.m_tx_results.find(tx_child->GetWitnessHash());
        BOOST_CHECK(it_parent != submit_cpfp.m_tx_results.end());
        BOOST_CHECK(it_parent->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
        BOOST_CHECK(it_parent->second.m_base_fees.value() == 0);
        BOOST_CHECK(it_child != submit_cpfp.m_tx_results.end());
        BOOST_CHECK(it_child->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
        BOOST_CHECK(it_child->second.m_base_fees.value() == COIN);

        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(tx_parent->GetHash())));
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(tx_child->GetHash())));

        const CFeeRate expected_feerate(coinbase_value - child_value,
                                        GetVirtualTransactionSize(*tx_parent) + GetVirtualTransactionSize(*tx_child));
        BOOST_CHECK(it_parent->second.m_effective_feerate.value() == expected_feerate);
        BOOST_CHECK(it_child->second.m_effective_feerate.value() == expected_feerate);
        std::vector<uint256> expected_wtxids({tx_parent->GetWitnessHash(), tx_child->GetWitnessHash()});
        BOOST_CHECK(it_parent->second.m_wtxids_fee_calculations.value() == expected_wtxids);
        BOOST_CHECK(it_child->second.m_wtxids_fee_calculations.value() == expected_wtxids);
        BOOST_CHECK(expected_feerate.GetFeePerK() > 1000);
    }

    // Just because we allow low-fee parents doesn't mean we allow low-feerate packages.
    // This package just pays 200 satoshis total. This would be enough to pay for the child alone,
    // but isn't enough for the entire package to meet the 1sat/vbyte minimum.
    Package package_still_too_low;
    auto mtx_parent_cheap = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[1], /*input_vout=*/0,
                                                          /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                          /*output_destination=*/parent_spk,
                                                          /*output_amount=*/coinbase_value, /*submit=*/false);
    CTransactionRef tx_parent_cheap = MakeTransactionRef(mtx_parent_cheap);
    package_still_too_low.push_back(tx_parent_cheap);

    auto mtx_child_cheap = CreateValidMempoolTransaction(/*input_transaction=*/tx_parent_cheap, /*input_vout=*/0,
                                                         /*input_height=*/101, /*input_signing_key=*/child_key,
                                                         /*output_destination=*/child_spk,
                                                         /*output_amount=*/coinbase_value - 200, /*submit=*/false);
    CTransactionRef tx_child_cheap = MakeTransactionRef(mtx_child_cheap);
    package_still_too_low.push_back(tx_child_cheap);

    // Cheap package should fail with package-fee-too-low.
    {
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        const auto submit_package_too_low = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                   package_still_too_low, /*test_accept=*/false);
        BOOST_CHECK_MESSAGE(submit_package_too_low.m_state.IsInvalid(), "Package validation unexpectedly succeeded");
        BOOST_CHECK_EQUAL(submit_package_too_low.m_state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_EQUAL(submit_package_too_low.m_state.GetRejectReason(), "package-fee-too-low");
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        const CFeeRate child_feerate(200, GetVirtualTransactionSize(*tx_child_cheap));
        BOOST_CHECK(child_feerate.GetFeePerK() > 1000);
        const CFeeRate expected_feerate(200,
            GetVirtualTransactionSize(*tx_parent_cheap) + GetVirtualTransactionSize(*tx_child_cheap));
        BOOST_CHECK(expected_feerate.GetFeePerK() < 1000);
    }

    // Package feerate includes the modified fees of the transactions.
    // This means a child with its fee delta from prioritisetransaction can pay for a parent.
    m_node.mempool->PrioritiseTransaction(tx_child_cheap->GetHash(), 1 * COIN);
    // Now that the child's fees have "increased" by 1 BTC, the cheap package should succeed.
    {
        const auto submit_prioritised_package = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                                  package_still_too_low, /*test_accept=*/false);
        expected_pool_size += 2;
        BOOST_CHECK_MESSAGE(submit_prioritised_package.m_state.IsValid(),
                "Package validation unexpectedly failed" << submit_prioritised_package.m_state.GetRejectReason());
        const CFeeRate expected_feerate(1 * COIN + 200,
            GetVirtualTransactionSize(*tx_parent_cheap) + GetVirtualTransactionSize(*tx_child_cheap));
        BOOST_CHECK_EQUAL(submit_prioritised_package.m_tx_results.size(), package_still_too_low.size());
        auto it_parent = submit_prioritised_package.m_tx_results.find(tx_parent_cheap->GetWitnessHash());
        auto it_child = submit_prioritised_package.m_tx_results.find(tx_child_cheap->GetWitnessHash());
        BOOST_CHECK(it_parent != submit_prioritised_package.m_tx_results.end());
        BOOST_CHECK(it_parent->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
        BOOST_CHECK(it_parent->second.m_base_fees.value() == 0);
        BOOST_CHECK(it_parent->second.m_effective_feerate.value() == expected_feerate);
        BOOST_CHECK(it_child != submit_prioritised_package.m_tx_results.end());
        BOOST_CHECK(it_child->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
        BOOST_CHECK(it_child->second.m_base_fees.value() == 200);
        BOOST_CHECK(it_child->second.m_effective_feerate.value() == expected_feerate);
        std::vector<uint256> expected_wtxids({tx_parent_cheap->GetWitnessHash(), tx_child_cheap->GetWitnessHash()});
        BOOST_CHECK(it_parent->second.m_wtxids_fee_calculations.value() == expected_wtxids);
        BOOST_CHECK(it_child->second.m_wtxids_fee_calculations.value() == expected_wtxids);
    }

    // Package feerate is calculated without topology in mind; it's just aggregating fees and sizes.
    // However, this should not allow parents to pay for children. Each transaction should be
    // validated individually first, eliminating sufficient-feerate parents before they are unfairly
    // included in the package feerate. It's also important that the low-fee child doesn't prevent
    // the parent from being accepted.
    Package package_rich_parent;
    const CAmount high_parent_fee{1 * COIN};
    auto mtx_parent_rich = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[2], /*input_vout=*/0,
                                                         /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                         /*output_destination=*/parent_spk,
                                                         /*output_amount=*/coinbase_value - high_parent_fee, /*submit=*/false);
    CTransactionRef tx_parent_rich = MakeTransactionRef(mtx_parent_rich);
    package_rich_parent.push_back(tx_parent_rich);

    auto mtx_child_poor = CreateValidMempoolTransaction(/*input_transaction=*/tx_parent_rich, /*input_vout=*/0,
                                                        /*input_height=*/101, /*input_signing_key=*/child_key,
                                                        /*output_destination=*/child_spk,
                                                        /*output_amount=*/coinbase_value - high_parent_fee, /*submit=*/false);
    CTransactionRef tx_child_poor = MakeTransactionRef(mtx_child_poor);
    package_rich_parent.push_back(tx_child_poor);

    // Parent pays 1 BTC and child pays none. The parent should be accepted without the child.
    {
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        const auto submit_rich_parent = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                          package_rich_parent, /*test_accept=*/false);
        expected_pool_size += 1;
        BOOST_CHECK_MESSAGE(submit_rich_parent.m_state.IsInvalid(), "Package validation unexpectedly succeeded");

        // The child would have been validated on its own and failed, then submitted as a "package" of 1.
        BOOST_CHECK_EQUAL(submit_rich_parent.m_state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_EQUAL(submit_rich_parent.m_state.GetRejectReason(), "package-fee-too-low");

        auto it_parent = submit_rich_parent.m_tx_results.find(tx_parent_rich->GetWitnessHash());
        BOOST_CHECK(it_parent != submit_rich_parent.m_tx_results.end());
        BOOST_CHECK(it_parent->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
        BOOST_CHECK(it_parent->second.m_state.GetRejectReason() == "");
        BOOST_CHECK_MESSAGE(it_parent->second.m_base_fees.value() == high_parent_fee,
                strprintf("rich parent: expected fee %s, got %s", high_parent_fee, it_parent->second.m_base_fees.value()));
        BOOST_CHECK(it_parent->second.m_effective_feerate == CFeeRate(high_parent_fee, GetVirtualTransactionSize(*tx_parent_rich)));

        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        BOOST_CHECK(m_node.mempool->exists(GenTxid::Txid(tx_parent_rich->GetHash())));
        BOOST_CHECK(!m_node.mempool->exists(GenTxid::Txid(tx_child_poor->GetHash())));
    }
}
BOOST_AUTO_TEST_SUITE_END()
