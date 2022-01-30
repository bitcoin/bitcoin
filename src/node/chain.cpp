// Copyright (c) 2025 The Bitcoin Core developers
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

void SyncChain(const Chainstate& chainstate, const CBlockIndex* block, const interfaces::Chain::NotifyOptions& options, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt, std::function<void()> on_sync)
{
    const CBlockIndex* pindex = block;
    // Last block in the chain syncing to which is known to be flushed.
    const CBlockIndex *last_flushed{nullptr};
    // Return flush status of the block being indexed.
    auto process_block = [&](bool append){
        if (pindex) {
            CBlock block;
            interfaces::BlockInfo block_info = kernel::MakeBlockInfo(pindex);
            block_info.chain_tip = false;
            if (pindex && last_flushed) {
                if (pindex == last_flushed) {
                    block_info.status = BlockInfo::FLUSHED_TIP;
                } else if (last_flushed->GetAncestor(pindex->nHeight) == pindex) {
                    block_info.status = BlockInfo::FLUSHED;
                }
            }
            notifications->blockConnected(ChainstateRole::NORMAL, block_info); // error logged internally
            if (append) {
                block_info.data = &block;
                if (!chainstate.m_blockman.ReadBlock(block, *pindex)) {
                    block_info.error = strprintf("Failed to read block %s from disk", pindex->GetBlockHash().ToString());
                }
            }
            notifications->blockConnected(ChainstateRole::NORMAL, block_info);
        }
    };
    while (true) {
        if (interrupt) {
            LogInfo("%s: interrupt set; exiting sync", options.thread_name);
            // Call process_block a final time to commit latest state.
            process_block(/*append=*/false);
            return;
        }

        const CBlockIndex* pindex_next = WITH_LOCK(cs_main,
            last_flushed = chainstate.LastFlushedBlock();
            return NextSyncBlock(pindex, chainstate.m_chain));
        // If pindex_next is null, it means pindex is the chain tip, so
        // commit data indexed so far.
        if (!pindex_next) {
            // Call process_block with append=false to force a commit.
            process_block(/*append=*/false);

            // If pindex is still the chain tip after committing, exit the sync
            // loop. Call on_sync with cs_main so caller can handle it before
            // new blocks are connected.
            LOCK(::cs_main);
            pindex_next = NextSyncBlock(pindex, chainstate.m_chain);
            if (!pindex_next) {
                if (on_sync) on_sync();
                break;
            }
        }
        if (pindex_next->pprev != pindex) {
            const CBlockIndex* current_tip = pindex;
            const CBlockIndex* new_tip = pindex_next->pprev;
            for (const CBlockIndex* iter_tip = current_tip; iter_tip != new_tip; iter_tip = iter_tip->pprev) {
                CBlock block;
                interfaces::BlockInfo block_info = kernel::MakeBlockInfo(iter_tip);
                block_info.chain_tip = false;
                if (options.disconnect_data) {
                    block_info.data = &block;
                    if (!chainstate.m_blockman.ReadBlock(block, *iter_tip)) {
                        block_info.error = strprintf("Failed to read block %s from disk", iter_tip->GetBlockHash().ToString());
                    }
                }
                notifications->blockDisconnected(block_info);
                if (interrupt) break;
            }
        }
        pindex = pindex_next;
        process_block(/*append=*/true);

        /* Send a final notification with no data and chain_tip = true at the end of the sync. */
        notifications->blockConnected(ChainstateRole::NORMAL, kernel::MakeBlockInfo(pindex));
    }
}
} // namespace node
