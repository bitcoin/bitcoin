// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <test/test_bitcoin.h>

#include <core/producerconsumerqueue.h>

BOOST_FIXTURE_TEST_SUITE(producerconsumerqueue_tests, BasicTestingSetup)

typedef ProducerConsumerQueue<int, WorkerMode::BLOCKING, WorkerMode::BLOCKING> QueueBB;
typedef ProducerConsumerQueue<int, WorkerMode::BLOCKING, WorkerMode::NONBLOCKING> QueueBN;
typedef ProducerConsumerQueue<int, WorkerMode::NONBLOCKING, WorkerMode::BLOCKING> QueueNB;
typedef ProducerConsumerQueue<int, WorkerMode::NONBLOCKING, WorkerMode::NONBLOCKING> QueueNN;

typedef boost::mpl::list<QueueBB, QueueBN, QueueNB, QueueNN> queue_types;

template <typename Q>
void Producer(Q& push, Q& recv, int id, int elements_to_push)
{
    // push all of these elements to one queue
    int elements_pushed = 0;
    while (elements_pushed < elements_to_push) {
        if (push.Push(id * elements_to_push + elements_pushed)) {
            elements_pushed++;
        } else if (push.GetProducerMode() == WorkerMode::BLOCKING) {
            BOOST_FAIL("a blocking push should always succeed");
        } else {
            std::this_thread::yield();
        }
    }

    std::set<int> received;
    while (received.size() < (unsigned)elements_to_push) {
        int e;
        if (recv.Pop(e)) {
            assert(!received.count(e));
            received.insert(e);
        } else if (recv.GetConsumerMode() == WorkerMode::BLOCKING) {
            BOOST_FAIL("a blocking pop should always succeed");
        } else {
            std::this_thread::yield();
        }
    }

    for (int i = 0; i < elements_to_push; i++) {
        assert(received.count(-i));
    }
}

template <typename Q>
void Consumer(Q& work, std::vector<Q*> push, int id, int n_producers, int bucket_size, int elements_to_receive)
{
    int elements_received = 0;
    std::vector<int> latest(n_producers, -1);

    while (elements_received != elements_to_receive) {
        int w;
        if (work.Pop(w)) {
            int producer_id = w / bucket_size;
            int i = w % bucket_size;

            assert(producer_id < n_producers);
            assert(i > latest[producer_id]);
            latest[producer_id] = i;

            while (!push[producer_id]->Push(-i)) {
                if (push[producer_id]->GetProducerMode() == WorkerMode::BLOCKING) {
                    BOOST_FAIL("a blocking push should always succeed");
                }
                std::this_thread::yield();
            }
            elements_received++;
        } else if (work.GetConsumerMode() == WorkerMode::BLOCKING) {
            BOOST_FAIL("a blocking pop should always succeed");
        } else {
            std::this_thread::yield();
        }
    }
}

template <typename Q>
void QueueTest(int capacity, int n_elements, int n_producers, int n_consumers)
{
    int bucket_size = n_elements / n_producers;

    Q push(capacity);
    std::vector<Q*> pull;
    for (int i = 0; i < n_producers; i++)
        pull.push_back(new Q(bucket_size));

    boost::thread_group test_threads;

    for (int i = 0; i < n_producers; i++) {
        test_threads.create_thread([&, i] { Producer(push, *(pull[i]), i, bucket_size); });
    }

    for (int i = 0; i < n_consumers; i++) {
        test_threads.create_thread([&, i] { Consumer(push, pull, i, n_producers, bucket_size, n_elements / n_consumers); });
    }

    test_threads.join_all();

    // queue should be empty
    BOOST_CHECK_EQUAL(push.size(), 0);
    for (int i = 0; i < n_producers; i++) {
        BOOST_CHECK_EQUAL(pull[i]->size(), 0);
        delete pull[i];
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(invalid_queue, Q, queue_types)
{
    Q q;
    BOOST_CHECK(q.GetCapacity() == 0);
}

BOOST_AUTO_TEST_CASE(basic_operation)
{
    int n = 10;
    QueueBB qBB(n);
    QueueBN qBN(n);
    QueueNB qNB(n);
    QueueNN qNN(n);

    BOOST_CHECK((int)qBB.GetCapacity() == n);
    for (int i = 0; i < n; i++) {
        BOOST_CHECK(qBB.Push(i));
        BOOST_CHECK(qBN.Push(i));
        BOOST_CHECK(qNB.Push(i));
        BOOST_CHECK(qNN.Push(i));
    }

    BOOST_CHECK(!qNB.Push(0));
    BOOST_CHECK(!qNN.Push(0));

    int t;
    for (int i = 0; i < n; i++) {
        BOOST_CHECK_EQUAL(qBB.Pop(), i);

        BOOST_CHECK(qBN.Pop(t));
        BOOST_CHECK_EQUAL(t, i);

        BOOST_CHECK_EQUAL(qNB.Pop(), i);

        BOOST_CHECK(qNN.Pop(t));
        BOOST_CHECK_EQUAL(t, i);
    }

    int ret;
    BOOST_CHECK(!qBN.Pop(ret));
    BOOST_CHECK(!qNN.Pop(ret));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(multithreaded_operation, Q, queue_types)
{
    QueueTest<Q>(1000, 100, 1, 1);
    QueueTest<Q>(100, 1000, 1, 1);

    QueueTest<Q>(1000, 100, 1, 10);
    QueueTest<Q>(100, 1000, 1, 10);

    QueueTest<Q>(1000, 100, 10, 1);
    QueueTest<Q>(100, 1000, 10, 1);

    QueueTest<Q>(1000, 100, 10, 10);
    QueueTest<Q>(100, 1000, 10, 10);

    QueueTest<Q>(1000, 100, 10, 5);
    QueueTest<Q>(100, 1000, 10, 5);

    QueueTest<Q>(1000, 100, 5, 10);
    QueueTest<Q>(100, 1000, 5, 10);
};

BOOST_AUTO_TEST_SUITE_END();
