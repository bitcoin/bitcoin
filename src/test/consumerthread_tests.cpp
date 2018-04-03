// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <atomic>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <test/test_bitcoin.h>

#include <core/consumerthread.h>
#include <core/producerconsumerqueue.h>

BOOST_FIXTURE_TEST_SUITE(consumerthread_tests, BasicTestingSetup)

class TestWorkItem : public WorkItem<WorkerMode::BLOCKING>
{
public:
    TestWorkItem(int& i) : m_i(i){};
    void operator()()
    {
        // yield to make unit tests somewhat more unpredictable
        std::this_thread::yield();
        ++m_i;
        std::this_thread::yield();
        ++m_i;
    }

private:
    int& m_i;
};

void ConsumerThreadTest(int n_elements, int n_threads)
{
    std::vector<int> work(n_elements);
    auto queue = std::shared_ptr<WorkQueue<WorkerMode::BLOCKING>>(new WorkQueue<WorkerMode::BLOCKING>(n_elements + 1));

    std::vector<std::unique_ptr<ConsumerThread<WorkerMode::BLOCKING>>> threads;
    for (int i = 0; i < n_threads; i++) {
        threads.emplace_back(std::unique_ptr<ConsumerThread<WorkerMode::BLOCKING>>(new ConsumerThread<WorkerMode::BLOCKING>(queue, std::to_string(i))));
    }

    for (int i = 0; i < n_elements; i++) {
        work[i] = i;
        queue->Push(std::unique_ptr<TestWorkItem>(new TestWorkItem(work[i])));
    }

    while (queue->size() > 0) {
        std::this_thread::yield();
    }
    queue->Sync();

    for (int i = 0; i < n_threads / 2; i++) {
        threads[i]->Terminate();
    }

    BOOST_CHECK_LT(queue->size(), n_threads + 1);
    for (int i = 0; i < n_elements; i++) {
        BOOST_CHECK_EQUAL(work[i], i + 2);
    }

    for (int i = 0; i < n_elements; i++) {
        queue->Push(std::unique_ptr<TestWorkItem>(new TestWorkItem(work[i])));
    }

    while (queue->size() > 0) {
        std::this_thread::yield();
    }
    queue->Sync();

    for (int i = n_threads / 2; i < n_threads; i++) {
        threads[i]->Terminate();
    }

    BOOST_CHECK_LT(queue->size(), n_threads + 1);
    for (int i = 0; i < n_elements; i++) {
        BOOST_CHECK_EQUAL(work[i], i + 4);
    }
}

BOOST_AUTO_TEST_CASE(foo)
{
    ConsumerThreadTest(100, 1);
    ConsumerThreadTest(100, 10);
    ConsumerThreadTest(0, 10);
    ConsumerThreadTest(3, 10);
}

BOOST_AUTO_TEST_SUITE_END();
