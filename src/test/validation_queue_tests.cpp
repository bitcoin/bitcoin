// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/cs_main.h>
#include <primitives/block.h>
#include <sync.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <validation_queue.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {
using namespace std::chrono_literals;

/** Open on assertion failure too; worker closures own their synchronization state. */
class JobGate
{
    std::shared_ptr<std::promise<void>> m_entered{std::make_shared<std::promise<void>>()};
    std::future<void> m_entered_future{m_entered->get_future()};
    std::promise<void> m_release;
    std::shared_future<void> m_released{m_release.get_future().share()};
    bool m_open{false};

public:
    ~JobGate() { Open(); }
    auto Waiter() const
    {
        return [entered = m_entered, released = m_released] {
            entered->set_value();
            released.wait();
        };
    }
    void WaitUntilEntered()
    {
        BOOST_REQUIRE(m_entered_future.wait_for(30s) == std::future_status::ready);
    }
    void Open()
    {
        if (!m_open) {
            m_open = true;
            m_release.set_value();
        }
    }
};

std::shared_ptr<const CBlock> Block(unsigned int nonce = 0)
{
    auto block{std::make_shared<CBlock>()};
    block->nNonce = nonce;
    return block;
}

BlockProcessingResult Process(const std::shared_ptr<const CBlock>&)
{
    return {.processing_success = true, .new_block = true};
}

std::future<BlockProcessingResult> Submit(BlockProcessingQueue& queue, const std::shared_ptr<const CBlock>& block, BlockProcessingQueue::Processor process = Process)
{
    auto submission{queue.Submit(block, std::move(process))};
    BOOST_REQUIRE(submission.has_value());
    return std::move(*submission);
}

BlockProcessingResult Get(std::future<BlockProcessingResult>& future)
{
    BOOST_REQUIRE(future.wait_for(30s) == std::future_status::ready);
    return future.get();
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(validation_queue_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(lifecycle)
{
    const auto block{Block()};
    BlockProcessingQueue queue;
    BOOST_CHECK(queue.Submit(block, Process).error() == BlockProcessingQueue::SubmitError::Inactive);
    queue.Start();
    BOOST_CHECK_THROW(queue.Start(), std::logic_error);
    auto result{Submit(queue, block)};
    BOOST_CHECK(Get(result).processing_success);
    queue.Interrupt();
    BOOST_CHECK(queue.Submit(block, Process).error() == BlockProcessingQueue::SubmitError::Stopping);
    BOOST_CHECK_THROW(queue.Start(), std::logic_error);
    queue.Stop();
    queue.Stop();
    BOOST_CHECK(queue.Submit(block, Process).error() == BlockProcessingQueue::SubmitError::Stopping);

    BlockProcessingQueue stopped;
    stopped.Stop();
    BOOST_CHECK_THROW(stopped.Start(), std::logic_error);
    BlockProcessingQueue interrupted;
    interrupted.Interrupt();
    BOOST_CHECK_THROW(interrupted.Start(), std::logic_error);
}

BOOST_AUTO_TEST_CASE(queued_jobs_preserve_order_and_ownership)
{
    auto first_block{Block(1)};
    auto second_block{Block(2)};
    const std::weak_ptr<const CBlock> first_weak{first_block}, second_weak{second_block};
    auto third_block{Block(3)};
    const std::weak_ptr<const CBlock> third_weak{third_block};
    std::vector<unsigned int> processed;
    BlockProcessingQueue queue;
    JobGate first_gate;
    queue.Start();
    auto first{Submit(queue, first_block, [wait = first_gate.Waiter(), &processed](const auto& block) {
        wait();
        processed.push_back(block->nNonce);
        return BlockProcessingResult{.processing_success = false, .new_block = true};
    })};
    first_gate.WaitUntilEntered();
    auto second{Submit(queue, second_block, [&processed](const auto& block) {
        processed.push_back(block->nNonce);
        return BlockProcessingResult{.processing_success = true, .new_block = false};
    })};
    auto third{Submit(queue, third_block, [&processed](const auto& block) {
        processed.push_back(block->nNonce);
        return Process(block);
    })};
    first_block.reset();
    second_block.reset();
    third_block.reset();
    BOOST_CHECK(!first_weak.expired());
    BOOST_CHECK(!second_weak.expired());
    BOOST_CHECK(!third_weak.expired());
    BOOST_CHECK(first.wait_for(0s) == std::future_status::timeout);
    BOOST_CHECK(second.wait_for(0s) == std::future_status::timeout);
    BOOST_CHECK(third.wait_for(0s) == std::future_status::timeout);

    first_gate.Open();
    const auto first_result{Get(first)};
    const auto second_result{Get(second)};
    BOOST_CHECK(!first_result.processing_success && first_result.new_block);
    BOOST_CHECK(second_result.processing_success && !second_result.new_block);
    BOOST_CHECK(Get(third).processing_success);
    BOOST_CHECK(first_weak.expired());
    BOOST_CHECK(second_weak.expired());
    BOOST_CHECK(third_weak.expired());
    BOOST_REQUIRE_EQUAL(processed.size(), 3);
    BOOST_CHECK_EQUAL(processed[0], 1);
    BOOST_CHECK_EQUAL(processed[1], 2);
    BOOST_CHECK_EQUAL(processed[2], 3);
}

BOOST_AUTO_TEST_CASE(exceptions_reach_future_and_worker_continues)
{
    const auto block{Block()};
    BlockProcessingQueue queue;
    queue.Start();
    auto standard{Submit(queue, block, [](const auto&) -> BlockProcessingResult {
        throw std::runtime_error("processing failed");
    })};
    BOOST_CHECK_EXCEPTION(Get(standard), std::runtime_error, HasReason("processing failed"));
    struct UnexpectedException {};
    auto unexpected{Submit(queue, block, [](const auto&) -> BlockProcessingResult {
        throw UnexpectedException{};
    })};
    BOOST_CHECK_THROW(Get(unexpected), UnexpectedException);
    auto success{Submit(queue, block)};
    BOOST_CHECK(Get(success).processing_success);
}

BOOST_AUTO_TEST_CASE(shutdown_finishes_jobs_on_one_worker)
{
    Mutex calls_mutex;
    std::vector<std::pair<unsigned int, std::thread::id>> calls;
    std::atomic<bool> stopped{false};
    const auto record = [&](const auto& block) {
        LOCK(calls_mutex);
        calls.emplace_back(block->nNonce, std::this_thread::get_id());
        return Process(block);
    };
    BlockProcessingQueue queue;
    // Gates open before joiners are destroyed if an assertion fails.
    std::future<void> first_stopper, second_stopper;
    JobGate first_gate;
    queue.Start();
    auto first{Submit(queue, Block(1), [wait = first_gate.Waiter(), &record](const auto& block) {
        auto result{record(block)};
        wait();
        return result;
    })};
    first_gate.WaitUntilEntered();
    auto second{Submit(queue, Block(2), record)};
    auto third{Submit(queue, Block(3), record)};
    queue.Interrupt();
    BOOST_CHECK(queue.Submit(Block(4), record).error() == BlockProcessingQueue::SubmitError::Stopping);
    first_stopper = std::async(std::launch::async, [&] { queue.Stop(); stopped = true; });
    second_stopper = std::async(std::launch::async, [&] { queue.Stop(); });
    BOOST_CHECK(!stopped.load());
    first_gate.Open();
    first_stopper.get();
    second_stopper.get();
    BOOST_CHECK(stopped.load());
    BOOST_CHECK(Get(first).processing_success);
    BOOST_CHECK(Get(second).processing_success);
    BOOST_CHECK(Get(third).processing_success);
    LOCK(calls_mutex);
    BOOST_REQUIRE_EQUAL(calls.size(), 3);
    BOOST_CHECK(calls[0].second != std::this_thread::get_id());
    for (size_t i = 0; i < calls.size(); ++i) {
        BOOST_CHECK_EQUAL(calls[i].first, i + 1);
        BOOST_CHECK(calls[i].second == calls[0].second);
    }
}

BOOST_AUTO_TEST_CASE(concurrent_producers_enqueue_while_worker_is_busy)
{
    const auto block{Block()};
    std::array<BlockProcessingQueue::Submission, 32> submissions;
    BlockProcessingQueue queue;
    JobGate gate;
    queue.Start();
    auto running{Submit(queue, block, [wait = gate.Waiter()](const auto& value) {
        wait();
        return Process(value);
    })};
    gate.WaitUntilEntered();
    {
        std::vector<std::future<void>> producers;
        producers.reserve(submissions.size());
        for (auto& submission : submissions) {
            producers.emplace_back(std::async(std::launch::async, [&queue, &block, &submission] { submission = queue.Submit(block, Process); }));
        }
        for (auto& producer : producers) producer.get();
    }
    for (const auto& submission : submissions) {
        BOOST_REQUIRE(submission.has_value());
        BOOST_CHECK(submission->wait_for(0s) == std::future_status::timeout);
    }
    gate.Open();
    BOOST_CHECK(Get(running).processing_success);
    for (auto& submission : submissions) {
        BOOST_CHECK(Get(*submission).processing_success);
    }
}

BOOST_AUTO_TEST_CASE(processor_can_submit_without_holding_queue_mutex)
{
    std::optional<BlockProcessingQueue::Submission> nested;
    const auto block{Block()};
    BlockProcessingQueue queue;
    queue.Start();
    auto first{Submit(queue, block, [&](const auto& value) {
        LOCK(cs_main);
        nested = queue.Submit(value, Process);
        return Process(value);
    })};
    BOOST_CHECK(Get(first).processing_success);
    BOOST_REQUIRE(nested && nested->has_value());
    BOOST_CHECK(Get(**nested).processing_success);
}

BOOST_AUTO_TEST_CASE(completion_notification_follows_future_and_releases_mutex)
{
    const auto block{Block()};
    std::future<BlockProcessingResult> first;
    std::optional<BlockProcessingQueue::Submission> nested;
    std::promise<bool> notified;
    auto notification{notified.get_future()};
    bool first_notification{true};
    BlockProcessingQueue queue;
    JobGate gate;
    queue.Start([&] {
        if (!first_notification) return;
        first_notification = false;
        const bool ready{first.wait_for(0s) == std::future_status::ready};
        nested = queue.Submit(block, Process);
        notified.set_value(ready);
    });
    first = Submit(queue, block, [wait = gate.Waiter()](const auto& value) {
        wait();
        return Process(value);
    });
    gate.WaitUntilEntered();
    gate.Open();
    BOOST_REQUIRE(notification.wait_for(30s) == std::future_status::ready);
    BOOST_CHECK(notification.get());
    BOOST_REQUIRE(nested && nested->has_value());
    BOOST_CHECK(Get(first).processing_success);
    BOOST_CHECK(Get(**nested).processing_success);
}

BOOST_AUTO_TEST_CASE(destructor_finishes_accepted_jobs)
{
    std::vector<std::future<BlockProcessingResult>> futures;
    {
        BlockProcessingQueue queue;
        queue.Start();
        for (int i = 0; i < 8; ++i) futures.push_back(Submit(queue, Block()));
    }
    for (auto& future : futures) BOOST_CHECK(Get(future).processing_success);
}

BOOST_FIXTURE_TEST_CASE(manager_worker_started_and_stopped_by_fixture, TestingSetup)
{
    // Exercise the fixture's early stop while its mempool and callbacks are alive.
    BOOST_CHECK_THROW(m_node.chainman->StartBlockProcessing(), std::logic_error);
}

BOOST_AUTO_TEST_SUITE_END()
