//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief shared definition for unit test log levels
// ***************************************************************************

#ifndef BOOST_TEST_LOG_LEVEL_HPP_011605GER
#define BOOST_TEST_LOG_LEVEL_HPP_011605GER

#include <boost/test/detail/config.hpp>

namespace boost {
namespace unit_test {

// ************************************************************************** //
// **************                   log levels                 ************** //
// ************************************************************************** //

//  each log level includes all subsequent higher loging levels
enum BOOST_TEST_ENUM_SYMBOL_VISIBLE log_level {
    invalid_log_level        = -1,
    log_successful_tests     = 0,
    log_test_units           = 1,
    log_messages             = 2,
    log_warnings             = 3,
    log_all_errors           = 4, // reported by unit test macros
    log_cpp_exception_errors = 5, // uncaught C++ exceptions
    log_system_errors        = 6, // including timeouts, signals, traps
    log_fatal_errors         = 7, // including unit test macros or
                                  // fatal system errors
    log_nothing              = 8
};

} // namespace unit_test
} // namespace boost

#endif // BOOST_TEST_LOG_LEVEL_HPP_011605GER
