///////////////////////////////////////////////////////////////////////////////
// as_quantifier.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_QUANTIFIER_HPP_EAN_04_01_2007
#define BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_QUANTIFIER_HPP_EAN_04_01_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/static/static.hpp>
#include <boost/proto/core.hpp>

namespace boost { namespace xpressive { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // generic_quant_tag
    template<uint_t Min, uint_t Max>
    struct generic_quant_tag
    {
        typedef mpl::integral_c<uint_t, Min> min_type;
        typedef mpl::integral_c<uint_t, Max> max_type;
    };
}}}

namespace boost { namespace xpressive { namespace grammar_detail
{
    using detail::uint_t;

    ///////////////////////////////////////////////////////////////////////////////
    // min_type / max_type
    template<typename Tag>
    struct min_type : Tag::min_type {};

    template<>
    struct min_type<proto::tag::unary_plus> : mpl::integral_c<uint_t, 1> {};

    template<>
    struct min_type<proto::tag::dereference> : mpl::integral_c<uint_t, 0> {};

    template<>
    struct min_type<proto::tag::logical_not> : mpl::integral_c<uint_t, 0> {};

    template<typename Tag>
    struct max_type : Tag::max_type {};

    template<>
    struct max_type<proto::tag::unary_plus> : mpl::integral_c<uint_t, UINT_MAX-1> {};

    template<>
    struct max_type<proto::tag::dereference> : mpl::integral_c<uint_t, UINT_MAX-1> {};

    template<>
    struct max_type<proto::tag::logical_not> : mpl::integral_c<uint_t, 1> {};

    ///////////////////////////////////////////////////////////////////////////////
    // as_simple_quantifier
    template<typename Grammar, typename Greedy, typename Callable = proto::callable>
    struct as_simple_quantifier : proto::transform<as_simple_quantifier<Grammar, Greedy, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                typename proto::result_of::child<Expr>::type
            arg_type;

            typedef
                typename Grammar::template impl<arg_type, detail::true_xpression, Data>::result_type
            xpr_type;

            typedef
                detail::simple_repeat_matcher<xpr_type, Greedy>
            matcher_type;

            typedef
                typename proto::terminal<matcher_type>::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                xpr_type xpr = typename Grammar::template impl<arg_type, detail::true_xpression, Data>()(
                    proto::child(expr)
                  , detail::true_xpression()
                  , data
                );

                typedef typename impl::expr expr_type;
                matcher_type matcher(
                    xpr
                  , (uint_t)min_type<typename expr_type::proto_tag>::value
                  , (uint_t)max_type<typename expr_type::proto_tag>::value
                  , xpr.get_width().value()
                );

                return result_type::make(matcher);
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // add_hidden_mark
    struct add_hidden_mark : proto::transform<add_hidden_mark>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::expr expr_type;
            typedef
                typename shift_right<
                    terminal<detail::mark_begin_matcher>::type
                  , typename shift_right<
                        Expr
                      , terminal<detail::mark_end_matcher>::type
                    >::type
                >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                // we're inserting a hidden mark ... so grab the next hidden mark number.
                int mark_nbr = data.get_hidden_mark();
                detail::mark_begin_matcher begin(mark_nbr);
                detail::mark_end_matcher end(mark_nbr);

                result_type that = {{begin}, {expr, {end}}};
                return that;
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // InsertMark
    struct InsertMark
      : or_<
            when<proto::assign<detail::basic_mark_tag, _>, _>
          , otherwise<add_hidden_mark>
        >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // as_default_quantifier_impl
    template<typename Greedy, uint_t Min, uint_t Max>
    struct as_default_quantifier_impl : proto::transform<as_default_quantifier_impl<Greedy, Min, Max> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                typename proto::result_of::child<Expr>::type
            xpr_type;

            typedef
                typename InsertMark::impl<xpr_type, State, Data>::result_type
            marked_sub_type;

            typedef
                typename shift_right<
                    terminal<detail::repeat_begin_matcher>::type
                  , typename shift_right<
                        marked_sub_type
                      , typename terminal<detail::repeat_end_matcher<Greedy> >::type
                    >::type
                >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param state
              , typename impl::data_param data
            ) const
            {
                // Ensure this sub-expression is book-ended with mark matchers
                marked_sub_type marked_sub =
                    InsertMark::impl<xpr_type, State, Data>()(proto::child(expr), state, data);

                // Get the mark_number from the begin_mark_matcher
                int mark_number = proto::value(proto::left(marked_sub)).mark_number_;
                BOOST_ASSERT(0 != mark_number);

                typedef typename impl::expr expr_type;
                uint_t min_ = (uint_t)min_type<typename expr_type::proto_tag>();
                uint_t max_ = (uint_t)max_type<typename expr_type::proto_tag>();

                detail::repeat_begin_matcher begin(mark_number);
                detail::repeat_end_matcher<Greedy> end(mark_number, min_, max_);

                result_type that = {{begin}, {marked_sub, {end}}};
                return that;
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // optional_tag
    template<typename Greedy>
    struct optional_tag
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // as_default_optional
    template<typename Grammar, typename Greedy, typename Callable = proto::callable>
    struct as_default_optional : proto::transform<as_default_optional<Grammar, Greedy, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                detail::alternate_end_xpression
            end_xpr;

            typedef
                detail::optional_matcher<
                    typename Grammar::template impl<Expr, end_xpr, Data>::result_type
                  , Greedy
                >
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                return result_type(
                    typename Grammar::template impl<Expr, end_xpr, Data>()(expr, end_xpr(), data)
                );
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // as_mark_optional
    template<typename Grammar, typename Greedy, typename Callable = proto::callable>
    struct as_mark_optional : proto::transform<as_mark_optional<Grammar, Greedy, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                detail::alternate_end_xpression
            end_xpr;

            typedef
                detail::optional_mark_matcher<
                    typename Grammar::template impl<Expr, end_xpr, Data>::result_type
                  , Greedy
                >
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                int mark_number = proto::value(proto::left(expr)).mark_number_;

                return result_type(
                    typename Grammar::template impl<Expr, end_xpr, Data>()(expr, end_xpr(), data)
                  , mark_number
                );
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // IsMarkerOrRepeater
    struct IsMarkerOrRepeater
      : or_<
            shift_right<terminal<detail::repeat_begin_matcher>, _>
          , assign<terminal<detail::mark_placeholder>, _>
        >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // as_optional
    template<typename Grammar, typename Greedy>
    struct as_optional
      : or_<
            when<IsMarkerOrRepeater, as_mark_optional<Grammar, Greedy> >
          , otherwise<as_default_optional<Grammar, Greedy> >
        >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // make_optional_
    template<typename Greedy, typename Callable = proto::callable>
    struct make_optional_ : proto::transform<make_optional_<Greedy, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::expr expr_type;
            typedef
                typename unary_expr<
                    optional_tag<Greedy>
                  , Expr
                >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param
            ) const
            {
                result_type that = {expr};
                return that;
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // as_default_quantifier_impl
    template<typename Greedy, uint_t Max>
    struct as_default_quantifier_impl<Greedy, 0, Max>
      : call<make_optional_<Greedy>(as_default_quantifier_impl<Greedy, 1, Max>)>
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // as_default_quantifier_impl
    template<typename Greedy>
    struct as_default_quantifier_impl<Greedy, 0, 1>
      : call<make_optional_<Greedy>(_child)>
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // as_default_quantifier
    template<typename Greedy, typename Callable = proto::callable>
    struct as_default_quantifier : proto::transform<as_default_quantifier<Greedy, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::expr expr_type;
            typedef
                as_default_quantifier_impl<
                    Greedy
                  , min_type<typename expr_type::proto_tag>::value
                  , max_type<typename expr_type::proto_tag>::value
                >
            other;

            typedef
                typename other::template impl<Expr, State, Data>::result_type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param state
              , typename impl::data_param data
            ) const
            {
                return typename other::template impl<Expr, State, Data>()(expr, state, data);
            }
        };
    };

}}}

#endif
