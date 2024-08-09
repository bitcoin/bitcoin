// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAIN_H
#define BITCOIN_NODE_CHAIN_H

#include <functional>

class CBlockIndex;
class CChain;
class CThreadInterrupt;
namespace node {
class BlockManager;
} // namespace node

namespace node {
//! Read block data and/or undo data from disk and update BlockInfo with pointers and errors.
bool ReadBlockData(node::BlockManager& blockman, const CBlockIndex& block, CBlock* data, CBlockUndo* undo_data, interfaces::BlockInfo& info);

//! Send blockConnected and blockDisconnected notifications needed to sync from
//! a specified block to the chain tip. This sync function locks the ::cs_main
//! mutex intermittently, releasing it while sending notifications and reading
//! block data, so it follows the tip if the tip changes, and follows any
//! reorgs.
//!
//! @param chain - chain to sync to
//! @param block - starting block to sync from
//! @param notifications - object to send notifications to
//! @param interrupt - flag to interrupt the sync
//! @param on_sync - optional callback invoked when reaching the chain tip
//!                  while cs_main is still held, before sending a final
//!                  blockConnected notification. This can be used to
//!                  synchronously register for new notifications.
//! @return true if synced, false if interrupted
bool SyncChain(BlockManager& blockman, const CChain& chain, const CBlockIndex* block, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt, std::function<void()> on_sync);
} // namespace node

#endif // BITCOIN_NODE_CHAIN_H
