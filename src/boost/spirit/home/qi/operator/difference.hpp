/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_OPERATOR_DIFFERENCE_HPP
#define BOOST_SPIRIT_QI_OPERATOR_DIFFERENCE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/proto/operators.hpp>
#include <boost/proto/tags.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<qi::domain, proto::tag::minus> // enables -
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
    template <typename Left, typename Right>
    struct difference : binary_parser<difference<Left, Right> >
    {
        typedef Left left_type;
        typedef Right right_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename
                traits::attribute_of<left_type, Context, Iterator>::type
            type;
        };

        difference(Left const& left_, Right const& right_)
          : left(left_), right(right_) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            // Unlike classic Spirit, with this version of difference, the rule
            // lit("policeman") - "police" will always fail to match.

            // Spirit2 does not count the matching chars while parsing and
            // there is no reliable and fast way to check if the LHS matches
            // more than the RHS.

            // Try RHS first
            Iterator start = first;
            if (right.parse(first, last, context, skipper, unused))
            {
                // RHS succeeds, we fail.
                first = start;
                return false;
            }
            // RHS fails, now try LHS
            return left.parse(first, last, context, skipper, attr_);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("difference",
                std::make_pair(left.what(context), right.what(context)));
        }

        Left left;
        Right right;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::minus, Elements, Modifiers>
      : make_binary_composite<Elements, difference>
    {};
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Left, typename Right>
    struct has_semantic_action<qi::difference<Left, Right> >
      : binary_has_semantic_action<Left, Right> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Left, typename Right, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<qi::difference<Left, Right>, Attribute, Context
      , Iterator>
      : binary_handles_container<Left, Right, Attribute, Context, Iterator> {};
}}}

#endif
