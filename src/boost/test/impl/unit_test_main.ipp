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
//  Description : main function implementation for Unit Test Framework
// ***************************************************************************

#ifndef BOOST_TEST_UNIT_TEST_MAIN_IPP_012205GER
#define BOOST_TEST_UNIT_TEST_MAIN_IPP_012205GER

// Boost.Test
#include <boost/test/framework.hpp>
#include <boost/test/results_collector.hpp>
#include <boost/test/results_reporter.hpp>

#include <boost/test/tree/visitor.hpp>
#include <boost/test/tree/test_unit.hpp>
#include <boost/test/tree/traverse.hpp>

#include <boost/test/unit_test_parameters.hpp>

#include <boost/test/utils/foreach.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>

// Boost
#include <boost/core/ignore_unused.hpp>
#include <boost/cstdlib.hpp>

// STL
#include <cstdio>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <set>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

namespace ut_detail {

// ************************************************************************** //
// **************             hrf_content_reporter             ************** //
// ************************************************************************** //

struct hrf_content_reporter : test_tree_visitor {
    explicit        hrf_content_reporter( std::ostream& os ) : m_os( os ), m_indent( -4 ) {} // skip master test suite

private:
    void            report_test_unit( test_unit const& tu )
    {
        m_os << std::setw( m_indent ) << "" << tu.p_name;
        m_os << (tu.p_default_status == test_unit::RS_ENABLED ? "*" : " ");
        //m_os << '[' << tu.p_sibling_rank << ']';
        if( !tu.p_description->empty() )
            m_os << ": " << tu.p_description;

        m_os << "\n";
    }
    void    visit( test_case const& tc ) BOOST_OVERRIDE { report_test_unit( tc ); }
    bool    test_suite_start( test_suite const& ts ) BOOST_OVERRIDE
    {
        if( m_indent >= 0 )
            report_test_unit( ts );
        m_indent += 4;
        return true;
    }
    void    test_suite_finish( test_suite const& ) BOOST_OVERRIDE
    {
        m_indent -= 4;
    }

    // Data members
    std::ostream&   m_os;
    int             m_indent;
};

// ************************************************************************** //
// **************             dot_content_reporter             ************** //
// ************************************************************************** //

struct dot_content_reporter : test_tree_visitor {
    explicit        dot_content_reporter( std::ostream& os ) : m_os( os ) {}

private:
    void            report_test_unit( test_unit const& tu )
    {
        bool master_ts = tu.p_parent_id == INV_TEST_UNIT_ID;

        m_os << "tu" << tu.p_id;

        m_os << (master_ts ? "[shape=ellipse,peripheries=2" : "[shape=Mrecord" );

        m_os << ",fontname=Helvetica";

        m_os << (tu.p_default_status == test_unit::RS_ENABLED ? ",color=green" : ",color=yellow");

        if( master_ts )
            m_os << ",label=\"" << tu.p_name << "\"];\n";
        else {
            m_os << ",label=\"" << tu.p_name << "|" << tu.p_file_name << "(" << tu.p_line_num << ")";
            if( tu.p_timeout > 0  )
                m_os << "|timeout=" << tu.p_timeout;
            if( tu.p_expected_failures != 0  )
                m_os << "|expected failures=" << tu.p_expected_failures;
            if( !tu.p_labels->empty() ) {
                m_os << "|labels:";

                BOOST_TEST_FOREACH( std::string const&, l, tu.p_labels.get() )
                    m_os << " @" << l;
            }
            m_os << "\"];\n";
        }

        if( !master_ts )
            m_os << "tu" << tu.p_parent_id << " -> " << "tu" << tu.p_id << ";\n";

        BOOST_TEST_FOREACH( test_unit_id, dep_id, tu.p_dependencies.get() ) {
            test_unit const& dep = framework::get( dep_id, TUT_ANY );

            m_os << "tu" << tu.p_id << " -> " << "tu" << dep.p_id << "[color=red,style=dotted,constraint=false];\n";
        }

    }
    void    visit( test_case const& tc ) BOOST_OVERRIDE
    { 
        report_test_unit( tc );
    }
    bool    test_suite_start( test_suite const& ts ) BOOST_OVERRIDE
    {
        if( ts.p_parent_id == INV_TEST_UNIT_ID )
            m_os << "digraph G {rankdir=LR;\n";

        report_test_unit( ts );

        m_os << "{\n";

        return true;
    }
    void    test_suite_finish( test_suite const& ts ) BOOST_OVERRIDE
    {
        m_os << "}\n";
        if( ts.p_parent_id == INV_TEST_UNIT_ID )
            m_os << "}\n";
    }

    std::ostream&   m_os;
};

// ************************************************************************** //
// **************               labels_collector               ************** //
// ************************************************************************** //

struct labels_collector : test_tree_visitor {
    std::set<std::string> const& labels() const { return m_labels; }

private:
    bool            visit( test_unit const& tu ) BOOST_OVERRIDE
    {
        m_labels.insert( tu.p_labels->begin(), tu.p_labels->end() );
        return true;
    }

    // Data members
    std::set<std::string>   m_labels;
};

struct framework_shutdown_helper {
    ~framework_shutdown_helper() {
        try {
            framework::shutdown();
        }
        catch(...) {
            std::cerr << "Boost.Test shutdown exception caught" << std::endl;
        }
    }
};

} // namespace ut_detail

// ************************************************************************** //
// **************                  unit_test_main              ************** //
// ************************************************************************** //



int BOOST_TEST_DECL
unit_test_main( init_unit_test_func init_func, int argc, char* argv[] )
{
    int result_code = 0;

    ut_detail::framework_shutdown_helper shutdown_helper;
    boost::ignore_unused(shutdown_helper);

    BOOST_TEST_I_TRY {
        
        framework::init( init_func, argc, argv );

        if( runtime_config::get<bool>( runtime_config::btrt_wait_for_debugger ) ) {
            results_reporter::get_stream() << "Press any key to continue..." << std::endl;

            // getchar is defined as a macro in uClibc. Use parenthesis to fix
            // gcc bug 58952 for gcc <= 4.8.2.
            (std::getchar)();
            results_reporter::get_stream() << "Continuing..." << std::endl;
        }

        framework::finalize_setup_phase();

        output_format list_cont = runtime_config::get<output_format>( runtime_config::btrt_list_content );
        if( list_cont != unit_test::OF_INVALID ) {
            if( list_cont == unit_test::OF_DOT ) {
                ut_detail::dot_content_reporter reporter( results_reporter::get_stream() );

                traverse_test_tree( framework::master_test_suite().p_id, reporter, true );
            }
            else {
                ut_detail::hrf_content_reporter reporter( results_reporter::get_stream() );

                traverse_test_tree( framework::master_test_suite().p_id, reporter, true );
            }

            return boost::exit_success;
        }

        if( runtime_config::get<bool>( runtime_config::btrt_list_labels ) ) {
            ut_detail::labels_collector collector;

            traverse_test_tree( framework::master_test_suite().p_id, collector, true );

            results_reporter::get_stream() << "Available labels:\n  ";
            std::copy( collector.labels().begin(), collector.labels().end(), 
                       std::ostream_iterator<std::string>( results_reporter::get_stream(), "\n  " ) );
            results_reporter::get_stream() << "\n";

            return boost::exit_success;
        }

        framework::run();

        result_code = !runtime_config::get<bool>( runtime_config::btrt_result_code )
                        ? boost::exit_success
                        : results_collector.results( framework::master_test_suite().p_id ).result_code();
    }
    BOOST_TEST_I_CATCH( framework::nothing_to_test, ex ) {
        result_code = ex.m_result_code;
    }
    BOOST_TEST_I_CATCH( framework::internal_error, ex ) {
        results_reporter::get_stream() << "Boost.Test framework internal error: " << ex.what() << std::endl;

        result_code = boost::exit_exception_failure;
    }
    BOOST_TEST_I_CATCH( framework::setup_error, ex ) {
        results_reporter::get_stream() << "Test setup error: " << ex.what() << std::endl;

        result_code = boost::exit_exception_failure;
    }
    BOOST_TEST_I_CATCH( std::logic_error, ex ) {
        results_reporter::get_stream() << "Test setup error: " << ex.what() << std::endl;

        result_code = boost::exit_exception_failure;
    }
    BOOST_TEST_I_CATCHALL() {
        results_reporter::get_stream() << "Boost.Test framework internal error: unknown reason" << std::endl;

        result_code = boost::exit_exception_failure;
    }

    return result_code;
}

} // namespace unit_test
} // namespace boost

#if !defined(BOOST_TEST_DYN_LINK) && !defined(BOOST_TEST_NO_MAIN)

// ************************************************************************** //
// **************        main function for tests using lib     ************** //
// ************************************************************************** //

int BOOST_TEST_CALL_DECL
main( int argc, char* argv[] )
{
    // prototype for user's unit test init function
#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
    extern bool init_unit_test();

    boost::unit_test::init_unit_test_func init_func = &init_unit_test;
#else
    extern ::boost::unit_test::test_suite* init_unit_test_suite( int argc, char* argv[] );

    boost::unit_test::init_unit_test_func init_func = &init_unit_test_suite;
#endif

    return ::boost::unit_test::unit_test_main( init_func, argc, argv );
}

#endif // !BOOST_TEST_DYN_LINK && !BOOST_TEST_NO_MAIN

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UNIT_TEST_MAIN_IPP_012205GER
