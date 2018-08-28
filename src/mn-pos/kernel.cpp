
#include <iostream>
#include <arith_uint256.h>
#include "kernel.h"
#include "../streams.h"
#include "../hash.h"
#include "proofpointer.h"

/*
 * A 'proof hash' (also referred to as a 'kernel') is comprised of the following:
 * Masternode's outpoint: the outpoint of the masternode is the collateral transaction that the masternode is created by. This is unique to each masternode.
 * Stake Modifier: entropy collected from points in the future of the blockchain, far enough after the outpoint was created to prevent any 'grinding'
 * Block Time: The time that the block containing the outpoint was added to the blockchain
 * Stake Time: The time that the stake hash is being created
 */
Kernel::Kernel(const std::pair<uint256, unsigned int>& outpoint, const uint64_t nAmount, const uint256& nStakeModifier,
                 const uint64_t& nTimeBlockFrom, const uint64_t& nTimeStake)
{
    m_outpoint = outpoint;
    m_nAmount = nAmount;
    m_nStakeModifier = nStakeModifier;
    m_nTimeBlockFrom = nTimeBlockFrom;
    m_nTimeStake = nTimeStake;
}

uint64_t Kernel::GetAmount() const
{
    return m_nAmount;
}

uint256 Kernel::GetStakeHash()
{
    CDataStream ss(SER_GETHASH, 0);
    ss << m_outpoint.first << m_outpoint.second << m_nStakeModifier << m_nTimeBlockFrom << m_nTimeStake;
    return Hash(ss.begin(), ss.end());
}

uint64_t Kernel::GetTime() const
{
    return m_nTimeStake;
}

bool Kernel::CheckProof(const arith_uint256& target, const arith_uint256& hash, const uint64_t nAmount)
{
    return hash < nAmount * target;
}

bool Kernel::IsValidProof(const uint256& nTarget)
{
    arith_uint256 target = UintToArith256(nTarget);
    arith_uint256 hashProof = UintToArith256(GetStakeHash());
    return CheckProof(target, hashProof, m_nAmount);
}

void Kernel::SetStakeTime(uint64_t nTime)
{
    m_nTimeStake = nTime;
}