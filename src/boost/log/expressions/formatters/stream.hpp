/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   stream.hpp
 * \author Andrey Semashev
 * \date   24.07.2012
 *
 * The header contains implementation of a stream placeholder in template expressions.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FORMATTERS_STREAM_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FORMATTERS_STREAM_HPP_INCLUDED_

#include <boost/phoenix/core/argument.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

/*!
 * Stream placeholder type in formatter template expressions.
 */
typedef phoenix::expression::argument< 2 >::type stream_type;

/*!
 * Stream placeholder in formatter template expressions.
 */
BOOST_INLINE_VARIABLE const stream_type stream = {};

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>
#if defined(BOOST_LOG_EXPRESSIONS_ATTR_HPP_INCLUDED_)
#include <boost/log/detail/attr_output_impl.hpp>
#endif

#endif // BOOST_LOG_EXPRESSIONS_FORMATTERS_STREAM_HPP_INCLUDED_
