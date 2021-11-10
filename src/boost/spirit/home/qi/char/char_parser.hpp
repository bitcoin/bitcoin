/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_CHAR_PARSER_APR_16_2006_0906AM)
#define BOOST_SPIRIT_CHAR_PARSER_APR_16_2006_0906AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/proto/operators.hpp>
#include <boost/proto/tags.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<qi::domain, proto::tag::complement> // enables ~
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace traits // classification
{
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(char_parser_id)
    }

    template <typename T>
    struct is_char_parser : detail::has_char_parser_id<T> {};
}}}

namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    // The base char_parser
    ///////////////////////////////////////////////////////////////////////////
    template <typename Derived, typename Char, typename Attr = Char>
    struct char_parser : primitive_parser<Derived>
    {
        typedef Char char_type;
        struct char_parser_id;

        // if Attr is unused_type, Derived must supply its own attribute
        // metafunction
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef Attr type;
        };

        template <typename Iterator, typename Context, typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper, Attribute& attr_) const
        {
            qi::skip_over(first, last, skipper);

            if (first != last && this->derived().test(*first, context))
            {
                spirit::traits::assign_to(*first, attr_);
                ++first;
                return true;
            }
            return false;
        }

        // Requirement: p.test(ch, context) -> bool
        //
        //  ch:         character being parsed
        //  context:    enclosing rule context
    };

    ///////////////////////////////////////////////////////////////////////////
    // negated_char_parser handles ~cp expressions (cp is a char_parser)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Positive>
    struct negated_char_parser :
        char_parser<negated_char_parser<Positive>, typename Positive::char_type>
    {
        negated_char_parser(Positive const& positive_)
          : positive(positive_) {}

        template <typename CharParam, typename Context>
        bool test(CharParam ch, Context& context) const
        {
            return !positive.test(ch, context);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("not", positive.what(context));
        }

        Positive positive;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Positive>
        struct make_negated_char_parser
        {
            typedef negated_char_parser<Positive> result_type;
            result_type operator()(Positive const& positive) const
            {
                return result_type(positive);
            }
        };

        template <typename Positive>
        struct make_negated_char_parser<negated_char_parser<Positive> >
        {
            typedef Positive result_type;
            result_type operator()(negated_char_parser<Positive> const& ncp) const
            {
                return ncp.positive;
            }
        };
    }

    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::complement, Elements, Modifiers>
    {
        typedef typename
            fusion::result_of::value_at_c<Elements, 0>::type
        subject;

        BOOST_SPIRIT_ASSERT_MSG((
            traits::is_char_parser<subject>::value
        ), subject_is_not_negatable, (subject));

        typedef typename
            detail::make_negated_char_parser<subject>::result_type
        result_type;

        result_type operator()(Elements const& elements, unused_type) const
        {
            return detail::make_negated_char_parser<subject>()(
                fusion::at_c<0>(elements));
        }
    };
}}}

#endif
