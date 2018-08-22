
#include <iostream>
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
CKernel::CKernel(const std::pair<uint256, unsigned int>& outpoint, const uint64_t nAmount, const uint256& nStakeModifier,
                 const uint64_t& nTimeBlockFrom, const uint64_t& nTimeStake)
{
    this->outpoint = outpoint;
    this->nAmount = nAmount;
    this->nStakeModifier = nStakeModifier;
    this->nTimeBlockFrom = nTimeBlockFrom;
    this->nTimeStake = nTimeStake;
}

uint64_t CKernel::GetAmount() const
{
    return nAmount;
}

uint256 CKernel::GetStakeHash()
{
    CDataStream ss(SER_GETHASH, 0);
    ss << outpoint.first << outpoint.second << nStakeModifier << nTimeBlockFrom << nTimeStake;
    return Hash(ss.begin(), ss.end());
}

uint64_t CKernel::GetTime() const
{
    return nTimeStake;
}

bool CKernel::CheckProof(const arith_uint256& target, const arith_uint256& hash, const uint64_t nAmount)
{
    return hash < nAmount * target;
}

bool CKernel::IsValidProof(const uint256& nTarget)
{
    arith_uint256 target = UintToArith256(nTarget);
    arith_uint256 hashProof = UintToArith256(GetStakeHash());
    return CheckProof(target, hashProof, nAmount);
}

void CKernel::SetStakeTime(uint64_t nTime)
{
    this->nTimeStake = nTime;
}