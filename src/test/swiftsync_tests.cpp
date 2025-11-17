// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <primitives/transaction.h>
#include <swiftsync.h>
#include <boost/test/unit_test.hpp>

namespace {
Txid txid_1{Txid{"bd0f71c1d5e50589063e134fad22053cdae5ab2320db5bf5e540198b0b5a4e69"}};
Txid txid_2{Txid{"b4749f017444b051c44dfd2720e88f314ff94f3dd6d56d40ef65854fcd7fff6b"}};
Txid txid_3{Txid{"ee707be5201160e32c4fc715bec227d1aeea5940fb4295605e7373edce3b1a93"}};
COutPoint outpoint_1{COutPoint{txid_1, 2142}};
COutPoint outpoint_2{COutPoint{txid_2, 99328}};
COutPoint outpoint_3{COutPoint{txid_3, 5438584}};
} // namespace

BOOST_AUTO_TEST_SUITE(swiftsync_tests);

BOOST_AUTO_TEST_CASE(swiftsync_aggregate_test)
{
    swiftsync::Aggregate agg{};
    agg.Create(outpoint_1);
    agg.Spend(outpoint_2);
    agg.Create(outpoint_3);
    BOOST_CHECK(!agg.IsBalanced());
    agg.Spend(outpoint_1);
    agg.Create(outpoint_2);
    agg.Spend(outpoint_3);
    BOOST_CHECK(agg.IsBalanced());
}

BOOST_AUTO_TEST_SUITE_END();
