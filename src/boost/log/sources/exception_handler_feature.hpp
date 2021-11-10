/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   exception_handler_feature.hpp
 * \author Andrey Semashev
 * \date   17.07.2009
 *
 * The header contains implementation of an exception handler support feature.
 */

#ifndef BOOST_LOG_SOURCES_EXCEPTION_HANDLER_FEATURE_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_EXCEPTION_HANDLER_FEATURE_HPP_INCLUDED_

#include <boost/mpl/if.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/detail/locks.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/sources/threading_models.hpp>
#include <boost/log/utility/strictest_lock.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/thread/exceptions.hpp>
#endif
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sources {

/*!
 * \brief Exception handler feature implementation
 */
template< typename BaseT >
class basic_exception_handler_logger :
    public BaseT
{
    //! Base type
    typedef BaseT base_type;
    typedef basic_exception_handler_logger this_type;
    BOOST_COPYABLE_AND_MOVABLE_ALT(this_type)

public:
    //! Threading model being used
    typedef typename base_type::threading_model threading_model;
    //! Final logger type
    typedef typename base_type::final_type final_type;
    //! Exception handler function type
    typedef boost::log::aux::light_function< void () > exception_handler_type;

#if defined(BOOST_LOG_DOXYGEN_PASS)
    //! Lock requirement for the open_record_unlocked method
    typedef typename strictest_lock<
        typename base_type::open_record_lock,
        no_lock< threading_model >
    >::type open_record_lock;
    //! Lock requirement for the push_record_unlocked method
    typedef typename strictest_lock<
        typename base_type::push_record_lock,
        no_lock< threading_model >
    >::type push_record_lock;
#endif // defined(BOOST_LOG_DOXYGEN_PASS)

    //! Lock requirement for the swap_unlocked method
    typedef typename strictest_lock<
        typename base_type::swap_lock,
#ifndef BOOST_LOG_NO_THREADS
        boost::log::aux::multiple_unique_lock2< threading_model, threading_model >
#else
        no_lock< threading_model >
#endif // !defined(BOOST_LOG_NO_THREADS)
    >::type swap_lock;

private:
    //! Exception handler
    exception_handler_type m_ExceptionHandler;

public:
    /*!
     * Default constructor. The constructed logger does not have an exception handler.
     */
    basic_exception_handler_logger() : base_type()
    {
    }
    /*!
     * Copy constructor
     */
    basic_exception_handler_logger(basic_exception_handler_logger const& that) :
        base_type(static_cast< base_type const& >(that)),
        m_ExceptionHandler(that.m_ExceptionHandler)
    {
    }
    /*!
     * Move constructor
     */
    basic_exception_handler_logger(BOOST_RV_REF(basic_exception_handler_logger) that) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_constructible< base_type >::value && boost::is_nothrow_move_constructible< exception_handler_type >::value) :
        base_type(boost::move(static_cast< base_type& >(that))),
        m_ExceptionHandler(boost::move(that.m_ExceptionHandler))
    {
    }
    /*!
     * Constructor with arguments. Passes arguments to other features.
     */
    template< typename ArgsT >
    explicit basic_exception_handler_logger(ArgsT const& args) :
        base_type(args)
    {
    }

    /*!
     * The method sets exception handler function. The function will be called with no arguments
     * in case if an exception occurs during either \c open_record or \c push_record method
     * execution. Since exception handler is called from a \c catch statement, the exception
     * can be rethrown in order to determine its type.
     *
     * By default no handler is installed, thus any exception is propagated as usual.
     *
     * \sa <tt>utility/exception_handler.hpp</tt>
     * \param handler Exception handling function
     *
     * \note The exception handler can be invoked in several threads concurrently.
     *
     * \note Thread interruptions are not affected by exception handlers.
     */
    template< typename HandlerT >
    void set_exception_handler(HandlerT const& handler)
    {
#ifndef BOOST_LOG_NO_THREADS
        boost::log::aux::exclusive_lock_guard< threading_model > lock(this->get_threading_model());
#endif
        m_ExceptionHandler = handler;
    }

protected:
    /*!
     * Unlocked \c open_record
     */
    template< typename ArgsT >
    record open_record_unlocked(ArgsT const& args)
    {
        try
        {
            return base_type::open_record_unlocked(args);
        }
#ifndef BOOST_LOG_NO_THREADS
        catch (thread_interrupted&)
        {
            throw;
        }
#endif
        catch (...)
        {
            handle_exception();
            return record();
        }
    }

    /*!
     * Unlocked \c push_record
     */
    void push_record_unlocked(BOOST_RV_REF(record) rec)
    {
        try
        {
            base_type::push_record_unlocked(boost::move(rec));
        }
#ifndef BOOST_LOG_NO_THREADS
        catch (thread_interrupted&)
        {
            throw;
        }
#endif
        catch (...)
        {
            handle_exception();
        }
    }

    /*!
     * Unlocked swap
     */
    void swap_unlocked(basic_exception_handler_logger& that)
    {
        base_type::swap_unlocked(static_cast< base_type& >(that));
        m_ExceptionHandler.swap(that.m_ExceptionHandler);
    }

private:
#if !defined(BOOST_LOG_DOXYGEN_PASS)
    //! The function handles the intercepted exception
    void handle_exception()
    {
#ifndef BOOST_LOG_NO_THREADS
        // Here's the trick with the lock type. Since the lock
        // is only needed when an exception is caught, we indicate
        // no locking requirements in the push_record_lock type.
        // However, if other features don't require locking either,
        // we shall acquire a read lock here, when an exception is caught.
        // If other features do require locking, the thread model is
        // already locked by now, and we don't do locking at all.
        typedef typename mpl::if_<
            is_same< no_lock< threading_model >, typename final_type::push_record_lock >,
            boost::log::aux::shared_lock_guard< threading_model >,
            no_lock< threading_model >
        >::type lock_type;
        lock_type lock(base_type::get_threading_model());
#endif // !defined(BOOST_LOG_NO_THREADS)

        if (m_ExceptionHandler.empty())
            throw;
        m_ExceptionHandler();
    }
#endif // !defined(BOOST_LOG_DOXYGEN_PASS)
};

/*!
 * \brief Exception handler support feature
 *
 * The logger with this feature will provide an additional method to
 * install an exception handler functional object. This functional
 * object will be called if during either opening or pushing a record
 * an exception is thrown from the logging core.
 */
struct exception_handler
{
    template< typename BaseT >
    struct apply
    {
        typedef basic_exception_handler_logger< BaseT > type;
    };
};

} // namespace sources

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SOURCES_EXCEPTION_HANDLER_FEATURE_HPP_INCLUDED_
