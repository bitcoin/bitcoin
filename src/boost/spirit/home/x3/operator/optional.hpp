/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_OPTIONAL_MARCH_23_2007_1117PM)
#define BOOST_SPIRIT_X3_OPTIONAL_MARCH_23_2007_1117PM

#include <boost/spirit/home/x3/core/proxy.hpp>
#include <boost/spirit/home/x3/core/detail/parse_into_container.hpp>
#include <boost/spirit/home/x3/support/traits/attribute_of.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <boost/spirit/home/x3/support/traits/optional_traits.hpp>
#include <boost/spirit/home/x3/support/traits/attribute_category.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename Subject>
    struct optional : proxy<Subject, optional<Subject>>
    {
        typedef proxy<Subject, optional<Subject>> base_type;
        static bool const is_pass_through_unary = false;
        static bool const handles_container = true;

        constexpr optional(Subject const& subject)
          : base_type(subject) {}

        using base_type::parse_subject;

        // Attribute is a container
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse_subject(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr
          , traits::container_attribute) const
        {
            detail::parse_into_container(
                this->subject, first, last, context, rcontext, attr);
            return true;
        }

        // Attribute is an optional
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse_subject(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr
          , traits::optional_attribute) const
        {
            typedef typename
                x3::traits::optional_value<Attribute>::type
            value_type;

            // create a local value
            value_type val{};

            if (this->subject.parse(first, last, context, rcontext, val))
            {
                // assign the parsed value into our attribute
                x3::traits::move_to(val, attr);
            }
            return true;
        }
    };

    template <typename Subject>
    constexpr optional<typename extension::as_parser<Subject>::value_type>
    operator-(Subject const& subject)
    {
        return { as_parser(subject) };
    }
}}}

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Subject, typename Context>
    struct attribute_of<x3::optional<Subject>, Context>
        : build_optional<
            typename attribute_of<Subject, Context>::type> {};
}}}}

#endif
