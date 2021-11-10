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
//  Description : implements compiler like Log formatter
// ***************************************************************************

#ifndef BOOST_TEST_COMPILER_LOG_FORMATTER_IPP_020105GER
#define BOOST_TEST_COMPILER_LOG_FORMATTER_IPP_020105GER

// Boost.Test
#include <boost/test/output/compiler_log_formatter.hpp>

#include <boost/test/framework.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test_parameters.hpp>

#include <boost/test/tree/test_unit.hpp>

#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/utils/lazy_ostream.hpp>

// Boost
#include <boost/version.hpp>

// STL
#include <iostream>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace output {

// ************************************************************************** //
// **************            compiler_log_formatter            ************** //
// ************************************************************************** //

namespace {

std::string
test_phase_identifier()
{
    return framework::test_in_progress() ? framework::current_test_unit().full_name() : std::string( "Test setup" );
}

} // local namespace

//____________________________________________________________________________//

void
compiler_log_formatter::log_start( std::ostream& output, counter_t test_cases_amount )
{
    m_color_output = runtime_config::get<bool>( runtime_config::btrt_color_output );

    if( test_cases_amount > 0 )
        output  << "Running " << test_cases_amount << " test "
                << (test_cases_amount > 1 ? "cases" : "case") << "...\n";
}

//____________________________________________________________________________//

void
compiler_log_formatter::log_finish( std::ostream& ostr )
{
    ostr.flush();
}

//____________________________________________________________________________//

void
compiler_log_formatter::log_build_info( std::ostream& output, bool log_build_info )
{
    if(log_build_info) {
        output  << "Platform: " << BOOST_PLATFORM            << '\n'
                << "Compiler: " << BOOST_COMPILER            << '\n'
                << "STL     : " << BOOST_STDLIB              << '\n'
                << "Boost   : " << BOOST_VERSION/100000      << "."
                                << BOOST_VERSION/100 % 1000  << "."
                                << BOOST_VERSION % 100       << std::endl;
    }
}

//____________________________________________________________________________//

void
compiler_log_formatter::test_unit_start( std::ostream& output, test_unit const& tu )
{
    BOOST_TEST_SCOPE_SETCOLOR( m_color_output, output, term_attr::BRIGHT, term_color::BLUE );

    print_prefix( output, tu.p_file_name, tu.p_line_num );

    output << "Entering test " << tu.p_type_name << " \"" << tu.p_name << "\"" << std::endl;
}

//____________________________________________________________________________//

void
compiler_log_formatter::test_unit_finish( std::ostream& output, test_unit const& tu, unsigned long elapsed )
{
    BOOST_TEST_SCOPE_SETCOLOR( m_color_output, output, term_attr::BRIGHT, term_color::BLUE );

    print_prefix( output, tu.p_file_name, tu.p_line_num );

    output << "Leaving test " << tu.p_type_name << " \"" << tu.p_name << "\"";

    if( elapsed > 0 ) {
        output << "; testing time: ";
        if( elapsed % 1000 == 0 )
            output << elapsed/1000 << "ms";
        else
            output << elapsed << "us";
    }

    output << std::endl;
}

//____________________________________________________________________________//

void
compiler_log_formatter::test_unit_skipped( std::ostream& output, test_unit const& tu, const_string reason )
{
    BOOST_TEST_SCOPE_SETCOLOR( m_color_output, output, term_attr::BRIGHT, term_color::YELLOW );

    print_prefix( output, tu.p_file_name, tu.p_line_num );

    output  << "Test " << tu.p_type_name << " \"" << tu.full_name() << "\"" << " is skipped because " << reason << std::endl;
}

//____________________________________________________________________________//

void
compiler_log_formatter::log_exception_start( std::ostream& output, log_checkpoint_data const& checkpoint_data, execution_exception const& ex )
{
    execution_exception::location const& loc = ex.where();

    print_prefix( output, loc.m_file_name, loc.m_line_num );

    {
        BOOST_TEST_SCOPE_SETCOLOR( m_color_output, output, term_attr::UNDERLINE, term_color::RED );

        output << "fatal error: in \"" << (loc.m_function.is_empty() ? test_phase_identifier() : loc.m_function ) << "\": "
               << ex.what();
    }

    if( !checkpoint_data.m_file_name.is_empty() ) {
        output << '\n';
        print_prefix( output, checkpoint_data.m_file_name, checkpoint_data.m_line_num );

        BOOST_TEST_SCOPE_SETCOLOR( m_color_output, output, term_attr::BRIGHT, term_color::CYAN );

        output << "last checkpoint";
        if( !checkpoint_data.m_message.empty() )
            output << ": " << checkpoint_data.m_message;
    }
}

//____________________________________________________________________________//

void
compiler_log_formatter::log_exception_finish( std::ostream& output )
{
    output << std::endl;
}

//____________________________________________________________________________//

void
compiler_log_formatter::log_entry_start( std::ostream& output, log_entry_data const& entry_data, log_entry_types let )
{
    using namespace utils;

    switch( let ) {
        case BOOST_UTL_ET_INFO:
            print_prefix( output, entry_data.m_file_name, entry_data.m_line_num );
            output << setcolor( m_color_output, term_attr::BRIGHT, term_color::GREEN, term_color::ORIGINAL, &m_color_state);
            output << "info: ";
            break;
        case BOOST_UTL_ET_MESSAGE:
            output << setcolor( m_color_output, term_attr::BRIGHT, term_color::CYAN, term_color::ORIGINAL, &m_color_state);
            break;
        case BOOST_UTL_ET_WARNING:
            print_prefix( output, entry_data.m_file_name, entry_data.m_line_num );
            output << setcolor( m_color_output, term_attr::BRIGHT, term_color::YELLOW, term_color::ORIGINAL, &m_color_state);
            output << "warning: in \"" << test_phase_identifier() << "\": ";
            break;
        case BOOST_UTL_ET_ERROR:
            print_prefix( output, entry_data.m_file_name, entry_data.m_line_num );
            output << setcolor( m_color_output, term_attr::BRIGHT, term_color::RED, term_color::ORIGINAL, &m_color_state);
            output << "error: in \"" << test_phase_identifier() << "\": ";
            break;
        case BOOST_UTL_ET_FATAL_ERROR:
            print_prefix( output, entry_data.m_file_name, entry_data.m_line_num );
            output << setcolor( m_color_output, term_attr::UNDERLINE, term_color::RED, term_color::ORIGINAL, &m_color_state);
            output << "fatal error: in \"" << test_phase_identifier() << "\": ";
            break;
    }
}

//____________________________________________________________________________//

void
compiler_log_formatter::log_entry_value( std::ostream& output, const_string value )
{
    output << value;
}

//____________________________________________________________________________//

void
compiler_log_formatter::log_entry_value( std::ostream& output, lazy_ostream const& value )
{
    output << value;
}

//____________________________________________________________________________//

void
compiler_log_formatter::log_entry_finish( std::ostream& output )
{
    if( m_color_output )
        output << utils::setcolor(m_color_output, &m_color_state);

    output << std::endl;
}


//____________________________________________________________________________//

void
compiler_log_formatter::print_prefix( std::ostream& output, const_string file_name, std::size_t line_num )
{
    if( !file_name.empty() ) {
#ifdef __APPLE_CC__
        // Xcode-compatible logging format, idea by Richard Dingwall at
        // <http://richarddingwall.name/2008/06/01/using-the-boost-unit-test-framework-with-xcode-3/>.
        output << file_name << ':' << line_num << ": ";
#else
        output << file_name << '(' << line_num << "): ";
#endif
    }
}

//____________________________________________________________________________//

void
compiler_log_formatter::entry_context_start( std::ostream& output, log_level l )
{
    if( l == log_messages ) {
        output << "\n[context:";
    }
    else {
        output << (l == log_successful_tests ? "\nAssertion" : "\nFailure" ) << " occurred in a following context:";
    }
}

//____________________________________________________________________________//

void
compiler_log_formatter::entry_context_finish( std::ostream& output, log_level l )
{
    if( l == log_messages ) {
        output << "]";
    }
    output.flush();
}

//____________________________________________________________________________//

void
compiler_log_formatter::log_entry_context( std::ostream& output, log_level /*l*/, const_string context_descr )
{
    output << "\n    " << context_descr;
}

//____________________________________________________________________________//

} // namespace output
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_COMPILER_LOG_FORMATTER_IPP_020105GER
