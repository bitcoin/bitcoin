/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   date_time_fmt_gen_traits_fwd.hpp
 * \author Andrey Semashev
 * \date   07.11.2012
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_DATE_TIME_FMT_GEN_TRAITS_FWD_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_DATE_TIME_FMT_GEN_TRAITS_FWD_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

namespace aux {

template< typename T, typename CharT, typename VoidT = void >
struct date_time_formatter_generator_traits;

} // namespace aux

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_DATE_TIME_FMT_GEN_TRAITS_FWD_HPP_INCLUDED_
