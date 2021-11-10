/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c)      2010 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_ATTRIBUTES_FWD_OCT_01_2009_0715AM)
#define BOOST_SPIRIT_ATTRIBUTES_FWD_OCT_01_2009_0715AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#if (defined(__GNUC__) && (__GNUC__ < 4)) || \
    (defined(__APPLE__) && defined(__INTEL_COMPILER))
#include <boost/utility/enable_if.hpp>
#endif
#include <boost/spirit/home/support/unused.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace result_of
{
    // forward declaration only
    template <typename Exposed, typename Attribute>
    struct extract_from;

    template <typename T, typename Attribute>
    struct attribute_as;

    template <typename T>
    struct optional_value;

    template <typename Container>
    struct begin;

    template <typename Container>
    struct end;

    template <typename Iterator>
    struct deref;
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // Find out if T can be a strong substitute for Expected attribute
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Expected, typename Enable = void>
    struct is_substitute;

    ///////////////////////////////////////////////////////////////////////////
    // Find out if T can be a weak substitute for Expected attribute
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Expected, typename Enable = void>
    struct is_weak_substitute;

    ///////////////////////////////////////////////////////////////////////////
    // Determine if T is a proxy
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable = void>
    struct is_proxy;

    ///////////////////////////////////////////////////////////////////////////
    // Retrieve the attribute type to use from the given type
    //
    // This is needed to extract the correct attribute type from proxy classes
    // as utilized in FUSION_ADAPT_ADT et. al.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Attribute, typename Enable = void>
    struct attribute_type;

    ///////////////////////////////////////////////////////////////////////////
    // Retrieve the size of a fusion sequence (compile time)
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct sequence_size;

    ///////////////////////////////////////////////////////////////////////////
    // Retrieve the size of an attribute (runtime)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Attribute, typename Enable = void>
    struct attribute_size;

    template <typename Attribute>
    typename attribute_size<Attribute>::type
    size(Attribute const& attr);

    ///////////////////////////////////////////////////////////////////////////
    // Determines how we pass attributes to semantic actions. This
    // may be specialized. By default, all attributes are wrapped in
    // a fusion sequence, because the attribute has to be treated as being
    // a single value in any case (even if it actually already is a fusion
    // sequence in its own).
    ///////////////////////////////////////////////////////////////////////////
    template <typename Component, typename Attribute, typename Enable = void>
    struct pass_attribute;

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable = void>
    struct optional_attribute;

    ///////////////////////////////////////////////////////////////////////////
    // Sometimes the user needs to transform the attribute types for certain
    // attributes. This template can be used as a customization point, where
    // the user is able specify specific transformation rules for any attribute
    // type.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Exposed, typename Transformed, typename Domain
      , typename Enable = void>
    struct transform_attribute;

    ///////////////////////////////////////////////////////////////////////////
    // Qi only
    template <typename Attribute, typename Iterator, typename Enable = void>
    struct assign_to_attribute_from_iterators;

    template <typename Iterator, typename Attribute>
    void assign_to(Iterator const& first, Iterator const& last, Attribute& attr);

    template <typename Iterator>
    void assign_to(Iterator const&, Iterator const&, unused_type);

    template <typename Attribute, typename T, typename Enable = void>
    struct assign_to_attribute_from_value;

    template <typename Attribute, typename T, typename Enable = void>
    struct assign_to_container_from_value;

    template <typename T, typename Attribute>
    void assign_to(T const& val, Attribute& attr);

    template <typename T>
    void assign_to(T const&, unused_type);

    ///////////////////////////////////////////////////////////////////////////
    // Karma only
    template <typename Attribute, typename Exposed, typename Enable = void>
    struct extract_from_attribute;

    template <typename Attribute, typename Exposed, typename Enable = void>
    struct extract_from_container;

    template <typename Exposed, typename Attribute, typename Context>
    typename spirit::result_of::extract_from<Exposed, Attribute>::type
    extract_from(Attribute const& attr, Context& ctx
#if (defined(__GNUC__) && (__GNUC__ < 4)) || \
    (defined(__APPLE__) && defined(__INTEL_COMPILER))
      , typename enable_if<traits::not_is_unused<Attribute> >::type* = NULL
#endif
    );

    ///////////////////////////////////////////////////////////////////////////
    // Karma only
    template <typename T, typename Attribute, typename Enable = void>
    struct attribute_as;

    template <typename T, typename Attribute>
    typename spirit::result_of::attribute_as<T, Attribute>::type
    as(Attribute const& attr);

    template <typename T, typename Attribute>
    bool valid_as(Attribute const& attr);

    ///////////////////////////////////////////////////////////////////////////
    // return the type currently stored in the given variant
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable = void>
    struct variant_which;

    template <typename T>
    int which(T const& v);

    ///////////////////////////////////////////////////////////////////////////
    // Determine, whether T is a variant like type
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Domain = unused_type, typename Enable = void>
    struct not_is_variant;

    ///////////////////////////////////////////////////////////////////////////
    // Determine, whether T is a variant like type
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Domain = unused_type, typename Enable = void>
    struct not_is_optional;

    ///////////////////////////////////////////////////////////////////////////
    // Clear data efficiently
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable = void>
    struct clear_value;

    ///////////////////////////////////////////////////////////////////////
    // Determine the value type of the given container type
    ///////////////////////////////////////////////////////////////////////
    template <typename Container, typename Enable = void>
    struct container_value;

    template <typename Container, typename Enable = void>
    struct container_iterator;

    template <typename T, typename Enable = void>
    struct is_container;

    template <typename T, typename Enable = void>
    struct is_iterator_range;

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Attribute, typename Context = unused_type
            , typename Iterator = unused_type, typename Enable = void>
    struct handles_container;

    template <typename Container, typename ValueType, typename Attribute
      , typename Sequence, typename Domain, typename Enable = void>
    struct pass_through_container;

    ///////////////////////////////////////////////////////////////////////////
    // Qi only
    template <typename Container, typename T, typename Enable = void>
    struct push_back_container;

    template <typename Container, typename Enable = void>
    struct is_empty_container;

    template <typename Container, typename Enable = void>
    struct make_container_attribute;

    ///////////////////////////////////////////////////////////////////////
    // Determine the iterator type of the given container type
    // Karma only
    ///////////////////////////////////////////////////////////////////////
    template <typename Container, typename Enable = void>
    struct begin_container;

    template <typename Container, typename Enable = void>
    struct end_container;

    template <typename Iterator, typename Enable = void>
    struct deref_iterator;

    template <typename Iterator, typename Enable = void>
    struct next_iterator;

    template <typename Iterator, typename Enable = void>
    struct compare_iterators;

    ///////////////////////////////////////////////////////////////////////////
    // Print the given attribute of type T to the stream given as Out
    ///////////////////////////////////////////////////////////////////////////
    template <typename Out, typename T, typename Enable = void>
    struct print_attribute_debug;

    template <typename Out, typename T>
    void print_attribute(Out&, T const&);

    template <typename Out>
    void print_attribute(Out&, unused_type);

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Enable = void>
    struct token_printer_debug;

    template<typename Out, typename T>
    void print_token(Out&, T const&);

    ///////////////////////////////////////////////////////////////////////////
    // Access attributes from a karma symbol table
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Attribute, typename Enable = void>
    struct symbols_lookup;

    template <typename Attribute, typename T, typename Enable = void>
    struct symbols_value;

    ///////////////////////////////////////////////////////////////////////////
    // transform attribute types exposed from compound operator components
    ///////////////////////////////////////////////////////////////////////////
    template <typename Attribute, typename Domain>
    struct alternative_attribute_transform;

    template <typename Attribute, typename Domain>
    struct sequence_attribute_transform;

    template <typename Attribute, typename Domain>
    struct permutation_attribute_transform;

    template <typename Attribute, typename Domain>
    struct sequential_or_attribute_transform;
}}}

#endif

