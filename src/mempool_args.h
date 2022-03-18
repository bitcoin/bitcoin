// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEMPOOL_ARGS_H
#define BITCOIN_MEMPOOL_ARGS_H

class ArgsManager;
namespace kernel {
struct MemPoolOptions;
};

void ApplyArgsManOptions(const ArgsManager& argsman, kernel::MemPoolOptions& mempool_opts);

#endif // BITCOIN_MEMPOOL_ARGS_H
