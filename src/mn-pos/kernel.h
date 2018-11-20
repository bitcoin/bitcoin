#ifndef CROWN_CORE_KERNEL_H
#define CROWN_CORE_KERNEL_H

#include "uint256.h"

class arith_uint256;
class StakePointer;

class Kernel
{
public:
    Kernel(const std::pair<uint256, unsigned int>& outpoint, const uint64_t nAmount, const uint256& nStakeModifier,
            const uint64_t& nTimeBlockFrom, const uint64_t& nTimeStake);
    uint256 GetStakeHash();
    uint64_t GetTime() const;
    bool IsValidProof(const uint256& nTarget);
    void SetStakeTime(uint64_t nTime);
    std::string ToString();

    static bool CheckProof(const arith_uint256& target, const arith_uint256& hash, const uint64_t nAmount);

private:
    std::pair<uint256, unsigned int> m_outpoint;
    uint256 m_nStakeModifier;
    uint64_t m_nTimeBlockFrom;
    uint64_t m_nTimeStake;
    uint64_t m_nAmount;
};

#endif //CROWN_CORE_KERNEL_H
