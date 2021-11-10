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
//  Description : implemets Unit Test Log
// ***************************************************************************

#ifndef BOOST_TEST_UNIT_TEST_LOG_IPP_012205GER
#define BOOST_TEST_UNIT_TEST_LOG_IPP_012205GER

// Boost.Test
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_log_formatter.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/unit_test_parameters.hpp>

#include <boost/test/utils/basic_cstring/compare.hpp>
#include <boost/test/utils/foreach.hpp>

#include <boost/test/output/compiler_log_formatter.hpp>
#include <boost/test/output/xml_log_formatter.hpp>
#include <boost/test/output/junit_log_formatter.hpp>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/io/ios_state.hpp>
typedef ::boost::io::ios_base_all_saver io_saver_type;

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

// ************************************************************************** //
// **************             entry_value_collector            ************** //
// ************************************************************************** //

namespace ut_detail {

entry_value_collector const&
entry_value_collector::operator<<( lazy_ostream const& v ) const
{
    unit_test_log << v;

    return *this;
}

//____________________________________________________________________________//

entry_value_collector const&
entry_value_collector::operator<<( const_string v ) const
{
    unit_test_log << v;

    return *this;
}

//____________________________________________________________________________//

entry_value_collector::~entry_value_collector()
{
    if( m_last )
        unit_test_log << log::end();
}

//____________________________________________________________________________//

} // namespace ut_detail

// ************************************************************************** //
// **************                 unit_test_log                ************** //
// ************************************************************************** //

namespace {

// log data
struct unit_test_log_data_helper_impl {
  typedef boost::shared_ptr<unit_test_log_formatter> formatter_ptr;
  typedef boost::shared_ptr<io_saver_type>           saver_ptr;

  bool                m_enabled;
  output_format       m_format;
  std::ostream*       m_stream;
  saver_ptr           m_stream_state_saver;
  formatter_ptr       m_log_formatter;
  bool                m_entry_in_progress;

  unit_test_log_data_helper_impl(unit_test_log_formatter* p_log_formatter, output_format format, bool enabled = false)
    : m_enabled( enabled )
    , m_format( format )
    , m_stream( &std::cout )
    , m_stream_state_saver( new io_saver_type( std::cout ) )
    , m_log_formatter()
    , m_entry_in_progress( false )
  {
    m_log_formatter.reset(p_log_formatter);
    m_log_formatter->set_log_level(log_all_errors);
  }

  // helper functions
  std::ostream&       stream()
  {
      return *m_stream;
  }

  log_level get_log_level() const
  {
      return m_log_formatter->get_log_level();
  }
};

struct unit_test_log_impl {
    // Constructor
    unit_test_log_impl()
    {
      m_log_formatter_data.push_back( unit_test_log_data_helper_impl(new output::compiler_log_formatter, OF_CLF, true) ); // only this one is active by default,
      m_log_formatter_data.push_back( unit_test_log_data_helper_impl(new output::xml_log_formatter, OF_XML, false) );
      m_log_formatter_data.push_back( unit_test_log_data_helper_impl(new output::junit_log_formatter, OF_JUNIT, false) );
    }

    typedef std::vector<unit_test_log_data_helper_impl> v_formatter_data_t;
    v_formatter_data_t m_log_formatter_data;

    typedef std::vector<unit_test_log_data_helper_impl*> vp_formatter_data_t;
    vp_formatter_data_t m_active_log_formatter_data;

    // entry data
    log_entry_data      m_entry_data;

    bool has_entry_in_progress() const {
        for( vp_formatter_data_t::const_iterator it(m_active_log_formatter_data.begin()), ite(m_active_log_formatter_data.end()); 
             it < ite; 
             ++it)
        {
            unit_test_log_data_helper_impl& current_logger_data = **it;
            if( current_logger_data.m_entry_in_progress )
                return true;
        }
        return false;
    }

    // check point data
    log_checkpoint_data m_checkpoint_data;

    void                set_checkpoint( const_string file, std::size_t line_num, const_string msg )
    {
        assign_op( m_checkpoint_data.m_message, msg, 0 );
        m_checkpoint_data.m_file_name   = file;
        m_checkpoint_data.m_line_num    = line_num;
    }
};

unit_test_log_impl& s_log_impl() { static unit_test_log_impl the_inst; return the_inst; }


//____________________________________________________________________________//

void
log_entry_context( log_level l, unit_test_log_data_helper_impl& current_logger_data)
{
    framework::context_generator const& context = framework::get_context();
    if( context.is_empty() )
        return;

    const_string frame;
    current_logger_data.m_log_formatter->entry_context_start( current_logger_data.stream(), l );
    while( !(frame=context.next()).is_empty() )
    {
        current_logger_data.m_log_formatter->log_entry_context( current_logger_data.stream(), l, frame );
    }
    current_logger_data.m_log_formatter->entry_context_finish( current_logger_data.stream(), l );
}

//____________________________________________________________________________//

void
clear_entry_context()
{
    framework::clear_context();
}

// convenience
typedef unit_test_log_impl::vp_formatter_data_t vp_logger_t;
typedef unit_test_log_impl::v_formatter_data_t v_logger_t;

} // local namespace

//____________________________________________________________________________//

BOOST_TEST_SINGLETON_CONS_IMPL( unit_test_log_t )

void
unit_test_log_t::configure( )
{
    // configure is not test_start:
    // test_start pushes the necessary log information when the test module is starting, and implies configure.
    // configure: should be called each time the set of loggers, stream or configuration is changed.
    s_log_impl().m_active_log_formatter_data.clear();
    for( unit_test_log_impl::v_formatter_data_t::iterator it(s_log_impl().m_log_formatter_data.begin()), 
                                                          ite(s_log_impl().m_log_formatter_data.end());
        it < ite;
        ++it)
    {
      if( !it->m_enabled || it->get_log_level() == log_nothing )
          continue;

      s_log_impl().m_active_log_formatter_data.push_back(&*it);
      it->m_entry_in_progress = false;
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::test_start( counter_t test_cases_amount, test_unit_id )
{
    configure();
    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
      unit_test_log_data_helper_impl& current_logger_data = **it;

      current_logger_data.m_log_formatter->log_start( current_logger_data.stream(), test_cases_amount );
      current_logger_data.m_log_formatter->log_build_info(
          current_logger_data.stream(),
          runtime_config::get<bool>( runtime_config::btrt_build_info ));

      //current_logger_data.stream().flush();
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::test_finish()
{
    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
      unit_test_log_data_helper_impl& current_logger_data = **it;
      current_logger_data.m_log_formatter->log_finish( current_logger_data.stream() );
      current_logger_data.stream().flush();
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::test_aborted()
{
    BOOST_TEST_LOG_ENTRY( log_messages ) << "Test is aborted";
}

//____________________________________________________________________________//

void
unit_test_log_t::test_unit_start( test_unit const& tu )
{
    if( s_log_impl().has_entry_in_progress() )
        *this << log::end();

    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = **it;
        if( current_logger_data.get_log_level() > log_test_units )
            continue;
        current_logger_data.m_log_formatter->test_unit_start( current_logger_data.stream(), tu );
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::test_unit_finish( test_unit const& tu, unsigned long elapsed )
{
    s_log_impl().m_checkpoint_data.clear();

    if( s_log_impl().has_entry_in_progress() )
        *this << log::end();

    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = **it;
        if( current_logger_data.get_log_level() > log_test_units )
            continue;

        current_logger_data.m_log_formatter->test_unit_finish( current_logger_data.stream(), tu, elapsed );
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::test_unit_skipped( test_unit const& tu, const_string reason )
{
    if( s_log_impl().has_entry_in_progress() )
        *this << log::end();

    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = **it;
        if( current_logger_data.get_log_level() > log_test_units )
            continue;

        current_logger_data.m_log_formatter->test_unit_skipped( current_logger_data.stream(), tu, reason );
    }
}

void
unit_test_log_t::test_unit_aborted( test_unit const& tu )
{
    if( s_log_impl().has_entry_in_progress() )
        *this << log::end();

    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = **it;
        if( current_logger_data.get_log_level() > log_test_units )
            continue;

        current_logger_data.m_log_formatter->test_unit_aborted(current_logger_data.stream(), tu );
    }
}

void
unit_test_log_t::test_unit_timed_out( test_unit const& tu )
{
    if( s_log_impl().has_entry_in_progress() )
        *this << log::end();

    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = **it;
        if( current_logger_data.get_log_level() > log_test_units )
            continue;

        current_logger_data.m_log_formatter->test_unit_timed_out(current_logger_data.stream(), tu );
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::exception_caught( execution_exception const& ex )
{
    log_level l =
        ex.code() <= execution_exception::cpp_exception_error   ? log_cpp_exception_errors :
        (ex.code() <= execution_exception::timeout_error        ? log_system_errors
                                                                : log_fatal_errors );

    if( s_log_impl().has_entry_in_progress() )
        *this << log::end();

    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
      unit_test_log_data_helper_impl& current_logger_data = **it;

      if( l >= current_logger_data.get_log_level() ) {

          current_logger_data.m_log_formatter->log_exception_start( current_logger_data.stream(), s_log_impl().m_checkpoint_data, ex );

          log_entry_context( l, current_logger_data );

          current_logger_data.m_log_formatter->log_exception_finish( current_logger_data.stream() );
      }
    }
    clear_entry_context();
}

//____________________________________________________________________________//

void
unit_test_log_t::set_checkpoint( const_string file, std::size_t line_num, const_string msg )
{
    s_log_impl().set_checkpoint( file, line_num, msg );
}

//____________________________________________________________________________//

char
set_unix_slash( char in )
{
    return in == '\\' ? '/' : in;
}

unit_test_log_t&
unit_test_log_t::operator<<( log::begin const& b )
{
    if( s_log_impl().has_entry_in_progress() )
        *this << log::end();

    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
      unit_test_log_data_helper_impl& current_logger_data = **it;
      current_logger_data.m_stream_state_saver->restore();
    }

    s_log_impl().m_entry_data.clear();

    assign_op( s_log_impl().m_entry_data.m_file_name, b.m_file_name, 0 );

    // normalize file name
    std::transform( s_log_impl().m_entry_data.m_file_name.begin(), s_log_impl().m_entry_data.m_file_name.end(),
                    s_log_impl().m_entry_data.m_file_name.begin(),
                    &set_unix_slash );

    s_log_impl().m_entry_data.m_line_num = b.m_line_num;

    return *this;
}

//____________________________________________________________________________//

unit_test_log_t&
unit_test_log_t::operator<<( log::end const& )
{
    if( s_log_impl().has_entry_in_progress() ) {
        vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
        log_level l = s_log_impl().m_entry_data.m_level;
        for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
        {
            unit_test_log_data_helper_impl& current_logger_data = **it;
            if( current_logger_data.m_entry_in_progress ) {
                if( l >= current_logger_data.get_log_level() ) {
                    log_entry_context( l, current_logger_data );
                }
                current_logger_data.m_log_formatter->log_entry_finish( current_logger_data.stream() );
            }
            current_logger_data.m_entry_in_progress = false;
        }
    }

    clear_entry_context();

    return *this;
}

//____________________________________________________________________________//

unit_test_log_t&
unit_test_log_t::operator<<( log_level l )
{
    s_log_impl().m_entry_data.m_level = l;

    return *this;
}

//____________________________________________________________________________//

ut_detail::entry_value_collector
unit_test_log_t::operator()( log_level l )
{
    *this << l;

    return ut_detail::entry_value_collector();
}

//____________________________________________________________________________//

bool
log_entry_start(unit_test_log_data_helper_impl &current_logger_data)
{
    if( current_logger_data.m_entry_in_progress )
        return true;

    switch( s_log_impl().m_entry_data.m_level ) {
    case log_successful_tests:
        current_logger_data.m_log_formatter->log_entry_start( current_logger_data.stream(), s_log_impl().m_entry_data,
                                              unit_test_log_formatter::BOOST_UTL_ET_INFO );
        break;
    case log_messages:
        current_logger_data.m_log_formatter->log_entry_start( current_logger_data.stream(), s_log_impl().m_entry_data,
                                              unit_test_log_formatter::BOOST_UTL_ET_MESSAGE );
        break;
    case log_warnings:
        current_logger_data.m_log_formatter->log_entry_start( current_logger_data.stream(), s_log_impl().m_entry_data,
                                              unit_test_log_formatter::BOOST_UTL_ET_WARNING );
        break;
    case log_all_errors:
    case log_cpp_exception_errors:
    case log_system_errors:
        current_logger_data.m_log_formatter->log_entry_start( current_logger_data.stream(), s_log_impl().m_entry_data,
                                              unit_test_log_formatter::BOOST_UTL_ET_ERROR );
        break;
    case log_fatal_errors:
        current_logger_data.m_log_formatter->log_entry_start( current_logger_data.stream(), s_log_impl().m_entry_data,
                                              unit_test_log_formatter::BOOST_UTL_ET_FATAL_ERROR );
        break;
    case log_nothing:
    case log_test_units:
    case invalid_log_level:
        return false;
    }

    current_logger_data.m_entry_in_progress = true;
    return true;
}

//____________________________________________________________________________//

unit_test_log_t&
unit_test_log_t::operator<<( const_string value )
{
    if(value.empty()) {
        return *this;
    }

    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = **it;
        if( s_log_impl().m_entry_data.m_level >= current_logger_data.get_log_level() )
            if( log_entry_start(current_logger_data) ) {
                current_logger_data.m_log_formatter->log_entry_value( current_logger_data.stream(), value );
            }
    }
    return *this;
}

//____________________________________________________________________________//

unit_test_log_t&
unit_test_log_t::operator<<( lazy_ostream const& value )
{
    if(value.empty()) {
        return *this;
    }

    vp_logger_t& vloggers = s_log_impl().m_active_log_formatter_data;
    for( vp_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = **it;
        if( s_log_impl().m_entry_data.m_level >= current_logger_data.get_log_level() ) {
            if( log_entry_start(current_logger_data) ) {
                current_logger_data.m_log_formatter->log_entry_value( current_logger_data.stream(), value );
            }
        }
    }
    return *this;
}

//____________________________________________________________________________//

void
unit_test_log_t::set_stream( std::ostream& str )
{
    if( s_log_impl().has_entry_in_progress() )
        return;

    v_logger_t& vloggers = s_log_impl().m_log_formatter_data;
    for( v_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = *it;

        current_logger_data.m_stream = &str;
        current_logger_data.m_stream_state_saver.reset( new io_saver_type( str ) );
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::set_stream( output_format log_format, std::ostream& str )
{
    if( s_log_impl().has_entry_in_progress() )
        return;

    v_logger_t& vloggers = s_log_impl().m_log_formatter_data;
    for( v_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = *it;
        if( current_logger_data.m_format == log_format) {
            current_logger_data.m_stream = &str;
            current_logger_data.m_stream_state_saver.reset( new io_saver_type( str ) );
            break;
        }
    }
}

std::ostream*
unit_test_log_t::get_stream( output_format log_format ) const
{
    v_logger_t& vloggers = s_log_impl().m_log_formatter_data;
    for( v_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = *it;
        if( current_logger_data.m_format == log_format) {
            return current_logger_data.m_stream;
        }
    }
    return 0;
}

//____________________________________________________________________________//

log_level
unit_test_log_t::set_threshold_level( log_level lev )
{
    if( s_log_impl().has_entry_in_progress() || lev == invalid_log_level )
        return invalid_log_level;

    log_level ret = log_nothing;
    v_logger_t& vloggers = s_log_impl().m_log_formatter_data;
    for( v_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = *it;
        ret = (std::min)(ret, current_logger_data.m_log_formatter->get_log_level());
        current_logger_data.m_log_formatter->set_log_level( lev );
    }
    return ret;
}

//____________________________________________________________________________//

log_level
unit_test_log_t::set_threshold_level( output_format log_format, log_level lev )
{
    if( s_log_impl().has_entry_in_progress() || lev == invalid_log_level )
        return invalid_log_level;

    log_level ret = log_nothing;
    v_logger_t& vloggers = s_log_impl().m_log_formatter_data;
    for( v_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = *it;
        if( current_logger_data.m_format == log_format) {
            ret = current_logger_data.m_log_formatter->get_log_level(); 
            current_logger_data.m_log_formatter->set_log_level( lev );
            break;
        }
    }
    return ret;
}

//____________________________________________________________________________//

void
unit_test_log_t::set_format( output_format log_format )
{
    if( s_log_impl().has_entry_in_progress() )
        return;

    v_logger_t& vloggers = s_log_impl().m_log_formatter_data;
    for( v_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = *it;
        current_logger_data.m_enabled = current_logger_data.m_format == log_format;
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::add_format( output_format log_format )
{
    if( s_log_impl().has_entry_in_progress() )
        return;

    v_logger_t& vloggers = s_log_impl().m_log_formatter_data;
    for( v_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = *it;
        if( current_logger_data.m_format == log_format) {
            current_logger_data.m_enabled = true;
            break;
        }
    }
}

//____________________________________________________________________________//

unit_test_log_formatter*
unit_test_log_t::get_formatter( output_format log_format ) {
    
    v_logger_t& vloggers = s_log_impl().m_log_formatter_data;
    for( v_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        unit_test_log_data_helper_impl& current_logger_data = *it;
        if( current_logger_data.m_format == log_format) {
            return current_logger_data.m_log_formatter.get();
        }
    }
    return 0;
}


void
unit_test_log_t::add_formatter( unit_test_log_formatter* the_formatter )
{
    // remove only user defined logger
    v_logger_t& vloggers = s_log_impl().m_log_formatter_data;
    for(v_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        if( it->m_format == OF_CUSTOM_LOGGER) {
            s_log_impl().m_log_formatter_data.erase(it);
            break;
        }
    }

    if( the_formatter ) {
        s_log_impl().m_active_log_formatter_data.clear(); // otherwise dandling references
        vloggers.push_back( unit_test_log_data_helper_impl(the_formatter, OF_CUSTOM_LOGGER, true) );
    }
}

void
unit_test_log_t::set_formatter( unit_test_log_formatter* the_formatter )
{
    if( s_log_impl().has_entry_in_progress() )
        return;

    // remove only user defined logger
    log_level current_level = invalid_log_level;
    std::ostream *current_stream = 0;
    output_format previous_format = OF_INVALID;
    v_logger_t& vloggers = s_log_impl().m_log_formatter_data;
    for(v_logger_t::iterator it(vloggers.begin()), ite(vloggers.end()); it < ite; ++it)
    {
        if( it->m_enabled ) {
            if( current_level == invalid_log_level || it->m_format < previous_format || it->m_format == OF_CUSTOM_LOGGER) {
                current_level = it->get_log_level();
                current_stream = &(it->stream());
                previous_format = it->m_format;
            }
        }
    }

    if( the_formatter ) {
        add_formatter(the_formatter);
        set_format(OF_CUSTOM_LOGGER);
        set_threshold_level(OF_CUSTOM_LOGGER, current_level);
        set_stream(OF_CUSTOM_LOGGER, *current_stream);
    }

    configure();
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************            unit_test_log_formatter           ************** //
// ************************************************************************** //

void
unit_test_log_formatter::log_entry_value( std::ostream& ostr, lazy_ostream const& value )
{
    log_entry_value( ostr, (wrap_stringstream().ref() << value).str() );
}

void
unit_test_log_formatter::set_log_level(log_level new_log_level)
{
    m_log_level = new_log_level;
}

log_level
unit_test_log_formatter::get_log_level() const
{
    return m_log_level;
}

//____________________________________________________________________________//

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UNIT_TEST_LOG_IPP_012205GER

