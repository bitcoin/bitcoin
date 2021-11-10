//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// @brief Defines testing result collector components
///
/// Defines classes for keeping track (@ref test_results) and collecting
/// (@ref results_collector_t) the states of the test units.
// ***************************************************************************

#ifndef BOOST_TEST_RESULTS_COLLECTOR_HPP_071894GER
#define BOOST_TEST_RESULTS_COLLECTOR_HPP_071894GER

// Boost.Test
#include <boost/test/tree/observer.hpp>

#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/detail/fwd_decl.hpp>

#include <boost/test/utils/class_properties.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

namespace {

// ************************************************************************** //
/// First failed assertion debugger hook
///
/// This function is a placeholder where user can set a breakpoint in debugger to catch the
/// very first assertion failure in each test case
// ************************************************************************** //
inline void first_failed_assertion() {}
}

// ************************************************************************** //
/// @brief Collection of attributes constituting test unit results
///
/// This class is a collection of attributes describing a test result.
///
/// The attributes presented as public properties on
/// an instance of the class. In addition summary conclusion methods are presented to generate simple answer to pass/fail question

class BOOST_TEST_DECL test_results {
public:
    test_results();

    /// Type representing counter like public property
    typedef BOOST_READONLY_PROPERTY( counter_t, (results_collector_t)
                                                (test_results)
                                                (results_collect_helper) ) counter_prop;
    /// Type representing boolean like public property
    typedef BOOST_READONLY_PROPERTY( bool,      (results_collector_t)
                                                (test_results)
                                                (results_collect_helper) ) bool_prop;

    counter_prop    p_test_suites;              //!< Number of test suites
    counter_prop    p_assertions_passed;        //!< Number of successful assertions
    counter_prop    p_assertions_failed;        //!< Number of failing assertions
    counter_prop    p_warnings_failed;          //!< Number of warnings
    counter_prop    p_expected_failures;
    counter_prop    p_test_cases_passed;        //!< Number of successfull test cases
    counter_prop    p_test_cases_warned;        //!< Number of warnings in test cases
    counter_prop    p_test_cases_failed;        //!< Number of failing test cases
    counter_prop    p_test_cases_skipped;       //!< Number of skipped test cases
    counter_prop    p_test_cases_aborted;       //!< Number of aborted test cases
    counter_prop    p_test_cases_timed_out;     //!< Number of timed out test cases
    counter_prop    p_test_suites_timed_out;    //!< Number of timed out test suites
    counter_prop    p_duration_microseconds;    //!< Duration of the test in microseconds
    bool_prop       p_aborted;                  //!< Indicates that the test unit execution has been aborted
    bool_prop       p_skipped;                  //!< Indicates that the test unit execution has been skipped
    bool_prop       p_timed_out;                //!< Indicates that the test unit has timed out

    /// Returns true if test unit passed
    bool            passed() const;

    /// Returns true if test unit skipped
    ///
    /// For test suites, this indicates if the test suite itself has been marked as
    /// skipped, and not if the test suite contains any skipped test.
    bool            skipped() const;

    /// Returns true if the test unit was aborted (hard failure)
    bool            aborted() const;

    /// Produces result code for the test unit execution
    ///
    /// This methhod return one of the result codes defined in @c boost/cstdlib.hpp
    /// @returns
    ///   - @c boost::exit_success on success,
    ///   - @c boost::exit_exception_failure in case test unit
    ///     was aborted for any reason (including uncaught exception)
    ///   - and @c boost::exit_test_failure otherwise
    int             result_code() const;

    //! Combines the results of the current instance with another
    //!
    //! Only the counters are updated and the @c p_aborted and @c p_skipped are left unchanged.
    void            operator+=( test_results const& );

    //! Resets the current state of the result
    void            clear();
};

// ************************************************************************** //
/// @brief Collects and combines the test results
///
/// This class collects and combines the results of the test unit during the execution of the
/// test tree. The results_collector_t::results() function combines the test results on a subtree
/// of the test tree.
///
/// @see boost::unit_test::test_observer
class BOOST_TEST_DECL results_collector_t : public test_observer {
public:

    void        test_start( counter_t, test_unit_id ) BOOST_OVERRIDE;

    void        test_unit_start( test_unit const& ) BOOST_OVERRIDE;
    void        test_unit_finish( test_unit const&, unsigned long ) BOOST_OVERRIDE;
    void        test_unit_skipped( test_unit const&, const_string ) BOOST_OVERRIDE;
    void        test_unit_aborted( test_unit const& ) BOOST_OVERRIDE;
    void        test_unit_timed_out( test_unit const& ) BOOST_OVERRIDE;

    void        assertion_result( unit_test::assertion_result ) BOOST_OVERRIDE;
    void        exception_caught( execution_exception const& ) BOOST_OVERRIDE;

    int         priority() BOOST_OVERRIDE { return 3; }

    /// Results access per test unit
    ///
    /// @param[in] tu_id id of a test unit
    test_results const& results( test_unit_id tu_id ) const;

    /// Singleton pattern
    BOOST_TEST_SINGLETON_CONS( results_collector_t )
};

BOOST_TEST_SINGLETON_INST( results_collector )

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_RESULTS_COLLECTOR_HPP_071894GER
