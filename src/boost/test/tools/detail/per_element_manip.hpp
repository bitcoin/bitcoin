//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//! @file
//! Per element comparison manipulator implementation
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_DETAIL_PER_ELEMENT_MANIP_HPP_050815GER
#define BOOST_TEST_TOOLS_DETAIL_PER_ELEMENT_MANIP_HPP_050815GER

// Boost Test
#include <boost/test/tools/detail/fwd.hpp>
#include <boost/test/tools/detail/indirections.hpp>

#include <boost/test/utils/lazy_ostream.hpp>
#include <boost/test/tools/assertion.hpp>
#include <boost/test/tools/collection_comparison_op.hpp>

#include <ostream>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {

// ************************************************************************** //
// **************      per element comparison manipulator      ************** //
// ************************************************************************** //

//! Per element comparison manipulator, for containers
//! This is a terminal that involves evaluation of the expression
struct per_element {};

//____________________________________________________________________________//

inline unit_test::lazy_ostream&
operator<<( unit_test::lazy_ostream &o, per_element )  { return o; }

// needed for the lazy evaluation in lazy_ostream as per_element is a terminal
inline std::ostream& 
operator<<( std::ostream& o, per_element )             { return o; }

//____________________________________________________________________________//

namespace tt_detail {

template<typename T1, typename T2, typename OP>
inline assertion_result
operator<<(assertion_evaluate_t<assertion::binary_expr<T1,T2,OP> > const& ae, per_element )
{
    typedef typename OP::elem_op elem_op;
    return assertion::op::element_compare<elem_op>( ae.m_e.lhs().value(), ae.m_e.rhs() );
}

//____________________________________________________________________________//

inline assertion_type
operator<<( assertion_type const&, per_element )
{
    return assertion_type(CHECK_BUILT_ASSERTION);
}

//____________________________________________________________________________//

} // namespace tt_detail
} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_DETAIL_PER_ELEMENT_MANIP_HPP_050815GER
