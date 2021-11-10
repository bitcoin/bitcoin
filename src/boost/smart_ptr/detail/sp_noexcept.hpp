#ifndef BOOST_SMART_PTR_DETAIL_SP_NOEXCEPT_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_NOEXCEPT_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  detail/sp_noexcept.hpp
//
//  Copyright 2016, 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>

// BOOST_SP_NOEXCEPT

#if defined( BOOST_MSVC ) && BOOST_MSVC >= 1700 && BOOST_MSVC < 1900

#  define BOOST_SP_NOEXCEPT BOOST_NOEXCEPT_OR_NOTHROW

#else

#  define BOOST_SP_NOEXCEPT BOOST_NOEXCEPT

#endif

// BOOST_SP_NOEXCEPT_WITH_ASSERT

#if defined(BOOST_DISABLE_ASSERTS) || ( defined(BOOST_ENABLE_ASSERT_DEBUG_HANDLER) && defined(NDEBUG) )

#  define BOOST_SP_NOEXCEPT_WITH_ASSERT BOOST_SP_NOEXCEPT

#elif defined(BOOST_ENABLE_ASSERT_HANDLER) || ( defined(BOOST_ENABLE_ASSERT_DEBUG_HANDLER) && !defined(NDEBUG) )

#  define BOOST_SP_NOEXCEPT_WITH_ASSERT

#else

#  define BOOST_SP_NOEXCEPT_WITH_ASSERT BOOST_SP_NOEXCEPT

#endif

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_NOEXCEPT_HPP_INCLUDED
