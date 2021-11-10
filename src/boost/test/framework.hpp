//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief Defines Unit Test Framework mono-state interfaces.
//! The framework interfaces are based on Monostate design pattern.
// ***************************************************************************

#ifndef BOOST_TEST_FRAMEWORK_HPP_020805GER
#define BOOST_TEST_FRAMEWORK_HPP_020805GER

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/detail/fwd_decl.hpp>
#include <boost/test/detail/throw_exception.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

// STL
#include <stdexcept>

//____________________________________________________________________________//

namespace boost {

/// Main namespace for the Unit Test Framework interfaces and implementation
namespace unit_test {

// ************************************************************************** //
// **************              init_unit_test_func             ************** //
// ************************************************************************** //

/// Test module initialization routine signature

/// Different depending on whether BOOST_TEST_ALTERNATIVE_INIT_API is defined or not
#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
typedef bool        (*init_unit_test_func)();
#else
typedef test_suite* (*init_unit_test_func)( int, char* [] );
#endif

// ************************************************************************** //
// **************                   framework                  ************** //
// ************************************************************************** //

/// Namespace of the Unit Test Framework mono-state
namespace framework {

/// @name Unit Test Framework initialization and shutdown
/// @{

/// @brief This function performs initialization of the framework mono-state.
///
/// It needs to be called every time before the test is started.
/// @param[in] init_func test module initialization routine
/// @param[in] argc command line arguments collection
/// @param[in] argv command line arguments collection
BOOST_TEST_DECL void                init( init_unit_test_func init_func, int argc, char* argv[] );

/// This function applies all the decorators and figures out default run status. This argument facilitates an
/// ability of the test cases to prepare some other test units (primarily used internally for self testing).
/// @param[in] tu Optional id of the test unit representing root of test tree. If absent, master test suite is used
BOOST_TEST_DECL void                finalize_setup_phase( test_unit_id tu = INV_TEST_UNIT_ID);

/// This function returns true when testing is in progress (setup is finished).
BOOST_TEST_DECL bool                test_in_progress();

/// This function shuts down the framework and clears up its mono-state.
///
/// It needs to be at the very end of test module execution
BOOST_TEST_DECL void                shutdown();
/// @}

/// @name Test unit registration
/// @{

/// Provides both read and write access to current "leaf" auto test suite during the test unit registration phase.
///
/// During auto-registration phase the framework maintain a FIFO queue of test units being registered. New test units become children
/// of the current "leaf" test suite and if this is test suite it is pushed back into queue and becomes a new leaf.
/// When test suite registration is completed, a test suite is popped from the back of the queue. Only automatically registered test suites
/// should be added to this queue. Master test suite is always a zero element in this queue, so if no other test suites are registered
/// all test cases are added to master test suite.

/// This function facilitates all three possible actions:
///    - if no argument are provided it returns the current queue leaf test suite
///    - if test suite is provided and no second argument are set, test suite is added to the queue
///    - if no test suite are provided and last argument is false, the semantic of this function is similar to queue pop: last element is popped from the queue
/// @param[in] ts test suite to push back to the queue
/// @param[in] push_or_pop should we push ts to the queue or pop leaf test suite instead
/// @returns a reference to the currently active/"leaf" test suite
BOOST_TEST_DECL test_suite&         current_auto_test_suite( test_suite* ts = 0, bool push_or_pop = true );

/// This function add new test case into the global collection of test units the framework aware of.

/// This function also assignes unique test unit id for every test case. Later on one can use this id to locate
/// the test case if necessary. This is the way for the framework to maintain weak references between test units.
/// @param[in]  tc  test case to register
BOOST_TEST_DECL void                register_test_unit( test_case* tc );

/// This function add new test suite into the global collection of test units the framework aware of.

/// This function also assignes unique test unit id for every test suite. Later on one can use this id to locate
/// the test case if necessary. This is the way for the framework to maintain weak references between test units.
/// @param[in]  ts  test suite to register
BOOST_TEST_DECL void                register_test_unit( test_suite* ts );

/// This function removes the test unit from the collection of known test units and destroys the test unit object.

/// This function also assigns unique test unit id for every test case. Later on one can use this id to located
/// the test case if necessary. This is the way for the framework to maintain weak references between test units.
/// @param[in]  tu  test unit to deregister
BOOST_TEST_DECL void                deregister_test_unit( test_unit* tu );

// This function clears up the framework mono-state.

/// After this call the framework can be reinitialized to perform a second test run during the same program lifetime.
BOOST_TEST_DECL void                clear();
/// @}

/// @name Test observer registration
/// @{
/// Adds new test execution observer object into the framework's list of test observers.

/// Observer lifetime should exceed the the testing execution timeframe
/// @param[in]  to  test observer object to add
BOOST_TEST_DECL void                register_observer( test_observer& to );

/// Excludes the observer object form the framework's list of test observers
/// @param[in]  to  test observer object to exclude
BOOST_TEST_DECL void                deregister_observer( test_observer& to );

/// @}

/// @name Global fixtures registration
/// @{

/// Adds a new global fixture to be setup before any other tests starts and tore down after
/// any other tests finished.
/// Test unit fixture lifetime should exceed the testing execution timeframe
/// @param[in]  tuf  fixture to add
BOOST_TEST_DECL void                register_global_fixture( global_fixture& tuf );

/// Removes a test global fixture from the framework
///
/// Test unit fixture lifetime should exceed the testing execution timeframe
/// @param[in]  tuf  fixture to remove
BOOST_TEST_DECL void                deregister_global_fixture( global_fixture& tuf );
/// @}

/// @name Assertion/uncaught exception context support
/// @{
/// Context accessor
struct BOOST_TEST_DECL context_generator {
    context_generator() : m_curr_frame( 0 ) {}

    /// Is there any context?
    bool            is_empty() const;

    /// Give me next frame; empty - last frame
    const_string    next() const;

private:
    // Data members
    mutable unsigned m_curr_frame;
};

/// Records context frame message.

/// Some context frames are sticky - they can only explicitly cleared by specifying context id. Other (non sticky) context frames cleared after every assertion.
/// @param[in] context_descr context frame message
/// @param[in] sticky is this sticky frame or not
/// @returns id of the newly created frame
BOOST_TEST_DECL int                 add_context( lazy_ostream const& context_descr, bool sticky );
/// Erases context frame (when test exits context scope)

/// If context_id is passed clears that specific context frame identified by this id, otherwise clears all non sticky contexts.
BOOST_TEST_DECL void                clear_context( int context_id = -1 );
/// Produces an instance of small "delegate" object, which facilitates access to collected context.
BOOST_TEST_DECL context_generator   get_context();
/// @}

/// @name Access to registered test units.
/// @{
/// This function provides access to the master test suite.

/// There is only only master test suite per test module.
/// @returns a reference the master test suite instance
BOOST_TEST_DECL master_test_suite_t& master_test_suite();

/// This function provides an access to the test unit currently being executed.

/// The difference with current_test_case is about the time between a test-suite
/// is being set up or torn down (fixtures) and when the test-cases of that suite start.

/// This function is only valid during test execution phase.
/// @see current_test_case_id, current_test_case
BOOST_TEST_DECL test_unit const&    current_test_unit();

/// This function provides an access to the test case currently being executed.

/// This function is only valid during test execution phase.
/// @see current_test_case_id
BOOST_TEST_DECL test_case const&    current_test_case();

/// This function provides an access to an id of the test case currently being executed.

/// This function safer than current_test_case, cause if wont throw if no test case is being executed.
/// @see current_test_case
BOOST_TEST_DECL test_unit_id        current_test_case_id(); /* safe version of above */

/// This function provides access to a test unit by id and type combination. It will throw if no test unit located.
/// @param[in]  tu_id    id of a test unit to locate
/// @param[in]  tu_type  type of a test unit to locate
/// @returns located test unit
BOOST_TEST_DECL test_unit&          get( test_unit_id tu_id, test_unit_type tu_type );

/// This function template provides access to a typed test unit by id

/// It will throw if you specify incorrect test unit type
/// @tparam UnitType compile time type of test unit to get (test_suite or test_case)
/// @param  id id of test unit to get
template<typename UnitType>
inline UnitType&                    get( test_unit_id id )
{
    return static_cast<UnitType&>( get( id, static_cast<test_unit_type>(UnitType::type) ) );
}
///@}

/// @name Test initiation interface
/// @{

/// Initiates test execution

/// This function is used to start the test execution from a specific "root" test unit.
/// If no root provided, test is started from master test suite. This second argument facilitates an ability of the test cases to
/// start some other test units (primarily used internally for self testing).
/// @param[in] tu Optional id of the test unit or test unit itself from which the test is started. If absent, master test suite is used
/// @param[in] continue_test true == continue test if it was already started, false == restart the test from scratch regardless
BOOST_TEST_DECL void                run( test_unit_id tu = INV_TEST_UNIT_ID, bool continue_test = true );
/// Initiates test execution. Same as other overload
BOOST_TEST_DECL void                run( test_unit const* tu, bool continue_test = true );
/// @}

/// @name Test events dispatchers
/// @{
/// Reports results of assertion to all test observers
BOOST_TEST_DECL void                assertion_result( unit_test::assertion_result ar );
/// Reports uncaught exception to all test observers
BOOST_TEST_DECL void                exception_caught( execution_exception const& );
/// Reports aborted test unit to all test observers
BOOST_TEST_DECL void                test_unit_aborted( test_unit const& );
/// Reports aborted test module to all test observers
BOOST_TEST_DECL void                test_aborted( );
/// @}

namespace impl {
// exclusively for self test
BOOST_TEST_DECL void                setup_for_execution( test_unit const& );
BOOST_TEST_DECL void                setup_loggers( );

// Helper for setting the name of the master test suite globally
struct BOOST_TEST_DECL master_test_suite_name_setter {
  master_test_suite_name_setter( const_string name );
};

} // namespace impl

// ************************************************************************** //
// **************                framework errors              ************** //
// ************************************************************************** //

/// This exception type is used to report internal Boost.Test framework errors.
struct BOOST_TEST_DECL internal_error : public std::runtime_error {
    internal_error( const_string m ) : std::runtime_error( std::string( m.begin(), m.size() ) ) {}
};

//____________________________________________________________________________//

/// This exception type is used to report test module setup errors.
struct BOOST_TEST_DECL setup_error : public std::runtime_error {
    setup_error( const_string m ) : std::runtime_error( std::string( m.begin(), m.size() ) ) {}
};

#define BOOST_TEST_SETUP_ASSERT( cond, msg ) BOOST_TEST_I_ASSRT( cond, unit_test::framework::setup_error( msg ) )

//____________________________________________________________________________//

struct nothing_to_test {
    explicit    nothing_to_test( int rc ) : m_result_code( rc ) {}

    int         m_result_code;
};

//____________________________________________________________________________//

} // namespace framework
} // unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_FRAMEWORK_HPP_020805GER
