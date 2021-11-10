//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// @brief Provides access to various Unit Test Framework runtime parameters
///
/// Primarily for use by the framework itself
// ***************************************************************************

#ifndef BOOST_TEST_UNIT_TEST_PARAMETERS_HPP_071894GER
#define BOOST_TEST_UNIT_TEST_PARAMETERS_HPP_071894GER

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/utils/runtime/argument.hpp>
#include <boost/make_shared.hpp>

// Boost
#include <boost/function/function0.hpp>

// STL
#include <iostream>
#include <fstream>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace runtime_config {

// ************************************************************************** //
// **************                 runtime_config               ************** //
// ************************************************************************** //

// UTF parameters
BOOST_TEST_DECL extern std::string btrt_auto_start_dbg;
BOOST_TEST_DECL extern std::string btrt_break_exec_path;
BOOST_TEST_DECL extern std::string btrt_build_info;
BOOST_TEST_DECL extern std::string btrt_catch_sys_errors;
BOOST_TEST_DECL extern std::string btrt_color_output;
BOOST_TEST_DECL extern std::string btrt_detect_fp_except;
BOOST_TEST_DECL extern std::string btrt_detect_mem_leaks;
BOOST_TEST_DECL extern std::string btrt_list_content;
BOOST_TEST_DECL extern std::string btrt_list_labels;
BOOST_TEST_DECL extern std::string btrt_log_format;
BOOST_TEST_DECL extern std::string btrt_log_level;
BOOST_TEST_DECL extern std::string btrt_log_sink;
BOOST_TEST_DECL extern std::string btrt_combined_logger;
BOOST_TEST_DECL extern std::string btrt_output_format;
BOOST_TEST_DECL extern std::string btrt_random_seed;
BOOST_TEST_DECL extern std::string btrt_report_format;
BOOST_TEST_DECL extern std::string btrt_report_level;
BOOST_TEST_DECL extern std::string btrt_report_mem_leaks;
BOOST_TEST_DECL extern std::string btrt_report_sink;
BOOST_TEST_DECL extern std::string btrt_result_code;
BOOST_TEST_DECL extern std::string btrt_run_filters;
BOOST_TEST_DECL extern std::string btrt_save_test_pattern;
BOOST_TEST_DECL extern std::string btrt_show_progress;
BOOST_TEST_DECL extern std::string btrt_use_alt_stack;
BOOST_TEST_DECL extern std::string btrt_wait_for_debugger;
BOOST_TEST_DECL extern std::string btrt_help;
BOOST_TEST_DECL extern std::string btrt_usage;
BOOST_TEST_DECL extern std::string btrt_version;

BOOST_TEST_DECL void init( int& argc, char** argv );

// ************************************************************************** //
// **************              runtime_param::get              ************** //
// ************************************************************************** //

/// Access to arguments
BOOST_TEST_DECL runtime::arguments_store const& argument_store();

template<typename T>
inline T const&
get( runtime::cstring parameter_name )
{
    return argument_store().get<T>( parameter_name );
}

inline bool has( runtime::cstring parameter_name )
{
    return argument_store().has( parameter_name );
}

/// For public access
BOOST_TEST_DECL bool save_pattern();

// ************************************************************************** //
// **************                  stream_holder               ************** //
// ************************************************************************** //

class stream_holder {
public:
    // Constructor
    explicit        stream_holder( std::ostream& default_stream = std::cout )
    : m_stream( &default_stream )
    {
    }

    void            setup( const const_string& stream_name,
                           boost::function<void ()> const &cleaner_callback = boost::function<void ()>() )
    {
        if(stream_name.empty())
            return;

        if( stream_name == "stderr" ) {
            m_stream = &std::cerr;
            if(cleaner_callback) {
                m_cleaner = boost::make_shared<callback_cleaner>(cleaner_callback);
            }
            else {
                m_cleaner.reset();
            }
        }
        else if( stream_name == "stdout" ) {
            m_stream = &std::cout;
            if (cleaner_callback) {
                m_cleaner = boost::make_shared<callback_cleaner>(cleaner_callback);
            }
            else {
                m_cleaner.reset();
            }
        }
        else {
            m_cleaner = boost::make_shared<callback_cleaner>(cleaner_callback);
            m_cleaner->m_file.open( std::string(stream_name.begin(), stream_name.end()).c_str() );
            m_stream = &m_cleaner->m_file;
        }
    }

    // Access methods
    std::ostream&   ref() const { return *m_stream; }

private:
    struct callback_cleaner {
        callback_cleaner(boost::function<void ()> cleaner_callback)
        : m_cleaner_callback(cleaner_callback)
        , m_file() {
        }
        ~callback_cleaner() {
            if( m_cleaner_callback )
                m_cleaner_callback();
        }
        boost::function<void ()> m_cleaner_callback;
        std::ofstream m_file;
    };

    // Data members
    boost::shared_ptr<callback_cleaner>   m_cleaner;
    std::ostream*                         m_stream;
};

} // namespace runtime_config
} // namespace unit_test
} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UNIT_TEST_PARAMETERS_HPP_071894GER
