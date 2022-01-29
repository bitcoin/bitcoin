// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/mmr/MMR.h>

#include <test_framework/TestMWEB.h>

using namespace mmr;

BOOST_FIXTURE_TEST_SUITE(TestMMR, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(MMRTest)
{
    auto pmmr = PMMR::Open(
        'O',
        GetDataDir() / "mmr",
        0,
        GetDB(),
        nullptr
    );

    std::vector<uint8_t> leaf0({ 0, 1, 2 });
    std::vector<uint8_t> leaf1({ 1, 2, 3 });
    std::vector<uint8_t> leaf2({ 2, 3, 4 });
    std::vector<uint8_t> leaf3({ 3, 4, 5 });
    std::vector<uint8_t> leaf4({ 4, 5, 6 });

    pmmr->Add(leaf0);
    pmmr->Add(leaf1);
    pmmr->Add(leaf2);
    pmmr->Add(leaf3);

    BOOST_REQUIRE(pmmr->GetLeaf(LeafIndex::At(0)) == Leaf::Create(LeafIndex::At(0), leaf0));
    BOOST_REQUIRE(pmmr->GetLeaf(LeafIndex::At(1)) == Leaf::Create(LeafIndex::At(1), leaf1));
    BOOST_REQUIRE(pmmr->GetLeaf(LeafIndex::At(2)) == Leaf::Create(LeafIndex::At(2), leaf2));
    BOOST_REQUIRE(pmmr->GetLeaf(LeafIndex::At(3)) == Leaf::Create(LeafIndex::At(3), leaf3));

    BOOST_REQUIRE(pmmr->GetNumLeaves() == 4);
    BOOST_REQUIRE(pmmr->GetNumNodes() == 7);
    BOOST_CHECK_EQUAL(pmmr->Root().ToHex(), "9ab6e3c4a8594b9846b39b6beefe8f704c1de720f28426ddf3898bd4f8d6e45f");

    pmmr->Add(leaf4);
    BOOST_REQUIRE(pmmr->GetLeaf(LeafIndex::At(4)) == Leaf::Create(LeafIndex::At(4), leaf4));
    BOOST_REQUIRE(pmmr->GetNumLeaves() == 5);
    BOOST_REQUIRE(pmmr->GetNumNodes() == 8);
    BOOST_CHECK_EQUAL(pmmr->Root().ToHex(), "376ef1612abbb461ab78f317569c9a19d054f2c928c79410d50403564b91c5f7");

    pmmr->Rewind(4);
    BOOST_REQUIRE(pmmr->GetNumLeaves() == 4);
    BOOST_REQUIRE(pmmr->GetNumNodes() == 7);
    BOOST_CHECK_EQUAL(pmmr->Root().ToHex(), "9ab6e3c4a8594b9846b39b6beefe8f704c1de720f28426ddf3898bd4f8d6e45f");
}

BOOST_AUTO_TEST_CASE(PMMRCacheTest)
{
    PMMR::Ptr pmmr = PMMR::Open(
        'O',
        GetDataDir() / "mmr",
        0,
        GetDB(),
        nullptr
    );

    PMMRCache cache(pmmr);
    std::vector<uint8_t> leaf0({ 0, 1, 2 });
    std::vector<uint8_t> leaf1({ 1, 2, 3 });
    std::vector<uint8_t> leaf2({ 2, 3, 4 });
    std::vector<uint8_t> leaf3({ 3, 4, 5 });
    std::vector<uint8_t> leaf4({ 4, 5, 6 });

    cache.Add(leaf0);
    cache.Add(leaf1);
    cache.Add(leaf2);
    cache.Add(leaf3);

    BOOST_REQUIRE(cache.GetLeaf(LeafIndex::At(0)) == Leaf::Create(LeafIndex::At(0), leaf0));
    BOOST_REQUIRE(cache.GetLeaf(LeafIndex::At(1)) == Leaf::Create(LeafIndex::At(1), leaf1));
    BOOST_REQUIRE(cache.GetLeaf(LeafIndex::At(2)) == Leaf::Create(LeafIndex::At(2), leaf2));
    BOOST_REQUIRE(cache.GetLeaf(LeafIndex::At(3)) == Leaf::Create(LeafIndex::At(3), leaf3));

    BOOST_REQUIRE(cache.GetNumLeaves() == 4);
    BOOST_CHECK_EQUAL(cache.Root().ToHex(), "9ab6e3c4a8594b9846b39b6beefe8f704c1de720f28426ddf3898bd4f8d6e45f");

    cache.Add(leaf4);
    BOOST_REQUIRE(cache.GetLeaf(LeafIndex::At(4)) == Leaf::Create(LeafIndex::At(4), leaf4));
    BOOST_REQUIRE(cache.GetNumLeaves() == 5);
    BOOST_CHECK_EQUAL(cache.Root().ToHex(), "376ef1612abbb461ab78f317569c9a19d054f2c928c79410d50403564b91c5f7");

    cache.Rewind(4);
    BOOST_REQUIRE(cache.GetNumLeaves() == 4);
    BOOST_CHECK_EQUAL(cache.Root().ToHex(), "9ab6e3c4a8594b9846b39b6beefe8f704c1de720f28426ddf3898bd4f8d6e45f");

    cache.Flush(1, nullptr);
}

BOOST_AUTO_TEST_SUITE_END()