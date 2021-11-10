//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// @brief Defines unit test log formatter interface
///
/// You can define a class with implements this interface and use an instance of it
/// as a Unit Test Framework log formatter
// ***************************************************************************

#ifndef BOOST_TEST_UNIT_TEST_LOG_FORMATTER_HPP_071894GER
#define BOOST_TEST_UNIT_TEST_LOG_FORMATTER_HPP_071894GER

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/detail/log_level.hpp>
#include <boost/test/detail/fwd_decl.hpp>

// STL
#include <iosfwd>
#include <string> // for std::string
#include <iostream>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

// ************************************************************************** //
/// Collection of log entry attributes
// ************************************************************************** //

struct BOOST_TEST_DECL log_entry_data {
    log_entry_data()
    {
        m_file_name.reserve( 200 );
    }

    std::string     m_file_name; ///< log entry file name
    std::size_t     m_line_num;  ///< log entry line number
    log_level       m_level;     ///< log entry level

    void clear()
    {
        m_file_name.erase();
        m_line_num      = 0;
        m_level     = log_nothing;
    }
};

// ************************************************************************** //
/// Collection of log checkpoint attributes
// ************************************************************************** //

struct BOOST_TEST_DECL log_checkpoint_data
{
    const_string    m_file_name; ///< log checkpoint file name
    std::size_t     m_line_num;  ///< log checkpoint file name
    std::string     m_message;   ///< log checkpoint message

    void clear()
    {
        m_file_name.clear();
        m_line_num  = 0;
        m_message   = std::string();
    }
};

// ************************************************************************** //
/// @brief Abstract Unit Test Framework log formatter interface
///
/// During the test module execution Unit Test Framework can report messages about success
/// or failure of assertions, which test suites are being run and more (specifically which
/// messages are reported depends on log level threshold selected by the user).
///
/// All these messages constitute Unit Test Framework log. There are many ways (formats) to present
/// these messages to the user.
///
/// Boost.Test comes with three formats:
/// - Compiler-like log format: intended for human consumption/diagnostic
/// - XML based log format:  intended for processing by automated regression test systems.
/// - JUNIT based log format:  intended for processing by automated regression test systems.
///
/// If you want to produce some other format you need to implement class with specific interface and use
/// method @c unit_test_log_t::set_formatter during a test module initialization to set an active formatter.
/// The class unit_test_log_formatter defines this interface.
///
/// This interface requires you to format all possible messages being produced in the log.
/// These includes error messages about failed assertions, messages about caught exceptions and
/// information messages about test units being started/ended. All the methods in this interface takes
/// a reference to standard stream as a first argument. This is where final messages needs to be directed
/// to. Also you are given all the information necessary to produce a message.
///
/// @par Since Boost 1.62:
/// - Each formatter may indicate the default output stream. This is convenient for instance for streams intended
///   for automated processing that indicate a file. See @c get_default_stream_description for more details.
/// - Each formatter may manage its own log level through the getter/setter @c get_log_level and @c set_log_level .
///
/// @see
/// - boost::unit_test::test_observer for an indication of the calls of the test observer interface
class BOOST_TEST_DECL unit_test_log_formatter {
public:
    /// Types of log entries (messages written into a log)
    enum log_entry_types { BOOST_UTL_ET_INFO,       ///< Information message from the framework
                           BOOST_UTL_ET_MESSAGE,    ///< Information message from the user
                           BOOST_UTL_ET_WARNING,    ///< Warning (non error) condition notification message
                           BOOST_UTL_ET_ERROR,      ///< Non fatal error notification message
                           BOOST_UTL_ET_FATAL_ERROR ///< Fatal error notification message
    };

    //! Constructor
    unit_test_log_formatter()
        : m_log_level(log_all_errors)
    {}

    // Destructor
    virtual             ~unit_test_log_formatter() {}

    // @name Test start/finish

    /// Invoked at the beginning of test module execution
    ///
    /// @param[in] os   output stream to write a messages to
    /// @param[in] test_cases_amount total test case amount to be run
    /// @see log_finish
    virtual void        log_start( std::ostream& os, counter_t test_cases_amount ) = 0;

    /// Invoked at the end of test module execution
    ///
    /// @param[in] os   output stream to write a messages into
    /// @see log_start
    virtual void        log_finish( std::ostream& os ) = 0;

    /// Invoked when Unit Test Framework build information is requested
    ///
    /// @param[in] os               output stream to write a messages into
    /// @param[in] log_build_info   indicates if build info should be logged or not
    virtual void        log_build_info( std::ostream& os, bool log_build_info = true ) = 0;
    // @}

    // @name Test unit start/finish

    /// Invoked when test unit starts (either test suite or test case)
    ///
    /// @param[in] os   output stream to write a messages into
    /// @param[in] tu   test unit being started
    /// @see test_unit_finish
    virtual void        test_unit_start( std::ostream& os, test_unit const& tu ) = 0;

    /// Invoked when test unit finishes
    ///
    /// @param[in] os   output stream to write a messages into
    /// @param[in] tu   test unit being finished
    /// @param[in] elapsed time in microseconds spend executing this test unit
    /// @see test_unit_start
    virtual void        test_unit_finish( std::ostream& os, test_unit const& tu, unsigned long elapsed ) = 0;

    /// Invoked if test unit skipped for any reason
    ///
    /// @param[in] os   output stream to write a messages into
    /// @param[in] tu   skipped test unit
    /// @param[in] reason explanation why was it skipped
    virtual void        test_unit_skipped( std::ostream& os, test_unit const& tu, const_string /* reason */)
    {
        test_unit_skipped( os, tu );
    }

    /// Deprecated version of this interface
    /// @deprecated
    virtual void        test_unit_skipped( std::ostream& /* os */, test_unit const& /* tu */) {}

    /// Invoked when a test unit is aborted
    virtual void        test_unit_aborted( std::ostream& /* os */, test_unit const& /* tu */) {}

    /// Invoked when a test unit times-out
    virtual void        test_unit_timed_out( std::ostream& /* os */, test_unit const& /* tu */) {}


    // @}

    // @name Uncaught exception report

    /// Invoked when Unit Test Framework detects uncaught exception
    ///
    /// The framwork calls this function when an uncaught exception it detected.
    /// This call is followed by context information:
    /// - one call to @c entry_context_start,
    /// - as many calls to @c log_entry_context as there are context entries
    /// - one call to @c entry_context_finish
    ///
    /// The logging of the exception information is finilized by a call to @c log_exception_finish.
    ///
    /// @param[in] os   output stream to write a messages into
    /// @param[in] lcd  information about the last checkpoint before the exception was triggered
    /// @param[in] ex   information about the caught exception
    /// @see log_exception_finish
    virtual void        log_exception_start( std::ostream& os, log_checkpoint_data const& lcd, execution_exception const& ex ) = 0;

    /// Invoked when Unit Test Framework detects uncaught exception
    ///
    /// Call to this function finishes uncaught exception report.
    /// @param[in] os   output stream to write a messages into
    /// @see log_exception_start
    virtual void        log_exception_finish( std::ostream& os ) = 0;
    // @}

    // @name Regular log entry

    /// Invoked by Unit Test Framework to start new log entry

    /// Call to this function starts new log entry. It is followed by series of log_entry_value calls and finally call to log_entry_finish.
    /// A log entry may consist of one or more values being reported. Some of these values will be plain strings, while others can be complicated
    /// expressions in a form of "lazy" expression template lazy_ostream.
    /// @param[in] os   output stream to write a messages into
    /// @param[in] led  log entry attributes
    /// @param[in] let  log entry type log_entry_finish
    /// @see log_entry_value, log_entry_finish
    ///
    /// @note call to this function may happen before any call to test_unit_start or all calls to test_unit_finish as the
    /// framework might log errors raised during global initialization/shutdown.
    virtual void        log_entry_start( std::ostream& os, log_entry_data const& led, log_entry_types let ) = 0;

    /// Invoked by Unit Test Framework to report a log entry content
    ///
    /// This is one of two overloaded methods to report log entry content. This one is used to report plain string value.
    /// @param[in] os   output stream to write a messages into.
    /// @param[in] value log entry string value
    /// @see log_entry_start, log_entry_finish
    virtual void        log_entry_value( std::ostream& os, const_string value ) = 0;

    /// Invoked by Unit Test Framework to report a log entry content

    /// This is one of two overloaded methods to report log entry content. This one is used to report some complicated expression passed as
    /// an expression template lazy_ostream. In most cases default implementation provided by the framework should work as is (it just converts
    /// the lazy expression into a string.
    /// @param[in] os   output stream to write a messages into
    /// @param[in] value log entry "lazy" value
    /// @see log_entry_start, log_entry_finish
    virtual void        log_entry_value( std::ostream& os, lazy_ostream const& value ); // there is a default impl

    /// Invoked by Unit Test Framework to finish a log entry report

    /// @param[in] os   output stream to write a messages into
    /// @see log_entry_start, log_entry_start
    virtual void        log_entry_finish( std::ostream& os ) = 0;
    // @}

    // @name Log entry context report

    /// Invoked by Unit Test Framework to start log entry context report
    //
    /// Unit Test Framework logs for failed assertions and uncaught exceptions context if one was defined by a test module.
    /// Context consists of multiple "scopes" identified by description messages assigned by the test module using
    /// BOOST_TEST_INFO/BOOST_TEST_CONTEXT statements.
    /// @param[in] os   output stream to write a messages into
    /// @param[in] l    entry log_level, to be used to fine tune the message
    /// @see log_entry_context, entry_context_finish
    virtual void        entry_context_start( std::ostream& os, log_level l ) = 0;

    /// Invoked by Unit Test Framework to report log entry context "scope" description
    //
    /// Each "scope" description is reported by separate call to log_entry_context.
    /// @param[in] os   output stream to write a messages into
    /// @param[in] l    entry log_level, to be used to fine tune the message
    /// @param[in] value  context "scope" description
    /// @see log_entry_start, entry_context_finish
    virtual void        log_entry_context( std::ostream& os, log_level l, const_string value ) = 0;

    /// Invoked by Unit Test Framework to finish log entry context report
    ///
    /// @param[in] os   output stream to write a messages into
    /// @param[in] l    entry log_level, to be used to fine tune the message
    /// @see log_entry_start, entry_context_context
    virtual void        entry_context_finish( std::ostream& os, log_level l ) = 0;
    // @}

    // @name Log level management

    /// Sets the log level of the logger/formatter
    ///
    /// Some loggers need to manage the log level by their own. This
    /// member function let the implementation decide of that.
    /// @par Since Boost 1.62
    virtual void        set_log_level(log_level new_log_level);

    /// Returns the log level of the logger/formatter
    /// @par Since Boost 1.62
    virtual log_level   get_log_level() const;
    // @}


    // @name Stream management

    /// Returns a default stream for this logger.
    ///
    /// The returned string describes the stream as if it was passed from
    /// the command line @c "--log_sink" parameter. With that regards, @b stdout and @b stderr
    /// have special meaning indicating the standard output or error stream respectively.
    ///
    /// @par Since Boost 1.62
    virtual std::string  get_default_stream_description() const
    {
        return "stdout";
    }

    // @}


protected:
    log_level           m_log_level;

};

} // namespace unit_test
} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UNIT_TEST_LOG_FORMATTER_HPP_071894GER
