// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/validation.h>

#include <coins.h>
#include <consensus/consensus.h>
#include <index/base.h>
#include <node/blockstorage.h>
#include <node/mining_types.h>
#include <primitives/block.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/time.h>
#include <validation.h>
#include <validation_queue.h>
#include <validationinterface.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

using kernel::ChainstateRole;

IndexTestGuard::~IndexTestGuard()
{
    m_index.Interrupt();
    m_index.Stop();
    m_signals.SyncWithValidationInterfaceQueue();
}

BlockWorkerGate::BlockWorkerGate(ChainstateManager& chainman)
{
    auto submission{chainman.m_block_processing_queue.Submit(std::make_shared<const CBlock>(), [this](const auto&) {
        m_entered.set_value();
        m_released.wait();
        return BlockProcessingResult{};
    })};
    Assert(submission.has_value());
    m_completion = std::move(*submission);
}

BlockWorkerGate::~BlockWorkerGate()
{
    Open();
    m_completion.wait();
}

void BlockWorkerGate::Wait()
{
    if (m_entered_future.wait_for(std::chrono::seconds{30}) != std::future_status::ready) {
        throw std::runtime_error{"Timed out waiting for the block worker gate"};
    }
}

void BlockWorkerGate::Open()
{
    if (!m_open) {
        m_open = true;
        m_release.set_value();
    }
}

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

std::vector<std::pair<COutPoint, CAmount>> ResetChainmanAndMempool(TestingSetup& setup, FakeNodeClock& node_clock)
{
    setup.m_node.chainman->StopBlockProcessing();
    node_clock.set(setup.m_node.chainman->GetParams().GenesisBlock().Time());

    bilingual_str error{};
    setup.m_node.mempool.reset();
    setup.m_node.mempool = std::make_unique<CTxMemPool>(MemPoolOptionsForTest(setup.m_node), error);
    Assert(error.empty());

    setup.m_node.chainman.reset();
    setup.m_make_chainman();
    setup.LoadVerifyActivateChainstate();

    node::BlockCreateOptions options;
    options.coinbase_output_script = P2WSH_OP_TRUE;

    std::vector<std::pair<COutPoint, CAmount>> mature_coinbase;
    for (int i = 0; i < 2 * COINBASE_MATURITY; ++i) {
        COutPoint prevout{MineBlock(setup.m_node, options)};
        if (i < COINBASE_MATURITY) {
            LOCK(cs_main);
            CAmount subsidy{setup.m_node.chainman->ActiveChainstate().CoinsTip().GetCoin(prevout)->out.nValue};
            mature_coinbase.emplace_back(prevout, subsidy);
        }
    }
    return mature_coinbase;
}
