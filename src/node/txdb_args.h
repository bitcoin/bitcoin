// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TXDB_ARGS_H
#define BITCOIN_NODE_TXDB_ARGS_H

namespace kernel {
struct BlockTreeDBOpts;
struct CoinsViewDBOpts;
} //namespace kernel

class ArgsManager;

namespace node {

void ApplyArgsManOptions(const ArgsManager& argsman, kernel::BlockTreeDBOpts& opts);
void ApplyArgsManOptions(const ArgsManager& argsman, kernel::CoinsViewDBOpts& coinsviewdb_opts);

} // namespace node

#endif // BITCOIN_NODE_TXDB_ARGS_H
