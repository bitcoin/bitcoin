//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : plain report formatter definition
// ***************************************************************************

#ifndef BOOST_TEST_PLAIN_REPORT_FORMATTER_IPP_020105GER
#define BOOST_TEST_PLAIN_REPORT_FORMATTER_IPP_020105GER

// Boost.Test
#include <boost/test/output/plain_report_formatter.hpp>
#include <boost/test/utils/custom_manip.hpp>
#include <boost/test/results_collector.hpp>
#include <boost/test/unit_test_parameters.hpp>

#include <boost/test/tree/test_unit.hpp>

#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/utils/setcolor.hpp>

// STL
#include <iomanip>
#include <boost/config/no_tr1/cmath.hpp>
#include <iostream>

#include <boost/test/detail/suppress_warnings.hpp>

# ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::log10; }
# endif

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace output {

namespace {

typedef utils::custom_manip<struct quote_t> quote;

template<typename T>
inline std::ostream&
operator<<( utils::custom_printer<quote> const& p, T const& value )
{
    *p << '"' << value << '"';

    return *p;
}

//____________________________________________________________________________//

void
print_stat_value( std::ostream& ostr, counter_t v, counter_t indent, counter_t total, const_string name, const_string res )
{
    if( v == 0 )
        return;

    if( total > 0 )
        ostr << std::setw( static_cast<int>(indent) ) << "" << v << ' ' << name << ( v != 1 ? "s" : "" )
             << " out of " << total << ' ' << res << '\n';
    else
        ostr << std::setw( static_cast<int>(indent) ) << "" << v << ' ' << res << ' ' << name << ( v != 1 ? "s" : "" ) << '\n';
}

//____________________________________________________________________________//

} // local namespace

// ************************************************************************** //
// **************             plain_report_formatter           ************** //
// ************************************************************************** //

void
plain_report_formatter::results_report_start( std::ostream& ostr )
{
    m_indent = 0;
    m_color_output = runtime_config::get<bool>( runtime_config::btrt_color_output );
    ostr << '\n';
}

//____________________________________________________________________________//

void
plain_report_formatter::results_report_finish( std::ostream& ostr )
{
    ostr.flush();
}

//____________________________________________________________________________//

void
plain_report_formatter::test_unit_report_start( test_unit const& tu, std::ostream& ostr )
{
    test_results const& tr = results_collector.results( tu.p_id );

    const_string descr;

    if( tr.passed() )
        descr = "has passed";
    else if( tr.p_skipped )
        descr = "was skipped";
    else if( tr.p_timed_out )
        descr = "has timed out";
    else if( tr.p_aborted )
        descr = "was aborted";
    else
        descr = "has failed";

    ostr << std::setw( static_cast<int>(m_indent) ) << ""
         << "Test " << tu.p_type_name << ' ' << quote() << tu.full_name() << ' ' << descr;

    if( tr.p_skipped ) {
        ostr  << "\n";
        m_indent += 2;
        return;
    }

    // aborted test case within failed ones, timed-out TC exclusive with failed/aborted
    counter_t total_assertions  = tr.p_assertions_passed + tr.p_assertions_failed;
    counter_t total_tc          = tr.p_test_cases_passed + tr.p_test_cases_warned + tr.p_test_cases_failed + tr.p_test_cases_skipped + tr.p_test_cases_timed_out;

    if( total_assertions > 0 || total_tc > 0 || tr.p_warnings_failed > 0)
        ostr << " with:";

    ostr << '\n';
    m_indent += 2;

    print_stat_value( ostr, tr.p_test_cases_passed , m_indent, total_tc        , "test case", "passed" );
    print_stat_value( ostr, tr.p_test_cases_warned , m_indent, total_tc        , "test case", "passed with warnings" );
    print_stat_value( ostr, tr.p_test_cases_failed , m_indent, total_tc        , "test case", "failed" );
    print_stat_value( ostr, tr.p_test_cases_timed_out, m_indent, total_tc      , "test case", "timed-out" );
    print_stat_value( ostr, tr.p_test_suites_timed_out, m_indent, tr.p_test_suites, "test suite", "timed-out" );
    print_stat_value( ostr, tr.p_test_cases_skipped, m_indent, total_tc        , "test case", "skipped" );
    print_stat_value( ostr, tr.p_test_cases_aborted, m_indent, total_tc        , "test case", "aborted" );
    print_stat_value( ostr, tr.p_assertions_passed , m_indent, total_assertions, "assertion", "passed" );
    print_stat_value( ostr, tr.p_assertions_failed , m_indent, total_assertions, "assertion", "failed" );
    print_stat_value( ostr, tr.p_warnings_failed   , m_indent, 0               , "warning"  , "failed" );
    print_stat_value( ostr, tr.p_expected_failures , m_indent, 0               , "failure"  , "expected" );

    ostr << '\n';
}

//____________________________________________________________________________//

void
plain_report_formatter::test_unit_report_finish( test_unit const&, std::ostream& )
{
    m_indent -= 2;
}

//____________________________________________________________________________//

void
plain_report_formatter::do_confirmation_report( test_unit const& tu, std::ostream& ostr )
{
    test_results const& tr = results_collector.results( tu.p_id );

    if( tr.passed() ) {
        BOOST_TEST_SCOPE_SETCOLOR( m_color_output, ostr, term_attr::BRIGHT, term_color::GREEN );

        ostr << "*** No errors detected\n";
        return;
    }

    BOOST_TEST_SCOPE_SETCOLOR( m_color_output, ostr, term_attr::BRIGHT, term_color::RED );

    if( tr.p_skipped ) {
        ostr << "*** The test " << tu.p_type_name << ' ' << quote() << tu.full_name() << " was skipped"
             << "; see standard output for details\n";
        return;
    }

    if( tr.p_timed_out ) {
        ostr << "*** The test " << tu.p_type_name << ' ' << quote() << tu.full_name() << " has timed out"
             << "; see standard output for details\n";
        return;
    }

    if( tr.p_aborted ) {
        ostr << "*** The test " << tu.p_type_name << ' ' << quote() << tu.full_name() << " was aborted"
             << "; see standard output for details\n";
    }

    if( tr.p_assertions_failed == 0 ) {
        if( !tr.p_aborted )
            ostr << "*** Errors were detected in the test " << tu.p_type_name << ' ' << quote() << tu.full_name()
                 << "; see standard output for details\n";
        return;
    }

    counter_t num_failures = tr.p_assertions_failed;

    ostr << "*** " << num_failures << " failure" << ( num_failures != 1 ? "s are" : " is" ) << " detected";

    if( tr.p_expected_failures > 0 )
        ostr << " (" << tr.p_expected_failures << " failure" << ( tr.p_expected_failures != 1 ? "s are" : " is" ) << " expected)";

    ostr << " in the test " << tu.p_type_name << " " << quote() << tu.full_name() << "\n";
}

//____________________________________________________________________________//

} // namespace output
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_PLAIN_REPORT_FORMATTER_IPP_020105GER
