/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   detail/event.hpp
 * \author Andrey Semashev
 * \date   24.07.2011
 */

#ifndef BOOST_LOG_DETAIL_EVENT_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_EVENT_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#ifndef BOOST_LOG_NO_THREADS

#if defined(BOOST_THREAD_PLATFORM_PTHREAD)
#   include <boost/atomic/capabilities.hpp>
#   if (defined(linux) || defined(__linux) || defined(__linux__)) && BOOST_ATOMIC_INT_LOCK_FREE == 2
#       include <boost/atomic/atomic.hpp>
#       define BOOST_LOG_EVENT_USE_FUTEX
#   elif defined(_POSIX_SEMAPHORES) && (_POSIX_SEMAPHORES + 0) > 0 && BOOST_ATOMIC_FLAG_LOCK_FREE == 2
#       include <semaphore.h>
#       include <boost/cstdint.hpp>
#       include <boost/atomic/atomic_flag.hpp>
#       define BOOST_LOG_EVENT_USE_POSIX_SEMAPHORE
#   endif
#elif defined(BOOST_THREAD_PLATFORM_WIN32)
#   include <boost/cstdint.hpp>
#   define BOOST_LOG_EVENT_USE_WINAPI
#endif

#if !defined(BOOST_LOG_EVENT_USE_POSIX_SEMAPHORE) && !defined(BOOST_LOG_EVENT_USE_WINAPI)
#   include <boost/thread/mutex.hpp>
#   include <boost/thread/condition_variable.hpp>
#   define BOOST_LOG_EVENT_USE_BOOST_CONDITION
#endif

#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

#if defined(BOOST_LOG_EVENT_USE_FUTEX)

class futex_based_event
{
private:
    boost::atomic< int > m_state;

public:
    //! Default constructor
    BOOST_LOG_API futex_based_event();
    //! Destructor
    BOOST_LOG_API ~futex_based_event();

    //! Waits for the object to become signalled
    BOOST_LOG_API void wait();
    //! Sets the object to a signalled state
    BOOST_LOG_API void set_signalled();

    //  Copying prohibited
    BOOST_DELETED_FUNCTION(futex_based_event(futex_based_event const&))
    BOOST_DELETED_FUNCTION(futex_based_event& operator= (futex_based_event const&))
};

typedef futex_based_event event;

#elif defined(BOOST_LOG_EVENT_USE_POSIX_SEMAPHORE)

class sem_based_event
{
private:
    boost::atomic_flag m_state;
    sem_t m_semaphore;

public:
    //! Default constructor
    BOOST_LOG_API sem_based_event();
    //! Destructor
    BOOST_LOG_API ~sem_based_event();

    //! Waits for the object to become signalled
    BOOST_LOG_API void wait();
    //! Sets the object to a signalled state
    BOOST_LOG_API void set_signalled();

    //  Copying prohibited
    BOOST_DELETED_FUNCTION(sem_based_event(sem_based_event const&))
    BOOST_DELETED_FUNCTION(sem_based_event& operator= (sem_based_event const&))
};

typedef sem_based_event event;

#elif defined(BOOST_LOG_EVENT_USE_WINAPI)

class winapi_based_event
{
private:
    boost::uint32_t m_state;
    void* m_event;

public:
    //! Default constructor
    BOOST_LOG_API winapi_based_event();
    //! Destructor
    BOOST_LOG_API ~winapi_based_event();

    //! Waits for the object to become signalled
    BOOST_LOG_API void wait();
    //! Sets the object to a signalled state
    BOOST_LOG_API void set_signalled();

    //  Copying prohibited
    BOOST_DELETED_FUNCTION(winapi_based_event(winapi_based_event const&))
    BOOST_DELETED_FUNCTION(winapi_based_event& operator= (winapi_based_event const&))
};

typedef winapi_based_event event;

#else

class generic_event
{
private:
    boost::mutex m_mutex;
    boost::condition_variable m_cond;
    bool m_state;

public:
    //! Default constructor
    BOOST_LOG_API generic_event();
    //! Destructor
    BOOST_LOG_API ~generic_event();

    //! Waits for the object to become signalled
    BOOST_LOG_API void wait();
    //! Sets the object to a signalled state
    BOOST_LOG_API void set_signalled();

    //  Copying prohibited
    BOOST_DELETED_FUNCTION(generic_event(generic_event const&))
    BOOST_DELETED_FUNCTION(generic_event& operator= (generic_event const&))
};

typedef generic_event event;

#endif

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_NO_THREADS

#endif // BOOST_LOG_DETAIL_EVENT_HPP_INCLUDED_
