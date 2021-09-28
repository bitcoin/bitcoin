// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <policy/policy.h>
#include <policy/rbf.h>
#include <txmempool.h>
#include <util/system.h>
#include <util/time.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <optional>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(rbf_tests, TestingSetup)

inline CTransactionRef make_tx(std::vector<CAmount>&& output_values,
                               std::vector<CTransactionRef>&& inputs=std::vector<CTransactionRef>(),
                               std::vector<uint32_t>&& input_indices=std::vector<uint32_t>())
{
    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(inputs.size());
    tx.vout.resize(output_values.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
        tx.vin[i].prevout.hash = inputs[i]->GetHash();
        tx.vin[i].prevout.n = input_indices.size() > i ? input_indices[i] : 0;
    }
    for (size_t i = 0; i < output_values.size(); ++i) {
        tx.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx.vout[i].nValue = output_values[i];
    }
    return MakeTransactionRef(tx);
}

BOOST_AUTO_TEST_CASE(AncestorScoreTests)
{
    CTxMemPool pool;
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    const CAmount low_fee{100};
    const CAmount normal_fee{10000};
    const CAmount high_fee{1 * COIN};

    // Create a parent and child with "fees" manually set to normal:
    // [tx1].0 <- [tx2].0
    //
    CTransactionRef tx1 = make_tx(/* output_values */ {10 * COIN});
    pool.addUnchecked(entry.Fee(normal_fee).FromTx(tx1));
    CTransactionRef tx2 = make_tx(/* output_values */ {995 * CENT}, /* inputs */ {tx1});
    pool.addUnchecked(entry.Fee(normal_fee).FromTx(tx2));
    CTxMemPool::setEntries normal_normal = pool.GetIterSet({tx1->GetHash(), tx2->GetHash()});

    /* Create a parent and child where the child has a very high "fee". */
    //
    // [tx3] <- [tx4]
    //
    CTransactionRef tx3 = make_tx(/* output_values */ {1099 * CENT});
    pool.addUnchecked(entry.Fee(low_fee).FromTx(tx3));
    CTransactionRef tx4 = make_tx(/* output_values */ {999 * CENT}, /* inputs */ {tx3});
    pool.addUnchecked(entry.Fee(high_fee).FromTx(tx4));
    CTxMemPool::setEntries cpfp = pool.GetIterSet({tx3->GetHash(), tx4->GetHash()});

    /* Create a parent and child where both have very low "fees". */
    //
    // [tx5] <- [tx6]
    //
    CTransactionRef tx5 = make_tx(/* output_values */ {1099 * CENT});
    pool.addUnchecked(entry.Fee(low_fee).FromTx(tx5));
    CTransactionRef tx6 = make_tx(/* output_values */ {1098 * CENT}, /* inputs */ {tx3});
    pool.addUnchecked(entry.Fee(low_fee).FromTx(tx6));
    CTxMemPool::setEntries low_low = pool.GetIterSet({tx5->GetHash(), tx6->GetHash()});

    const auto tx1_fee = pool.GetIter(tx1->GetHash()).value()->GetFee();
    const auto tx2_fee = pool.GetIter(tx2->GetHash()).value()->GetFee();
    const auto tx3_fee = pool.GetIter(tx3->GetHash()).value()->GetFee();
    const auto tx4_fee = pool.GetIter(tx4->GetHash()).value()->GetFee();
    const auto tx5_fee = pool.GetIter(tx5->GetHash()).value()->GetFee();
    const auto tx6_fee = pool.GetIter(tx6->GetHash()).value()->GetFee();

    BOOST_CHECK_EQUAL(tx1_fee, normal_fee);
    BOOST_CHECK_EQUAL(tx2_fee, normal_fee);
    BOOST_CHECK_EQUAL(tx3_fee, low_fee);
    BOOST_CHECK_EQUAL(tx4_fee, high_fee);
    BOOST_CHECK_EQUAL(tx5_fee, low_fee);
    BOOST_CHECK_EQUAL(tx6_fee, low_fee);

    // Hypothetical replacement transaction virtual size.
    const int64_t replacement_vsize{300};
    const int64_t huge_replacement_vsize{99999};

    // We don't allow replacements with a low ancestor score
    BOOST_CHECK(CheckAncestorScores(/* replacement_fees */ normal_fee, replacement_vsize,
                                    /* ancestors */ low_low, /* conflicts */ cpfp));
    BOOST_CHECK(CheckAncestorScores(/* replacement_fees */ normal_fee, replacement_vsize,
                                    /* ancestors */ low_low, /* conflicts */ normal_normal));
    BOOST_CHECK(CheckAncestorScores(/* replacement_fees */ high_fee, huge_replacement_vsize,
                                    /* ancestors */ low_low, /* conflicts */ cpfp));
    BOOST_CHECK(CheckAncestorScores(/* replacement_fees */ low_fee, replacement_vsize,
                                    /* ancestors */ low_low, /* conflicts */ normal_normal));

    // We do allow replacements with a higher ancestor score
    BOOST_CHECK(!CheckAncestorScores(/* replacement_fees */ normal_fee, replacement_vsize,
                                     /* ancestors */ normal_normal,
                                     /* conflicts */ low_low));
    BOOST_CHECK(!CheckAncestorScores(/* replacement_fees */ normal_fee, replacement_vsize,
                                     /* ancestors */ cpfp,
                                     /* conflicts */ normal_normal));
    BOOST_CHECK(!CheckAncestorScores(/* replacement_fees */ high_fee, replacement_vsize,
                                     /* ancestors */ low_low,
                                     /* conflicts */ normal_normal));
}

BOOST_AUTO_TEST_SUITE_END()
