/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_PLUS_MARCH_13_2007_0127PM)
#define BOOST_SPIRIT_X3_PLUS_MARCH_13_2007_0127PM

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/support/traits/container_traits.hpp>
#include <boost/spirit/home/x3/support/traits/attribute_of.hpp>
#include <boost/spirit/home/x3/core/detail/parse_into_container.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename Subject>
    struct plus : unary_parser<Subject, plus<Subject>>
    {
        typedef unary_parser<Subject, plus<Subject>> base_type;
        static bool const handles_container = true;

        constexpr plus(Subject const& subject)
          : base_type(subject) {}

        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            if (!detail::parse_into_container(
                this->subject, first, last, context, rcontext, attr))
                return false;

            while (detail::parse_into_container(
                this->subject, first, last, context, rcontext, attr))
                ;
            return true;
        }
    };

    template <typename Subject>
    constexpr plus<typename extension::as_parser<Subject>::value_type>
    operator+(Subject const& subject)
    {
        return { as_parser(subject) };
    }
}}}

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Subject, typename Context>
    struct attribute_of<x3::plus<Subject>, Context>
        : build_container<
            typename attribute_of<Subject, Context>::type> {};
}}}}

#endif
