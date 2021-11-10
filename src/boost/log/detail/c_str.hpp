/*
 *             Copyright Andrey Semashev 2016.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   c_str.hpp
 * \author Andrey Semashev
 * \date   23.02.2016
 *
 * This header is the Boost.Log library implementation, see the library documentation
 * at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_C_STR_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_C_STR_HPP_INCLUDED_

#include <string>
#include <boost/core/enable_if.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/is_character_type.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

template< typename T >
inline typename boost::enable_if_c< is_character_type< T >::value, const T* >::type c_str(const T* str) BOOST_NOEXCEPT
{
    return str;
}

template< typename T, typename TraitsT, typename AllocatorT >
inline typename boost::enable_if_c< is_character_type< T >::value, const T* >::type c_str(std::basic_string< T, TraitsT, AllocatorT > const& str) BOOST_NOEXCEPT
{
    return str.c_str();
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_C_STR_HPP_INCLUDED_
