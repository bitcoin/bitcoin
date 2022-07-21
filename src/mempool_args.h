// Copyright (c) 2022 The Revolt Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef REVOLT_MEMPOOL_ARGS_H
#define REVOLT_MEMPOOL_ARGS_H

class ArgsManager;
namespace kernel {
struct MemPoolOptions;
};

/**
 * Overlay the options set in \p argsman on top of corresponding members in \p mempool_opts.
 *
 * @param[in]  argsman The ArgsManager in which to check set options.
 * @param[in,out] mempool_opts The MemPoolOptions to modify according to \p argsman.
 */
void ApplyArgsManOptions(const ArgsManager& argsman, kernel::MemPoolOptions& mempool_opts);


#endif // REVOLT_MEMPOOL_ARGS_H
