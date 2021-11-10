//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_DETAIL_CONFIG_HPP
#define BOOST_BEAST_CORE_DETAIL_CONFIG_HPP

// Available to every header
#include <boost/config.hpp>
#include <boost/version.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/static_assert.hpp>

namespace boost {
namespace asio
{
} // asio
namespace beast {
namespace net = boost::asio;
} // beast
} // boost

/*
    _MSC_VER and _MSC_FULL_VER by version:

    14.0 (2015)             1900        190023026
    14.0 (2015 Update 1)    1900        190023506
    14.0 (2015 Update 2)    1900        190023918
    14.0 (2015 Update 3)    1900        190024210
*/

#if defined(BOOST_MSVC)
# if BOOST_MSVC_FULL_VER < 190024210
#  error Beast requires C++11: Visual Studio 2015 Update 3 or later needed
# endif

#elif defined(BOOST_GCC)
# if(BOOST_GCC < 40801)
#  error Beast requires C++11: gcc version 4.8 or later needed
# endif

#else
# if \
    defined(BOOST_NO_CXX11_DECLTYPE) || \
    defined(BOOST_NO_CXX11_HDR_TUPLE) || \
    defined(BOOST_NO_CXX11_TEMPLATE_ALIASES) || \
    defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#  error Beast requires C++11: a conforming compiler is needed
# endif

#endif

#define BOOST_BEAST_DEPRECATION_STRING \
    "This is a deprecated interface, #define BOOST_BEAST_ALLOW_DEPRECATED to allow it"

#ifndef BOOST_BEAST_ASSUME
# ifdef BOOST_GCC
#  define BOOST_BEAST_ASSUME(cond) \
    do { if (!(cond)) __builtin_unreachable(); } while (0)
# else
#  define BOOST_BEAST_ASSUME(cond) do { } while(0)
# endif
#endif

// Default to a header-only implementation. The user must specifically
// request separate compilation by defining BOOST_BEAST_SEPARATE_COMPILATION
#ifndef BOOST_BEAST_HEADER_ONLY
# ifndef BOOST_BEAST_SEPARATE_COMPILATION
#   define BOOST_BEAST_HEADER_ONLY 1
# endif
#endif

#if BOOST_BEAST_DOXYGEN
# define BOOST_BEAST_DECL
#elif defined(BOOST_BEAST_HEADER_ONLY)
# define BOOST_BEAST_DECL inline
#else
# define BOOST_BEAST_DECL
#endif

#ifndef BOOST_BEAST_ASYNC_RESULT1
#define BOOST_BEAST_ASYNC_RESULT1(type) \
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(type, void(::boost::beast::error_code))
#endif

#ifndef BOOST_BEAST_ASYNC_RESULT2
#define BOOST_BEAST_ASYNC_RESULT2(type) \
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(type, void(::boost::beast::error_code, ::std::size_t))
#endif

#ifndef BOOST_BEAST_ASYNC_TPARAM1
#define BOOST_BEAST_ASYNC_TPARAM1 BOOST_ASIO_COMPLETION_TOKEN_FOR(void(::boost::beast::error_code))
#endif

#ifndef BOOST_BEAST_ASYNC_TPARAM2
#define BOOST_BEAST_ASYNC_TPARAM2 BOOST_ASIO_COMPLETION_TOKEN_FOR(void(::boost::beast::error_code, ::std::size_t))
#endif

#endif
