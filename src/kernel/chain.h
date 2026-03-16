// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_CHAIN_H
#define BITCOIN_KERNEL_CHAIN_H

#include <attributes.h>

#include <iostream>

class CBlock;
class CBlockIndex;
class CBlockUndo;
class uint256;

namespace interfaces {
//! Block data sent with blockConnected, blockDisconnected notifications.
struct BlockInfo {
    const uint256& hash;
    const uint256* prev_hash = nullptr;
    int height = -1;
    int file_number = -1;
    unsigned data_pos = 0;
    const CBlock* data = nullptr;
    const CBlockUndo* undo_data = nullptr;
    // The maximum time in the chain up to and including this block.
    // A timestamp that can only move forward.
    unsigned int chain_time_max{0};

    BlockInfo(const uint256& hash LIFETIMEBOUND) : hash(hash) {}
};
} // namespace interfaces

namespace kernel {
struct ChainstateRole;
//! Return data from block index.
interfaces::BlockInfo MakeBlockInfo(const CBlockIndex* block_index, const CBlock* data = nullptr);
std::ostream& operator<<(std::ostream& os, const ChainstateRole& role);
} // namespace kernel

#endif // BITCOIN_KERNEL_CHAIN_H
