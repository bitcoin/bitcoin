/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_EPS_MARCH_23_2007_0454PM)
#define BOOST_SPIRIT_X3_EPS_MARCH_23_2007_0454PM

#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>

namespace boost { namespace spirit { namespace x3
{
    struct rule_context_tag;

    struct semantic_predicate : parser<semantic_predicate>
    {
        typedef unused_type attribute_type;
        static bool const has_attribute = false;

        constexpr semantic_predicate(bool predicate)
          : predicate(predicate) {}

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute&) const
        {
            x3::skip_over(first, last, context);
            return predicate;
        }

        bool predicate;
    };

    template <typename F>
    struct lazy_semantic_predicate : parser<lazy_semantic_predicate<F>>
    {
        typedef unused_type attribute_type;
        static bool const has_attribute = false;

        constexpr lazy_semantic_predicate(F f)
          : f(f) {}

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute& /* attr */) const
        {
            x3::skip_over(first, last, context);
            return f(x3::get<rule_context_tag>(context));
        }

        F f;
    };

    struct eps_parser : parser<eps_parser>
    {
        typedef unused_type attribute_type;
        static bool const has_attribute = false;

        template <typename Iterator, typename Context
          , typename RuleContext, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RuleContext&, Attribute&) const
        {
            x3::skip_over(first, last, context);
            return true;
        }

        constexpr semantic_predicate operator()(bool predicate) const
        {
            return { predicate };
        }

        template <typename F>
        constexpr lazy_semantic_predicate<F> operator()(F f) const
        {
            return { f };
        }
    };

    constexpr auto eps = eps_parser{};
}}}

#endif
