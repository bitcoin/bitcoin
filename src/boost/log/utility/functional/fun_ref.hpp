/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   fun_ref.hpp
 * \author Andrey Semashev
 * \date   30.03.2008
 *
 * This header contains function object reference adapter. The adapter stores a reference to external
 * function object and forwards all calls to the referred function.
 */

#ifndef BOOST_LOG_UTILITY_FUNCTIONAL_FUN_REF_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_FUNCTIONAL_FUN_REF_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

//! Reference wrapper for function objects
template< typename FunT >
struct function_reference_wrapper
{
    typedef typename FunT::result_type result_type;

    explicit function_reference_wrapper(FunT& fun) : m_Fun(fun) {}

    result_type operator() () const
    {
        return m_Fun();
    }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template< typename... ArgsT >
    result_type operator() (ArgsT const&... args) const
    {
        return m_Fun(args...);
    }
#else
    template< typename T >
    result_type operator() (T const& arg) const
    {
        return m_Fun(arg);
    }

    template< typename T1, typename T2 >
    result_type operator() (T1 const& arg1, T2 const& arg2) const
    {
        return m_Fun(arg1, arg2);
    }
#endif

private:
    FunT& m_Fun;
};

template< typename FunT >
BOOST_FORCEINLINE function_reference_wrapper< FunT > fun_ref(FunT& fun)
{
    return function_reference_wrapper< FunT >(fun);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_FUNCTIONAL_FUN_REF_HPP_INCLUDED_
