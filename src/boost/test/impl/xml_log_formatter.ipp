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
//  Description : implements OF_XML Log formatter
// ***************************************************************************

#ifndef BOOST_TEST_XML_LOG_FORMATTER_IPP_020105GER
#define BOOST_TEST_XML_LOG_FORMATTER_IPP_020105GER

// Boost.Test
#include <boost/test/output/xml_log_formatter.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/tree/test_unit.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/utils/xml_printer.hpp>

// Boost
#include <boost/version.hpp>

// STL
#include <iostream>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace output {

static const_string tu_type_name( test_unit const& tu )
{
    return tu.p_type == TUT_CASE ? "TestCase" : "TestSuite";
}

// ************************************************************************** //
// **************               xml_log_formatter              ************** //
// ************************************************************************** //

void
xml_log_formatter::log_start( std::ostream& ostr, counter_t )
{
    ostr  << "<TestLog>";
}

//____________________________________________________________________________//

void
xml_log_formatter::log_finish( std::ostream& ostr )
{
    ostr  << "</TestLog>";
}

//____________________________________________________________________________//

void
xml_log_formatter::log_build_info( std::ostream& ostr, bool log_build_info )
{
    if( log_build_info ) {
        ostr  << "<BuildInfo"
                << " platform"  << utils::attr_value() << BOOST_PLATFORM
                << " compiler"  << utils::attr_value() << BOOST_COMPILER
                << " stl"       << utils::attr_value() << BOOST_STDLIB
                << " boost=\""  << BOOST_VERSION/100000     << "."
                                << BOOST_VERSION/100 % 1000 << "."
                                << BOOST_VERSION % 100      << '\"'
                << "/>";
    }
}

//____________________________________________________________________________//

void
xml_log_formatter::test_unit_start( std::ostream& ostr, test_unit const& tu )
{
    ostr << "<" << tu_type_name( tu ) << " name" << utils::attr_value() << tu.p_name.get();

    if( !tu.p_file_name.empty() )
        ostr << BOOST_TEST_L( " file" ) << utils::attr_value() << tu.p_file_name
             << BOOST_TEST_L( " line" ) << utils::attr_value() << tu.p_line_num;

    ostr << ">";
}

//____________________________________________________________________________//

void
xml_log_formatter::test_unit_finish( std::ostream& ostr, test_unit const& tu, unsigned long elapsed )
{
    if( tu.p_type == TUT_CASE )
        ostr << "<TestingTime>" << elapsed << "</TestingTime>";

    ostr << "</" << tu_type_name( tu ) << ">";
}

//____________________________________________________________________________//

void
xml_log_formatter::test_unit_skipped( std::ostream& ostr, test_unit const& tu, const_string reason )
{
    ostr << "<" << tu_type_name( tu )
         << " name"    << utils::attr_value() << tu.p_name.get()
         << " skipped" << utils::attr_value() << "yes"
         << " reason"  << utils::attr_value() << reason
         << "/>";
}

//____________________________________________________________________________//

void
xml_log_formatter::log_exception_start( std::ostream& ostr, log_checkpoint_data const& checkpoint_data, execution_exception const& ex )
{
    execution_exception::location const& loc = ex.where();

    ostr << "<Exception file" << utils::attr_value() << loc.m_file_name
         << " line"           << utils::attr_value() << loc.m_line_num;

    if( !loc.m_function.is_empty() )
        ostr << " function"   << utils::attr_value() << loc.m_function;

    ostr << ">" << utils::cdata() << ex.what();

    if( !checkpoint_data.m_file_name.is_empty() ) {
        ostr << "<LastCheckpoint file" << utils::attr_value() << checkpoint_data.m_file_name
             << " line"                << utils::attr_value() << checkpoint_data.m_line_num
             << ">"
             << utils::cdata() << checkpoint_data.m_message
             << "</LastCheckpoint>";
    }
}

//____________________________________________________________________________//

void
xml_log_formatter::log_exception_finish( std::ostream& ostr )
{
    ostr << "</Exception>";
}

//____________________________________________________________________________//

void
xml_log_formatter::log_entry_start( std::ostream& ostr, log_entry_data const& entry_data, log_entry_types let )
{
    static literal_string xml_tags[] = { "Info", "Message", "Warning", "Error", "FatalError" };

    m_curr_tag = xml_tags[let];
    ostr << '<' << m_curr_tag
         << BOOST_TEST_L( " file" ) << utils::attr_value() << entry_data.m_file_name
         << BOOST_TEST_L( " line" ) << utils::attr_value() << entry_data.m_line_num
         << BOOST_TEST_L( "><![CDATA[" );

    m_value_closed = false;
}

//____________________________________________________________________________//

void
xml_log_formatter::log_entry_value( std::ostream& ostr, const_string value )
{
    utils::print_escaped_cdata( ostr, value );
}

//____________________________________________________________________________//

void
xml_log_formatter::log_entry_finish( std::ostream& ostr )
{
    if( !m_value_closed ) {
        ostr << BOOST_TEST_L( "]]>" );
        m_value_closed = true;
    }

    ostr << BOOST_TEST_L( "</" ) << m_curr_tag << BOOST_TEST_L( ">" );

    m_curr_tag.clear();
}

//____________________________________________________________________________//

void
xml_log_formatter::entry_context_start( std::ostream& ostr, log_level )
{
    if( !m_value_closed ) {
        ostr << BOOST_TEST_L( "]]>" );
        m_value_closed = true;
    }

    ostr << BOOST_TEST_L( "<Context>" );
}

//____________________________________________________________________________//

void
xml_log_formatter::entry_context_finish( std::ostream& ostr, log_level )
{
    ostr << BOOST_TEST_L( "</Context>" );
}

//____________________________________________________________________________//

void
xml_log_formatter::log_entry_context( std::ostream& ostr, log_level, const_string context_descr )
{
    ostr << BOOST_TEST_L( "<Frame>" ) << utils::cdata() << context_descr << BOOST_TEST_L( "</Frame>" );
}

//____________________________________________________________________________//

} // namespace output
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_XML_LOG_FORMATTER_IPP_020105GER
