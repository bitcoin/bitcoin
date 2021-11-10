/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_OPERATOR_EXPECT_HPP
#define BOOST_SPIRIT_QI_OPERATOR_EXPECT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/operator/sequence_base.hpp>
#include <boost/spirit/home/qi/detail/expect_function.hpp>
#include <boost/spirit/home/qi/detail/expectation_failure.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/proto/operators.hpp>
#include <boost/proto/tags.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<qi::domain, proto::tag::greater> // enables >
      : mpl::true_ {};

    template <>
    struct flatten_tree<qi::domain, proto::tag::greater> // flattens >
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
    template <typename Elements>
    struct expect_operator : sequence_base<expect_operator<Elements>, Elements>
    {
        friend struct sequence_base<expect_operator<Elements>, Elements>;

        expect_operator(Elements const& elements)
          : sequence_base<expect_operator<Elements>, Elements>(elements) {}

    private:

        template <typename Iterator, typename Context, typename Skipper>
        static detail::expect_function<
            Iterator, Context, Skipper
          , expectation_failure<Iterator> >
        fail_function(
            Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper)
        {
            return detail::expect_function<
                Iterator, Context, Skipper, expectation_failure<Iterator> >
                (first, last, context, skipper);
        }

        std::string id() const { return "expect_operator"; }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::greater, Elements, Modifiers>
      : make_nary_composite<Elements, expect_operator>
    {};
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements>
    struct has_semantic_action<qi::expect_operator<Elements> >
      : nary_has_semantic_action<Elements> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<qi::expect_operator<Elements>, Attribute, Context
          , Iterator>
      : mpl::true_ {};
}}}

#endif
