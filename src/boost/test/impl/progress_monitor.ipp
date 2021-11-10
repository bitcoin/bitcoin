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
//  Description : implements simple text based progress monitor
// ***************************************************************************

#ifndef BOOST_TEST_PROGRESS_MONITOR_IPP_020105GER
#define BOOST_TEST_PROGRESS_MONITOR_IPP_020105GER

// Boost.Test
#include <boost/test/progress_monitor.hpp>
#include <boost/test/unit_test_parameters.hpp>

#include <boost/test/utils/setcolor.hpp>

#include <boost/test/tree/test_unit.hpp>
#include <boost/test/tree/test_case_counter.hpp>
#include <boost/test/tree/traverse.hpp>

// Boost
#include <boost/scoped_ptr.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

// ************************************************************************** //
// **************                progress_monitor              ************** //
// ************************************************************************** //

struct progress_display {
    progress_display( counter_t expected_count, std::ostream& os )
    : m_os(os)
    , m_count( 0 )
    , m_expected_count( expected_count )
    , m_next_tic_count( 0 )
    , m_tic( 0 )
    {

        m_os << "\n0%   10   20   30   40   50   60   70   80   90   100%"
             << "\n|----|----|----|----|----|----|----|----|----|----|"
             << std::endl;

        if( !m_expected_count )
            m_expected_count = 1;  // prevent divide by zero
    }

    unsigned long  operator+=( unsigned long increment )
    {
        if( (m_count += increment) < m_next_tic_count )
            return m_count;

        // use of floating point ensures that both large and small counts
        // work correctly.  static_cast<>() is also used several places
        // to suppress spurious compiler warnings.
        unsigned int tics_needed =  static_cast<unsigned int>(
            (static_cast<double>(m_count)/m_expected_count)*50.0 );

        do {
            m_os << '*' << std::flush;
        } while( ++m_tic < tics_needed );

        m_next_tic_count = static_cast<unsigned long>((m_tic/50.0) * m_expected_count);

        if( m_count == m_expected_count ) {
            if( m_tic < 51 )
                m_os << '*';

            m_os << std::endl;
        }

        return m_count;
    }
    unsigned long   operator++()           { return operator+=( 1 ); }
    unsigned long   count() const          { return m_count; }

private:
    BOOST_DELETED_FUNCTION(progress_display(progress_display const&))
    BOOST_DELETED_FUNCTION(progress_display& operator=(progress_display const&))

    std::ostream&   m_os;  // may not be present in all imps

    unsigned long   m_count;
    unsigned long   m_expected_count;
    unsigned long   m_next_tic_count;
    unsigned int    m_tic;
};

namespace {

struct progress_monitor_impl {
    // Constructor
    progress_monitor_impl()
    : m_stream( &std::cout )
    , m_color_output( false )
    {
    }

    std::ostream*                   m_stream;
    scoped_ptr<progress_display>    m_progress_display;
    bool                            m_color_output;
};

progress_monitor_impl& s_pm_impl() { static progress_monitor_impl the_inst; return the_inst; }

#define PM_SCOPED_COLOR() \
    BOOST_TEST_SCOPE_SETCOLOR( s_pm_impl().m_color_output, *s_pm_impl().m_stream, term_attr::BRIGHT, term_color::MAGENTA )

} // local namespace

//____________________________________________________________________________//

BOOST_TEST_SINGLETON_CONS_IMPL(progress_monitor_t)

//____________________________________________________________________________//

void
progress_monitor_t::test_start( counter_t test_cases_amount, test_unit_id )
{
    s_pm_impl().m_color_output = runtime_config::get<bool>( runtime_config::btrt_color_output );

    PM_SCOPED_COLOR();

    s_pm_impl().m_progress_display.reset( new progress_display( test_cases_amount, *s_pm_impl().m_stream ) );
}

//____________________________________________________________________________//

void
progress_monitor_t::test_aborted()
{
    PM_SCOPED_COLOR();

    (*s_pm_impl().m_progress_display) += s_pm_impl().m_progress_display->count();
}

//____________________________________________________________________________//

void
progress_monitor_t::test_unit_finish( test_unit const& tu, unsigned long )
{
    PM_SCOPED_COLOR();

    if( tu.p_type == TUT_CASE )
        ++(*s_pm_impl().m_progress_display);
}

//____________________________________________________________________________//

void
progress_monitor_t::test_unit_skipped( test_unit const& tu, const_string /*reason*/ )
{
    PM_SCOPED_COLOR();

    test_case_counter tcc;
    traverse_test_tree( tu, tcc );

    (*s_pm_impl().m_progress_display) += tcc.p_count;
}

//____________________________________________________________________________//

void
progress_monitor_t::set_stream( std::ostream& ostr )
{
    s_pm_impl().m_stream = &ostr;
}

//____________________________________________________________________________//

#undef PM_SCOPED_COLOR

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_PROGRESS_MONITOR_IPP_020105GER
