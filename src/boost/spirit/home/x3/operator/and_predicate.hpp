/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_AND_PREDICATE_MARCH_23_2007_0617PM)
#define BOOST_SPIRIT_X3_AND_PREDICATE_MARCH_23_2007_0617PM

#include <boost/spirit/home/x3/core/parser.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename Subject>
    struct and_predicate : unary_parser<Subject, and_predicate<Subject>>
    {
        typedef unary_parser<Subject, and_predicate<Subject>> base_type;

        typedef unused_type attribute_type;
        static bool const has_attribute = false;

        constexpr and_predicate(Subject const& subject)
          : base_type(subject) {}

        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& /*attr*/) const
        {
            Iterator i = first;
            return this->subject.parse(i, last, context, rcontext, unused);
        }
    };

    template <typename Subject>
    constexpr and_predicate<typename extension::as_parser<Subject>::value_type>
    operator&(Subject const& subject)
    {
        return { as_parser(subject) };
    }
}}}

#endif
