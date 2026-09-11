// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <scheduler.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <validation_queue.h>
#include <validationinterface.h>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>

BOOST_FIXTURE_TEST_SUITE(validationinterface_tests, ChainTestingSetup)

struct TestSubscriberNoop final : public CValidationInterface {
    void BlockChecked(const std::shared_ptr<const CBlock>&, const BlockValidationState&) override {}
};

BOOST_AUTO_TEST_CASE(unregister_validation_interface_race)
{
    std::atomic<bool> generate{true};

    // Start thread to generate notifications
    std::thread gen{[&] {
        BlockValidationState state_dummy;
        while (generate) {
            m_node.validation_signals->BlockChecked(std::make_shared<const CBlock>(), state_dummy);
            m_node.validation_signals->BlockProcessed();
        }
    }};

    // Start thread to consume notifications
    std::thread sub{[&] {
        // keep going for about 1 sec, which is 250k iterations
        for (int i = 0; i < 250000; i++) {
            auto sub = std::make_shared<TestSubscriberNoop>();
            m_node.validation_signals->RegisterSharedValidationInterface(sub);
            m_node.validation_signals->UnregisterSharedValidationInterface(sub);
        }
        // tell the other thread we are done
        generate = false;
    }};

    gen.join();
    sub.join();
    BOOST_CHECK(!generate);
}

BOOST_AUTO_TEST_CASE(unregister_then_sync_covers_block_processed)
{
    using namespace std::chrono_literals;
    struct Subscriber final : CValidationInterface {
        std::promise<void> entered;
        std::promise<void> release;
        std::shared_future<void> released{release.get_future().share()};
        std::atomic<int> calls{0};
        std::atomic<bool> returned{false};

        void BlockProcessed() override
        {
            if (calls.fetch_add(1) != 0) return;
            entered.set_value();
            released.wait();
            returned = true;
        }
    } sub;

    auto& signals{*m_node.validation_signals};
    auto entered{sub.entered.get_future()};
    BlockProcessingQueue queue;
    std::future<bool> drained;
    std::promise<void> drain_started;
    auto started{drain_started.get_future()};
    signals.RegisterValidationInterface(&sub);
    struct Cleanup {
        Subscriber& sub;
        ValidationSignals& signals;
        BlockProcessingQueue& queue;
        std::future<bool>& drained;
        bool open{false};

        void Open()
        {
            if (!open) {
                open = true;
                sub.release.set_value();
            }
        }
        ~Cleanup()
        {
            Open();
            queue.Stop();
            if (drained.valid()) drained.wait();
            signals.UnregisterValidationInterface(&sub);
            signals.SyncWithValidationInterfaceQueue();
        }
    } cleanup{sub, signals, queue, drained};

    queue.Start([&] { signals.BlockProcessed(); });
    auto first{queue.Submit(std::make_shared<const CBlock>(), [](const auto&) { return BlockProcessingResult{}; })};
    BOOST_REQUIRE(first.has_value());
    BOOST_REQUIRE(entered.wait_for(30s) == std::future_status::ready);
    BOOST_CHECK(first->wait_for(0s) == std::future_status::ready);

    // Index Stop() unregisters first. The queue barrier must then cover any
    // completion callback already running before the raw subscriber is destroyed.
    signals.UnregisterValidationInterface(&sub);
    drained = std::async(std::launch::async, [&] {
        drain_started.set_value();
        signals.SyncWithValidationInterfaceQueue();
        return sub.returned.load();
    });
    BOOST_REQUIRE(started.wait_for(30s) == std::future_status::ready);
    BOOST_CHECK(drained.wait_for(100ms) == std::future_status::timeout);

    // A parked notification does not prevent the worker from processing another job.
    auto second{queue.Submit(std::make_shared<const CBlock>(), [](const auto&) { return BlockProcessingResult{}; })};
    BOOST_REQUIRE(second.has_value());
    BOOST_CHECK(second->wait_for(30s) == std::future_status::ready);
    cleanup.Open();
    BOOST_REQUIRE(drained.wait_for(30s) == std::future_status::ready);
    BOOST_CHECK(drained.get());
    queue.Stop();
    signals.SyncWithValidationInterfaceQueue();
    BOOST_CHECK_EQUAL(sub.calls.load(), 1);
}

class TestInterface : public CValidationInterface
{
public:
    TestInterface(ValidationSignals& signals, std::function<void()> on_call = nullptr, std::function<void()> on_destroy = nullptr)
        : m_on_call(std::move(on_call)), m_on_destroy(std::move(on_destroy)), m_signals{signals}
    {
    }
    virtual ~TestInterface()
    {
        if (m_on_destroy) m_on_destroy();
    }
    void BlockChecked(const std::shared_ptr<const CBlock>& block, const BlockValidationState& state) override
    {
        if (m_on_call) m_on_call();
    }
    void Call()
    {
        BlockValidationState state;
        m_signals.BlockChecked(std::make_shared<const CBlock>(), state);
    }
    std::function<void()> m_on_call;
    std::function<void()> m_on_destroy;
    ValidationSignals& m_signals;
};

// Regression test to ensure UnregisterAllValidationInterfaces calls don't
// destroy a validation interface while it is being called. Bug:
// https://github.com/bitcoin/bitcoin/pull/18551
BOOST_AUTO_TEST_CASE(unregister_all_during_call)
{
    bool destroyed = false;
    auto shared{std::make_shared<TestInterface>(
        *m_node.validation_signals,
        [&] {
            // First call should decrements reference count 2 -> 1
            m_node.validation_signals->UnregisterAllValidationInterfaces();
            BOOST_CHECK(!destroyed);
            // Second call should not decrement reference count 1 -> 0
            m_node.validation_signals->UnregisterAllValidationInterfaces();
            BOOST_CHECK(!destroyed);
        },
        [&] { destroyed = true; })};
    m_node.validation_signals->RegisterSharedValidationInterface(shared);
    BOOST_CHECK(shared.use_count() == 2);
    shared->Call();
    BOOST_CHECK(shared.use_count() == 1);
    BOOST_CHECK(!destroyed);
    shared.reset();
    BOOST_CHECK(destroyed);
}

BOOST_AUTO_TEST_SUITE_END()
