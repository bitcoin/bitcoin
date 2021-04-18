// Copyright (c) 2019-2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <chainparams.h>
#include <consensus/validation.h>
#include <random.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <validation.h>
#include <validationinterface.h>

#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_chainstatemanager_tests, TestingSetup)

//! Basic tests for ChainstateManager.
//!
//! First create a legacy (IBD) chainstate, then create a snapshot chainstate.
BOOST_AUTO_TEST_CASE(chainstatemanager)
{
    ChainstateManager manager;
    CTxMemPool mempool;
    std::vector<CChainState*> chainstates;
    const CChainParams& chainparams = Params();

    // Create a legacy (IBD) chainstate.
    //
    CChainState& c1 = *WITH_LOCK(::cs_main, return &manager.InitializeChainstate(mempool));
    chainstates.push_back(&c1);
    c1.InitCoinsDB(
        /* cache_size_bytes */ 1 << 23, /* in_memory */ true, /* should_wipe */ false);
    WITH_LOCK(::cs_main, c1.InitCoinsCache(1 << 23));

    BOOST_CHECK(!manager.IsSnapshotActive());
    BOOST_CHECK(!manager.IsSnapshotValidated());
    BOOST_CHECK(!manager.IsBackgroundIBD(&c1));
    auto all = manager.GetAll();
    BOOST_CHECK_EQUAL_COLLECTIONS(all.begin(), all.end(), chainstates.begin(), chainstates.end());

    auto& active_chain = manager.ActiveChain();
    BOOST_CHECK_EQUAL(&active_chain, &c1.m_chain);

    BOOST_CHECK_EQUAL(manager.ActiveHeight(), -1);

    auto active_tip = manager.ActiveTip();
    auto exp_tip = c1.m_chain.Tip();
    BOOST_CHECK_EQUAL(active_tip, exp_tip);

    auto& validated_cs = manager.ValidatedChainstate();
    BOOST_CHECK_EQUAL(&validated_cs, &c1);

    // Create a snapshot-based chainstate.
    //
    CChainState& c2 = *WITH_LOCK(::cs_main, return &manager.InitializeChainstate(mempool, GetRandHash()));
    chainstates.push_back(&c2);
    c2.InitCoinsDB(
        /* cache_size_bytes */ 1 << 23, /* in_memory */ true, /* should_wipe */ false);
    WITH_LOCK(::cs_main, c2.InitCoinsCache(1 << 23));
    // Unlike c1, which doesn't have any blocks. Gets us different tip, height.
    c2.LoadGenesisBlock(chainparams);
    BlockValidationState _;
    BOOST_CHECK(c2.ActivateBestChain(_, chainparams, nullptr));

    BOOST_CHECK(manager.IsSnapshotActive());
    BOOST_CHECK(!manager.IsSnapshotValidated());
    BOOST_CHECK(manager.IsBackgroundIBD(&c1));
    BOOST_CHECK(!manager.IsBackgroundIBD(&c2));
    auto all2 = manager.GetAll();
    BOOST_CHECK_EQUAL_COLLECTIONS(all2.begin(), all2.end(), chainstates.begin(), chainstates.end());

    auto& active_chain2 = manager.ActiveChain();
    BOOST_CHECK_EQUAL(&active_chain2, &c2.m_chain);

    BOOST_CHECK_EQUAL(manager.ActiveHeight(), 0);

    auto active_tip2 = manager.ActiveTip();
    auto exp_tip2 = c2.m_chain.Tip();
    BOOST_CHECK_EQUAL(active_tip2, exp_tip2);

    // Ensure that these pointers actually correspond to different
    // CCoinsViewCache instances.
    BOOST_CHECK(exp_tip != exp_tip2);

    auto& validated_cs2 = manager.ValidatedChainstate();
    BOOST_CHECK_EQUAL(&validated_cs2, &c1);

    auto& validated_chain = manager.ValidatedChain();
    BOOST_CHECK_EQUAL(&validated_chain, &c1.m_chain);

    auto validated_tip = manager.ValidatedTip();
    exp_tip = c1.m_chain.Tip();
    BOOST_CHECK_EQUAL(validated_tip, exp_tip);

    // Let scheduler events finish running to avoid accessing memory that is going to be unloaded
    SyncWithValidationInterfaceQueue();

    WITH_LOCK(::cs_main, manager.Unload());
}

//! Test rebalancing the caches associated with each chainstate.
BOOST_AUTO_TEST_CASE(chainstatemanager_rebalance_caches)
{
    ChainstateManager manager;
    CTxMemPool mempool;
    size_t max_cache = 10000;
    manager.m_total_coinsdb_cache = max_cache;
    manager.m_total_coinstip_cache = max_cache;

    std::vector<CChainState*> chainstates;

    // Create a legacy (IBD) chainstate.
    //
    CChainState& c1 = *WITH_LOCK(cs_main, return &manager.InitializeChainstate(mempool));
    chainstates.push_back(&c1);
    c1.InitCoinsDB(
        /* cache_size_bytes */ 1 << 23, /* in_memory */ true, /* should_wipe */ false);

    {
        LOCK(::cs_main);
        c1.InitCoinsCache(1 << 23);
        c1.CoinsTip().SetBestBlock(InsecureRand256());
        manager.MaybeRebalanceCaches();
    }

    BOOST_CHECK_EQUAL(c1.m_coinstip_cache_size_bytes, max_cache);
    BOOST_CHECK_EQUAL(c1.m_coinsdb_cache_size_bytes, max_cache);

    // Create a snapshot-based chainstate.
    //
    CChainState& c2 = *WITH_LOCK(cs_main, return &manager.InitializeChainstate(mempool, GetRandHash()));
    chainstates.push_back(&c2);
    c2.InitCoinsDB(
        /* cache_size_bytes */ 1 << 23, /* in_memory */ true, /* should_wipe */ false);

    {
        LOCK(::cs_main);
        c2.InitCoinsCache(1 << 23);
        c2.CoinsTip().SetBestBlock(InsecureRand256());
        manager.MaybeRebalanceCaches();
    }

    // Since both chainstates are considered to be in initial block download,
    // the snapshot chainstate should take priority.
    BOOST_CHECK_CLOSE(c1.m_coinstip_cache_size_bytes, max_cache * 0.05, 1);
    BOOST_CHECK_CLOSE(c1.m_coinsdb_cache_size_bytes, max_cache * 0.05, 1);
    BOOST_CHECK_CLOSE(c2.m_coinstip_cache_size_bytes, max_cache * 0.95, 1);
    BOOST_CHECK_CLOSE(c2.m_coinsdb_cache_size_bytes, max_cache * 0.95, 1);
}

BOOST_AUTO_TEST_SUITE_END()
