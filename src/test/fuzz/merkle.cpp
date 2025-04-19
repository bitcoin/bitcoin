// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/util/str.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <vector>

FUZZ_TARGET(merkle)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    // Generate a random number of transactions (0-1000 to keep it reasonable)
    const size_t num_txs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 1000);
    std::vector<uint256> tx_hashes;
    tx_hashes.reserve(num_txs);

    // Create a CBlock with fuzzed transactions
    CBlock block;
    block.vtx.resize(num_txs);

    for (size_t i = 0; i < num_txs; ++i) {
        CMutableTransaction mtx;
        // Add minimal valid content to make it a transaction
        mtx.version = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        mtx.nLockTime = fuzzed_data_provider.ConsumeIntegral<uint32_t>();

        // Add at least one input with a properly constructed Txid
        CTxIn txin;
        std::vector<unsigned char> prevout_hash_bytes = fuzzed_data_provider.ConsumeBytes<unsigned char>(32);
        if (prevout_hash_bytes.size() < 32) {
            prevout_hash_bytes.resize(32, 0); // Pad with zeros if needed
        }
        uint256 prevout_hash;
        memcpy(prevout_hash.begin(), prevout_hash_bytes.data(), 32);
        txin.prevout = COutPoint(Txid::FromUint256(prevout_hash), fuzzed_data_provider.ConsumeIntegral<uint32_t>());
        std::vector<unsigned char> script_sig_bytes = fuzzed_data_provider.ConsumeBytes<unsigned char>(MAX_SCRIPT_SIZE);
        txin.scriptSig = CScript(script_sig_bytes.begin(), script_sig_bytes.end());
        txin.nSequence = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        mtx.vin.push_back(txin);

        CTxOut txout;
        txout.nValue = fuzzed_data_provider.ConsumeIntegral<int64_t>();
        std::vector<unsigned char> script_pk_bytes = fuzzed_data_provider.ConsumeBytes<unsigned char>(MAX_SCRIPT_SIZE);
        txout.scriptPubKey = CScript(script_pk_bytes.begin(), script_pk_bytes.end());
        mtx.vout.push_back(txout);

        // Compute the hash and store it
        block.vtx[i] = MakeTransactionRef(std::move(mtx));
        tx_hashes.push_back(block.vtx[i]->GetHash());
    }

    // Test ComputeMerkleRoot
    bool mutated = fuzzed_data_provider.ConsumeBool();
    //bool mutated = false;
    const uint256 merkle_root = ComputeMerkleRoot(tx_hashes, &mutated);

    // Basic sanity checks for ComputeMerkleRoot
    if (tx_hashes.size() == 1) {
        assert(merkle_root == tx_hashes[0]);
    }


    const uint256 block_merkle_root = BlockMerkleRoot(block, &mutated);
    if (tx_hashes.size() == 1) {
        assert(block_merkle_root == tx_hashes[0]);
    }

    if (!block.vtx.empty()){
        const uint256 block_witness_merkle_root = BlockWitnessMerkleRoot(block, &mutated);
        if (tx_hashes.size() == 1) {
            assert(block_witness_merkle_root == uint256());
        }
    }

    // Test TransactionMerklePath
    const uint32_t position = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, num_txs);
    std::vector<uint256> merkle_path = TransactionMerklePath(block, position);

    // Basic sanity checks for TransactionMerklePath
    assert(merkle_path.size() <= 32); // Maximum depth of a Merkle tree with 2^32 leaves
    if (num_txs == 1 || num_txs == 0) {
        assert(merkle_path.empty()); // Single transaction has no path
    }
}
