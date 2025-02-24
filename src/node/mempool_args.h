// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MEMPOOL_ARGS_H
#define BITCOIN_NODE_MEMPOOL_ARGS_H

#include <util/result.h>

#include <cstdint>
#include <string>

class ArgsManager;
class CChainParams;
struct bilingual_str;
namespace kernel {
struct MemPoolOptions;
};

[[nodiscard]] util::Result<int32_t> ParseDustDynamicOpt(const std::string& optstr, unsigned int max_fee_estimate_blocks);

/**
 * Overlay the options set in \p argsman on top of corresponding members in \p mempool_opts.
 * Returns an error if one was encountered.
 *
 * @param[in]  argsman The ArgsManager in which to check set options.
 * @param[in,out] mempool_opts The MemPoolOptions to modify according to \p argsman.
 */
[[nodiscard]] util::Result<void> ApplyArgsManOptions(const ArgsManager& argsman, const CChainParams& chainparams, kernel::MemPoolOptions& mempool_opts);


#endif // BITCOIN_NODE_MEMPOOL_ARGS_H
