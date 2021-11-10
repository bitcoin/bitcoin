/*
 *              Copyright Andrey Semashev 2016.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   open_mode.hpp
 * \author Andrey Semashev
 * \date   01.01.2016
 *
 * The header defines resource opening modes.
 */

#ifndef BOOST_LOG_UTILITY_OPEN_MODE_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_OPEN_MODE_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace open_mode {

//! Create a new resource; fail if exists already
struct create_only_tag {} const create_only = create_only_tag();
//! Opens an existing resource; fail if not exist
struct open_only_tag {} const open_only = open_only_tag();
//! Creates a new resource or opens an existing one
struct open_or_create_tag {} const open_or_create = open_or_create_tag();

} // namespace open_mode

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_OPEN_MODE_HPP_INCLUDED_
