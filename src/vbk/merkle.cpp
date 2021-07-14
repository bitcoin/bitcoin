// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "merkle.hpp"
#include <consensus/merkle.h>
#include <hash.h>
#include <vbk/pop_common.hpp>
#include <vbk/pop_service.hpp>

namespace VeriBlock {

uint256 TopLevelMerkleRoot(const CBlockIndex* prevIndex, const CBlock& block, bool* mutated)
{
    using altintegration::CalculateTopLevelMerkleRoot;
    auto& altParams = VeriBlock::GetPop().getConfig().getAltParams();

    // first, build regular merkle root from transactions
    auto txRoot = BlockMerkleRoot(block, mutated);

    // if POP is not enabled for 'block' , use original txRoot as merkle root
    if (!VeriBlock::isPopEnabled()) {
        return txRoot;
    }

    const auto height = prevIndex == nullptr ? 0 : prevIndex->nHeight + 1;
    if (!Params().isPopActive(height)) {
        return txRoot;
    }

    // POP is enabled.

    // then, find BlockIndex in AltBlockTree.
    // if returns nullptr, 'prevIndex' is behind bootstrap block.
    auto* prev = VeriBlock::GetAltBlockIndex(prevIndex);
    auto tlmr = CalculateTopLevelMerkleRoot(txRoot.asVector(), block.popData, prev, altParams);
    return uint256(tlmr.asVector());
}

bool VerifyTopLevelMerkleRoot(const CBlock& block, const CBlockIndex* pprevIndex, BlockValidationState& state)
{
    bool mutated = false;
    uint256 hashMerkleRoot2 = VeriBlock::TopLevelMerkleRoot(pprevIndex, block, &mutated);

    if (block.hashMerkleRoot != hashMerkleRoot2) {
        return state.Invalid(BlockValidationResult ::BLOCK_MUTATED, "bad-txnmrklroot",
            strprintf("hashMerkleRoot mismatch. expected %s, got %s", hashMerkleRoot2.GetHex(), block.hashMerkleRoot.GetHex()));
    }

    // Check for merkle tree malleability (CVE-2012-2459): repeating sequences
    // of transactions in a block without affecting the merkle root of a block,
    // while still invalidating it.
    if (mutated) {
        return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-txns-duplicate", "duplicate transaction");
    }

    return true;
}

} // namespace VeriBlock
