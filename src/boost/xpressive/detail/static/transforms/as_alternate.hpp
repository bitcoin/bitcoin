///////////////////////////////////////////////////////////////////////////////
// as_alternate.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_ALTERNATE_HPP_EAN_04_01_2007
#define BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_ALTERNATE_HPP_EAN_04_01_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/proto/core.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/static/static.hpp>
#include <boost/xpressive/detail/core/matcher/alternate_matcher.hpp>
#include <boost/xpressive/detail/utility/cons.hpp>

namespace boost { namespace xpressive
{
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////////////
        // alternates_list
        //   a fusion-compatible sequence of alternate expressions, that also keeps
        //   track of the list's width and purity.
        template<typename Head, typename Tail>
        struct alternates_list
          : fusion::cons<Head, Tail>
        {
            BOOST_STATIC_CONSTANT(std::size_t, width = Head::width == Tail::width ? Head::width : detail::unknown_width::value);
            BOOST_STATIC_CONSTANT(bool, pure = Head::pure && Tail::pure);

            alternates_list(Head const &head, Tail const &tail)
              : fusion::cons<Head, Tail>(head, tail)
            {
            }
        };

        template<typename Head>
        struct alternates_list<Head, fusion::nil>
          : fusion::cons<Head, fusion::nil>
        {
            BOOST_STATIC_CONSTANT(std::size_t, width = Head::width);
            BOOST_STATIC_CONSTANT(bool, pure = Head::pure);

            alternates_list(Head const &head, fusion::nil const &tail)
              : fusion::cons<Head, fusion::nil>(head, tail)
            {
            }
        };
    }

    namespace grammar_detail
    {
        ///////////////////////////////////////////////////////////////////////////////
        // in_alternate_list
        template<typename Grammar, typename Callable = proto::callable>
        struct in_alternate_list : proto::transform<in_alternate_list<Grammar, Callable> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl : proto::transform_impl<Expr, State, Data>
            {
                typedef
                    detail::alternates_list<
                        typename Grammar::template impl<
                            Expr
                          , detail::alternate_end_xpression
                          , Data
                        >::result_type
                      , State
                    >
                result_type;

                result_type operator ()(
                    typename impl::expr_param expr
                  , typename impl::state_param state
                  , typename impl::data_param data
                ) const
                {
                    return result_type(
                        typename Grammar::template impl<Expr, detail::alternate_end_xpression, Data>()(
                            expr
                          , detail::alternate_end_xpression()
                          , data
                        )
                      , state
                    );
                }
            };
        };

        ///////////////////////////////////////////////////////////////////////////////
        // as_alternate_matcher
        template<typename Grammar, typename Callable = proto::callable>
        struct as_alternate_matcher : proto::transform<as_alternate_matcher<Grammar, Callable> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl : proto::transform_impl<Expr, State, Data>
            {
                typedef typename impl::data data_type;
                typedef
                    detail::alternate_matcher<
                        typename Grammar::template impl<Expr, State, Data>::result_type
                      , typename data_type::traits_type
                    >
                result_type;

                result_type operator ()(
                    typename impl::expr_param expr
                  , typename impl::state_param state
                  , typename impl::data_param data
                ) const
                {
                    return result_type(
                        typename Grammar::template impl<Expr, State, Data>()(expr, state, data)
                    );
                }
            };
        };
    }

}}

#endif
