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
bool CheckProofOfStake(const CBlock& block, const CBlockIndex* prevBlock, const COutPoint& outpoint, const CTransaction& txPayment)
{
    const CTransaction tx = block.vtx[1];
    if (!tx.IsCoinStake())
        return error("CheckProofOfStake() : called on non-coinstake %s", tx.GetHash().ToString().c_str());

    auto pairOut = std::make_pair(outpoint.hash, outpoint.n);
    CAmount nAmountCollateral = (outpoint.n == 1 ? MASTERNODE_COLLATERAL : SYSTEMNODE_COLLATERAL);

    uint256 nStakeModifier = prevBlock->GetAncestor(prevBlock->nHeight - 100)->GetBlockHash();

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