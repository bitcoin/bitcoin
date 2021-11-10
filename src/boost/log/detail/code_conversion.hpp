/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   code_conversion.hpp
 * \author Andrey Semashev
 * \date   08.11.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_CODE_CONVERSION_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_CODE_CONVERSION_HPP_INCLUDED_

#include <cstddef>
#include <locale>
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

// Implementation note: We have to implement char<->wchar_t conversions even in the absence of the native wchar_t
// type. These conversions are used in sinks, e.g. to convert multibyte strings to wide-character filesystem paths.

//! The function converts one string to the character type of another
BOOST_LOG_API bool code_convert_impl(const wchar_t* str1, std::size_t len, std::string& str2, std::size_t max_size, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API bool code_convert_impl(const char* str1, std::size_t len, std::wstring& str2, std::size_t max_size, std::locale const& loc = std::locale());

#if !defined(BOOST_LOG_NO_CXX11_CODECVT_FACETS)
#if !defined(BOOST_NO_CXX11_CHAR16_T)
//! The function converts one string to the character type of another
BOOST_LOG_API bool code_convert_impl(const char16_t* str1, std::size_t len, std::string& str2, std::size_t max_size, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API bool code_convert_impl(const char* str1, std::size_t len, std::u16string& str2, std::size_t max_size, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API bool code_convert_impl(const char16_t* str1, std::size_t len, std::wstring& str2, std::size_t max_size, std::locale const& loc = std::locale());
#endif
#if !defined(BOOST_NO_CXX11_CHAR32_T)
//! The function converts one string to the character type of another
BOOST_LOG_API bool code_convert_impl(const char32_t* str1, std::size_t len, std::string& str2, std::size_t max_size, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API bool code_convert_impl(const char* str1, std::size_t len, std::u32string& str2, std::size_t max_size, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API bool code_convert_impl(const char32_t* str1, std::size_t len, std::wstring& str2, std::size_t max_size, std::locale const& loc = std::locale());
#endif
#if !defined(BOOST_NO_CXX11_CHAR16_T) && !defined(BOOST_NO_CXX11_CHAR32_T)
//! The function converts one string to the character type of another
BOOST_LOG_API bool code_convert_impl(const char16_t* str1, std::size_t len, std::u32string& str2, std::size_t max_size, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API bool code_convert_impl(const char32_t* str1, std::size_t len, std::u16string& str2, std::size_t max_size, std::locale const& loc = std::locale());
#endif
#endif // !defined(BOOST_LOG_NO_CXX11_CODECVT_FACETS)

//! The function converts one string to the character type of another
template< typename SourceCharT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) == sizeof(TargetCharT), bool >::type
code_convert(const SourceCharT* str1, std::size_t len, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::size_t max_size, std::locale const& = std::locale())
{
    std::size_t size_left = str2.size() < max_size ? max_size - str2.size() : static_cast< std::size_t >(0u);
    const bool overflow = len > size_left;
    str2.append(reinterpret_cast< const TargetCharT* >(str1), overflow ? size_left : len);
    return !overflow;
}

//! The function converts one string to the character type of another
template< typename SourceCharT, typename SourceTraitsT, typename SourceAllocatorT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) == sizeof(TargetCharT), bool >::type
code_convert(std::basic_string< SourceCharT, SourceTraitsT, SourceAllocatorT > const& str1, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::size_t max_size, std::locale const& loc = std::locale())
{
    return aux::code_convert(str1.data(), str1.size(), str2, max_size, loc);
}

//! The function converts one string to the character type of another
template< typename SourceCharT, typename SourceTraitsT, typename SourceAllocatorT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) == sizeof(TargetCharT), bool >::type
code_convert(std::basic_string< SourceCharT, SourceTraitsT, SourceAllocatorT > const& str1, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::locale const& loc = std::locale())
{
    return aux::code_convert(str1.data(), str1.size(), str2, str2.max_size(), loc);
}
//! The function converts one string to the character type of another
template< typename SourceCharT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) == sizeof(TargetCharT), bool >::type
code_convert(const SourceCharT* str1, std::size_t len, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::locale const& loc = std::locale())
{
    return aux::code_convert(str1, len, str2, str2.max_size(), loc);
}

//! The function converts one string to the character type of another
template< typename SourceCharT, typename SourceTraitsT, typename SourceAllocatorT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) != sizeof(TargetCharT), bool >::type
code_convert(std::basic_string< SourceCharT, SourceTraitsT, SourceAllocatorT > const& str1, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::locale const& loc = std::locale())
{
    return aux::code_convert_impl(str1.c_str(), str1.size(), str2, str2.max_size(), loc);
}

//! The function converts one string to the character type of another
template< typename SourceCharT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) != sizeof(TargetCharT), bool >::type
code_convert(const SourceCharT* str1, std::size_t len, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::locale const& loc = std::locale())
{
    return aux::code_convert_impl(str1, len, str2, str2.max_size(), loc);
}

//! The function converts one string to the character type of another
template< typename SourceCharT, typename SourceTraitsT, typename SourceAllocatorT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) != sizeof(TargetCharT), bool >::type
code_convert(std::basic_string< SourceCharT, SourceTraitsT, SourceAllocatorT > const& str1, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::size_t max_size, std::locale const& loc = std::locale())
{
    return aux::code_convert_impl(str1.c_str(), str1.size(), str2, max_size, loc);
}

//! The function converts one string to the character type of another
template< typename SourceCharT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) != sizeof(TargetCharT), bool >::type
code_convert(const SourceCharT* str1, std::size_t len, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::size_t max_size, std::locale const& loc = std::locale())
{
    return aux::code_convert_impl(str1, len, str2, max_size, loc);
}

//! The function converts the passed string to the narrow-character encoding
inline std::string const& to_narrow(std::string const& str)
{
    return str;
}

//! The function converts the passed string to the narrow-character encoding
inline std::string const& to_narrow(std::string const& str, std::locale const&)
{
    return str;
}

//! The function converts the passed string to the narrow-character encoding
inline std::string to_narrow(std::wstring const& str, std::locale const& loc = std::locale())
{
    std::string res;
    aux::code_convert_impl(str.c_str(), str.size(), res, res.max_size(), loc);
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_HAS_NRVO)
    return static_cast< std::string&& >(res);
#else
    return res;
#endif
}

//! The function converts the passed string to the wide-character encoding
inline std::wstring const& to_wide(std::wstring const& str)
{
    return str;
}

//! The function converts the passed string to the wide-character encoding
inline std::wstring const& to_wide(std::wstring const& str, std::locale const&)
{
    return str;
}

//! The function converts the passed string to the wide-character encoding
inline std::wstring to_wide(std::string const& str, std::locale const& loc = std::locale())
{
    std::wstring res;
    aux::code_convert_impl(str.c_str(), str.size(), res, res.max_size(), loc);
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_HAS_NRVO)
    return static_cast< std::wstring&& >(res);
#else
    return res;
#endif
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_CODE_CONVERSION_HPP_INCLUDED_
