// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/utxo_set_share.h>

#include <hash.h>
#include <uint256.h>

#include <cstdint>
#include <vector>

namespace node {

bool VerifyChunkMerkleProof(const uint256& leaf_hash,
                            const std::vector<uint256>& proof,
                            uint32_t chunk_index,
                            uint32_t total_chunks,
                            const uint256& merkle_root)
{
    if (total_chunks == 0) return false;

    // Verify proof depth matches the expected tree height
    uint32_t expected_depth{0};
    for (uint32_t n{total_chunks - 1}; n > 0; n >>= 1) ++expected_depth;
    if (proof.size() != expected_depth) return false;

    uint256 computed{leaf_hash};
    uint32_t index{chunk_index};

    for (const uint256& sibling : proof) {
        bool is_right_child{(index % 2) != 0};
        if (is_right_child) {
            computed = Hash(sibling, computed);
        } else {
            computed = Hash(computed, sibling);
        }
        index /= 2;
    }

    return computed == merkle_root;
}

} // namespace node
