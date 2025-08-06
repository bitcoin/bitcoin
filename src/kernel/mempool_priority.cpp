#include <kernel/mempool_priority.h>

int64_t CalculateStakePriority(CAmount nStakeAmount)
{
    if (nStakeAmount >= 1000 * COIN) {
        return STAKE_PRIORITY_POINTS;
    }
    return (nStakeAmount / COIN) * STAKE_PRIORITY_SCALE;
}

int64_t CalculateFeePriority(CAmount nFee)
{
    int64_t priority = (nFee / 1000) * FEE_PRIORITY_SCALE;
    return (priority > FEE_PRIORITY_POINTS) ? FEE_PRIORITY_POINTS : priority;
}

int64_t CalculateStakeDurationPriority(int64_t nStakeDuration)
{
    if (nStakeDuration >= STAKE_DURATION_30_DAYS) {
        return STAKE_DURATION_30_DAYS_POINTS;
    }
    if (nStakeDuration >= STAKE_DURATION_7_DAYS) {
        return STAKE_DURATION_7_DAYS_POINTS;
    }
    return 0;
}
