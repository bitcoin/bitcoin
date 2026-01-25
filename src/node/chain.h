// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAIN_H
#define BITCOIN_NODE_CHAIN_H

#include <functional>

#include <interfaces/chain.h>

class CBlock;
class CBlockIndex;
class CBlockUndo;
class CThreadInterrupt;
class Chainstate;
namespace interfaces {
struct BlockInfo;
} // namespace interfaces

namespace node {
class BlockManager;

//! Return data from block index.
//! By default assumes data for an UPDATING notification and the block is not
//! flushed. But if chainstate is provided, this assumes a SYNCING notification
//! and flush status will be looked up in the ChainState.
interfaces::BlockInfo MakeBlockInfo(const CBlockIndex* block_index, const CBlock* data = nullptr, const Chainstate* chainstate = nullptr);

//! Read block data and/or undo data from disk and update BlockInfo with pointers and errors.
bool ReadBlockData(BlockManager& blockman, const CBlockIndex& block, CBlock* data, CBlockUndo* undo_data, interfaces::BlockInfo& info);

//! Send blockConnected and blockDisconnected notifications needed to sync from
//! a specified block to the chain tip.
//!
//! @param chainstate - chain to sync to
//! @param block - starting block to sync from
//! @param options - notification options
//! @param notifications - object to send notifications to
//! @param interrupt - flag to interrupt the sync
//! @param on_sync - optional callback invoked when reaching the chain tip
//!                  while cs_main is still held, before sending a final
//!                  blockConnected notification. This can be used to
//!                  synchronously register for new notifications.
void SyncChain(const Chainstate& chainstate, const CBlockIndex* block, const interfaces::Chain::NotifyOptions& options, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt, std::function<void(const CBlockIndex*)> on_sync);
} // namespace node

#endif // BITCOIN_NODE_CHAIN_H
