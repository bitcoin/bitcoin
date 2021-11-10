//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: -1 $
//
//  Description : defines traverse_test_tree algorithm
// ***************************************************************************

#ifndef BOOST_TEST_TREE_TRAVERSE_HPP_100211GER
#define BOOST_TEST_TREE_TRAVERSE_HPP_100211GER

// Boost.Test
#include <boost/test/detail/config.hpp>

#include <boost/test/tree/test_unit.hpp>
#include <boost/test/tree/visitor.hpp>

#include <boost/test/detail/suppress_warnings.hpp>


//____________________________________________________________________________//

namespace boost {
namespace unit_test {

// ************************************************************************** //
// **************               traverse_test_tree             ************** //
// ************************************************************************** //

BOOST_TEST_DECL void    traverse_test_tree( test_case const&, test_tree_visitor&, bool ignore_status = false );
BOOST_TEST_DECL void    traverse_test_tree( test_suite const&, test_tree_visitor&, bool ignore_status = false );
BOOST_TEST_DECL void    traverse_test_tree( test_unit_id     , test_tree_visitor&, bool ignore_status = false );

//____________________________________________________________________________//

inline void
traverse_test_tree( test_unit const& tu, test_tree_visitor& V, bool ignore_status = false )
{
    if( tu.p_type == TUT_CASE )
        traverse_test_tree( static_cast<test_case const&>( tu ), V, ignore_status );
    else
        traverse_test_tree( static_cast<test_suite const&>( tu ), V, ignore_status );
}

//____________________________________________________________________________//

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TREE_TRAVERSE_HPP_100211GER
