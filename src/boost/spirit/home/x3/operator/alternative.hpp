/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_ALTERNATIVE_JAN_07_2013_1131AM)
#define BOOST_SPIRIT_X3_ALTERNATIVE_JAN_07_2013_1131AM

#include <boost/spirit/home/x3/support/traits/attribute_of_binary.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/operator/detail/alternative.hpp>

#include <boost/variant/variant_fwd.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename Left, typename Right>
    struct alternative : binary_parser<Left, Right, alternative<Left, Right>>
    {
        typedef binary_parser<Left, Right, alternative<Left, Right>> base_type;

        constexpr alternative(Left const& left, Right const& right)
            : base_type(left, right) {}

        template <typename Iterator, typename Context, typename RContext>
        bool parse(
            Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, unused_type) const
        {
            return this->left.parse(first, last, context, rcontext, unused)
               || this->right.parse(first, last, context, rcontext, unused);
        }

        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse(
            Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            return detail::parse_alternative(this->left, first, last, context, rcontext, attr)
               || detail::parse_alternative(this->right, first, last, context, rcontext, attr);
        }
    };

    template <typename Left, typename Right>
    constexpr alternative<
        typename extension::as_parser<Left>::value_type
      , typename extension::as_parser<Right>::value_type>
    operator|(Left const& left, Right const& right)
    {
        return { as_parser(left), as_parser(right) };
    }
}}}

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Left, typename Right, typename Context>
    struct attribute_of<x3::alternative<Left, Right>, Context>
        : x3::detail::attribute_of_binary<boost::variant, x3::alternative, Left, Right, Context> {};
}}}}

#endif
