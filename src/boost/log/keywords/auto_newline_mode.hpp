/*
 *             Copyright Andrey Semashev 2019.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/auto_newline_mode.hpp
 * \author Andrey Semashev
 * \date   23.06.2019
 *
 * The header contains the \c auto_newline_mode keyword declaration.
 */

#ifndef BOOST_LOG_KEYWORDS_AUTO_NEWLINE_MODE_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_AUTO_NEWLINE_MODE_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace keywords {

//! The keyword for passing automatic trailing newline insertion mode to a sink backend initialization
BOOST_PARAMETER_KEYWORD(tag, auto_newline_mode)

} // namespace keywords

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_AUTO_NEWLINE_MODE_HPP_INCLUDED_
