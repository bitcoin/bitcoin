/*
 *             Copyright Andrey Semashev 2019.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   sinks/auto_newline_mode.hpp
 * \author Andrey Semashev
 * \date   23.06.2019
 *
 * The header contains definition of auto-newline modes.
 */

#ifndef BOOST_LOG_SINKS_AUTO_NEWLINE_MODE_HPP_INCLUDED_HPP_
#define BOOST_LOG_SINKS_AUTO_NEWLINE_MODE_HPP_INCLUDED_HPP_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

//! The enum lists automatic trailing newline modes
enum auto_newline_mode
{
    disabled_auto_newline,  //!< Do not insert automatic trailing newline characters
    always_insert,          //!< Always insert automatic trailing newline characters
    insert_if_missing       //!< Insert automatic trailing newline characters, if not present already
};

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_AUTO_NEWLINE_MODE_HPP_INCLUDED_HPP_
