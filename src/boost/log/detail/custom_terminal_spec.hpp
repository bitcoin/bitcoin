/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   custom_terminal_spec.hpp
 * \author Andrey Semashev
 * \date   29.01.2012
 *
 * The header contains Boost.Phoenix custom terminal specialization for Boost.Log terminals.
 */

#ifndef BOOST_LOG_DETAIL_CUSTOM_TERMINAL_SPEC_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_CUSTOM_TERMINAL_SPEC_HPP_INCLUDED_

#include <boost/mpl/bool.hpp>
#include <boost/phoenix/core/terminal_fwd.hpp>
#include <boost/phoenix/core/is_nullary.hpp>
#include <boost/phoenix/core/terminal.hpp> // needed for terminal-related part of the grammar
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

namespace phoenix {

template< typename T >
struct is_custom_terminal< T, typename T::_is_boost_log_terminal > :
    public mpl::true_
{
};

template< typename T >
struct custom_terminal< T, typename T::_is_boost_log_terminal >
{
    typedef custom_terminal< T, typename T::_is_boost_log_terminal > this_type;

    template< typename >
    struct result;

    template< typename ThisT, typename TermT, typename ContextT >
    struct result< ThisT(TermT, ContextT) >
    {
        typedef typename remove_cv< typename remove_reference< TermT >::type >::type term;
        typedef typename boost::result_of< const term(ContextT) >::type type;
    };

    template< typename ContextT >
    typename result< const this_type(T const&, ContextT&) >::type operator() (T const& term, ContextT& ctx) const
    {
        return term(ctx);
    }
};

} // namespace phoenix

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_CUSTOM_TERMINAL_SPEC_HPP_INCLUDED_
