// (c) Copyright Raffi Enficiaud 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/test for the library home page.
//
//! @file
//! An observer for monitoring the success/failure of the other observers
// ***************************************************************************

#ifndef BOOST_TEST_FRAMEWORK_INIT_OBSERVER_IPP_021105GER
#define BOOST_TEST_FRAMEWORK_INIT_OBSERVER_IPP_021105GER

// Boost.Test
#include <boost/test/test_framework_init_observer.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {


//____________________________________________________________________________//

// ************************************************************************** //
// **************           framework_init_observer_t          ************** //
// ************************************************************************** //

void
framework_init_observer_t::clear()
{
    m_has_failure = false;
}

//____________________________________________________________________________//

void
framework_init_observer_t::test_start( counter_t, test_unit_id )
{
    clear();
}

//____________________________________________________________________________//

void
framework_init_observer_t::assertion_result( unit_test::assertion_result ar )
{
    switch( ar ) {
    case AR_FAILED: m_has_failure = true; break;
    default:
        break;
    }
}

//____________________________________________________________________________//

void
framework_init_observer_t::exception_caught( execution_exception const& )
{
    m_has_failure = true;
}

void
framework_init_observer_t::test_aborted()
{
    m_has_failure = true;
}


//____________________________________________________________________________//

bool
framework_init_observer_t::has_failed() const
{
    return m_has_failure;
}

//____________________________________________________________________________//

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_FRAMEWORK_INIT_OBSERVER_IPP_021105GER
