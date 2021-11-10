///////////////////////////////////////////////////////////////////////////////
// width_of.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_WIDTH_OF_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_STATIC_WIDTH_OF_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/ref.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/times.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/static/type_traits.hpp>
#include <boost/proto/traits.hpp>

namespace boost { namespace xpressive { namespace detail
{
    template<typename Expr, typename Char, typename Tag = typename Expr::proto_tag>
    struct width_of;

    ///////////////////////////////////////////////////////////////////////////////
    // add_widths
    //
    template<std::size_t N, std::size_t M>
    struct add_widths
      : mpl::size_t<N + M>
    {};

    template<std::size_t M>
    struct add_widths<unknown_width::value, M>
      : unknown_width
    {};

    template<std::size_t N>
    struct add_widths<N, unknown_width::value>
      : unknown_width
    {};

    template<>
    struct add_widths<unknown_width::value, unknown_width::value>
      : unknown_width
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // or_widths
    //
    template<std::size_t N, std::size_t M>
    struct or_widths
      : unknown_width
    {};

    template<std::size_t N>
    struct or_widths<N, N>
      : mpl::size_t<N>
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // width_of_terminal
    //
    template<typename Expr, typename Char, bool IsXpr = is_xpr<Expr>::value>
    struct width_of_terminal
      : mpl::size_t<Expr::width>    // xpressive literals
    {};

    template<typename Expr, typename Char>
    struct width_of_terminal<Expr, Char, false>
      : unknown_width       // unknown literals (eg, basic_string, basic_regex, etc.)
    {};

    template<typename Char>
    struct width_of_terminal<Char, Char, false>
      : mpl::size_t<1>      // char literals
    {};

    template<typename Char>
    struct width_of_terminal<char, Char, false>
      : mpl::size_t<1>      // char literals
    {};

    template<>
    struct width_of_terminal<char, char, false>
      : mpl::size_t<1>      // char literals
    {};

    template<typename Elem, std::size_t N, typename Char>
    struct width_of_terminal<Elem (&) [N], Char, false>
      : mpl::size_t<N-is_char<Elem>::value>    // string literals
    {};

    template<typename Elem, std::size_t N, typename Char>
    struct width_of_terminal<Elem const (&) [N], Char, false>
      : mpl::size_t<N-is_char<Elem>::value>    // string literals
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // width_of
    //
    template<typename Expr, typename Char, typename Tag>
    struct width_of
    {};

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::terminal>
      : width_of_terminal<typename proto::result_of::value<Expr>::type, Char>
    {};

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::shift_right>
      : add_widths<
            width_of<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char>::value
          , width_of<typename remove_reference<typename Expr::proto_child1>::type::proto_base_expr, Char>::value
        >
    {};

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::bitwise_or>
      : or_widths<
            width_of<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char>::value
          , width_of<typename remove_reference<typename Expr::proto_child1>::type::proto_base_expr, Char>::value
        >
    {};

    template<typename Expr, typename Char, typename Left>
    struct width_of_assign
    {};

    template<typename Expr, typename Char>
    struct width_of_assign<Expr, Char, mark_placeholder>
      : width_of<typename remove_reference<typename Expr::proto_child1>::type::proto_base_expr, Char>
    {};

    template<typename Expr, typename Char>
    struct width_of_assign<Expr, Char, set_initializer>
      : mpl::size_t<1>
    {};

    template<typename Expr, typename Char, typename Nbr>
    struct width_of_assign<Expr, Char, attribute_placeholder<Nbr> >
      : unknown_width
    {};

    // either (s1 = ...) or (a1 = ...) or (set = ...)
    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::assign>
      : width_of_assign<
            Expr
          , Char
          , typename proto::result_of::value<
                typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr
            >::type
        >
    {};

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, modifier_tag>
      : width_of<typename remove_reference<typename Expr::proto_child1>::type::proto_base_expr, Char>
    {};

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, lookahead_tag>
      : mpl::size_t<0>
    {};

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, lookbehind_tag>
      : mpl::size_t<0>
    {};

    // keep() is used to turn off backtracking, so they should only be used
    // for things that are variable-width (eg. quantified)
    template<typename Expr, typename Char>
    struct width_of<Expr, Char, keeper_tag>
      : unknown_width
    {
        // TODO: keep() now has a second meaning: execute actions immediately.
        // In that sense, it is perfectly reasonable to put a fixed-width
        // sub-expression in a keep. Can fixed-width keep() sub-expressions
        // use the simple_repeat_matcher?
    };

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::unary_plus>
      : unknown_width
    {};

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::dereference>
      : unknown_width
    {};

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::logical_not>
      : unknown_width
    {};

    template<typename Expr, typename Char, uint_t Min, uint_t Max>
    struct width_of<Expr, Char, generic_quant_tag<Min, Max> >
      : unknown_width
    {};

    template<typename Expr, typename Char, uint_t Count>
    struct width_of<Expr, Char, generic_quant_tag<Count, Count> >
      : mpl::if_c<
            mpl::equal_to<unknown_width, width_of<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char> >::value
          , unknown_width
          , mpl::times<
                width_of<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char>
              , mpl::size_t<Count>
            >
        >::type
    {};

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::negate>
      : width_of<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char>
    {};

    // when complementing a set or an assertion, the width is that of the set (1) or the assertion (0)
    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::complement>
      : width_of<typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr, Char>
    {};

    // The comma is used in list-initialized sets, and the width of sets are 1
    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::comma>
      : mpl::size_t<1>
    {};

    // The subscript operator[] is used for sets, as in set['a' | range('b','h')],
    // or for actions as in (any >> expr)[ action ]
    template<typename Expr, typename Char, typename Left>
    struct width_of_subscript
      : width_of<Left, Char>
    {};

    template<typename Expr, typename Char>
    struct width_of_subscript<Expr, Char, set_initializer_type>
      : mpl::size_t<1>
    {
        // If Left is "set" then make sure that Right has a width_of 1
        BOOST_MPL_ASSERT_RELATION(
            1
          , ==
          , (width_of<typename remove_reference<typename Expr::proto_child1>::type::proto_base_expr, Char>::value));
    };

    template<typename Expr, typename Char>
    struct width_of<Expr, Char, proto::tag::subscript>
      : width_of_subscript<Expr, Char, typename remove_reference<typename Expr::proto_child0>::type::proto_base_expr>
    {};

}}} // namespace boost::xpressive::detail

#undef UNREF

#endif
