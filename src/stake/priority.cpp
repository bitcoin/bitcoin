#include <stake/priority.h>
#include <kernel/mempool_priority.h>
#include <algorithm>

long CalculateStakePriority(long nStakeAmount)
{
    long priority;
    if (nStakeAmount >= 1000 * COIN) {
        priority = STAKE_PRIORITY_POINTS;
    } else {
        priority = (nStakeAmount / COIN) * STAKE_PRIORITY_SCALE;
    }
    return std::max(0L, priority);
}

long CalculateFeePriority(long nFee)
{
    long priority;
    if (nFee < 100) {
        priority = 0;
    } else {
        priority = 1 + (nFee - 100) / 1000;
        priority = (priority > FEE_PRIORITY_POINTS) ? FEE_PRIORITY_POINTS : priority;
    }
    return std::max(0L, priority);
}

long CalculateStakeDurationPriority(long nStakeDuration)
{
    long priority;
    if (nStakeDuration >= STAKE_DURATION_30_DAYS) {
        priority = STAKE_DURATION_30_DAYS_POINTS;
    } else if (nStakeDuration >= STAKE_DURATION_7_DAYS) {
        priority = STAKE_DURATION_7_DAYS_POINTS;
    } else {
        priority = 0;
    }
    return std::max(0L, priority);
}
