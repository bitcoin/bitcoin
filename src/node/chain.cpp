// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/chain.h>

#include <chain.h>
#include <interfaces/chain.h>
#include <interfaces/types.h>
#include <kernel/chain.h>
#include <kernel/cs_main.h>
#include <kernel/types.h>
#include <node/blockstorage.h>
#include <node/chain.h>
#include <sync.h>
#include <uint256.h>
#include <undo.h>
#include <util/threadinterrupt.h>
#include <validation.h>

class CBlock;

using interfaces::BlockInfo;

namespace node {
BlockInfo MakeBlockInfo(const CBlockIndex* index, const CBlock* data, const Chainstate* chainstate)
{
    BlockInfo info{index ? *index->phashBlock : uint256::ZERO};
    if (index) {
        info.prev_hash = index->pprev ? index->pprev->phashBlock : nullptr;
        info.height = index->nHeight;
        info.chain_time_max = index->GetBlockTimeMax();
        LOCK(::cs_main);
        info.file_number = index->nFile;
        info.data_pos = index->nDataPos;
        if (chainstate) {
            info.state = BlockInfo::SYNCING;
            const CBlockIndex* last_flushed{chainstate->LastFlushedBlock()};
            if (index == last_flushed) {
                info.status = BlockInfo::FLUSHED_TIP;
            } else if (!last_flushed || last_flushed->GetAncestor(index->nHeight) == index) {
                info.status = BlockInfo::FLUSHED;
            }
        }
    }
    info.data = data;
    return info;
}

bool ReadBlockData(node::BlockManager& blockman, const CBlockIndex& block, CBlock* data, CBlockUndo* undo_data, BlockInfo& info)
{
    if (data) {
        if (blockman.ReadBlock(*data, block)) {
            info.data = data;
        } else {
            info.error = strprintf("Failed to read block %s from disk", block.GetBlockHash().ToString());
            return false;
        }
    }
    if (undo_data) {
        if (block.nHeight == 0 || blockman.ReadBlockUndo(*undo_data, block)) {
            info.undo_data = undo_data;
        } else {
            info.error = strprintf("Failed to read block %s undo data from disk", block.GetBlockHash().ToString());
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

bool SyncChain(const Chainstate& chainstate, const CBlockIndex* block, const interfaces::Chain::NotifyOptions& options, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt, std::function<void(const CBlockIndex*)> on_sync)
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

        auto role{chainstate.GetRole()};
        if (block) {
            // Read block data and send notifications.
            BlockInfo block_info = MakeBlockInfo(block, nullptr, &chainstate);
            REVERSE_LOCK(main_lock, ::cs_main);
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
                notifications->blockConnected(role, block_info);
            }
        } else {
            block = chainstate.m_chain.Tip();
        }

        if (interrupt) return false;

        if (block == chainstate.m_chain.Tip()) {
            // Sent an extra notification when reaching the tip to signal that
            // sync is likely finished, and allow the app to flush data.
            {
                BlockInfo block_info = MakeBlockInfo(block, nullptr, &chainstate);
                REVERSE_LOCK(main_lock, ::cs_main);
                notifications->blockConnected(role, block_info);
            }
            // If tip did not change, the sync is done.
            if (block == chainstate.m_chain.Tip()) {
                if (on_sync) on_sync(block);
                return true;
            }
        }
    }
}
} // namespace node
