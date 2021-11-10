/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_OPERATOR_LIST_HPP
#define BOOST_SPIRIT_QI_OPERATOR_LIST_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/qi/detail/fail_function.hpp>
#include <boost/spirit/home/qi/detail/pass_container.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/proto/operators.hpp>
#include <boost/proto/tags.hpp>
#include <vector>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<qi::domain, proto::tag::modulus> // enables p % d
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
    template <typename Left, typename Right>
    struct list : binary_parser<list<Left, Right> >
    {
        typedef Left left_type;
        typedef Right right_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            // Build a std::vector from the LHS's attribute. Note
            // that build_std_vector may return unused_type if the
            // subject's attribute is an unused_type.
            typedef typename
                traits::build_std_vector<
                    typename traits::
                        attribute_of<Left, Context, Iterator>::type
                >::type
            type;
        };

        list(Left const& left_, Right const& right_)
          : left(left_), right(right_) {}

        template <typename F>
        bool parse_container(F f) const
        {
            // in order to succeed we need to match at least one element 
            if (f (left))
                return false;

            typename F::iterator_type save = f.f.first;
            while (right.parse(f.f.first, f.f.last, f.f.context, f.f.skipper, unused)
              && !f (left))
            {
                save = f.f.first;
            }

            f.f.first = save;
            return true;
        }

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            typedef detail::fail_function<Iterator, Context, Skipper>
                fail_function;

            // ensure the attribute is actually a container type
            traits::make_container(attr_);

            Iterator iter = first;
            fail_function f(iter, last, context, skipper);
            if (!parse_container(detail::make_pass_container(f, attr_)))
                return false;

            first = f.first;
            return true;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("list",
                std::make_pair(left.what(context), right.what(context)));
        }

        Left left;
        Right right;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::modulus, Elements, Modifiers>
      : make_binary_composite<Elements, list>
    {};
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Left, typename Right>
    struct has_semantic_action<qi::list<Left, Right> >
      : binary_has_semantic_action<Left, Right> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Left, typename Right, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<qi::list<Left, Right>, Attribute, Context
          , Iterator> 
      : mpl::true_ {};
}}}

#endif
