/*=============================================================================
    Copyright (c) 2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_X3_DIRECTIVE_RAW_HPP
#define BOOST_SPIRIT_X3_DIRECTIVE_RAW_HPP

#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <boost/spirit/home/x3/support/traits/pseudo_attribute.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace boost { namespace spirit { namespace x3
{
    // this is a pseudo attribute type indicating that the parser wants the
    // iterator range pointing to the [first, last) matching characters from
    // the input iterators.
    struct raw_attribute_type {};

    template <typename Subject>
    struct raw_directive : unary_parser<Subject, raw_directive<Subject>>
    {
        typedef unary_parser<Subject, raw_directive<Subject> > base_type;
        typedef raw_attribute_type attribute_type;
        static bool const handles_container = true;
        typedef Subject subject_type;

        constexpr raw_directive(Subject const& subject)
          : base_type(subject) {}

        template <typename Iterator, typename Context
            , typename RContext, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            x3::skip_over(first, last, context);
            Iterator i = first;
            if (this->subject.parse(i, last, context, rcontext, unused))
            {
                traits::move_to(first, i, attr);
                first = i;
                return true;
            }
            return false;
        }

        template <typename Iterator, typename Context, typename RContext>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, unused_type) const
        {
            return this->subject.parse(first, last, context, rcontext, unused);
        }
    };

    struct raw_gen
    {
        template <typename Subject>
        constexpr raw_directive<typename extension::as_parser<Subject>::value_type>
        operator[](Subject const& subject) const
        {
            return { as_parser(subject) };
        }
    };

    constexpr auto raw = raw_gen{};

    namespace traits
    {
        template <typename Context, typename Iterator>
        struct pseudo_attribute<Context, raw_attribute_type, Iterator>
        {
            using attribute_type = raw_attribute_type;
            using type = boost::iterator_range<Iterator>;

            static type call(Iterator& first, Iterator const& last, attribute_type)
            {
                return { first, last };
            }
        };
    }
}}}

#endif
