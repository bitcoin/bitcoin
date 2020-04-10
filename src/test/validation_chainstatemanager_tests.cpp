// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <chainparams.h>
#include <random.h>
#include <uint256.h>
#include <consensus/validation.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_chainstatemanager_tests, TestingSetup)

//! Basic tests for ChainstateManager.
//!
//! First create a legacy (IBD) chainstate, then create a snapshot chainstate.
BOOST_AUTO_TEST_CASE(chainstatemanager)
{
    ChainstateManager manager;
    std::vector<CChainState*> chainstates;
    const CChainParams& chainparams = Params();

    // Create a legacy (IBD) chainstate.
    //
    ENTER_CRITICAL_SECTION(cs_main);
    CChainState& c1 = manager.InitializeChainstate();
    LEAVE_CRITICAL_SECTION(cs_main);
    chainstates.push_back(&c1);
    c1.InitCoinsDB(
        /* cache_size_bytes */ 1 << 23, /* in_memory */ true, /* should_wipe */ false);
    WITH_LOCK(::cs_main, c1.InitCoinsCache());

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
    ENTER_CRITICAL_SECTION(cs_main);
    CChainState& c2 = manager.InitializeChainstate(GetRandHash());
    LEAVE_CRITICAL_SECTION(cs_main);
    chainstates.push_back(&c2);
    c2.InitCoinsDB(
        /* cache_size_bytes */ 1 << 23, /* in_memory */ true, /* should_wipe */ false);
    WITH_LOCK(::cs_main, c2.InitCoinsCache());
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

    // Avoid triggering the address sanitizer.
    WITH_LOCK(::cs_main, manager.Unload());
}

BOOST_AUTO_TEST_SUITE_END()
