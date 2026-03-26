// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_UTXO_SET_SHARE_H
#define BITCOIN_NODE_UTXO_SET_SHARE_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include <uint256.h>

namespace node {

static constexpr uint64_t UTXO_SET_CHUNK_SIZE = 3'900'000;

/** Verify a Merkle proof for a chunk against a known root */
bool VerifyChunkMerkleProof(const uint256& leaf_hash,
                            const std::vector<uint256>& proof,
                            uint32_t chunk_index,
                            uint32_t total_chunks,
                            const uint256& merkle_root);

} // namespace node

#endif // BITCOIN_NODE_UTXO_SET_SHARE_H
