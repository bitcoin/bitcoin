/*
 *             Copyright Andrey Semashev 2019.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/target.hpp
 * \author Andrey Semashev
 * \date   05.01.2019
 *
 * The header contains the \c target_file_name keyword declaration.
 */

#ifndef BOOST_LOG_KEYWORDS_TARGET_FILE_NAME_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_TARGET_FILE_NAME_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace keywords {

//! The keyword allows to pass the target file name for file sink
BOOST_PARAMETER_KEYWORD(tag, target_file_name)

} // namespace keywords

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_TARGET_FILE_NAME_HPP_INCLUDED_
