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
//  Description : defines test_tree_visitor
// ***************************************************************************

#ifndef BOOST_TEST_TREE_VISITOR_HPP_100211GER
#define BOOST_TEST_TREE_VISITOR_HPP_100211GER

// Boost.Test
#include <boost/test/detail/config.hpp>

#include <boost/test/tree/test_unit.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

// ************************************************************************** //
// **************               test_tree_visitor              ************** //
// ************************************************************************** //

class BOOST_TEST_DECL test_tree_visitor {
public:
    // test tree visitor interface
    virtual bool    visit( test_unit const& )               { return true; }
    virtual void    visit( test_case const& tc )            { visit( (test_unit const&)tc ); }
    virtual bool    test_suite_start( test_suite const& ts ){ return visit( (test_unit const&)ts ); }
    virtual void    test_suite_finish( test_suite const& )  {}

protected:
    BOOST_TEST_PROTECTED_VIRTUAL ~test_tree_visitor() {}
};

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TREE_VISITOR_HPP_100211GER

