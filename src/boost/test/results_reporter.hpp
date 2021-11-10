//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// @brief defines testing result reporter interfaces
///
/// This file defines interfaces that are responsible for results reporting. Interface is presented in a form of
/// free standing function implemented in namespace result_reporter
// ***************************************************************************

#ifndef BOOST_TEST_RESULTS_REPORTER_HPP_021205GER
#define BOOST_TEST_RESULTS_REPORTER_HPP_021205GER

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/detail/fwd_decl.hpp>

// STL
#include <iosfwd>   // for std::ostream&

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

/// Namespace for results reporter interfaces
namespace results_reporter {

// ************************************************************************** //
/// @brief Results report formatter interface
///
/// This is abstract interface for the report formatter used by results reporter routines.
/// You can define a custom formatter by implementing this interface and setting the formatter using set_format function.
/// This is usually done during test module initialization
// ************************************************************************** //

class BOOST_TEST_DECL format {
public:
    // Destructor
    virtual ~format() {}

    virtual void    results_report_start( std::ostream& ostr ) = 0;
    virtual void    results_report_finish( std::ostream& ostr ) = 0;

    virtual void    test_unit_report_start( test_unit const&, std::ostream& ostr ) = 0;
    virtual void    test_unit_report_finish( test_unit const&, std::ostream& ostr ) = 0;

    virtual void    do_confirmation_report( test_unit const&, std::ostream& ostr ) = 0;
};

// ************************************************************************** //
/// @name report configuration
// ************************************************************************** //

/// Sets reporting level

/// There are only four possible levels for results report:
/// - confirmation report (boost::unit_test::CONFIRMATION_REPORT). This report level only produces short confirmation
///   message about test module pass/fail status
/// - short report (boost::unit_test::SHORT_REPORT). This report level produces short summary report for failed/passed
///   assertions and test units.
/// - detailed report (boost::unit_test::DETAILED_REPORT). This report level produces detailed report per test unit for
///   passed/failed assertions and uncaught exceptions
/// - no report (boost::unit_test::NO_REPORT). This report level produces no results report. This is used for test modules
///   running as part of some kind of continues integration framework
/// @param[in] l report level
BOOST_TEST_DECL void    set_level( report_level l );

/// Sets output stream for results reporting

/// By default std::cerr is used. Use this function to set a different stream. The framework
/// refers to the stream by reference, so you need to make sure the stream object lifetime exceeds the testing main scope.
BOOST_TEST_DECL void    set_stream( std::ostream& );

/// Sets one of the predefined formats

/// The framework implements two results report formats:
/// - plain human readable format (boost::unit_test::OF_CLF)
/// - XML format (boost::unit_test::OF_XML)
/// @param[in] of one of the presefined enumeration values for output formats
BOOST_TEST_DECL void    set_format( output_format of );

/// Sets custom report formatter

/// The framework takes ownership of the pointer passed as an argument. So this should be a pointer to
/// a heap allocated object
/// @param[in] f pointer to heap allocated instance of custom report formatter class
BOOST_TEST_DECL void    set_format( results_reporter::format* f );

/// @brief Access to configured results reporter stream
///
/// Use this stream to report additional information abut test module execution
BOOST_TEST_DECL std::ostream& get_stream();

/// @}

// ************************************************************************** //
// **************               report initiation              ************** //
// ************************************************************************** //

BOOST_TEST_DECL void    make_report( report_level l = INV_REPORT_LEVEL, test_unit_id = INV_TEST_UNIT_ID );
inline void             confirmation_report( test_unit_id id = INV_TEST_UNIT_ID )
{ make_report( CONFIRMATION_REPORT, id ); }
inline void             short_report( test_unit_id id = INV_TEST_UNIT_ID )
{ make_report( SHORT_REPORT, id ); }
inline void             detailed_report( test_unit_id id = INV_TEST_UNIT_ID )
{ make_report( DETAILED_REPORT, id ); }

} // namespace results_reporter
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_RESULTS_REPORTER_HPP_021205GER

