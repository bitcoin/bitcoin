/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/max_files.hpp
 * \author Erich Keane
 * \date   29.12.2015
 *
 * The header contains the \c max_files keyword declaration.
 */

#ifndef BOOST_LOG_KEYWORDS_MAX_FILES_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_MAX_FILES_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace keywords {

//! The keyword allows to specify maximum total number of log files
BOOST_PARAMETER_KEYWORD(tag, max_files)

} // namespace keywords

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_MAX_FILES_HPP_INCLUDED_
