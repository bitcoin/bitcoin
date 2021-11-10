/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   adaptive_mutex.hpp
 * \author Andrey Semashev
 * \date   01.08.2010
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_ADAPTIVE_MUTEX_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_ADAPTIVE_MUTEX_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#ifndef BOOST_LOG_NO_THREADS

#include <boost/throw_exception.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/assert/source_location.hpp>

#if defined(BOOST_THREAD_POSIX) // This one can be defined by users, so it should go first
#define BOOST_LOG_ADAPTIVE_MUTEX_USE_PTHREAD
#elif defined(BOOST_WINDOWS)
#define BOOST_LOG_ADAPTIVE_MUTEX_USE_WINAPI
#elif defined(BOOST_HAS_PTHREADS)
#define BOOST_LOG_ADAPTIVE_MUTEX_USE_PTHREAD
#endif

#if defined(BOOST_LOG_ADAPTIVE_MUTEX_USE_WINAPI)

#include <boost/log/detail/pause.hpp>
#include <boost/winapi/thread.hpp>
#include <boost/detail/interlocked.hpp>

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#    if defined(__INTEL_COMPILER)
#        define BOOST_LOG_COMPILER_BARRIER __memory_barrier()
#    elif defined(__clang__) // clang-win also defines _MSC_VER
#        define BOOST_LOG_COMPILER_BARRIER __atomic_signal_fence(__ATOMIC_SEQ_CST)
#    else
extern "C" void _ReadWriteBarrier(void);
#        if defined(BOOST_MSVC)
#            pragma intrinsic(_ReadWriteBarrier)
#        endif
#        define BOOST_LOG_COMPILER_BARRIER _ReadWriteBarrier()
#    endif
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#    define BOOST_LOG_COMPILER_BARRIER __asm__ __volatile__("" : : : "memory")
#endif

#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! A mutex that performs spinning or thread yielding in case of contention
class adaptive_mutex
{
private:
    enum state
    {
        initial_pause = 2,
        max_pause = 16
    };

    long m_State;

public:
    adaptive_mutex() : m_State(0) {}

    bool try_lock()
    {
        return (BOOST_INTERLOCKED_COMPARE_EXCHANGE(&m_State, 1L, 0L) == 0L);
    }

    void lock()
    {
#if defined(BOOST_LOG_AUX_PAUSE)
        unsigned int pause_count = initial_pause;
#endif
        while (!try_lock())
        {
#if defined(BOOST_LOG_AUX_PAUSE)
            if (pause_count < max_pause)
            {
                for (unsigned int i = 0; i < pause_count; ++i)
                {
                    BOOST_LOG_AUX_PAUSE;
                }
                pause_count += pause_count;
            }
            else
            {
                // Restart spinning after waking up this thread
                pause_count = initial_pause;
                boost::winapi::SwitchToThread();
            }
#else
            boost::winapi::SwitchToThread();
#endif
        }
    }

    void unlock()
    {
#if (defined(_M_IX86) || defined(_M_AMD64)) && defined(BOOST_LOG_COMPILER_BARRIER)
        BOOST_LOG_COMPILER_BARRIER;
        m_State = 0L;
        BOOST_LOG_COMPILER_BARRIER;
#else
        BOOST_INTERLOCKED_EXCHANGE(&m_State, 0L);
#endif
    }

    //  Non-copyable
    BOOST_DELETED_FUNCTION(adaptive_mutex(adaptive_mutex const&))
    BOOST_DELETED_FUNCTION(adaptive_mutex& operator= (adaptive_mutex const&))
};

#undef BOOST_LOG_AUX_PAUSE
#undef BOOST_LOG_COMPILER_BARRIER

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#elif defined(BOOST_LOG_ADAPTIVE_MUTEX_USE_PTHREAD)

#include <pthread.h>
#include <boost/assert.hpp>
#include <boost/log/detail/header.hpp>

#if defined(PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP)
#define BOOST_LOG_ADAPTIVE_MUTEX_USE_PTHREAD_MUTEX_ADAPTIVE_NP
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! A mutex that performs spinning or thread yielding in case of contention
class adaptive_mutex
{
private:
    pthread_mutex_t m_State;

public:
    adaptive_mutex()
    {
#if defined(BOOST_LOG_ADAPTIVE_MUTEX_USE_PTHREAD_MUTEX_ADAPTIVE_NP)
        pthread_mutexattr_t attrs;
        pthread_mutexattr_init(&attrs);
        pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_ADAPTIVE_NP);

        const int err = pthread_mutex_init(&m_State, &attrs);
        pthread_mutexattr_destroy(&attrs);
#else
        const int err = pthread_mutex_init(&m_State, NULL);
#endif
        if (BOOST_UNLIKELY(err != 0))
            throw_exception< thread_resource_error >(err, "Failed to initialize an adaptive mutex", "adaptive_mutex::adaptive_mutex()", __FILE__, __LINE__);
    }

    ~adaptive_mutex()
    {
        BOOST_VERIFY(pthread_mutex_destroy(&m_State) == 0);
    }

    bool try_lock()
    {
        const int err = pthread_mutex_trylock(&m_State);
        if (err == 0)
            return true;
        if (BOOST_UNLIKELY(err != EBUSY))
            throw_exception< lock_error >(err, "Failed to lock an adaptive mutex", "adaptive_mutex::try_lock()", __FILE__, __LINE__);
        return false;
    }

    void lock()
    {
        const int err = pthread_mutex_lock(&m_State);
        if (BOOST_UNLIKELY(err != 0))
            throw_exception< lock_error >(err, "Failed to lock an adaptive mutex", "adaptive_mutex::lock()", __FILE__, __LINE__);
    }

    void unlock()
    {
        BOOST_VERIFY(pthread_mutex_unlock(&m_State) == 0);
    }

    //  Non-copyable
    BOOST_DELETED_FUNCTION(adaptive_mutex(adaptive_mutex const&))
    BOOST_DELETED_FUNCTION(adaptive_mutex& operator= (adaptive_mutex const&))

private:
    template< typename ExceptionT >
    static BOOST_NOINLINE BOOST_LOG_NORETURN void throw_exception(int err, const char* descr, const char* func, const char* file, int line)
    {
        boost::throw_exception(ExceptionT(err, descr), boost::source_location(file, line, func));
    }
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif

#endif // BOOST_LOG_NO_THREADS

#endif // BOOST_LOG_DETAIL_ADAPTIVE_MUTEX_HPP_INCLUDED_
