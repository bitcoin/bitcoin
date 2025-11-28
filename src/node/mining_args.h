// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MINING_ARGS_H
#define BITCOIN_NODE_MINING_ARGS_H

#include <node/mining.h>
#include <util/result.h>

class ArgsManager;

namespace node {
/**
 * Overlay the options set in \p args on top of corresponding members in
 * \p mining_args. Returns an error if one was encountered.
 *
 * @param[in]  args The ArgsManager in which to check set options.
 * @param[in,out] mining_args The MiningArgs to modify according to \p args.
 */
[[nodiscard]] util::Result<void> ApplyArgsManOptions(const ArgsManager& args, MiningArgs& mining_args);
} // namespace node

#endif // BITCOIN_NODE_MINING_ARGS_H
