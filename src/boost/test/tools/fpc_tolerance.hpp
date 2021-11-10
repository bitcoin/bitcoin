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
//  Description : FPC tools tolerance holder
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_FPC_TOLERANCE_HPP_121612GER
#define BOOST_TEST_TOOLS_FPC_TOLERANCE_HPP_121612GER

// Boost Test
#include <boost/test/tree/decorator.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {

namespace fpc = math::fpc;

// ************************************************************************** //
// **************     floating point comparison tolerance      ************** //
// ************************************************************************** //

template<typename FPT>
inline FPT&
fpc_tolerance()
{
    static FPT s_value = 0;
    return s_value;
}

//____________________________________________________________________________//

template<typename FPT>
struct local_fpc_tolerance {
    local_fpc_tolerance( FPT fraction_tolerance ) : m_old_tolerance( fpc_tolerance<FPT>() )
    {
        fpc_tolerance<FPT>() = fraction_tolerance;
    }

    ~local_fpc_tolerance()
    {
        if( m_old_tolerance != (FPT)-1 )
            fpc_tolerance<FPT>() = m_old_tolerance;
    }

private:
    // Data members
    FPT         m_old_tolerance;
};

//____________________________________________________________________________//

} // namespace test_tools

// ************************************************************************** //
// **************             decorator::tolerance             ************** //
// ************************************************************************** //

namespace unit_test {
namespace decorator {

template<typename FPT>
inline fixture_t
tolerance( FPT v )
{
    return fixture_t( test_unit_fixture_ptr(
        new unit_test::class_based_fixture<test_tools::local_fpc_tolerance<FPT>,FPT>( v ) ) );
}

//____________________________________________________________________________//

template<typename FPT>
inline fixture_t
tolerance( test_tools::fpc::percent_tolerance_t<FPT> v )
{
    return fixture_t( test_unit_fixture_ptr(
        new unit_test::class_based_fixture<test_tools::local_fpc_tolerance<FPT>,FPT>( boost::math::fpc::fpc_detail::fraction_tolerance<FPT>( v ) ) ) );
}

//____________________________________________________________________________//

} // namespace decorator

using decorator::tolerance;

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_FPC_TOLERANCE_HPP_121612GER
