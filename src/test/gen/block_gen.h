#ifndef BITCOIN_TEST_GEN_BLOCK_GEN_H
#define BITCOIN_TEST_GEN_BLOCK_GEN_H

#include "test/gen/crypto_gen.h"
#include "test/gen/transaction_gen.h"
#include "uint256.h"
#include "primitives/block.h"

#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/Gen.h>


typedef std::tuple<int32_t, uint256, uint256, uint32_t, uint32_t, uint32_t> BlockHeaderTup;

/** Generator for the primitives of a block header */
rc::Gen<BlockHeaderTup> BlockHeaderPrimitives();

namespace rc
{
/** Generator for a new CBlockHeader */
template <>
struct Arbitrary<CBlockHeader> {
    static Gen<CBlockHeader> arbitrary()
    {
        return gen::map(BlockHeaderPrimitives(), [](const BlockHeaderTup& primitives) {
            int32_t nVersion;
            uint256 hashPrevBlock;
            uint256 hashMerkleRoot;
            uint32_t nTime;
            uint32_t nBits;
            uint32_t nNonce;
            std::tie(nVersion, hashPrevBlock, hashMerkleRoot, nTime, nBits, nNonce) = primitives;
            CBlockHeader header;
            header.nVersion = nVersion;
            header.hashPrevBlock = hashPrevBlock;
            header.hashMerkleRoot = hashMerkleRoot;
            header.nTime = nTime;
            header.nBits = nBits;
            header.nNonce = nNonce;
            return header;
        });
    };
};

/** Generator for a new CBlock */
template <>
struct Arbitrary<CBlock> {
    static Gen<CBlock> arbitrary()
    {
        return gen::map(gen::nonEmpty<std::vector<CTransactionRef>>(), [](const std::vector<CTransactionRef>& refs) {
            CBlock block;
            block.vtx = refs;
            return block;
        });
    }
};
} //namespace rc
#endif
