
#include "kernel.h"
#include "../streams.h"
#include "../hash.h"

/*
 * A 'proof hash' (also referred to as a 'kernel') is comprised of the following:
 * Masternode's outpoint: the outpoint of the masternode is the collateral transaction that the masternode is created by. This is unique to each masternode.
 * Stake Modifier: entropy collected from points in the future of the blockchain, far enough after the outpoint was created to prevent any 'grinding'
 * Block Time: The time that the block containing the outpoint was added to the blockchain
 * Stake Time: The time that the stake hash is being created
 */
CKernel::CKernel(const arith_uint256& txidOutPoint, const unsigned int nOutPoint, const arith_uint256& nStakeModifier,
                 const uint64_t& nTimeBlockFrom, const uint64_t& nTimeStake)
{
    this->txidOutPoint = txidOutPoint;
    this->nOutPoint = nOutPoint;
    this->nStakeModifier = nStakeModifier;
    this->nTimeBlockFrom = nTimeBlockFrom;
    this->nTimeStake = nTimeStake;
}

arith_uint256 CKernel::GetStakeHash()
{
    CDataStream ss(SER_GETHASH, 0);
    ss << txidOutPoint << nOutPoint << nStakeModifier << nTimeBlockFrom << nTimeStake;
    uint256 hash = Hash(ss.begin(), ss.end());

    return UintToArith256(hash);
}

bool CKernel::IsValidProof(const arith_uint256& nTarget, int64_t nAmount)
{
    return GetStakeHash() < nAmount * nTarget;
}

void CKernel::SetStakeTime(uint64_t nTime)
{
    this->nTimeStake = nTime;
}