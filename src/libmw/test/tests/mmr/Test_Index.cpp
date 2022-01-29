// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <mw/mmr/Index.h>

#include <test_framework/TestMWEB.h>

using namespace mmr;

BOOST_FIXTURE_TEST_SUITE(TestMMRIndex, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(GetHeight)
{
    BOOST_REQUIRE(Index::At(0).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(1).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(2).GetHeight() == 1);
    BOOST_REQUIRE(Index::At(3).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(4).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(5).GetHeight() == 1);
    BOOST_REQUIRE(Index::At(6).GetHeight() == 2);
    BOOST_REQUIRE(Index::At(7).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(8).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(9).GetHeight() == 1);
    BOOST_REQUIRE(Index::At(10).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(11).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(12).GetHeight() == 1);
    BOOST_REQUIRE(Index::At(13).GetHeight() == 2);
    BOOST_REQUIRE(Index::At(14).GetHeight() == 3);
    BOOST_REQUIRE(Index::At(15).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(16).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(17).GetHeight() == 1);
    BOOST_REQUIRE(Index::At(18).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(19).GetHeight() == 0);
    BOOST_REQUIRE(Index::At(20).GetHeight() == 1);
}

BOOST_AUTO_TEST_CASE(GetLeafIndex)
{
    BOOST_REQUIRE(Index::At(0).GetLeafIndex() == 0);
    BOOST_REQUIRE(Index::At(1).GetLeafIndex() == 1);
    BOOST_REQUIRE(Index::At(3).GetLeafIndex() == 2);
    BOOST_REQUIRE(Index::At(4).GetLeafIndex() == 3);
    BOOST_REQUIRE(Index::At(7).GetLeafIndex() == 4);
    BOOST_REQUIRE(Index::At(8).GetLeafIndex() == 5);
    BOOST_REQUIRE(Index::At(10).GetLeafIndex() == 6);
    BOOST_REQUIRE(Index::At(11).GetLeafIndex() == 7);
    BOOST_REQUIRE(Index::At(15).GetLeafIndex() == 8);
    BOOST_REQUIRE(Index::At(16).GetLeafIndex() == 9);
    BOOST_REQUIRE(Index::At(18).GetLeafIndex() == 10);
    BOOST_REQUIRE(Index::At(19).GetLeafIndex() == 11);
}

BOOST_AUTO_TEST_CASE(IsLeaf)
{
    BOOST_REQUIRE(Index::At(0).IsLeaf());
    BOOST_REQUIRE(Index::At(1).IsLeaf());
    BOOST_REQUIRE(Index::At(3).IsLeaf());
    BOOST_REQUIRE(Index::At(4).IsLeaf());
    BOOST_REQUIRE(Index::At(7).IsLeaf());
    BOOST_REQUIRE(Index::At(8).IsLeaf());
    BOOST_REQUIRE(Index::At(10).IsLeaf());
    BOOST_REQUIRE(Index::At(11).IsLeaf());
    BOOST_REQUIRE(Index::At(15).IsLeaf());
    BOOST_REQUIRE(Index::At(16).IsLeaf());
    BOOST_REQUIRE(Index::At(18).IsLeaf());
    BOOST_REQUIRE(Index::At(19).IsLeaf());

    BOOST_REQUIRE(!Index::At(2).IsLeaf());
    BOOST_REQUIRE(!Index::At(5).IsLeaf());
    BOOST_REQUIRE(!Index::At(6).IsLeaf());
    BOOST_REQUIRE(!Index::At(9).IsLeaf());
    BOOST_REQUIRE(!Index::At(12).IsLeaf());
    BOOST_REQUIRE(!Index::At(13).IsLeaf());
    BOOST_REQUIRE(!Index::At(14).IsLeaf());
    BOOST_REQUIRE(!Index::At(17).IsLeaf());
    BOOST_REQUIRE(!Index::At(20).IsLeaf());
}

BOOST_AUTO_TEST_CASE(GetNext)
{
    BOOST_REQUIRE(Index::At(0).GetNext() == Index::At(1));
    BOOST_REQUIRE(Index::At(1).GetNext() == Index::At(2));
    BOOST_REQUIRE(Index::At(2).GetNext() == Index::At(3));
    BOOST_REQUIRE(Index::At(3).GetNext() == Index::At(4));
    BOOST_REQUIRE(Index::At(4).GetNext() == Index::At(5));
    BOOST_REQUIRE(Index::At(5).GetNext() == Index::At(6));
    BOOST_REQUIRE(Index::At(6).GetNext() == Index::At(7));
    BOOST_REQUIRE(Index::At(7).GetNext() == Index::At(8));
    BOOST_REQUIRE(Index::At(8).GetNext() == Index::At(9));
    BOOST_REQUIRE(Index::At(9).GetNext() == Index::At(10));
    BOOST_REQUIRE(Index::At(10).GetNext() == Index::At(11));
    BOOST_REQUIRE(Index::At(11).GetNext() == Index::At(12));
    BOOST_REQUIRE(Index::At(12).GetNext() == Index::At(13));
    BOOST_REQUIRE(Index::At(13).GetNext() == Index::At(14));
    BOOST_REQUIRE(Index::At(14).GetNext() == Index::At(15));
    BOOST_REQUIRE(Index::At(15).GetNext() == Index::At(16));
    BOOST_REQUIRE(Index::At(16).GetNext() == Index::At(17));
    BOOST_REQUIRE(Index::At(17).GetNext() == Index::At(18));
    BOOST_REQUIRE(Index::At(18).GetNext() == Index::At(19));
    BOOST_REQUIRE(Index::At(19).GetNext() == Index::At(20));
    BOOST_REQUIRE(Index::At(20).GetNext() == Index::At(21));
}

BOOST_AUTO_TEST_CASE(GetParent)
{
    BOOST_REQUIRE(Index::At(0).GetParent() == Index::At(2));
    BOOST_REQUIRE(Index::At(1).GetParent() == Index::At(2));
    BOOST_REQUIRE(Index::At(2).GetParent() == Index::At(6));
    BOOST_REQUIRE(Index::At(3).GetParent() == Index::At(5));
    BOOST_REQUIRE(Index::At(4).GetParent() == Index::At(5));
    BOOST_REQUIRE(Index::At(5).GetParent() == Index::At(6));
    BOOST_REQUIRE(Index::At(6).GetParent() == Index::At(14));
    BOOST_REQUIRE(Index::At(7).GetParent() == Index::At(9));
    BOOST_REQUIRE(Index::At(8).GetParent() == Index::At(9));
    BOOST_REQUIRE(Index::At(9).GetParent() == Index::At(13));
    BOOST_REQUIRE(Index::At(10).GetParent() == Index::At(12));
    BOOST_REQUIRE(Index::At(11).GetParent() == Index::At(12));
    BOOST_REQUIRE(Index::At(12).GetParent() == Index::At(13));
    BOOST_REQUIRE(Index::At(13).GetParent() == Index::At(14));
    BOOST_REQUIRE(Index::At(14).GetParent() == Index::At(30));
    BOOST_REQUIRE(Index::At(15).GetParent() == Index::At(17));
    BOOST_REQUIRE(Index::At(16).GetParent() == Index::At(17));
    BOOST_REQUIRE(Index::At(17).GetParent() == Index::At(21));
    BOOST_REQUIRE(Index::At(18).GetParent() == Index::At(20));
    BOOST_REQUIRE(Index::At(19).GetParent() == Index::At(20));
    BOOST_REQUIRE(Index::At(20).GetParent() == Index::At(21));
}

BOOST_AUTO_TEST_CASE(GetSibling)
{
    BOOST_REQUIRE(Index::At(0).GetSibling() == Index::At(1));
    BOOST_REQUIRE(Index::At(1).GetSibling() == Index::At(0));
    BOOST_REQUIRE(Index::At(2).GetSibling() == Index::At(5));
    BOOST_REQUIRE(Index::At(3).GetSibling() == Index::At(4));
    BOOST_REQUIRE(Index::At(4).GetSibling() == Index::At(3));
    BOOST_REQUIRE(Index::At(5).GetSibling() == Index::At(2));
    BOOST_REQUIRE(Index::At(6).GetSibling() == Index::At(13));
    BOOST_REQUIRE(Index::At(7).GetSibling() == Index::At(8));
    BOOST_REQUIRE(Index::At(8).GetSibling() == Index::At(7));
    BOOST_REQUIRE(Index::At(9).GetSibling() == Index::At(12));
    BOOST_REQUIRE(Index::At(10).GetSibling() == Index::At(11));
    BOOST_REQUIRE(Index::At(11).GetSibling() == Index::At(10));
    BOOST_REQUIRE(Index::At(12).GetSibling() == Index::At(9));
    BOOST_REQUIRE(Index::At(13).GetSibling() == Index::At(6));
    BOOST_REQUIRE(Index::At(14).GetSibling() == Index::At(29));
    BOOST_REQUIRE(Index::At(15).GetSibling() == Index::At(16));
    BOOST_REQUIRE(Index::At(16).GetSibling() == Index::At(15));
    BOOST_REQUIRE(Index::At(17).GetSibling() == Index::At(20));
    BOOST_REQUIRE(Index::At(18).GetSibling() == Index::At(19));
    BOOST_REQUIRE(Index::At(19).GetSibling() == Index::At(18));
    BOOST_REQUIRE(Index::At(20).GetSibling() == Index::At(17));
}

BOOST_AUTO_TEST_CASE(GetLeftChild)
{
    BOOST_REQUIRE(Index::At(2).GetLeftChild() == Index::At(0));
    BOOST_REQUIRE(Index::At(5).GetLeftChild() == Index::At(3));
    BOOST_REQUIRE(Index::At(6).GetLeftChild() == Index::At(2));
    BOOST_REQUIRE(Index::At(9).GetLeftChild() == Index::At(7));
    BOOST_REQUIRE(Index::At(12).GetLeftChild() == Index::At(10));
    BOOST_REQUIRE(Index::At(13).GetLeftChild() == Index::At(9));
    BOOST_REQUIRE(Index::At(14).GetLeftChild() == Index::At(6));
    BOOST_REQUIRE(Index::At(17).GetLeftChild() == Index::At(15));
    BOOST_REQUIRE(Index::At(20).GetLeftChild() == Index::At(18));
}

BOOST_AUTO_TEST_CASE(GetRightChild)
{
    BOOST_REQUIRE(Index::At(2).GetRightChild() == Index::At(1));
    BOOST_REQUIRE(Index::At(5).GetRightChild() == Index::At(4));
    BOOST_REQUIRE(Index::At(6).GetRightChild() == Index::At(5));
    BOOST_REQUIRE(Index::At(9).GetRightChild() == Index::At(8));
    BOOST_REQUIRE(Index::At(12).GetRightChild() == Index::At(11));
    BOOST_REQUIRE(Index::At(13).GetRightChild() == Index::At(12));
    BOOST_REQUIRE(Index::At(14).GetRightChild() == Index::At(13));
    BOOST_REQUIRE(Index::At(17).GetRightChild() == Index::At(16));
    BOOST_REQUIRE(Index::At(20).GetRightChild() == Index::At(19));
}

BOOST_AUTO_TEST_SUITE_END()