// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <logging.h>
#include <random.h>
#include <test/util/common.h>
#include <util/string.h>
#include <util/threadpool.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <functional>
#include <latch>
#include <ranges>
#include <semaphore>

// General test values
int NUM_WORKERS_DEFAULT = 0;
constexpr char POOL_NAME[] = "test";
constexpr auto WAIT_TIMEOUT = 120s;

struct ThreadPoolFixture {
    ThreadPoolFixture() {
        NUM_WORKERS_DEFAULT = FastRandomContext().randrange(GetNumCores()) + 1;
        LogInfo("thread pool workers count: %d", NUM_WORKERS_DEFAULT);
    }
};

// Test Cases Overview
// 0) Submit task to a non-started pool.
// 1) Submit tasks and verify completion.
// 2) Maintain all threads busy except one.
// 3) Wait for work to finish.
// 4) Wait for result object.
// 5) The task throws an exception, catch must be done in the consumer side.
// 6) Busy workers, help them by processing tasks externally.
// 7) Recursive submission of tasks.
// 8) Submit task when all threads are busy, stop pool and verify task gets executed.
// 9) Congestion test; create more workers than available cores.
// 10) Ensure Interrupt() prevents further submissions.
// 11) Start() must not cause a deadlock when called during Stop().
// 12) Ensure queued tasks complete after Interrupt().
// 13) Ensure the Stop() calling thread helps drain the queue.
// 14) Submit range of tasks in one lock acquisition.
BOOST_FIXTURE_TEST_SUITE(threadpool_tests, ThreadPoolFixture)

#define WAIT_FOR(futures)                                                         \
    do {                                                                          \
        for (const auto& f : futures) {                                           \
            BOOST_REQUIRE(f.wait_for(WAIT_TIMEOUT) == std::future_status::ready); \
        }                                                                         \
    } while (0)

// Helper to unwrap a valid pool submission
template <typename F>
[[nodiscard]] auto Submit(ThreadPool& pool, F&& fn)
{
    return std::move(*Assert(pool.Submit(std::forward<F>(fn))));
}

// Block a number of worker threads by submitting tasks that wait on `release_sem`.
// Returns the futures of the blocking tasks, ensuring all have started and are waiting.
std::vector<std::future<void>> BlockWorkers(ThreadPool& threadPool, std::counting_semaphore<>& release_sem, size_t num_of_threads_to_block)
{
    assert(threadPool.WorkersCount() >= num_of_threads_to_block);
    std::latch ready{static_cast<std::ptrdiff_t>(num_of_threads_to_block)};
    std::vector<std::future<void>> blocking_tasks(num_of_threads_to_block);
    for (auto& f : blocking_tasks) f = Submit(threadPool, [&] {
        ready.count_down();
        release_sem.acquire();
    });
    ready.wait();
    return blocking_tasks;
}

// Test 0, submit task to a non-started, interrupted, or stopped pool
BOOST_AUTO_TEST_CASE(submit_fails_with_correct_error)
{
    ThreadPool threadPool(POOL_NAME);
    const auto fn_empty = [&] {};

    // Never started: Inactive
    auto res = threadPool.Submit(fn_empty);
    BOOST_CHECK(!res);
    BOOST_CHECK_EQUAL(SubmitErrorString(res.error()), "No active workers");

    // Interrupted (workers still alive): Interrupted, and Start() must be rejected too
    std::counting_semaphore<> blocker(0);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    const auto blocking_tasks = BlockWorkers(threadPool, blocker, NUM_WORKERS_DEFAULT);
    threadPool.Interrupt();
    res = threadPool.Submit(fn_empty);
    BOOST_CHECK(!res);
    BOOST_CHECK_EQUAL(SubmitErrorString(res.error()), "Interrupted");
    BOOST_CHECK_EXCEPTION(threadPool.Start(NUM_WORKERS_DEFAULT), std::runtime_error, HasReason("Thread pool has been interrupted or is stopping"));
    blocker.release(NUM_WORKERS_DEFAULT);
    WAIT_FOR(blocking_tasks);

    // Interrupted then stopped: Inactive
    threadPool.Stop();
    res = threadPool.Submit(fn_empty);
    BOOST_CHECK(!res);
    BOOST_CHECK_EQUAL(SubmitErrorString(res.error()), "No active workers");

    // Started then stopped: Inactive
    threadPool.Start(NUM_WORKERS_DEFAULT);
    threadPool.Stop();
    res = threadPool.Submit(fn_empty);
    BOOST_CHECK(!res);
    BOOST_CHECK_EQUAL(SubmitErrorString(res.error()), "No active workers");

    std::vector<std::function<void()>> tasks;
    const auto range_res{threadPool.Submit(std::move(tasks))};
    BOOST_CHECK(!range_res);
    BOOST_CHECK_EQUAL(SubmitErrorString(range_res.error()), "No active workers");
}

// Test 1, submit tasks and verify completion
BOOST_AUTO_TEST_CASE(submit_tasks_complete_successfully)
{
    int num_tasks = 50;

    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    std::atomic<int> counter = 0;

    // Store futures to ensure completion before checking counter.
    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);
    for (int i = 1; i <= num_tasks; i++) {
        futures.emplace_back(Submit(threadPool, [&counter, i]() {
            counter.fetch_add(i, std::memory_order_relaxed);
        }));
    }

    // Wait for all tasks to finish
    WAIT_FOR(futures);
    int expected_value = (num_tasks * (num_tasks + 1)) / 2; // Gauss sum.
    BOOST_CHECK_EQUAL(counter.load(), expected_value);
    BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 0);
}

// Test 2, maintain all threads busy except one
BOOST_AUTO_TEST_CASE(single_available_worker_executes_all_tasks)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    std::counting_semaphore<> blocker(0);
    const auto blocking_tasks = BlockWorkers(threadPool, blocker, NUM_WORKERS_DEFAULT - 1);

    // Now execute tasks on the single available worker
    // and check that all the tasks are executed.
    int num_tasks = 15;
    int counter = 0;

    // Store futures to wait on
    std::vector<std::future<void>> futures(num_tasks);
    for (auto& f : futures) f = Submit(threadPool, [&counter]{ counter++; });

    WAIT_FOR(futures);
    BOOST_CHECK_EQUAL(counter, num_tasks);

    blocker.release(NUM_WORKERS_DEFAULT - 1);
    WAIT_FOR(blocking_tasks);
    threadPool.Stop();
    BOOST_CHECK_EQUAL(threadPool.WorkersCount(), 0);
}

// Test 3, wait for work to finish
BOOST_AUTO_TEST_CASE(wait_for_task_to_finish)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    std::atomic<bool> flag = false;
    std::future<void> future = Submit(threadPool, [&flag]() {
        UninterruptibleSleep(200ms);
        flag.store(true, std::memory_order_release);
    });
    BOOST_CHECK(future.wait_for(WAIT_TIMEOUT) == std::future_status::ready);
    BOOST_CHECK(flag.load(std::memory_order_acquire));
}

// Test 4, obtain result object
BOOST_AUTO_TEST_CASE(get_result_from_completed_task)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    std::future<bool> future_bool = Submit(threadPool, []() { return true; });
    BOOST_CHECK(future_bool.get());

    std::future<std::string> future_str = Submit(threadPool, []() { return std::string("true"); });
    std::string result = future_str.get();
    BOOST_CHECK_EQUAL(result, "true");
}

// Test 5, throw exception and catch it on the consumer side
BOOST_AUTO_TEST_CASE(task_exception_propagates_to_future)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    const auto make_err{[&](size_t n) { return strprintf("error on thread #%s", n); }};

    const int num_tasks = 5;
    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);
    for (int i = 0; i < num_tasks; i++) {
        futures.emplace_back(Submit(threadPool, [&make_err, i] { throw std::runtime_error(make_err(i)); }));
    }

    for (int i = 0; i < num_tasks; i++) {
        BOOST_CHECK_EXCEPTION(futures[i].get(), std::runtime_error, HasReason{make_err(i)});
    }
}

// Test 6, all workers are busy, help them by processing tasks from outside
BOOST_AUTO_TEST_CASE(process_tasks_manually_when_workers_busy)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    std::counting_semaphore<> blocker(0);
    const auto& blocking_tasks = BlockWorkers(threadPool, blocker, NUM_WORKERS_DEFAULT);

    // Now submit tasks and check that none of them are executed.
    int num_tasks = 20;
    std::atomic<int> counter = 0;
    for (int i = 0; i < num_tasks; i++) {
        (void)Submit(threadPool, [&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    UninterruptibleSleep(100ms);
    BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), num_tasks);

    // Now process manually
    for (int i = 0; i < num_tasks; i++) {
        threadPool.ProcessTask();
    }
    BOOST_CHECK_EQUAL(counter.load(), num_tasks);
    BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 0);
    blocker.release(NUM_WORKERS_DEFAULT);
    threadPool.Stop();
    WAIT_FOR(blocking_tasks);
}

// Test 7, submit tasks from other tasks
BOOST_AUTO_TEST_CASE(recursive_task_submission)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    std::promise<void> signal;
    (void)Submit(threadPool, [&]() {
        (void)Submit(threadPool, [&]() {
            signal.set_value();
        });
    });

    signal.get_future().wait();
    threadPool.Stop();
}

// Test 8, submit task when all threads are busy and then stop the pool
BOOST_AUTO_TEST_CASE(task_submitted_while_busy_completes)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    std::counting_semaphore<> blocker(0);
    const auto& blocking_tasks = BlockWorkers(threadPool, blocker, NUM_WORKERS_DEFAULT);

    // Submit an extra task that should execute once a worker is free
    std::future<bool> future = Submit(threadPool, []() { return true; });

    // At this point, all workers are blocked, and the extra task is queued
    BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 1);

    // Wait a short moment before unblocking the threads to mimic a concurrent shutdown
    std::thread thread_unblocker([&blocker]() {
        UninterruptibleSleep(300ms);
        blocker.release(NUM_WORKERS_DEFAULT);
    });

    // Stop the pool while the workers are still blocked
    threadPool.Stop();

    // Expect the submitted task to complete
    BOOST_CHECK(future.get());
    thread_unblocker.join();

    // Obviously all the previously blocking tasks should be completed at this point too
    WAIT_FOR(blocking_tasks);

    // Pool should be stopped and no workers remaining
    BOOST_CHECK_EQUAL(threadPool.WorkersCount(), 0);
}

// Test 9, more workers than available cores (congestion test)
BOOST_AUTO_TEST_CASE(congestion_more_workers_than_cores)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(std::max(1, GetNumCores() * 2)); // Oversubscribe by 2×

    int num_tasks = 200;
    std::atomic<int> counter{0};

    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);
    for (int i = 0; i < num_tasks; i++) {
        futures.emplace_back(Submit(threadPool, [&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    WAIT_FOR(futures);
    BOOST_CHECK_EQUAL(counter.load(), num_tasks);
}

// Test 10, Interrupt() prevents further submissions
BOOST_AUTO_TEST_CASE(interrupt_blocks_new_submissions)
{
    // 1) Interrupt from main thread
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);
    threadPool.Interrupt();

    auto res = threadPool.Submit([]{});
    BOOST_CHECK(!res);
    BOOST_CHECK_EQUAL(SubmitErrorString(res.error()), "Interrupted");

    std::vector<std::function<void()>> tasks;
    const auto range_res{threadPool.Submit(std::move(tasks))};
    BOOST_CHECK(!range_res);
    BOOST_CHECK_EQUAL(SubmitErrorString(range_res.error()), "Interrupted");

    // Reset pool
    threadPool.Stop();

    // 2) Interrupt() from a worker thread
    // One worker is blocked, another calls Interrupt(), and the remaining one waits for tasks.
    threadPool.Start(/*num_workers=*/3);
    std::atomic<int> counter{0};
    std::counting_semaphore<> blocker(0);
    const auto blocking_tasks = BlockWorkers(threadPool, blocker, 1);
    Submit(threadPool, [&threadPool, &counter]{
        threadPool.Interrupt();
        counter.fetch_add(1, std::memory_order_relaxed);
    }).get();
    blocker.release(1); // unblock worker

    BOOST_CHECK_EQUAL(counter.load(), 1);
    threadPool.Stop();
    WAIT_FOR(blocking_tasks);
    BOOST_CHECK_EQUAL(threadPool.WorkersCount(), 0);
}

// Test 11, Start() must not cause a deadlock when called during Stop()
BOOST_AUTO_TEST_CASE(start_mid_stop_does_not_deadlock)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    // Keep all workers busy so Stop() gets stuck waiting for them to finish during join()
    std::counting_semaphore<> workers_blocker(0);
    const auto blocking_tasks = BlockWorkers(threadPool, workers_blocker, NUM_WORKERS_DEFAULT);

    std::thread stopper_thread([&threadPool] { threadPool.Stop(); });

    // Stop() takes ownership of the workers before joining them, so WorkersCount()
    // hits 0 the moment Stop() is waiting for them to join. That is our signal
    // to call Start() right into the middle of the joining phase.
    while (threadPool.WorkersCount() != 0) {
        std::this_thread::yield(); // let the OS breathe so it can switch context
    }
    // Now we know for sure the stopper thread is hanging while workers are still alive.
    // Restart the pool and resume workers so the stopper thread can proceed.
    // This will throw an exception only if the pool handles Start-Stop race properly,
    // otherwise it will proceed and hang the stopper_thread.
    try {
        threadPool.Start(NUM_WORKERS_DEFAULT);
    } catch (std::exception& e) {
        BOOST_CHECK_EQUAL(e.what(), "Thread pool has been interrupted or is stopping");
    }
    workers_blocker.release(NUM_WORKERS_DEFAULT);
    WAIT_FOR(blocking_tasks);

    // If Stop() is stuck, joining the stopper thread will deadlock
    stopper_thread.join();
}

// Test 12, queued tasks complete after Interrupt()
BOOST_AUTO_TEST_CASE(queued_tasks_complete_after_interrupt)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    std::counting_semaphore<> blocker(0);
    const auto blocking_tasks = BlockWorkers(threadPool, blocker, NUM_WORKERS_DEFAULT);

    // Queue tasks while all workers are busy, then interrupt
    std::atomic<int> counter{0};
    const int num_tasks = 10;
    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);
    for (int i = 0; i < num_tasks; i++) {
        futures.emplace_back(Submit(threadPool, [&counter]{ counter.fetch_add(1, std::memory_order_relaxed); }));
    }
    threadPool.Interrupt();

    // Queued tasks must still complete despite the interrupt
    blocker.release(NUM_WORKERS_DEFAULT);
    WAIT_FOR(futures);
    BOOST_CHECK_EQUAL(counter.load(), num_tasks);

    threadPool.Stop();
    WAIT_FOR(blocking_tasks);
}

// Test 13, ensure the Stop() calling thread helps drain the queue
BOOST_AUTO_TEST_CASE(stop_active_wait_drains_queue)
{
    ThreadPool threadPool(POOL_NAME);
    threadPool.Start(NUM_WORKERS_DEFAULT);

    std::counting_semaphore<> blocker(0);
    const auto blocking_tasks = BlockWorkers(threadPool, blocker, NUM_WORKERS_DEFAULT);

    auto main_thread_id = std::this_thread::get_id();
    std::atomic<int> main_thread_tasks{0};
    const size_t num_tasks = 20;
    for (size_t i = 0; i < num_tasks; i++) {
        (void)Submit(threadPool, [&main_thread_tasks, main_thread_id]() {
            if (std::this_thread::get_id() == main_thread_id)
                main_thread_tasks.fetch_add(1, std::memory_order_relaxed);
        });
    }
    BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), num_tasks);

    // Delay release so Stop() drains all tasks from the calling thread
    std::thread unblocker([&blocker, &threadPool]() {
        while (threadPool.WorkQueueSize() > 0) {
            std::this_thread::yield();
        }
        blocker.release(NUM_WORKERS_DEFAULT);
    });

    threadPool.Stop();
    unblocker.join();

    // Check the main thread processed all tasks
    BOOST_CHECK_EQUAL(main_thread_tasks.load(), num_tasks);
    WAIT_FOR(blocking_tasks);
}

// Test 14, submit range of tasks in one lock acquisition
BOOST_AUTO_TEST_CASE(submit_range_of_tasks_complete_successfully)
{
    constexpr int32_t num_tasks{50};

    ThreadPool threadPool{POOL_NAME};
    threadPool.Start(NUM_WORKERS_DEFAULT);
    std::atomic_int32_t sum{0};
    const auto square{[&sum](int32_t i) {
        sum.fetch_add(i, std::memory_order_relaxed);
        return i * i;
    }};

    std::array<std::function<int32_t()>, static_cast<size_t>(num_tasks)> array_tasks;
    std::vector<std::function<int32_t()>> vector_tasks;
    vector_tasks.reserve(static_cast<size_t>(num_tasks));
    for (const auto i : std::views::iota(int32_t{1}, num_tasks + 1)) {
        array_tasks.at(static_cast<size_t>(i - 1)) = [i, square] { return square(i); };
        vector_tasks.emplace_back([i, square] { return square(i); });
    }

    auto futures{std::move(*Assert(threadPool.Submit(std::move(array_tasks))))};
    BOOST_CHECK_EQUAL(futures.size(), static_cast<size_t>(num_tasks));
    std::ranges::move(*Assert(threadPool.Submit(std::move(vector_tasks))), std::back_inserter(futures));
    BOOST_CHECK_EQUAL(futures.size(), static_cast<size_t>(num_tasks * 2));

    auto squares_sum{0};
    for (auto& future : futures) {
        squares_sum += future.get();
    }

    // 2x Gauss sum.
    const auto expected_sum{2 * ((num_tasks * (num_tasks + 1)) / 2)};
    const auto expected_squares_sum{2 * ((num_tasks * (num_tasks + 1) * ((num_tasks * 2) + 1)) / 6)};
    BOOST_CHECK_EQUAL(sum, expected_sum);
    BOOST_CHECK_EQUAL(squares_sum, expected_squares_sum);
    BOOST_CHECK_EQUAL(threadPool.WorkQueueSize(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
