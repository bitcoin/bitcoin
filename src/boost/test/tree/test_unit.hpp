//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines @ref boost::unit_test::test_unit "test_unit", @ref boost::unit_test::test_case "test_case",
/// @ref boost::unit_test::test_suite "test_suite" and @ref boost::unit_test::master_test_suite_t "master_test_suite_t"
// ***************************************************************************

#ifndef BOOST_TEST_TREE_TEST_UNIT_HPP_100211GER
#define BOOST_TEST_TREE_TEST_UNIT_HPP_100211GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/detail/fwd_decl.hpp>

#include <boost/test/tree/decorator.hpp>
#include <boost/test/tree/fixture.hpp>
#include <boost/test/framework.hpp>

#include <boost/test/tools/assertion_result.hpp>

#include <boost/test/utils/class_properties.hpp>

// Boost
#include <boost/function/function0.hpp>
#include <boost/function/function1.hpp>

// STL
#include <vector>
#include <string>
#include <map>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

namespace framework {
  class state;
}

// ************************************************************************** //
// **************                   test_unit                  ************** //
// ************************************************************************** //

typedef std::vector<test_unit_id> test_unit_id_list;

class BOOST_TEST_DECL test_unit {
public:
    enum { type = TUT_ANY };
    enum run_status { RS_DISABLED, RS_ENABLED, RS_INHERIT, RS_INVALID };

    typedef std::vector<test_unit_id>                                       id_list;
    typedef std::vector<test_unit_fixture_ptr>                              fixture_list_t;
    typedef BOOST_READONLY_PROPERTY(test_unit_id,(framework::state))        id_t;
    typedef BOOST_READONLY_PROPERTY(test_unit_id,(test_suite))              parent_id_t;
    typedef BOOST_READONLY_PROPERTY(id_list,(test_unit))                    id_list_t;
    typedef std::vector<decorator::base_ptr>                                decor_list_t;
    typedef BOOST_READONLY_PROPERTY(std::vector<std::string>,(test_unit))   label_list_t;

    typedef boost::function<test_tools::assertion_result (test_unit_id)>    precondition_t;
    typedef BOOST_READONLY_PROPERTY(std::vector<precondition_t>,(test_unit)) precond_list_t;

    // preconditions management
    void                                depends_on( test_unit* tu );
    void                                add_precondition( precondition_t const& );
    test_tools::assertion_result        check_preconditions() const;

    // labels management
    void                                add_label( const_string l );
    bool                                has_label( const_string l ) const;

    // helper access methods
    void                                increase_exp_fail( counter_t num );
    bool                                is_enabled() const    { return p_run_status == RS_ENABLED; }
    std::string                         full_name() const;

    // Public r/o properties
    test_unit_type const                p_type;                 ///< type for this test unit
    const_string const                  p_type_name;            ///< "case"/"suite"/"module"
    const_string const                  p_file_name;
    std::size_t const                   p_line_num;
    id_t                                p_id;                   ///< unique id for this test unit
    parent_id_t                         p_parent_id;            ///< parent test suite id
    label_list_t                        p_labels;               ///< list of labels associated with this test unit

    id_list_t                           p_dependencies;         ///< list of test units this one depends on
    precond_list_t                      p_preconditions;        ///< user supplied preconditions for this test unit;

    // Public r/w properties
    readwrite_property<std::string>     p_name;                 ///< name for this test unit
    readwrite_property<std::string>     p_description;          ///< description for this test unit
    readwrite_property<unsigned>        p_timeout;              ///< timeout for the test unit execution in seconds
    readwrite_property<counter_t>       p_expected_failures;    ///< number of expected failures in this test unit

    readwrite_property<run_status>      p_default_status;       ///< run status obtained by this unit during setup phase
    readwrite_property<run_status>      p_run_status;           ///< run status assigned to this unit before execution phase after applying all filters

    readwrite_property<counter_t>       p_sibling_rank;         ///< rank of this test unit amoung siblings of the same parent

    readwrite_property<decor_list_t>    p_decorators;           ///< automatically assigned decorators; execution is delayed till framework::finalize_setup_phase function
    readwrite_property<fixture_list_t>  p_fixtures;             ///< fixtures associated with this test unit

protected:
    ~test_unit();
    // Constructor
    test_unit( const_string tu_name, const_string tc_file, std::size_t tc_line, test_unit_type t );
    // Master test suite constructor
    explicit                            test_unit( const_string module_name );
};

// ************************************************************************** //
// **************              test_unit_generator             ************** //
// ************************************************************************** //

class BOOST_TEST_DECL test_unit_generator {
public:
    virtual test_unit*  next() const = 0;

protected:
    BOOST_TEST_PROTECTED_VIRTUAL ~test_unit_generator() {}
};

// ************************************************************************** //
// **************                   test_case                  ************** //
// ************************************************************************** //

class BOOST_TEST_DECL test_case : public test_unit {
public:
    enum { type = TUT_CASE };

    // Constructor
    test_case( const_string tc_name, boost::function<void ()> const& test_func );
    test_case( const_string tc_name, const_string tc_file, std::size_t tc_line, boost::function<void ()> const& test_func );

    // Public property
    typedef BOOST_READONLY_PROPERTY(boost::function<void ()>,(test_case))  test_func;

    test_func   p_test_func;

private:
    friend class framework::state;
    ~test_case() {}
};

// ************************************************************************** //
// **************                  test_suite                  ************** //
// ************************************************************************** //

//! Class representing test suites
class BOOST_TEST_DECL test_suite : public test_unit {
public:
    enum { type = TUT_SUITE };

    // Constructor
    explicit        test_suite( const_string ts_name, const_string ts_file, std::size_t ts_line );

    // test unit list management

    /*!@brief Adds a test unit to a test suite.
     *
     * It is possible to specify the timeout and the expected failures.
     */
    void            add( test_unit* tu, counter_t expected_failures = 0, unsigned timeout = 0 );

    /// @overload
    void            add( test_unit_generator const& gen, unsigned timeout = 0 );

    /// @overload
    void            add( test_unit_generator const& gen, decorator::collector_t& decorators );
  
    /// @overload
    void            add( boost::shared_ptr<test_unit_generator> gen_ptr, decorator::collector_t& decorators );

    //! Removes a test from the test suite.
    void            remove( test_unit_id id );
  
    //! Generates all the delayed test_units from the generators
    void            generate( );

    //! Check for duplicates name in test cases
    //!
    //! Raises a setup_error if there are duplicates
    void            check_for_duplicate_test_cases();

    // access methods
    test_unit_id    get( const_string tu_name ) const;
    std::size_t     size() const { return m_children.size(); }

protected:
    // Master test suite constructor
    explicit        test_suite( const_string module_name );

    friend BOOST_TEST_DECL
    void            traverse_test_tree( test_suite const&, test_tree_visitor&, bool );
    friend class    framework::state;
    virtual         ~test_suite() {}

    typedef std::multimap<counter_t,test_unit_id> children_per_rank;
    // Data members

    test_unit_id_list   m_children;
    children_per_rank   m_ranked_children; ///< maps child sibling rank to list of children with that rank
  
    std::vector< std::pair<boost::shared_ptr<test_unit_generator>, std::vector<decorator::base_ptr> > > m_generators; /// lazy evaluation
};

// ************************************************************************** //
// **************               master_test_suite              ************** //
// ************************************************************************** //

class BOOST_TEST_DECL master_test_suite_t : public test_suite {
private:
    master_test_suite_t();
    master_test_suite_t(const master_test_suite_t&); // undefined
    master_test_suite_t& operator=(master_test_suite_t const &); // undefined
  
public:
    // Data members
    int      argc;
    char**   argv;
  
    friend BOOST_TEST_DECL master_test_suite_t& boost::unit_test::framework::master_test_suite();
};

// ************************************************************************** //
// **************            user_tc_method_invoker            ************** //
// ************************************************************************** //

namespace ut_detail {

BOOST_TEST_DECL std::string normalize_test_case_name( const_string tu_name );

//____________________________________________________________________________//

template<typename InstanceType,typename UserTestCase>
struct user_tc_method_invoker {
    typedef void (UserTestCase::*TestMethod )();

    user_tc_method_invoker( shared_ptr<InstanceType> inst, TestMethod test_method )
    : m_inst( inst ), m_test_method( test_method ) {}

    void operator()() { ((*m_inst).*m_test_method)(); }

    shared_ptr<InstanceType> m_inst;
    TestMethod               m_test_method;
};

} // namespace ut_detail

// ************************************************************************** //
// **************                make_test_case                ************** //
// ************************************************************************** //

inline test_case*
make_test_case( boost::function<void ()> const& test_func, const_string tc_name, const_string tc_file, std::size_t tc_line )
{
    return new test_case( ut_detail::normalize_test_case_name( tc_name ), tc_file, tc_line, test_func );
}

//____________________________________________________________________________//

template<typename UserTestCase, typename InstanceType>
inline test_case*
make_test_case( void (UserTestCase::*           test_method )(),
                const_string                    tc_name,
                const_string                    tc_file,
                std::size_t                     tc_line,
                boost::shared_ptr<InstanceType> user_test_case )
{
    return new test_case( ut_detail::normalize_test_case_name( tc_name ),
                          tc_file,
                          tc_line,
                          ut_detail::user_tc_method_invoker<InstanceType,UserTestCase>( user_test_case, test_method ) );
}

//____________________________________________________________________________//

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TREE_TEST_UNIT_HPP_100211GER
