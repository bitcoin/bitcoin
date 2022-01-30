// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/index.h>

#include <index/base.h>
#include <node/chain.h>

void IndexTester::Sync()
{
   CThreadInterrupt interrupt;
   node::SyncChain(*Assert(m_index.m_chainstate),
                   m_index.m_best_block_index,
                   m_index.CustomOptions(),
                   m_index.Notifications(),
                   interrupt,
                   nullptr);
}
