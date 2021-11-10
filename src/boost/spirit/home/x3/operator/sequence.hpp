/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_SEQUENCE_JAN_06_2013_1015AM)
#define BOOST_SPIRIT_X3_SEQUENCE_JAN_06_2013_1015AM

#include <boost/spirit/home/x3/support/traits/attribute_of_binary.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/operator/detail/sequence.hpp>
#include <boost/spirit/home/x3/directive/expect.hpp>

#include <boost/fusion/include/deque_fwd.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename Left, typename Right>
    struct sequence : binary_parser<Left, Right, sequence<Left, Right>>
    {
        typedef binary_parser<Left, Right, sequence<Left, Right>> base_type;

        constexpr sequence(Left const& left, Right const& right)
            : base_type(left, right) {}

        template <typename Iterator, typename Context, typename RContext>
        bool parse(
            Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, unused_type) const
        {
            Iterator save = first;
            if (this->left.parse(first, last, context, rcontext, unused)
                && this->right.parse(first, last, context, rcontext, unused))
                return true;
            first = save;
            return false;
        }

        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse(
            Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            return detail::parse_sequence(*this, first, last, context, rcontext, attr
              , typename traits::attribute_category<Attribute>::type());
        }
    };

    template <typename Left, typename Right>
    constexpr sequence<
        typename extension::as_parser<Left>::value_type
      , typename extension::as_parser<Right>::value_type>
    operator>>(Left const& left, Right const& right)
    {
        return { as_parser(left), as_parser(right) };
    }

    template <typename Left, typename Right>
    constexpr auto operator>(Left const& left, Right const& right)
    {
        return left >> expect[right];
    }
}}}

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Left, typename Right, typename Context>
    struct attribute_of<x3::sequence<Left, Right>, Context>
        : x3::detail::attribute_of_binary<fusion::deque, x3::sequence, Left, Right, Context> {};
}}}}

#endif
