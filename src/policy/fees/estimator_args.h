// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_ESTIMATOR_ARGS_H
#define BITCOIN_POLICY_FEES_ESTIMATOR_ARGS_H

#include <util/fs.h>

class ArgsManager;

/** Move a legacy fee_estimates.dat file to the current block policy fee estimator path, if needed. */
void MaybeMigrateLegacyFeeEstimates(const ArgsManager& argsman);

/** @return The block policy fee estimator data file path. */
fs::path BlockPolicyFeeEstPath(const ArgsManager& argsman);

/** @return The mempool policy estimator data file path. */
fs::path MempoolPolicyEstimatorPath(const ArgsManager& argsman);

#endif // BITCOIN_POLICY_FEES_ESTIMATOR_ARGS_H
