/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   nop.hpp
 * \author Andrey Semashev
 * \date   30.03.2008
 *
 * This header contains a function object that does nothing.
 */

#ifndef BOOST_LOG_UTILITY_FUNCTIONAL_NOP_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_FUNCTIONAL_NOP_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

//! The function object that does nothing
struct nop
{
    typedef void result_type;

    void operator() () const BOOST_NOEXCEPT {}

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template< typename... ArgsT >
    void operator() (ArgsT const&...) const BOOST_NOEXCEPT {}
#else
    template< typename T >
    void operator() (T const&) const BOOST_NOEXCEPT {}
    template< typename T1, typename T2 >
    void operator() (T1 const&, T2 const&) const BOOST_NOEXCEPT {}
    template< typename T1, typename T2, typename T3 >
    void operator() (T1 const&, T2 const&, T3 const&) const BOOST_NOEXCEPT {}
#endif
};

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_FUNCTIONAL_NOP_HPP_INCLUDED_
