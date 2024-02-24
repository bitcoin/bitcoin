// Copyright (c) 2022 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mw/mmr/MMR.h>
#include <mw/mmr/MMRUtil.h>
#include <mw/mmr/LeafSet.h>
#include <mw/mmr/Segment.h>
#include <boost/optional/optional_io.hpp>

#include <test_framework/TestMWEB.h>

namespace std {
ostream& operator<<(ostream& os, const mw::Hash& hash)
{
    os << hash.ToHex();
    return os;
}

ostream& operator<<(ostream& os, const mmr::Leaf& leaf)
{
    os << leaf.GetHash().ToHex();
    return os;
}
} // namespace std

using namespace mmr;

struct MMRWithLeafset {
    IMMR::Ptr mmr;
    ILeafSet::Ptr leafset;
};

static mmr::Leaf DeterministicLeaf(const uint64_t i)
{
    std::vector<uint8_t> serialized{
        uint8_t(i >> 24),
        uint8_t(i >> 16),
        uint8_t(i >> 8),
        uint8_t(i)};
    return mmr::Leaf::Create(mmr::LeafIndex::At(i), serialized);
}

static MMRWithLeafset BuildDetermininisticMMR(const uint64_t num_leaves)
{
    auto mmr = std::make_shared<MemMMR>();
    auto leafset = LeafSet::Open(GetDataDir(), 0);
    for (size_t i = 0; i < num_leaves; i++) {
        mmr->AddLeaf(DeterministicLeaf(i));
        leafset->Add(mmr::LeafIndex::At(i));
    }

    return MMRWithLeafset{mmr, leafset};
}

BOOST_FIXTURE_TEST_SUITE(TestSegment, MWEBTestingSetup)

BOOST_AUTO_TEST_CASE(AssembleSegment)
{
    auto mmr_with_leafset = BuildDetermininisticMMR(15);
    auto mmr = mmr_with_leafset.mmr;
    auto leafset = mmr_with_leafset.leafset;
    Segment segment = SegmentFactory::Assemble(
        *mmr,
        *leafset,
        mmr::LeafIndex::At(0),
        4
    );

    std::vector<mmr::Leaf> expected_leaves{
        DeterministicLeaf(0),
        DeterministicLeaf(1),
        DeterministicLeaf(2),
        DeterministicLeaf(3)
    };
    BOOST_REQUIRE_EQUAL_COLLECTIONS(segment.leaves.begin(), segment.leaves.end(), expected_leaves.begin(), expected_leaves.end());

    std::vector<mw::Hash> expected_hashes{
        mmr->GetHash(mmr::Index::At(13))
    };
    BOOST_REQUIRE_EQUAL_COLLECTIONS(segment.hashes.begin(), segment.hashes.end(), expected_hashes.begin(), expected_hashes.end());

    boost::optional<mw::Hash> expected_lower_peak = MMRUtil::CalcBaggedPeak(*mmr, mmr::Index::At(21));
    BOOST_REQUIRE_EQUAL(expected_lower_peak, segment.lower_peak);

    // Verify PMMR root can be fully recomputed
    mw::Hash n2 = MMRUtil::CalcParentHash(mmr::Index::At(2), segment.leaves[0].GetHash(), segment.leaves[1].GetHash());
    mw::Hash n5 = MMRUtil::CalcParentHash(mmr::Index::At(5), segment.leaves[2].GetHash(), segment.leaves[3].GetHash());
    mw::Hash n6 = MMRUtil::CalcParentHash(mmr::Index::At(6), n2, n5);
    mw::Hash n14 = MMRUtil::CalcParentHash(mmr::Index::At(14), n6, segment.hashes[0]);
    mw::Hash root = MMRUtil::CalcParentHash(Index::At(26), n14, *segment.lower_peak);
    BOOST_REQUIRE_EQUAL(root, mmr->Root());
}

BOOST_AUTO_TEST_SUITE_END()