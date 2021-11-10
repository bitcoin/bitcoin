///////////////////////////////////////////////////////////////////////////////
// as_action.hpp
//
//  Copyright 2008 Eric Niebler.
//  Copyright 2008 David Jenkins.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_ACTION_HPP_EAN_04_05_2007
#define BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_ACTION_HPP_EAN_04_05_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/sizeof.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/mpl/apply_wrap.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/matcher/attr_end_matcher.hpp>
#include <boost/xpressive/detail/static/static.hpp>
#include <boost/xpressive/detail/static/transforms/as_quantifier.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/transform/arg.hpp>
#include <boost/proto/transform/call.hpp>
#include <boost/proto/transform/make.hpp>
#include <boost/proto/transform/when.hpp>
#include <boost/proto/transform/fold.hpp>
#include <boost/proto/transform/fold_tree.hpp>

namespace boost { namespace xpressive { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // read_attr
    //  Placeholder that knows the slot number of an attribute as well as the type
    //  of the object stored in it.
    template<typename Nbr, typename Matcher>
    struct read_attr
    {
        typedef Nbr nbr_type;
        typedef Matcher matcher_type;
        static Nbr nbr() { return Nbr(); }
    };

    template<typename Nbr, typename Matcher>
    struct read_attr<Nbr, Matcher &>
    {
        typedef Nbr nbr_type;
        typedef Matcher matcher_type;
    };

}}}

namespace boost { namespace xpressive { namespace grammar_detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // FindAttr
    //  Look for patterns like (a1= terminal<RHS>) and return the type of the RHS.
    template<typename Nbr>
    struct FindAttr
      : or_<
            // Ignore nested actions, because attributes are scoped
            when< subscript<_, _>,                                  _state >
          , when< terminal<_>,                                      _state >
          , when< proto::assign<terminal<detail::attribute_placeholder<Nbr> >, _>, call<_value(_right)> >
          , otherwise< fold<_, _state, FindAttr<Nbr> > >
        >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // as_read_attr
    //  For patterns like (a1 = RHS)[ref(i) = a1], transform to
    //  (a1 = RHS)[ref(i) = read_attr<1, RHS>] so that when reading the attribute
    //  we know what type is stored in the attribute slot.
    struct as_read_attr : proto::transform<as_read_attr>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::expr expr_type;
            typedef
                typename FindAttr<typename expr_type::proto_child0::nbr_type>::template impl<
                    State
                  , mpl::void_
                  , int
                >::result_type
            attr_type;

            typedef
                typename proto::terminal<
                    detail::read_attr<
                        typename expr_type::proto_child0::nbr_type
                      , BOOST_PROTO_UNCVREF(attr_type)
                    >
                >::type
            result_type;

            result_type operator ()(proto::ignore, proto::ignore, proto::ignore) const
            {
                result_type that = {{}};
                return that;
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // DeepCopy
    //  Turn all refs into values, and also bind all attribute placeholders with
    //  the types from which they are being assigned.
    struct DeepCopy
      : or_<
            when< terminal<detail::attribute_placeholder<_> >,  as_read_attr>
          , when< terminal<_>,                                  proto::_deep_copy>
          , otherwise< nary_expr<_, vararg<DeepCopy> > >
        >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // attr_nbr
    //  For an attribute placeholder, return the attribute's slot number.
    struct attr_nbr : proto::transform<attr_nbr>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::expr expr_type;
            typedef typename expr_type::proto_child0::nbr_type::type result_type;
        };
    };

    struct max_attr;

    ///////////////////////////////////////////////////////////////////////////////
    // MaxAttr
    //  In an action (rx)[act], find the largest attribute slot being used.
    struct MaxAttr
      : or_<
            when< terminal<detail::attribute_placeholder<_> >, attr_nbr>
          , when< terminal<_>, make<mpl::int_<0> > >
            // Ignore nested actions, because attributes are scoped:
          , when< subscript<_, _>, make<mpl::int_<0> > >
          , otherwise< fold<_, make<mpl::int_<0> >, max_attr> >
        >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // max_attr
    //  Take the maximum of the current attr slot number and the state.
    struct max_attr : proto::transform<max_attr>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                typename mpl::max<
                    typename impl::state
                  , typename MaxAttr::template impl<Expr, State, Data>::result_type
                >::type
            result_type;
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // as_attr_matcher
    //  turn a1=matcher into attr_matcher<Matcher>(1)
    struct as_attr_matcher : proto::transform<as_attr_matcher>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::expr expr_type;
            typedef typename impl::data data_type;
            typedef
                detail::attr_matcher<
                    typename proto::result_of::value<typename expr_type::proto_child1>::type
                  , typename data_type::traits_type
                  , typename data_type::icase_type
                >
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                return result_type(
                    proto::value(proto::left(expr)).nbr()
                  , proto::value(proto::right(expr))
                  , data.traits()
                );
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // add_attrs
    //  Wrap an expression in attr_begin_matcher/attr_end_matcher pair
    struct add_attrs : proto::transform<add_attrs>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                detail::attr_begin_matcher<
                    typename MaxAttr::template impl<Expr, mpl::int_<0>, int>::result_type
                >
            begin_type;

            typedef typename impl::expr expr_type;

            typedef
                typename shift_right<
                    typename terminal<begin_type>::type
                  , typename shift_right<
                        Expr
                      , terminal<detail::attr_end_matcher>::type
                    >::type
                >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param
            ) const
            {
                begin_type begin;
                detail::attr_end_matcher end;
                result_type that = {{begin}, {expr, {end}}};
                return that;
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // InsertAttrs
    struct InsertAttrs
      : if_<MaxAttr, add_attrs, _>
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // CheckAssertion
    struct CheckAssertion
      : proto::function<terminal<detail::check_tag>, _>
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // action_transform
    //  Turn A[B] into (mark_begin(n) >> A >> mark_end(n) >> action_matcher<B>(n))
    //  If A and B use attributes, wrap the above expression in
    //  a attr_begin_matcher<Count> / attr_end_matcher pair, where Count is
    //  the number of attribute slots used by the pattern/action.
    struct as_action : proto::transform<as_action>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::result_of::left<Expr>::type expr_type;
            typedef typename proto::result_of::right<Expr>::type action_type;

            typedef
                typename DeepCopy::impl<action_type, expr_type, int>::result_type
            action_copy_type;

            typedef
                typename InsertMark::impl<expr_type, State, Data>::result_type
            marked_expr_type;

            typedef
                typename mpl::if_c<
                    proto::matches<action_type, CheckAssertion>::value
                  , detail::predicate_matcher<action_copy_type>
                  , detail::action_matcher<action_copy_type>
                >::type
            matcher_type;

            typedef
                typename proto::shift_right<
                    marked_expr_type
                  , typename proto::terminal<matcher_type>::type
                >::type
            no_attr_type;

            typedef
                typename InsertAttrs::impl<no_attr_type, State, Data>::result_type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param state
              , typename impl::data_param data
            ) const
            {
                int dummy = 0;
                marked_expr_type marked_expr =
                    InsertMark::impl<expr_type, State, Data>()(proto::left(expr), state, data);

                no_attr_type that = {
                    marked_expr
                  , {
                        matcher_type(
                            DeepCopy::impl<action_type, expr_type, int>()(
                                proto::right(expr)
                              , proto::left(expr)
                              , dummy
                            )
                          , proto::value(proto::left(marked_expr)).mark_number_
                        )
                    }
                };
                return InsertAttrs::impl<no_attr_type, State, Data>()(that, state, data);
            }
        };
    };

}}}

#endif
