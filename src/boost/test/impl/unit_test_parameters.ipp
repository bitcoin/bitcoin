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
//  Description : simple implementation for Unit Test Framework parameter
//  handling routines. May be rewritten in future to use some kind of
//  command-line arguments parsing facility and environment variable handling
//  facility
// ***************************************************************************

#ifndef BOOST_TEST_UNIT_TEST_PARAMETERS_IPP_012205GER
#define BOOST_TEST_UNIT_TEST_PARAMETERS_IPP_012205GER

// Boost.Test
#include <boost/test/unit_test_parameters.hpp>

#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <boost/test/utils/basic_cstring/compare.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/utils/iterator/token_iterator.hpp>

#include <boost/test/debug.hpp>
#include <boost/test/framework.hpp>

#include <boost/test/detail/log_level.hpp>
#include <boost/test/detail/throw_exception.hpp>

// Boost.Runtime.Param
#include <boost/test/utils/runtime/parameter.hpp>
#include <boost/test/utils/runtime/argument.hpp>
#include <boost/test/utils/runtime/finalize.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>
#include <boost/test/utils/runtime/env/fetch.hpp>

// Boost
#include <boost/config.hpp>
#include <boost/test/detail/suppress_warnings.hpp>
#include <boost/test/detail/enable_warnings.hpp>
#include <boost/cstdlib.hpp>

// STL
#include <cstdlib>
#include <iostream>
#include <fstream>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

# ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::getenv; using ::strncmp; using ::strcmp; }
# endif

namespace boost {
namespace unit_test {

namespace rt = boost::runtime;

// ************************************************************************** //
// **************                 runtime_config               ************** //
// ************************************************************************** //

namespace runtime_config {

// UTF parameters
std::string btrt_auto_start_dbg    = "auto_start_dbg";
std::string btrt_break_exec_path   = "break_exec_path";
std::string btrt_build_info        = "build_info";
std::string btrt_catch_sys_errors  = "catch_system_errors";
std::string btrt_color_output      = "color_output";
std::string btrt_detect_fp_except  = "detect_fp_exceptions";
std::string btrt_detect_mem_leaks  = "detect_memory_leaks";
std::string btrt_list_content      = "list_content";
std::string btrt_list_labels       = "list_labels";
std::string btrt_log_format        = "log_format";
std::string btrt_log_level         = "log_level";
std::string btrt_log_sink          = "log_sink";
std::string btrt_combined_logger   = "logger";
std::string btrt_output_format     = "output_format";
std::string btrt_random_seed       = "random";
std::string btrt_report_format     = "report_format";
std::string btrt_report_level      = "report_level";
std::string btrt_report_mem_leaks  = "report_memory_leaks_to";
std::string btrt_report_sink       = "report_sink";
std::string btrt_result_code       = "result_code";
std::string btrt_run_filters       = "run_test";
std::string btrt_save_test_pattern = "save_pattern";
std::string btrt_show_progress     = "show_progress";
std::string btrt_use_alt_stack     = "use_alt_stack";
std::string btrt_wait_for_debugger = "wait_for_debugger";

std::string btrt_help              = "help";
std::string btrt_usage             = "usage";
std::string btrt_version           = "version";

//____________________________________________________________________________//

namespace {

void
register_parameters( rt::parameters_store& store )
{
    rt::option auto_start_dbg( btrt_auto_start_dbg, (
        rt::description = "Automatically attaches debugger in case of system level failure (signal).",
        rt::env_var = "BOOST_TEST_AUTO_START_DBG",

        rt::help = "Specifies whether Boost.Test should attempt "
                   "to attach a debugger when fatal system error occurs. At the moment this feature "
                   "is only available on a few selected platforms: Win32 and *nix. There is a "
                   "default debugger configured for these platforms. You can manually configure "
                   "different debugger. For more details on how to configure the debugger see the "
                   "Boost.Test debug API, specifically the function boost::debug::set_debugger."
    ));

    auto_start_dbg.add_cla_id( "--", btrt_auto_start_dbg, "=" );
    auto_start_dbg.add_cla_id( "-", "d", " " );
    store.add( auto_start_dbg );

    ///////////////////////////////////////////////

    rt::parameter<std::string> break_exec_path( btrt_break_exec_path, (
        rt::description = "For the exception safety testing allows to break at specific execution path.",
        rt::env_var = "BOOST_TEST_BREAK_EXEC_PATH"
#ifndef BOOST_NO_CXX11_LAMBDAS
        ,
        rt::callback = [](rt::cstring) {
            BOOST_TEST_SETUP_ASSERT( false, "parameter break_exec_path is disabled in this release" );
        }
#endif
    ));

    break_exec_path.add_cla_id( "--", btrt_break_exec_path, "=" );
    store.add( break_exec_path );

    ///////////////////////////////////////////////

    rt::option build_info( btrt_build_info, (
        rt::description = "Displays library build information.",
        rt::env_var = "BOOST_TEST_BUILD_INFO",
        rt::help = "Displays library build information, including: platform, "
                   "compiler, STL version and Boost version."
    ));

    build_info.add_cla_id( "--", btrt_build_info, "=" );
    build_info.add_cla_id( "-", "i", " " );
    store.add( build_info );

    ///////////////////////////////////////////////

    rt::option catch_sys_errors( btrt_catch_sys_errors, (
        rt::description = "Allows to switch between catching and ignoring system errors (signals).",
        rt::env_var = "BOOST_TEST_CATCH_SYSTEM_ERRORS",
        rt::default_value =
#ifdef BOOST_TEST_DEFAULTS_TO_CORE_DUMP
            false,
#else
            true,
#endif
        rt::help = "If option " + btrt_catch_sys_errors + " has value 'no' the frameworks does not attempt to catch "
                   "asynchronous system failure events (signals on *NIX platforms or structured exceptions on Windows). "
                   " Default value is "
#ifdef BOOST_TEST_DEFAULTS_TO_CORE_DUMP
                    "no."
#else
                    "true."
#endif
    ));

    catch_sys_errors.add_cla_id( "--", btrt_catch_sys_errors, "=", true );
    catch_sys_errors.add_cla_id( "-", "s", " " );
    store.add( catch_sys_errors );

    ///////////////////////////////////////////////

    rt::option color_output( btrt_color_output, (
        rt::description = "Enables color output of the framework log and report messages.",
        rt::env_var = "BOOST_TEST_COLOR_OUTPUT",
        rt::default_value = true,
        rt::help = "Produces color output for logs, reports and help. "
                   "Defaults to true. "
    ));

    color_output.add_cla_id( "--", btrt_color_output, "=", true );
    color_output.add_cla_id( "-", "x", " " );
    store.add( color_output );

    ///////////////////////////////////////////////

    rt::option detect_fp_except( btrt_detect_fp_except, (
        rt::description = "Enables/disables floating point exceptions traps.",
        rt::env_var = "BOOST_TEST_DETECT_FP_EXCEPTIONS",
        rt::help = "Enables/disables hardware traps for the floating "
                   "point exceptions (if supported on your platfrom)."
    ));

    detect_fp_except.add_cla_id( "--", btrt_detect_fp_except, "=", true );
    store.add( detect_fp_except );

    ///////////////////////////////////////////////

    rt::parameter<unsigned long> detect_mem_leaks( btrt_detect_mem_leaks, (
        rt::description = "Turns on/off memory leaks detection (optionally breaking on specified alloc order number).",
        rt::env_var = "BOOST_TEST_DETECT_MEMORY_LEAK",
        rt::default_value = 1L,
        rt::optional_value = 1L,
        rt::value_hint = "<alloc order number>",
        rt::help = "Enables/disables memory leaks detection. "
                   "This parameter has optional long integer value. The default value is 1, which "
                   "enables the memory leak detection. The value 0 disables memory leak detection. "
                   "Any value N greater than 1 is treated as leak allocation number and tells the "
                   "framework to setup runtime breakpoint at Nth heap allocation. If value is "
                   "omitted the default value is assumed."
    ));

    detect_mem_leaks.add_cla_id( "--", btrt_detect_mem_leaks, "=" );
    store.add( detect_mem_leaks );

    ///////////////////////////////////////////////

    rt::enum_parameter<unit_test::output_format> list_content( btrt_list_content, (
        rt::description = "Lists the content of test tree - names of all test suites and test cases.",
        rt::env_var = "BOOST_TEST_LIST_CONTENT",
        rt::default_value = OF_INVALID,
        rt::optional_value = OF_CLF,
        rt::enum_values<unit_test::output_format>::value =
#if defined(BOOST_TEST_CLA_NEW_API)
        {
            { "HRF", OF_CLF },
            { "DOT", OF_DOT }
        },
#else
        rt::enum_values_list<unit_test::output_format>()
            ( "HRF", OF_CLF )
            ( "DOT", OF_DOT )
        ,
#endif
        rt::help = "Lists the test suites and cases "
                   "of the test module instead of executing the test cases. The format of the "
                   "desired output can be passed to the command. Currently the "
                   "framework supports two formats: human readable format (HRF) and dot graph "
                   "format (DOT). If value is omitted HRF value is assumed."
    ));
    list_content.add_cla_id( "--", btrt_list_content, "=" );
    store.add( list_content );

    ///////////////////////////////////////////////

    rt::option list_labels( btrt_list_labels, (
        rt::description = "Lists all available labels.",
        rt::env_var = "BOOST_TEST_LIST_LABELS",
        rt::help = "Option " + btrt_list_labels + " instructs the framework to list all the the labels "
                   "defined in the test module instead of executing the test cases."
    ));

    list_labels.add_cla_id( "--", btrt_list_labels, "=" );
    store.add( list_labels );

    ///////////////////////////////////////////////

    rt::enum_parameter<unit_test::output_format> log_format( btrt_log_format, (
        rt::description = "Specifies log format.",
        rt::env_var = "BOOST_TEST_LOG_FORMAT",
        rt::default_value = OF_CLF,
        rt::enum_values<unit_test::output_format>::value =
#if defined(BOOST_TEST_CLA_NEW_API)
        {
            { "HRF", OF_CLF },
            { "CLF", OF_CLF },
            { "XML", OF_XML },
            { "JUNIT", OF_JUNIT },
        },
#else
        rt::enum_values_list<unit_test::output_format>()
            ( "HRF", OF_CLF )
            ( "CLF", OF_CLF )
            ( "XML", OF_XML )
            ( "JUNIT", OF_JUNIT )
        ,
#endif
        rt::help = "Set the frameowrk's log format to one "
                   "of the formats supplied by the framework. The only acceptable values for this "
                   "parameter are the names of the output formats supplied by the framework. By "
                   "default the framework uses human readable format (HRF) for testing log. This "
                   "format is similar to compiler error format. Alternatively you can specify XML "
                   "or JUNIT as log format, which are easier to process by testing automation tools."
    ));

    log_format.add_cla_id( "--", btrt_log_format, "=" );
    log_format.add_cla_id( "-", "f", " " );
    store.add( log_format );

    ///////////////////////////////////////////////

    rt::enum_parameter<unit_test::log_level> log_level( btrt_log_level, (
        rt::description = "Specifies the logging level of the test execution.",
        rt::env_var = "BOOST_TEST_LOG_LEVEL",
        rt::default_value = log_all_errors,
        rt::enum_values<unit_test::log_level>::value =
#if defined(BOOST_TEST_CLA_NEW_API)
        {
            { "all"           , log_successful_tests },
            { "success"       , log_successful_tests },
            { "test_suite"    , log_test_units },
            { "unit_scope"    , log_test_units },
            { "message"       , log_messages },
            { "warning"       , log_warnings },
            { "error"         , log_all_errors },
            { "cpp_exception" , log_cpp_exception_errors },
            { "system_error"  , log_system_errors },
            { "fatal_error"   , log_fatal_errors },
            { "nothing"       , log_nothing }
        },
#else
        rt::enum_values_list<unit_test::log_level>()
            ( "all"           , log_successful_tests )
            ( "success"       , log_successful_tests )
            ( "test_suite"    , log_test_units )
            ( "unit_scope"    , log_test_units )
            ( "message"       , log_messages )
            ( "warning"       , log_warnings )
            ( "error"         , log_all_errors )
            ( "cpp_exception" , log_cpp_exception_errors )
            ( "system_error"  , log_system_errors )
            ( "fatal_error"   , log_fatal_errors )
            ( "nothing"       , log_nothing )
        ,
#endif
        rt::help = "Set the framework's log level. "
                   "The log level defines the verbosity of the testing logs produced by a test "
                   "module. The verbosity ranges from a complete log, when all assertions "
                   "(both successful and failing) are reported, all notifications about "
                   "test units start and finish are included, to an empty log when nothing "
                   "is reported to a testing log stream."
    ));

    log_level.add_cla_id( "--", btrt_log_level, "=" );
    log_level.add_cla_id( "-", "l", " " );
    store.add( log_level );

    ///////////////////////////////////////////////

    rt::parameter<std::string> log_sink( btrt_log_sink, (
        rt::description = "Specifies log sink: stdout (default), stderr or file name.",
        rt::env_var = "BOOST_TEST_LOG_SINK",
        rt::value_hint = "<stderr|stdout|file name>",
        rt::help = "Sets the log sink - the location "
                   "where Boost.Test writes the logs of the test execution. It allows to easily redirect the "
                   "test logs to file or standard streams. By default testing log is "
                   "directed to standard output."
    ));

    log_sink.add_cla_id( "--", btrt_log_sink, "=" );
    log_sink.add_cla_id( "-", "k", " " );
    store.add( log_sink );

    ///////////////////////////////////////////////

    rt::enum_parameter<unit_test::output_format> output_format( btrt_output_format, (
        rt::description = "Specifies output format (both log and report).",
        rt::env_var = "BOOST_TEST_OUTPUT_FORMAT",
        rt::enum_values<unit_test::output_format>::value =
#if defined(BOOST_TEST_CLA_NEW_API)
        {
            { "HRF", OF_CLF },
            { "CLF", OF_CLF },
            { "XML", OF_XML }
        },
#else
        rt::enum_values_list<unit_test::output_format>()
            ( "HRF", OF_CLF )
            ( "CLF", OF_CLF )
            ( "XML", OF_XML )
        ,
#endif
        rt::help = "Combines an effect of " + btrt_report_format +
                   " and " + btrt_log_format + " parameters. If this parameter is specified, "
                   "it overrides the value of other two parameters. This parameter does not "
                   "have a default value. The only acceptable values are string names of "
                   "output formats: HRF - human readable format and XML - XML formats for "
                   "automation tools processing."
    ));

    output_format.add_cla_id( "--", btrt_output_format, "=" );
    output_format.add_cla_id( "-", "o", " " );
    store.add( output_format );

    /////////////////////////////////////////////// combined logger option

    rt::parameter<std::string,rt::REPEATABLE_PARAM> combined_logger( btrt_combined_logger, (
        rt::description = "Specifies log level and sink for one or several log format",
        rt::env_var = "BOOST_TEST_LOGGER",
        rt::value_hint = "log_format,log_level,log_sink[:log_format,log_level,log_sink]",
        rt::help = "Specify one or more logging definition, which include the logger type, level and sink. "
                   "The log format, level and sink follow the same format as for the argument '--" + btrt_log_format +
                   "', '--" + btrt_log_level + "' and '--" + btrt_log_sink + "' respetively. "
                   "This command can take several logging definition separated by a ':', or be repeated "
                   "on the command line."
    ));

    combined_logger.add_cla_id( "--", btrt_combined_logger, "=" );
    store.add( combined_logger );

    ///////////////////////////////////////////////

    rt::parameter<unsigned> random_seed( btrt_random_seed, (
        rt::description = "Allows to switch between sequential and random order of test units execution."
                          " Optionally allows to specify concrete seed for random number generator.",
        rt::env_var = "BOOST_TEST_RANDOM",
        rt::default_value = 0U,
        rt::optional_value = 1U,
        rt::value_hint = "<seed>",
        rt::help = "Instructs the framework to execute the "
                   "test cases in random order. This parameter accepts an optional unsigned "
                   "integer argument. If parameter is specified without the argument value testing "
                   "order is randomized based on current time. Alternatively you can specify "
                   "any positive value greater than 1 and it will be used as random seed for "
                   "the run. "
                   "By default, the test cases are executed in an "
                   "order defined by their declaration and the optional dependencies among the test units."
    ));

    random_seed.add_cla_id( "--", btrt_random_seed, "=" );
    store.add( random_seed );

    ///////////////////////////////////////////////

    rt::enum_parameter<unit_test::output_format> report_format( btrt_report_format, (
        rt::description = "Specifies the test report format.",
        rt::env_var = "BOOST_TEST_REPORT_FORMAT",
        rt::default_value = OF_CLF,
        rt::enum_values<unit_test::output_format>::value =
#if defined(BOOST_TEST_CLA_NEW_API)
        {
            { "HRF", OF_CLF },
            { "CLF", OF_CLF },
            { "XML", OF_XML }
        },
#else
        rt::enum_values_list<unit_test::output_format>()
            ( "HRF", OF_CLF )
            ( "CLF", OF_CLF )
            ( "XML", OF_XML )
        ,
#endif
        rt::help = "Set the framework's report format "
                   "to one of the formats supplied by the framework. The only acceptable values "
                   "for this parameter are the names of the output formats. By default the framework "
                   "uses human readable format (HRF) for results reporting. Alternatively you can "
                   "specify XML as report format. This format is easier to process by testing "
                   "automation tools."
    ));

    report_format.add_cla_id( "--", btrt_report_format, "=" );
    report_format.add_cla_id( "-", "m", " " );
    store.add( report_format );

    ///////////////////////////////////////////////

    rt::enum_parameter<unit_test::report_level> report_level( btrt_report_level, (
        rt::description = "Specifies test report level.",
        rt::env_var = "BOOST_TEST_REPORT_LEVEL",
        rt::default_value = CONFIRMATION_REPORT,
        rt::enum_values<unit_test::report_level>::value =
#if defined(BOOST_TEST_CLA_NEW_API)
        {
            { "confirm",  CONFIRMATION_REPORT },
            { "short",    SHORT_REPORT },
            { "detailed", DETAILED_REPORT },
            { "no",       NO_REPORT }
        },
#else
        rt::enum_values_list<unit_test::report_level>()
            ( "confirm",  CONFIRMATION_REPORT )
            ( "short",    SHORT_REPORT )
            ( "detailed", DETAILED_REPORT )
            ( "no",       NO_REPORT )
        ,
#endif
        rt::help = "Set the verbosity level of the "
                   "result report generated by the testing framework. Use value 'no' to "
                   "disable the results report completely."
    ));

    report_level.add_cla_id( "--", btrt_report_level, "=" );
    report_level.add_cla_id( "-", "r", " " );
    store.add( report_level );

    ///////////////////////////////////////////////

    rt::parameter<std::string> report_mem_leaks( btrt_report_mem_leaks, (
        rt::description = "File where to report memory leaks to.",
        rt::env_var = "BOOST_TEST_REPORT_MEMORY_LEAKS_TO",
        rt::default_value = std::string(),
        rt::value_hint = "<file name>",
        rt::help = "Parameter " + btrt_report_mem_leaks + " allows to specify a file where to report "
                   "memory leaks to. The parameter does not have default value. If it is not specified, "
                   "memory leaks (if any) are reported to the standard error stream."
    ));

    report_mem_leaks.add_cla_id( "--", btrt_report_mem_leaks, "=" );
    store.add( report_mem_leaks );

    ///////////////////////////////////////////////

    rt::parameter<std::string> report_sink( btrt_report_sink, (
        rt::description = "Specifies report sink: stderr(default), stdout or file name.",
        rt::env_var = "BOOST_TEST_REPORT_SINK",
        rt::value_hint = "<stderr|stdout|file name>",
        rt::help = "Sets the result report sink - "
                   "the location where the framework writes the result report to. "
                   "The sink may be a a file or a standard "
                   "stream. The default is 'stderr': the "
                   "standard error stream."
    ));

    report_sink.add_cla_id( "--", btrt_report_sink, "=" );
    report_sink.add_cla_id( "-", "e", " " );
    store.add( report_sink );

    ///////////////////////////////////////////////

    rt::option result_code( btrt_result_code, (
        rt::description = "Disables test modules's result code generation.",
        rt::env_var = "BOOST_TEST_RESULT_CODE",
        rt::default_value = true,
        rt::help = "The 'no' argument value for the parameter " + btrt_result_code + " instructs the "
                   "framework to always return zero result code. This can be used for test programs "
                   "executed within IDE. By default this parameter has value 'yes'."
    ));

    result_code.add_cla_id( "--", btrt_result_code, "=", true );
    result_code.add_cla_id( "-", "c", " " );
    store.add( result_code );

    ///////////////////////////////////////////////

    rt::parameter<std::string,rt::REPEATABLE_PARAM> tests_to_run( btrt_run_filters, (
        rt::description = "Filters which tests to execute.",
        rt::env_var = "BOOST_TEST_RUN_FILTERS",
        rt::value_hint = "<test unit filter>",
        rt::help = "Filters which test units to execute. "
                   "The framework supports both 'selection filters', which allow to select "
                   "which test units to enable from the set of available test units, and 'disabler "
                   "filters', which allow to disable some test units. Boost.test also supports "
                   "enabling/disabling test units at compile time. These settings identify the default "
                   "set of test units to run. Parameter " + btrt_run_filters + " is used to change this default. "
                   "This parameter is repeatable, so you can specify more than one filter if necessary."
    ));

    tests_to_run.add_cla_id( "--", btrt_run_filters, "=" );
    tests_to_run.add_cla_id( "-", "t", " " );
    store.add( tests_to_run );

    ///////////////////////////////////////////////

    rt::option save_test_pattern( btrt_save_test_pattern, (
        rt::description = "Allows to switch between saving or matching test pattern file.",
        rt::env_var = "BOOST_TEST_SAVE_PATTERN",
        rt::help = "Parameter " + btrt_save_test_pattern + " facilitates switching mode of operation for "
                   "testing output streams.\n\nThis parameter serves no particular purpose within the "
                   "framework itself. It can be used by test modules relying on output_test_stream to "
                   "implement testing logic. Default mode is 'match' (false)."
    ));

    save_test_pattern.add_cla_id( "--", btrt_save_test_pattern, "=" );
    store.add( save_test_pattern );

    ///////////////////////////////////////////////

    rt::option show_progress( btrt_show_progress, (
        rt::description = "Turns on progress display.",
        rt::env_var = "BOOST_TEST_SHOW_PROGRESS",
        rt::help = "Instructs the framework to display the progress of the tests. "
                   "This feature is turned off by default."
    ));

    show_progress.add_cla_id( "--", btrt_show_progress, "=" );
    show_progress.add_cla_id( "-", "p", " " );
    store.add( show_progress );

    ///////////////////////////////////////////////

    rt::option use_alt_stack( btrt_use_alt_stack, (
        rt::description = "Turns on/off usage of an alternative stack for signal handling.",
        rt::env_var = "BOOST_TEST_USE_ALT_STACK",
        rt::default_value = true,
        rt::help = "Instructs the framework to use an alternative "
                   "stack for operating system's signals handling (on platforms where this is supported). "
                   "The feature is enabled by default, but can be disabled using this command line switch."
    ));

    use_alt_stack.add_cla_id( "--", btrt_use_alt_stack, "=", true );
    store.add( use_alt_stack );

    ///////////////////////////////////////////////

    rt::option wait_for_debugger( btrt_wait_for_debugger, (
        rt::description = "Forces test module to wait for button to be pressed before starting test run.",
        rt::env_var = "BOOST_TEST_WAIT_FOR_DEBUGGER",
        rt::help = "Instructs the framework to pause before starting "
                   "test units execution, so that you can attach a debugger to the test module process. "
                   "This feature is turned off by default."
    ));

    wait_for_debugger.add_cla_id( "--", btrt_wait_for_debugger, "=" );
    wait_for_debugger.add_cla_id( "-", "w", " " );
    store.add( wait_for_debugger );

    ///////////////////////////////////////////////

    rt::parameter<std::string> help( btrt_help, (
        rt::description = "Help for framework parameters.",
        rt::optional_value = std::string(),
        rt::value_hint = "<parameter name>",
        rt::help = "Displays help on the framework's parameters. "
                   "The parameter accepts an optional argument value. If present, an argument value is "
                   "interpreted as a parameter name (name guessing works as well, so for example "
                   "'--help=rand' displays help on the parameter 'random'). If the parameter name is unknown "
                   "or ambiguous error is reported. If argument value is absent, a summary of all "
                   "framework's parameter is displayed."
    ));
    help.add_cla_id( "--", btrt_help, "=" );
    store.add( help );

    ///////////////////////////////////////////////

    rt::option usage( btrt_usage, (
        rt::description = "Short message explaining usage of Boost.Test parameters."
    ));
    usage.add_cla_id( "-", "?", " " );
    store.add( usage );

    ///////////////////////////////////////////////

    rt::option version( btrt_version, (
        rt::description = "Prints Boost.Test version and exits."
    ));
    version.add_cla_id( "--", btrt_version, " " );
    store.add( version );
}

static rt::arguments_store  s_arguments_store;
static rt::parameters_store s_parameters_store;

//____________________________________________________________________________//

} // local namespace

void
init( int& argc, char** argv )
{
    shared_ptr<rt::cla::parser> parser;

    BOOST_TEST_I_TRY {
        // Initialize parameters list
        if( s_parameters_store.is_empty() )
            register_parameters( s_parameters_store );

        // Clear up arguments store just in case (of multiple init invocations)
        s_arguments_store.clear();

        // Parse CLA they take precedence over  environment
        parser.reset( new rt::cla::parser( s_parameters_store, (rt::end_of_params = "--", rt::negation_prefix = "no_") ) );
        argc = parser->parse( argc, argv, s_arguments_store );

        // Try to fetch missing arguments from environment
        rt::env::fetch_absent( s_parameters_store, s_arguments_store );

        // Set arguments to default values if defined and perform all the validations
        rt::finalize_arguments( s_parameters_store, s_arguments_store );

        // check if colorized output is enabled
        bool use_color = true;
        if( s_arguments_store.has(btrt_color_output ) ) {
            use_color = runtime_config::get<bool>(runtime_config::btrt_color_output);
        }

        // Report help if requested
        if( runtime_config::get<bool>( btrt_version ) ) {
            parser->version( std::cerr );
            BOOST_TEST_I_THROW( framework::nothing_to_test( boost::exit_success ) );
        }
        else if( runtime_config::get<bool>( btrt_usage ) ) {
            parser->usage( std::cerr, runtime::cstring(), use_color );
            BOOST_TEST_I_THROW( framework::nothing_to_test( boost::exit_success ) );
        }
        else if( s_arguments_store.has( btrt_help ) ) {
            parser->help(std::cerr, 
                         s_parameters_store, 
                         runtime_config::get<std::string>( btrt_help ),
                         use_color );
            BOOST_TEST_I_THROW( framework::nothing_to_test( boost::exit_success ) );
        }

        // A bit of business logic: output_format takes precedence over log/report formats
        if( s_arguments_store.has( btrt_output_format ) ) {
            unit_test::output_format of = s_arguments_store.get<unit_test::output_format>( btrt_output_format );
            s_arguments_store.set( btrt_report_format, of );
            s_arguments_store.set( btrt_log_format, of );
        }

    }
    BOOST_TEST_I_CATCH( rt::init_error, ex ) {
        BOOST_TEST_SETUP_ASSERT( false, ex.msg );
    }
    BOOST_TEST_I_CATCH( rt::ambiguous_param, ex ) {
        std::cerr << ex.msg << "\n Did you mean one of these?\n";

        BOOST_TEST_FOREACH( rt::cstring, name, ex.m_amb_candidates )
            std::cerr << "   " << name << "\n";

        BOOST_TEST_I_THROW( framework::nothing_to_test( boost::exit_exception_failure ) );
    }
    BOOST_TEST_I_CATCH( rt::unrecognized_param, ex ) {
        std::cerr << ex.msg << "\n";

        if( !ex.m_typo_candidates.empty() ) {
            std::cerr << " Did you mean one of these?\n";

            BOOST_TEST_FOREACH( rt::cstring, name, ex.m_typo_candidates )
                std::cerr << "   " << name << "\n";
        }
        else if( parser ) {
            std::cerr << "\n";
            parser->usage( std::cerr );
        }

        BOOST_TEST_I_THROW( framework::nothing_to_test( boost::exit_exception_failure ) );
    }
    BOOST_TEST_I_CATCH( rt::input_error, ex ) {
        std::cerr << ex.msg << "\n\n";

        if( parser )
            parser->usage( std::cerr, ex.param_name );

        BOOST_TEST_I_THROW( framework::nothing_to_test( boost::exit_exception_failure ) );
    }
}

//____________________________________________________________________________//

rt::arguments_store const&
argument_store()
{
    return s_arguments_store;
}

//____________________________________________________________________________//

bool
save_pattern()
{
    return runtime_config::get<bool>( btrt_save_test_pattern );
}

//____________________________________________________________________________//

} // namespace runtime_config
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UNIT_TEST_PARAMETERS_IPP_012205GER
