// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <node/rpc_load_monitor.h>
#include <test/util/setup_common.h>

#include <atomic>
#include <thread>
#include <vector>

using namespace node;

BOOST_AUTO_TEST_SUITE(rpc_load_monitor_tests)

BOOST_AUTO_TEST_CASE(initial_state_is_normal)
{
    AtomicRpcLoadMonitor monitor;
    BOOST_CHECK(monitor.GetState() == RpcLoadState::NORMAL);
    BOOST_CHECK_EQUAL(monitor.GetQueueDepth(), 0);
    BOOST_CHECK_EQUAL(monitor.GetQueueCapacity(), 0);
}

BOOST_AUTO_TEST_CASE(state_transitions_normal_to_elevated)
{
    AtomicRpcLoadMonitor monitor;
    
    // Below threshold - stay NORMAL
    monitor.OnQueueDepthSample(70, 100);  // 70%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::NORMAL);
    
    // At threshold - transition to ELEVATED
    monitor.OnQueueDepthSample(75, 100);  // 75%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::ELEVATED);
}

BOOST_AUTO_TEST_CASE(state_transitions_normal_to_critical)
{
    AtomicRpcLoadMonitor monitor;
    
    // Jump directly to CRITICAL
    monitor.OnQueueDepthSample(90, 100);  // 90%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::CRITICAL);
}

BOOST_AUTO_TEST_CASE(state_transitions_elevated_to_critical)
{
    AtomicRpcLoadMonitor monitor;
    
    // First go to ELEVATED
    monitor.OnQueueDepthSample(80, 100);  // 80%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::ELEVATED);
    
    // Then to CRITICAL
    monitor.OnQueueDepthSample(95, 100);  // 95%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::CRITICAL);
}

BOOST_AUTO_TEST_CASE(hysteresis_elevated_to_normal)
{
    AtomicRpcLoadMonitor monitor;
    
    // Go to ELEVATED
    monitor.OnQueueDepthSample(80, 100);  // 80%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::ELEVATED);
    
    // Drop to 60% - still ELEVATED (hysteresis)
    monitor.OnQueueDepthSample(60, 100);  // 60%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::ELEVATED);
    
    // Drop to 50% - still ELEVATED (at threshold)
    monitor.OnQueueDepthSample(50, 100);  // 50%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::ELEVATED);
    
    // Drop below 50% - back to NORMAL
    monitor.OnQueueDepthSample(49, 100);  // 49%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::NORMAL);
}

BOOST_AUTO_TEST_CASE(hysteresis_critical_to_elevated)
{
    AtomicRpcLoadMonitor monitor;
    
    // Go to CRITICAL
    monitor.OnQueueDepthSample(95, 100);  // 95%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::CRITICAL);
    
    // Drop to 75% - still CRITICAL (hysteresis)
    monitor.OnQueueDepthSample(75, 100);  // 75%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::CRITICAL);
    
    // Drop to 70% - still CRITICAL (at threshold)
    monitor.OnQueueDepthSample(70, 100);  // 70%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::CRITICAL);
    
    // Drop below 70% - back to ELEVATED
    monitor.OnQueueDepthSample(69, 100);  // 69%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::ELEVATED);
}

BOOST_AUTO_TEST_CASE(full_cycle_normal_critical_normal)
{
    AtomicRpcLoadMonitor monitor;
    
    // Start NORMAL
    BOOST_CHECK(monitor.GetState() == RpcLoadState::NORMAL);
    
    // Spike to CRITICAL
    monitor.OnQueueDepthSample(95, 100);
    BOOST_CHECK(monitor.GetState() == RpcLoadState::CRITICAL);
    
    // Drop to ELEVATED
    monitor.OnQueueDepthSample(65, 100);
    BOOST_CHECK(monitor.GetState() == RpcLoadState::ELEVATED);
    
    // Drop to NORMAL
    monitor.OnQueueDepthSample(40, 100);
    BOOST_CHECK(monitor.GetState() == RpcLoadState::NORMAL);
}

BOOST_AUTO_TEST_CASE(zero_capacity_safe)
{
    AtomicRpcLoadMonitor monitor;
    
    // Zero capacity should not crash or change state
    monitor.OnQueueDepthSample(10, 0);
    BOOST_CHECK(monitor.GetState() == RpcLoadState::NORMAL);
    BOOST_CHECK_EQUAL(monitor.GetQueueDepth(), 10);
    BOOST_CHECK_EQUAL(monitor.GetQueueCapacity(), 0);
}

BOOST_AUTO_TEST_CASE(negative_capacity_safe)
{
    AtomicRpcLoadMonitor monitor;
    
    // Negative capacity should not crash
    monitor.OnQueueDepthSample(10, -1);
    BOOST_CHECK(monitor.GetState() == RpcLoadState::NORMAL);
}

BOOST_AUTO_TEST_CASE(custom_thresholds)
{
    RpcLoadMonitorConfig cfg{
        .elevated_ratio = 0.50,    // Enter ELEVATED at 50%
        .critical_ratio = 0.80,    // Enter CRITICAL at 80%
        .leave_elevated = 0.30,    // Leave ELEVATED at 30%
        .leave_critical = 0.60     // Leave CRITICAL at 60%
    };
    AtomicRpcLoadMonitor monitor(cfg);
    
    // Test custom thresholds
    monitor.OnQueueDepthSample(50, 100);  // 50%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::ELEVATED);
    
    monitor.OnQueueDepthSample(80, 100);  // 80%
    BOOST_CHECK(monitor.GetState() == RpcLoadState::CRITICAL);
    
    monitor.OnQueueDepthSample(59, 100);  // 59% - back to ELEVATED
    BOOST_CHECK(monitor.GetState() == RpcLoadState::ELEVATED);
    
    monitor.OnQueueDepthSample(29, 100);  // 29% - back to NORMAL
    BOOST_CHECK(monitor.GetState() == RpcLoadState::NORMAL);
}

BOOST_AUTO_TEST_CASE(thread_safety)
{
    AtomicRpcLoadMonitor monitor;
    std::atomic<bool> running{true};
    constexpr int NUM_READERS = 4;
    constexpr int NUM_WRITERS = 2;
    constexpr int ITERATIONS = 10000;
    
    std::vector<std::thread> threads;
    
    // Writer threads
    for (int w = 0; w < NUM_WRITERS; ++w) {
        threads.emplace_back([&monitor, &running]() {
            for (int i = 0; i < ITERATIONS && running; ++i) {
                int depth = (i % 100);  // Oscillate 0-99
                monitor.OnQueueDepthSample(depth, 100);
            }
        });
    }
    
    // Reader threads
    for (int r = 0; r < NUM_READERS; ++r) {
        threads.emplace_back([&monitor, &running]() {
            for (int i = 0; i < ITERATIONS && running; ++i) {
                auto state = monitor.GetState();
                auto depth = monitor.GetQueueDepth();
                auto cap = monitor.GetQueueCapacity();
                // Just read - no crashes
                (void)state;
                (void)depth;
                (void)cap;
            }
        });
    }
    
    for (auto& th : threads) {
        th.join();
    }
    
    // If we get here without crash, test passes
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(depth_and_capacity_tracking)
{
    AtomicRpcLoadMonitor monitor;
    
    monitor.OnQueueDepthSample(42, 100);
    BOOST_CHECK_EQUAL(monitor.GetQueueDepth(), 42);
    BOOST_CHECK_EQUAL(monitor.GetQueueCapacity(), 100);
    
    monitor.OnQueueDepthSample(17, 64);
    BOOST_CHECK_EQUAL(monitor.GetQueueDepth(), 17);
    BOOST_CHECK_EQUAL(monitor.GetQueueCapacity(), 64);
}

BOOST_AUTO_TEST_SUITE_END()
