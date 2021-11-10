//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 74248 $
//
//  Description : inidiration interfaces to support manipulators and message output
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_DETAIL_INDIRECTIONS_HPP_112812GER
#define BOOST_TEST_TOOLS_DETAIL_INDIRECTIONS_HPP_112812GER

// Boost.Test
#include <boost/test/tools/detail/fwd.hpp>

#include <boost/test/tools/assertion_result.hpp>
#include <boost/test/utils/lazy_ostream.hpp>

#include <boost/shared_ptr.hpp>
#include <list>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {
namespace tt_detail {

struct assertion_evaluation_context 
{
    assertion_evaluation_context(bool has_report = false)
    : m_has_report(has_report)
    {}

    bool m_has_report;
};

// ************************************************************************** //
// **************        assertion_evaluate indirection        ************** //
// ************************************************************************** //

template<typename E>
struct assertion_evaluate_t {

    typedef shared_ptr<assertion_evaluation_context> context_holder;

    assertion_evaluate_t( E const& e ) : m_e( e ), m_evaluate( true ) 
    {}
    
    operator assertion_result() { return m_e.evaluate( m_evaluate ); }

    assertion_evaluate_t<E> 
    stack_context(context_holder context) const {
        assertion_evaluate_t<E> added_context(*this);
    
        added_context.m_context_holder.push_back(context);
        added_context.m_evaluate = !context->m_has_report;
        return added_context;
    }

    E const& m_e;
    std::list< context_holder > m_context_holder;
    bool m_evaluate;
};

//____________________________________________________________________________//

template<typename E>
inline assertion_evaluate_t<E>
assertion_evaluate( E const& e ) { return assertion_evaluate_t<E>( e ); }

//____________________________________________________________________________//

template<typename E, typename T>
inline assertion_evaluate_t<E>
operator<<( assertion_evaluate_t<E> const& ae, T const& ) { return ae; }

//____________________________________________________________________________//

// ************************************************************************** //
// **************          assertion_text indirection          ************** //
// ************************************************************************** //

inline unit_test::lazy_ostream const&
assertion_text( unit_test::lazy_ostream const& et, unit_test::lazy_ostream const& s) { 
    if(!s.empty())
        return s;
    return et; 
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************        assertion_evaluate indirection        ************** //
// ************************************************************************** //

struct assertion_type {
    assertion_type(check_type ct = CHECK_MSG) : m_check_type(ct)
    {}

    operator check_type() { return m_check_type; }
    check_type m_check_type;
};

//____________________________________________________________________________//

template<typename T>
inline assertion_type
operator<<( assertion_type const& at, T const& ) { return at; }

//____________________________________________________________________________//

} // namespace tt_detail
} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_DETAIL_INDIRECTIONS_HPP_112812GER
