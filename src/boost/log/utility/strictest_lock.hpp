/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   utility/strictest_lock.hpp
 * \author Andrey Semashev
 * \date   30.05.2010
 *
 * The header contains definition of the \c strictest_lock metafunction that
 * allows to select a lock with the strictest access requirements.
 */

#ifndef BOOST_LOG_UTILITY_STRICTEST_LOCK_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_STRICTEST_LOCK_HPP_INCLUDED_

#include <boost/mpl/integral_c.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/locks.hpp>
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/log/detail/pp_identity.hpp>
#endif
#if defined(BOOST_LOG_BROKEN_CONSTANT_EXPRESSIONS)
#include <boost/mpl/less.hpp>
#endif
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#if !defined(BOOST_LOG_STRICTEST_LOCK_LIMIT)
/*!
 * The macro defines the maximum number of template arguments that the \c strictest_lock
 * metafunction accepts. Should not be less than 2.
 */
#define BOOST_LOG_STRICTEST_LOCK_LIMIT 10
#endif // BOOST_LOG_STRICTEST_LOCK_LIMIT
#if BOOST_LOG_STRICTEST_LOCK_LIMIT < 2
#error The BOOST_LOG_STRICTEST_LOCK_LIMIT macro should not be less than 2
#endif
#endif // defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

//! Access modes for different types of locks
enum lock_access_mode
{
    unlocked_access,    //!< A thread that owns this kind of lock doesn't restrict other threads in any way
    shared_access,      //!< A thread that owns this kind of lock requires that no other thread modify the locked data
    exclusive_access    //!< A thread that owns this kind of lock requires that no other thread has access to the locked data
};

//! The trait allows to select an access mode by the lock type
template< typename LockT >
struct thread_access_mode_of;

template< typename MutexT >
struct thread_access_mode_of< no_lock< MutexT > > : mpl::integral_c< lock_access_mode, unlocked_access >
{
};

#if !defined(BOOST_LOG_NO_THREADS)

template< typename MutexT >
struct thread_access_mode_of< lock_guard< MutexT > > : mpl::integral_c< lock_access_mode, exclusive_access >
{
};

template< typename MutexT >
struct thread_access_mode_of< shared_lock_guard< MutexT > > : mpl::integral_c< lock_access_mode, shared_access >
{
};

template< typename MutexT >
struct thread_access_mode_of< unique_lock< MutexT > > : mpl::integral_c< lock_access_mode, exclusive_access >
{
};

template< typename MutexT >
struct thread_access_mode_of< shared_lock< MutexT > > : mpl::integral_c< lock_access_mode, shared_access >
{
};

template< typename MutexT >
struct thread_access_mode_of< upgrade_lock< MutexT > > : mpl::integral_c< lock_access_mode, shared_access >
{
};

template< typename MutexT >
struct thread_access_mode_of< boost::log::aux::exclusive_lock_guard< MutexT > > : mpl::integral_c< lock_access_mode, exclusive_access >
{
};

template< typename MutexT >
struct thread_access_mode_of< boost::log::aux::shared_lock_guard< MutexT > > : mpl::integral_c< lock_access_mode, shared_access >
{
};

template< typename MutexT1, typename MutexT2 >
struct thread_access_mode_of< boost::log::aux::multiple_unique_lock2< MutexT1, MutexT2 > > : mpl::integral_c< lock_access_mode, exclusive_access >
{
};

#endif // !defined(BOOST_LOG_NO_THREADS)

namespace aux {

//! The metafunction selects the most strict lock type of the two
template<
    typename LeftLockT,
    typename RightLockT,
#if !defined(BOOST_LOG_BROKEN_CONSTANT_EXPRESSIONS)
    bool CondV = (thread_access_mode_of< LeftLockT >::value < thread_access_mode_of< RightLockT >::value)
#else
    bool CondV = mpl::less< thread_access_mode_of< LeftLockT >, thread_access_mode_of< RightLockT > >::value
#endif
>
struct strictest_lock_impl
{
    typedef RightLockT type;
};
template< typename LeftLockT, typename RightLockT >
struct strictest_lock_impl< LeftLockT, RightLockT, false >
{
    typedef LeftLockT type;
};

} // namespace aux

#if defined(BOOST_LOG_DOXYGEN_PASS)

/*!
 * \brief The metafunction selects the most strict lock type of the specified.
 *
 * The template supports all lock types provided by the Boost.Thread
 * library (except for \c upgrade_to_unique_lock), plus additional
 * pseudo-lock \c no_lock that indicates no locking at all.
 * Exclusive locks are considered the strictest, shared locks are weaker,
 * and \c no_lock is the weakest.
 */
template< typename... LocksT >
struct strictest_lock
{
    typedef implementation_defined type;
};

#else // defined(BOOST_LOG_DOXYGEN_PASS)

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

template< typename LockT, typename... LocksT >
struct strictest_lock;

template< typename LockT >
struct strictest_lock< LockT >
{
    typedef LockT type;
};

template< typename LeftLockT, typename RightLockT >
struct strictest_lock< LeftLockT, RightLockT >
{
    typedef typename aux::strictest_lock_impl< LeftLockT, RightLockT >::type type;
};

template< typename LeftLockT, typename RightLockT, typename... LocksT >
struct strictest_lock< LeftLockT, RightLockT, LocksT... >
{
    typedef typename strictest_lock<
        typename aux::strictest_lock_impl< LeftLockT, RightLockT >::type,
        LocksT...
    >::type type;
};

#else // !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#   define BOOST_LOG_TYPE_INTERNAL(z, i, data) BOOST_PP_CAT(T, BOOST_PP_INC(i))

template<
    typename T,
    BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_PP_DEC(BOOST_LOG_STRICTEST_LOCK_LIMIT), typename T, void)
>
struct strictest_lock
{
    typedef typename strictest_lock<
        typename boost::log::aux::strictest_lock_impl< T, T0 >::type
        BOOST_PP_ENUM_TRAILING(BOOST_PP_SUB(BOOST_LOG_STRICTEST_LOCK_LIMIT, 2), BOOST_LOG_TYPE_INTERNAL, ~)
    >::type type;
};

template< typename T >
struct strictest_lock<
    T
    BOOST_PP_ENUM_TRAILING(BOOST_PP_DEC(BOOST_LOG_STRICTEST_LOCK_LIMIT), BOOST_LOG_PP_IDENTITY, void)
>
{
    typedef T type;
};

#   undef BOOST_LOG_TYPE_INTERNAL

#endif // !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#endif // defined(BOOST_LOG_DOXYGEN_PASS)

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_STRICTEST_LOCK_HPP_INCLUDED_
