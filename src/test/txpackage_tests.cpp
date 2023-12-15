// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <key_io.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/random.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <test/util/txmempool.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txpackage_tests)
// A fee amount that is above 1sat/vB but below 5sat/vB for most transactions created within these
// unit tests.
static const CAmount low_fee_amt{200};

// Create placeholder transactions that have no meaning.
inline CTransactionRef create_placeholder_tx(size_t num_inputs, size_t num_outputs)
{
    CMutableTransaction mtx = CMutableTransaction();
    mtx.vin.resize(num_inputs);
    mtx.vout.resize(num_outputs);
    auto random_script = CScript() << ToByteVector(InsecureRand256()) << ToByteVector(InsecureRand256());
    for (size_t i{0}; i < num_inputs; ++i) {
        mtx.vin[i].prevout.hash = Txid::FromUint256(InsecureRand256());
        mtx.vin[i].prevout.n = 0;
        mtx.vin[i].scriptSig = random_script;
    }
    for (size_t o{0}; o < num_outputs; ++o) {
        mtx.vout[o].nValue = 1 * CENT;
        mtx.vout[o].scriptPubKey = random_script;
    }
    return MakeTransactionRef(mtx);
}

// Create a Wtxid from a hex string
inline Wtxid WtxidFromString(std::string_view str)
{
    return Wtxid::FromUint256(uint256S(str.data()));
}

BOOST_FIXTURE_TEST_CASE(package_hash_tests, TestChain100Setup)
{
    // Random real segwit transaction
    DataStream stream_1{
        ParseHex("02000000000101964b8aa63509579ca6086e6012eeaa4c2f4dd1e283da29b67c8eea38b3c6fd220000000000fdffffff0294c618000000000017a9145afbbb42f4e83312666d0697f9e66259912ecde38768fa2c0000000000160014897388a0889390fd0e153a22bb2cf9d8f019faf50247304402200547406380719f84d68cf4e96cc3e4a1688309ef475b150be2b471c70ea562aa02206d255f5acc40fd95981874d77201d2eb07883657ce1c796513f32b6079545cdf0121023ae77335cefcb5ab4c1dc1fb0d2acfece184e593727d7d5906c78e564c7c11d125cf0c00"),
    };
    CTransaction tx_1(deserialize, TX_WITH_WITNESS, stream_1);
    CTransactionRef ptx_1{MakeTransactionRef(tx_1)};

    // Random real nonsegwit transaction
    DataStream stream_2{
        ParseHex("01000000010b26e9b7735eb6aabdf358bab62f9816a21ba9ebdb719d5299e88607d722c190000000008b4830450220070aca44506c5cef3a16ed519d7c3c39f8aab192c4e1c90d065f37b8a4af6141022100a8e160b856c2d43d27d8fba71e5aef6405b8643ac4cb7cb3c462aced7f14711a0141046d11fee51b0e60666d5049a9101a72741df480b96ee26488a4d3466b95c9a40ac5eeef87e10a5cd336c19a84565f80fa6c547957b7700ff4dfbdefe76036c339ffffffff021bff3d11000000001976a91404943fdd508053c75000106d3bc6e2754dbcff1988ac2f15de00000000001976a914a266436d2965547608b9e15d9032a7b9d64fa43188ac00000000"),
    };
    CTransaction tx_2(deserialize, TX_WITH_WITNESS, stream_2);
    CTransactionRef ptx_2{MakeTransactionRef(tx_2)};

    // Random real segwit transaction
    DataStream stream_3{
        ParseHex("0200000000010177862801f77c2c068a70372b4c435ef8dd621291c36a64eb4dd491f02218f5324600000000fdffffff014a0100000000000022512035ea312034cfac01e956a269f3bf147f569c2fbb00180677421262da042290d803402be713325ff285e66b0380f53f2fae0d0fb4e16f378a440fed51ce835061437566729d4883bc917632f3cff474d6384bc8b989961a1d730d4a87ed38ad28bd337b20f1d658c6c138b1c312e072b4446f50f01ae0da03a42e6274f8788aae53416a7fac0063036f7264010118746578742f706c61696e3b636861727365743d7574662d3800357b2270223a226272632d3230222c226f70223a226d696e74222c227469636b223a224342414c222c22616d74223a2236393639227d6821c1f1d658c6c138b1c312e072b4446f50f01ae0da03a42e6274f8788aae53416a7f00000000"),
    };
    CTransaction tx_3(deserialize, TX_WITH_WITNESS, stream_3);
    CTransactionRef ptx_3{MakeTransactionRef(tx_3)};

    // It's easy to see that wtxids are sorted in lexicographical order:
    Wtxid wtxid_1{WtxidFromString("0x85cd1a31eb38f74ed5742ec9cb546712ab5aaf747de28a9168b53e846cbda17f")};
    Wtxid wtxid_2{WtxidFromString("0xb4749f017444b051c44dfd2720e88f314ff94f3dd6d56d40ef65854fcd7fff6b")};
    Wtxid wtxid_3{WtxidFromString("0xe065bac15f62bb4e761d761db928ddee65a47296b2b776785abb912cdec474e3")};
    BOOST_CHECK_EQUAL(tx_1.GetWitnessHash(), wtxid_1);
    BOOST_CHECK_EQUAL(tx_2.GetWitnessHash(), wtxid_2);
    BOOST_CHECK_EQUAL(tx_3.GetWitnessHash(), wtxid_3);

    BOOST_CHECK(wtxid_1.GetHex() < wtxid_2.GetHex());
    BOOST_CHECK(wtxid_2.GetHex() < wtxid_3.GetHex());

    // The txids are not (we want to test that sorting and hashing use wtxid, not txid):
    Txid txid_1{TxidFromString("0xbd0f71c1d5e50589063e134fad22053cdae5ab2320db5bf5e540198b0b5a4e69")};
    Txid txid_2{TxidFromString("0xb4749f017444b051c44dfd2720e88f314ff94f3dd6d56d40ef65854fcd7fff6b")};
    Txid txid_3{TxidFromString("0xee707be5201160e32c4fc715bec227d1aeea5940fb4295605e7373edce3b1a93")};
    BOOST_CHECK_EQUAL(tx_1.GetHash(), txid_1);
    BOOST_CHECK_EQUAL(tx_2.GetHash(), txid_2);
    BOOST_CHECK_EQUAL(tx_3.GetHash(), txid_3);

    BOOST_CHECK(txid_2.GetHex() < txid_1.GetHex());

    BOOST_CHECK(txid_1.ToUint256() != wtxid_1.ToUint256());
    BOOST_CHECK(txid_2.ToUint256() == wtxid_2.ToUint256());
    BOOST_CHECK(txid_3.ToUint256() != wtxid_3.ToUint256());

    // We are testing that both functions compare using GetHex() and not uint256.
    // (in this pair of wtxids, hex string order != uint256 order)
    BOOST_CHECK(wtxid_2 < wtxid_1);
    // (in this pair of wtxids, hex string order == uint256 order)
    BOOST_CHECK(wtxid_2 < wtxid_3);

    // All permutations of the package containing ptx_1, ptx_2, ptx_3 have the same package hash
    std::vector<CTransactionRef> package_123{ptx_1, ptx_2, ptx_3};
    std::vector<CTransactionRef> package_132{ptx_1, ptx_3, ptx_2};
    std::vector<CTransactionRef> package_231{ptx_2, ptx_3, ptx_1};
    std::vector<CTransactionRef> package_213{ptx_2, ptx_1, ptx_3};
    std::vector<CTransactionRef> package_312{ptx_3, ptx_1, ptx_2};
    std::vector<CTransactionRef> package_321{ptx_3, ptx_2, ptx_1};

    uint256 calculated_hash_123 = (HashWriter() << wtxid_1 << wtxid_2 << wtxid_3).GetSHA256();

    uint256 hash_if_by_txid = (HashWriter() << wtxid_2 << wtxid_1 << wtxid_3).GetSHA256();
    BOOST_CHECK(hash_if_by_txid != calculated_hash_123);

    uint256 hash_if_use_txid = (HashWriter() << txid_2 << txid_1 << txid_3).GetSHA256();
    BOOST_CHECK(hash_if_use_txid != calculated_hash_123);

    uint256 hash_if_use_int_order = (HashWriter() << wtxid_2 << wtxid_1 << wtxid_3).GetSHA256();
    BOOST_CHECK(hash_if_use_int_order != calculated_hash_123);

    BOOST_CHECK_EQUAL(calculated_hash_123, GetPackageHash(package_123));
    BOOST_CHECK_EQUAL(calculated_hash_123, GetPackageHash(package_132));
    BOOST_CHECK_EQUAL(calculated_hash_123, GetPackageHash(package_231));
    BOOST_CHECK_EQUAL(calculated_hash_123, GetPackageHash(package_213));
    BOOST_CHECK_EQUAL(calculated_hash_123, GetPackageHash(package_312));
    BOOST_CHECK_EQUAL(calculated_hash_123, GetPackageHash(package_321));
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
    BOOST_CHECK(!IsWellFormedPackage(package_too_many, state_too_many, /*require_sorted=*/true));
    BOOST_CHECK_EQUAL(state_too_many.GetResult(), PackageValidationResult::PCKG_POLICY);
    BOOST_CHECK_EQUAL(state_too_many.GetRejectReason(), "package-too-many-transactions");

    // Packages can't have a total weight of more than 404'000WU.
    CTransactionRef large_ptx = create_placeholder_tx(150, 150);
    Package package_too_large;
    auto size_large = GetTransactionWeight(*large_ptx);
    size_t total_weight{0};
    while (total_weight <= MAX_PACKAGE_WEIGHT) {
        package_too_large.push_back(large_ptx);
        total_weight += size_large;
    }
    BOOST_CHECK(package_too_large.size() <= MAX_PACKAGE_COUNT);
    PackageValidationState state_too_large;
    BOOST_CHECK(!IsWellFormedPackage(package_too_large, state_too_large, /*require_sorted=*/true));
    BOOST_CHECK_EQUAL(state_too_large.GetResult(), PackageValidationResult::PCKG_POLICY);
    BOOST_CHECK_EQUAL(state_too_large.GetRejectReason(), "package-too-large");

    // Packages can't contain transactions with the same txid.
    Package package_duplicate_txids_empty;
    for (auto i{0}; i < 3; ++i) {
        CMutableTransaction empty_tx;
        package_duplicate_txids_empty.emplace_back(MakeTransactionRef(empty_tx));
    }
    PackageValidationState state_duplicates;
    BOOST_CHECK(!IsWellFormedPackage(package_duplicate_txids_empty, state_duplicates, /*require_sorted=*/true));
    BOOST_CHECK_EQUAL(state_duplicates.GetResult(), PackageValidationResult::PCKG_POLICY);
    BOOST_CHECK_EQUAL(state_duplicates.GetRejectReason(), "package-contains-duplicates");
    BOOST_CHECK(!IsConsistentPackage(package_duplicate_txids_empty));

    // Packages can't have transactions spending the same prevout
    CMutableTransaction tx_zero_1;
    CMutableTransaction tx_zero_2;
    COutPoint same_prevout{Txid::FromUint256(InsecureRand256()), 0};
    tx_zero_1.vin.emplace_back(same_prevout);
    tx_zero_2.vin.emplace_back(same_prevout);
    // Different vouts (not the same tx)
    tx_zero_1.vout.emplace_back(CENT, P2WSH_OP_TRUE);
    tx_zero_2.vout.emplace_back(2 * CENT, P2WSH_OP_TRUE);
    Package package_conflicts{MakeTransactionRef(tx_zero_1), MakeTransactionRef(tx_zero_2)};
    BOOST_CHECK(!IsConsistentPackage(package_conflicts));
    // Transactions are considered sorted when they have no dependencies.
    BOOST_CHECK(IsTopoSortedPackage(package_conflicts));
    PackageValidationState state_conflicts;
    BOOST_CHECK(!IsWellFormedPackage(package_conflicts, state_conflicts, /*require_sorted=*/true));
    BOOST_CHECK_EQUAL(state_conflicts.GetResult(), PackageValidationResult::PCKG_POLICY);
    BOOST_CHECK_EQUAL(state_conflicts.GetRejectReason(), "conflict-in-package");

    // IsConsistentPackage only cares about conflicts between transactions, not about a transaction
    // conflicting with itself (i.e. duplicate prevouts in vin).
    CMutableTransaction dup_tx;
    const COutPoint rand_prevout{Txid::FromUint256(InsecureRand256()), 0};
    dup_tx.vin.emplace_back(rand_prevout);
    dup_tx.vin.emplace_back(rand_prevout);
    Package package_with_dup_tx{MakeTransactionRef(dup_tx)};
    BOOST_CHECK(IsConsistentPackage(package_with_dup_tx));
    package_with_dup_tx.emplace_back(create_placeholder_tx(1, 1));
    BOOST_CHECK(IsConsistentPackage(package_with_dup_tx));
}

BOOST_FIXTURE_TEST_CASE(package_validation_tests, TestChain100Setup)
{
    LOCK(cs_main);
    unsigned int initialPoolSize = m_node.mempool->size();

    // Parent and Child Package
    CKey parent_key = GenerateRandomKey();
    CScript parent_locking_script = GetScriptForDestination(PKHash(parent_key.GetPubKey()));
    auto mtx_parent = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[0], /*input_vout=*/0,
                                                    /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                    /*output_destination=*/parent_locking_script,
                                                    /*output_amount=*/CAmount(49 * COIN), /*submit=*/false);
    CTransactionRef tx_parent = MakeTransactionRef(mtx_parent);

    CKey child_key = GenerateRandomKey();
    CScript child_locking_script = GetScriptForDestination(PKHash(child_key.GetPubKey()));
    auto mtx_child = CreateValidMempoolTransaction(/*input_transaction=*/tx_parent, /*input_vout=*/0,
                                                   /*input_height=*/101, /*input_signing_key=*/parent_key,
                                                   /*output_destination=*/child_locking_script,
                                                   /*output_amount=*/CAmount(48 * COIN), /*submit=*/false);
    CTransactionRef tx_child = MakeTransactionRef(mtx_child);
    Package package_parent_child{tx_parent, tx_child};
    const auto result_parent_child = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool, package_parent_child, /*test_accept=*/true, /*client_maxfeerate=*/{});
    if (auto err_parent_child{CheckPackageMempoolAcceptResult(package_parent_child, result_parent_child, /*expect_valid=*/true, nullptr)}) {
        BOOST_ERROR(err_parent_child.value());
    } else {
        auto it_parent = result_parent_child.m_tx_results.find(tx_parent->GetWitnessHash());
        auto it_child = result_parent_child.m_tx_results.find(tx_child->GetWitnessHash());

        BOOST_CHECK(it_parent->second.m_effective_feerate.value().GetFee(GetVirtualTransactionSize(*tx_parent)) == COIN);
        BOOST_CHECK_EQUAL(it_parent->second.m_wtxids_fee_calculations.value().size(), 1);
        BOOST_CHECK_EQUAL(it_parent->second.m_wtxids_fee_calculations.value().front(), tx_parent->GetWitnessHash());

        BOOST_CHECK(it_child->second.m_effective_feerate.value().GetFee(GetVirtualTransactionSize(*tx_child)) == COIN);
        BOOST_CHECK_EQUAL(it_child->second.m_wtxids_fee_calculations.value().size(), 1);
        BOOST_CHECK_EQUAL(it_child->second.m_wtxids_fee_calculations.value().front(), tx_child->GetWitnessHash());
    }
    // A single, giant transaction submitted through ProcessNewPackage fails on single tx policy.
    CTransactionRef giant_ptx = create_placeholder_tx(999, 999);
    BOOST_CHECK(GetVirtualTransactionSize(*giant_ptx) > DEFAULT_ANCESTOR_SIZE_LIMIT_KVB * 1000);
    Package package_single_giant{giant_ptx};
    auto result_single_large = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool, package_single_giant, /*test_accept=*/true, /*client_maxfeerate=*/{});
    if (auto err_single_large{CheckPackageMempoolAcceptResult(package_single_giant, result_single_large, /*expect_valid=*/false, nullptr)}) {
        BOOST_ERROR(err_single_large.value());
    } else {
        BOOST_CHECK_EQUAL(result_single_large.m_state.GetResult(), PackageValidationResult::PCKG_TX);
        BOOST_CHECK_EQUAL(result_single_large.m_state.GetRejectReason(), "transaction failed");
        auto it_giant_tx = result_single_large.m_tx_results.find(giant_ptx->GetWitnessHash());
        BOOST_CHECK_EQUAL(it_giant_tx->second.m_state.GetRejectReason(), "tx-size");
    }

    // Check that mempool size hasn't changed.
    BOOST_CHECK_EQUAL(m_node.mempool->size(), initialPoolSize);
}

BOOST_FIXTURE_TEST_CASE(noncontextual_package_tests, TestChain100Setup)
{
    // The signatures won't be verified so we can just use a placeholder
    CKey placeholder_key = GenerateRandomKey();
    CScript spk = GetScriptForDestination(PKHash(placeholder_key.GetPubKey()));
    CKey placeholder_key_2 = GenerateRandomKey();
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
        BOOST_CHECK(IsWellFormedPackage({tx_parent, tx_child}, state, /*require_sorted=*/true));
        BOOST_CHECK(!IsWellFormedPackage({tx_child, tx_parent}, state, /*require_sorted=*/true));
        BOOST_CHECK_EQUAL(state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "package-not-sorted");
        BOOST_CHECK(IsChildWithParents({tx_parent, tx_child}));
        BOOST_CHECK(IsChildWithParentsTree({tx_parent, tx_child}));
        BOOST_CHECK(GetPackageHash({tx_parent}) != GetPackageHash({tx_child}));
        BOOST_CHECK(GetPackageHash({tx_child, tx_child}) != GetPackageHash({tx_child}));
        BOOST_CHECK(GetPackageHash({tx_child, tx_parent}) != GetPackageHash({tx_child, tx_child}));
    }

    // 24 Parents and 1 Child
    {
        Package package;
        CMutableTransaction child;
        for (int i{0}; i < 24; ++i) {
            auto parent = MakeTransactionRef(CreateValidMempoolTransaction(m_coinbase_txns[i + 1],
                                             0, 0, coinbaseKey, spk, CAmount(48 * COIN), false));
            package.emplace_back(parent);
            child.vin.emplace_back(COutPoint(parent->GetHash(), 0));
        }
        child.vout.emplace_back(47 * COIN, spk2);

        // The child must be in the package.
        BOOST_CHECK(!IsChildWithParents(package));

        // The parents can be in any order.
        FastRandomContext rng;
        Shuffle(package.begin(), package.end(), rng);
        package.push_back(MakeTransactionRef(child));

        PackageValidationState state;
        BOOST_CHECK(IsWellFormedPackage(package, state, /*require_sorted=*/true));
        BOOST_CHECK(IsChildWithParents(package));
        BOOST_CHECK(IsChildWithParentsTree(package));

        package.erase(package.begin());
        BOOST_CHECK(IsChildWithParents(package));

        // The package cannot have unrelated transactions.
        package.insert(package.begin(), m_coinbase_txns[0]);
        BOOST_CHECK(!IsChildWithParents(package));
    }

    // 2 Parents and 1 Child where one parent depends on the other.
    {
        CMutableTransaction mtx_parent;
        mtx_parent.vin.emplace_back(COutPoint(m_coinbase_txns[0]->GetHash(), 0));
        mtx_parent.vout.emplace_back(20 * COIN, spk);
        mtx_parent.vout.emplace_back(20 * COIN, spk2);
        CTransactionRef tx_parent = MakeTransactionRef(mtx_parent);

        CMutableTransaction mtx_parent_also_child;
        mtx_parent_also_child.vin.emplace_back(COutPoint(tx_parent->GetHash(), 0));
        mtx_parent_also_child.vout.emplace_back(20 * COIN, spk);
        CTransactionRef tx_parent_also_child = MakeTransactionRef(mtx_parent_also_child);

        CMutableTransaction mtx_child;
        mtx_child.vin.emplace_back(COutPoint(tx_parent->GetHash(), 1));
        mtx_child.vin.emplace_back(COutPoint(tx_parent_also_child->GetHash(), 0));
        mtx_child.vout.emplace_back(39 * COIN, spk);
        CTransactionRef tx_child = MakeTransactionRef(mtx_child);

        PackageValidationState state;
        BOOST_CHECK(IsChildWithParents({tx_parent, tx_parent_also_child}));
        BOOST_CHECK(IsChildWithParents({tx_parent, tx_child}));
        BOOST_CHECK(IsChildWithParents({tx_parent, tx_parent_also_child, tx_child}));
        BOOST_CHECK(!IsChildWithParentsTree({tx_parent, tx_parent_also_child, tx_child}));
        // IsChildWithParents does not detect unsorted parents.
        BOOST_CHECK(IsChildWithParents({tx_parent_also_child, tx_parent, tx_child}));
        BOOST_CHECK(IsWellFormedPackage({tx_parent, tx_parent_also_child, tx_child}, state, /*require_sorted=*/true));
        BOOST_CHECK(!IsWellFormedPackage({tx_parent_also_child, tx_parent, tx_child}, state, /*require_sorted=*/true));
        BOOST_CHECK_EQUAL(state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "package-not-sorted");
    }
}

BOOST_FIXTURE_TEST_CASE(package_submission_tests, TestChain100Setup)
{
    LOCK(cs_main);
    unsigned int expected_pool_size = m_node.mempool->size();
    CKey parent_key = GenerateRandomKey();
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
                                                     package_unrelated, /*test_accept=*/false, /*client_maxfeerate=*/{});
    // We don't expect m_tx_results for each transaction when basic sanity checks haven't passed.
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

    CKey child_key = GenerateRandomKey();
    CScript child_locking_script = GetScriptForDestination(PKHash(child_key.GetPubKey()));
    auto mtx_child = CreateValidMempoolTransaction(/*input_transaction=*/tx_parent, /*input_vout=*/0,
                                                   /*input_height=*/101, /*input_signing_key=*/parent_key,
                                                   /*output_destination=*/child_locking_script,
                                                   /*output_amount=*/CAmount(48 * COIN), /*submit=*/false);
    CTransactionRef tx_child = MakeTransactionRef(mtx_child);
    package_parent_child.push_back(tx_child);
    package_3gen.push_back(tx_child);

    CKey grandchild_key = GenerateRandomKey();
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
                                                    package_3gen, /*test_accept=*/false, /*client_maxfeerate=*/{});
        BOOST_CHECK(result_3gen_submit.m_state.IsInvalid());
        BOOST_CHECK_EQUAL(result_3gen_submit.m_state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_EQUAL(result_3gen_submit.m_state.GetRejectReason(), "package-not-child-with-parents");
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
    }

    // Parent and child package where transactions are invalid for reasons other than fee and
    // missing inputs, so the package validation isn't expected to happen.
    {
        CScriptWitness bad_witness;
        bad_witness.stack.emplace_back(1);
        CMutableTransaction mtx_parent_invalid{mtx_parent};
        mtx_parent_invalid.vin[0].scriptWitness = bad_witness;
        CTransactionRef tx_parent_invalid = MakeTransactionRef(mtx_parent_invalid);
        Package package_invalid_parent{tx_parent_invalid, tx_child};
        auto result_quit_early = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                   package_invalid_parent, /*test_accept=*/ false, /*client_maxfeerate=*/{});
        if (auto err_parent_invalid{CheckPackageMempoolAcceptResult(package_invalid_parent, result_quit_early, /*expect_valid=*/false, m_node.mempool.get())}) {
            BOOST_ERROR(err_parent_invalid.value());
        } else {
            auto it_parent = result_quit_early.m_tx_results.find(tx_parent_invalid->GetWitnessHash());
            auto it_child = result_quit_early.m_tx_results.find(tx_child->GetWitnessHash());
            BOOST_CHECK_EQUAL(it_parent->second.m_state.GetResult(), TxValidationResult::TX_WITNESS_MUTATED);
            BOOST_CHECK_EQUAL(it_parent->second.m_state.GetRejectReason(), "bad-witness-nonstandard");
            BOOST_CHECK_EQUAL(it_child->second.m_state.GetResult(), TxValidationResult::TX_MISSING_INPUTS);
            BOOST_CHECK_EQUAL(it_child->second.m_state.GetRejectReason(), "bad-txns-inputs-missingorspent");
        }
        BOOST_CHECK_EQUAL(result_quit_early.m_state.GetResult(), PackageValidationResult::PCKG_TX);
    }

    // Child with missing parent.
    mtx_child.vin.emplace_back(COutPoint(package_unrelated[0]->GetHash(), 0));
    Package package_missing_parent;
    package_missing_parent.push_back(tx_parent);
    package_missing_parent.push_back(MakeTransactionRef(mtx_child));
    {
        const auto result_missing_parent = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                             package_missing_parent, /*test_accept=*/false, /*client_maxfeerate=*/{});
        BOOST_CHECK(result_missing_parent.m_state.IsInvalid());
        BOOST_CHECK_EQUAL(result_missing_parent.m_state.GetResult(), PackageValidationResult::PCKG_POLICY);
        BOOST_CHECK_EQUAL(result_missing_parent.m_state.GetRejectReason(), "package-not-child-with-unconfirmed-parents");
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
    }

    // Submit package with parent + child.
    {
        const auto submit_parent_child = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                           package_parent_child, /*test_accept=*/false, /*client_maxfeerate=*/{});
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
        BOOST_CHECK(it_child->second.m_effective_feerate == CFeeRate(1 * COIN, GetVirtualTransactionSize(*tx_child)));
        BOOST_CHECK_EQUAL(it_child->second.m_wtxids_fee_calculations.value().size(), 1);
        BOOST_CHECK_EQUAL(it_child->second.m_wtxids_fee_calculations.value().front(), tx_child->GetWitnessHash());

        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
    }

    // Already-in-mempool transactions should be detected and de-duplicated.
    {
        const auto submit_deduped = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                      package_parent_child, /*test_accept=*/false, /*client_maxfeerate=*/{});
        if (auto err_deduped{CheckPackageMempoolAcceptResult(package_parent_child, submit_deduped, /*expect_valid=*/true, m_node.mempool.get())}) {
            BOOST_ERROR(err_deduped.value());
        } else {
            auto it_parent_deduped = submit_deduped.m_tx_results.find(tx_parent->GetWitnessHash());
            auto it_child_deduped = submit_deduped.m_tx_results.find(tx_child->GetWitnessHash());
            BOOST_CHECK(it_parent_deduped->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
            BOOST_CHECK(it_child_deduped->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
        }

        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
    }
}

// Tests for packages containing transactions that have same-txid-different-witness equivalents in
// the mempool.
BOOST_FIXTURE_TEST_CASE(package_witness_swap_tests, TestChain100Setup)
{
    // Mine blocks to mature coinbases.
    mineBlocks(5);
    MockMempoolMinFee(CFeeRate(5000));
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
    witness1.stack.emplace_back(1);
    witness1.stack.emplace_back(witnessScript.begin(), witnessScript.end());

    CScriptWitness witness2(witness1);
    witness2.stack.emplace_back(2);
    witness2.stack.emplace_back(witnessScript.begin(), witnessScript.end());

    CKey child_key = GenerateRandomKey();
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
    // Check that they have different package hashes
    BOOST_CHECK(GetPackageHash({ptx_parent, ptx_child1}) != GetPackageHash({ptx_parent, ptx_child2}));

    // Try submitting Package1{parent, child1} and Package2{parent, child2} where the children are
    // same-txid-different-witness.
    {
        Package package_parent_child1{ptx_parent, ptx_child1};
        const auto submit_witness1 = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                       package_parent_child1, /*test_accept=*/false, /*client_maxfeerate=*/{});
        if (auto err_witness1{CheckPackageMempoolAcceptResult(package_parent_child1, submit_witness1, /*expect_valid=*/true, m_node.mempool.get())}) {
            BOOST_ERROR(err_witness1.value());
        }

        // Child2 would have been validated individually.
        Package package_parent_child2{ptx_parent, ptx_child2};
        const auto submit_witness2 = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                       package_parent_child2, /*test_accept=*/false, /*client_maxfeerate=*/{});
        if (auto err_witness2{CheckPackageMempoolAcceptResult(package_parent_child2, submit_witness2, /*expect_valid=*/true, m_node.mempool.get())}) {
            BOOST_ERROR(err_witness2.value());
        } else {
            auto it_parent2_deduped = submit_witness2.m_tx_results.find(ptx_parent->GetWitnessHash());
            auto it_child2 = submit_witness2.m_tx_results.find(ptx_child2->GetWitnessHash());
            BOOST_CHECK(it_parent2_deduped->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
            BOOST_CHECK(it_child2->second.m_result_type == MempoolAcceptResult::ResultType::DIFFERENT_WITNESS);
            BOOST_CHECK_EQUAL(ptx_child1->GetWitnessHash(), it_child2->second.m_other_wtxid.value());
        }

        // Deduplication should work when wtxid != txid. Submit package with the already-in-mempool
        // transactions again, which should not fail.
        const auto submit_segwit_dedup = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                           package_parent_child1, /*test_accept=*/false, /*client_maxfeerate=*/{});
        if (auto err_segwit_dedup{CheckPackageMempoolAcceptResult(package_parent_child1, submit_segwit_dedup, /*expect_valid=*/true, m_node.mempool.get())}) {
            BOOST_ERROR(err_segwit_dedup.value());
        } else {
            auto it_parent_dup = submit_segwit_dedup.m_tx_results.find(ptx_parent->GetWitnessHash());
            auto it_child_dup = submit_segwit_dedup.m_tx_results.find(ptx_child1->GetWitnessHash());
            BOOST_CHECK(it_parent_dup->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
            BOOST_CHECK(it_child_dup->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
        }
    }

    // Try submitting Package1{child2, grandchild} where child2 is same-txid-different-witness as
    // the in-mempool transaction, child1. Since child1 exists in the mempool and its outputs are
    // available, child2 should be ignored and grandchild should be accepted.
    //
    // This tests a potential censorship vector in which an attacker broadcasts a competing package
    // where a parent's witness is mutated. The honest package should be accepted despite the fact
    // that we don't allow witness replacement.
    CKey grandchild_key = GenerateRandomKey();
    CScript grandchild_locking_script = GetScriptForDestination(WitnessV0KeyHash(grandchild_key.GetPubKey()));
    auto mtx_grandchild = CreateValidMempoolTransaction(/*input_transaction=*/ptx_child2, /*input_vout=*/0,
                                                        /*input_height=*/0, /*input_signing_key=*/child_key,
                                                        /*output_destination=*/grandchild_locking_script,
                                                        /*output_amount=*/CAmount(47 * COIN), /*submit=*/false);
    CTransactionRef ptx_grandchild = MakeTransactionRef(mtx_grandchild);
    // Check that they have different package hashes
    BOOST_CHECK(GetPackageHash({ptx_child1, ptx_grandchild}) != GetPackageHash({ptx_child2, ptx_grandchild}));
    // We already submitted child1 above.
    {
        Package package_child2_grandchild{ptx_child2, ptx_grandchild};
        const auto submit_spend_ignored = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                            package_child2_grandchild, /*test_accept=*/false, /*client_maxfeerate=*/{});
        if (auto err_spend_ignored{CheckPackageMempoolAcceptResult(package_child2_grandchild, submit_spend_ignored, /*expect_valid=*/true, m_node.mempool.get())}) {
            BOOST_ERROR(err_spend_ignored.value());
        } else {
            auto it_child2_ignored = submit_spend_ignored.m_tx_results.find(ptx_child2->GetWitnessHash());
            auto it_grandchild = submit_spend_ignored.m_tx_results.find(ptx_grandchild->GetWitnessHash());
            BOOST_CHECK(it_child2_ignored->second.m_result_type == MempoolAcceptResult::ResultType::DIFFERENT_WITNESS);
            BOOST_CHECK(it_grandchild->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
        }
    }

    // A package Package{parent1, parent2, parent3, child} where the parents are a mixture of
    // identical-tx-in-mempool, same-txid-different-witness-in-mempool, and new transactions.
    Package package_mixed;

    // Give all the parents anyone-can-spend scripts so we don't have to deal with signing the child.
    CScript acs_script = CScript() << OP_TRUE;
    CScript acs_spk = GetScriptForDestination(WitnessV0ScriptHash(acs_script));
    CScriptWitness acs_witness;
    acs_witness.stack.emplace_back(acs_script.begin(), acs_script.end());

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
    parent2_witness1.stack.emplace_back(1);
    parent2_witness1.stack.emplace_back(grandparent2_script.begin(), grandparent2_script.end());
    CScriptWitness parent2_witness2;
    parent2_witness2.stack.emplace_back(2);
    parent2_witness2.stack.emplace_back(grandparent2_script.begin(), grandparent2_script.end());

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

    // parent3 will be a new transaction. Put a low feerate to make it invalid on its own.
    auto mtx_parent3 = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[3], /*input_vout=*/0,
                                                     /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                     /*output_destination=*/acs_spk,
                                                     /*output_amount=*/CAmount(50 * COIN - low_fee_amt), /*submit=*/false);
    CTransactionRef ptx_parent3 = MakeTransactionRef(mtx_parent3);
    package_mixed.push_back(ptx_parent3);
    BOOST_CHECK(m_node.mempool->GetMinFee().GetFee(GetVirtualTransactionSize(*ptx_parent3)) > low_fee_amt);
    BOOST_CHECK(m_node.mempool->m_opts.min_relay_feerate.GetFee(GetVirtualTransactionSize(*ptx_parent3)) <= low_fee_amt);

    // child spends parent1, parent2, and parent3
    CKey mixed_grandchild_key = GenerateRandomKey();
    CScript mixed_child_spk = GetScriptForDestination(WitnessV0KeyHash(mixed_grandchild_key.GetPubKey()));

    CMutableTransaction mtx_mixed_child;
    mtx_mixed_child.vin.emplace_back(COutPoint(ptx_parent1->GetHash(), 0));
    mtx_mixed_child.vin.emplace_back(COutPoint(ptx_parent2_v1->GetHash(), 0));
    mtx_mixed_child.vin.emplace_back(COutPoint(ptx_parent3->GetHash(), 0));
    mtx_mixed_child.vin[0].scriptWitness = acs_witness;
    mtx_mixed_child.vin[1].scriptWitness = acs_witness;
    mtx_mixed_child.vin[2].scriptWitness = acs_witness;
    mtx_mixed_child.vout.emplace_back((48 + 49 + 50 - 1) * COIN, mixed_child_spk);
    CTransactionRef ptx_mixed_child = MakeTransactionRef(mtx_mixed_child);
    package_mixed.push_back(ptx_mixed_child);

    // Submit package:
    // parent1 should be ignored
    // parent2_v1 should be ignored (and v2 wtxid returned)
    // parent3 should be accepted
    // child should be accepted
    {
        const auto mixed_result = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool, package_mixed, false, /*client_maxfeerate=*/{});
        if (auto err_mixed{CheckPackageMempoolAcceptResult(package_mixed, mixed_result, /*expect_valid=*/true, m_node.mempool.get())}) {
            BOOST_ERROR(err_mixed.value());
        } else {
            auto it_parent1 = mixed_result.m_tx_results.find(ptx_parent1->GetWitnessHash());
            auto it_parent2 = mixed_result.m_tx_results.find(ptx_parent2_v1->GetWitnessHash());
            auto it_parent3 = mixed_result.m_tx_results.find(ptx_parent3->GetWitnessHash());
            auto it_child = mixed_result.m_tx_results.find(ptx_mixed_child->GetWitnessHash());

            BOOST_CHECK(it_parent1->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
            BOOST_CHECK(it_parent2->second.m_result_type == MempoolAcceptResult::ResultType::DIFFERENT_WITNESS);
            BOOST_CHECK(it_parent3->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
            BOOST_CHECK(it_child->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
            BOOST_CHECK_EQUAL(ptx_parent2_v2->GetWitnessHash(), it_parent2->second.m_other_wtxid.value());

            // package feerate should include parent3 and child. It should not include parent1 or parent2_v1.
            const CFeeRate expected_feerate(1 * COIN, GetVirtualTransactionSize(*ptx_parent3) + GetVirtualTransactionSize(*ptx_mixed_child));
            BOOST_CHECK(it_parent3->second.m_effective_feerate.value() == expected_feerate);
            BOOST_CHECK(it_child->second.m_effective_feerate.value() == expected_feerate);
            std::vector<Wtxid> expected_wtxids({ptx_parent3->GetWitnessHash(), ptx_mixed_child->GetWitnessHash()});
            BOOST_CHECK(it_parent3->second.m_wtxids_fee_calculations.value() == expected_wtxids);
            BOOST_CHECK(it_child->second.m_wtxids_fee_calculations.value() == expected_wtxids);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(package_cpfp_tests, TestChain100Setup)
{
    mineBlocks(5);
    MockMempoolMinFee(CFeeRate(5000));
    LOCK(::cs_main);
    size_t expected_pool_size = m_node.mempool->size();
    CKey child_key = GenerateRandomKey();
    CScript parent_spk = GetScriptForDestination(WitnessV0KeyHash(child_key.GetPubKey()));
    CKey grandchild_key = GenerateRandomKey();
    CScript child_spk = GetScriptForDestination(WitnessV0KeyHash(grandchild_key.GetPubKey()));

    // low-fee parent and high-fee child package
    const CAmount coinbase_value{50 * COIN};
    const CAmount parent_value{coinbase_value - low_fee_amt};
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
    // fee deltas. This should be taken into account. De-prioritise the parent transaction
    // to bring the package feerate to 0.
    m_node.mempool->PrioritiseTransaction(tx_parent->GetHash(), child_value - coinbase_value);
    {
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        const auto submit_cpfp_deprio = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                   package_cpfp, /*test_accept=*/ false, /*client_maxfeerate=*/{});
        if (auto err_cpfp_deprio{CheckPackageMempoolAcceptResult(package_cpfp, submit_cpfp_deprio, /*expect_valid=*/false, m_node.mempool.get())}) {
            BOOST_ERROR(err_cpfp_deprio.value());
        } else {
            BOOST_CHECK_EQUAL(submit_cpfp_deprio.m_state.GetResult(), PackageValidationResult::PCKG_TX);
            BOOST_CHECK_EQUAL(submit_cpfp_deprio.m_tx_results.find(tx_parent->GetWitnessHash())->second.m_state.GetResult(),
                              TxValidationResult::TX_MEMPOOL_POLICY);
            BOOST_CHECK_EQUAL(submit_cpfp_deprio.m_tx_results.find(tx_child->GetWitnessHash())->second.m_state.GetResult(),
                              TxValidationResult::TX_MISSING_INPUTS);
            BOOST_CHECK(submit_cpfp_deprio.m_tx_results.find(tx_parent->GetWitnessHash())->second.m_state.GetRejectReason() == "min relay fee not met");
            BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        }
    }

    // Clear the prioritisation of the parent transaction.
    WITH_LOCK(m_node.mempool->cs, m_node.mempool->ClearPrioritisation(tx_parent->GetHash()));

    // Package CPFP: Even though the parent's feerate is below the mempool minimum feerate, the
    // child pays enough for the package feerate to meet the threshold.
    {
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
        const auto submit_cpfp = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                   package_cpfp, /*test_accept=*/ false, /*client_maxfeerate=*/{});
        if (auto err_cpfp{CheckPackageMempoolAcceptResult(package_cpfp, submit_cpfp, /*expect_valid=*/true, m_node.mempool.get())}) {
            BOOST_ERROR(err_cpfp.value());
        } else {
            auto it_parent = submit_cpfp.m_tx_results.find(tx_parent->GetWitnessHash());
            auto it_child = submit_cpfp.m_tx_results.find(tx_child->GetWitnessHash());
            BOOST_CHECK(it_parent->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
            BOOST_CHECK(it_parent->second.m_base_fees.value() == coinbase_value - parent_value);
            BOOST_CHECK(it_child->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
            BOOST_CHECK(it_child->second.m_base_fees.value() == COIN);

            const CFeeRate expected_feerate(coinbase_value - child_value,
                                            GetVirtualTransactionSize(*tx_parent) + GetVirtualTransactionSize(*tx_child));
            BOOST_CHECK(it_parent->second.m_effective_feerate.value() == expected_feerate);
            BOOST_CHECK(it_child->second.m_effective_feerate.value() == expected_feerate);
            std::vector<Wtxid> expected_wtxids({tx_parent->GetWitnessHash(), tx_child->GetWitnessHash()});
            BOOST_CHECK(it_parent->second.m_wtxids_fee_calculations.value() == expected_wtxids);
            BOOST_CHECK(it_child->second.m_wtxids_fee_calculations.value() == expected_wtxids);
            BOOST_CHECK(expected_feerate.GetFeePerK() > 1000);
        }
        expected_pool_size += 2;
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
    }

    // Just because we allow low-fee parents doesn't mean we allow low-feerate packages.
    // The mempool minimum feerate is 5sat/vB, but this package just pays 800 satoshis total.
    // The child fees would be able to pay for itself, but isn't enough for the entire package.
    Package package_still_too_low;
    const CAmount parent_fee{200};
    const CAmount child_fee{600};
    auto mtx_parent_cheap = CreateValidMempoolTransaction(/*input_transaction=*/m_coinbase_txns[1], /*input_vout=*/0,
                                                          /*input_height=*/0, /*input_signing_key=*/coinbaseKey,
                                                          /*output_destination=*/parent_spk,
                                                          /*output_amount=*/coinbase_value - parent_fee, /*submit=*/false);
    CTransactionRef tx_parent_cheap = MakeTransactionRef(mtx_parent_cheap);
    package_still_too_low.push_back(tx_parent_cheap);
    BOOST_CHECK(m_node.mempool->GetMinFee().GetFee(GetVirtualTransactionSize(*tx_parent_cheap)) > parent_fee);
    BOOST_CHECK(m_node.mempool->m_opts.min_relay_feerate.GetFee(GetVirtualTransactionSize(*tx_parent_cheap)) <= parent_fee);

    auto mtx_child_cheap = CreateValidMempoolTransaction(/*input_transaction=*/tx_parent_cheap, /*input_vout=*/0,
                                                         /*input_height=*/101, /*input_signing_key=*/child_key,
                                                         /*output_destination=*/child_spk,
                                                         /*output_amount=*/coinbase_value - parent_fee - child_fee, /*submit=*/false);
    CTransactionRef tx_child_cheap = MakeTransactionRef(mtx_child_cheap);
    package_still_too_low.push_back(tx_child_cheap);
    BOOST_CHECK(m_node.mempool->GetMinFee().GetFee(GetVirtualTransactionSize(*tx_child_cheap)) <= child_fee);
    BOOST_CHECK(m_node.mempool->GetMinFee().GetFee(GetVirtualTransactionSize(*tx_parent_cheap) + GetVirtualTransactionSize(*tx_child_cheap)) > parent_fee + child_fee);
    BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);

    // Cheap package should fail for being too low fee.
    {
        const auto submit_package_too_low = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                   package_still_too_low, /*test_accept=*/false, /*client_maxfeerate=*/{});
        if (auto err_package_too_low{CheckPackageMempoolAcceptResult(package_still_too_low, submit_package_too_low, /*expect_valid=*/false, m_node.mempool.get())}) {
            BOOST_ERROR(err_package_too_low.value());
        } else {
            // Individual feerate of parent is too low.
            BOOST_CHECK_EQUAL(submit_package_too_low.m_tx_results.at(tx_parent_cheap->GetWitnessHash()).m_state.GetResult(),
                              TxValidationResult::TX_RECONSIDERABLE);
            BOOST_CHECK(submit_package_too_low.m_tx_results.at(tx_parent_cheap->GetWitnessHash()).m_effective_feerate.value() ==
                        CFeeRate(parent_fee, GetVirtualTransactionSize(*tx_parent_cheap)));
            // Package feerate of parent + child is too low.
            BOOST_CHECK_EQUAL(submit_package_too_low.m_tx_results.at(tx_child_cheap->GetWitnessHash()).m_state.GetResult(),
                              TxValidationResult::TX_RECONSIDERABLE);
            BOOST_CHECK(submit_package_too_low.m_tx_results.at(tx_child_cheap->GetWitnessHash()).m_effective_feerate.value() ==
                        CFeeRate(parent_fee + child_fee, GetVirtualTransactionSize(*tx_parent_cheap) + GetVirtualTransactionSize(*tx_child_cheap)));
        }
        BOOST_CHECK_EQUAL(submit_package_too_low.m_state.GetResult(), PackageValidationResult::PCKG_TX);
        BOOST_CHECK_EQUAL(submit_package_too_low.m_state.GetRejectReason(), "transaction failed");
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
    }

    // Package feerate includes the modified fees of the transactions.
    // This means a child with its fee delta from prioritisetransaction can pay for a parent.
    m_node.mempool->PrioritiseTransaction(tx_child_cheap->GetHash(), 1 * COIN);
    // Now that the child's fees have "increased" by 1 BTC, the cheap package should succeed.
    {
        const auto submit_prioritised_package = ProcessNewPackage(m_node.chainman->ActiveChainstate(), *m_node.mempool,
                                                                  package_still_too_low, /*test_accept=*/false, /*client_maxfeerate=*/{});
        if (auto err_prioritised{CheckPackageMempoolAcceptResult(package_still_too_low, submit_prioritised_package, /*expect_valid=*/true, m_node.mempool.get())}) {
            BOOST_ERROR(err_prioritised.value());
        } else {
            const CFeeRate expected_feerate(1 * COIN + parent_fee + child_fee,
                GetVirtualTransactionSize(*tx_parent_cheap) + GetVirtualTransactionSize(*tx_child_cheap));
            BOOST_CHECK_EQUAL(submit_prioritised_package.m_tx_results.size(), package_still_too_low.size());
            auto it_parent = submit_prioritised_package.m_tx_results.find(tx_parent_cheap->GetWitnessHash());
            auto it_child = submit_prioritised_package.m_tx_results.find(tx_child_cheap->GetWitnessHash());
            BOOST_CHECK(it_parent->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
            BOOST_CHECK(it_parent->second.m_base_fees.value() == parent_fee);
            BOOST_CHECK(it_parent->second.m_effective_feerate.value() == expected_feerate);
            BOOST_CHECK(it_child->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
            BOOST_CHECK(it_child->second.m_base_fees.value() == child_fee);
            BOOST_CHECK(it_child->second.m_effective_feerate.value() == expected_feerate);
            std::vector<Wtxid> expected_wtxids({tx_parent_cheap->GetWitnessHash(), tx_child_cheap->GetWitnessHash()});
            BOOST_CHECK(it_parent->second.m_wtxids_fee_calculations.value() == expected_wtxids);
            BOOST_CHECK(it_child->second.m_wtxids_fee_calculations.value() == expected_wtxids);
        }
        expected_pool_size += 2;
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
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
                                                          package_rich_parent, /*test_accept=*/false, /*client_maxfeerate=*/{});
        if (auto err_rich_parent{CheckPackageMempoolAcceptResult(package_rich_parent, submit_rich_parent, /*expect_valid=*/false, m_node.mempool.get())}) {
            BOOST_ERROR(err_rich_parent.value());
        } else {
            // The child would have been validated on its own and failed.
            BOOST_CHECK_EQUAL(submit_rich_parent.m_state.GetResult(), PackageValidationResult::PCKG_TX);
            BOOST_CHECK_EQUAL(submit_rich_parent.m_state.GetRejectReason(), "transaction failed");

            auto it_parent = submit_rich_parent.m_tx_results.find(tx_parent_rich->GetWitnessHash());
            auto it_child = submit_rich_parent.m_tx_results.find(tx_child_poor->GetWitnessHash());
            BOOST_CHECK(it_parent->second.m_result_type == MempoolAcceptResult::ResultType::VALID);
            BOOST_CHECK(it_child->second.m_result_type == MempoolAcceptResult::ResultType::INVALID);
            BOOST_CHECK(it_parent->second.m_state.GetRejectReason() == "");
            BOOST_CHECK_MESSAGE(it_parent->second.m_base_fees.value() == high_parent_fee,
                    strprintf("rich parent: expected fee %s, got %s", high_parent_fee, it_parent->second.m_base_fees.value()));
            BOOST_CHECK(it_parent->second.m_effective_feerate == CFeeRate(high_parent_fee, GetVirtualTransactionSize(*tx_parent_rich)));
            BOOST_CHECK_EQUAL(it_child->second.m_result_type, MempoolAcceptResult::ResultType::INVALID);
            BOOST_CHECK_EQUAL(it_child->second.m_state.GetResult(), TxValidationResult::TX_MEMPOOL_POLICY);
            BOOST_CHECK(it_child->second.m_state.GetRejectReason() == "min relay fee not met");
        }
        expected_pool_size += 1;
        BOOST_CHECK_EQUAL(m_node.mempool->size(), expected_pool_size);
    }
}
BOOST_AUTO_TEST_SUITE_END()
