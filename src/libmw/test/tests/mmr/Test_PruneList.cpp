// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/mmr/PruneList.h>
#include <mw/file/File.h>

#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestPruneList, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(PruneListTest)
{
    // Bitset: 00100000 01000000 00000000 00110011 11111111 100000000
    File(GetDataDir() / "prun000009.dat")
        .Write({ 0x20, 0x40, 0x00, 0x33, 0xff, 0x80 });

    PruneList::Ptr pPruneList = PruneList::Open(GetDataDir(), 9);

    BOOST_REQUIRE(pPruneList->GetTotalShift() == 15);
    BOOST_REQUIRE(pPruneList->GetShift(mmr::Index::At(1)) == 0);
    BOOST_REQUIRE(pPruneList->GetShift(mmr::Index::At(3)) == 1);
    BOOST_REQUIRE(pPruneList->GetShift(mmr::Index::At(28)) == 4);
    BOOST_REQUIRE(pPruneList->GetShift(mmr::Index::At(60)) == 15);
}

BOOST_AUTO_TEST_SUITE_END()