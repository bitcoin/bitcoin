//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief test case family based on data generator
// ***************************************************************************

#ifndef BOOST_TEST_DATA_TEST_CASE_HPP_102211GER
#define BOOST_TEST_DATA_TEST_CASE_HPP_102211GER

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/dataset.hpp>
#include <boost/test/data/for_each_sample.hpp>
#include <boost/test/tree/test_unit.hpp>

// Boost
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/comparison/equal.hpp>

#include <boost/bind/bind.hpp>
#include <boost/type_traits/is_copy_constructible.hpp>

#include <boost/test/tools/detail/print_helper.hpp>
#include <boost/test/utils/string_cast.hpp>

#include <list>
#include <string>

#include <boost/test/detail/suppress_warnings.hpp>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) \
   && !defined(BOOST_TEST_DATASET_MAX_ARITY)
# define BOOST_TEST_DATASET_MAX_ARITY 10
#endif

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {

namespace ds_detail {

// ************************************************************************** //
// **************                     seed                     ************** //
// ************************************************************************** //

struct seed {
    template<typename DataSet>
    typename data::result_of::make<DataSet>::type
    operator->*( DataSet&& ds ) const
    {
        return data::make( std::forward<DataSet>( ds ) );
    }
};


#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !defined(BOOST_NO_CXX11_DECLTYPE) && \
    !defined(BOOST_NO_CXX11_TRAILING_RESULT_TYPES) && \
    !defined(BOOST_NO_CXX11_SMART_PTR)

#define BOOST_TEST_DATASET_VARIADIC
template <class T>
struct parameter_holder {
    std::shared_ptr<T> value;

    parameter_holder(T && value_)
        : value(std::make_shared<T>(std::move(value_)))
    {}

    operator T const&() const {
        return *value;
    }
};

template <class T>
parameter_holder<typename std::remove_reference<T>::type>
boost_bind_rvalue_holder_helper_impl(T&& value, boost::false_type /* is copy constructible */) {
    return parameter_holder<typename std::remove_reference<T>::type>(std::forward<T>(value));
}

template <class T>
T&& boost_bind_rvalue_holder_helper_impl(T&& value, boost::true_type /* is copy constructible */) {
    return std::forward<T>(value);
}

template <class T>
auto boost_bind_rvalue_holder_helper(T&& value)
  -> decltype(boost_bind_rvalue_holder_helper_impl(
                  std::forward<T>(value),
                  typename boost::is_copy_constructible<typename std::remove_reference<T>::type>::type()))
{
    // need to use boost::is_copy_constructible because std::is_copy_constructible is broken on MSVC12
    return boost_bind_rvalue_holder_helper_impl(
              std::forward<T>(value),
              typename boost::is_copy_constructible<typename std::remove_reference<T>::type>::type());
}

#endif


// ************************************************************************** //
// **************                 test_case_gen                ************** //
// ************************************************************************** //

template<typename TestCase,typename DataSet>
class test_case_gen : public test_unit_generator {
public:
    // Constructor
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    test_case_gen( const_string tc_name, const_string tc_file, std::size_t tc_line, DataSet&& ds )
    : m_dataset( std::forward<DataSet>( ds ) )
    , m_generated( false )
    , m_tc_name( ut_detail::normalize_test_case_name( tc_name ) )
    , m_tc_file( tc_file )
    , m_tc_line( tc_line )
    , m_tc_index( 0 )
    {}
    test_case_gen( test_case_gen&& gen )
    : m_dataset( std::move( gen.m_dataset ) )
    , m_generated( gen.m_generated )
    , m_tc_name( gen.m_tc_name )
    , m_tc_file( gen.m_tc_file )
    , m_tc_line( gen.m_tc_line )
    , m_tc_index( gen.m_tc_index )
    , m_test_cases( std::move(gen.m_test_cases) )
    {}
#else
    test_case_gen( const_string tc_name, const_string tc_file, std::size_t tc_line, DataSet const& ds )
    : m_dataset( ds )
    , m_generated( false )
    , m_tc_name( ut_detail::normalize_test_case_name( tc_name ) )
    , m_tc_file( tc_file )
    , m_tc_line( tc_line )
    , m_tc_index( 0 )
    {}
#endif

public:
    virtual test_unit* next() const
    {
        if(!m_generated) {
            data::for_each_sample( m_dataset, *this );
            m_generated = true;
        }

        if( m_test_cases.empty() )
            return 0;

        test_unit* res = m_test_cases.front();
        m_test_cases.pop_front();

        return res;
    }


#if !defined(BOOST_TEST_DATASET_VARIADIC)
    // see BOOST_TEST_DATASET_MAX_ARITY to increase the default supported arity
    // there is also a limit on boost::bind
#define TC_MAKE(z,arity,_)                                                              \
    template<BOOST_PP_ENUM_PARAMS(arity, typename Arg)>                                 \
    void    operator()( BOOST_PP_ENUM_BINARY_PARAMS(arity, Arg, const& arg) ) const     \
    {                                                                                   \
        m_test_cases.push_back( new test_case( genTestCaseName(), m_tc_file, m_tc_line, \
           boost::bind( &TestCase::template test_method<BOOST_PP_ENUM_PARAMS(arity,Arg)>,\
           BOOST_PP_ENUM_PARAMS(arity, arg) ) ) );                                      \
    }                                                                                   \

    BOOST_PP_REPEAT_FROM_TO(1, BOOST_TEST_DATASET_MAX_ARITY, TC_MAKE, _)
#else
    template<typename ...Arg>
    void    operator()(Arg&& ... arg) const
    {
        m_test_cases.push_back(
            new test_case( genTestCaseName(),
                           m_tc_file,
                           m_tc_line,
                           std::bind( &TestCase::template test_method<Arg...>,
                                      boost_bind_rvalue_holder_helper(std::forward<Arg>(arg))...)));
    }
#endif

private:
    std::string  genTestCaseName() const
    {
        return "_" + utils::string_cast(m_tc_index++);
    }

    // Data members
    DataSet                         m_dataset;
    mutable bool                    m_generated;
    std::string                     m_tc_name;
    const_string                    m_tc_file;
    std::size_t                     m_tc_line;
    mutable std::size_t             m_tc_index;
    mutable std::list<test_unit*>   m_test_cases;
};

//____________________________________________________________________________//

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template<typename TestCase,typename DataSet>
boost::shared_ptr<test_unit_generator> //test_case_gen<TestCase,DataSet>
make_test_case_gen( const_string tc_name, const_string tc_file, std::size_t tc_line, DataSet&& ds )
{
    return boost::shared_ptr<test_unit_generator>(new test_case_gen<TestCase,DataSet>( tc_name, tc_file, tc_line, std::forward<DataSet>(ds) ));
}
#else
template<typename TestCase,typename DataSet>
test_case_gen<TestCase,DataSet>
make_test_case_gen( const_string tc_name, const_string tc_file, std::size_t tc_line, DataSet const& ds )
{
    return test_case_gen<TestCase,DataSet>( tc_name, tc_file, tc_line, ds );
}
#endif

//____________________________________________________________________________//

} // namespace ds_detail

// ************************************************************************** //
// **************             BOOST_DATA_TEST_CASE             ************** //
// ************************************************************************** //

#define BOOST_DATA_TEST_CASE_PARAM(r, _, i, param)  (BOOST_PP_CAT(Arg, i) const& param)
#define BOOST_DATA_TEST_CONTEXT(r, _, param)  << BOOST_STRINGIZE(param) << " = " << boost::test_tools::tt_detail::print_helper(param) << "; "

#define BOOST_DATA_TEST_CASE_PARAMS( params )                           \
    BOOST_PP_SEQ_ENUM(                                                  \
        BOOST_PP_SEQ_FOR_EACH_I(BOOST_DATA_TEST_CASE_PARAM, _, params)) \
/**/

#define BOOST_DATA_TEST_CASE_IMPL(arity, F, test_name, dataset, params) \
struct BOOST_PP_CAT(test_name, case) : public F {                       \
    template<BOOST_PP_ENUM_PARAMS(arity, typename Arg)>                 \
    static void test_method( BOOST_DATA_TEST_CASE_PARAMS( params ) )    \
    {                                                                   \
        BOOST_TEST_CONTEXT( ""                                          \
            BOOST_PP_SEQ_FOR_EACH(BOOST_DATA_TEST_CONTEXT, _, params))  \
        {                                                               \
          BOOST_TEST_CHECKPOINT('"' << #test_name << "\" fixture ctor");\
          BOOST_PP_CAT(test_name, case) t;                              \
          BOOST_TEST_CHECKPOINT('"'                                     \
              << #test_name << "\" fixture setup");                     \
          boost::unit_test::setup_conditional(t);                       \
          BOOST_TEST_CHECKPOINT('"' << #test_name << "\" test entry");  \
          t._impl(BOOST_PP_SEQ_ENUM(params));                           \
          BOOST_TEST_CHECKPOINT('"'                                     \
              << #test_name << "\" fixture teardown");                  \
          boost::unit_test::teardown_conditional(t);                    \
          BOOST_TEST_CHECKPOINT('"' << #test_name << "\" fixture dtor");\
        }                                                               \
    }                                                                   \
private:                                                                \
    template<BOOST_PP_ENUM_PARAMS(arity, typename Arg)>                 \
    void _impl(BOOST_DATA_TEST_CASE_PARAMS( params ));                  \
};                                                                      \
                                                                        \
BOOST_AUTO_TEST_SUITE( test_name,                                       \
                       *boost::unit_test::decorator::stack_decorator()) \
                                                                        \
BOOST_AUTO_TU_REGISTRAR( BOOST_PP_CAT(test_name, case) )(               \
    boost::unit_test::data::ds_detail::make_test_case_gen<              \
                                      BOOST_PP_CAT(test_name, case)>(   \
          BOOST_STRINGIZE( test_name ),                                 \
          __FILE__, __LINE__,                                           \
          boost::unit_test::data::ds_detail::seed{} ->* dataset ),      \
    boost::unit_test::decorator::collector_t::instance() );             \
                                                                        \
BOOST_AUTO_TEST_SUITE_END()                                             \
                                                                        \
    template<BOOST_PP_ENUM_PARAMS(arity, typename Arg)>                 \
    void BOOST_PP_CAT(test_name, case)::_impl(                          \
                                BOOST_DATA_TEST_CASE_PARAMS( params ) ) \
/**/

#define BOOST_DATA_TEST_CASE_WITH_PARAMS( F, test_name, dataset, ... )  \
    BOOST_DATA_TEST_CASE_IMPL( BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),     \
                               F, test_name, dataset,                   \
                               BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__) )  \
/**/
#define BOOST_DATA_TEST_CASE_NO_PARAMS( F, test_name, dataset )         \
    BOOST_DATA_TEST_CASE_WITH_PARAMS( F, test_name, dataset, sample )   \
/**/

#if BOOST_PP_VARIADICS_MSVC

#define BOOST_DATA_TEST_CASE( ... )                                     \
    BOOST_PP_CAT(                                                       \
    BOOST_PP_IIF(BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),2), \
                     BOOST_DATA_TEST_CASE_NO_PARAMS,                    \
                     BOOST_DATA_TEST_CASE_WITH_PARAMS) (                \
                        BOOST_AUTO_TEST_CASE_FIXTURE, __VA_ARGS__), )   \
/**/

#define BOOST_DATA_TEST_CASE_F( F, ... )                                \
    BOOST_PP_CAT(                                                       \
    BOOST_PP_IIF(BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),2), \
                     BOOST_DATA_TEST_CASE_NO_PARAMS,                    \
                     BOOST_DATA_TEST_CASE_WITH_PARAMS) (                \
                        F, __VA_ARGS__), )                              \
/**/

#else

#define BOOST_DATA_TEST_CASE( ... )                                     \
    BOOST_PP_IIF(BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),2), \
                     BOOST_DATA_TEST_CASE_NO_PARAMS,                    \
                     BOOST_DATA_TEST_CASE_WITH_PARAMS) (                \
                        BOOST_AUTO_TEST_CASE_FIXTURE, __VA_ARGS__)      \
/**/

#define BOOST_DATA_TEST_CASE_F( F, ... )                                \
    BOOST_PP_IIF(BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),2), \
                     BOOST_DATA_TEST_CASE_NO_PARAMS,                    \
                     BOOST_DATA_TEST_CASE_WITH_PARAMS) (                \
                        F, __VA_ARGS__)                                 \
/**/
#endif

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_TEST_CASE_HPP_102211GER

