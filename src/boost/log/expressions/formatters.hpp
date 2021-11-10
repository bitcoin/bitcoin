/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatters.hpp
 * \author Andrey Semashev
 * \date   10.11.2012
 *
 * The header includes all template expression formatters.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FORMATTERS_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FORMATTERS_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/expressions/formatters/format.hpp>

#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>

#include <boost/log/expressions/formatters/char_decorator.hpp>
#include <boost/log/expressions/formatters/xml_decorator.hpp>
#include <boost/log/expressions/formatters/csv_decorator.hpp>
#include <boost/log/expressions/formatters/c_decorator.hpp>
#include <boost/log/expressions/formatters/max_size_decorator.hpp>

#include <boost/log/expressions/formatters/if.hpp>
#include <boost/log/expressions/formatters/wrap_formatter.hpp>
#include <boost/log/expressions/formatters/auto_newline.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#endif // BOOST_LOG_EXPRESSIONS_FORMATTERS_HPP_INCLUDED_
