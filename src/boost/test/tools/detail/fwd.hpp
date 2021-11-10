//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 74248 $
//
//  Description : toolbox implementation types and forward declarations
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_DETAIL_FWD_HPP_012705GER
#define BOOST_TEST_TOOLS_DETAIL_FWD_HPP_012705GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>

// STL
#include <cstddef>          // for std::size_t

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

class lazy_ostream;

} // namespace unit_test

namespace test_tools {

using unit_test::const_string;
class assertion_result;

//____________________________________________________________________________//

namespace tt_detail {

inline bool dummy_cond() { return false; }

// ************************************************************************** //
// **************        types of supported assertions         ************** //
// ************************************************************************** //

//____________________________________________________________________________//

enum check_type {
    CHECK_PRED,
    CHECK_MSG,
    CHECK_EQUAL,
    CHECK_NE,
    CHECK_LT,
    CHECK_LE,
    CHECK_GT,
    CHECK_GE,
    CHECK_CLOSE,
    CHECK_CLOSE_FRACTION,
    CHECK_SMALL,
    CHECK_BITWISE_EQUAL,
    CHECK_PRED_WITH_ARGS,
    CHECK_EQUAL_COLL,
    CHECK_BUILT_ASSERTION
};

//____________________________________________________________________________//

// ************************************************************************** //
// **************        levels of supported assertions        ************** //
// ************************************************************************** //

enum tool_level {
    WARN, CHECK, REQUIRE, PASS
};

//____________________________________________________________________________//

// ************************************************************************** //
// **************         Tools offline implementation         ************** //
// ************************************************************************** //

BOOST_TEST_DECL bool
report_assertion( assertion_result const& pr, unit_test::lazy_ostream const& assertion_descr,
                  const_string file_name, std::size_t line_num,
                  tool_level tl, check_type ct,
                  std::size_t num_args, ... );

//____________________________________________________________________________//

BOOST_TEST_DECL assertion_result
format_assertion_result( const_string expr_val, const_string details );

//____________________________________________________________________________//

BOOST_TEST_DECL assertion_result
format_fpc_report( const_string expr_val, const_string details );

//____________________________________________________________________________//

BOOST_TEST_DECL bool
is_defined_impl( const_string symbol_name, const_string symbol_value );

//____________________________________________________________________________//

BOOST_TEST_DECL assertion_result
equal_impl( char const* left, char const* right );

//____________________________________________________________________________//

} // namespace tt_detail
} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_DETAIL_FWD_HPP_012705GER
