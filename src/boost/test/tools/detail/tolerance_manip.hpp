//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//! @file
//! @brief Floating point comparison tolerance manipulators
//! 
//! This file defines several manipulators for floating point comparison. These
//! manipulators are intended to be used with BOOST_TEST.
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_DETAIL_TOLERANCE_MANIP_HPP_012705GER
#define BOOST_TEST_TOOLS_DETAIL_TOLERANCE_MANIP_HPP_012705GER

// Boost Test
#include <boost/test/tools/detail/fwd.hpp>
#include <boost/test/tools/detail/indirections.hpp>

#include <boost/test/utils/lazy_ostream.hpp>
#include <boost/test/tools/fpc_tolerance.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

#include <ostream>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {
namespace tt_detail {

// ************************************************************************** //
// **************           fpc tolerance manipulator          ************** //
// ************************************************************************** //

//! Tolerance manipulator, not to be used directly
//! This is not a terminal of the expression
template<typename FPT>
struct tolerance_manip {
    explicit tolerance_manip( FPT const & tol ) : m_value( tol ) {}

    FPT m_value;
};

//____________________________________________________________________________//

struct tolerance_manip_delay {};

template<typename FPT>
inline tolerance_manip<FPT>
operator%( FPT v, tolerance_manip_delay const& )
{
    BOOST_STATIC_ASSERT_MSG( (fpc::tolerance_based<FPT>::value), 
                             "tolerance should be specified using a floating points type" );

    return tolerance_manip<FPT>( FPT(v / 100) );
}

template <typename FPT>
struct tolerance_evaluation_context: assertion_evaluation_context {
    tolerance_evaluation_context(FPT tol)
    : assertion_evaluation_context( true ) // has report
    , m_tolerance_context(tol)
    {}

    local_fpc_tolerance<FPT> m_tolerance_context;
};

//____________________________________________________________________________//

template<typename E, typename FPT>
inline assertion_evaluate_t<E>
operator<<(assertion_evaluate_t<E> const& ae, tolerance_manip<FPT> const& tol)
{
    return ae.stack_context(
      typename assertion_evaluate_t<E>::context_holder(
        new tolerance_evaluation_context<FPT>( tol.m_value ))
    );
}

//____________________________________________________________________________//

template<typename FPT>
unit_test::lazy_ostream &
operator<<( unit_test::lazy_ostream &o, tolerance_manip<FPT> const& )   { return o; }

// needed for the lazy evaluation in lazy_ostream as for commutativity with other arguments
template<typename FPT>
std::ostream& 
operator<<( std::ostream& o, tolerance_manip<FPT> const& )              { return o; }


//____________________________________________________________________________//

template<typename FPT>
inline assertion_type
operator<<( assertion_type const& /*at*/, tolerance_manip<FPT> const& ) {
    return assertion_type(CHECK_BUILT_ASSERTION); 
}

//____________________________________________________________________________//

} // namespace tt_detail


/*! Tolerance manipulator
 *
 * These functions return a manipulator that can be used in conjunction with BOOST_TEST
 * in order to specify the tolerance with which floating point comparisons are made.
 */
template<typename FPT>
inline tt_detail::tolerance_manip<FPT>
tolerance( FPT v )
{
    BOOST_STATIC_ASSERT_MSG( (fpc::tolerance_based<FPT>::value), 
                             "tolerance only for floating points" );

    return tt_detail::tolerance_manip<FPT>( v );
}

//____________________________________________________________________________//

//! @overload tolerance( FPT v )
template<typename FPT>
inline tt_detail::tolerance_manip<FPT>
tolerance( fpc::percent_tolerance_t<FPT> v )
{
    BOOST_STATIC_ASSERT_MSG( (fpc::tolerance_based<FPT>::value), 
                             "tolerance only for floating points" );

    return tt_detail::tolerance_manip<FPT>( static_cast<FPT>(v.m_value / 100) );
}

//____________________________________________________________________________//

//! @overload tolerance( FPT v )
inline tt_detail::tolerance_manip_delay
tolerance()
{
    return tt_detail::tolerance_manip_delay();
}

//____________________________________________________________________________//

} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_DETAIL_TOLERANCE_MANIP_HPP_012705GER
