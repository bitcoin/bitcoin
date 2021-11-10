//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Provides core implementation for Unit Test Framework.
/// Extensions can be provided in separate files
// ***************************************************************************

#ifndef BOOST_TEST_UNIT_TEST_SUITE_IPP_012205GER
#define BOOST_TEST_UNIT_TEST_SUITE_IPP_012205GER

// Boost.Test
#include <boost/detail/workaround.hpp>

#include <boost/test/framework.hpp>
#include <boost/test/results_collector.hpp>

#include <boost/test/tree/test_unit.hpp>
#include <boost/test/tree/visitor.hpp>
#include <boost/test/tree/traverse.hpp>
#include <boost/test/tree/auto_registration.hpp>
#include <boost/test/tree/global_fixture.hpp>

#include <boost/test/utils/foreach.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>

#include <boost/test/unit_test_parameters.hpp>

// STL
#include <algorithm>
#include <vector>
#include <set>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

// ************************************************************************** //
// **************                   test_unit                  ************** //
// ************************************************************************** //

test_unit::test_unit( const_string name, const_string file_name, std::size_t line_num, test_unit_type t )
: p_type( t )
, p_type_name( t == TUT_CASE ? "case" : "suite" )
, p_file_name( file_name )
, p_line_num( line_num )
, p_id( INV_TEST_UNIT_ID )
, p_parent_id( INV_TEST_UNIT_ID )
, p_name( std::string( name.begin(), name.size() ) )
, p_timeout( 0 )
, p_expected_failures( 0 )
, p_default_status( RS_INHERIT )
, p_run_status( RS_INVALID )
, p_sibling_rank(0)
{
}

//____________________________________________________________________________//

test_unit::test_unit( const_string module_name )
: p_type( TUT_SUITE )
, p_type_name( "module" )
, p_line_num( 0 )
, p_id( INV_TEST_UNIT_ID )
, p_parent_id( INV_TEST_UNIT_ID )
, p_name( std::string( module_name.begin(), module_name.size() ) )
, p_timeout( 0 )
, p_expected_failures( 0 )
, p_default_status( RS_INHERIT )
, p_run_status( RS_INVALID )
, p_sibling_rank(0)
{
}

//____________________________________________________________________________//

test_unit::~test_unit()
{
    framework::deregister_test_unit( this );
}

//____________________________________________________________________________//

void
test_unit::depends_on( test_unit* tu )
{
    BOOST_TEST_SETUP_ASSERT( p_id != framework::master_test_suite().p_id, 
                             "Can't add dependency to the master test suite" );

    p_dependencies.value.push_back( tu->p_id );
}

//____________________________________________________________________________//

void
test_unit::add_precondition( precondition_t const& pc )
{
    p_preconditions.value.push_back( pc );
}

//____________________________________________________________________________//

test_tools::assertion_result
test_unit::check_preconditions() const
{
    BOOST_TEST_FOREACH( test_unit_id, dep_id, p_dependencies.get() ) {
        test_unit const& dep = framework::get( dep_id, TUT_ANY );

        if( !dep.is_enabled() ) {
            test_tools::assertion_result res(false);
            res.message() << "dependency test " << dep.p_type_name << " \"" << dep.full_name() << "\" is disabled";
            return res;
        }

        test_results const& test_rslt = unit_test::results_collector.results( dep_id );
        if( !test_rslt.passed() ) {
            test_tools::assertion_result res(false);
            res.message() << "dependency test " << dep.p_type_name << " \"" << dep.full_name() << (test_rslt.skipped() ? "\" was skipped":"\" has failed");
            return res;
        }

        if( test_rslt.p_test_cases_skipped > 0 ) {
            test_tools::assertion_result res(false);
            res.message() << "dependency test " << dep.p_type_name << " \"" << dep.full_name() << "\" has skipped test cases";
            return res;
        }
    }

    BOOST_TEST_FOREACH( precondition_t, precondition, p_preconditions.get() ) {
        test_tools::assertion_result res = precondition( p_id );
        if( !res ) {
            test_tools::assertion_result res_out(false);
            res_out.message() << "precondition failed";
            if( !res.has_empty_message() )
                res_out.message() << ": " << res.message();
            return res_out;
        }
    }

    return true;
}

//____________________________________________________________________________//

void
test_unit::increase_exp_fail( counter_t num )
{
    p_expected_failures.value += num;

    if( p_parent_id != INV_TEST_UNIT_ID )
        framework::get<test_suite>( p_parent_id ).increase_exp_fail( num );
}

//____________________________________________________________________________//

std::string
test_unit::full_name() const
{
    if( p_parent_id == INV_TEST_UNIT_ID || p_parent_id == framework::master_test_suite().p_id )
        return p_name;

    std::string res = framework::get<test_suite>( p_parent_id ).full_name();
    res.append("/");

    res.append( p_name );

    return res;
}

//____________________________________________________________________________//

void
test_unit::add_label( const_string l )
{
    p_labels.value.push_back( std::string() + l );
}

//____________________________________________________________________________//

bool
test_unit::has_label( const_string l ) const
{
    return std::find( p_labels->begin(), p_labels->end(), l ) != p_labels->end();
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                   test_case                  ************** //
// ************************************************************************** //

test_case::test_case( const_string name, boost::function<void ()> const& test_func )
: test_unit( name, "", 0, static_cast<test_unit_type>(type) )
, p_test_func( test_func )
{
    framework::register_test_unit( this );
}

//____________________________________________________________________________//

test_case::test_case( const_string name, const_string file_name, std::size_t line_num, boost::function<void ()> const& test_func )
: test_unit( name, file_name, line_num, static_cast<test_unit_type>(type) )
, p_test_func( test_func )
{
    framework::register_test_unit( this );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                  test_suite                  ************** //
// ************************************************************************** //

//____________________________________________________________________________//

test_suite::test_suite( const_string name, const_string file_name, std::size_t line_num )
: test_unit( ut_detail::normalize_test_case_name( name ), file_name, line_num, static_cast<test_unit_type>(type) )
{
    framework::register_test_unit( this );
}

//____________________________________________________________________________//

test_suite::test_suite( const_string module_name )
: test_unit( module_name )
{
    framework::register_test_unit( this );
}

//____________________________________________________________________________//

void
test_suite::add( test_unit* tu, counter_t expected_failures, unsigned timeout )
{
    tu->p_timeout.value = timeout;

    m_children.push_back( tu->p_id );
    tu->p_parent_id.value = p_id;

    if( tu->p_expected_failures != 0 )
        increase_exp_fail( tu->p_expected_failures );

    if( expected_failures )
        tu->increase_exp_fail( expected_failures );
}

//____________________________________________________________________________//

void
test_suite::add( test_unit_generator const& gen, unsigned timeout )
{
    test_unit* tu;
    while((tu = gen.next()) != 0)
        add( tu, 0, timeout );
}

//____________________________________________________________________________//

void
test_suite::add( test_unit_generator const& gen, decorator::collector_t& decorators )
{
    test_unit* tu;
    while((tu = gen.next()) != 0) {
        decorators.store_in( *tu );
        add( tu, 0 );
    }
    decorators.reset();
}

//____________________________________________________________________________//

void
test_suite::add( boost::shared_ptr<test_unit_generator> gen_ptr, decorator::collector_t& decorators )
{
    std::pair<boost::shared_ptr<test_unit_generator>, std::vector<decorator::base_ptr> > tmp_p(gen_ptr, decorators.get_lazy_decorators() );
    m_generators.push_back(tmp_p);
    decorators.reset();
}

//____________________________________________________________________________//

void
test_suite::generate( )
{
    typedef std::pair<boost::shared_ptr<test_unit_generator>, std::vector<decorator::base_ptr> > element_t;
  
    for(std::vector<element_t>::iterator it(m_generators.begin()), ite(m_generators.end());
        it < ite;
        ++it)
    {
      test_unit* tu;
      while((tu = it->first->next()) != 0) {
          tu->p_decorators.value.insert( tu->p_decorators.value.end(), it->second.begin(), it->second.end() );
          //it->second.store_in( *tu );
          add( tu, 0 );
      }

    }
    m_generators.clear();
    
    #if 0
    test_unit* tu;
    while((tu = gen.next()) != 0) {
        decorators.store_in( *tu );
        add( tu, 0 );
    }
    #endif
}

//____________________________________________________________________________//

void
test_suite::check_for_duplicate_test_cases() {
    // check for clashing names #12597
    std::set<std::string> names;
    for( test_unit_id_list::const_iterator it(m_children.begin()), ite(m_children.end());
         it < ite;
         ++it) {
         std::string name = framework::get(*it, TUT_ANY).p_name;
         std::pair<std::set<std::string>::iterator, bool> ret = names.insert(name);
         BOOST_TEST_SETUP_ASSERT(ret.second,
            "test unit with name '"
            + name
            + std::string("' registered multiple times in the test suite '")
            + this->p_name.value
            + "'");
    }

    return;
}

//____________________________________________________________________________//

void
test_suite::remove( test_unit_id id )
{
    test_unit_id_list::iterator it = std::find( m_children.begin(), m_children.end(), id );

    if( it != m_children.end() )
        m_children.erase( it );
}

//____________________________________________________________________________//

test_unit_id
test_suite::get( const_string tu_name ) const
{
    BOOST_TEST_FOREACH( test_unit_id, id, m_children ) {
        if( tu_name == framework::get( id, ut_detail::test_id_2_unit_type( id ) ).p_name.get() )
            return id;
    }

    return INV_TEST_UNIT_ID;
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************               master_test_suite              ************** //
// ************************************************************************** //

master_test_suite_t::master_test_suite_t()
: test_suite( "Master Test Suite" )
, argc( 0 )
, argv( 0 )
{
    p_default_status.value = RS_ENABLED;
}

// ************************************************************************** //
// **************               traverse_test_tree             ************** //
// ************************************************************************** //

void
traverse_test_tree( test_case const& tc, test_tree_visitor& V, bool ignore_status )
{
    if( tc.is_enabled() || ignore_status )
        V.visit( tc );
}

//____________________________________________________________________________//

void
traverse_test_tree( test_suite const& suite, test_tree_visitor& V, bool ignore_status )
{
    // skip disabled test suite unless we asked to ignore this condition
    if( !ignore_status && !suite.is_enabled() )
        return;

    // Invoke test_suite_start callback
    if( !V.test_suite_start( suite ) )
        return;

    // Recurse into children
    std::size_t total_children = suite.m_children.size();
    for( std::size_t i=0; i < total_children; ) {
        // this statement can remove the test unit from this list
        traverse_test_tree( suite.m_children[i], V, ignore_status );
        if( total_children > suite.m_children.size() )
            total_children = suite.m_children.size();
        else
            ++i;
    }

    // Invoke test_suite_finish callback
    V.test_suite_finish( suite );
}

//____________________________________________________________________________//

void
traverse_test_tree( test_unit_id id, test_tree_visitor& V, bool ignore_status )
{
    if( ut_detail::test_id_2_unit_type( id ) == TUT_CASE )
        traverse_test_tree( framework::get<test_case>( id ), V, ignore_status );
    else
        traverse_test_tree( framework::get<test_suite>( id ), V, ignore_status );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************               object generators              ************** //
// ************************************************************************** //

namespace ut_detail {

std::string
normalize_test_case_name( const_string name )
{
    std::string norm_name( name.begin(), name.size() );

    if( name[0] == '&' )
        norm_name = norm_name.substr( 1 );

    // trim spaces
    std::size_t first_not_space = norm_name.find_first_not_of(' ');
    if( first_not_space ) {
        norm_name.erase(0, first_not_space);
    }

    std::size_t last_not_space = norm_name.find_last_not_of(' ');
    if( last_not_space !=std::string::npos ) {
        norm_name.erase(last_not_space + 1);
    }

    // sanitize all chars that might be used in runtime filters
    static const char to_replace[] = { ':', '*', '@', '+', '!', '/', ',' };
    for(std::size_t index = 0;
        index < sizeof(to_replace)/sizeof(to_replace[0]);
        index++) {
        std::replace(norm_name.begin(), norm_name.end(), to_replace[index], '_');
    }

    return norm_name;
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************           auto_test_unit_registrar           ************** //
// ************************************************************************** //

auto_test_unit_registrar::auto_test_unit_registrar( test_case* tc, decorator::collector_t& decorators, counter_t exp_fail )
{
    framework::current_auto_test_suite().add( tc, exp_fail );

    decorators.store_in( *tc );
    decorators.reset();
}

//____________________________________________________________________________//

auto_test_unit_registrar::auto_test_unit_registrar( const_string ts_name, const_string ts_file, std::size_t ts_line, decorator::collector_t& decorators )
{
    test_unit_id id = framework::current_auto_test_suite().get( ts_name );

    test_suite* ts;

    if( id != INV_TEST_UNIT_ID ) {
        ts = &framework::get<test_suite>( id );
        BOOST_ASSERT( ts->p_parent_id == framework::current_auto_test_suite().p_id );
    }
    else {
        ts = new test_suite( ts_name, ts_file, ts_line );
        framework::current_auto_test_suite().add( ts );
    }

    decorators.store_in( *ts );
    decorators.reset();

    framework::current_auto_test_suite( ts );
}

//____________________________________________________________________________//

auto_test_unit_registrar::auto_test_unit_registrar( test_unit_generator const& tc_gen, decorator::collector_t& decorators )
{
    framework::current_auto_test_suite().add( tc_gen, decorators );
}

//____________________________________________________________________________//

auto_test_unit_registrar::auto_test_unit_registrar( boost::shared_ptr<test_unit_generator> tc_gen, decorator::collector_t& decorators )
{
    framework::current_auto_test_suite().add( tc_gen, decorators );
}


//____________________________________________________________________________//

auto_test_unit_registrar::auto_test_unit_registrar( int )
{
    framework::current_auto_test_suite( 0, false );
}

//____________________________________________________________________________//

} // namespace ut_detail

// ************************************************************************** //
// **************                global_fixture                ************** //
// ************************************************************************** //

global_fixture::global_fixture(): registered(false)
{
    framework::register_global_fixture( *this );
    registered = true;
}

void global_fixture::unregister_from_framework() {
    // not accessing the framework singleton after deregistering -> release
    // of the observer from the framework
    if(registered) {
        framework::deregister_global_fixture( *this );
    }
    registered = false;
}

global_fixture::~global_fixture()
{
    this->unregister_from_framework();
}

// ************************************************************************** //
// **************            global_configuration              ************** //
// ************************************************************************** //

global_configuration::global_configuration(): registered(false)
{
    framework::register_observer( *this );
    registered = true;
}

void global_configuration::unregister_from_framework()
{
    // not accessing the framework singleton after deregistering -> release
    // of the observer from the framework
    if(registered) {
        framework::deregister_observer( *this );
    }
    registered = false;
}

global_configuration::~global_configuration()
{
    this->unregister_from_framework();
}

//____________________________________________________________________________//

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UNIT_TEST_SUITE_IPP_012205GER
