/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   asio_fwd.hpp
 * \author Andrey Semashev
 * \date   20.04.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 *
 * The header provides forward declarations of Boost.ASIO that are required for the user's
 * code to compile with Boost.Log. The forward declarations allow to avoid including the major
 * part of Boost.ASIO and system headers into user's code.
 */

#ifndef BOOST_LOG_DETAIL_ASIO_FWD_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_ASIO_FWD_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

namespace asio {

namespace ip {

class address;

} // namespace ip

} // namespace asio

} // namespace boost

#endif // BOOST_LOG_DETAIL_ASIO_FWD_HPP_INCLUDED_
