/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   block_on_overflow.hpp
 * \author Andrey Semashev
 * \date   04.01.2012
 *
 * The header contains implementation of \c block_on_overflow strategy for handling
 * queue overflows in bounded queues for the asynchronous sink frontend.
 */

#ifndef BOOST_LOG_SINKS_BLOCK_ON_OVERFLOW_HPP_INCLUDED_
#define BOOST_LOG_SINKS_BLOCK_ON_OVERFLOW_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_LOG_NO_THREADS)
#error Boost.Log: This header content is only supported in multithreaded environment
#endif

#include <boost/intrusive/options.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

/*!
 * \brief Blocking strategy for handling log record queue overflows
 *
 * This strategy will cause enqueueing threads to block when the
 * log record queue overflows. The blocked threads will be woken as
 * soon as there appears free space in the queue, in the same order
 * they attempted to enqueue records.
 */
class block_on_overflow
{
#ifndef BOOST_LOG_DOXYGEN_PASS
private:
    typedef intrusive::list_base_hook<
        intrusive::link_mode< intrusive::auto_unlink >
    > thread_context_hook_t;

    struct thread_context :
        public thread_context_hook_t
    {
        condition_variable cond;
        bool result;

        thread_context() : result(true) {}
    };

    typedef intrusive::list<
        thread_context,
        intrusive::base_hook< thread_context_hook_t >,
        intrusive::constant_time_size< false >
    > thread_contexts;

private:
    //! Blocked threads
    thread_contexts m_thread_contexts;

public:
    /*!
     * Default constructor.
     */
    BOOST_DEFAULTED_FUNCTION(block_on_overflow(), {})

    /*!
     * This method is called by the queue when overflow is detected.
     *
     * \param lock An internal lock that protects the queue
     *
     * \retval true Attempt to enqueue the record again.
     * \retval false Discard the record.
     */
    template< typename LockT >
    bool on_overflow(record_view const&, LockT& lock)
    {
        thread_context context;
        m_thread_contexts.push_back(context);
        do
        {
            context.cond.wait(lock);
        }
        while (context.is_linked());

        return context.result;
    }

    /*!
     * This method is called by the queue when there appears a free space.
     * The internal lock protecting the queue is locked when calling this method.
     */
    void on_queue_space_available()
    {
        if (!m_thread_contexts.empty())
        {
            m_thread_contexts.front().cond.notify_one();
            m_thread_contexts.pop_front();
        }
    }

    /*!
     * This method is called by the queue to interrupt any possible waits in \c on_overflow.
     * The internal lock protecting the queue is locked when calling this method.
     */
    void interrupt()
    {
        while (!m_thread_contexts.empty())
        {
            thread_context& context = m_thread_contexts.front();
            context.result = false;
            context.cond.notify_one();
            m_thread_contexts.pop_front();
        }
    }

    //  Copying prohibited
    BOOST_DELETED_FUNCTION(block_on_overflow(block_on_overflow const&))
    BOOST_DELETED_FUNCTION(block_on_overflow& operator= (block_on_overflow const&))
#endif // BOOST_LOG_DOXYGEN_PASS
};

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_BLOCK_ON_OVERFLOW_HPP_INCLUDED_
