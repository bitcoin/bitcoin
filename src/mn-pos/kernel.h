#ifndef CROWN_CORE_KERNEL_H
#define CROWN_CORE_KERNEL_H

#include "../arith_uint256.h"
#include "../uint256.h"

class CKernel
{
private:
    std::pair<uint256, unsigned int> outpoint;
    uint256 nStakeModifier;
    uint64_t nTimeBlockFrom;
    uint64_t nTimeStake;
    uint64_t nAmount;
public:
    CKernel(const std::pair<uint256, unsigned int>& outpoint, const uint64_t nAmount, const uint256& nStakeModifier,
            const uint64_t& nTimeBlockFrom, const uint64_t& nTimeStake);
    uint64_t GetAmount() const;
    uint256 GetStakeHash();
    uint64_t GetTime() const;
    bool IsValidProof(const uint256& nTarget);
    void SetStakeTime(uint64_t nTime);

    static bool CheckProof(const arith_uint256& target, const arith_uint256& hash, const uint64_t nAmount);
};

#endif //CROWN_CORE_KERNEL_H
