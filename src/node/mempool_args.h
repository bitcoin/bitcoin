// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_NODE_MEMPOOL_ARGS_H
#define SYSCOIN_NODE_MEMPOOL_ARGS_H

#include <optional>

class ArgsManager;
class CChainParams;
struct bilingual_str;
namespace kernel {
struct MemPoolOptions;
};

/**
 * Overlay the options set in \p argsman on top of corresponding members in \p mempool_opts.
 * Returns an error if one was encountered.
 *
 * @param[in]  argsman The ArgsManager in which to check set options.
 * @param[in,out] mempool_opts The MemPoolOptions to modify according to \p argsman.
 */
[[nodiscard]] std::optional<bilingual_str> ApplyArgsManOptions(const ArgsManager& argsman, const CChainParams& chainparams, kernel::MemPoolOptions& mempool_opts);


#endif // SYSCOIN_NODE_MEMPOOL_ARGS_H
