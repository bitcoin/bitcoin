#include "stakeminer.h"
#include "kernel.h"

bool SearchTimeSpan(CKernel& kernel, uint64_t nTimeStart, uint64_t nTimeEnd, const uint64_t nAmount, const arith_uint256& nTarget)
{
    uint64_t nTimeStake = nTimeStart;
    kernel.SetStakeTime(nTimeStart);

    bool isValid = kernel.IsValidProof(nTarget, nAmount);
    while (!isValid && nTimeStake <= nTimeEnd) {
        ++nTimeStake;
        kernel.SetStakeTime(nTimeStake);
        isValid = kernel.IsValidProof(nTarget, nAmount);
    }

    return isValid;
}


