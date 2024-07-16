// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <deque>

#include <consensus/merkle.h>
#include <hash.h>

/*     WARNING! If you're reading this because you're learning about crypto
       and/or designing a new system that will use merkle trees, keep in mind
       that the following merkle tree algorithm has a serious flaw related to
       duplicate txids, resulting in a vulnerability (CVE-2012-2459).

       The reason is that if the number of hashes in the list at a given level
       is odd, the last one is duplicated before computing the next level (which
       is unusual in Merkle trees). This results in certain sequences of
       transactions leading to the same merkle root. For example, these two
       trees:

                    A               A
                  /  \            /   \
                B     C         B       C
               / \    |        / \     / \
              D   E   F       D   E   F   F
             / \ / \ / \     / \ / \ / \ / \
             1 2 3 4 5 6     1 2 3 4 5 6 5 6

       for transaction lists [1,2,3,4,5,6] and [1,2,3,4,5,6,5,6] (where 5 and
       6 are repeated) result in the same root hash A (because the hash of both
       of (F) and (F,F) is C).

       The vulnerability results from being able to send a block with such a
       transaction list, with the same merkle root, and the same block hash as
       the original without duplication, resulting in failed validation. If the
       receiving node proceeds to mark that block as permanently invalid
       however, it will fail to accept further unmodified (and thus potentially
       valid) versions of the same block. We defend against this by detecting
       the case where we would hash two identical hashes at the end of the list
       together, and treating that identically to the block having an invalid
       merkle root. Assuming no double-SHA256 collisions, this will detect all
       known ways of changing the transactions without affecting the merkle
       root.
*/


uint256 ComputeMerkleRoot(std::vector<uint256> hashes, bool* mutated) {
    bool mutation = false;
    while (hashes.size() > 1) {
        if (mutated) {
            for (size_t pos = 0; pos + 1 < hashes.size(); pos += 2) {
                if (hashes[pos] == hashes[pos + 1]) mutation = true;
            }
        }
        if (hashes.size() & 1) {
            hashes.push_back(hashes.back());
        }
        SHA256D64(hashes[0].begin(), hashes[0].begin(), hashes.size() / 2);
        hashes.resize(hashes.size() / 2);
    }
    if (mutated) *mutated = mutation;
    if (hashes.size() == 0) return uint256();
    return hashes[0];
}


uint256 BlockMerkleRoot(const CBlock& block, bool* mutated)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    for (size_t s = 0; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s]->GetHash();
    }
    return ComputeMerkleRoot(std::move(leaves), mutated);
}

uint256 BlockWitnessMerkleRoot(const CBlock& block, bool* mutated)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    leaves[0].SetNull(); // The witness hash of the coinbase is 0.
    for (size_t s = 1; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s]->GetWitnessHash();
    }
    return ComputeMerkleRoot(std::move(leaves), mutated);
}

static uint256 HashTwoTxIDs(const uint256& txid1, const uint256& txid2) {
    HashWriter hasher{};
    hasher << txid1 << txid2;
    return hasher.GetHash();
}

std::vector<uint256> GetCoinBaseMerklePath(const CBlock& block)
{
    auto size = block.vtx.size();
    // If we have only the coinbase tx, we don't have a merkle path
    if (size == 1) {
        return {};
    // If we have coinbase tx and another tx the path is the second node id
    } else if (size == 2) {
        std::vector<uint256> path;
        path.push_back(block.vtx[1]->GetHash());
        return path;
    // Otherwise we calculate the merkle path
    } else {
        std::deque<uint256> id_list;
        for (const auto& tx : block.vtx) {
            id_list.push_back(tx->GetHash());
        }
        // Last id must be duplicated when txs are odds
        if (size % 2 == 1) {
            id_list.push_back(block.vtx[size - 1]->GetHash());
        }

        // Remove coinbase
        id_list.pop_front();

        std::vector<uint256> path;

        // First path element is always the second tx
        path.push_back(id_list.front());
        id_list.pop_front();

        while (!id_list.empty()) {
            for (size_t i = 0; i < id_list.size() / 2; ++i) {
                id_list[i] = HashTwoTxIDs(id_list[i * 2], id_list[i * 2 + 1]);
            }
            id_list.resize(id_list.size()/2);
            path.push_back(id_list.front());
            id_list.pop_front();
            if (id_list.size() % 2 == 1) {
                id_list.push_back(id_list[id_list.size() - 1]);
            }
        }

        return path;
    }
}
