/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   async_frontend.hpp
 * \author Andrey Semashev
 * \date   14.07.2009
 *
 * The header contains implementation of asynchronous sink frontend.
 */

#ifndef BOOST_LOG_SINKS_ASYNC_FRONTEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_ASYNC_FRONTEND_HPP_INCLUDED_

#include <exception> // std::terminate
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_LOG_NO_THREADS)
#error Boost.Log: Asynchronous sink frontend is only supported in multithreaded environment
#endif

#include <boost/static_assert.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/detail/locking_ptr.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/sinks/basic_sink_frontend.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/sinks/unbounded_fifo_queue.hpp>
#include <boost/log/keywords/start_thread.hpp>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_1(z, n, data)\
    template< typename T0 >\
    explicit asynchronous_sink(T0 const& arg0, typename boost::log::aux::enable_if_named_parameters< T0, boost::log::aux::sfinae_dummy >::type = boost::log::aux::sfinae_dummy()) :\
        base_type(true),\
        queue_base_type(arg0),\
        m_pBackend(boost::make_shared< sink_backend_type >(arg0)),\
        m_ActiveOperation(idle),\
        m_StopRequested(false),\
        m_FlushRequested(false)\
    {\
        if (arg0[keywords::start_thread | true])\
            start_feeding_thread();\
    }\
    template< typename T0 >\
    explicit asynchronous_sink(shared_ptr< sink_backend_type > const& backend, T0 const& arg0) :\
        base_type(true),\
        queue_base_type(arg0),\
        m_pBackend(backend),\
        m_ActiveOperation(idle),\
        m_StopRequested(false),\
        m_FlushRequested(false)\
    {\
        if (arg0[keywords::start_thread | true])\
            start_feeding_thread();\
    }

#define BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_N(z, n, data)\
    template< BOOST_PP_ENUM_PARAMS_Z(z, n, typename T) >\
    explicit asynchronous_sink(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, T, const& arg)) :\
        base_type(true),\
        queue_base_type((BOOST_PP_ENUM_PARAMS_Z(z, n, arg))),\
        m_pBackend(boost::make_shared< sink_backend_type >(BOOST_PP_ENUM_PARAMS_Z(z, n, arg))),\
        m_ActiveOperation(idle),\
        m_StopRequested(false),\
        m_FlushRequested(false)\
    {\
        if ((BOOST_PP_ENUM_PARAMS_Z(z, n, arg))[keywords::start_thread | true])\
            start_feeding_thread();\
    }\
    template< BOOST_PP_ENUM_PARAMS_Z(z, n, typename T) >\
    explicit asynchronous_sink(shared_ptr< sink_backend_type > const& backend, BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, T, const& arg)) :\
        base_type(true),\
        queue_base_type((BOOST_PP_ENUM_PARAMS_Z(z, n, arg))),\
        m_pBackend(backend),\
        m_ActiveOperation(idle),\
        m_StopRequested(false),\
        m_FlushRequested(false)\
    {\
        if ((BOOST_PP_ENUM_PARAMS_Z(z, n, arg))[keywords::start_thread | true])\
            start_feeding_thread();\
    }

#define BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL(z, n, data)\
    BOOST_PP_IF(BOOST_PP_EQUAL(n, 1), BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_1, BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_N)(z, n, data)

#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief Asynchronous logging sink frontend
 *
 * The frontend starts a separate thread on construction. All logging records are passed
 * to the backend in this dedicated thread.
 *
 * The user can prevent spawning the internal thread by specifying \c start_thread parameter
 * with the value of \c false on construction. In this case log records will be buffered
 * in the internal queue until the user calls \c run, \c feed_records or \c flush in his own
 * thread. Log record queueing strategy is specified in the \c QueueingStrategyT template
 * parameter.
 */
template< typename SinkBackendT, typename QueueingStrategyT = unbounded_fifo_queue >
class asynchronous_sink :
    public aux::make_sink_frontend_base< SinkBackendT >::type,
    public QueueingStrategyT
{
    typedef typename aux::make_sink_frontend_base< SinkBackendT >::type base_type;
    typedef QueueingStrategyT queue_base_type;

private:
    //! Backend synchronization mutex type
    typedef boost::recursive_mutex backend_mutex_type;
    //! Frontend synchronization mutex type
    typedef typename base_type::mutex_type frontend_mutex_type;

    //! Operation bit mask
    enum operation
    {
        idle = 0u,
        feeding_records = 1u,
        flushing = 3u
    };

    //! Function object to run the log record feeding thread
    class run_func
    {
    public:
        typedef void result_type;

    private:
        asynchronous_sink* m_self;

    public:
        explicit run_func(asynchronous_sink* self) BOOST_NOEXCEPT : m_self(self)
        {
        }

        result_type operator()() const
        {
            m_self->run();
        }
    };

    //! A scope guard that implements active operation management
    class scoped_feeding_opereation
    {
    private:
        asynchronous_sink& m_self;

    public:
        //! Initializing constructor
        explicit scoped_feeding_opereation(asynchronous_sink& self) : m_self(self)
        {
        }
        //! Destructor
        ~scoped_feeding_opereation()
        {
            m_self.complete_feeding_operation();
        }

        BOOST_DELETED_FUNCTION(scoped_feeding_opereation(scoped_feeding_opereation const&))
        BOOST_DELETED_FUNCTION(scoped_feeding_opereation& operator= (scoped_feeding_opereation const&))
    };

    //! A scope guard that resets a flag on destructor
    class scoped_flag
    {
    private:
        frontend_mutex_type& m_Mutex;
        condition_variable_any& m_Cond;
        boost::atomic< bool >& m_Flag;

    public:
        explicit scoped_flag(frontend_mutex_type& mut, condition_variable_any& cond, boost::atomic< bool >& f) :
            m_Mutex(mut), m_Cond(cond), m_Flag(f)
        {
        }
        ~scoped_flag()
        {
            try
            {
                lock_guard< frontend_mutex_type > lock(m_Mutex);
                m_Flag.store(false, boost::memory_order_relaxed);
                m_Cond.notify_all();
            }
            catch (...)
            {
            }
        }

        BOOST_DELETED_FUNCTION(scoped_flag(scoped_flag const&))
        BOOST_DELETED_FUNCTION(scoped_flag& operator= (scoped_flag const&))
    };

public:
    //! Sink implementation type
    typedef SinkBackendT sink_backend_type;
    //! \cond
    BOOST_STATIC_ASSERT_MSG((has_requirement< typename sink_backend_type::frontend_requirements, synchronized_feeding >::value), "Asynchronous sink frontend is incompatible with the specified backend: thread synchronization requirements are not met");
    //! \endcond

#ifndef BOOST_LOG_DOXYGEN_PASS

    //! A pointer type that locks the backend until it's destroyed
    typedef boost::log::aux::locking_ptr< sink_backend_type, backend_mutex_type > locked_backend_ptr;

#else // BOOST_LOG_DOXYGEN_PASS

    //! A pointer type that locks the backend until it's destroyed
    typedef implementation_defined locked_backend_ptr;

#endif // BOOST_LOG_DOXYGEN_PASS

private:
    //! Synchronization mutex
    backend_mutex_type m_BackendMutex;
    //! Pointer to the backend
    const shared_ptr< sink_backend_type > m_pBackend;

    //! Dedicated record feeding thread
    thread m_DedicatedFeedingThread;
    //! Condition variable to implement blocking operations
    condition_variable_any m_BlockCond;

    //! Currently active operation
    operation m_ActiveOperation;
    //! The flag indicates that the feeding loop has to be stopped
    boost::atomic< bool > m_StopRequested;
    //! The flag indicates that queue flush has been requested
    boost::atomic< bool > m_FlushRequested;

public:
    /*!
     * Default constructor. Constructs the sink backend instance.
     * Requires the backend to be default-constructible.
     *
     * \param start_thread If \c true, the frontend creates a thread to feed
     *                     log records to the backend. Otherwise no thread is
     *                     started and it is assumed that the user will call
     *                     \c run, \c feed_records or \c flush himself.
     */
    explicit asynchronous_sink(bool start_thread = true) :
        base_type(true),
        m_pBackend(boost::make_shared< sink_backend_type >()),
        m_ActiveOperation(idle),
        m_StopRequested(false),
        m_FlushRequested(false)
    {
        if (start_thread)
            start_feeding_thread();
    }
    /*!
     * Constructor attaches user-constructed backend instance
     *
     * \param backend Pointer to the backend instance.
     * \param start_thread If \c true, the frontend creates a thread to feed
     *                     log records to the backend. Otherwise no thread is
     *                     started and it is assumed that the user will call
     *                     \c run, \c feed_records or \c flush himself.
     *
     * \pre \a backend is not \c NULL.
     */
    explicit asynchronous_sink(shared_ptr< sink_backend_type > const& backend, bool start_thread = true) :
        base_type(true),
        m_pBackend(backend),
        m_ActiveOperation(idle),
        m_StopRequested(false),
        m_FlushRequested(false)
    {
        if (start_thread)
            start_feeding_thread();
    }

    /*!
     * Constructor that passes arbitrary named parameters to the interprocess sink backend constructor.
     * Refer to the backend documentation for the list of supported parameters.
     *
     * The frontend uses the following named parameters:
     *
     *   \li start_thread - If \c true, the frontend creates a thread to feed
     *                      log records to the backend. Otherwise no thread is
     *                      started and it is assumed that the user will call
     *                      \c run, \c feed_records or \c flush himself.
     */
#ifndef BOOST_LOG_DOXYGEN_PASS
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_GEN(BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL, ~)
#else
    template< typename... Args >
    explicit asynchronous_sink(Args&&... args);
#endif

    /*!
     * Destructor. Implicitly stops the dedicated feeding thread, if one is running.
     */
    ~asynchronous_sink() BOOST_NOEXCEPT BOOST_OVERRIDE
    {
        try
        {
            boost::this_thread::disable_interruption no_interrupts;
            stop();
        }
        catch (...)
        {
            std::terminate();
        }
    }

    /*!
     * Locking accessor to the attached backend
     */
    locked_backend_ptr locked_backend()
    {
        return locked_backend_ptr(m_pBackend, m_BackendMutex);
    }

    /*!
     * Enqueues the log record to the backend
     */
    void consume(record_view const& rec) BOOST_OVERRIDE
    {
        if (BOOST_UNLIKELY(m_FlushRequested.load(boost::memory_order_acquire)))
        {
            unique_lock< frontend_mutex_type > lock(base_type::frontend_mutex());
            // Wait until flush is done
            while (m_FlushRequested.load(boost::memory_order_acquire))
                m_BlockCond.wait(lock);
        }
        queue_base_type::enqueue(rec);
    }

    /*!
     * The method attempts to pass logging record to the backend
     */
    bool try_consume(record_view const& rec) BOOST_OVERRIDE
    {
        if (!m_FlushRequested.load(boost::memory_order_acquire))
        {
            return queue_base_type::try_enqueue(rec);
        }
        else
            return false;
    }

    /*!
     * The method starts record feeding loop and effectively blocks until either of this happens:
     *
     * \li the thread is interrupted due to either standard thread interruption or a call to \c stop
     * \li an exception is thrown while processing a log record in the backend, and the exception is
     *     not terminated by the exception handler, if one is installed
     *
     * \pre The sink frontend must be constructed without spawning a dedicated thread
     */
    void run()
    {
        // First check that no other thread is running
        {
            unique_lock< frontend_mutex_type > lock(base_type::frontend_mutex());
            if (start_feeding_operation(lock, feeding_records))
                return;
        }

        scoped_feeding_opereation guard(*this);

        // Now start the feeding loop
        while (true)
        {
            do_feed_records();
            if (!m_StopRequested.load(boost::memory_order_acquire))
            {
                // Block until new record is available
                record_view rec;
                if (queue_base_type::dequeue_ready(rec))
                    base_type::feed_record(rec, m_BackendMutex, *m_pBackend);
            }
            else
                break;
        }
    }

    /*!
     * The method softly interrupts record feeding loop. This method must be called when \c run,
     * \c feed_records or \c flush method execution has to be interrupted. Unlike regular thread
     * interruption, calling \c stop will not interrupt the record processing in the middle.
     * Instead, the sink frontend will attempt to finish its business with the record in progress
     * and return afterwards. This method can be called either if the sink was created with
     * an internal dedicated thread, or if the feeding loop was initiated by user.
     *
     * If no record feeding operation is in progress, calling \c stop marks the sink frontend
     * so that the next feeding operation stops immediately.
     *
     * \note Returning from this method does not guarantee that there are no records left buffered
     *       in the sink frontend. It is possible that log records keep coming during and after this
     *       method is called. At some point of execution of this method log records stop being processed,
     *       and all records that come after this point are put into the queue. These records will be
     *       processed upon further calls to \c run or \c feed_records.
     *
     * \note If the record feeding loop is being run in a user's thread (i.e. \c start_thread was specified
     *       as \c false on frontend construction), this method does not guarantee that upon return the thread
     *       has returned from the record feeding loop or that it won't enter it in the future. The method
     *       only ensures that the record feeding thread will eventually return from the feeding loop. It is
     *       user's responsibility to synchronize with the user's record feeding thread.
     */
    void stop()
    {
        boost::thread feeding_thread;
        {
            lock_guard< frontend_mutex_type > lock(base_type::frontend_mutex());

            m_StopRequested.store(true, boost::memory_order_release);
            queue_base_type::interrupt_dequeue();

            m_DedicatedFeedingThread.swap(feeding_thread);
        }

        if (feeding_thread.joinable())
            feeding_thread.join();
    }

    /*!
     * The method feeds log records that may have been buffered to the backend and returns
     *
     * \pre The sink frontend must be constructed without spawning a dedicated thread
     */
    void feed_records()
    {
        // First check that no other thread is running
        {
            unique_lock< frontend_mutex_type > lock(base_type::frontend_mutex());
            if (start_feeding_operation(lock, feeding_records))
                return;
        }

        scoped_feeding_opereation guard(*this);

        // Now start the feeding loop
        do_feed_records();
    }

    /*!
     * The method feeds all log records that may have been buffered to the backend and returns.
     * Unlike \c feed_records, in case of ordering queueing the method also feeds records
     * that were enqueued during the ordering window, attempting to drain the queue completely.
     */
    void flush() BOOST_OVERRIDE
    {
        {
            unique_lock< frontend_mutex_type > lock(base_type::frontend_mutex());
            if (static_cast< unsigned int >(m_ActiveOperation & feeding_records) != 0u)
            {
                // There is already a thread feeding records, let it do the job
                m_FlushRequested.store(true, boost::memory_order_release);
                queue_base_type::interrupt_dequeue();
                while (!m_StopRequested.load(boost::memory_order_acquire) && m_FlushRequested.load(boost::memory_order_acquire))
                    m_BlockCond.wait(lock);

                // The condition may have been signalled when the feeding operation was finishing.
                // In that case records may not have been flushed, and we do the flush ourselves.
                if (m_ActiveOperation != idle)
                    return;
            }

            m_ActiveOperation = flushing;
            m_FlushRequested.store(true, boost::memory_order_relaxed);
        }

        scoped_feeding_opereation guard(*this);

        do_feed_records();
    }

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! The method spawns record feeding thread
    void start_feeding_thread()
    {
        boost::thread(run_func(this)).swap(m_DedicatedFeedingThread);
    }

    //! Starts record feeding operation. The method blocks or throws if another feeding operation is in progress.
    bool start_feeding_operation(unique_lock< frontend_mutex_type >& lock, operation op)
    {
        while (m_ActiveOperation != idle)
        {
            if (BOOST_UNLIKELY(op == feeding_records && m_ActiveOperation == feeding_records))
                BOOST_LOG_THROW_DESCR(unexpected_call, "Asynchronous sink frontend already runs a record feeding thread");

            if (BOOST_UNLIKELY(m_StopRequested.load(boost::memory_order_relaxed)))
            {
                m_StopRequested.store(false, boost::memory_order_relaxed);
                return true;
            }

            m_BlockCond.wait(lock);
        }

        m_ActiveOperation = op;

        return false;
    }

    //! Completes record feeding operation
    void complete_feeding_operation() BOOST_NOEXCEPT
    {
        try
        {
            lock_guard< frontend_mutex_type > lock(base_type::frontend_mutex());
            m_ActiveOperation = idle;
            m_StopRequested.store(false, boost::memory_order_relaxed);
            m_BlockCond.notify_all();
        }
        catch (...)
        {
        }
    }

    //! The record feeding loop
    void do_feed_records()
    {
        while (!m_StopRequested.load(boost::memory_order_acquire))
        {
            record_view rec;
            bool dequeued = false;
            if (BOOST_LIKELY(!m_FlushRequested.load(boost::memory_order_acquire)))
                dequeued = queue_base_type::try_dequeue_ready(rec);
            else
                dequeued = queue_base_type::try_dequeue(rec);

            if (dequeued)
                base_type::feed_record(rec, m_BackendMutex, *m_pBackend);
            else
                break;
        }

        if (BOOST_UNLIKELY(m_FlushRequested.load(boost::memory_order_acquire)))
        {
            scoped_flag guard(base_type::frontend_mutex(), m_BlockCond, m_FlushRequested);
            base_type::flush_backend(m_BackendMutex, *m_pBackend);
        }
    }
#endif // BOOST_LOG_DOXYGEN_PASS
};

#undef BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_1
#undef BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_N
#undef BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_ASYNC_FRONTEND_HPP_INCLUDED_
