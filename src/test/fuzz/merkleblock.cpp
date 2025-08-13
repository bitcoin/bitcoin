// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <merkleblock.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

FUZZ_TARGET(merkleblock)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CPartialMerkleTree partial_merkle_tree;
    CallOneOf(
        fuzzed_data_provider,
        [&] {
            const std::optional<CPartialMerkleTree> opt_partial_merkle_tree = ConsumeDeserializable<CPartialMerkleTree>(fuzzed_data_provider);
            if (opt_partial_merkle_tree) {
                partial_merkle_tree = *opt_partial_merkle_tree;
            }
        },
        [&] {
            CMerkleBlock merkle_block;
            const std::optional<CBlock> opt_block = ConsumeDeserializable<CBlock>(fuzzed_data_provider, TX_WITH_WITNESS);
            CBloomFilter bloom_filter;
            std::set<Txid> txids;
            if (opt_block && !opt_block->vtx.empty()) {
                if (fuzzed_data_provider.ConsumeBool()) {
                    merkle_block = CMerkleBlock{*opt_block, bloom_filter};
                } else if (fuzzed_data_provider.ConsumeBool()) {
                    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
                        txids.insert(Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)));
                    }
                    merkle_block = CMerkleBlock{*opt_block, txids};
                }
            }
            partial_merkle_tree = merkle_block.txn;
        });
    (void)partial_merkle_tree.GetNumTransactions();
    std::vector<Txid> matches;
    std::vector<unsigned int> indices;
    (void)partial_merkle_tree.ExtractMatches(matches, indices);
}
