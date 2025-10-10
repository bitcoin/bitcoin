// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/validation.h>

#include <util/check.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

using kernel::ChainstateRole;

void TestChainstateManager::DisableNextWrite()
{
    struct TestChainstate : public Chainstate {
        void ResetNextWrite() { m_next_write = NodeClock::time_point::max() - 1s; }
    };
    LOCK(::cs_main);
    for (const auto& cs : m_chainstates) {
        static_cast<TestChainstate&>(*cs).ResetNextWrite();
    }
}

void TestChainstateManager::ResetIbd()
{
    m_cached_finished_ibd = false;
    assert(IsInitialBlockDownload());
}

void TestChainstateManager::JumpOutOfIbd()
{
    Assert(IsInitialBlockDownload());
    m_cached_finished_ibd = true;
    Assert(!IsInitialBlockDownload());
}

void ValidationInterfaceTest::BlockConnected(
    const ChainstateRole& role,
    CValidationInterface& obj,
    const std::shared_ptr<const CBlock>& block,
    const CBlockIndex* pindex)
{
    obj.BlockConnected(role, block, pindex);
}
