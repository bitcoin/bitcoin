// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <hash.h>
#include <node/utxo_set_share.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(utxo_set_share_tests, BasicTestingSetup)

std::vector<uint256> MakeLeaves(FastRandomContext& rng, size_t count)
{
    std::vector<uint256> leaves;
    leaves.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        leaves.push_back(rng.rand256());
    }
    return leaves;
}

BOOST_AUTO_TEST_CASE(merkle_root_single_leaf)
{
    std::vector<uint256> leaves{MakeLeaves(m_rng, 1)};
    uint256 root{ComputeMerkleRoot(leaves)};
    // Root is the leaf itself
    BOOST_CHECK_EQUAL(root.ToString(), leaves[0].ToString());
}

BOOST_AUTO_TEST_CASE(merkle_root_two_leaves)
{
    std::vector<uint256> leaves{MakeLeaves(m_rng, 2)};
    uint256 root{ComputeMerkleRoot(leaves)};
    BOOST_CHECK_EQUAL(root.ToString(), Hash(leaves[0], leaves[1]).ToString());
}

BOOST_AUTO_TEST_CASE(merkle_root_three_leaves)
{
    std::vector<uint256> leaves{MakeLeaves(m_rng, 3)};
    uint256 root{ComputeMerkleRoot(leaves)};

    uint256 h01{Hash(leaves[0], leaves[1])};
    // Last leaf is duplicated to make up for the uneven count
    uint256 h22{Hash(leaves[2], leaves[2])};
    BOOST_CHECK_EQUAL(root.ToString(), Hash(h01, h22).ToString());
}

BOOST_AUTO_TEST_CASE(merkle_proof_roundtrip)
{
    for (uint32_t num_leaves : {1, 2, 3, 4, 5, 7, 8, 16}) {
        std::vector<uint256> leaves{MakeLeaves(m_rng, num_leaves)};
        uint256 root{ComputeMerkleRoot(leaves)};

        for (uint32_t pos = 0; pos < num_leaves; ++pos) {
            std::vector<uint256> proof{ComputeMerklePath(leaves, pos)};
            BOOST_CHECK(node::VerifyChunkMerkleProof(leaves[pos], proof, pos, num_leaves, root));
        }
    }
}

BOOST_AUTO_TEST_CASE(merkle_proof_invalid_leaf)
{
    std::vector<uint256> leaves{MakeLeaves(m_rng, 4)};
    uint256 root{ComputeMerkleRoot(leaves)};
    std::vector<uint256> proof{ComputeMerklePath(leaves, 0)};

    uint256 wrong_leaf{m_rng.rand256()};
    BOOST_CHECK(!node::VerifyChunkMerkleProof(wrong_leaf, proof, 0, 4, root));
}

BOOST_AUTO_TEST_CASE(merkle_proof_invalid_root)
{
    std::vector<uint256> leaves{MakeLeaves(m_rng, 4)};
    uint256 root{ComputeMerkleRoot(leaves)};
    std::vector<uint256> proof{ComputeMerklePath(leaves, 0)};

    BOOST_CHECK(node::VerifyChunkMerkleProof(leaves[0], proof, 0, 4, root));

    uint256 wrong_root{m_rng.rand256()};
    BOOST_CHECK(!node::VerifyChunkMerkleProof(leaves[0], proof, 0, 4, wrong_root));
}

BOOST_AUTO_TEST_CASE(merkle_proof_invalid_proof)
{
    std::vector<uint256> leaves{MakeLeaves(m_rng, 8)};
    uint256 root{ComputeMerkleRoot(leaves)};

    std::vector<uint256> proof{ComputeMerklePath(leaves, 3)};

    // Corrupt one proof hash
    proof[0] = m_rng.rand256();
    BOOST_CHECK(!node::VerifyChunkMerkleProof(leaves[3], proof, 3, 8, root));
}

BOOST_AUTO_TEST_CASE(merkle_proof_zero_chunks)
{
    uint256 leaf{m_rng.rand256()};
    uint256 root{m_rng.rand256()};
    std::vector<uint256> proof;
    BOOST_CHECK(!node::VerifyChunkMerkleProof(leaf, proof, 0, 0, root));
}

BOOST_AUTO_TEST_SUITE_END()
