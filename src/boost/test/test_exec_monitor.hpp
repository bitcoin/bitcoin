//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// @brief Deprecated implementation of Test Execution Monitor
///
/// To convert to Unit Test Framework simply rewrite:
/// @code
/// #include <boost/test/test_exec_monitor.hpp>
///
/// int test_main( int, char *[] )
/// {
///   ...
/// }
/// @endcode
/// as
/// @code
/// #include <boost/test/unit_test.hpp>
///
/// BOOST_AUTO_TEST_CASE(test_main)
/// {
///   ...
/// }
/// @endcode
/// and link with boost_unit_test_framework library *instead of* boost_test_exec_monitor
// ***************************************************************************

#ifndef BOOST_TEST_EXEC_MONITOR_HPP_071894GER
#define BOOST_TEST_EXEC_MONITOR_HPP_071894GER

// Boost.Test
#include <boost/test/test_tools.hpp>

//____________________________________________________________________________//

// ************************************************************************** //
// **************                 Auto Linking                 ************** //
// ************************************************************************** //

// Automatically link to the correct build variant where possible.
#if !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_TEST_NO_LIB) && \
    !defined(BOOST_TEST_SOURCE) && !defined(BOOST_TEST_INCLUDED)

#  define BOOST_LIB_NAME boost_test_exec_monitor
#  include <boost/config/auto_link.hpp>

#endif  // auto-linking disabled

#endif // BOOST_TEST_EXEC_MONITOR_HPP_071894GER
