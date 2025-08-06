#ifndef BITCOIN_KERNEL_MEMPOOL_PRIORITY_H
#define BITCOIN_KERNEL_MEMPOOL_PRIORITY_H

#include <consensus/amount.h>
#include <cstdint>

// Constants for priority calculation
static const int64_t STAKE_PRIORITY_SCALE = 1;
static const int64_t STAKE_DURATION_7_DAYS = 7 * 24 * 60 * 60;
static const int64_t STAKE_DURATION_30_DAYS = 30 * 24 * 60 * 60;

// Priority point values
static const int64_t STAKE_PRIORITY_POINTS = 50;
static const int64_t FEE_PRIORITY_POINTS = 50;
static const int64_t STAKE_DURATION_7_DAYS_POINTS = 10;
static const int64_t STAKE_DURATION_30_DAYS_POINTS = 20;
static const int64_t CONGESTION_PENALTY = -10;

/**
 * Calculate priority based on stake amount.
 *
 * @param nStakeAmount The amount of BGD staked.
 * @return The priority score based on the stake.
 */
int64_t CalculateStakePriority(CAmount nStakeAmount);

/**
 * Calculate priority based on transaction fees.
 *
 * @param nFee The transaction fee in satoshis.
 * @return The priority score based on the fee.
 */
int64_t CalculateFeePriority(CAmount nFee);

/**
 * Calculate priority based on stake duration.
 *
 * @param nStakeDuration The duration of the stake in seconds.
 * @return The priority score based on the stake duration.
 */
int64_t CalculateStakeDurationPriority(int64_t nStakeDuration);

#endif // BITCOIN_KERNEL_MEMPOOL_PRIORITY_H
