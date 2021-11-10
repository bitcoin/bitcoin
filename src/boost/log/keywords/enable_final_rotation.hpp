/*
 *             Copyright Andrey Semashev 2016.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/enable_final_rotation.hpp
 * \author Andrey Semashev
 * \date   27.11.2016
 *
 * The header contains the \c enable_final_rotation keyword declaration.
 */

#ifndef BOOST_LOG_KEYWORDS_ENABLE_FINAL_ROTATION_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_ENABLE_FINAL_ROTATION_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace keywords {

//! The keyword for enabling/disabling final log file rotation on sink backend destruction
BOOST_PARAMETER_KEYWORD(tag, enable_final_rotation)

} // namespace keywords

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_ENABLE_FINAL_ROTATION_HPP_INCLUDED_
