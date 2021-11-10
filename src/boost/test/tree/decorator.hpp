//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 62016 $
//
//  Description : defines decorators to be using with auto registered test units
// ***************************************************************************

#ifndef BOOST_TEST_TREE_DECORATOR_HPP_091911GER
#define BOOST_TEST_TREE_DECORATOR_HPP_091911GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/detail/global_typedef.hpp>

#include <boost/test/tree/fixture.hpp>

#include <boost/test/tools/assertion_result.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/function/function0.hpp>
#include <boost/function/function1.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

// STL
#include <vector>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

class test_unit;

namespace decorator {

// ************************************************************************** //
// **************             decorator::collector_t             ************** //
// ************************************************************************** //

class base;
typedef boost::shared_ptr<base> base_ptr;

class BOOST_TEST_DECL collector_t {

public:
    collector_t&            operator*( base const& d );

    void                    store_in( test_unit& tu );

    void                    reset();

    void                    stack();

    std::vector<base_ptr>   get_lazy_decorators() const;

    // singleton pattern without ctor
    BOOST_TEST_SINGLETON_CONS_NO_CTOR( collector_t )

private:
    // Class invariant: minimal size is 1.
    collector_t() : m_tu_decorators_stack(1) {}

    // Data members
    std::vector< std::vector<base_ptr> >   m_tu_decorators_stack;
};


// ************************************************************************** //
// **************              decorator::base                 ************** //
// ************************************************************************** //

class BOOST_TEST_DECL base {
public:
    // composition interface
    virtual collector_t&    operator*() const;

    // application interface
    virtual void            apply( test_unit& tu ) = 0;

    // deep cloning interface
    virtual base_ptr        clone() const = 0;

protected:
    virtual ~base() {}
};

// ************************************************************************** //
// **************         decorator::stack_decorator           ************** //
// ************************************************************************** //

//!@ A decorator that creates a new stack in the collector
//!
//! This decorator may be used in places where the currently accumulated decorators
//! in the collector should be applied to lower levels of the hierarchy rather
//! than the current one. This is for instance for dataset test cases, where the
//! macro does not let the user specify decorators for the underlying generated tests
//! (but rather on the main generator function), applying the stack_decorator at the
//! parent level lets us consume the decorator at the underlying test cases level.
class BOOST_TEST_DECL stack_decorator : public decorator::base {
public:
    explicit                stack_decorator() {}

    collector_t&    operator*() const BOOST_OVERRIDE;

private:
    // decorator::base interface
    void            apply( test_unit& tu ) BOOST_OVERRIDE;
    base_ptr        clone() const BOOST_OVERRIDE { return base_ptr(new stack_decorator()); }
};

// ************************************************************************** //
// **************               decorator::label               ************** //
// ************************************************************************** //

class BOOST_TEST_DECL label : public decorator::base {
public:
    explicit                label( const_string l ) : m_label( l ) {}

private:
    // decorator::base interface
    void            apply( test_unit& tu ) BOOST_OVERRIDE;
    base_ptr        clone() const BOOST_OVERRIDE { return base_ptr(new label( m_label )); }

    // Data members
    const_string            m_label;
};

// ************************************************************************** //
// **************         decorator::expected_failures         ************** //
// ************************************************************************** //

class BOOST_TEST_DECL expected_failures : public decorator::base {
public:
    explicit                expected_failures( counter_t ef ) : m_exp_fail( ef ) {}

private:
    // decorator::base interface
    void            apply( test_unit& tu ) BOOST_OVERRIDE;
    base_ptr        clone() const BOOST_OVERRIDE { return base_ptr(new expected_failures( m_exp_fail )); }

    // Data members
    counter_t               m_exp_fail;
};

// ************************************************************************** //
// **************              decorator::timeout              ************** //
// ************************************************************************** //

class BOOST_TEST_DECL timeout : public decorator::base {
public:
    explicit                timeout( unsigned t ) : m_timeout( t ) {}

private:
    // decorator::base interface
    void            apply( test_unit& tu ) BOOST_OVERRIDE;
    base_ptr        clone() const BOOST_OVERRIDE { return base_ptr(new timeout( m_timeout )); }

    // Data members
    unsigned                m_timeout;
};

// ************************************************************************** //
// **************            decorator::description            ************** //
// ************************************************************************** //

class BOOST_TEST_DECL description : public decorator::base {
public:
    explicit                description( const_string descr ) : m_description( descr ) {}

private:
    // decorator::base interface
    void            apply( test_unit& tu ) BOOST_OVERRIDE;
    base_ptr        clone() const BOOST_OVERRIDE { return base_ptr(new description( m_description )); }

    // Data members
    const_string            m_description;
};

// ************************************************************************** //
// **************            decorator::depends_on             ************** //
// ************************************************************************** //

class BOOST_TEST_DECL depends_on : public decorator::base {
public:
    explicit                depends_on( const_string dependency ) : m_dependency( dependency ) {}

private:
    // decorator::base interface
    void            apply( test_unit& tu ) BOOST_OVERRIDE;
    base_ptr        clone() const BOOST_OVERRIDE { return base_ptr(new depends_on( m_dependency )); }

    // Data members
    const_string            m_dependency;
};

// ************************************************************************** //
// **************    decorator::enable_if/enabled/disabled     ************** //
// ************************************************************************** //

class BOOST_TEST_DECL enable_if_impl : public decorator::base {
protected:
    void                    apply_impl( test_unit& tu, bool condition );
};

template<bool condition>
class enable_if : public enable_if_impl {
private:
    // decorator::base interface
    void            apply( test_unit& tu ) BOOST_OVERRIDE   { this->apply_impl( tu, condition ); }
    base_ptr        clone() const BOOST_OVERRIDE            { return base_ptr(new enable_if<condition>()); }
};

typedef enable_if<true> enabled;
typedef enable_if<false> disabled;

// ************************************************************************** //
// **************              decorator::fixture              ************** //
// ************************************************************************** //

class BOOST_TEST_DECL fixture_t : public decorator::base {
public:
    // Constructor
    explicit                fixture_t( test_unit_fixture_ptr impl ) : m_impl( impl ) {}

private:
    // decorator::base interface
    void            apply( test_unit& tu ) BOOST_OVERRIDE;
    base_ptr        clone() const BOOST_OVERRIDE { return base_ptr(new fixture_t( m_impl )); }

    // Data members
    test_unit_fixture_ptr m_impl;
};

//____________________________________________________________________________//

template<typename F>
inline fixture_t
fixture()
{
    return fixture_t( test_unit_fixture_ptr( new unit_test::class_based_fixture<F>() ) );
}

//____________________________________________________________________________//

template<typename F, typename Arg>
inline fixture_t
fixture( Arg const& arg )
{
    return fixture_t( test_unit_fixture_ptr( new unit_test::class_based_fixture<F,Arg>( arg ) ) );
}

//____________________________________________________________________________//

inline fixture_t
fixture( boost::function<void()> const& setup, boost::function<void()> const& teardown = boost::function<void()>() )
{
    return fixture_t( test_unit_fixture_ptr( new unit_test::function_based_fixture( setup, teardown ) ) );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************            decorator::depends_on             ************** //
// ************************************************************************** //

class BOOST_TEST_DECL precondition : public decorator::base {
public:
    typedef boost::function<test_tools::assertion_result (test_unit_id)>   predicate_t;

    explicit                precondition( predicate_t p ) : m_precondition( p ) {}

private:
    // decorator::base interface
    void            apply( test_unit& tu ) BOOST_OVERRIDE;
    base_ptr        clone() const BOOST_OVERRIDE { return base_ptr(new precondition( m_precondition )); }

    // Data members
    predicate_t             m_precondition;
};

} // namespace decorator

using decorator::label;
using decorator::expected_failures;
using decorator::timeout;
using decorator::description;
using decorator::depends_on;
using decorator::enable_if;
using decorator::enabled;
using decorator::disabled;
using decorator::fixture;
using decorator::precondition;

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TREE_DECORATOR_HPP_091911GER
