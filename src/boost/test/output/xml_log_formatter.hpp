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
//  Description : contains OF_XML Log formatter definition
// ***************************************************************************

#ifndef BOOST_TEST_XML_LOG_FORMATTER_020105GER
#define BOOST_TEST_XML_LOG_FORMATTER_020105GER

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/unit_test_log_formatter.hpp>

// STL
#include <cstddef> // std::size_t

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace output {

// ************************************************************************** //
// **************               xml_log_formatter              ************** //
// ************************************************************************** //

class xml_log_formatter : public unit_test_log_formatter {
public:
    // Formatter interface
    void    log_start( std::ostream&, counter_t test_cases_amount ) BOOST_OVERRIDE;
    void    log_finish( std::ostream& ) BOOST_OVERRIDE;
    void    log_build_info( std::ostream&, bool ) BOOST_OVERRIDE;

    void    test_unit_start( std::ostream&, test_unit const& tu ) BOOST_OVERRIDE;
    void    test_unit_finish( std::ostream&, test_unit const& tu, unsigned long elapsed ) BOOST_OVERRIDE;
    void    test_unit_skipped( std::ostream&, test_unit const& tu, const_string reason ) BOOST_OVERRIDE;

    void    log_exception_start( std::ostream&, log_checkpoint_data const&, execution_exception const& ex ) BOOST_OVERRIDE;
    void    log_exception_finish( std::ostream& ) BOOST_OVERRIDE;

    void    log_entry_start( std::ostream&, log_entry_data const&, log_entry_types let ) BOOST_OVERRIDE;
    using   unit_test_log_formatter::log_entry_value; // bring base class functions into overload set
    void    log_entry_value( std::ostream&, const_string value ) BOOST_OVERRIDE;
    void    log_entry_finish( std::ostream& ) BOOST_OVERRIDE;

    void    entry_context_start( std::ostream&, log_level ) BOOST_OVERRIDE;
    void    log_entry_context( std::ostream&, log_level, const_string ) BOOST_OVERRIDE;
    void    entry_context_finish( std::ostream&, log_level ) BOOST_OVERRIDE;

private:
    // Data members
    const_string    m_curr_tag;
    bool            m_value_closed;
};

} // namespace output
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_XML_LOG_FORMATTER_020105GER
