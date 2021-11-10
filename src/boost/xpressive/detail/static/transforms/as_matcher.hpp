///////////////////////////////////////////////////////////////////////////////
// as_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_MATCHER_HPP_EAN_04_01_2007
#define BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_MATCHER_HPP_EAN_04_01_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/assert.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/static/static.hpp>

namespace boost { namespace xpressive { namespace grammar_detail
{
    struct as_matcher : proto::transform<as_matcher>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::data data_type;

            typedef
                typename data_type::template apply<
                    typename proto::result_of::value<typename impl::expr>::type
                >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                return data.call(proto::value(expr));
            }
        };
    };

}}}

#endif
