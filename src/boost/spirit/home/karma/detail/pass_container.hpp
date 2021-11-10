/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_KARMA_DETAIL_PASS_CONTAINER_HPP
#define BOOST_SPIRIT_KARMA_DETAIL_PASS_CONTAINER_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/support/detail/hold_any.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/or.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/fusion/include/deduce_sequence.hpp>

#include <boost/mpl/print.hpp>

namespace boost { namespace spirit { namespace karma { namespace detail
{
    // Helper meta-function allowing to evaluate weak substitutability and
    // negate the result if the predicate (Sequence) is not true
    template <typename Sequence, typename Attribute, typename ValueType>
    struct negate_weak_substitute_if_not
      : mpl::if_<
            Sequence
          , typename traits::is_weak_substitute<Attribute, ValueType>::type
          , typename mpl::not_<
                traits::is_weak_substitute<Attribute, ValueType>
            >::type>
    {};

    // pass_through_container: utility to check decide whether a provided
    // container attribute needs to be passed through to the current component
    // or of we need to split the container by passing along instances of its
    // value type

    // if the expected attribute of the current component is neither a Fusion
    // sequence nor a container, we will pass through the provided container
    // only if its value type is not compatible with the component
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence, typename Enable = void>
    struct pass_through_container_base
      : negate_weak_substitute_if_not<Sequence, ValueType, Attribute>
    {};

    // Specialization for fusion sequences, in this case we check whether all
    // the types in the sequence are convertible to the lhs attribute.
    //
    // We return false if the rhs attribute itself is a fusion sequence, which
    // is compatible with the LHS sequence (we want to pass through this
    // attribute without it being split apart).
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence = mpl::true_>
    struct not_compatible_element
      : mpl::and_<
            negate_weak_substitute_if_not<Sequence, Container, Attribute>
          , negate_weak_substitute_if_not<Sequence, ValueType, Attribute> >
    {};

    // If the value type of the container is not a Fusion sequence, we pass
    // through the container if each of the elements of the Attribute
    // sequence is compatible with either the container or its value type.
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence
      , bool IsSequence = fusion::traits::is_sequence<ValueType>::value>
    struct pass_through_container_fusion_sequence
    {
        typedef typename mpl::find_if<
            Attribute, not_compatible_element<Container, ValueType, mpl::_1>
        >::type iter;
        typedef typename mpl::end<Attribute>::type end;

        typedef typename is_same<iter, end>::type type;
    };

    // If both, the Attribute and the value type of the provided container
    // are Fusion sequences, we pass the container only if the two
    // sequences are not compatible.
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence>
    struct pass_through_container_fusion_sequence<
            Container, ValueType, Attribute, Sequence, true>
    {
        typedef typename mpl::find_if<
            Attribute
          , not_compatible_element<Container, ValueType, mpl::_1, Sequence>
        >::type iter;
        typedef typename mpl::end<Attribute>::type end;

        typedef typename is_same<iter, end>::type type;
    };

    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence>
    struct pass_through_container_base<Container, ValueType, Attribute
          , Sequence
          , typename enable_if<fusion::traits::is_sequence<Attribute> >::type>
      : pass_through_container_fusion_sequence<
            Container, ValueType, Attribute, Sequence>
    {};

    // Specialization for containers
    //
    // If the value type of the attribute of the current component is not
    // a Fusion sequence, we have to pass through the provided container if
    // both are compatible.
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence, typename AttributeValueType
      , bool IsSequence = fusion::traits::is_sequence<AttributeValueType>::value>
    struct pass_through_container_container
      : mpl::or_<
            traits::is_weak_substitute<Container, Attribute>
          , traits::is_weak_substitute<Container, AttributeValueType> >
    {};

    // If the value type of the exposed container attribute is a Fusion
    // sequence, we use the already existing logic for those.
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence, typename AttributeValueType>
    struct pass_through_container_container<
            Container, ValueType, Attribute, Sequence, AttributeValueType, true>
      : pass_through_container_fusion_sequence<
            Container, ValueType, AttributeValueType, Sequence>
    {};

    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence>
    struct pass_through_container_base<Container, ValueType, Attribute
          , Sequence
          , typename enable_if<traits::is_container<Attribute> >::type>
      : detail::pass_through_container_container<
          Container, ValueType, Attribute, Sequence
        , typename traits::container_value<Attribute>::type>
    {};

    // Specialization for exposed optional attributes
    //
    // If the type embedded in the exposed optional is not a Fusion
    // sequence we pass through the container attribute if it is compatible
    // either to the optionals embedded type or to the containers value
    // type.
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence
      , bool IsSequence = fusion::traits::is_sequence<Attribute>::value>
    struct pass_through_container_optional
      : mpl::or_<
            traits::is_weak_substitute<Container, Attribute>
          , traits::is_weak_substitute<ValueType, Attribute> >
    {};

    // If the embedded type of the exposed optional attribute is a Fusion
    // sequence, we use the already existing logic for those.
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence>
    struct pass_through_container_optional<
            Container, ValueType, Attribute, Sequence, true>
      : pass_through_container_fusion_sequence<
            Container, ValueType, Attribute, Sequence>
    {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence>
    struct pass_through_container
      : pass_through_container_base<Container, ValueType, Attribute, Sequence>
    {};

    // Handle optional attributes
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence>
    struct pass_through_container<
            Container, ValueType, boost::optional<Attribute>, Sequence>
      : pass_through_container_optional<
            Container, ValueType, Attribute, Sequence>
    {};

    // If both, the containers value type and the exposed attribute type are
    // optionals we are allowed to pass through the container only if the
    // embedded types of those optionals are not compatible.
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence>
    struct pass_through_container<
            Container, boost::optional<ValueType>, boost::optional<Attribute>
          , Sequence>
      : mpl::not_<traits::is_weak_substitute<ValueType, Attribute> >
    {};

    // Specialization for exposed variant attributes
    //
    // We pass through the container attribute if at least one of the embedded
    // types in the variant requires to pass through the attribute

#if !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)
    template <typename Container, typename ValueType, typename Sequence
      , typename T>
    struct pass_through_container<Container, ValueType, boost::variant<T>
          , Sequence>
      : pass_through_container<Container, ValueType, T, Sequence>
    {};

    template <typename Container, typename ValueType, typename Sequence
      , typename T0, typename ...TN>
    struct pass_through_container<Container, ValueType
          , boost::variant<T0, TN...>, Sequence>
      : mpl::bool_<pass_through_container<
            Container, ValueType, T0, Sequence
            >::type::value || pass_through_container<
                Container, ValueType, boost::variant<TN...>, Sequence
            >::type::value>
    {};
#else
#define BOOST_SPIRIT_PASS_THROUGH_CONTAINER(z, N, _)                          \
    pass_through_container<Container, ValueType,                              \
        BOOST_PP_CAT(T, N), Sequence>::type::value ||                         \
    /***/

    // make sure unused variant parameters do not affect the outcome
    template <typename Container, typename ValueType, typename Sequence>
    struct pass_through_container<Container, ValueType
          , boost::detail::variant::void_, Sequence>
      : mpl::false_
    {};

    template <typename Container, typename ValueType, typename Sequence
      , BOOST_VARIANT_ENUM_PARAMS(typename T)>
    struct pass_through_container<Container, ValueType
          , boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Sequence>
      : mpl::bool_<BOOST_PP_REPEAT(BOOST_VARIANT_LIMIT_TYPES
          , BOOST_SPIRIT_PASS_THROUGH_CONTAINER, _) false>
    {};

#undef BOOST_SPIRIT_PASS_THROUGH_CONTAINER
#endif
}}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // forwarding customization point for domain karma::domain
    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence>
    struct pass_through_container<
            Container, ValueType, Attribute, Sequence, karma::domain>
      : karma::detail::pass_through_container<
            Container, ValueType, Attribute, Sequence>
    {};
}}}

namespace boost { namespace spirit { namespace karma { namespace detail
{
    template <typename Iterator>
    struct pass_container_base
    {
        pass_container_base(Iterator begin, Iterator end)
          : iter(begin), end(end)
        {}

        mutable Iterator iter;
        mutable Iterator end;
    };

    template <typename Iterator>
    struct pass_container_base<Iterator&>
    {
        pass_container_base(Iterator& begin, Iterator& end)
          : iter(begin), end(end)
        {}

        Iterator& iter;
        Iterator& end;
    };

    ///////////////////////////////////////////////////////////////////////////
    // This function handles the case where the attribute (Attr) given
    // to the sequence is an STL container. This is a wrapper around F.
    // The function F does the actual generating.
    template <typename F, typename Attr, typename Iterator, typename Sequence>
    struct pass_container : pass_container_base<Iterator>
    {
        typedef pass_container_base<Iterator> base_type;
        typedef typename F::context_type context_type;

        pass_container(F const& f, Iterator begin, Iterator end)
          : base_type(begin, end)
          , f(f)
        {}

        bool is_at_end() const
        {
            return traits::compare(this->iter, this->end);
        }

        void next()
        {
            traits::next(this->iter);
        }

        // this is for the case when the current element expects an attribute
        // which is taken from the next entry in the container
        template <typename Component>
        bool dispatch_container(Component const& component, mpl::false_) const
        {
            // get the next value to generate from container
            if (!is_at_end() && !f(component, traits::deref(this->iter)))
            {
                // needs to return false as long as everything is ok
                traits::next(this->iter);
                return false;
            }

            // either no elements available any more or generation failed
            return true;
        }

        // this is for the case when the current element is able to handle an
        // attribute which is a container itself, this element will push its
        // data directly into the attribute container
        template <typename Component>
        bool dispatch_container(Component const& component, mpl::true_) const
        {
            return f(component, make_iterator_range(this->iter, this->end));
        }

        ///////////////////////////////////////////////////////////////////////
        // this is for the case when the current element doesn't expect an
        // attribute
        template <typename Component>
        bool dispatch_attribute(Component const& component, mpl::false_) const
        {
            return f(component, unused);
        }

        // the current element expects an attribute
        template <typename Component>
        bool dispatch_attribute(Component const& component, mpl::true_) const
        {
            typedef typename traits::container_value<Attr>::type value_type;
            typedef typename
                traits::attribute_of<Component, context_type>::type
            lhs_attribute;

            // this predicate detects, whether the value type of the container
            // attribute is a substitute for the attribute of the current
            // element
            typedef mpl::and_<
                traits::handles_container<Component, Attr, context_type>
              , traits::pass_through_container<
                    Attr, value_type, lhs_attribute, Sequence, karma::domain>
            > predicate;

            return dispatch_container(component, predicate());
        }

        // Dispatches to dispatch_main depending on the attribute type
        // of the Component
        template <typename Component>
        bool operator()(Component const& component) const
        {
            // we need to dispatch depending on the type of the attribute
            // of the current element (component). If this is has no attribute
            // we shouldn't use an element of the container but unused_type
            // instead
            typedef traits::not_is_unused<
                typename traits::attribute_of<Component, context_type>::type
            > predicate;

            return dispatch_attribute(component, predicate());
        }

        F f;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(pass_container& operator= (pass_container const&))
    };
}}}}

#endif
