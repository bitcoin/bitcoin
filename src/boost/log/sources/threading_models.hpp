/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   sources/threading_models.hpp
 * \author Andrey Semashev
 * \date   04.10.2008
 *
 * The header contains definition of threading models that can be used in loggers.
 * The header also provides a number of tags that can be used to express lock requirements
 * on a function callee.
 */

#ifndef BOOST_LOG_SOURCES_THREADING_MODELS_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_THREADING_MODELS_HPP_INCLUDED_

#include <boost/type_traits/has_nothrow_constructor.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/locks.hpp> // is_mutex_type
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/mpl/bool.hpp>
#endif
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sources {

//! Single thread locking model
struct single_thread_model
{
    // We provide methods for the most advanced locking concept: UpgradeLockable
    void lock_shared() const BOOST_NOEXCEPT {}
    bool try_lock_shared() const BOOST_NOEXCEPT { return true; }
    template< typename TimeT >
    bool timed_lock_shared(TimeT const&) const BOOST_NOEXCEPT { return true; }
    void unlock_shared() const BOOST_NOEXCEPT {}
    void lock() const BOOST_NOEXCEPT {}
    bool try_lock() const BOOST_NOEXCEPT { return true; }
    template< typename TimeT >
    bool timed_lock(TimeT const&) const BOOST_NOEXCEPT { return true; }
    void unlock() const BOOST_NOEXCEPT {}
    void lock_upgrade() const BOOST_NOEXCEPT {}
    bool try_lock_upgrade() const BOOST_NOEXCEPT { return true; }
    template< typename TimeT >
    bool timed_lock_upgrade(TimeT const&) const BOOST_NOEXCEPT { return true; }
    void unlock_upgrade() const BOOST_NOEXCEPT {}
    void unlock_upgrade_and_lock() const BOOST_NOEXCEPT {}
    void unlock_and_lock_upgrade() const BOOST_NOEXCEPT {}
    void unlock_and_lock_shared() const BOOST_NOEXCEPT {}
    void unlock_upgrade_and_lock_shared() const BOOST_NOEXCEPT {}

    void swap(single_thread_model&) BOOST_NOEXCEPT {}
};

inline void swap(single_thread_model&, single_thread_model&) BOOST_NOEXCEPT
{
}

#if !defined(BOOST_LOG_NO_THREADS)

//! Multi-thread locking model with maximum locking capabilities
template< typename MutexT >
struct multi_thread_model
{
    multi_thread_model() BOOST_NOEXCEPT_IF(boost::has_nothrow_constructor< MutexT >::value) {}
    multi_thread_model(multi_thread_model const&) BOOST_NOEXCEPT_IF(boost::has_nothrow_constructor< MutexT >::value) {}
    multi_thread_model& operator= (multi_thread_model const&) BOOST_NOEXCEPT { return *this; }
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    multi_thread_model(multi_thread_model&&) BOOST_NOEXCEPT_IF(boost::has_nothrow_constructor< MutexT >::value) {}
    multi_thread_model& operator= (multi_thread_model&&) BOOST_NOEXCEPT { return *this; }
#endif

    void lock_shared() const { m_Mutex.lock_shared(); }
    bool try_lock_shared() const { return m_Mutex.try_lock_shared(); }
    template< typename TimeT >
    bool timed_lock_shared(TimeT const& t) const { return m_Mutex.timed_lock_shared(t); }
    void unlock_shared() const BOOST_NOEXCEPT { m_Mutex.unlock_shared(); }
    void lock() const { m_Mutex.lock(); }
    bool try_lock() const { return m_Mutex.try_lock(); }
    template< typename TimeT >
    bool timed_lock(TimeT const& t) const { return m_Mutex.timed_lock(t); }
    void unlock() const BOOST_NOEXCEPT { m_Mutex.unlock(); }
    void lock_upgrade() const { m_Mutex.lock_upgrade(); }
    bool try_lock_upgrade() const { return m_Mutex.try_lock_upgrade(); }
    template< typename TimeT >
    bool timed_lock_upgrade(TimeT const& t) const { return m_Mutex.timed_lock_upgrade(t); }
    void unlock_upgrade() const BOOST_NOEXCEPT { m_Mutex.unlock_upgrade(); }
    void unlock_upgrade_and_lock() const { m_Mutex.unlock_upgrade_and_lock(); }
    void unlock_and_lock_upgrade() const { m_Mutex.unlock_and_lock_upgrade(); }
    void unlock_and_lock_shared() const { m_Mutex.unlock_and_lock_shared(); }
    void unlock_upgrade_and_lock_shared() const { m_Mutex.unlock_upgrade_and_lock_shared(); }

    void swap(multi_thread_model&) BOOST_NOEXCEPT {}

private:
    //! Synchronization primitive
    mutable MutexT m_Mutex;
};

template< typename MutexT >
inline void swap(multi_thread_model< MutexT >&, multi_thread_model< MutexT >&) BOOST_NOEXCEPT
{
}

#endif // !defined(BOOST_LOG_NO_THREADS)

} // namespace sources

BOOST_LOG_CLOSE_NAMESPACE // namespace log

#if !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_LOG_DOXYGEN_PASS)

template< >
struct is_mutex_type< boost::log::sources::single_thread_model > : mpl::true_
{
};

template< typename T >
struct is_mutex_type< boost::log::sources::multi_thread_model< T > > : mpl::true_
{
};

#endif // !defined(BOOST_LOG_NO_THREADS)

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SOURCES_THREADING_MODELS_HPP_INCLUDED_
