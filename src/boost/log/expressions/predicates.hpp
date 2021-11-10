/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   predicates.hpp
 * \author Andrey Semashev
 * \date   29.01.2012
 *
 * The header includes all template expression predicates.
 */

#ifndef BOOST_LOG_EXPRESSIONS_PREDICATES_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_PREDICATES_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#include <boost/log/expressions/predicates/has_attr.hpp>
#include <boost/log/expressions/predicates/begins_with.hpp>
#include <boost/log/expressions/predicates/ends_with.hpp>
#include <boost/log/expressions/predicates/contains.hpp>
#include <boost/log/expressions/predicates/matches.hpp>
#include <boost/log/expressions/predicates/is_in_range.hpp>

#include <boost/log/expressions/predicates/is_debugger_present.hpp>
#include <boost/log/expressions/predicates/channel_severity_filter.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#endif // BOOST_LOG_EXPRESSIONS_PREDICATES_HPP_INCLUDED_
