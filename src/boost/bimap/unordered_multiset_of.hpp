// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file unordered_multiset_of.hpp
/// \brief Include support for unordered_multiset constrains for the bimap container

#ifndef BOOST_BIMAP_UNORDERED_MULTISET_OF_HPP
#define BOOST_BIMAP_UNORDERED_MULTISET_OF_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/bimap/detail/user_interface_config.hpp>

#include <cstdlib>
#include <functional>
#include <boost/functional/hash.hpp>
#include <boost/mpl/bool.hpp>

#include <boost/concept_check.hpp>

#include <boost/bimap/detail/concept_tags.hpp>

#include <boost/bimap/tags/support/value_type_of.hpp>

#include <boost/bimap/detail/generate_index_binder.hpp>
#include <boost/bimap/detail/generate_view_binder.hpp>
#include <boost/bimap/detail/generate_relation_binder.hpp>

#include <boost/multi_index/hashed_index.hpp>

#include <boost/bimap/views/unordered_multimap_view.hpp>
#include <boost/bimap/views/unordered_multiset_view.hpp>

namespace boost {
namespace bimaps {


/// \brief Set Type Specification
/**
This struct is used to specify an unordered_multiset specification.
It is not a container, it is just a metaprogramming facility to
express the type of a set. Generally, this specification will
be used in other place to create a container.
It has the same syntax that an tr1::unordered_multiset instantiation,
except that the allocator cannot be specified. The rationale behind
this difference is that the allocator is not part of the
unordered_multiset type specification, rather it is a container
configuration parameter.
The first parameter is the type of the objects in the set, the
second one is a Hash Functor that takes objects of this type, and
the third one is a Functor that compares them for equality.
Bimap binding metafunctions can be used with this class in
the following way:

\code
using namespace support;

BOOST_STATIC_ASSERT( is_set_type_of< unordered_multiset_of<Type> >::value )

BOOST_STATIC_ASSERT
(
     is_same
     <
        compute_index_type
        <
            unordered_multiset_of<Type,HashFunctor,EqualKey>,
            KeyExtractor,
            Tag

        >::type
        ,
        hashed_nonunique< tag<Tag>, KeyExtractor, HashFunctor, EqualKey >

    >::value
)

typedef bimap
<
    unordered_multiset_of<Type>, RightKeyType

> bimap_with_left_type_as_unordered_multiset;

BOOST_STATIC_ASSERT
(
    is_same
    <
        compute_map_view_type
        <
            member_at::left,
            bimap_with_left_type_as_unordered_multiset

        >::type,

        unordered_multimap_view
        <
            member_at::left,
            bimap_with_left_type_as_unordered_multiset
        >

    >::value
)

\endcode

See also unordered_multiset_of_relation.
                                                                        **/

template
<
    class KeyType,
    class HashFunctor   = hash< BOOST_DEDUCED_TYPENAME 
        ::boost::bimaps::tags::support::value_type_of<KeyType>::type >,
    class EqualKey      = std::equal_to< BOOST_DEDUCED_TYPENAME 
        ::boost::bimaps::tags::support::value_type_of<KeyType>::type >
>
struct unordered_multiset_of : public ::boost::bimaps::detail::set_type_of_tag
{
    /// User type, can be tagged
    typedef KeyType user_type;

    /// Type of the object that will be stored in the container
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::tags::support::
        value_type_of<user_type>::type value_type;

    /// Hash Functor that takes value_type objects
    typedef HashFunctor     hasher;

    /// Functor that compare two value_type objects for equality
    typedef EqualKey        key_equal;

    struct lazy_concept_checked
    {
        BOOST_CLASS_REQUIRE ( value_type,
                              boost, AssignableConcept );

        BOOST_CLASS_REQUIRE3( hasher, std::size_t, value_type,
                              boost, UnaryFunctionConcept );

        BOOST_CLASS_REQUIRE4( key_equal, bool, value_type, value_type,
                              boost, BinaryFunctionConcept );

        typedef unordered_multiset_of type;
    };

    BOOST_BIMAP_GENERATE_INDEX_BINDER_2CP(

        // binds to
        multi_index::hashed_non_unique,

        // with
        hasher,
        key_equal
    )

    BOOST_BIMAP_GENERATE_MAP_VIEW_BINDER(

        // binds to
        views::unordered_multimap_view
    )

    BOOST_BIMAP_GENERATE_SET_VIEW_BINDER(

        // binds to
        views::unordered_multiset_view
    )

    typedef mpl::bool_<false> mutable_key;
};


/// \brief Set Of Relation Specification
/**
This struct is similar to unordered_multiset_of but it is bind logically
to a relation. It is used in the bimap instantiation to specify the
desired type of the main view. This struct implements internally
a metafunction named bind_to that manages the quite complicated
task of finding the right type of the set for the relation.

\code
template<class Relation>
struct bind_to
{
    typedef -unspecified- type;
};
\endcode

See also unordered_multiset_of, is_set_type_of_relation.
                                                                **/

template
<
    class HashFunctor   = hash< _relation >,
    class EqualKey      = std::equal_to< _relation >
>
struct unordered_multiset_of_relation : public ::boost::bimaps::detail::set_type_of_relation_tag
{
    /// Hash Functor that takes value_type objects
    typedef HashFunctor     hasher;

    /// Functor that compare two value_type objects for equality
    typedef EqualKey        key_equal;


    BOOST_BIMAP_GENERATE_RELATION_BINDER_2CP(

        // binds to
        unordered_multiset_of,

        // with
        hasher,
        key_equal
    )

    typedef mpl::bool_<false>  left_mutable_key;
    typedef mpl::bool_<false> right_mutable_key;
};


} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_UNORDERED_MULTISET_OF_HPP

