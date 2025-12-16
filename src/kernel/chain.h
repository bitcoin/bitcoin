// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_CHAIN_H
#define BITCOIN_KERNEL_CHAIN_H

#include<iostream>

class CBlock;
class CBlockIndex;
namespace interfaces {
struct BlockInfo;
} // namespace interfaces

namespace kernel {
struct ChainstateRole;
//! Return data from block index.
interfaces::BlockInfo MakeBlockInfo(const CBlockIndex* block_index, const CBlock* data = nullptr);
std::ostream& operator<<(std::ostream& os, const ChainstateRole& role);
} // namespace kernel

#endif // BITCOIN_KERNEL_CHAIN_H
