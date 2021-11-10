//  boost/filesystem/v3/config.hpp  ----------------------------------------------------//

//  Copyright Beman Dawes 2003
//  Copyright Andrey Semashev 2021

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

//--------------------------------------------------------------------------------------//

#ifndef BOOST_FILESYSTEM_CONFIG_HPP
#define BOOST_FILESYSTEM_CONFIG_HPP

// This header implements separate compilation features as described in
// http://www.boost.org/more/separate_compilation.html

#include <boost/config.hpp>
#include <boost/system/api_config.hpp> // for BOOST_POSIX_API or BOOST_WINDOWS_API
#include <boost/detail/workaround.hpp>

#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION != 3 && BOOST_FILESYSTEM_VERSION != 4
#error Compiling Boost.Filesystem file with BOOST_FILESYSTEM_VERSION defined != 3 or 4
#endif

#if defined(BOOST_FILESYSTEM_SOURCE)
#undef BOOST_FILESYSTEM_VERSION
#define BOOST_FILESYSTEM_VERSION 4
#elif !defined(BOOST_FILESYSTEM_VERSION)
#define BOOST_FILESYSTEM_VERSION 3
#endif

#define BOOST_FILESYSTEM_VERSIONED_SYM(sym) BOOST_JOIN(sym, BOOST_JOIN(_v, BOOST_FILESYSTEM_VERSION))

#if BOOST_FILESYSTEM_VERSION == 4
#undef BOOST_FILESYSTEM_DEPRECATED
#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED)
#define BOOST_FILESYSTEM_NO_DEPRECATED
#endif
#endif

#define BOOST_FILESYSTEM_I18N // aid users wishing to compile several versions

//  BOOST_FILESYSTEM_DEPRECATED needed for source compiles -----------------------------//

#ifdef BOOST_FILESYSTEM_SOURCE
#define BOOST_FILESYSTEM_DEPRECATED
#undef BOOST_FILESYSTEM_NO_DEPRECATED // fixes #9454, src bld fails if NO_DEP defined
#endif

#if defined(BOOST_FILESYSTEM_DEPRECATED) && defined(BOOST_FILESYSTEM_NO_DEPRECATED)
#error Both BOOST_FILESYSTEM_DEPRECATED and BOOST_FILESYSTEM_NO_DEPRECATED are defined
#endif

//  throw an exception  ----------------------------------------------------------------//
//
//  Exceptions were originally thrown via boost::throw_exception().
//  As throw_exception() became more complex, it caused user error reporting
//  to be harder to interpret, since the exception reported became much more complex.
//  The immediate fix was to throw directly, wrapped in a macro to make any later change
//  easier.

#define BOOST_FILESYSTEM_THROW(EX) throw EX

#if defined(BOOST_NO_STD_WSTRING)
#error Configuration not supported: Boost.Filesystem V3 and later requires std::wstring support
#endif

//  This header implements separate compilation features as described in
//  http://www.boost.org/more/separate_compilation.html

//  normalize macros  ------------------------------------------------------------------//

#if !defined(BOOST_FILESYSTEM_DYN_LINK) && !defined(BOOST_FILESYSTEM_STATIC_LINK) && !defined(BOOST_ALL_DYN_LINK) && !defined(BOOST_ALL_STATIC_LINK)
#define BOOST_FILESYSTEM_STATIC_LINK
#endif

#if defined(BOOST_ALL_DYN_LINK) && !defined(BOOST_FILESYSTEM_DYN_LINK)
#define BOOST_FILESYSTEM_DYN_LINK
#elif defined(BOOST_ALL_STATIC_LINK) && !defined(BOOST_FILESYSTEM_STATIC_LINK)
#define BOOST_FILESYSTEM_STATIC_LINK
#endif

#if defined(BOOST_FILESYSTEM_DYN_LINK) && defined(BOOST_FILESYSTEM_STATIC_LINK)
#error Must not define both BOOST_FILESYSTEM_DYN_LINK and BOOST_FILESYSTEM_STATIC_LINK
#endif

#if defined(BOOST_ALL_NO_LIB) && !defined(BOOST_FILESYSTEM_NO_LIB)
#define BOOST_FILESYSTEM_NO_LIB
#endif

//  enable dynamic linking  ------------------------------------------------------------//

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_FILESYSTEM_DYN_LINK)
#if defined(BOOST_FILESYSTEM_SOURCE)
#define BOOST_FILESYSTEM_DECL BOOST_SYMBOL_EXPORT
#else
#define BOOST_FILESYSTEM_DECL BOOST_SYMBOL_IMPORT
#endif
#else
#define BOOST_FILESYSTEM_DECL
#endif

//  enable automatic library variant selection  ----------------------------------------//

#if !defined(BOOST_FILESYSTEM_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_FILESYSTEM_NO_LIB)
//
// Set the name of our library, this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#define BOOST_LIB_NAME boost_filesystem
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_FILESYSTEM_DYN_LINK)
#define BOOST_DYN_LINK
#endif
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif // auto-linking disabled

#endif // BOOST_FILESYSTEM_CONFIG_HPP
