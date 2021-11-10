//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_OPERATOR_SEQUENCE_HPP
#define BOOST_SPIRIT_KARMA_OPERATOR_SEQUENCE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/detail/fail_function.hpp>
#include <boost/spirit/home/karma/detail/pass_container.hpp>
#include <boost/spirit/home/karma/detail/get_stricttag.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/detail/what_function.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/karma/detail/indirect_iterator.hpp>
#include <boost/spirit/home/support/algorithm/any_if.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/sequence_base_id.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/support/attributes.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/bitor.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/fusion/include/transform.hpp>
#include <boost/mpl/accumulate.hpp>
#include <boost/proto/operators.hpp>
#include <boost/proto/tags.hpp>
#include <boost/config.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<karma::domain, proto::tag::shift_left> // enables <<
      : mpl::true_ {};

    template <>
    struct flatten_tree<karma::domain, proto::tag::shift_left> // flattens <<
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    // specialization for sequences
    template <typename Elements>
    struct sequence_properties
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
          , mpl::int_<karma::generator_properties::no_properties>
          , mpl::bitor_<mpl::_2, mpl::_1>
        >::type type;
    };
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
    template <typename Elements, typename Strict, typename Derived>
    struct base_sequence : nary_generator<Derived>
    {
        typedef typename traits::sequence_properties<Elements>::type properties;

        base_sequence(Elements const& elements)
          : elements(elements) {}

        typedef Elements elements_type;
        struct sequence_base_id;

        template <typename Context, typename Iterator = unused_type>
        struct attribute
        {
            // Put all the element attributes in a tuple
            typedef typename traits::build_attribute_sequence<
                Elements, Context, traits::sequence_attribute_transform
              , Iterator, karma::domain
            >::type all_attributes;

            // Now, build a fusion vector over the attributes. Note
            // that build_fusion_vector 1) removes all unused attributes
            // and 2) may return unused_type if all elements have
            // unused_type(s).
            typedef typename
                traits::build_fusion_vector<all_attributes>::type
            type_;

            // Finally, strip single element vectors into its
            // naked form: vector1<T> --> T
            typedef typename
                traits::strip_single_element_vector<type_>::type
            type;
        };

        // standard case. Attribute is a fusion tuple
        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute, typename Pred1, typename Pred2>
        bool generate_impl(OutputIterator& sink, Context& ctx
          , Delimiter const& d, Attribute& attr_, Pred1, Pred2) const
        {
            typedef detail::fail_function<
                OutputIterator, Context, Delimiter> fail_function;
            typedef traits::attribute_not_unused<Context> predicate;

            // wrap the attribute in a tuple if it is not a tuple or if the
            // attribute of this sequence is a single element tuple
            typedef typename attribute<Context>::type_ attr_type_;
            typename traits::wrap_if_not_tuple<Attribute
              , typename mpl::and_<
                    traits::one_element_sequence<attr_type_>
                  , mpl::not_<traits::one_element_sequence<Attribute> >
                >::type
            >::type attr(attr_);

            // return false if *any* of the generators fail
            bool r = spirit::any_if(elements, attr
                          , fail_function(sink, ctx, d), predicate());

            typedef typename traits::attribute_size<Attribute>::type size_type;

            // fail generating if sequences have not the same (logical) length
            return !r && (!Strict::value ||
                // This ignores container element count (which is not good),
                // but allows valid attributes to succeed. This will lead to
                // false positives (failing generators, even if they shouldn't)
                // if the embedded component is restricting the number of
                // container elements it consumes (i.e. repeat). This solution
                // is not optimal but much better than letting _all_ repetitive
                // components fail.
                Pred1::value ||
                size_type(traits::sequence_size<attr_type_>::value) == traits::size(attr_));
        }

        // Special case when Attribute is an stl container and the sequence's
        // attribute is not a one element sequence
        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate_impl(OutputIterator& sink, Context& ctx
          , Delimiter const& d, Attribute const& attr_
          , mpl::true_, mpl::false_) const
        {
            // return false if *any* of the generators fail
            typedef detail::fail_function<
                OutputIterator, Context, Delimiter> fail_function;

            typedef typename traits::container_iterator<
                typename add_const<Attribute>::type
            >::type iterator_type;

            typedef
                typename traits::make_indirect_iterator<iterator_type>::type
            indirect_iterator_type;
            typedef detail::pass_container<
                fail_function, Attribute, indirect_iterator_type, mpl::true_>
            pass_container;

            iterator_type begin = traits::begin(attr_);
            iterator_type end = traits::end(attr_);

            pass_container pass(fail_function(sink, ctx, d),
                indirect_iterator_type(begin), indirect_iterator_type(end));
            bool r = fusion::any(elements, pass);

            // fail generating if sequences have not the same (logical) length
            return !r && (!Strict::value || begin == end);
        }

        // main generate function. Dispatches to generate_impl depending
        // on the Attribute type.
        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            typedef typename traits::is_container<Attribute>::type
                is_container;

            typedef typename attribute<Context>::type_ attr_type_;
            typedef typename traits::one_element_sequence<attr_type_>::type
                is_one_element_sequence;

            return generate_impl(sink, ctx, d, attr, is_container()
              , is_one_element_sequence());
        }

        template <typename Context>
        info what(Context& context) const
        {
            info result("sequence");
            fusion::for_each(elements,
                spirit::detail::what_function<Context>(result, context));
            return result;
        }

        Elements elements;
    };

    template <typename Elements>
    struct sequence
      : base_sequence<Elements, mpl::false_, sequence<Elements> >
    {
        typedef base_sequence<Elements, mpl::false_, sequence> base_sequence_;

        sequence(Elements const& subject)
          : base_sequence_(subject) {}
    };

    template <typename Elements>
    struct strict_sequence
      : base_sequence<Elements, mpl::true_, strict_sequence<Elements> >
    {
        typedef base_sequence<Elements, mpl::true_, strict_sequence>
            base_sequence_;

        strict_sequence(Elements const& subject)
          : base_sequence_(subject) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Elements, bool strict_mode = false>
        struct make_sequence
          : make_nary_composite<Elements, sequence>
        {};

        template <typename Elements>
        struct make_sequence<Elements, true>
          : make_nary_composite<Elements, strict_sequence>
        {};
    }

    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::shift_left, Elements, Modifiers>
      : detail::make_sequence<Elements, detail::get_stricttag<Modifiers>::value>
    {};

    ///////////////////////////////////////////////////////////////////////////
    // Helper template allowing to get the required container type for a rule
    // attribute, which is part of a sequence.
    template <typename Iterator>
    struct make_sequence_iterator_range
    {
        typedef iterator_range<detail::indirect_iterator<Iterator> > type;
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements>
    struct has_semantic_action<karma::sequence<Elements> >
      : nary_has_semantic_action<Elements> {};

    template <typename Elements>
    struct has_semantic_action<karma::strict_sequence<Elements> >
      : nary_has_semantic_action<Elements> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<karma::sequence<Elements>, Attribute, Context
          , Iterator>
      : mpl::true_ {};

    template <typename Elements, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<karma::strict_sequence<Elements>, Attribute
          , Context, Iterator>
      : mpl::true_ {};
}}}

#endif
