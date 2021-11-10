/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   bounded_fifo_queue.hpp
 * \author Andrey Semashev
 * \date   04.01.2012
 *
 * The header contains implementation of bounded FIFO queueing strategy for
 * the asynchronous sink frontend.
 */

#ifndef BOOST_LOG_SINKS_BOUNDED_FIFO_QUEUE_HPP_INCLUDED_
#define BOOST_LOG_SINKS_BOUNDED_FIFO_QUEUE_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_LOG_NO_THREADS)
#error Boost.Log: This header content is only supported in multithreaded environment
#endif

#include <cstddef>
#include <queue>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

/*!
 * \brief Bounded FIFO log record queueing strategy
 *
 * The \c bounded_fifo_queue class is intended to be used with
 * the \c asynchronous_sink frontend as a log record queueing strategy.
 *
 * This strategy describes log record queueing logic.
 * The queue has a limited capacity, upon reaching which the enqueue operation will
 * invoke the overflow handling strategy specified in the \c OverflowStrategyT
 * template parameter to handle the situation. The library provides overflow handling
 * strategies for most common cases: \c drop_on_overflow will silently discard the log record,
 * and \c block_on_overflow will put the enqueueing thread to wait until there is space
 * in the queue.
 *
 * The log record queue imposes no ordering over the queued
 * elements aside from the order in which they are enqueued.
 */
template< std::size_t MaxQueueSizeV, typename OverflowStrategyT >
class bounded_fifo_queue :
    private OverflowStrategyT
{
private:
    typedef OverflowStrategyT overflow_strategy;
    typedef std::queue< record_view > queue_type;
    typedef boost::mutex mutex_type;

private:
    //! Synchronization primitive
    mutex_type m_mutex;
    //! Condition to block the consuming thread on
    condition_variable m_cond;
    //! Log record queue
    queue_type m_queue;
    //! Interruption flag
    bool m_interruption_requested;

protected:
    //! Default constructor
    bounded_fifo_queue() : m_interruption_requested(false)
    {
    }
    //! Initializing constructor
    template< typename ArgsT >
    explicit bounded_fifo_queue(ArgsT const&) : m_interruption_requested(false)
    {
    }

    //! Enqueues log record to the queue
    void enqueue(record_view const& rec)
    {
        unique_lock< mutex_type > lock(m_mutex);
        std::size_t size = m_queue.size();
        for (; size >= MaxQueueSizeV; size = m_queue.size())
        {
            if (!overflow_strategy::on_overflow(rec, lock))
                return;
        }

        m_queue.push(rec);
        if (size == 0)
            m_cond.notify_one();
    }

    //! Attempts to enqueue log record to the queue
    bool try_enqueue(record_view const& rec)
    {
        unique_lock< mutex_type > lock(m_mutex, try_to_lock);
        if (lock.owns_lock())
        {
            const std::size_t size = m_queue.size();

            // Do not invoke the bounding strategy in case of overflow as it may block
            if (size < MaxQueueSizeV)
            {
                m_queue.push(rec);
                if (size == 0)
                    m_cond.notify_one();
                return true;
            }
        }

        return false;
    }

    //! Attempts to dequeue a log record ready for processing from the queue, does not block if the queue is empty
    bool try_dequeue_ready(record_view& rec)
    {
        return try_dequeue(rec);
    }

    //! Attempts to dequeue log record from the queue, does not block if the queue is empty
    bool try_dequeue(record_view& rec)
    {
        lock_guard< mutex_type > lock(m_mutex);
        const std::size_t size = m_queue.size();
        if (size > 0)
        {
            rec.swap(m_queue.front());
            m_queue.pop();
            overflow_strategy::on_queue_space_available();
            return true;
        }

        return false;
    }

    //! Dequeues log record from the queue, blocks if the queue is empty
    bool dequeue_ready(record_view& rec)
    {
        unique_lock< mutex_type > lock(m_mutex);

        while (!m_interruption_requested)
        {
            const std::size_t size = m_queue.size();
            if (size > 0)
            {
                rec.swap(m_queue.front());
                m_queue.pop();
                overflow_strategy::on_queue_space_available();
                return true;
            }
            else
            {
                m_cond.wait(lock);
            }
        }
        m_interruption_requested = false;

        return false;
    }

    //! Wakes a thread possibly blocked in the \c dequeue method
    void interrupt_dequeue()
    {
        lock_guard< mutex_type > lock(m_mutex);
        m_interruption_requested = true;
        overflow_strategy::interrupt();
        m_cond.notify_one();
    }
};

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_BOUNDED_FIFO_QUEUE_HPP_INCLUDED_
