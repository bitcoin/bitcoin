//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Test results collecting facility.
///
// ***************************************************************************

#ifndef BOOST_TEST_RESULTS_COLLECTOR_IPP_021105GER
#define BOOST_TEST_RESULTS_COLLECTOR_IPP_021105GER

// Boost.Test
#include <boost/test/unit_test_log.hpp>
#include <boost/test/results_collector.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/execution_monitor.hpp>

#include <boost/test/tree/test_unit.hpp>
#include <boost/test/tree/visitor.hpp>
#include <boost/test/tree/test_case_counter.hpp>
#include <boost/test/tree/traverse.hpp>

// Boost
#include <boost/cstdlib.hpp>

// STL
#include <map>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

// ************************************************************************** //
// **************                 test_results                 ************** //
// ************************************************************************** //

test_results::test_results()
{
    clear();
}

//____________________________________________________________________________//

bool
test_results::passed() const
{
    // if it is skipped, it is not passed. However, if any children is not failed/aborted
    // then their skipped status is not taken into account.
    return  !p_skipped                                  &&
            p_test_cases_failed == 0                    &&
            p_assertions_failed <= p_expected_failures  &&
            // p_test_cases_skipped == 0                   &&
            !p_timed_out                                 &&
            p_test_cases_timed_out == 0                  &&
            !aborted();
}

//____________________________________________________________________________//

bool
test_results::aborted() const
{
    return  p_aborted;
}

//____________________________________________________________________________//

bool
test_results::skipped() const
{
    return  p_skipped;
}

//____________________________________________________________________________//

int
test_results::result_code() const
{
    return passed() ? exit_success
           : ( (p_assertions_failed > p_expected_failures || p_skipped || p_timed_out || p_test_cases_timed_out )
                    ? exit_test_failure
                    : exit_exception_failure );
}

//____________________________________________________________________________//

void
test_results::operator+=( test_results const& tr )
{
    p_test_suites.value         += tr.p_test_suites;
    p_assertions_passed.value   += tr.p_assertions_passed;
    p_assertions_failed.value   += tr.p_assertions_failed;
    p_warnings_failed.value     += tr.p_warnings_failed;
    p_test_cases_passed.value   += tr.p_test_cases_passed;
    p_test_cases_warned.value   += tr.p_test_cases_warned;
    p_test_cases_failed.value   += tr.p_test_cases_failed;
    p_test_cases_skipped.value  += tr.p_test_cases_skipped;
    p_test_cases_aborted.value  += tr.p_test_cases_aborted;
    p_test_cases_timed_out.value += tr.p_test_cases_timed_out;
    p_test_suites_timed_out.value += tr.p_test_suites_timed_out;
    p_duration_microseconds.value += tr.p_duration_microseconds;
}

//____________________________________________________________________________//

void
test_results::clear()
{
    p_test_suites.value         = 0;
    p_assertions_passed.value   = 0;
    p_assertions_failed.value   = 0;
    p_warnings_failed.value     = 0;
    p_expected_failures.value   = 0;
    p_test_cases_passed.value   = 0;
    p_test_cases_warned.value   = 0;
    p_test_cases_failed.value   = 0;
    p_test_cases_skipped.value  = 0;
    p_test_cases_aborted.value  = 0;
    p_test_cases_timed_out.value = 0;
    p_test_suites_timed_out.value = 0;
    p_duration_microseconds.value= 0;
    p_aborted.value             = false;
    p_skipped.value             = false;
    p_timed_out.value           = false;
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************               results_collector              ************** //
// ************************************************************************** //

namespace {

struct results_collector_impl {
    std::map<test_unit_id,test_results> m_results_store;
};

results_collector_impl& s_rc_impl() { static results_collector_impl the_inst; return the_inst; }

// deletes the entries of results_collector_impl
class clear_subtree_result : public test_tree_visitor {
public:
    clear_subtree_result(results_collector_impl& store)
    : m_store( store )
    {}

private:
    bool visit( test_unit const& tu) BOOST_OVERRIDE
    {
      typedef std::map<test_unit_id,test_results>::iterator iterator;
      iterator found = m_store.m_results_store.find(tu.p_id);
      if(found != m_store.m_results_store.end()) {
        m_store.m_results_store.erase( found );
      }
      return true;
    }

    results_collector_impl& m_store;
};

} // local namespace

//____________________________________________________________________________//

BOOST_TEST_SINGLETON_CONS_IMPL( results_collector_t )

//____________________________________________________________________________//

void
results_collector_t::test_start( counter_t, test_unit_id id )
{
    // deletes the results under id only
    clear_subtree_result tree_clear(s_rc_impl());
    traverse_test_tree( id, tree_clear );
}

//____________________________________________________________________________//

void
results_collector_t::test_unit_start( test_unit const& tu )
{
    // init test_results entry
    test_results& tr = s_rc_impl().m_results_store[tu.p_id];

    tr.clear();

    tr.p_expected_failures.value = tu.p_expected_failures;
}

//____________________________________________________________________________//

class results_collect_helper : public test_tree_visitor {
public:
    explicit results_collect_helper( test_results& tr, test_unit const& ts ) : m_tr( tr ), m_ts( ts ) {}

    void    visit( test_case const& tc ) BOOST_OVERRIDE
    {
        test_results const& tr = results_collector.results( tc.p_id );
        m_tr += tr;

        if( tr.passed() ) {
            if( tr.p_warnings_failed )
                m_tr.p_test_cases_warned.value++;
            else
                m_tr.p_test_cases_passed.value++;
        }
        else if( tr.p_timed_out ) {
            m_tr.p_test_cases_timed_out.value++;
        }
        else if( tr.p_skipped || !tc.is_enabled() ) {
            m_tr.p_test_cases_skipped.value++;
        }
        else {
            if( tr.p_aborted )
                m_tr.p_test_cases_aborted.value++;

            m_tr.p_test_cases_failed.value++;
        }
    }
    bool    test_suite_start( test_suite const& ts ) BOOST_OVERRIDE
    {
        if( m_ts.p_id == ts.p_id )
            return true;

        m_tr += results_collector.results( ts.p_id );
        m_tr.p_test_suites.value++;

        if( results_collector.results( ts.p_id ).p_timed_out )
            m_tr.p_test_suites_timed_out.value++;
        return false;
    }

private:
    // Data members
    test_results&       m_tr;
    test_unit const&    m_ts;
};

//____________________________________________________________________________//

void
results_collector_t::test_unit_finish( test_unit const& tu, unsigned long elapsed_in_microseconds )
{
    test_results & tr = s_rc_impl().m_results_store[tu.p_id];
    if( tu.p_type == TUT_SUITE ) {
        results_collect_helper ch( tr, tu );
        traverse_test_tree( tu, ch, true ); // true to ignore the status: we need to count the skipped/disabled tests
    }
    else {
        bool num_failures_match = tr.p_aborted || tr.p_assertions_failed >= tr.p_expected_failures;
        if( !num_failures_match )
            BOOST_TEST_FRAMEWORK_MESSAGE( "Test case " << tu.full_name() << " has fewer failures than expected" );

        bool check_any_assertions = tr.p_aborted || (tr.p_assertions_failed != 0) || (tr.p_assertions_passed != 0);
        if( !check_any_assertions )
            BOOST_TEST_FRAMEWORK_MESSAGE( "Test case " << tu.full_name() << " did not check any assertions" );
    }
    tr.p_duration_microseconds.value = elapsed_in_microseconds;
}

//____________________________________________________________________________//

void
results_collector_t::test_unit_skipped( test_unit const& tu, const_string /*reason*/ )
{
    test_results& tr = s_rc_impl().m_results_store[tu.p_id];
    tr.clear();

    tr.p_skipped.value = true;

    if( tu.p_type == TUT_SUITE ) {
        test_case_counter tcc(true);
        traverse_test_tree( tu, tcc, true ); // true because need to count the disabled tests/units

        tr.p_test_cases_skipped.value = tcc.p_count;
    }
}

//____________________________________________________________________________//

void
results_collector_t::test_unit_timed_out(test_unit const& tu)
{
    test_results& tr = s_rc_impl().m_results_store[tu.p_id];
    tr.p_timed_out.value = true;
}

//____________________________________________________________________________//

void
results_collector_t::assertion_result( unit_test::assertion_result ar )
{
    test_results& tr = s_rc_impl().m_results_store[framework::current_test_case_id()];

    switch( ar ) {
    case AR_PASSED: tr.p_assertions_passed.value++; break;
    case AR_FAILED: tr.p_assertions_failed.value++; break;
    case AR_TRIGGERED: tr.p_warnings_failed.value++; break;
    }

    if( tr.p_assertions_failed == 1 )
        first_failed_assertion();
}

//____________________________________________________________________________//

void
results_collector_t::exception_caught( execution_exception const& ex)
{
    test_results& tr = s_rc_impl().m_results_store[framework::current_test_case_id()];

    tr.p_assertions_failed.value++;
    if( ex.code() == execution_exception::timeout_error ) {
        tr.p_timed_out.value = true;
    }
}

//____________________________________________________________________________//

void
results_collector_t::test_unit_aborted( test_unit const& tu )
{
    s_rc_impl().m_results_store[tu.p_id].p_aborted.value = true;
}

//____________________________________________________________________________//

test_results const&
results_collector_t::results( test_unit_id id ) const
{
    return s_rc_impl().m_results_store[id];
}

//____________________________________________________________________________//

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_RESULTS_COLLECTOR_IPP_021105GER
