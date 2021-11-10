///////////////////////////////////////////////////////////////////////////////
// as_marker.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_MARKER_HPP_EAN_04_01_2007
#define BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_MARKER_HPP_EAN_04_01_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/static/static.hpp>
#include <boost/proto/core.hpp>

namespace boost { namespace xpressive { namespace grammar_detail
{

    ///////////////////////////////////////////////////////////////////////////////
    // as_marker
    //   Insert mark tags before and after the expression
    struct as_marker : proto::transform<as_marker>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                typename shift_right<
                    terminal<detail::mark_begin_matcher>::type
                  , typename shift_right<
                        typename proto::result_of::right<typename impl::expr>::type
                      , terminal<detail::mark_end_matcher>::type
                    >::type
                >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param
            ) const
            {
                int mark_nbr = detail::get_mark_number(proto::left(expr));
                detail::mark_begin_matcher begin(mark_nbr);
                detail::mark_end_matcher end(mark_nbr);

                result_type that = {{begin}, {proto::right(expr), {end}}};
                return that;
            }
        };
    };
}}}

#endif
