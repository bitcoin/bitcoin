//
// Copyright 2007-2008 Andreas Pokorny, Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_IO_PATH_SPEC_HPP
#define BOOST_GIL_IO_PATH_SPEC_HPP

#ifdef BOOST_GIL_IO_ADD_FS_PATH_SUPPORT
// Disable warning: conversion to 'std::atomic<int>::__integral_type {aka int}' from 'long int' may alter its value
#if defined(BOOST_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem/path.hpp>

#if defined(BOOST_CLANG)
#pragma clang diagnostic pop
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic pop
#endif
#endif // BOOST_GIL_IO_ADD_FS_PATH_SUPPORT

#include <cstdlib>
#include <string>
#include <type_traits>

namespace boost { namespace gil { namespace detail {

template<typename P> struct is_supported_path_spec              : std::false_type {};
template<> struct is_supported_path_spec< std::string >         : std::true_type {};
template<> struct is_supported_path_spec< const std::string >   : std::true_type {};
template<> struct is_supported_path_spec< std::wstring >        : std::true_type {};
template<> struct is_supported_path_spec< const std::wstring >  : std::true_type {};
template<> struct is_supported_path_spec< const char* >         : std::true_type {};
template<> struct is_supported_path_spec< char* >               : std::true_type {};
template<> struct is_supported_path_spec< const wchar_t* >      : std::true_type {};
template<> struct is_supported_path_spec< wchar_t* >            : std::true_type {};

template<int i> struct is_supported_path_spec<const char [i]>       : std::true_type {};
template<int i> struct is_supported_path_spec<char [i]>             : std::true_type {};
template<int i> struct is_supported_path_spec<const wchar_t [i]>    : std::true_type {};
template<int i> struct is_supported_path_spec<wchar_t [i]>          : std::true_type {};

#ifdef BOOST_GIL_IO_ADD_FS_PATH_SUPPORT
template<> struct is_supported_path_spec< filesystem::path > : std::true_type {};
template<> struct is_supported_path_spec< const filesystem::path > : std::true_type {};
#endif // BOOST_GIL_IO_ADD_FS_PATH_SUPPORT


///
/// convert_to_string
///

inline std::string convert_to_string( std::string const& obj)
{
   return obj;
}

inline std::string convert_to_string( std::wstring const& s )
{
    std::size_t len = wcslen( s.c_str() );
    char* c = reinterpret_cast<char*>( alloca( len ));
    wcstombs( c, s.c_str(), len );

    return std::string( c, c + len );
}

inline std::string convert_to_string( const char* str )
{
    return std::string( str );
}

inline std::string convert_to_string( char* str )
{
    return std::string( str );
}

#ifdef BOOST_GIL_IO_ADD_FS_PATH_SUPPORT
inline std::string convert_to_string( const filesystem::path& path )
{
    return convert_to_string( path.string() );
}
#endif // BOOST_GIL_IO_ADD_FS_PATH_SUPPORT

///
/// convert_to_native_string
///

inline const char* convert_to_native_string( char* str )
{
    return str;
}

inline const char* convert_to_native_string( const char* str )
{
    return str;
}

inline const char* convert_to_native_string( const std::string& str )
{
   return str.c_str();
}

inline const char* convert_to_native_string( const wchar_t* str )
{
    std::size_t len = wcslen( str ) + 1;
    char* c = new char[len];
    wcstombs( c, str, len );

    return c;
}

inline const char* convert_to_native_string( const std::wstring& str )
{
    std::size_t len = wcslen( str.c_str() ) + 1;
    char* c = new char[len];
    wcstombs( c, str.c_str(), len );

    return c;
}

} // namespace detail
} // namespace gil
} // namespace boost

#endif
