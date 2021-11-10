/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   embedded_string_type.hpp
 * \author Andrey Semashev
 * \date   16.08.2009
 *
 * This header is the Boost.Log library implementation, see the library documentation
 * at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_EMBEDDED_STRING_TYPE_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_EMBEDDED_STRING_TYPE_HPP_INCLUDED_

#include <string>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

template< typename T, typename ArgT >
struct make_embedded_string_type_impl
{
    typedef ArgT type;
};

template< typename ArgT >
struct make_embedded_string_type_impl< char, ArgT >
{
    typedef std::basic_string< char > type;
};

template< typename ArgT >
struct make_embedded_string_type_impl< const char, ArgT >
{
    typedef std::basic_string< char > type;
};

template< typename ArgT >
struct make_embedded_string_type_impl< wchar_t, ArgT >
{
    typedef std::basic_string< wchar_t > type;
};

template< typename ArgT >
struct make_embedded_string_type_impl< const wchar_t, ArgT >
{
    typedef std::basic_string< wchar_t > type;
};

#if !defined(BOOST_NO_CXX11_CHAR16_T)
template< typename ArgT >
struct make_embedded_string_type_impl< char16_t, ArgT >
{
    typedef std::basic_string< char16_t > type;
};

template< typename ArgT >
struct make_embedded_string_type_impl< const char16_t, ArgT >
{
    typedef std::basic_string< char16_t > type;
};
#endif

#if !defined(BOOST_NO_CXX11_CHAR32_T)
template< typename ArgT >
struct make_embedded_string_type_impl< char32_t, ArgT >
{
    typedef std::basic_string< char32_t > type;
};

template< typename ArgT >
struct make_embedded_string_type_impl< const char32_t, ArgT >
{
    typedef std::basic_string< char32_t > type;
};
#endif

//! An auxiliary type translator to store strings by value in function objects and attribute values
template< typename ArgT >
struct make_embedded_string_type :
    public remove_cv< ArgT >
{
};

template< typename ArgT >
struct make_embedded_string_type< ArgT* > :
    public make_embedded_string_type_impl< ArgT, ArgT* >
{
};

template< typename ArgT, unsigned int CountV >
struct make_embedded_string_type< ArgT[CountV] > :
    public make_embedded_string_type_impl< ArgT, ArgT[CountV] >
{
};

template< typename ArgT, unsigned int CountV >
struct make_embedded_string_type< ArgT(&)[CountV] > :
    public make_embedded_string_type_impl< ArgT, ArgT(&)[CountV] >
{
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_EMBEDDED_STRING_TYPE_HPP_INCLUDED_
