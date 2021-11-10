/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   basic_sink_frontend.hpp
 * \author Andrey Semashev
 * \date   14.07.2009
 *
 * The header contains implementation of a base class for sink frontends.
 */

#ifndef BOOST_LOG_SINKS_BASIC_SINK_FRONTEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_BASIC_SINK_FRONTEND_HPP_INCLUDED_

#include <boost/mpl/bool.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/code_conversion.hpp>
#include <boost/log/detail/attachable_sstream_buf.hpp>
#include <boost/log/detail/fake_mutex.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/sinks/sink.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/expressions/filter.hpp>
#include <boost/log/expressions/formatter.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/memory_order.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/tss.hpp>
#include <boost/log/detail/locks.hpp>
#include <boost/log/detail/light_rw_mutex.hpp>
#endif // !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

//! A base class for a logging sink frontend
class BOOST_LOG_NO_VTABLE basic_sink_frontend :
    public sink
{
    //! Base type
    typedef sink base_type;

public:
    //! An exception handler type
    typedef base_type::exception_handler_type exception_handler_type;

#if !defined(BOOST_LOG_NO_THREADS)
protected:
    //! Mutex type
    typedef boost::log::aux::light_rw_mutex mutex_type;

private:
    //! Synchronization mutex
    mutable mutex_type m_Mutex;
#endif

private:
    //! Filter
    filter m_Filter;
    //! Exception handler
    exception_handler_type m_ExceptionHandler;

public:
    /*!
     * \brief Initializing constructor
     *
     * \param cross_thread The flag indicates whether the sink passes log records between different threads
     */
    explicit basic_sink_frontend(bool cross_thread) : sink(cross_thread)
    {
    }

    /*!
     * The method sets sink-specific filter functional object
     */
    template< typename FunT >
    void set_filter(FunT const& filter)
    {
        BOOST_LOG_EXPR_IF_MT(boost::log::aux::exclusive_lock_guard< mutex_type > lock(m_Mutex);)
        m_Filter = filter;
    }
    /*!
     * The method resets the filter
     */
    void reset_filter()
    {
        BOOST_LOG_EXPR_IF_MT(boost::log::aux::exclusive_lock_guard< mutex_type > lock(m_Mutex);)
        m_Filter.reset();
    }

    /*!
     * The method sets an exception handler function
     */
    template< typename FunT >
    void set_exception_handler(FunT const& handler)
    {
        BOOST_LOG_EXPR_IF_MT(boost::log::aux::exclusive_lock_guard< mutex_type > lock(m_Mutex);)
        m_ExceptionHandler = handler;
    }

    /*!
     * The method resets the exception handler function
     */
    void reset_exception_handler()
    {
        BOOST_LOG_EXPR_IF_MT(boost::log::aux::exclusive_lock_guard< mutex_type > lock(m_Mutex);)
        m_ExceptionHandler.clear();
    }

    /*!
     * The method returns \c true if no filter is set or the attribute values pass the filter
     *
     * \param attrs A set of attribute values of a logging record
     */
    bool will_consume(attribute_value_set const& attrs) BOOST_OVERRIDE
    {
        BOOST_LOG_EXPR_IF_MT(boost::log::aux::shared_lock_guard< mutex_type > lock(m_Mutex);)
        try
        {
            return m_Filter(attrs);
        }
#if !defined(BOOST_LOG_NO_THREADS)
        catch (thread_interrupted&)
        {
            throw;
        }
#endif
        catch (...)
        {
            if (m_ExceptionHandler.empty())
                throw;
            m_ExceptionHandler();
            return false;
        }
    }

protected:
#if !defined(BOOST_LOG_NO_THREADS)
    //! Returns reference to the frontend mutex
    mutex_type& frontend_mutex() const { return m_Mutex; }
#endif

    //! Returns reference to the exception handler
    exception_handler_type& exception_handler() { return m_ExceptionHandler; }
    //! Returns reference to the exception handler
    exception_handler_type const& exception_handler() const { return m_ExceptionHandler; }

    //! Feeds log record to the backend
    template< typename BackendMutexT, typename BackendT >
    void feed_record(record_view const& rec, BackendMutexT& backend_mutex, BackendT& backend)
    {
        try
        {
            BOOST_LOG_EXPR_IF_MT(boost::log::aux::exclusive_lock_guard< BackendMutexT > lock(backend_mutex);)
            backend.consume(rec);
        }
#if !defined(BOOST_LOG_NO_THREADS)
        catch (thread_interrupted&)
        {
            throw;
        }
#endif
        catch (...)
        {
            BOOST_LOG_EXPR_IF_MT(boost::log::aux::shared_lock_guard< mutex_type > lock(m_Mutex);)
            if (m_ExceptionHandler.empty())
                throw;
            m_ExceptionHandler();
        }
    }

    //! Attempts to feeds log record to the backend, does not block if \a backend_mutex is locked
    template< typename BackendMutexT, typename BackendT >
    bool try_feed_record(record_view const& rec, BackendMutexT& backend_mutex, BackendT& backend)
    {
#if !defined(BOOST_LOG_NO_THREADS)
        try
        {
            if (!backend_mutex.try_lock())
                return false;
        }
        catch (thread_interrupted&)
        {
            throw;
        }
        catch (...)
        {
            boost::log::aux::shared_lock_guard< mutex_type > frontend_lock(this->frontend_mutex());
            if (this->exception_handler().empty())
                throw;
            this->exception_handler()();
            return false;
        }

        boost::log::aux::exclusive_auto_unlocker< BackendMutexT > unlocker(backend_mutex);
#endif
        // No need to lock anything in the feed_record method
        boost::log::aux::fake_mutex m;
        feed_record(rec, m, backend);
        return true;
    }

    //! Flushes record buffers in the backend, if one supports it
    template< typename BackendMutexT, typename BackendT >
    void flush_backend(BackendMutexT& backend_mutex, BackendT& backend)
    {
        typedef typename BackendT::frontend_requirements frontend_requirements;
        flush_backend_impl(backend_mutex, backend,
            typename has_requirement< frontend_requirements, flushing >::type());
    }

private:
    //! Flushes record buffers in the backend (the actual implementation)
    template< typename BackendMutexT, typename BackendT >
    void flush_backend_impl(BackendMutexT& backend_mutex, BackendT& backend, mpl::true_)
    {
        try
        {
            BOOST_LOG_EXPR_IF_MT(boost::log::aux::exclusive_lock_guard< BackendMutexT > lock(backend_mutex);)
            backend.flush();
        }
#if !defined(BOOST_LOG_NO_THREADS)
        catch (thread_interrupted&)
        {
            throw;
        }
#endif
        catch (...)
        {
            BOOST_LOG_EXPR_IF_MT(boost::log::aux::shared_lock_guard< mutex_type > lock(m_Mutex);)
            if (m_ExceptionHandler.empty())
                throw;
            m_ExceptionHandler();
        }
    }
    //! Flushes record buffers in the backend (stub for backends that don't support flushing)
    template< typename BackendMutexT, typename BackendT >
    void flush_backend_impl(BackendMutexT&, BackendT&, mpl::false_)
    {
    }
};

//! A base class for a logging sink frontend with formatting support
template< typename CharT >
class BOOST_LOG_NO_VTABLE basic_formatting_sink_frontend :
    public basic_sink_frontend
{
    typedef basic_sink_frontend base_type;

public:
    //! Character type
    typedef CharT char_type;
    //! Formatted string type
    typedef std::basic_string< char_type > string_type;

    //! Formatter function object type
    typedef basic_formatter< char_type > formatter_type;
    //! Output stream type
    typedef typename formatter_type::stream_type stream_type;

#if !defined(BOOST_LOG_NO_THREADS)
protected:
    //! Mutex type
    typedef typename base_type::mutex_type mutex_type;
#endif

private:
    struct formatting_context
    {
        class cleanup_guard
        {
        private:
            formatting_context& m_context;

        public:
            explicit cleanup_guard(formatting_context& ctx) BOOST_NOEXCEPT : m_context(ctx)
            {
            }

            ~cleanup_guard()
            {
                m_context.m_FormattedRecord.clear();
                m_context.m_FormattingStream.rdbuf()->max_size(m_context.m_FormattedRecord.max_size());
                m_context.m_FormattingStream.rdbuf()->storage_overflow(false);
                m_context.m_FormattingStream.clear();
            }

            BOOST_DELETED_FUNCTION(cleanup_guard(cleanup_guard const&))
            BOOST_DELETED_FUNCTION(cleanup_guard& operator=(cleanup_guard const&))
        };

#if !defined(BOOST_LOG_NO_THREADS)
        //! Object version
        const unsigned int m_Version;
#endif
        //! Formatted log record storage
        string_type m_FormattedRecord;
        //! Formatting stream
        stream_type m_FormattingStream;
        //! Formatter functor
        formatter_type m_Formatter;

        formatting_context() :
#if !defined(BOOST_LOG_NO_THREADS)
            m_Version(0u),
#endif
            m_FormattingStream(m_FormattedRecord)
        {
            m_FormattingStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
        }
#if !defined(BOOST_LOG_NO_THREADS)
        formatting_context(unsigned int version, std::locale const& loc, formatter_type const& formatter) :
            m_Version(version),
            m_FormattingStream(m_FormattedRecord),
            m_Formatter(formatter)
        {
            m_FormattingStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
            m_FormattingStream.imbue(loc);
        }
#endif
    };

private:
#if !defined(BOOST_LOG_NO_THREADS)

    //! State version
    boost::atomic< unsigned int > m_Version;

    //! Formatter functor
    formatter_type m_Formatter;
    //! Locale to perform formatting
    std::locale m_Locale;

    //! Formatting state
    thread_specific_ptr< formatting_context > m_pContext;

#else

    //! Formatting state
    formatting_context m_Context;

#endif // !defined(BOOST_LOG_NO_THREADS)

public:
    /*!
     * \brief Initializing constructor
     *
     * \param cross_thread The flag indicates whether the sink passes log records between different threads
     */
    explicit basic_formatting_sink_frontend(bool cross_thread) :
        basic_sink_frontend(cross_thread)
#if !defined(BOOST_LOG_NO_THREADS)
        , m_Version(0u)
#endif
    {
    }

    /*!
     * The method sets sink-specific formatter function object
     */
    template< typename FunT >
    void set_formatter(FunT const& formatter)
    {
#if !defined(BOOST_LOG_NO_THREADS)
        boost::log::aux::exclusive_lock_guard< mutex_type > lock(this->frontend_mutex());
        m_Formatter = formatter;
        m_Version.opaque_add(1u, boost::memory_order_relaxed);
#else
        m_Context.m_Formatter = formatter;
#endif
    }
    /*!
     * The method resets the formatter
     */
    void reset_formatter()
    {
#if !defined(BOOST_LOG_NO_THREADS)
        boost::log::aux::exclusive_lock_guard< mutex_type > lock(this->frontend_mutex());
        m_Formatter.reset();
        m_Version.opaque_add(1u, boost::memory_order_relaxed);
#else
        m_Context.m_Formatter.reset();
#endif
    }

    /*!
     * The method returns the current locale used for formatting
     */
    std::locale getloc() const
    {
#if !defined(BOOST_LOG_NO_THREADS)
        boost::log::aux::exclusive_lock_guard< mutex_type > lock(this->frontend_mutex());
        return m_Locale;
#else
        return m_Context.m_FormattingStream.getloc();
#endif
    }
    /*!
     * The method sets the locale used for formatting
     */
    void imbue(std::locale const& loc)
    {
#if !defined(BOOST_LOG_NO_THREADS)
        boost::log::aux::exclusive_lock_guard< mutex_type > lock(this->frontend_mutex());
        m_Locale = loc;
        m_Version.opaque_add(1u, boost::memory_order_relaxed);
#else
        m_Context.m_FormattingStream.imbue(loc);
#endif
    }

protected:
    //! Returns reference to the formatter
    formatter_type& formatter()
    {
#if !defined(BOOST_LOG_NO_THREADS)
        return m_Formatter;
#else
        return m_Context.m_Formatter;
#endif
    }

    //! Feeds log record to the backend
    template< typename BackendMutexT, typename BackendT >
    void feed_record(record_view const& rec, BackendMutexT& backend_mutex, BackendT& backend)
    {
        formatting_context* context;

#if !defined(BOOST_LOG_NO_THREADS)
        context = m_pContext.get();
        if (!context || context->m_Version != m_Version.load(boost::memory_order_relaxed))
        {
            {
                boost::log::aux::shared_lock_guard< mutex_type > lock(this->frontend_mutex());
                context = new formatting_context(m_Version.load(boost::memory_order_relaxed), m_Locale, m_Formatter);
            }
            m_pContext.reset(context);
        }
#else
        context = &m_Context;
#endif

        typename formatting_context::cleanup_guard cleanup(*context);

        try
        {
            // Perform the formatting
            context->m_Formatter(rec, context->m_FormattingStream);
            context->m_FormattingStream.flush();

            // Feed the record
            BOOST_LOG_EXPR_IF_MT(boost::log::aux::exclusive_lock_guard< BackendMutexT > lock(backend_mutex);)
            backend.consume(rec, context->m_FormattedRecord);
        }
#if !defined(BOOST_LOG_NO_THREADS)
        catch (thread_interrupted&)
        {
            throw;
        }
#endif
        catch (...)
        {
            BOOST_LOG_EXPR_IF_MT(boost::log::aux::shared_lock_guard< mutex_type > lock(this->frontend_mutex());)
            if (this->exception_handler().empty())
                throw;
            this->exception_handler()();
        }
    }

    //! Attempts to feeds log record to the backend, does not block if \a backend_mutex is locked
    template< typename BackendMutexT, typename BackendT >
    bool try_feed_record(record_view const& rec, BackendMutexT& backend_mutex, BackendT& backend)
    {
#if !defined(BOOST_LOG_NO_THREADS)
        try
        {
            if (!backend_mutex.try_lock())
                return false;
        }
        catch (thread_interrupted&)
        {
            throw;
        }
        catch (...)
        {
            boost::log::aux::shared_lock_guard< mutex_type > frontend_lock(this->frontend_mutex());
            if (this->exception_handler().empty())
                throw;
            this->exception_handler()();
            return false;
        }

        boost::log::aux::exclusive_auto_unlocker< BackendMutexT > unlocker(backend_mutex);
#endif
        // No need to lock anything in the feed_record method
        boost::log::aux::fake_mutex m;
        feed_record(rec, m, backend);
        return true;
    }
};

namespace aux {

    template<
        typename BackendT,
        bool RequiresFormattingV = has_requirement<
            typename BackendT::frontend_requirements,
            formatted_records
        >::value
    >
    struct make_sink_frontend_base
    {
        typedef basic_sink_frontend type;
    };
    template< typename BackendT >
    struct make_sink_frontend_base< BackendT, true >
    {
        typedef basic_formatting_sink_frontend< typename BackendT::char_type > type;
    };

} // namespace aux

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_BASIC_SINK_FRONTEND_HPP_INCLUDED_
