///////////////////////////////////////////////////////////////////////////////
// as_modifier.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_MODIFIER_HPP_EAN_04_05_2007
#define BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_MODIFIER_HPP_EAN_04_05_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/sizeof.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/static/static.hpp>
#include <boost/proto/core.hpp>

#define UNCV(x) typename remove_const<x>::type
#define UNREF(x) typename remove_reference<x>::type
#define UNCVREF(x) UNCV(UNREF(x))

namespace boost { namespace xpressive { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // regex operator tags
    struct modifier_tag
    {};

}}}

namespace boost { namespace xpressive { namespace grammar_detail
{

    ///////////////////////////////////////////////////////////////////////////////
    // as_modifier
    template<typename Grammar, typename Callable = proto::callable>
    struct as_modifier : proto::transform<as_modifier<Grammar, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                typename proto::result_of::value<
                    typename proto::result_of::left<typename impl::expr>::type
                >::type
            modifier_type;

            typedef
                typename modifier_type::template apply<typename impl::data>::type
            visitor_type;

            typedef
                typename proto::result_of::right<Expr>::type
            expr_type;

            typedef
                typename Grammar::template impl<expr_type, State, visitor_type &>::result_type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param state
              , typename impl::data_param data
            ) const
            {
                visitor_type new_visitor(proto::value(proto::left(expr)).call(data));
                return typename Grammar::template impl<expr_type, State, visitor_type &>()(
                    proto::right(expr)
                  , state
                  , new_visitor
                );
            }
        };
    };

}}}

#undef UNCV
#undef UNREF
#undef UNCVREF

#endif
