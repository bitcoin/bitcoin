#ifndef BOOST_BIND_APPLY_HPP_INCLUDED
#define BOOST_BIND_APPLY_HPP_INCLUDED

//
//  apply.hpp
//
//  Copyright (c) 2002, 2003 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/config.hpp>

namespace boost
{

template<class R> struct apply
{
    typedef R result_type;

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

    template<class F, class... A> result_type operator()( F&& f, A&&... a ) const
    {
        return static_cast<F&&>( f )( static_cast<A&&>( a )... );
    }

#else

    template<class F> result_type operator()(F & f) const
    {
        return f();
    }

    template<class F, class A1> result_type operator()(F & f, A1 & a1) const
    {
        return f(a1);
    }

    template<class F, class A1, class A2> result_type operator()(F & f, A1 & a1, A2 & a2) const
    {
        return f(a1, a2);
    }

    template<class F, class A1, class A2, class A3> result_type operator()(F & f, A1 & a1, A2 & a2, A3 & a3) const
    {
        return f(a1, a2, a3);
    }

    template<class F, class A1, class A2, class A3, class A4> result_type operator()(F & f, A1 & a1, A2 & a2, A3 & a3, A4 & a4) const
    {
        return f(a1, a2, a3, a4);
    }

    template<class F, class A1, class A2, class A3, class A4, class A5> result_type operator()(F & f, A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5) const
    {
        return f(a1, a2, a3, a4, a5);
    }

    template<class F, class A1, class A2, class A3, class A4, class A5, class A6> result_type operator()(F & f, A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6) const
    {
        return f(a1, a2, a3, a4, a5, a6);
    }

    template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7> result_type operator()(F & f, A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7) const
    {
        return f(a1, a2, a3, a4, a5, a6, a7);
    }

    template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> result_type operator()(F & f, A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8) const
    {
        return f(a1, a2, a3, a4, a5, a6, a7, a8);
    }

    template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> result_type operator()(F & f, A1 & a1, A2 & a2, A3 & a3, A4 & a4, A5 & a5, A6 & a6, A7 & a7, A8 & a8, A9 & a9) const
    {
        return f(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    }

#endif
};

} // namespace boost

#endif // #ifndef BOOST_BIND_APPLY_HPP_INCLUDED
