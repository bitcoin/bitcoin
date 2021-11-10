/*=============================================================================
    Copyright (c) 2015 Mario Lang
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_HOME_X3_EXTENSIONS_MATCHES_HPP)
#define BOOST_SPIRIT_HOME_X3_EXTENSIONS_MATCHES_HPP

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename Subject>
    struct matches_directive : unary_parser<Subject, matches_directive<Subject>>
    {
        using base_type = unary_parser<Subject, matches_directive<Subject>>;
        static bool const has_attribute = true;
        using attribute_type = bool;

        constexpr matches_directive(Subject const& subject) : base_type(subject) {}

        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            bool const result = this->subject.parse(
                    first, last, context, rcontext, unused);
            traits::move_to(result, attr);
            return true;
        }
    };

    struct matches_gen
    {
        template <typename Subject>
        constexpr matches_directive<typename extension::as_parser<Subject>::value_type>
        operator[](Subject const& subject) const
        {
            return { as_parser(subject) };
        }
    };

    constexpr auto matches = matches_gen{};
}}}

#endif
