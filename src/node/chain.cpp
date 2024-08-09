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

using interfaces::BlockInfo;
using kernel::MakeBlockInfo;

namespace node {
bool ReadBlockData(node::BlockManager& blockman, const CBlockIndex& block, CBlock* data, CBlockUndo* undo_data, interfaces::BlockInfo& info)
{
    if (data) {
        if (blockman.ReadBlockFromDisk(*data, block)) {
            info.data = data;
        } else {
            info.error = strprintf("%s: Failed to read block %s from disk", __func__, block.GetBlockHash().ToString());
            return false;
        }
    }
    if (undo_data && block.nHeight > 0) {
        if (blockman.UndoReadFromDisk(*undo_data, block)) {
            info.undo_data = undo_data;
        } else {
            info.error = strprintf("%s: Failed to read block %s undo data from disk", __func__, block.GetBlockHash().ToString());
            return false;
        }
    }
    return true;
}

bool SyncChain(BlockManager& blockman, const CChain& chain, const CBlockIndex* block, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt, std::function<void()> on_sync)
{
    while (true) {
        AssertLockNotHeld(::cs_main);
        WAIT_LOCK(::cs_main, main_lock);

        bool rewind = false;
        if (!block) {
            block = chain.Genesis();
        } else if (chain.Contains(block)) {
            block = chain.Next(block);
        } else {
            rewind = true;
        }

        if (block) {
            // Release cs_main while reading block data and sending notifications.
            REVERSE_LOCK(main_lock);
            BlockInfo block_info = MakeBlockInfo(block);
            block_info.chain_tip = false;
            CBlock data;
            CBlockUndo undo_data;
            ReadBlockData(blockman, *block, &data, &undo_data, block_info);
            if (rewind) {
                notifications->blockDisconnected(block_info);
                block = Assert(block->pprev);
            } else {
                notifications->blockConnected(ChainstateRole::NORMAL, block_info);
            }
        } else {
            block = chain.Tip();
        }

        bool synced = block == chain.Tip();
        if (synced || interrupt) {
            if (synced && on_sync) on_sync();
            if (block) {
                CBlockLocator locator = ::GetLocator(block);
                // Release cs_main while calling notification handlers.
                REVERSE_LOCK(main_lock);
                if (synced) notifications->blockConnected(ChainstateRole::NORMAL, MakeBlockInfo(block));
                notifications->chainStateFlushed(ChainstateRole::NORMAL, locator);
            }
            return synced;
        }
    }
}
} // namespace node
