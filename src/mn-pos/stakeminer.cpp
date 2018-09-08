#include "stakeminer.h"
#include "kernel.h"

//! Search a specific period of timestamps to see if a valid proof hash is created
bool SearchTimeSpan(Kernel& kernel, uint32_t nTimeStart, uint32_t nTimeEnd, const uint256& nTarget)
{
    uint64_t nTimeStake = nTimeStart;
    kernel.SetStakeTime(nTimeStart);

    while (!kernel.IsValidProof(nTarget)) {
        ++nTimeStake;
        kernel.SetStakeTime(nTimeStake);

        //Searched through the requested period
        if (nTimeStake > nTimeEnd)
            break;
    }

    return kernel.IsValidProof(nTarget);
}


