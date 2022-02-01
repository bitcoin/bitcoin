// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/index.h>

#include <chain.h>
#include <index/base.h>
#include <kernel/types.h>
#include <node/chain.h>
#include <validation.h>

using interfaces::BlockInfo;
using kernel::ChainstateRole;
using node::MakeBlockInfo;
using node::SyncChain;

void IndexTester::Sync()
{
    CThreadInterrupt interrupt;
    CBlockIndex* best_block{nullptr};
    if (std::optional<interfaces::BlockRef> block{m_index.GetBestBlock()}) {
        best_block = WITH_LOCK(::cs_main, return m_index.m_chainstate->m_blockman.LookupBlockIndex(block->hash));
    }
    SyncChain(*Assert(m_index.m_chainstate), best_block, m_index.CustomOptions(), m_index.Notifications(), interrupt, nullptr);
    BlockInfo block = MakeBlockInfo(best_block, nullptr, m_index.m_chainstate);
    block.state = BlockInfo::SYNCED;
    m_index.Notifications()->blockConnected(ChainstateRole{}, block);
}
