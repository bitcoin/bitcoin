// Copyright (c) 2021 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/mmr/LeafSet.h>
#include <mw/crypto/Hasher.h>

#include <test_framework/TestMWEB.h>

BOOST_FIXTURE_TEST_SUITE(TestMMRLeafSetCache, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(LeafSetCacheTest)
{
    {
        LeafSet::Ptr pLeafset = LeafSet::Open(GetDataDir(), 0);

        pLeafset->Add(mmr::LeafIndex::At(0));
        pLeafset->Add(mmr::LeafIndex::At(1));
        pLeafset->Add(mmr::LeafIndex::At(2));
        pLeafset->Add(mmr::LeafIndex::At(4));

        // Flush to disk
        pLeafset->Flush(1);
    }

    {
        // Reload from disk
        LeafSet::Ptr pLeafset = LeafSet::Open(GetDataDir(), 1);

        // Create cache and validate
        LeafSetCache::Ptr pCache = std::make_shared<LeafSetCache>(pLeafset);
        BOOST_REQUIRE(pCache->GetNextLeafIdx().Get() == 5);
        BOOST_REQUIRE(pCache->Root() == Hashed({0b11101000}));

        // Test with 2 layers of caches to cover all scenarios
        LeafSetCache::Ptr pCache2 = std::make_shared<LeafSetCache>(pCache);
        pCache2->Add(mmr::LeafIndex::At(5));
        pCache2->Remove(mmr::LeafIndex::At(2));

        // pCache2 should contain the updates, but pCache and pLeafset should remain unchanged.
        BOOST_REQUIRE(pLeafset->GetNextLeafIdx().Get() == 5);
        BOOST_REQUIRE(pLeafset->Root() == Hashed({0b11101000}));
        BOOST_REQUIRE(pCache->GetNextLeafIdx().Get() == 5);
        BOOST_REQUIRE(pCache->Root() == Hashed({0b11101000}));
        BOOST_REQUIRE(pCache2->GetNextLeafIdx().Get() == 6);
        BOOST_REQUIRE(pCache2->Root() == Hashed({0b11001100}));

        // Flush pCache2 to pCache. pCache should be updated, but pLeafset should remain unchanged.
        pCache2->Flush(0);
        BOOST_REQUIRE(pLeafset->GetNextLeafIdx().Get() == 5);
        BOOST_REQUIRE(pLeafset->Root() == Hashed({0b11101000}));
        BOOST_REQUIRE(pCache->GetNextLeafIdx().Get() == 6);
        BOOST_REQUIRE(pCache->Root() == Hashed({0b11001100}));
        BOOST_REQUIRE(pCache2->GetNextLeafIdx().Get() == 6);
        BOOST_REQUIRE(pCache2->Root() == Hashed({0b11001100}));
    }
}

BOOST_AUTO_TEST_SUITE_END()