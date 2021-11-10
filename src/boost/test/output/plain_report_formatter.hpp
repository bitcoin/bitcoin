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
//  Description : plain report formatter implementation
// ***************************************************************************

#ifndef BOOST_TEST_PLAIN_REPORT_FORMATTER_HPP_020105GER
#define BOOST_TEST_PLAIN_REPORT_FORMATTER_HPP_020105GER

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/results_reporter.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace output {

// ************************************************************************** //
// **************             plain_report_formatter           ************** //
// ************************************************************************** //

class plain_report_formatter : public results_reporter::format {
public:
    plain_report_formatter() : m_indent( 0 ), m_color_output( false ) {}

    // Formatter interface
    void    results_report_start( std::ostream& ostr ) BOOST_OVERRIDE;
    void    results_report_finish( std::ostream& ostr ) BOOST_OVERRIDE;

    void    test_unit_report_start( test_unit const&, std::ostream& ostr ) BOOST_OVERRIDE;
    void    test_unit_report_finish( test_unit const&, std::ostream& ostr ) BOOST_OVERRIDE;

    void    do_confirmation_report( test_unit const&, std::ostream& ostr ) BOOST_OVERRIDE;

private:
    // Data members
    counter_t m_indent;
    bool    m_color_output;
};

} // namespace output
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_PLAIN_REPORT_FORMATTER_HPP_020105GER
