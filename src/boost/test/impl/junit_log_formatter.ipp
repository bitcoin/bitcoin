//  (C) Copyright 2016 Raffi Enficiaud.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
///@file
///@brief Contains the implementatoin of the Junit log formatter (OF_JUNIT)
// ***************************************************************************

#ifndef BOOST_TEST_JUNIT_LOG_FORMATTER_IPP__
#define BOOST_TEST_JUNIT_LOG_FORMATTER_IPP__

// Boost.Test
#include <boost/test/output/junit_log_formatter.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/tree/test_unit.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/utils/xml_printer.hpp>
#include <boost/test/utils/string_cast.hpp>
#include <boost/test/framework.hpp>

#include <boost/test/tree/visitor.hpp>
#include <boost/test/tree/traverse.hpp>
#include <boost/test/results_collector.hpp>

#include <boost/test/utils/algorithm.hpp>
#include <boost/test/utils/string_cast.hpp>

//#include <boost/test/results_reporter.hpp>


// Boost
#include <boost/version.hpp>
#include <boost/core/ignore_unused.hpp>

// STL
#include <iostream>
#include <fstream>
#include <set>

#include <boost/test/detail/suppress_warnings.hpp>


//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace output {


struct s_replace_chars {
  template <class T>
  void operator()(T& to_replace)
  {
    if(to_replace == '/')
      to_replace = '.';
    else if(to_replace == ' ')
      to_replace = '_';
  }
};

inline std::string tu_name_normalize(std::string full_name)
{
  // maybe directly using normalize_test_case_name instead?
  std::for_each(full_name.begin(), full_name.end(), s_replace_chars());
  return full_name;
}

inline std::string tu_name_remove_newlines(std::string full_name)
{
  full_name.erase(std::remove(full_name.begin(), full_name.end(), '\n'), full_name.end());
  return full_name;
}

const_string file_basename(const_string filename) {

    const_string path_sep( "\\/" );
    const_string::iterator it = unit_test::utils::find_last_of( filename.begin(), filename.end(),
                                                                path_sep.begin(), path_sep.end() );
    if( it != filename.end() )
        filename.trim_left( it + 1 );

    return filename;

}

// ************************************************************************** //
// **************               junit_log_formatter              ************** //
// ************************************************************************** //

void
junit_log_formatter::log_start( std::ostream& /*ostr*/, counter_t /*test_cases_amount*/)
{
    map_tests.clear();
    list_path_to_root.clear();
    runner_log_entry.clear();
}

//____________________________________________________________________________//

class junit_result_helper : public test_tree_visitor {
private:
    typedef junit_impl::junit_log_helper::assertion_entry assertion_entry;
    typedef std::vector< assertion_entry >::const_iterator vect_assertion_entry_citerator;
    typedef std::list<std::string>::const_iterator list_str_citerator;

public:
    explicit junit_result_helper(
        std::ostream& stream,
        test_unit const& ts,
        junit_log_formatter::map_trace_t const& mt,
        junit_impl::junit_log_helper const& runner_log_,
        bool display_build_info )
    : m_stream(stream)
    , m_ts( ts )
    , m_map_test( mt )
    , runner_log( runner_log_ )
    , m_id( 0 )
    , m_display_build_info(display_build_info)
    { }

    void add_log_entry(assertion_entry const& log) const
    {
        std::string entry_type;
        if( log.log_entry == assertion_entry::log_entry_failure ) {
            entry_type = "failure";
        }
        else if( log.log_entry == assertion_entry::log_entry_error ) {
            entry_type = "error";
        }
        else {
            return;
        }

        m_stream
            << "<" << entry_type
            << " message" << utils::attr_value() << log.logentry_message
            << " type" << utils::attr_value() << log.logentry_type
            << ">";

        if(!log.output.empty()) {
            m_stream << utils::cdata() << "\n" + log.output;
        }

        m_stream << "</" << entry_type << ">";
    }

    struct conditional_cdata_helper {
        std::ostream &ostr;
        std::string const field;
        bool empty;

        conditional_cdata_helper(std::ostream &ostr_, std::string field_)
        : ostr(ostr_)
        , field(field_)
        , empty(true)
        {}

        ~conditional_cdata_helper() {
            if(!empty) {
                ostr << BOOST_TEST_L( "]]>" ) << "</" << field << '>' << std::endl;
            }
        }

        void operator()(const std::string& s) {
            bool current_empty = s.empty();
            if(empty) {
                if(!current_empty) {
                    empty = false;
                    ostr << '<' << field << '>' << BOOST_TEST_L( "<![CDATA[" );
                }
            }
            if(!current_empty) {
                ostr << s;
            }
        }
    };

    std::list<std::string> build_skipping_chain(test_unit const & tu) const
    {
        // we enter here because we know that the tu has been skipped.
        // either junit has not seen this tu, or it is indicated as disabled
        assert(m_map_test.count(tu.p_id) == 0 || results_collector.results( tu.p_id ).p_skipped);

        std::list<std::string> out;

        test_unit_id id(tu.p_id);
        while( id != m_ts.p_id && id != INV_TEST_UNIT_ID) {
            test_unit const& tu_hierarchy = boost::unit_test::framework::get( id, TUT_ANY );
            out.push_back("- disabled test unit: '" + tu_name_remove_newlines(tu_hierarchy.full_name()) + "'\n");
            if(m_map_test.count(id) > 0)
            {
                // junit has seen the reason: this is enough for constructing the chain
                break;
            }
            id = tu_hierarchy.p_parent_id;
        }
        junit_log_formatter::map_trace_t::const_iterator it_element_stack(m_map_test.find(id));
        if( it_element_stack != m_map_test.end() )
        {
            out.push_back("- reason: '" + it_element_stack->second.skipping_reason + "'");
            out.push_front("Test case disabled because of the following chain of decision:\n");
        }

        return out;
    }

    std::string get_class_name(test_unit const & tu_class) const {
        std::string classname;
        test_unit_id id(tu_class.p_parent_id);
        while( id != m_ts.p_id && id != INV_TEST_UNIT_ID ) {
            test_unit const& tu = boost::unit_test::framework::get( id, TUT_ANY );
            classname = tu_name_normalize(tu.p_name) + "." + classname;
            id = tu.p_parent_id;
        }

        // removes the trailing dot
        if(!classname.empty() && *classname.rbegin() == '.') {
            classname.erase(classname.size()-1);
        }

        return classname;
    }

    void    write_testcase_header(test_unit const & tu,
                                  test_results const *tr,
                                  int nb_assertions) const
    {
        std::string name;
        std::string classname;

        if(tu.p_id == m_ts.p_id ) {
            name = "boost_test";
        }
        else {
            classname = get_class_name(tu);
            name = tu_name_normalize(tu.p_name);
        }

        if( tu.p_type == TUT_SUITE ) {
            if(tr->p_timed_out)
              name += "-timed-execution";
            else
              name += "-setup-teardown";
        }

        m_stream << "<testcase assertions" << utils::attr_value() << nb_assertions;
        if(!classname.empty())
            m_stream << " classname" << utils::attr_value() << classname;

        // test case name and time taken
        m_stream
            << " name"      << utils::attr_value() << name
            << " time"      << utils::attr_value() << double(tr->p_duration_microseconds) * 1E-6
            << ">" << std::endl;
    }

    void    write_testcase_system_out(junit_impl::junit_log_helper const &detailed_log,
                                      test_unit const * tu,
                                      bool skipped) const
    {
        // system-out + all info/messages, the object skips the empty entries
        conditional_cdata_helper system_out_helper(m_stream, "system-out");

        // indicate why the test has been skipped first
        if( skipped ) {
            std::list<std::string> skipping_decision_chain = build_skipping_chain(*tu);
            for(list_str_citerator it(skipping_decision_chain.begin()), ite(skipping_decision_chain.end());
                it != ite;
                ++it)
            {
              system_out_helper(*it);
            }
        }

        // stdout
        for(list_str_citerator it(detailed_log.system_out.begin()), ite(detailed_log.system_out.end());
            it != ite;
            ++it)
        {
          system_out_helper(*it);
        }

        // warning/info message last
        for(vect_assertion_entry_citerator it(detailed_log.assertion_entries.begin());
            it != detailed_log.assertion_entries.end();
            ++it)
        {
            if(it->log_entry != assertion_entry::log_entry_info)
                continue;
            system_out_helper(it->output);
        }
    }

    void    write_testcase_system_err(junit_impl::junit_log_helper const &detailed_log,
                                      test_unit const * tu,
                                      test_results const *tr) const
    {
        // system-err output + test case informations
        bool has_failed = (tr != 0) ? !tr->p_skipped && !tr->passed() : false;
        if(!detailed_log.system_err.empty() || has_failed)
        {
            std::ostringstream o;
            if(has_failed) {
                o << "Failures detected in:" << std::endl;
            }
            else {
                o << "ERROR STREAM:" << std::endl;
            }

            if(tu->p_type == TUT_SUITE) {
                if( tu->p_id == m_ts.p_id ) {
                    o << " boost.test global setup/teardown" << std::endl;
                } else {
                    o << "- test suite: " << tu_name_remove_newlines(tu->full_name()) << std::endl;
                }
            }
            else {
              o << "- test case: " << tu_name_remove_newlines(tu->full_name());
              if(!tu->p_description.value.empty())
                  o << " '" << tu->p_description << "'";

              o << std::endl
                  << "- file: " << file_basename(tu->p_file_name) << std::endl
                  << "- line: " << tu->p_line_num << std::endl
                  ;
            }

            if(!detailed_log.system_err.empty())
                o << std::endl << "STDERR BEGIN: ------------" << std::endl;

            for(list_str_citerator it(detailed_log.system_err.begin()), ite(detailed_log.system_err.end());
                it != ite;
                ++it)
            {
              o << *it;
            }

            if(!detailed_log.system_err.empty())
                o << std::endl << "STDERR END    ------------" << std::endl;

            conditional_cdata_helper system_err_helper(m_stream, "system-err");
            system_err_helper(o.str());
        }
    }

    int     get_nb_assertions(junit_impl::junit_log_helper const &detailed_log,
                              test_unit const & tu,
                              test_results const *tr) const {
        int nb_assertions(-1);
        if( tu.p_type == TUT_SUITE ) {
            nb_assertions = 0;
            for(vect_assertion_entry_citerator it(detailed_log.assertion_entries.begin());
                it != detailed_log.assertion_entries.end();
                ++it)
            {
                if(it->log_entry != assertion_entry::log_entry_info)
                    nb_assertions++;
            }
        }
        else {
            nb_assertions = static_cast<int>(tr->p_assertions_passed + tr->p_assertions_failed);
        }

        return nb_assertions;
    }

    void    output_detailed_logs(junit_impl::junit_log_helper const &detailed_log,
                                 test_unit const & tu,
                                 bool skipped,
                                 test_results const *tr) const
    {
        int nb_assertions = get_nb_assertions(detailed_log, tu, tr);
        if(!nb_assertions && tu.p_type == TUT_SUITE)
            return;

        write_testcase_header(tu, tr, nb_assertions);

        if( skipped ) {
            m_stream << "<skipped/>" << std::endl;
        }
        else {

          for(vect_assertion_entry_citerator it(detailed_log.assertion_entries.begin());
              it != detailed_log.assertion_entries.end();
              ++it)
          {
              add_log_entry(*it);
          }
        }

        write_testcase_system_out(detailed_log, &tu, skipped);
        write_testcase_system_err(detailed_log, &tu, tr);
        m_stream << "</testcase>" << std::endl;
    }

    void    visit( test_case const& tc ) BOOST_OVERRIDE
    {

        test_results const& tr = results_collector.results( tc.p_id );
        junit_log_formatter::map_trace_t::const_iterator it_find = m_map_test.find(tc.p_id);
        if(it_find == m_map_test.end())
        {
            // test has been skipped and not seen by the logger
            output_detailed_logs(junit_impl::junit_log_helper(), tc, true, &tr);
        }
        else {
            output_detailed_logs(it_find->second, tc, tr.p_skipped, &tr);
        }
    }

    bool    test_suite_start( test_suite const& ts ) BOOST_OVERRIDE
    {
        test_results const& tr = results_collector.results( ts.p_id );

        // unique test suite, without s, nesting not supported in CI
        if( m_ts.p_id == ts.p_id ) {
            m_stream << "<testsuite";

            // think about: maybe we should add the number of fixtures of a test_suite as
            // independent tests (field p_fixtures).
            // same goes for the timed-execution: we can think of that as a separate test-unit
            // in the suite.
            // see https://llg.cubic.org/docs/junit/ and
            // http://svn.apache.org/viewvc/ant/core/trunk/src/main/org/apache/tools/ant/taskdefs/optional/junit/XMLJUnitResultFormatter.java?view=markup
            m_stream
              // << "disabled=\"" << tr.p_test_cases_skipped << "\" "
              << " tests"     << utils::attr_value()
                  << tr.p_test_cases_passed
                     + tr.p_test_cases_failed
                     // + tr.p_test_cases_aborted // aborted is also failed, we avoid counting it twice
              << " skipped"   << utils::attr_value() << tr.p_test_cases_skipped
              << " errors"    << utils::attr_value() << tr.p_test_cases_aborted
              << " failures"  << utils::attr_value()
                  << tr.p_test_cases_failed
                     + tr.p_test_suites_timed_out
                     + tr.p_test_cases_timed_out
                     - tr.p_test_cases_aborted // failed is not aborted in the Junit sense
              << " id"        << utils::attr_value() << m_id++
              << " name"      << utils::attr_value() << tu_name_normalize(ts.p_name)
              << " time"      << utils::attr_value() << (tr.p_duration_microseconds * 1E-6)
              << ">" << std::endl;

            if(m_display_build_info)
            {
                m_stream  << "<properties>" << std::endl;
                m_stream  << "<property name=\"platform\" value" << utils::attr_value() << BOOST_PLATFORM << " />" << std::endl;
                m_stream  << "<property name=\"compiler\" value" << utils::attr_value() << BOOST_COMPILER << " />" << std::endl;
                m_stream  << "<property name=\"stl\" value" << utils::attr_value() << BOOST_STDLIB << " />" << std::endl;

                std::ostringstream o;
                o << BOOST_VERSION/100000 << "." << BOOST_VERSION/100 % 1000 << "." << BOOST_VERSION % 100;
                m_stream  << "<property name=\"boost\" value" << utils::attr_value() << o.str() << " />" << std::endl;
                m_stream  << "</properties>" << std::endl;
            }
        }

        if( !tr.p_skipped ) {
            // if we land here, then this is a chance that we are logging the fixture setup/teardown of a test-suite.
            // the setup/teardown logging of a test-case is part of the test case.
            // we do not care about the test-suite that were skipped (really??)
            junit_log_formatter::map_trace_t::const_iterator it_find = m_map_test.find(ts.p_id);
            if(it_find != m_map_test.end()) {
                output_detailed_logs(it_find->second, ts, false, &tr);
            }
        }

        return true; // indicates that the children should also be parsed
    }

    void    test_suite_finish( test_suite const& ts ) BOOST_OVERRIDE
    {
        if( m_ts.p_id == ts.p_id ) {
            write_testcase_system_out(runner_log, 0, false);
            write_testcase_system_err(runner_log, 0, 0);

            m_stream << "</testsuite>";
            return;
        }
    }

private:
    // Data members
    std::ostream& m_stream;
    test_unit const& m_ts;
    junit_log_formatter::map_trace_t const& m_map_test;
    junit_impl::junit_log_helper const& runner_log;
    size_t m_id;
    bool m_display_build_info;
};



void
junit_log_formatter::log_finish( std::ostream& ostr )
{
    ostr << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;

    // getting the root test suite
    if(!map_tests.empty()) {
        test_unit* root = &boost::unit_test::framework::get( map_tests.begin()->first, TUT_ANY );

        // looking for the root of the SUBtree (we stay in the subtree)
        while(root->p_parent_id != INV_TEST_UNIT_ID && map_tests.count(root->p_parent_id) > 0) {
            root = &boost::unit_test::framework::get( root->p_parent_id, TUT_ANY );
        }
        junit_result_helper ch( ostr, *root, map_tests, this->runner_log_entry, m_display_build_info );
        traverse_test_tree( root->p_id, ch, true ); // last is to ignore disabled suite special handling
    }
    else {
        ostr << "<testsuites errors=\"1\">";
        ostr << "<testsuite errors=\"1\" name=\"boost-test-framework\">";
        ostr << "<testcase assertions=\"1\" name=\"test-setup\">";
        ostr << "<system-out>Incorrect setup: no test case executed</system-out>";
        ostr << "</testcase></testsuite></testsuites>";
    }
    return;
}

//____________________________________________________________________________//

void
junit_log_formatter::log_build_info( std::ostream& /*ostr*/, bool log_build_info )
{
    m_display_build_info = log_build_info;
}

//____________________________________________________________________________//

void
junit_log_formatter::test_unit_start( std::ostream& /*ostr*/, test_unit const& tu )
{
    list_path_to_root.push_back( tu.p_id );
    map_tests.insert(std::make_pair(tu.p_id, junit_impl::junit_log_helper())); // current_test_case_id not working here
}



//____________________________________________________________________________//


void
junit_log_formatter::test_unit_finish( std::ostream& /*ostr*/, test_unit const& tu, unsigned long /*elapsed*/ )
{
    // the time is already stored in the result_reporter
    boost::ignore_unused( tu );
    assert( tu.p_id == list_path_to_root.back() );
    list_path_to_root.pop_back();
}

void
junit_log_formatter::test_unit_aborted( std::ostream& /*ostr*/, test_unit const& tu )
{
    boost::ignore_unused( tu );
    assert( tu.p_id == list_path_to_root.back() );
    //list_path_to_root.pop_back();
}

//____________________________________________________________________________//

void
junit_log_formatter::test_unit_timed_out( std::ostream& /*os*/, test_unit const& tu)
{
    if(tu.p_type == TUT_SUITE)
    {
        // if we reach this call, it means that the test has already started and
        // test_unit_start has already been called on the tu.
        junit_impl::junit_log_helper& last_entry = get_current_log_entry();
        junit_impl::junit_log_helper::assertion_entry entry;
        entry.logentry_message = "test-suite time out";
        entry.logentry_type = "execution timeout";
        entry.log_entry = junit_impl::junit_log_helper::assertion_entry::log_entry_error;
        entry.output = "the current suite exceeded the allocated execution time";
        last_entry.assertion_entries.push_back(entry);
    }
}

//____________________________________________________________________________//

void
junit_log_formatter::test_unit_skipped( std::ostream& /*ostr*/, test_unit const& tu, const_string reason )
{
    // if a test unit is skipped, then the start of this TU has not been called yet.
    // we cannot use get_current_log_entry here, but the TU id should appear in the map.
    // The "skip" boolean is given by the boost.test framework
    junit_impl::junit_log_helper& v = map_tests[tu.p_id]; // not sure if we can use get_current_log_entry()
    v.skipping_reason.assign(reason.begin(), reason.end());
}

//____________________________________________________________________________//

void
junit_log_formatter::log_exception_start( std::ostream& /*ostr*/, log_checkpoint_data const& checkpoint_data, execution_exception const& ex )
{
    std::ostringstream o;
    execution_exception::location const& loc = ex.where();

    m_is_last_assertion_or_error = false;

    junit_impl::junit_log_helper& last_entry = get_current_log_entry();

    junit_impl::junit_log_helper::assertion_entry entry;

    entry.logentry_message = "unexpected exception";
    entry.log_entry = junit_impl::junit_log_helper::assertion_entry::log_entry_error;

    switch(ex.code())
    {
    case execution_exception::cpp_exception_error:
        entry.logentry_type = "uncaught exception";
        break;
    case execution_exception::timeout_error:
        entry.logentry_type = "execution timeout";
        break;
    case execution_exception::user_error:
        entry.logentry_type = "user, assert() or CRT error";
        break;
    case execution_exception::user_fatal_error:
        // Looks like never used
        entry.logentry_type = "user fatal error";
        break;
    case execution_exception::system_error:
        entry.logentry_type = "system error";
        break;
    case execution_exception::system_fatal_error:
        entry.logentry_type = "system fatal error";
        break;
    default:
        entry.logentry_type = "no error"; // not sure how to handle this one
        break;
    }

    o << "UNCAUGHT EXCEPTION:" << std::endl;
    if( !loc.m_function.is_empty() )
        o << "- function: \""   << loc.m_function << "\"" << std::endl;

    o << "- file: " << file_basename(loc.m_file_name) << std::endl
      << "- line: " << loc.m_line_num << std::endl
      << std::endl;

    o << "\nEXCEPTION STACK TRACE: --------------\n" << ex.what()
      << "\n-------------------------------------";

    if( !checkpoint_data.m_file_name.is_empty() ) {
        o << std::endl << std::endl
          << "Last checkpoint:" << std::endl
          << "- message: \"" << checkpoint_data.m_message << "\"" << std::endl
          << "- file: " << file_basename(checkpoint_data.m_file_name) << std::endl
          << "- line: " << checkpoint_data.m_line_num << std::endl
        ;
    }

    entry.output = o.str();

    last_entry.assertion_entries.push_back(entry);
}

//____________________________________________________________________________//

void
junit_log_formatter::log_exception_finish( std::ostream& /*ostr*/ )
{
    // sealing the last entry
    assert(!get_current_log_entry().assertion_entries.back().sealed);
    get_current_log_entry().assertion_entries.back().sealed = true;
}

//____________________________________________________________________________//

void
junit_log_formatter::log_entry_start( std::ostream& /*ostr*/, log_entry_data const& entry_data, log_entry_types let )
{
    junit_impl::junit_log_helper& last_entry = get_current_log_entry();
    last_entry.skipping = false;
    m_is_last_assertion_or_error = true;
    switch(let)
    {
      case unit_test_log_formatter::BOOST_UTL_ET_INFO:
      {
        if(m_log_level_internal > log_successful_tests) {
          last_entry.skipping = true;
          break;
        }
        BOOST_FALLTHROUGH;
      }
      case unit_test_log_formatter::BOOST_UTL_ET_MESSAGE:
      {
        if(m_log_level_internal > log_messages) {
          last_entry.skipping = true;
          break;
        }
        BOOST_FALLTHROUGH;
      }
      case unit_test_log_formatter::BOOST_UTL_ET_WARNING:
      {
        if(m_log_level_internal > log_warnings) {
          last_entry.skipping = true;
          break;
        }
        std::ostringstream o;
        junit_impl::junit_log_helper::assertion_entry entry;

        entry.log_entry = junit_impl::junit_log_helper::assertion_entry::log_entry_info;
        entry.logentry_message = "info";
        entry.logentry_type = "message";

        o << (let == unit_test_log_formatter::BOOST_UTL_ET_WARNING ?
              "WARNING:" : (let == unit_test_log_formatter::BOOST_UTL_ET_MESSAGE ?
                            "MESSAGE:" : "INFO:"))
             << std::endl
          << "- file   : " << file_basename(entry_data.m_file_name) << std::endl
          << "- line   : " << entry_data.m_line_num << std::endl
          << "- message: "; // no CR

        entry.output += o.str();
        last_entry.assertion_entries.push_back(entry);
        break;
      }
      default:
      case unit_test_log_formatter::BOOST_UTL_ET_ERROR:
      case unit_test_log_formatter::BOOST_UTL_ET_FATAL_ERROR:
      {
        std::ostringstream o;
        junit_impl::junit_log_helper::assertion_entry entry;
        entry.log_entry = junit_impl::junit_log_helper::assertion_entry::log_entry_failure;
        entry.logentry_message = "failure";
        entry.logentry_type = (let == unit_test_log_formatter::BOOST_UTL_ET_ERROR ? "assertion error" : "fatal error");

        o << "ASSERTION FAILURE:" << std::endl
          << "- file   : " << file_basename(entry_data.m_file_name) << std::endl
          << "- line   : " << entry_data.m_line_num << std::endl
          << "- message: " ; // no CR

        entry.output += o.str();
        last_entry.assertion_entries.push_back(entry);
        break;
      }
    }
}

//____________________________________________________________________________//

void
junit_log_formatter::log_entry_value( std::ostream& /*ostr*/, const_string value )
{
    junit_impl::junit_log_helper& last_entry = get_current_log_entry();
    if(last_entry.skipping)
        return;

    assert(last_entry.assertion_entries.empty() || !last_entry.assertion_entries.back().sealed);

    if(!last_entry.assertion_entries.empty())
    {
        junit_impl::junit_log_helper::assertion_entry& log_entry = last_entry.assertion_entries.back();
        log_entry.output += value;
    }
    else
    {
        // this may be a message coming from another observer
        // the prefix is set in the log_entry_start
        last_entry.system_out.push_back(std::string(value.begin(), value.end()));
    }
}

//____________________________________________________________________________//

void
junit_log_formatter::log_entry_finish( std::ostream& /*ostr*/ )
{
    junit_impl::junit_log_helper& last_entry = get_current_log_entry();
    if(!last_entry.skipping)
    {
        assert(last_entry.assertion_entries.empty() || !last_entry.assertion_entries.back().sealed);

        if(!last_entry.assertion_entries.empty()) {
            junit_impl::junit_log_helper::assertion_entry& log_entry = last_entry.assertion_entries.back();
            log_entry.output += "\n\n"; // quote end, CR
            log_entry.sealed = true;
        }
        else {
            last_entry.system_out.push_back("\n\n"); // quote end, CR
        }
    }

    last_entry.skipping = false;
}

//____________________________________________________________________________//

void
junit_log_formatter::entry_context_start( std::ostream& /*ostr*/, log_level )
{
    junit_impl::junit_log_helper& last_entry = get_current_log_entry();
    if(last_entry.skipping)
        return;

    std::vector< junit_impl::junit_log_helper::assertion_entry > &v_failure_or_error = last_entry.assertion_entries;
    assert(!v_failure_or_error.back().sealed);

    junit_impl::junit_log_helper::assertion_entry& last_log_entry = v_failure_or_error.back();
    if(m_is_last_assertion_or_error)
    {
        last_log_entry.output += "\n- context:\n";
    }
    else
    {
        last_log_entry.output += "\n\nCONTEXT:\n";
    }
}

//____________________________________________________________________________//

void
junit_log_formatter::entry_context_finish( std::ostream& /*ostr*/, log_level )
{
    // no op, may be removed
    junit_impl::junit_log_helper& last_entry = get_current_log_entry();
    if(last_entry.skipping)
        return;
    assert(!get_current_log_entry().assertion_entries.back().sealed);
}

//____________________________________________________________________________//

void
junit_log_formatter::log_entry_context( std::ostream& /*ostr*/, log_level , const_string context_descr )
{
    junit_impl::junit_log_helper& last_entry = get_current_log_entry();
    if(last_entry.skipping)
        return;

    assert(!last_entry.assertion_entries.back().sealed);
    junit_impl::junit_log_helper::assertion_entry& last_log_entry = get_current_log_entry().assertion_entries.back();

    last_log_entry.output +=
        (m_is_last_assertion_or_error ? "  - '": "- '") + std::string(context_descr.begin(), context_descr.end()) + "'\n"; // quote end
}

//____________________________________________________________________________//


std::string
junit_log_formatter::get_default_stream_description() const {
    std::string name = framework::master_test_suite().p_name.value;

    static const std::string to_replace[] =  { " ", "\"", "/", "\\", ":"};
    static const std::string replacement[] = { "_", "_" , "_", "_" , "_"};

    name = unit_test::utils::replace_all_occurrences_of(
        name,
        to_replace, to_replace + sizeof(to_replace)/sizeof(to_replace[0]),
        replacement, replacement + sizeof(replacement)/sizeof(replacement[0]));

    std::ifstream check_init((name + ".xml").c_str());
    if(!check_init)
        return name + ".xml";

    int index = 0;
    for(; index < 100; index++) {
      std::string candidate = name + "_" + utils::string_cast(index) + ".xml";
      std::ifstream file(candidate.c_str());
      if(!file)
          return candidate;
    }

    return name + ".xml";
}

} // namespace output
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_junit_log_formatter_IPP_020105GER
