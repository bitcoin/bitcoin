/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_PARSER_BINDER_DECEMBER_05_2008_0516_PM)
#define BOOST_SPIRIT_PARSER_BINDER_DECEMBER_05_2008_0516_PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/fusion/include/at.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>

namespace boost { namespace spirit { namespace qi { namespace detail
{
    // parser_binder for plain rules
    template <typename Parser, typename Auto>
    struct parser_binder
    {
        parser_binder(Parser const& p_)
          : p(p_) {}

        template <typename Iterator, typename Skipper, typename Context>
        bool call(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper, mpl::true_) const
        {
            // If DeducedAuto is false (semantic actions is present), the 
            // component's attribute is unused.
            return p.parse(first, last, context, skipper, unused);
        }

        template <typename Iterator, typename Skipper, typename Context>
        bool call(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper, mpl::false_) const
        {
            // If DeducedAuto is true (no semantic action), we pass the rule's 
            // attribute on to the component.
            return p.parse(first, last, context, skipper
                , fusion::at_c<0>(context.attributes));
        }

        template <typename Iterator, typename Skipper, typename Context>
        bool operator()(
            Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper) const
        {
            // If Auto is false, we need to deduce whether to apply auto rule
            typedef typename traits::has_semantic_action<Parser>::type auto_rule;
            return call(first, last, context, skipper, auto_rule());
        }

        Parser p;
    };

    // parser_binder for auto rules
    template <typename Parser>
    struct parser_binder<Parser, mpl::true_>
    {
        parser_binder(Parser const& p_)
          : p(p_) {}

        template <typename Iterator, typename Skipper, typename Context>
        bool operator()(
            Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper) const
        {
            // If Auto is true, we pass the rule's attribute on to the component.
            return p.parse(first, last, context, skipper
                , fusion::at_c<0>(context.attributes));
        }

        Parser p;
    };

    template <typename Auto, typename Parser>
    inline parser_binder<Parser, Auto>
    bind_parser(Parser const& p)
    {
        return parser_binder<Parser, Auto>(p);
    }
}}}}

#endif
