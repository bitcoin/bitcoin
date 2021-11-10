//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_CHAR_GENERATOR_SEP_07_2009_0417PM)
#define BOOST_SPIRIT_CHAR_GENERATOR_SEP_07_2009_0417PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/detail/generate_to.hpp>
#include <boost/spirit/home/karma/detail/extract_from.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/proto/operators.hpp>
#include <boost/proto/tags.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<karma::domain, proto::tag::complement> // enables ~
      : mpl::true_ {};

}}

namespace boost { namespace spirit { namespace traits // classification
{
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(char_generator_id)
    }

    template <typename T>
    struct is_char_generator : detail::has_char_generator_id<T> {};
}}}

namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    // The base char_parser
    ///////////////////////////////////////////////////////////////////////////
    template <typename Derived, typename CharEncoding, typename Tag
      , typename Char = typename CharEncoding::char_type, typename Attr = Char>
    struct char_generator : primitive_generator<Derived>
    {
        typedef CharEncoding char_encoding;
        typedef Tag tag;
        typedef Char char_type;
        struct char_generator_id;

        // if Attr is unused_type, Derived must supply its own attribute
        // metafunction
        template <typename Context, typename Unused>
        struct attribute
        {
            typedef Attr type;
        };

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& context, Delimiter const& d
          , Attribute const& attr) const
        {
            if (!traits::has_optional_value(attr))
                return false;

            Attr ch = Attr();
            if (!this->derived().test(traits::extract_from<Attr>(attr, context), ch, context))
                return false;

            return karma::detail::generate_to(sink, ch, char_encoding(), tag()) &&
                   karma::delimit_out(sink, d);       // always do post-delimiting
        }

        // Requirement: g.test(attr, ch, context) -> bool
        //
        //  attr:       associated attribute
        //  ch:         character to be generated (set by test())
        //  context:    enclosing rule context
    };

    ///////////////////////////////////////////////////////////////////////////
    // negated_char_generator handles ~cg expressions (cg is a char_generator)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Positive>
    struct negated_char_generator
      : char_generator<negated_char_generator<Positive>
          , typename Positive::char_encoding, typename Positive::tag>
    {
        negated_char_generator(Positive const& positive)
          : positive(positive) {}

        template <typename Attribute, typename CharParam, typename Context>
        bool test(Attribute const& attr, CharParam& ch, Context& context) const
        {
            return !positive.test(attr, ch, context);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("not", positive.what(context));
        }

        Positive positive;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Positive>
        struct make_negated_char_generator
        {
            typedef negated_char_generator<Positive> result_type;
            result_type operator()(Positive const& positive) const
            {
                return result_type(positive);
            }
        };

        template <typename Positive>
        struct make_negated_char_generator<negated_char_generator<Positive> >
        {
            typedef Positive result_type;
            result_type operator()(negated_char_generator<Positive> const& ncg) const
            {
                return ncg.positive;
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
            traits::is_char_generator<subject>::value
        ), subject_is_not_negatable, (subject));

        typedef typename
            detail::make_negated_char_generator<subject>::result_type
        result_type;

        result_type operator()(Elements const& elements, unused_type) const
        {
            return detail::make_negated_char_generator<subject>()(
                fusion::at_c<0>(elements));
        }
    };

}}}

#endif
