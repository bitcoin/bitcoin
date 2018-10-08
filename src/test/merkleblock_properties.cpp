#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <rapidcheck/Gen.h>
#include <rapidcheck/boost_test.h>
#include <rapidcheck/gen/Arbitrary.h>

#include <merkleblock.h>
#include <test/gen/merkleblock_gen.h>

#include <iostream>
#include <streams.h>
BOOST_FIXTURE_TEST_SUITE(merkleblock_properties, BasicTestingSetup)

RC_BOOST_PROP(merkleblock_serialization_symmetry, (const CMerkleBlock& mb))
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << mb;
    CMerkleBlock mb2;
    ss >> mb2;
    CDataStream ss1(SER_NETWORK, PROTOCOL_VERSION);
    ss << mb;
    ss1 << mb2;
    RC_ASSERT(ss.str() == ss1.str());
}

/** Should find all txids we inserted in the merkle block */
RC_BOOST_PROP(merkle_block_match_symmetry, (std::pair<CMerkleBlock, std::set<uint256>> p))
{
    const CMerkleBlock& mb = p.first;
    const std::set<uint256>& inserted_hashes = p.second;
    for (unsigned int i = 0; i < mb.vMatchedTxn.size(); i++) {
        const auto& h = mb.vMatchedTxn[i].second;
        RC_ASSERT(inserted_hashes.find(h) != inserted_hashes.end());
    }
}

RC_BOOST_PROP(partialmerkletree_serialization_symmetry, (const CPartialMerkleTree& tree))
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << tree;
    CPartialMerkleTree tree2;
    ss >> tree2;
    CDataStream ss1(SER_NETWORK, PROTOCOL_VERSION);
    ss << tree;
    ss1 << tree2;
    RC_ASSERT(ss.str() == ss1.str());
}


/** Should find all txids we inserted in the PartialMerkleTree */
RC_BOOST_PROP(partialmerkletree_extract_matches_symmetry, (std::pair<CPartialMerkleTree, std::vector<uint256>> p))
{
    CPartialMerkleTree& tree = p.first;
    const std::vector<uint256>& expectedMatches = p.second;
    std::vector<uint256> matches;
    std::vector<unsigned int> indices;
    tree.ExtractMatches(matches, indices);
    RC_ASSERT(matches == expectedMatches);
}

BOOST_AUTO_TEST_SUITE_END()
