/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_OPERATOR_SEQUENCE_BASE_HPP
#define BOOST_SPIRIT_QI_OPERATOR_SEQUENCE_BASE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/detail/pass_container.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/algorithm/any_if.hpp>
#include <boost/spirit/home/support/detail/what_function.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/sequence_base_id.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/mpl/identity.hpp>

namespace boost { namespace spirit { namespace qi
{
    template <typename Derived, typename Elements>
    struct sequence_base // this class is shared by sequence and expect
      : nary_parser<Derived>
    {
        typedef Elements elements_type;
        struct sequence_base_id;

        template <typename Context, typename Iterator>
        struct attribute
        {
            // Put all the element attributes in a tuple
            typedef typename traits::build_attribute_sequence<
                Elements, Context, traits::sequence_attribute_transform
              , Iterator, qi::domain
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

        sequence_base(Elements const& elements_)
          : elements(elements_) {}

        // standard case. Attribute is a fusion tuple
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse_impl(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_, mpl::false_) const
        {
            Iterator iter = first;
            typedef traits::attribute_not_unused<Context, Iterator> predicate;

            // wrap the attribute in a tuple if it is not a tuple or if the 
            // attribute of this sequence is a single element tuple
            typedef typename attribute<Context, Iterator>::type_ attr_type_;
            typename traits::wrap_if_not_tuple<Attribute
              , typename mpl::and_<
                    traits::one_element_sequence<attr_type_>
                  , mpl::not_<traits::one_element_sequence<Attribute> >
                >::type
            >::type attr_local(attr_);

            // return false if *any* of the parsers fail
            if (spirit::any_if(elements, attr_local
              , Derived::fail_function(iter, last, context, skipper), predicate()))
                return false;
            first = iter;
            return true;
        }

        // Special case when Attribute is an stl container
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse_impl(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_, mpl::true_) const
        {
            // ensure the attribute is actually a container type
            traits::make_container(attr_);

            Iterator iter = first;
            // return false if *any* of the parsers fail
            if (fusion::any(elements
              , detail::make_sequence_pass_container(
                    Derived::fail_function(iter, last, context, skipper), attr_))
                )
                return false;
            first = iter;
            return true;
        }

        // main parse function. Dispatches to parse_impl depending
        // on the Attribute type.
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            return parse_impl(first, last, context, skipper, attr_
              , traits::is_container<Attribute>());
        }

        template <typename Context>
        info what(Context& context) const
        {
            info result(this->derived().id());
            fusion::for_each(elements,
                spirit::detail::what_function<Context>(result, context));
            return result;
        }

        Elements elements;
    };
}}}

#endif
