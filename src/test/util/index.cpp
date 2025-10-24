// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/index.h>

#include <index/base.h>
#include <node/chain.h>
#include <node/context.h>
#include <validation.h>

void IndexTester::Sync()
{
    Chainstate& chainstate{WITH_LOCK(::cs_main, return m_index.m_chain->context()->chainman->GetChainstateForIndexing())};
    CThreadInterrupt interrupt;
    node::SyncChain(chainstate,
                    WITH_LOCK(::cs_main, return chainstate.m_blockman.LookupBlockIndex(Assert(m_index.GetBestBlock())->hash)),
                    m_index.CustomOptions(),
                    m_index.Notifications(),
                    interrupt,
                    nullptr);
}
