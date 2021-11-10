/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   locks.hpp
 * \author Andrey Semashev
 * \date   30.05.2010
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_LOCKS_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_LOCKS_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

#ifndef BOOST_LOG_NO_THREADS

// Forward declaration of Boost.Thread locks. Specified here to avoid including Boost.Thread,
// which would bring in many dependent headers, including a great deal of Boost.DateTime.
template< typename >
class lock_guard;
template< typename >
class shared_lock_guard;
template< typename >
class shared_lock;
template< typename >
class upgrade_lock;
template< typename >
class unique_lock;

template< typename >
struct is_mutex_type;

#endif // BOOST_LOG_NO_THREADS

BOOST_LOG_OPEN_NAMESPACE

//! An auxiliary pseudo-lock to express no locking requirements in logger features
template< typename MutexT >
class no_lock
{
public:
    /*!
     * Constructs the pseudo-lock. The mutex is not affected during the construction.
     */
    explicit no_lock(MutexT&) BOOST_NOEXCEPT {}

private:
    no_lock(no_lock const&);
    no_lock& operator= (no_lock const&);
};

namespace aux {

#ifndef BOOST_LOG_NO_THREADS

//! A trait to detect if the mutex supports exclusive locking
template< typename MutexT >
struct is_exclusively_lockable
{
    typedef char true_type;
    struct false_type { char t[2]; };

    template< typename T >
    static true_type check_lockable(T*, void (T::*)() = &T::lock, void (T::*)() = &T::unlock);
    static false_type check_lockable(void*);

    enum value_t { value = sizeof(check_lockable((MutexT*)NULL)) == sizeof(true_type) };
};

//! A trait to detect if the mutex supports shared locking
template< typename MutexT >
struct is_shared_lockable
{
    typedef char true_type;
    struct false_type { char t[2]; };

    template< typename T >
    static true_type check_shared_lockable(T*, void (T::*)() = &T::lock_shared, void (T::*)() = &T::unlock_shared);
    static false_type check_shared_lockable(void*);

    enum value_t { value = sizeof(check_shared_lockable((MutexT*)NULL)) == sizeof(true_type) };
};

//! A scope guard that automatically unlocks the mutex on destruction
template< typename MutexT >
struct exclusive_auto_unlocker
{
    explicit exclusive_auto_unlocker(MutexT& m) BOOST_NOEXCEPT : m_Mutex(m)
    {
    }
    ~exclusive_auto_unlocker()
    {
        m_Mutex.unlock();
    }

    BOOST_DELETED_FUNCTION(exclusive_auto_unlocker(exclusive_auto_unlocker const&))
    BOOST_DELETED_FUNCTION(exclusive_auto_unlocker& operator= (exclusive_auto_unlocker const&))

protected:
    MutexT& m_Mutex;
};

//! An analogue to the minimalistic \c lock_guard template. Defined here to avoid including Boost.Thread.
template< typename MutexT >
struct exclusive_lock_guard
{
    explicit exclusive_lock_guard(MutexT& m) BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(m.lock())) : m_Mutex(m)
    {
        m.lock();
    }
    ~exclusive_lock_guard()
    {
        m_Mutex.unlock();
    }

    BOOST_DELETED_FUNCTION(exclusive_lock_guard(exclusive_lock_guard const&))
    BOOST_DELETED_FUNCTION(exclusive_lock_guard& operator= (exclusive_lock_guard const&))

private:
    MutexT& m_Mutex;
};

//! An analogue to the minimalistic \c lock_guard template that locks \c shared_mutex with shared ownership.
template< typename MutexT >
struct shared_lock_guard
{
    explicit shared_lock_guard(MutexT& m) BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(m.lock_shared())) : m_Mutex(m)
    {
        m.lock_shared();
    }
    ~shared_lock_guard()
    {
        m_Mutex.unlock_shared();
    }

    BOOST_DELETED_FUNCTION(shared_lock_guard(shared_lock_guard const&))
    BOOST_DELETED_FUNCTION(shared_lock_guard& operator= (shared_lock_guard const&))

private:
    MutexT& m_Mutex;
};

//! A deadlock-safe lock type that exclusively locks two mutexes
template< typename MutexT1, typename MutexT2 >
class multiple_unique_lock2
{
public:
    multiple_unique_lock2(MutexT1& m1, MutexT2& m2) BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(m1.lock()) && BOOST_NOEXCEPT_EXPR(m2.lock())) :
        m_p1(&m1),
        m_p2(&m2)
    {
        // Yes, it's not conforming, but it works
        // and it doesn't require to #include <functional>
        if (static_cast< void* >(m_p1) < static_cast< void* >(m_p2))
        {
            m_p1->lock();
            m_p2->lock();
        }
        else
        {
            m_p2->lock();
            m_p1->lock();
        }
    }
    ~multiple_unique_lock2()
    {
        m_p2->unlock();
        m_p1->unlock();
    }

private:
    MutexT1* m_p1;
    MutexT2* m_p2;
};

#endif // BOOST_LOG_NO_THREADS

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_LOCKS_HPP_INCLUDED_
