// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <hash.h>
#include <kernel/chainparams.h>
#include <node/utxo_set_share.h>
#include <node/utxo_snapshot.h>
#include <streams.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/fs.h>

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

BOOST_AUTO_TEST_CASE(provider_init_and_serve)
{
    // Create a share directory with a synthetic snapshot file that matches
    // regtest assumeutxo params at height 110.
    auto regtest_params{CChainParams::RegTest({})};
    auto au_data{regtest_params->AssumeutxoForHeight(110)};
    BOOST_REQUIRE(au_data.has_value());

    fs::path share_dir{m_path_root / "share"};
    fs::create_directories(share_dir);

    // Write a fake snapshot
    fs::path snapshot_path{share_dir / "utxo-snapshot-110.dat"};
    {
        AutoFile file{fsbridge::fopen(snapshot_path, "wb")};
        BOOST_REQUIRE(!file.IsNull());

        node::SnapshotMetadata metadata(regtest_params->MessageStart(), au_data->blockhash, /*coins_count=*/42);
        file << metadata;

        // Write some random body data
        std::vector<unsigned char> body{m_rng.randbytes(node::UTXO_SET_CHUNK_SIZE * 2 + 500)};
        file.write(MakeByteSpan(body));
        BOOST_REQUIRE(file.fclose() == 0);
    }

    // Initialize the provider
    node::UTXOSetShareProvider provider;
    size_t count{provider.Initialize(share_dir, *regtest_params)};
    BOOST_CHECK_EQUAL(count, 1U);

    // Check info entries
    auto entries{provider.GetInfoEntries()};
    BOOST_CHECK_EQUAL(entries.size(), 1U);
    BOOST_CHECK_EQUAL(entries[0].height, static_cast<uint32_t>(au_data->height));
    BOOST_CHECK_EQUAL(entries[0].block_hash.ToString(), au_data->blockhash.ToString());

    // Check chunks
    uint64_t file_size{entries[0].data_length};
    uint32_t total_chunks{static_cast<uint32_t>((file_size + node::UTXO_SET_CHUNK_SIZE - 1) / node::UTXO_SET_CHUNK_SIZE)};
    BOOST_CHECK_EQUAL(total_chunks, 3U);

    // Retrieve each chunk and verify it's Merkle proof
    for (uint32_t i = 0; i < total_chunks; ++i) {
        auto result{provider.GetChunk(entries[0].height, entries[0].block_hash, i)};
        BOOST_REQUIRE(result.has_value());

        auto& [data, proof] = *result;
        BOOST_CHECK(!data.empty());

        uint256 leaf_hash{Hash(data)};
        BOOST_CHECK(node::VerifyChunkMerkleProof(leaf_hash, proof, i, total_chunks, entries[0].merkle_root));
    }

    // Out-of-range chunk should fail
    auto bad{provider.GetChunk(entries[0].height, entries[0].block_hash, total_chunks)};
    BOOST_CHECK(!bad.has_value());

    // Verify sidecar file
    fs::path sidecar_path{snapshot_path};
    sidecar_path += ".merkle";
    BOOST_CHECK(fs::exists(sidecar_path));

    // Re-init should load from sidecar
    node::UTXOSetShareProvider provider2;
    BOOST_CHECK_EQUAL(provider2.Initialize(share_dir, *regtest_params), 1U);
    auto entries2{provider2.GetInfoEntries()};
    BOOST_CHECK_EQUAL(entries2[0].merkle_root.ToString(), entries[0].merkle_root.ToString());
}

BOOST_AUTO_TEST_SUITE_END()
