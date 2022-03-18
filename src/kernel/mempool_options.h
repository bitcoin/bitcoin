// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KERNEL_MEMPOOL_OPTIONS_H
#define BITCOIN_KERNEL_MEMPOOL_OPTIONS_H

class CBlockPolicyEstimator;

namespace kernel {
/**
 * Options struct containing options for constructing a CTxMemPool. Default
 * constructor populates the struct with sane default values which can be
 * modified.
 *
 * Most of the time, this struct should be referenced as CTxMemPool::Options.
 */
struct MemPoolOptions {
    /* Used to estimate appropriate transaction fees. */
    CBlockPolicyEstimator* estimator{nullptr};
    /* The ratio used to determine how often sanity checks will run.  */
    int check_ratio{0};
};
} // namespace kernel

#endif // BITCOIN_KERNEL_MEMPOOL_OPTIONS_H
