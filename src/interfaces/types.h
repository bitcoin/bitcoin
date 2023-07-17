// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_TYPES_H
#define BITCOIN_INTERFACES_TYPES_H

#include <uint256.h>

#include <attributes.h>

class CBlock;
class CBlockUndo;
class uint256;

namespace interfaces {

//! Hash/height pair to help track and identify blocks.
struct BlockRef {
    uint256 hash;
    int height = -1;
};

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
    // True if notification comes from a background sync thread while initially
    // attaching to the chain. False if notification comes from the validation
    // interface queue.
    bool background_sync{false};
    // Whether block data has been flushed and saved to disk yet. FLUSHED_TIP
    // and FLUSHED both indicate that block data has been flushed, but
    // FLUSHED_TIP additionally means this block is the *latest* block that's
    // been flushed. FLUSHED_TIP is useful for chain clients that are syncing
    // with the node and want to flush their state at the same points as the
    // node, without flushing too frequently.
    enum { UNFLUSHED, FLUSHED, FLUSHED_TIP } status{UNFLUSHED};

    BlockInfo(const uint256& hash LIFETIMEBOUND) : hash(hash) {}
};

} // namespace interfaces

#endif // BITCOIN_INTERFACES_TYPES_H
