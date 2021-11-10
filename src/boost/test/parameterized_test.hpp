//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief generators and helper macros for parameterized tests
// ***************************************************************************

#ifndef BOOST_TEST_PARAMETERIZED_TEST_HPP_021102GER
#define BOOST_TEST_PARAMETERIZED_TEST_HPP_021102GER

// Boost.Test
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/utils/string_cast.hpp>

// Boost
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <boost/bind/bind.hpp>
#include <boost/function/function1.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

#define BOOST_PARAM_TEST_CASE( function, begin, end )                      \
boost::unit_test::make_test_case( function,                                \
                                  BOOST_TEST_STRINGIZE( function ),        \
                                  __FILE__, __LINE__,                      \
                                  (begin), (end) )                         \
/**/

#define BOOST_PARAM_CLASS_TEST_CASE( function, tc_instance, begin, end )   \
boost::unit_test::make_test_case( function,                                \
                                  BOOST_TEST_STRINGIZE( function ),        \
                                  __FILE__, __LINE__,                      \
                                  (tc_instance),                           \
                                  (begin), (end) )                         \
/**/

namespace boost {
namespace unit_test {

namespace ut_detail {

// ************************************************************************** //
// **************           param_test_case_generator          ************** //
// ************************************************************************** //

template<typename ParamType, typename ParamIter>
class param_test_case_generator : public test_unit_generator {
public:
    param_test_case_generator( boost::function<void (ParamType)> const& test_func,
                               const_string                             tc_name,
                               const_string                             tc_file,
                               std::size_t                              tc_line,
                               ParamIter                                par_begin,
                               ParamIter                                par_end )
    : m_test_func( test_func )
    , m_tc_name( ut_detail::normalize_test_case_name( tc_name ) )
    , m_tc_file( tc_file )
    , m_tc_line( tc_line )
    , m_par_begin( par_begin )
    , m_par_end( par_end )
    , m_index( 0 )
    {}

    virtual test_unit* next() const
    {
        if( m_par_begin == m_par_end )
            return (test_unit*)0;

        test_unit* res = new test_case( m_tc_name + "_" + utils::string_cast(m_index), m_tc_file, m_tc_line, boost::bind( m_test_func, *m_par_begin ) );

        ++m_par_begin;
        ++m_index;

        return res;
    }

private:
    // Data members
    boost::function<void (ParamType)>    m_test_func;
    std::string             m_tc_name;
    const_string            m_tc_file;
    std::size_t             m_tc_line;
    mutable ParamIter       m_par_begin;
    ParamIter               m_par_end;
    mutable std::size_t     m_index;
};

//____________________________________________________________________________//

template<typename UserTestCase,typename ParamType>
struct user_param_tc_method_invoker {
    typedef void (UserTestCase::*test_method)( ParamType );

    // Constructor
    user_param_tc_method_invoker( shared_ptr<UserTestCase> inst, test_method test_method )
    : m_inst( inst ), m_test_method( test_method ) {}

    void operator()( ParamType p ) { ((*m_inst).*m_test_method)( p ); }

    // Data members
    shared_ptr<UserTestCase> m_inst;
    test_method              m_test_method;
};

//____________________________________________________________________________//

} // namespace ut_detail

template<typename ParamType, typename ParamIter>
inline ut_detail::param_test_case_generator<ParamType,ParamIter>
make_test_case( boost::function<void (ParamType)> const& test_func,
                const_string                             tc_name,
                const_string                             tc_file,
                std::size_t                              tc_line,
                ParamIter                                par_begin,
                ParamIter                                par_end )
{
    return ut_detail::param_test_case_generator<ParamType,ParamIter>( test_func, tc_name, tc_file, tc_line, par_begin, par_end );
}

//____________________________________________________________________________//

template<typename ParamType, typename ParamIter>
inline ut_detail::param_test_case_generator<
    BOOST_DEDUCED_TYPENAME remove_const<BOOST_DEDUCED_TYPENAME remove_reference<ParamType>::type>::type,ParamIter>
make_test_case( void (*test_func)( ParamType ),
                const_string  tc_name,
                const_string  tc_file,
                std::size_t   tc_line,
                ParamIter     par_begin,
                ParamIter     par_end )
{
    typedef BOOST_DEDUCED_TYPENAME remove_const<BOOST_DEDUCED_TYPENAME remove_reference<ParamType>::type>::type param_value_type;
    return ut_detail::param_test_case_generator<param_value_type,ParamIter>( test_func, tc_name, tc_file, tc_line, par_begin, par_end );
}

//____________________________________________________________________________//

template<typename UserTestCase,typename ParamType, typename ParamIter>
inline ut_detail::param_test_case_generator<
    BOOST_DEDUCED_TYPENAME remove_const<BOOST_DEDUCED_TYPENAME remove_reference<ParamType>::type>::type,ParamIter>
make_test_case( void (UserTestCase::*test_method )( ParamType ),
                const_string                           tc_name,
                const_string                           tc_file,
                std::size_t                            tc_line,
                boost::shared_ptr<UserTestCase> const& user_test_case,
                ParamIter                              par_begin,
                ParamIter                              par_end )
{
    typedef BOOST_DEDUCED_TYPENAME remove_const<BOOST_DEDUCED_TYPENAME remove_reference<ParamType>::type>::type param_value_type;
    return ut_detail::param_test_case_generator<param_value_type,ParamIter>(
               ut_detail::user_param_tc_method_invoker<UserTestCase,ParamType>( user_test_case, test_method ),
               tc_name,
               tc_file,
               tc_line,
               par_begin,
               par_end );
}

//____________________________________________________________________________//

} // unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_PARAMETERIZED_TEST_HPP_021102GER

