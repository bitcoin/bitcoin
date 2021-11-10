///////////////////////////////////////////////////////////////////////////////
/// \file as_expr.hpp
/// Contains definition of the as_expr\<\> and as_child\<\> helper class
/// templates used to implement proto::domain's as_expr\<\> and as_child\<\>
/// member templates.
//
//  Copyright 2010 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_DETAIL_AS_EXPR_HPP_EAN_06_09_2010
#define BOOST_PROTO_DETAIL_AS_EXPR_HPP_EAN_06_09_2010

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/args.hpp>

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable : 4714) // function 'xxx' marked as __forceinline not inlined
#endif

namespace boost { namespace proto { namespace detail
{

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename Generator>
    struct base_generator
    {
        typedef Generator type;
    };

    template<typename Generator>
    struct base_generator<use_basic_expr<Generator> >
    {
        typedef Generator type;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename Generator, bool WantsBasicExpr>
    struct as_expr;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename Generator>
    struct as_expr<T, Generator, false>
    {
        typedef typename term_traits<T &>::value_type value_type;
        typedef proto::expr<proto::tag::terminal, term<value_type>, 0> expr_type;
        typedef typename Generator::template result<Generator(expr_type)>::type result_type;

        BOOST_FORCEINLINE
        result_type operator()(T &t) const
        {
            return Generator()(expr_type::make(t));
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename Generator>
    struct as_expr<T, Generator, true>
    {
        typedef typename term_traits<T &>::value_type value_type;
        typedef proto::basic_expr<proto::tag::terminal, term<value_type>, 0> expr_type;
        typedef typename Generator::template result<Generator(expr_type)>::type result_type;

        BOOST_FORCEINLINE
        result_type operator()(T &t) const
        {
            return Generator()(expr_type::make(t));
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T>
    struct as_expr<T, proto::default_generator, false>
    {
        typedef typename term_traits<T &>::value_type value_type;
        typedef proto::expr<proto::tag::terminal, term<value_type>, 0> result_type;

        BOOST_FORCEINLINE
        result_type operator()(T &t) const
        {
            return result_type::make(t);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T>
    struct as_expr<T, proto::default_generator, true>
    {
        typedef typename term_traits<T &>::value_type value_type;
        typedef proto::basic_expr<proto::tag::terminal, term<value_type>, 0> result_type;

        BOOST_FORCEINLINE
        result_type operator()(T &t) const
        {
            return result_type::make(t);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename Generator, bool WantsBasicExpr>
    struct as_child;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename Generator>
    struct as_child<T, Generator, false>
    {
    #if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
        typedef typename term_traits<T &>::reference reference;
    #else
        typedef T &reference;
    #endif
        typedef proto::expr<proto::tag::terminal, term<reference>, 0> expr_type;
        typedef typename Generator::template result<Generator(expr_type)>::type result_type;

        BOOST_FORCEINLINE
        result_type operator()(T &t) const
        {
            return Generator()(expr_type::make(t));
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename Generator>
    struct as_child<T, Generator, true>
    {
    #if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
        typedef typename term_traits<T &>::reference reference;
    #else
        typedef T &reference;
    #endif
        typedef proto::basic_expr<proto::tag::terminal, term<reference>, 0> expr_type;
        typedef typename Generator::template result<Generator(expr_type)>::type result_type;

        BOOST_FORCEINLINE
        result_type operator()(T &t) const
        {
            return Generator()(expr_type::make(t));
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T>
    struct as_child<T, proto::default_generator, false>
    {
    #if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
        typedef typename term_traits<T &>::reference reference;
    #else
        typedef T &reference;
    #endif
        typedef proto::expr<proto::tag::terminal, term<reference>, 0> result_type;

        BOOST_FORCEINLINE
        result_type operator()(T &t) const
        {
            return result_type::make(t);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T>
    struct as_child<T, proto::default_generator, true>
    {
    #if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
        typedef typename term_traits<T &>::reference reference;
    #else
        typedef T &reference;
    #endif
        typedef proto::basic_expr<proto::tag::terminal, term<reference>, 0> result_type;

        BOOST_FORCEINLINE
        result_type operator()(T &t) const
        {
            return result_type::make(t);
        }
    };

}}}

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif
