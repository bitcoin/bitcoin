// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAINSTATEMANAGER_ARGS_H
#define BITCOIN_NODE_CHAINSTATEMANAGER_ARGS_H

class ArgsManager;
namespace kernel {
struct ChainstateManagerOpts;
};

namespace node {
/**
 * Overlay the options set in \p argsman on top of corresponding members in \p chainman_opts.
 */
void ApplyArgsManOptions(const ArgsManager& argsman, kernel::ChainstateManagerOpts& chainman_opts);
} // namespace node


#endif // BITCOIN_NODE_CHAINSTATEMANAGER_ARGS_H
