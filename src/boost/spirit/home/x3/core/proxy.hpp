/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_PROXY_FEBRUARY_1_2013_0211PM)
#define BOOST_SPIRIT_X3_PROXY_FEBRUARY_1_2013_0211PM

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/detail/parse_into_container.hpp>
#include <boost/spirit/home/x3/support/traits/attribute_category.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename Subject, typename Derived>
    struct proxy : unary_parser<Subject, Derived>
    {
        static bool const is_pass_through_unary = true;

        constexpr proxy(Subject const& subject)
          : unary_parser<Subject, Derived>(subject) {}

        // Overload this when appropriate. The proxy parser will pick up
        // the most derived overload.
        template <typename Iterator, typename Context
          , typename RuleContext, typename Attribute, typename Category>
        bool parse_subject(Iterator& first, Iterator const& last
          , Context const& context, RuleContext& rcontext, Attribute& attr, Category) const
        {
            this->subject.parse(first, last, context, rcontext, attr);
            return true;
        }

        // Main entry point.
        template <typename Iterator, typename Context
          , typename RuleContext, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RuleContext& rcontext, Attribute& attr) const
        {
            return this->derived().parse_subject(first, last, context, rcontext, attr
                , typename traits::attribute_category<Attribute>::type());
        }
    };

}}}

#endif
