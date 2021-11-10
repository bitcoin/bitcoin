/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   bind.hpp
 * \author Andrey Semashev
 * \date   30.03.2008
 *
 * This header contains function object adapters.
 * This is a lightweight alternative to what Boost.Phoenix and Boost.Bind provides.
 */

#ifndef BOOST_LOG_UTILITY_FUNCTIONAL_BIND_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_FUNCTIONAL_BIND_HPP_INCLUDED_

#include <boost/type_traits/remove_cv.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

template< typename T >
struct make_arg_type
{
    typedef T const& type;
};

template< typename T >
struct make_arg_type< T& >
{
    typedef T& type;
};

} // namespace aux

//! First argument binder
template< typename FunT, typename FirstArgT >
struct binder1st :
    private FunT
{
    typedef typename FunT::result_type result_type;

    binder1st(FunT const& fun, typename aux::make_arg_type< FirstArgT >::type arg) : FunT(fun), m_arg(arg) {}

    result_type operator() () const
    {
        return FunT::operator()(m_arg);
    }

    template< typename T0 >
    result_type operator() (T0 const& arg0) const
    {
        return FunT::operator()(m_arg, arg0);
    }

    template< typename T0, typename T1 >
    result_type operator() (T0 const& arg0, T1 const& arg1) const
    {
        return FunT::operator()(m_arg, arg0, arg1);
    }

private:
    FirstArgT m_arg;
};

//! First argument binder
template< typename FunT, typename FirstArgT >
struct binder1st< FunT&, FirstArgT >
{
    typedef typename remove_cv< FunT >::type::result_type result_type;

    binder1st(FunT& fun, typename aux::make_arg_type< FirstArgT >::type arg) : m_fun(fun), m_arg(arg) {}

    result_type operator() () const
    {
        return m_fun(m_arg);
    }

    template< typename T0 >
    result_type operator() (T0 const& arg0) const
    {
        return m_fun(m_arg, arg0);
    }

    template< typename T0, typename T1 >
    result_type operator() (T0 const& arg0, T1 const& arg1) const
    {
        return m_fun(m_arg, arg0, arg1);
    }

private:
    FunT& m_fun;
    FirstArgT m_arg;
};

template< typename FunT, typename FirstArgT >
BOOST_FORCEINLINE binder1st< FunT, FirstArgT > bind1st(FunT fun, FirstArgT const& arg)
{
    return binder1st< FunT, FirstArgT >(fun, arg);
}

template< typename FunT, typename FirstArgT >
BOOST_FORCEINLINE binder1st< FunT, FirstArgT > bind1st(FunT fun, FirstArgT& arg)
{
    return binder1st< FunT, FirstArgT >(fun, arg);
}

//! Second argument binder
template< typename FunT, typename SecondArgT >
struct binder2nd :
    private FunT
{
    typedef typename FunT::result_type result_type;

    binder2nd(FunT const& fun, typename aux::make_arg_type< SecondArgT >::type arg) : FunT(fun), m_arg(arg) {}

    template< typename T >
    result_type operator() (T const& arg) const
    {
        return FunT::operator()(arg, m_arg);
    }

    template< typename T0, typename T1 >
    result_type operator() (T0 const& arg0, T1 const& arg1) const
    {
        return FunT::operator()(arg0, m_arg, arg1);
    }

private:
    SecondArgT m_arg;
};

//! Second argument binder
template< typename FunT, typename SecondArgT >
struct binder2nd< FunT&, SecondArgT >
{
    typedef typename remove_cv< FunT >::type::result_type result_type;

    binder2nd(FunT& fun, typename aux::make_arg_type< SecondArgT >::type arg) : m_fun(fun), m_arg(arg) {}

    template< typename T >
    result_type operator() (T const& arg) const
    {
        return m_fun(arg, m_arg);
    }

    template< typename T0, typename T1 >
    result_type operator() (T0 const& arg0, T1 const& arg1) const
    {
        return m_fun(arg0, m_arg, arg1);
    }

private:
    FunT& m_fun;
    SecondArgT m_arg;
};

template< typename FunT, typename SecondArgT >
BOOST_FORCEINLINE binder2nd< FunT, SecondArgT > bind2nd(FunT fun, SecondArgT const& arg)
{
    return binder2nd< FunT, SecondArgT >(fun, arg);
}

template< typename FunT, typename SecondArgT >
BOOST_FORCEINLINE binder2nd< FunT, SecondArgT > bind2nd(FunT fun, SecondArgT& arg)
{
    return binder2nd< FunT, SecondArgT >(fun, arg);
}

//! Third argument binder
template< typename FunT, typename ThirdArgT >
struct binder3rd :
    private FunT
{
    typedef typename FunT::result_type result_type;

    binder3rd(FunT const& fun, typename aux::make_arg_type< ThirdArgT >::type arg) : FunT(fun), m_arg(arg) {}

    template< typename T0, typename T1 >
    result_type operator() (T0 const& arg0, T1 const& arg1) const
    {
        return FunT::operator()(arg0, arg1, m_arg);
    }

private:
    ThirdArgT m_arg;
};

//! Third argument binder
template< typename FunT, typename ThirdArgT >
struct binder3rd< FunT&, ThirdArgT >
{
    typedef typename remove_cv< FunT >::type::result_type result_type;

    binder3rd(FunT& fun, typename aux::make_arg_type< ThirdArgT >::type arg) : m_fun(fun), m_arg(arg) {}

    template< typename T0, typename T1 >
    result_type operator() (T0 const& arg0, T1 const& arg1) const
    {
        return m_fun(arg0, arg1, m_arg);
    }

private:
    FunT& m_fun;
    ThirdArgT m_arg;
};

template< typename FunT, typename ThirdArgT >
BOOST_FORCEINLINE binder3rd< FunT, ThirdArgT > bind3rd(FunT fun, ThirdArgT const& arg)
{
    return binder3rd< FunT, ThirdArgT >(fun, arg);
}

template< typename FunT, typename ThirdArgT >
BOOST_FORCEINLINE binder3rd< FunT, ThirdArgT > bind3rd(FunT fun, ThirdArgT& arg)
{
    return binder3rd< FunT, ThirdArgT >(fun, arg);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_FUNCTIONAL_BIND_HPP_INCLUDED_
