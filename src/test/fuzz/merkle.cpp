// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <test/util/str.h>
#include <util/strencodings.h>
#include <hash.h>

#include <cassert>
#include <cstdint>
#include <vector>

uint256 ComputeMerkleRootFromPath(const CBlock& block, uint32_t position, const std::vector<uint256>& merkle_path) {
    if (position >= block.vtx.size()) {
        throw std::out_of_range("Position out of range");
    }

    uint256 current_hash = block.vtx[position]->GetHash();

    for (const uint256& sibling : merkle_path) {
        if (position % 2 == 0) {
            current_hash = Hash(current_hash, sibling);
        } else {
            current_hash = Hash(sibling, current_hash);
        }
        position = position / 2;
    }

    return current_hash;
}

FUZZ_TARGET(merkle)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const bool with_witness = fuzzed_data_provider.ConsumeBool();
    std::optional<CBlock> block {ConsumeDeserializable<CBlock>(fuzzed_data_provider, with_witness ? TX_WITH_WITNESS : TX_NO_WITNESS)};
    if (!block){
        return;
    }
    const size_t num_txs = block->vtx.size();
    std::vector<uint256> tx_hashes;
    tx_hashes.reserve(num_txs);

    for (size_t i = 0; i < num_txs; ++i) {
        tx_hashes.push_back(block->vtx[i]->GetHash());
    }

    // Test ComputeMerkleRoot
    bool mutated = fuzzed_data_provider.ConsumeBool();
    const uint256 merkle_root = ComputeMerkleRoot(tx_hashes, &mutated);

    // Basic sanity checks for ComputeMerkleRoot
    if (tx_hashes.size() == 1) {
        assert(merkle_root == tx_hashes[0]);
    }


    const uint256 block_merkle_root = BlockMerkleRoot(*block, &mutated);
    if (tx_hashes.size() == 1) {
        assert(block_merkle_root == tx_hashes[0]);
    }

    if (!block->vtx.empty()){
        const uint256 block_witness_merkle_root = BlockWitnessMerkleRoot(*block, &mutated);
        if (tx_hashes.size() == 1) {
            assert(block_witness_merkle_root == uint256());
        }
    }

    // Test TransactionMerklePath
    const uint32_t position = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, num_txs > 0 ? num_txs - 1 : 0);
    std::vector<uint256> merkle_path = TransactionMerklePath(*block, position);

    // Check that the root we compute from TransactionMerklePath equals the same merkle_root and block_merkle_root
    if (tx_hashes.size() > 1) {
        uint256 merkle_root_from_merkle_path = ComputeMerkleRootFromPath(*block, position, merkle_path);
        assert(merkle_root_from_merkle_path == merkle_root);
        assert(merkle_root_from_merkle_path == block_merkle_root);
    }

    // Basic sanity checks for TransactionMerklePath
    assert(merkle_path.size() <= 32); // Maximum depth of a Merkle tree with 2^32 leaves
    if (num_txs == 1 || num_txs == 0) {
        assert(merkle_path.empty()); // Single transaction has no path
    }

}
