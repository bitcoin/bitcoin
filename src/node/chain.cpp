// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <interfaces/chain.h>
#include <kernel/chain.h>
#include <node/blockstorage.h>
#include <node/chain.h>
#include <sync.h>
#include <uint256.h>
#include <undo.h>
#include <util/threadinterrupt.h>
#include <validation.h>

using interfaces::BlockInfo;
using kernel::MakeBlockInfo;

namespace node {
bool ReadBlockData(node::BlockManager& blockman, const CBlockIndex& block, CBlock* data, CBlockUndo* undo_data, interfaces::BlockInfo& info)
{
    if (data) {
        if (blockman.ReadBlock(*data, block)) {
            info.data = data;
        } else {
            info.error = strprintf("%s: Failed to read block %s from disk", __func__, block.GetBlockHash().ToString());
            return false;
        }
    }
    if (undo_data && block.nHeight > 0) {
        if (blockman.ReadBlockUndo(*undo_data, block)) {
            info.undo_data = undo_data;
        } else {
            info.error = strprintf("%s: Failed to read block %s undo data from disk", __func__, block.GetBlockHash().ToString());
            return false;
        }
    }
    return true;
}

bool SyncChain(const Chainstate& chainstate, const CBlockIndex* block, const interfaces::Chain::NotifyOptions& options, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt, std::function<void()> on_sync)
{
    while (true) {
        AssertLockNotHeld(::cs_main);
        WAIT_LOCK(::cs_main, main_lock);

        bool rewind = false;
        if (!block) {
            block = chainstate.m_chain.Genesis();
        } else if (chainstate.m_chain.Contains(block)) {
            block = chainstate.m_chain.Next(block);
        } else {
            rewind = true;
        }

        const CBlockIndex* last_flushed{chainstate.LastFlushedBlock()};
        auto flushed_status = [&block, &last_flushed] {
            if (block && last_flushed) {
                if (block == last_flushed) {
                    return BlockInfo::FLUSHED_TIP;
                } else if (last_flushed->GetAncestor(block->nHeight) == block) {
                    return BlockInfo::FLUSHED;
                }
            }
            return BlockInfo::UNFLUSHED;
        };

        if (block) {
            // Release cs_main while reading block data and sending notifications.
            REVERSE_LOCK(main_lock, ::cs_main);
            BlockInfo block_info = MakeBlockInfo(block);
            block_info.chain_tip = false;
            block_info.status = flushed_status();
            CBlock data;
            CBlockUndo undo_data;
            ReadBlockData(chainstate.m_blockman, *block,
                          !rewind || options.disconnect_data ? &data : nullptr,
                          (!rewind && options.connect_undo_data) || (rewind && options.disconnect_undo_data) ? &undo_data : nullptr,
                          block_info);
            if (rewind) {
                notifications->blockDisconnected(block_info);
                block = Assert(block->pprev);
            } else {
                notifications->blockConnected(ChainstateRole::NORMAL, block_info);
            }
        } else {
            block = chainstate.m_chain.Tip();
        }

        bool synced = block == chainstate.m_chain.Tip();
        if (synced || interrupt) {
            if (synced && on_sync) on_sync();
            if (block) {
                BlockInfo block_info = MakeBlockInfo(block);
                block_info.status = flushed_status();
                CBlockLocator locator = ::GetLocator(block);
                // Release cs_main while calling notification handlers.
                REVERSE_LOCK(main_lock, ::cs_main);
                if (synced) notifications->blockConnected(ChainstateRole::NORMAL, block_info);
                notifications->chainStateFlushed(ChainstateRole::NORMAL, locator);
            }
            return synced;
        }
    }
}
} // namespace node
