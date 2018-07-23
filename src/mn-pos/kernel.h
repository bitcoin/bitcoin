#ifndef CROWN_CORE_KERNEL_H
#define CROWN_CORE_KERNEL_H

#include "../arith_uint256.h"

class CKernel
{
private:
    arith_uint256 txidOutPoint;
    unsigned int nOutPoint;
    arith_uint256 nStakeModifier;
    uint64_t nTimeBlockFrom;
    uint64_t nTimeStake;
public:
    CKernel(const arith_uint256& txidOutPoint, unsigned int nOutPoint, const arith_uint256& nStakeModifier,
            const uint64_t& nTimeBlockFrom, const uint64_t& nTimeStake);
    arith_uint256 GetStakeHash();
    bool IsValidProof(const arith_uint256& nTarget, int64_t nAmount);
    void SetStakeTime(uint64_t nTime);
};

#endif //CROWN_CORE_KERNEL_H
