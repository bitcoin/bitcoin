//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_OPERATOR_ALTERNATIVE_HPP
#define BOOST_SPIRIT_KARMA_OPERATOR_ALTERNATIVE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/detail/alternative_function.hpp>
#include <boost/spirit/home/karma/detail/get_stricttag.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/support/detail/what_function.hpp>
#include <boost/fusion/include/any.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/mpl/accumulate.hpp>
#include <boost/mpl/bitor.hpp>
#include <boost/proto/tags.hpp>
#include <boost/config.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<karma::domain, proto::tag::bitwise_or>  // enables |
      : mpl::true_ {};

    template <>
    struct flatten_tree<karma::domain, proto::tag::bitwise_or>  // flattens |
      : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    // specialization for sequences
    template <typename Elements>
    struct alternative_properties
    {
        struct element_properties
        {
            template <typename T>
            struct result;

            template <typename F, typename Element>
            struct result<F(Element)>
            {
                typedef properties_of<Element> type;
            };

            // never called, but needed for decltype-based result_of (C++0x)
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
            template <typename Element>
            typename result<element_properties(Element)>::type
            operator()(Element&&) const;
#endif
        };

        typedef typename mpl::accumulate<
            typename fusion::result_of::transform<
                Elements, element_properties>::type
          , mpl::int_<karma::generator_properties::countingbuffer>
          , mpl::bitor_<mpl::_2, mpl::_1>
        >::type type;
    };

}}}

namespace boost { namespace spirit { namespace karma
{
    template <typename Elements, typename Strict, typename Derived>
    struct base_alternative : nary_generator<Derived>
    {
        typedef typename traits::alternative_properties<Elements>::type 
            properties;

        template <typename Context, typename Iterator = unused_type>
        struct attribute
        {
            // Put all the element attributes in a tuple
            typedef typename traits::build_attribute_sequence<
                Elements, Context, traits::alternative_attribute_transform
              , Iterator, karma::domain
            >::type all_attributes;

            // Ok, now make a variant over the attribute sequence. Note that
            // build_variant makes sure that 1) all attributes in the variant
            // are unique 2) puts the unused attribute, if there is any, to
            // the front and 3) collapses single element variants, variant<T>
            // to T.
            typedef typename traits::build_variant<all_attributes>::type type;
        };

        base_alternative(Elements const& elements)
          : elements(elements) {}

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx
          , Delimiter const& d, Attribute const& attr) const
        {
            typedef detail::alternative_generate_function<
                OutputIterator, Context, Delimiter, Attribute, Strict
            > functor;

            // f return true if *any* of the parser succeeds
            functor f (sink, ctx, d, attr);
            return fusion::any(elements, f);
        }

        template <typename Context>
        info what(Context& context) const
        {
            info result("alternative");
            fusion::for_each(elements,
                spirit::detail::what_function<Context>(result, context));
            return result;
        }

        Elements elements;
    };

    template <typename Elements>
    struct alternative 
      : base_alternative<Elements, mpl::false_, alternative<Elements> >
    {
        typedef base_alternative<Elements, mpl::false_, alternative> 
            base_alternative_;

        alternative(Elements const& elements)
          : base_alternative_(elements) {}
    };

    template <typename Elements>
    struct strict_alternative 
      : base_alternative<Elements, mpl::true_, strict_alternative<Elements> >
    {
        typedef base_alternative<Elements, mpl::true_, strict_alternative> 
            base_alternative_;

        strict_alternative(Elements const& elements)
          : base_alternative_(elements) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Elements, bool strict_mode = false>
        struct make_alternative 
          : make_nary_composite<Elements, alternative>
        {};

        template <typename Elements>
        struct make_alternative<Elements, true> 
          : make_nary_composite<Elements, strict_alternative>
        {};
    }

    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::bitwise_or, Elements, Modifiers>
      : detail::make_alternative<Elements
          , detail::get_stricttag<Modifiers>::value>
    {};

}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements>
    struct has_semantic_action<karma::alternative<Elements> >
      : nary_has_semantic_action<Elements> {};

    template <typename Elements>
    struct has_semantic_action<karma::strict_alternative<Elements> >
      : nary_has_semantic_action<Elements> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<karma::alternative<Elements>
          , Attribute, Context, Iterator>
      : nary_handles_container<Elements, Attribute, Context, Iterator> {};

    template <typename Elements, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<karma::strict_alternative<Elements>
          , Attribute, Context, Iterator>
      : nary_handles_container<Elements, Attribute, Context, Iterator> {};
}}}

#endif
