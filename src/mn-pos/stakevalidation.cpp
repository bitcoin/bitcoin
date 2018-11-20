#include "stakevalidation.h"
#include "kernel.h"

#include <primitives/block.h>
#include <chain.h>
#include <pubkey.h>
#include <wallet.h>
#include <util.h>
#include <arith_uint256.h>

bool CheckBlockSignature(const CBlock& block, const CPubKey& pubkeyMasternode)
{
    uint256 hashBlock = block.GetHash();

    return pubkeyMasternode.Verify(hashBlock, block.vchBlockSig);
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(const CBlock& block, const CBlockIndex* prevBlock, const COutPoint& outpointStakePointer)
{
    const CTransaction tx = block.vtx[1];
    if (!tx.IsCoinStake())
        return error("CheckProofOfStake() : called on non-coinstake %s", tx.GetHash().ToString().c_str());

    // Get the stake modifier
    auto pindexModifier = prevBlock->GetAncestor(prevBlock->nHeight - Params().KernelModifierOffset());
    if (!pindexModifier)
        return error("CheckProofOfStake() : could not find modifier index for stake");
    uint256 nStakeModifier = pindexModifier->GetBlockHash();

    // Get the correct amount for the collateral
    CAmount nAmountCollateral = 0;
    if (outpointStakePointer.n == 1)
        nAmountCollateral = MASTERNODE_COLLATERAL;
    else if (outpointStakePointer.n == 2)
        nAmountCollateral = SYSTEMNODE_COLLATERAL;
    else
        return error("%s: Stake pointer is neither pos 1 or 2", __func__);

    // Reconstruct the kernel that created the stake
    auto pairOut = std::make_pair(outpointStakePointer.hash, outpointStakePointer.n);
    Kernel kernel(pairOut, nAmountCollateral, nStakeModifier, prevBlock->GetBlockTime(), block.nTime);

    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow)
        return error("CheckProofOfStake() : nBits below minimum stake");

    LogPrintf("%s : %s\n", __func__, kernel.ToString());

    return kernel.IsValidProof(ArithToUint256(bnTarget));
}