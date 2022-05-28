// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_KERNEL_CHAINSTATEMANAGER_OPTS_H
#define SYSCOIN_KERNEL_CHAINSTATEMANAGER_OPTS_H

#include <cstdint>
#include <functional>

class CChainParams;

/**
 * An options struct for `ChainstateManager`, more ergonomically referred to as
 * `ChainstateManager::Options` due to the using-declaration in
 * `ChainstateManager`.
 */
struct ChainstateManagerOpts {
    const CChainParams& chainparams;
    const std::function<int64_t()> adjusted_time_callback{nullptr};
};

#endif // SYSCOIN_KERNEL_CHAINSTATEMANAGER_OPTS_H
