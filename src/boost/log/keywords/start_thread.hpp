/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/start_thread.hpp
 * \author Andrey Semashev
 * \date   14.07.2009
 *
 * The header contains the \c start_thread keyword declaration.
 */

#ifndef BOOST_LOG_KEYWORDS_START_THREAD_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_START_THREAD_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace keywords {

//! The keyword allows enable/disable spawning a dedicated thread in the asynchronous sink frontend
BOOST_PARAMETER_KEYWORD(tag, start_thread)

} // namespace keywords

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_START_THREAD_HPP_INCLUDED_
