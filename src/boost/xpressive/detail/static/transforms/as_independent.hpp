///////////////////////////////////////////////////////////////////////////////
// as_independent.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_INDEPENDENT_HPP_EAN_04_05_2007
#define BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_INDEPENDENT_HPP_EAN_04_05_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/sizeof.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/static/static.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/transform/arg.hpp>
#include <boost/proto/transform/when.hpp>
#include <boost/proto/transform/fold.hpp>
#include <boost/proto/transform/fold_tree.hpp>

namespace boost { namespace xpressive { namespace detail
{
    struct keeper_tag
    {};

    struct lookahead_tag
    {};

    struct lookbehind_tag
    {};
}}}

namespace boost { namespace xpressive { namespace grammar_detail
{
    // A grammar that only accepts static regexes that
    // don't have semantic actions.
    struct NotHasAction
      : proto::switch_<struct NotHasActionCases>
    {};

    struct NotHasActionCases
    {
        template<typename Tag, int Dummy = 0>
        struct case_
          : proto::nary_expr<Tag, proto::vararg<NotHasAction> >
        {};

        template<int Dummy>
        struct case_<proto::tag::terminal, Dummy>
          : not_< or_<
                proto::terminal<detail::tracking_ptr<detail::regex_impl<_> > >,
                proto::terminal<reference_wrapper<_> >
            > >
        {};

        template<int Dummy>
        struct case_<proto::tag::comma, Dummy>
          : proto::_    // because (set='a','b') can't contain an action
        {};

        template<int Dummy>
        struct case_<proto::tag::complement, Dummy>
          : proto::_    // because in ~X, X can't contain an unscoped action
        {};

        template<int Dummy>
        struct case_<detail::lookahead_tag, Dummy>
          : proto::_    // because actions in lookaheads are scoped
        {};

        template<int Dummy>
        struct case_<detail::lookbehind_tag, Dummy>
          : proto::_    // because actions in lookbehinds are scoped
        {};

        template<int Dummy>
        struct case_<detail::keeper_tag, Dummy>
          : proto::_    // because actions in keepers are scoped
        {};

        template<int Dummy>
        struct case_<proto::tag::subscript, Dummy>
          : proto::subscript<detail::set_initializer_type, _>
        {}; // only accept set[...], not actions!
    };

    struct IndependentEndXpression
      : or_<
            when<NotHasAction, detail::true_xpression()>
          , otherwise<detail::independent_end_xpression()>
        >
    {};

    template<typename Grammar, typename Callable = proto::callable>
    struct as_lookahead : proto::transform<as_lookahead<Grammar, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::result_of::child<Expr>::type arg_type;

            typedef
                typename IndependentEndXpression::impl<arg_type, int, int>::result_type
            end_xpr_type;

            typedef
                typename Grammar::template impl<arg_type, end_xpr_type, Data>::result_type
            xpr_type;

            typedef
                detail::lookahead_matcher<xpr_type>
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                int i = 0;
                return result_type(
                    typename Grammar::template impl<arg_type, end_xpr_type, Data>()(
                        proto::child(expr)
                      , IndependentEndXpression::impl<arg_type, int, int>()(proto::child(expr), i, i)
                      , data
                    )
                  , false
                );
            }
        };
    };

    template<typename Grammar, typename Callable = proto::callable>
    struct as_lookbehind : proto::transform<as_lookbehind<Grammar, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::result_of::child<Expr>::type arg_type;

            typedef
                typename IndependentEndXpression::impl<arg_type, int, int>::result_type
            end_xpr_type;

            typedef
                typename Grammar::template impl<arg_type, end_xpr_type, Data>::result_type
            xpr_type;

            typedef
                detail::lookbehind_matcher<xpr_type>
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                int i = 0;
                xpr_type expr2 = typename Grammar::template impl<arg_type, end_xpr_type, Data>()(
                    proto::child(expr)
                  , IndependentEndXpression::impl<arg_type, int, int>()(proto::child(expr), i, i)
                  , data
                );
                std::size_t width = expr2.get_width().value();
                return result_type(expr2, width, false);
            }
        };
    };

    template<typename Grammar, typename Callable = proto::callable>
    struct as_keeper : proto::transform<as_keeper<Grammar, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::result_of::child<Expr>::type arg_type;

            typedef
                typename IndependentEndXpression::impl<arg_type, int, int>::result_type
            end_xpr_type;

            typedef
                typename Grammar::template impl<arg_type, end_xpr_type, Data>::result_type
            xpr_type;

            typedef
                detail::keeper_matcher<xpr_type>
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                int i = 0;
                return result_type(
                    typename Grammar::template impl<arg_type, end_xpr_type, Data>()(
                        proto::child(expr)
                      , IndependentEndXpression::impl<arg_type, int, int>()(proto::child(expr), i, i)
                      , data
                    )
                );
            }
        };
    };

}}}

#endif
