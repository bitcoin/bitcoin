#ifndef BITCOIN_SRC_VBK_MERKLE_HPP
#define BITCOIN_SRC_VBK_MERKLE_HPP

#include "vbk/config.hpp"
#include "vbk/service_locator.hpp"
#include "vbk/util_service.hpp"

#include <chain.h>            // for CBlockIndex
#include <consensus/merkle.h> // for BlockMerkleRoot
#include <consensus/validation.h>
#include <primitives/block.h> // for CBlock
#include <primitives/transaction.h>

namespace VeriBlock {

inline int GetPopMerkleRootCommitmentIndex(const CBlock& block)
{
    int commitpos = -1;
    if (!block.vtx.empty()) {
        for (size_t o = 0; o < block.vtx[0]->vout.size(); o++) {
            auto& s = block.vtx[0]->vout[o].scriptPubKey;
            if (s.size() >= 37 && s[0] == OP_RETURN && s[1] == 0x23 && s[2] == 0x3a && s[3] == 0xe6 && s[4] == 0xca) {
                commitpos = o;
            }
        }
    }
    return commitpos;
}

inline uint256 BlockPopTxMerkleRoot(const CBlock& block)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    for (size_t s = 0; s < block.vtx.size(); s++) {
        if (isPopTx(*block.vtx[s])) {
            leaves[s] = block.vtx[s]->GetWitnessHash();
        }
    }
    return ComputeMerkleRoot(std::move(leaves), nullptr);
}

inline uint256 TopLevelMerkleRoot(const CBlockIndex* prevIndex, const CBlock& block, bool* mutated = nullptr)
{
    auto& util = VeriBlock::getService<VeriBlock::UtilService>();
    if (prevIndex == nullptr) {
        // special case: this is genesis block
        KeystoneArray keystones;
        return util.makeTopLevelRoot(0, keystones, BlockMerkleRoot(block, mutated));
    }

    auto keystones = util.getKeystoneHashesForTheNextBlock(prevIndex);
    return util.makeTopLevelRoot(prevIndex->nHeight + 1, keystones, BlockMerkleRoot(block, mutated));
}

inline bool VerifyTopLevelMerkleRoot(const CBlock& block, BlockValidationState& state, const CBlockIndex* pprevIndex)
{
    bool mutated = false;
    uint256 hashMerkleRoot2 = VeriBlock::TopLevelMerkleRoot(pprevIndex, block, &mutated);

    if (block.hashMerkleRoot != hashMerkleRoot2) {
        return state.Invalid(BlockValidationResult ::BLOCK_MUTATED, "bad-txnmrklroot", "hashMerkleRoot mismatch");
    }

    // Check for merkle tree malleability (CVE-2012-2459): repeating sequences
    // of transactions in a block without affecting the merkle root of a block,
    // while still invalidating it.
    if (mutated) {
        return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-txns-duplicate", "duplicate transaction");
    }

    // Add PopMerkleRoot commitment validation
    int commitpos = GetPopMerkleRootCommitmentIndex(block);
    if (commitpos != -1) {
        uint256 popMerkleRoot = BlockPopTxMerkleRoot(block);
        if (!memcpy(popMerkleRoot.begin(), &block.vtx[0]->vout[commitpos].scriptPubKey[4], 32)) {
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-pop-tx-root-commitment", "pop merkle root mismatch");
        }
    } else {
        // If block is not genesis
        if (pprevIndex != nullptr) {
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-pop-tx-root-commitment", "commitment is missing");
        }
    }

    return true;
}

inline CTxOut addPopTransactionRootIntoCoinbaseCommitment(const CBlock& block)
{
    CTxOut out;
    out.nValue = 0;
    out.scriptPubKey.resize(37);
    out.scriptPubKey[0] = OP_RETURN;
    out.scriptPubKey[1] = 0x23;
    out.scriptPubKey[2] = 0x3a;
    out.scriptPubKey[3] = 0xe6;
    out.scriptPubKey[4] = 0xca;

    uint256 popMerkleRoot = BlockPopTxMerkleRoot(block);
    memcpy(&out.scriptPubKey[5], popMerkleRoot.begin(), 32);

    return out;
}

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_MERKLE_HPP
