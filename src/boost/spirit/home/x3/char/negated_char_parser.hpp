/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_NEGATED_CHAR_PARSER_APR_16_2006_0906AM)
#define BOOST_SPIRIT_X3_NEGATED_CHAR_PARSER_APR_16_2006_0906AM

#include <boost/spirit/home/x3/support/traits/attribute_of.hpp>
#include <boost/spirit/home/x3/support/traits/has_attribute.hpp>
#include <boost/spirit/home/x3/char/char_parser.hpp>

namespace boost { namespace spirit { namespace x3
{
    ///////////////////////////////////////////////////////////////////////////
    // negated_char_parser handles ~cp expressions (cp is a char_parser)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Positive>
    struct negated_char_parser :
        char_parser<negated_char_parser<Positive>>
    {
        constexpr negated_char_parser(Positive const& positive)
          : positive(positive) {}

        template <typename CharParam, typename Context>
        bool test(CharParam ch, Context const& context) const
        {
            return !positive.test(ch, context);
        }

        Positive positive;
    };

    template <typename Positive>
    constexpr negated_char_parser<Positive>
    operator~(char_parser<Positive> const& cp)
    {
        return { cp.derived() };
    }

    template <typename Positive>
    constexpr Positive const&
    operator~(negated_char_parser<Positive> const& cp)
    {
        return cp.positive;
    }
}}}

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Positive, typename Context>
    struct attribute_of<x3::negated_char_parser<Positive>, Context>
        : attribute_of<Positive, Context> {};
    
    template <typename Positive, typename Context>
    struct has_attribute<x3::negated_char_parser<Positive>, Context>
        : has_attribute<Positive, Context> {};
}}}}

#endif
