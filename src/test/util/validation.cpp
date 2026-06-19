// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/validation.h>

#include <coins.h>
#include <consensus/consensus.h>
#include <node/blockstorage.h>
#include <node/mining_types.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <memory>
#include <utility>
#include <vector>

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

std::vector<std::pair<COutPoint, CAmount>> ResetChainmanAndMempool(TestingSetup& setup)
{
    GetFakeNodeClock().set(setup.m_node.chainman->GetParams().GenesisBlock().Time());

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
