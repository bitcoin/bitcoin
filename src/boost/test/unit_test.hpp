//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// @brief Entry point into the Unit Test Framework
///
/// This header should be the only header necessary to include to start using the framework
// ***************************************************************************

#ifndef BOOST_TEST_UNIT_TEST_HPP_071894GER
#define BOOST_TEST_UNIT_TEST_HPP_071894GER

// Boost.Test
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test_suite.hpp>

//____________________________________________________________________________//

// ************************************************************************** //
// **************                 Auto Linking                 ************** //
// ************************************************************************** //

#if !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_TEST_NO_LIB) && \
    !defined(BOOST_TEST_SOURCE) && !defined(BOOST_TEST_INCLUDED) && \
    defined(BOOST_TEST_MAIN)
#  define BOOST_LIB_NAME boost_unit_test_framework

#  if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_TEST_DYN_LINK)
#    define BOOST_DYN_LINK
#  endif

#  include <boost/config/auto_link.hpp>

#endif  // auto-linking disabled

// ************************************************************************** //
// **************                  unit_test_main              ************** //
// ************************************************************************** //

namespace boost { namespace unit_test {

int BOOST_TEST_DECL unit_test_main( init_unit_test_func init_func, int argc, char* argv[] );

}

// !! ?? to remove
namespace unit_test_framework=unit_test;

}

#if defined(BOOST_TEST_DYN_LINK) && defined(BOOST_TEST_MAIN) && !defined(BOOST_TEST_NO_MAIN)

// ************************************************************************** //
// **************        main function for tests using dll     ************** //
// ************************************************************************** //

int BOOST_TEST_CALL_DECL
main( int argc, char* argv[] )
{
    return ::boost::unit_test::unit_test_main( &init_unit_test, argc, argv );
}

//____________________________________________________________________________//

#endif // BOOST_TEST_DYN_LINK && BOOST_TEST_MAIN && !BOOST_TEST_NO_MAIN

#endif // BOOST_TEST_UNIT_TEST_HPP_071894GER
