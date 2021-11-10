//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines @ref test_case_counter
// ***************************************************************************

#ifndef BOOST_TEST_TREE_TEST_CASE_COUNTER_HPP_100211GER
#define BOOST_TEST_TREE_TEST_CASE_COUNTER_HPP_100211GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/utils/class_properties.hpp>

#include <boost/test/tree/test_unit.hpp>
#include <boost/test/tree/visitor.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

// ************************************************************************** //
// **************                test_case_counter             ************** //
// ************************************************************************** //

///! Counts the number of enabled test cases
class test_case_counter : public test_tree_visitor {
public:
    // Constructor
    // @param ignore_disabled ignore the status when counting
    test_case_counter(bool ignore_status = false)
    : p_count( 0 )
    , m_ignore_status(ignore_status)
    {}

    BOOST_READONLY_PROPERTY( counter_t, (test_case_counter)) p_count;
private:
    // test tree visitor interface
    void    visit( test_case const& tc ) BOOST_OVERRIDE                { if( m_ignore_status || tc.is_enabled() ) ++p_count.value; }
    bool    test_suite_start( test_suite const& ts ) BOOST_OVERRIDE    { return m_ignore_status || ts.is_enabled(); }
  
    bool m_ignore_status;
};

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TREE_TEST_CASE_COUNTER_HPP_100211GER

