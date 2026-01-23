// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAIN_H
#define BITCOIN_NODE_CHAIN_H

class CBlock;
class CBlockIndex;
namespace interfaces {
struct BlockInfo;
} // namespace interfaces

namespace node {
//! Return data from block index.
interfaces::BlockInfo MakeBlockInfo(const CBlockIndex* block_index, const CBlock* data = nullptr);
} // namespace node

#endif // BITCOIN_NODE_CHAIN_H
