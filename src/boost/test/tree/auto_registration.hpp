//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 74640 $
//
//  Description : defines auto_test_unit_registrar
// ***************************************************************************

#ifndef BOOST_TEST_TREE_AUTO_REGISTRATION_HPP_100211GER
#define BOOST_TEST_TREE_AUTO_REGISTRATION_HPP_100211GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/tree/decorator.hpp>
#include <boost/test/tree/test_unit.hpp>

// STL
#include <list>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace ut_detail {

// ************************************************************************** //
// **************           auto_test_unit_registrar           ************** //
// ************************************************************************** //

struct BOOST_TEST_DECL auto_test_unit_registrar {
    // Constructors
                auto_test_unit_registrar( test_case* tc, decorator::collector_t& decorators, counter_t exp_fail = 0 );
    explicit    auto_test_unit_registrar( const_string ts_name, const_string ts_file, std::size_t ts_line, decorator::collector_t& decorators );
    explicit    auto_test_unit_registrar( test_unit_generator const& tc_gen, decorator::collector_t& decorators );
    explicit    auto_test_unit_registrar( boost::shared_ptr<test_unit_generator>  tc_gen, decorator::collector_t& decorators );
    explicit    auto_test_unit_registrar( int );
};

} // namespace ut_detail
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TREE_AUTO_REGISTRATION_HPP_100211GER

