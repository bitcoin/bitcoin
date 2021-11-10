/*
 *             Copyright Andrey Semashev 2016.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/overflow_policy.hpp
 * \author Andrey Semashev
 * \date   23.02.2016
 *
 * The header contains the \c overflow_policy keyword declaration.
 */

#ifndef BOOST_LOG_KEYWORDS_OVERFLOW_POLICY_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_OVERFLOW_POLICY_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace keywords {

//! The keyword allows to pass interprocess queue overflow policy to the queue constructor
BOOST_PARAMETER_KEYWORD(tag, overflow_policy)

} // namespace keywords

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_OVERFLOW_POLICY_HPP_INCLUDED_
