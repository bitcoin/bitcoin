/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ASSERT_HPP)
#define BOOST_SPIRIT_ASSERT_HPP

///////////////////////////////////////////////////////////////////////////////
//
//  BOOST_SPIRIT_ASSERT is used throughout the framework.  It can be
//  overridden by the user. If BOOST_SPIRIT_ASSERT_EXCEPTION is defined,
//  then that will be thrown, otherwise, BOOST_SPIRIT_ASSERT simply turns
//  into a plain BOOST_ASSERT()
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_ASSERT)
#if defined(NDEBUG)
    #define BOOST_SPIRIT_ASSERT(x)
#elif defined (BOOST_SPIRIT_ASSERT_EXCEPTION)
    #include <boost/throw_exception.hpp>
    #define BOOST_SPIRIT_ASSERT_AUX(f, l, x) BOOST_SPIRIT_ASSERT_AUX2(f, l, x)
    #define BOOST_SPIRIT_ASSERT_AUX2(f, l, x)                                   \
    ( (x) ? (void)0 : boost::throw_exception(                                   \
        BOOST_SPIRIT_ASSERT_EXCEPTION(f "(" #l "): " #x)) )
    #define BOOST_SPIRIT_ASSERT(x) BOOST_SPIRIT_ASSERT_AUX(__FILE__, __LINE__, x)
#else
    #include <boost/assert.hpp>
    #define BOOST_SPIRIT_ASSERT(x) BOOST_ASSERT(x)
#endif
#endif // !defined(BOOST_SPIRIT_ASSERT)

#endif // BOOST_SPIRIT_ASSERT_HPP
