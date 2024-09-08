// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(chainstate_write_tests)

BOOST_FIXTURE_TEST_CASE(chainstate_write_interval, TestingSetup)
{
    struct TestSubscriber final : CValidationInterface {
        bool m_did_flush{false};
        void ChainStateFlushed(ChainstateRole, const CBlockLocator&) override
        {
            m_did_flush = true;
        }
    };

    const auto sub{std::make_shared<TestSubscriber>()};
    m_node.validation_signals->RegisterSharedValidationInterface(sub);
    auto& chainstate{Assert(m_node.chainman)->ActiveChainstate()};
    BlockValidationState state_dummy{};

    // The first periodic flush sets m_next_write and does not flush
    chainstate.FlushStateToDisk(state_dummy, FlushStateMode::PERIODIC);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK(!sub->m_did_flush);

    // The periodic flush interval is between 50 and 70 minutes (inclusive)
    SetMockTime(GetTime<std::chrono::minutes>() + 49min);
    chainstate.FlushStateToDisk(state_dummy, FlushStateMode::PERIODIC);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK(!sub->m_did_flush);

    SetMockTime(GetTime<std::chrono::minutes>() + 70min);
    chainstate.FlushStateToDisk(state_dummy, FlushStateMode::PERIODIC);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK(sub->m_did_flush);
}

BOOST_AUTO_TEST_SUITE_END()
