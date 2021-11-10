/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   thread_id.hpp
 * \author Andrey Semashev
 * \date   08.01.2012
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_THREAD_ID_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_THREAD_ID_HPP_INCLUDED_

#include <iosfwd>
#include <boost/cstdint.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/id.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! The thread id descriptor
struct thread
{
#if defined(BOOST_WINDOWS)
    typedef uint32_t native_type;
#else
    typedef uintmax_t native_type;
#endif
    typedef boost::log::aux::id< thread > id;
};

namespace this_thread {

//! The function returns current thread identifier
BOOST_LOG_API thread::id const& get_id();

} // namespace this_thread

template< typename CharT, typename TraitsT >
BOOST_LOG_API std::basic_ostream< CharT, TraitsT >&
operator<< (std::basic_ostream< CharT, TraitsT >& strm, thread::id const& tid);

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_THREAD_ID_HPP_INCLUDED_
