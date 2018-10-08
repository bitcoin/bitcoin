// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_TEST_GEN_MERKLEBLOCK_GEN_H
#define BITCOIN_TEST_GEN_MERKLEBLOCK_GEN_H

#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Arbitrary.h>

#include <merkleblock.h>
#include <test/gen/block_gen.h>

#include <iostream>
namespace rc {
/** Returns a CMerkleblock with the hashes that match inside of the CPartialMerkleTree */
template <>
struct Arbitrary<std::pair<CMerkleBlock, std::set<uint256>>> {
    static Gen<std::pair<CMerkleBlock, std::set<uint256>>> arbitrary()
    {
        return gen::map(gen::arbitrary<CBlock>(), [](const CBlock& block) {
            std::set<uint256> hashes;
            for (unsigned int i = 0; i < block.vtx.size(); i++) {
                //pretty naive to include every other txid in the merkle block
                //but this will work for now.
                if (i % 2 == 0) {
                    hashes.insert(block.vtx[i]->GetHash());
                }
            }
            return std::make_pair(CMerkleBlock(block, hashes), hashes);
        });
    };
};

/** Returns [0,100) uint256s */
Gen<std::vector<uint256>> BetweenZeroAnd100()
{
    return gen::suchThat<std::vector<uint256>>([](const std::vector<uint256>& hashes) {
        return hashes.size() <= 100;
    });
}
/** Returns [1,100) uint256s */
Gen<std::vector<uint256>> Between1And100()
{
    return gen::suchThat(BetweenZeroAnd100(), [](const std::vector<uint256>& hashes) {
        return hashes.size() > 0 && hashes.size() <= 100;
    });
}

/** Returns an arbitrary CMerkleBlock */
template <>
struct Arbitrary<CMerkleBlock> {
    static Gen<CMerkleBlock> arbitrary()
    {
        return gen::map(gen::arbitrary<std::pair<CMerkleBlock, std::set<uint256>>>(),
            [](const std::pair<const CMerkleBlock&, const std::set<uint256>&>& mb_with_txids) {
                const CMerkleBlock& mb = mb_with_txids.first;
                return mb;
            });
    };
};

/** Generates a CPartialMerkleTree and returns the PartialMerkleTree along
   * with the txids that should be matched inside of it */
template <>
struct Arbitrary<std::pair<CPartialMerkleTree, std::vector<uint256>>> {
    static Gen<std::pair<CPartialMerkleTree, std::vector<uint256>>> arbitrary()
    {
        return gen::map(Between1And100(), [](const std::vector<uint256>& txids) {
            std::vector<bool> matches;
            std::vector<uint256> matched_txs;
            for (unsigned int i = 0; i < txids.size(); i++) {
                //pretty naive to include every other txid in the merkle block
                //but this will work for now.
                matches.push_back(i % 2 == 1);
                if (i % 2 == 1) {
                    matched_txs.push_back(txids[i]);
                }
            }
            return std::make_pair(CPartialMerkleTree(txids, matches), matched_txs);
        });
    };
};

template <>
struct Arbitrary<CPartialMerkleTree> {
    static Gen<CPartialMerkleTree> arbitrary()
    {
        return gen::map(gen::arbitrary<std::pair<CPartialMerkleTree, std::vector<uint256>>>(),
            [](const std::pair<const CPartialMerkleTree&, const std::vector<uint256>&>& p) {
                return p.first;
            });
    };
};
} // namespace rc
#endif
