/*
 *             Copyright Andrey Semashev 2019.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   expressions/formatters/auto_newline.hpp
 * \author Andrey Semashev
 * \date   23.06.2019
 *
 * The header contains implementation of formatter for inserting a newline, unless there is already one inserted.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FORMATTERS_AUTO_NEWLINE_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FORMATTERS_AUTO_NEWLINE_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/utility/manipulators/auto_newline.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

/*!
 * Formatter for inserting a newline character, unless the last character
 * inserted into the stream is already a newline.
 */
using boost::log::auto_newline;

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_EXPRESSIONS_FORMATTERS_AUTO_NEWLINE_HPP_INCLUDED_
