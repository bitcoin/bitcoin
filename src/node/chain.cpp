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
using kernel::ChainstateRole;

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
    return true;
}

static const CBlockIndex* NextSyncBlock(const CBlockIndex* pindex_prev, const CChain& chain) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    if (!pindex_prev) {
        return chain.Genesis();
    }

    const CBlockIndex* pindex = chain.Next(pindex_prev);
    if (pindex) {
        return pindex;
    }

    // Since block is not in the chain, return the next block in the chain AFTER the last common ancestor.
    // Caller will be responsible for rewinding back to the common ancestor.
    return chain.Next(chain.FindFork(pindex_prev));
}

void SyncChain(const Chainstate& chainstate, const CBlockIndex* block, const interfaces::Chain::NotifyOptions& options, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt, std::function<void(const CBlockIndex*)> on_sync)
{
    assert(chainstate.m_chain.Tip());
    const CBlockIndex* pindex = block;
    std::optional<kernel::ChainstateRole> role;
    while (true) {
        if (interrupt) {
            LogInfo("%s: interrupt set; exiting sync", options.thread_name);
            // Call blockConnected with no data to commit latest state.
            if (role) notifications->blockConnected(*role, MakeBlockInfo(pindex, nullptr, &chainstate)); // error logged internally
            return;
        }

        const CBlockIndex* pindex_next = WITH_LOCK(cs_main,
            role = chainstate.GetRole();
            return NextSyncBlock(pindex, chainstate.m_chain));
        // If pindex_next is null, it means pindex is the chain tip, so
        // commit data indexed so far.
        if (!pindex_next) {
            // Call blockConnected with no data to commit latest state.
            notifications->blockConnected(*role, MakeBlockInfo(pindex, nullptr, &chainstate)); // error logged internally
            // If pindex is still the chain tip after committing, exit the sync
            // loop. Call on_sync with cs_main so caller can handle it before
            // new blocks are connected.
            LOCK(::cs_main);
            pindex_next = NextSyncBlock(pindex, chainstate.m_chain);
            if (!pindex_next) {
                if (on_sync) on_sync(pindex);
                break;
            }
        }
        if (pindex_next->pprev != pindex) {
            for (const CBlockIndex* new_tip = pindex_next->pprev; pindex != new_tip && !interrupt; pindex = pindex->pprev) {
                CBlock data;
                CBlockUndo undo_data;
                BlockInfo block = MakeBlockInfo(pindex, nullptr, &chainstate);
                ReadBlockData(chainstate.m_blockman, *pindex, options.disconnect_data ? &data : nullptr, options.disconnect_undo_data ? &undo_data : nullptr, block);
                notifications->blockDisconnected(block);
            }
            if (interrupt) continue;
        }
        pindex = pindex_next;
        CBlock data;
        CBlockUndo undo_data;
        BlockInfo block = MakeBlockInfo(pindex, nullptr, &chainstate);
        ReadBlockData(chainstate.m_blockman, *pindex, &data, options.connect_undo_data ? &undo_data : nullptr, block);
        notifications->blockConnected(*role, block); // error logged internally
    }
}
} // namespace node
