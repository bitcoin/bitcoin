// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/validation.h>

#include <node/blockstorage.h>
#include <util/check.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

using kernel::ChainstateRole;

void TestBlockManager::CleanupForFuzzing()
{
    m_dirty_blockindex.clear();
    m_dirty_fileinfo.clear();
    m_blockfile_info.resize(1);
}

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
    m_cached_is_ibd = true;
    assert(IsInitialBlockDownload());
}

void TestChainstateManager::JumpOutOfIbd()
{
    Assert(IsInitialBlockDownload());
    m_cached_is_ibd = false;
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
void TestChainstateManager::InvalidBlockFound(CBlockIndex* pindex, const BlockValidationState& state)
{
    struct TestChainstate : public Chainstate {
        void CallInvalidBlockFound(CBlockIndex* pindex, const BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
        {
            InvalidBlockFound(pindex, state);
        }
    };

    static_cast<TestChainstate*>(&ActiveChainstate())->CallInvalidBlockFound(pindex, state);
}

void TestChainstateManager::InvalidChainFound(CBlockIndex* pindexNew)
{
    struct TestChainstate : public Chainstate {
        void CallInvalidChainFound(CBlockIndex* pindexNew) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
        {
            InvalidChainFound(pindexNew);
        }
    };

    static_cast<TestChainstate*>(&ActiveChainstate())->CallInvalidChainFound(pindexNew);
}

CBlockIndex* TestChainstateManager::FindMostWorkChain()
{
    struct TestChainstate : public Chainstate {
        CBlockIndex* CallFindMostWorkChain() EXCLUSIVE_LOCKS_REQUIRED(cs_main)
        {
            return FindMostWorkChain();
        }
    };

    return static_cast<TestChainstate*>(&ActiveChainstate())->CallFindMostWorkChain();
}

void TestChainstateManager::ResetBestInvalid()
{
    m_best_invalid = nullptr;
}
