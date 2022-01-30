// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAIN_H
#define BITCOIN_NODE_CHAIN_H

#include <interfaces/chain.h>

#include <functional>

class CBlockIndex;
class Chainstate;
class CThreadInterrupt;

namespace node {
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
void SyncChain(const Chainstate& chainstate, const CBlockIndex* block, const interfaces::Chain::NotifyOptions& options, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt, std::function<void()> on_sync);
} // namespace node

#endif // BITCOIN_NODE_CHAIN_H
