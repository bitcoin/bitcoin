// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MINING_ARGS_H
#define BITCOIN_NODE_MINING_ARGS_H

#include <policy/feerate.h>
#include <policy/policy.h>
#include <util/result.h>

class ArgsManager;

namespace node {

static const bool DEFAULT_PRINT_MODIFIED_FEE = false;

/**
 * Block template creation defaults and limits configured for the node.
 */
struct MiningArgs {
    CFeeRate blockMinFeeRate{DEFAULT_BLOCK_MIN_TX_FEE};
    bool print_modified_fee{DEFAULT_PRINT_MODIFIED_FEE};
    /**
     * The default maximum block weight.
     */
    size_t default_block_max_weight{DEFAULT_BLOCK_MAX_WEIGHT};
    /**
     * The default reserved weight for the fixed-size block header,
     * transaction count and coinbase transaction.
     */
    size_t default_block_reserved_weight{DEFAULT_BLOCK_RESERVED_WEIGHT};
};

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
