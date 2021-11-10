///////////////////////////////////////////////////////////////////////////////
// is_pure.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_IS_PURE_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_STATIC_IS_PURE_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/ref.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/not_equal_to.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/static/width_of.hpp>

namespace boost { namespace xpressive { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // use_simple_repeat_terminal
    //
    template<typename Expr, typename Char, bool IsXpr = is_xpr<Expr>::value>
    struct use_simple_repeat_terminal
      : mpl::bool_<
            Expr::quant == quant_fixed_width
        || (Expr::width != unknown_width::value && Expr::pure)
        >
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_terminal<Expr, Char, false>
      : mpl::true_              // char literals, string literals, etc.
    {};

    template<typename BidiIter, typename Char>
    struct use_simple_repeat_terminal<tracking_ptr<regex_impl<BidiIter> >, Char, false>
      : mpl::false_             // basic_regex
    {};

    template<typename BidiIter, typename Char>
    struct use_simple_repeat_terminal<reference_wrapper<basic_regex<BidiIter> >, Char, false>
      : mpl::false_             // basic_regex
    {};

    template<typename BidiIter, typename Char>
    struct use_simple_repeat_terminal<reference_wrapper<basic_regex<BidiIter> const>, Char, false>
      : mpl::false_             // basic_regex
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // use_simple_repeat_
    //
    template<typename Expr, typename Char, typename Tag = typename Expr::proto_tag>
    struct use_simple_repeat_
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::terminal>
      : use_simple_repeat_terminal<typename proto::result_of::value<Expr>::type, Char>
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::shift_right>
      : mpl::and_<
            use_simple_repeat_<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char>
          , use_simple_repeat_<typename remove_reference<typename Expr::proto_child1>::type::proto_base_expr, Char>
        >
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::bitwise_or>
      : mpl::and_<
            mpl::not_equal_to<unknown_width, width_of<Expr, Char> >
          , use_simple_repeat_<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char>
          , use_simple_repeat_<typename remove_reference<typename Expr::proto_child1>::type::proto_base_expr, Char>
        >
    {};

    template<typename Left>
    struct use_simple_repeat_assign
    {};

    template<>
    struct use_simple_repeat_assign<mark_placeholder>
      : mpl::false_
    {};

    template<>
    struct use_simple_repeat_assign<set_initializer>
      : mpl::true_
    {};

    template<typename Nbr>
    struct use_simple_repeat_assign<attribute_placeholder<Nbr> >
      : mpl::false_
    {};

    // either (s1 = ...) or (a1 = ...) or (set = ...)
    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::assign>
      : use_simple_repeat_assign<
            typename proto::result_of::value<
                typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr
            >::type
        >
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, modifier_tag>
      : use_simple_repeat_<typename remove_reference<typename Expr::proto_child1>::type::proto_base_expr, Char>
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, lookahead_tag>
      : mpl::false_
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, lookbehind_tag>
      : mpl::false_
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, keeper_tag>
      : mpl::false_
    {};

    // when complementing a set or an assertion, the purity is that of the set (true) or the assertion
    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::complement>
      : use_simple_repeat_<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char>
    {};

    // The comma is used in list-initialized sets, which are pure
    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::comma>
      : mpl::true_
    {};

    // The subscript operator[] is used for sets, as in set['a' | range('b','h')]
    // It is also used for actions, which by definition have side-effects and thus are impure
    template<typename Expr, typename Char, typename Left>
    struct use_simple_repeat_subscript
      : mpl::false_
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_subscript<Expr, Char, set_initializer_type>
      : mpl::true_
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::subscript>
      : use_simple_repeat_subscript<Expr, Char, typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr>
    {};

    // Quantified expressions are variable-width and cannot use the simple quantifier
    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::unary_plus>
      : mpl::false_
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::dereference>
      : mpl::false_
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::logical_not>
      : mpl::false_
    {};

    template<typename Expr, typename Char, uint_t Min, uint_t Max>
    struct use_simple_repeat_<Expr, Char, generic_quant_tag<Min, Max> >
      : mpl::false_
    {};

    template<typename Expr, typename Char, uint_t Count>
    struct use_simple_repeat_<Expr, Char, generic_quant_tag<Count, Count> >
      : use_simple_repeat_<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char>
    {};

    template<typename Expr, typename Char>
    struct use_simple_repeat_<Expr, Char, proto::tag::negate>
      : use_simple_repeat_<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char>
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // use_simple_repeat
    //
    template<typename Expr, typename Char>
    struct use_simple_repeat
      : use_simple_repeat_<Expr, Char>
    {
        // should never try to repeat something of 0-width
        BOOST_MPL_ASSERT_RELATION(0, !=, (width_of<Expr, Char>::value));
    };

    template<typename Expr, typename Char>
    struct use_simple_repeat<Expr &, Char>
      : use_simple_repeat_<Expr, Char>
    {
        // should never try to repeat something of 0-width
        BOOST_MPL_ASSERT_RELATION(0, !=, (width_of<Expr, Char>::value));
    };

}}} // namespace boost::xpressive::detail

#endif
