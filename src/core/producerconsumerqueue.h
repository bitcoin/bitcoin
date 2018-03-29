// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CORE_PRODUCERCONSUMERQUEUE_H
#define BITCOIN_CORE_PRODUCERCONSUMERQUEUE_H

#include <assert.h>
#include <deque>
#include <sync.h>
#include <type_traits>

/**
 * The mode in which the queue operates
 * Modes may be specified for both producers and consumers
 */
enum class WorkerMode {
    BLOCKING,   //!< cv_wait until the action may proceed
    NONBLOCKING //!< do not block, immediately return failure if the action is not possible
};

/**
 * A FIFO thread safe producer consumer queue with two operations Push() and Pop()
 * Producers Push() and Consumers Pop()
 *
 * @param T the type of the data contained
 * @param m_producer_mode queue behavior when calling Push() on a full queue (block till space becomes available, or immediately fail)
 * @param m_consumer_mode queue behavior when calling Pop() on an empty queue (block until there is data, or immediately fail)
 *
 * @see WorkerMode
 */
template <typename T, WorkerMode m_producer_mode = WorkerMode::BLOCKING, WorkerMode m_consumer_mode = WorkerMode::BLOCKING>
class ProducerConsumerQueue
{
public:
    /**
     * Constructs a ProducerConsumerQueue()
     * @param[in] capacity the maximum size of this queue
     */
    ProducerConsumerQueue(int capacity)
        : m_capacity(capacity)
    {
        assert(m_capacity > 0);
    };

    /**
     * Constructs an empty ProducerConsumerQueue with capacity 0
     * In nonblocking mode all operations will immediately fail
     * In blocking mode all operations will fail an assertion to avoid blocking forever
     */
    ProducerConsumerQueue()
        : m_capacity(0){};
    ~ProducerConsumerQueue(){};

    /**
     * Push an element to the back of the queue
     * Blocking producer mode: will always eventually succeed
     * Non-blocking producer mode: Push() returns failure when the queue is at capacity
     * @param[in] data the data to be pushed
     * @return the success of the operation
     * @see WorkerMode
     */
    template <typename TT>
    bool Push(TT&& data, WorkerMode mode = m_producer_mode)
    {
        // TT needed for perfect forwarding to vector::push_back

        // attempting a push to a queue of capacity 0 is likely unintended
        assert(m_capacity > 0);

        {
            std::unique_lock<std::mutex> l(m_queue_lock);
            if (m_data.size() >= m_capacity) {
                if (mode == WorkerMode::NONBLOCKING) {
                    return false;
                }

                m_producer_cv.wait(l, [&]() { return m_data.size() < m_capacity; });
            }

            m_data.push_back(std::forward<TT>(data));
        }
        m_consumer_cv.notify_one();
        return true;
    };

    /**
     * Try to pop the oldest element from the front of the queue, if present
     * Blocking consumer mode: will always eventually succeed
     * Nonblocking consumer mode: Pop() returns failure when the queue is empty
     * @param[out] the data popped, if the operation was successful
     * @return the success of the operation
     * @see WorkerMode
     */
    bool Pop(T& data, WorkerMode mode = m_consumer_mode)
    {
        // attempting a pop from a queue of capacity 0 is likely unintended
        assert(m_capacity > 0);

        {
            std::unique_lock<std::mutex> l(m_queue_lock);
            if (m_data.size() <= 0) {
                if (mode == WorkerMode::NONBLOCKING) {
                    return false;
                }

                m_consumer_cv.wait(l, [&]() { return m_data.size() > 0; });
            }

            data = std::move(m_data.front());
            m_data.pop_front();
        }
        m_producer_cv.notify_one();
        return true;
    }

    /**
     * Shortcut for bool Pop(T&) when consumer mode is blocking
     * This must always succeed and thus may only be called in producer blocking mode
     * @return the element popped
     */
    T Pop()
    {
        static_assert(m_consumer_mode == WorkerMode::BLOCKING, "");

        T ret;

        // use a temporary so theres no side effecting code inside an assert which could be disabled
        bool success = Pop(ret);
        assert(success);

        return ret;
    }

    typename std::deque<T>::size_type size()
    {
        std::unique_lock<std::mutex> l(m_queue_lock);
        return m_data.size();
    }
    unsigned int GetCapacity() const { return m_capacity; }
    static constexpr WorkerMode GetProducerMode() { return m_producer_mode; }
    static constexpr WorkerMode GetConsumerMode() { return m_consumer_mode; }

private:
    unsigned int m_capacity; //!< maximum capacity the queue can hold above which pushes will block or fail
    std::deque<T> m_data;    //!< the data currently in the queue

    std::mutex m_queue_lock;          //!< synchronizes access to m_data
    CConditionVariable m_consumer_cv; //!< consumers waiting for m_data to become non-empty
    CConditionVariable m_producer_cv; //!< producers waiting for available space in m_data
};

template <typename T, WorkerMode consumer_mode>
using BlockingConsumerQueue = ProducerConsumerQueue<T, WorkerMode::BLOCKING, consumer_mode>;

#endif // BITCOIN_CORE_PRODUCERCONSUMERQUEUE_H
