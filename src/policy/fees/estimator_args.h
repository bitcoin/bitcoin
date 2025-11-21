// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_ESTIMATOR_ARGS_H
#define BITCOIN_POLICY_FEES_ESTIMATOR_ARGS_H

#include <util/fs.h>

class ArgsManager;

/** @return The block policy fee estimator data file path. */
fs::path BlockPolicyFeeestPath(const ArgsManager& argsman);

/** @return The mempool policy fee estimator data file path. */
fs::path MempoolPolicyFeeestPath(const ArgsManager& argsman);

#endif // BITCOIN_POLICY_FEES_ESTIMATOR_ARGS_H
