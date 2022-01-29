// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/mmr/LeafIndex.h>

#include <test_framework/TestMWEB.h>

using namespace mmr;

BOOST_FIXTURE_TEST_SUITE(TestMMRLeafIndex, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(GetLeafIndex)
{
    for (uint64_t i = 0; i < 1000; i++)
    {
        BOOST_REQUIRE(LeafIndex::At(i).Get() == i);
    }
}

BOOST_AUTO_TEST_CASE(GetNodeIndex)
{
    BOOST_REQUIRE(LeafIndex::At(0).GetNodeIndex() == Index::At(0));
    BOOST_REQUIRE(LeafIndex::At(1).GetNodeIndex() == Index::At(1));
    BOOST_REQUIRE(LeafIndex::At(2).GetNodeIndex() == Index::At(3));
    BOOST_REQUIRE(LeafIndex::At(3).GetNodeIndex() == Index::At(4));
    BOOST_REQUIRE(LeafIndex::At(4).GetNodeIndex() == Index::At(7));
    BOOST_REQUIRE(LeafIndex::At(5).GetNodeIndex() == Index::At(8));
    BOOST_REQUIRE(LeafIndex::At(6).GetNodeIndex() == Index::At(10));
    BOOST_REQUIRE(LeafIndex::At(7).GetNodeIndex() == Index::At(11));
    BOOST_REQUIRE(LeafIndex::At(8).GetNodeIndex() == Index::At(15));
    BOOST_REQUIRE(LeafIndex::At(9).GetNodeIndex() == Index::At(16));
    BOOST_REQUIRE(LeafIndex::At(10).GetNodeIndex() == Index::At(18));
    BOOST_REQUIRE(LeafIndex::At(11).GetNodeIndex() == Index::At(19));
    BOOST_REQUIRE(LeafIndex::At(12).GetNodeIndex() == Index::At(22));
    BOOST_REQUIRE(LeafIndex::At(13).GetNodeIndex() == Index::At(23));
    BOOST_REQUIRE(LeafIndex::At(14).GetNodeIndex() == Index::At(25));
    BOOST_REQUIRE(LeafIndex::At(15).GetNodeIndex() == Index::At(26));
    BOOST_REQUIRE(LeafIndex::At(16).GetNodeIndex() == Index::At(31));
    BOOST_REQUIRE(LeafIndex::At(17).GetNodeIndex() == Index::At(32));
    BOOST_REQUIRE(LeafIndex::At(18).GetNodeIndex() == Index::At(34));
    BOOST_REQUIRE(LeafIndex::At(19).GetNodeIndex() == Index::At(35));
    BOOST_REQUIRE(LeafIndex::At(20).GetNodeIndex() == Index::At(38));
}

BOOST_AUTO_TEST_CASE(GetPosition)
{
    BOOST_REQUIRE(LeafIndex::At(0).GetPosition() == 0);
    BOOST_REQUIRE(LeafIndex::At(1).GetPosition() == 1);
    BOOST_REQUIRE(LeafIndex::At(2).GetPosition() == 3);
    BOOST_REQUIRE(LeafIndex::At(3).GetPosition() == 4);
    BOOST_REQUIRE(LeafIndex::At(4).GetPosition() == 7);
    BOOST_REQUIRE(LeafIndex::At(5).GetPosition() == 8);
    BOOST_REQUIRE(LeafIndex::At(6).GetPosition() == 10);
    BOOST_REQUIRE(LeafIndex::At(7).GetPosition() == 11);
    BOOST_REQUIRE(LeafIndex::At(8).GetPosition() == 15);
    BOOST_REQUIRE(LeafIndex::At(9).GetPosition() == 16);
    BOOST_REQUIRE(LeafIndex::At(10).GetPosition() == 18);
    BOOST_REQUIRE(LeafIndex::At(11).GetPosition() == 19);
    BOOST_REQUIRE(LeafIndex::At(12).GetPosition() == 22);
    BOOST_REQUIRE(LeafIndex::At(13).GetPosition() == 23);
    BOOST_REQUIRE(LeafIndex::At(14).GetPosition() == 25);
    BOOST_REQUIRE(LeafIndex::At(15).GetPosition() == 26);
    BOOST_REQUIRE(LeafIndex::At(16).GetPosition() == 31);
    BOOST_REQUIRE(LeafIndex::At(17).GetPosition() == 32);
    BOOST_REQUIRE(LeafIndex::At(18).GetPosition() == 34);
    BOOST_REQUIRE(LeafIndex::At(19).GetPosition() == 35);
    BOOST_REQUIRE(LeafIndex::At(20).GetPosition() == 38);
}

BOOST_AUTO_TEST_SUITE_END()