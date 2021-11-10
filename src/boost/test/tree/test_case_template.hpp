//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
///@ file
/// Defines template_test_case_gen
// ***************************************************************************

#ifndef BOOST_TEST_TREE_TEST_CASE_TEMPLATE_HPP_091911GER
#define BOOST_TEST_TREE_TEST_CASE_TEMPLATE_HPP_091911GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/detail/fwd_decl.hpp>
#include <boost/test/tree/test_unit.hpp>

#include <boost/test/utils/class_properties.hpp>
#include <boost/test/tree/observer.hpp>
#include <boost/test/utils/algorithm.hpp>


// Boost
#include <boost/shared_ptr.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_volatile.hpp>
#include <boost/type_traits/is_lvalue_reference.hpp>
#include <boost/type_traits/is_rvalue_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/function/function0.hpp>

#if defined(BOOST_NO_TYPEID) || defined(BOOST_NO_RTTI)
#  include <boost/current_function.hpp>
#else
#  include <boost/core/demangle.hpp>
#endif

// STL
#include <string>   // for std::string
#include <list>     // for std::list

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_AUTO_DECLARATIONS)
  #include <type_traits>
  #include <boost/mpl/is_sequence.hpp>
#endif

#include <boost/test/detail/suppress_warnings.hpp>


//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace ut_detail {

// ************************************************************************** //
// **************          test_case_template_invoker          ************** //
// ************************************************************************** //

template<typename TestCaseTemplate,typename TestType>
class test_case_template_invoker {
public:
    void    operator()()    { TestCaseTemplate::run( (boost::type<TestType>*)0 ); }
};

// ************************************************************************** //
// **************           generate_test_case_4_type          ************** //
// ************************************************************************** //

template<typename Generator, typename TestCaseTemplate>
struct generate_test_case_4_type {
    explicit    generate_test_case_4_type( const_string tc_name, const_string tc_file, std::size_t tc_line, Generator& G )
    : m_test_case_name( tc_name )
    , m_test_case_file( tc_file )
    , m_test_case_line( tc_line )
    , m_holder( G )
    {}

    template<typename TestType>
    void        operator()( mpl::identity<TestType> )
    {
        std::string full_name;
        assign_op( full_name, m_test_case_name, 0 );
        full_name += '<';
#if !defined(BOOST_NO_TYPEID) && !defined(BOOST_NO_RTTI)
        full_name += boost::core::demangle(typeid(TestType).name()); // same as execution_monitor.ipp
#else
        full_name += BOOST_CURRENT_FUNCTION;
#endif

        // replacing ',' by ', ' first, and then removing any double space
        static const std::string to_replace[] =  { "class ", "struct ", ",", "  ", " <", " >"};
        static const std::string replacement[] = { "", "" , ", ", " ", "<" , ">"};

        full_name = unit_test::utils::replace_all_occurrences_of(
            full_name,
            to_replace, to_replace + sizeof(to_replace)/sizeof(to_replace[0]),
            replacement, replacement + sizeof(replacement)/sizeof(replacement[0]));

        typedef typename boost::remove_reference<TestType>::type TestTypewoRef;
        if( boost::is_const<TestTypewoRef>::value )
            full_name += "_const";
        if( boost::is_volatile<TestTypewoRef>::value )
            full_name += "_volatile";
        if( boost::is_rvalue_reference<TestType>::value )
            full_name += "_refref";
        else if( boost::is_lvalue_reference<TestType>::value )
            full_name += "_ref";

        full_name += '>';

        m_holder.m_test_cases.push_back( new test_case( ut_detail::normalize_test_case_name( full_name ),
                                                        m_test_case_file,
                                                        m_test_case_line,
                                                        test_case_template_invoker<TestCaseTemplate,TestType>() ) );
    }

private:
    // Data members
    const_string    m_test_case_name;
    const_string    m_test_case_file;
    std::size_t     m_test_case_line;
    Generator&      m_holder;
};

// ************************************************************************** //
// **************              test_case_template              ************** //
// ************************************************************************** //

class template_test_case_gen_base : public test_unit_generator {
public:
    test_unit* next() const BOOST_OVERRIDE
    {
        if( m_test_cases.empty() )
            return 0;

        test_unit* res = m_test_cases.front();
        m_test_cases.pop_front();

        return res;
    }

    // Data members
    mutable std::list<test_unit*> m_test_cases;
};

template<typename TestCaseTemplate,typename TestTypesList, typename enabler = void>
class template_test_case_gen : public template_test_case_gen_base {
public:
    // Constructor
    template_test_case_gen( const_string tc_name, const_string tc_file, std::size_t tc_line )
    {
        typedef generate_test_case_4_type<template_test_case_gen<TestCaseTemplate,TestTypesList>,TestCaseTemplate> single_test_gen;

        mpl::for_each<TestTypesList,mpl::make_identity<mpl::_> >( single_test_gen( tc_name, tc_file, tc_line, *this ) );
    }
};

// Describing template test cases with tuples
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_AUTO_DECLARATIONS) && \
    !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

template<typename TestCaseTemplate,
         template <class ...> class C,
         typename... parameter_pack>
class template_test_case_gen<
    TestCaseTemplate,
    C<parameter_pack...>,
    typename std::enable_if<!boost::mpl::is_sequence<C<parameter_pack...>>::value>::type >
  : public template_test_case_gen_base {

    template<typename F>
    void for_each(F &f)
    {
        auto l = { (f(mpl::identity<parameter_pack>()), 0)... };
        (void)l; // silence warning
    }

public:
    // Constructor
    template_test_case_gen( const_string tc_name, const_string tc_file, std::size_t tc_line )
    {
        using this_type = template_test_case_gen<
            TestCaseTemplate,
            C<parameter_pack...>,
            typename std::enable_if<!boost::mpl::is_sequence<C<parameter_pack...>>::value>::type>;
        using single_test_gen = generate_test_case_4_type<this_type, TestCaseTemplate>;

        single_test_gen op( tc_name, tc_file, tc_line, *this );

        this->for_each(op);
    }
};

#endif /* C++11 variadic, type alias */

} // namespace ut_detail
} // unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TREE_TEST_CASE_TEMPLATE_HPP_091911GER
