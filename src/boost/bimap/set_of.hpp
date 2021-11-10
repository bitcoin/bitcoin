// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file set_of.hpp
/// \brief Include support for set constrains for the bimap container

#ifndef BOOST_BIMAP_SET_OF_HPP
#define BOOST_BIMAP_SET_OF_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/bimap/detail/user_interface_config.hpp>

#include <functional>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/aux_/na.hpp>

#include <boost/concept_check.hpp>

#include <boost/bimap/detail/concept_tags.hpp>

#include <boost/bimap/detail/generate_index_binder.hpp>
#include <boost/bimap/detail/generate_view_binder.hpp>
#include <boost/bimap/detail/generate_relation_binder.hpp>

#include <boost/bimap/tags/support/value_type_of.hpp>

#include <boost/multi_index/ordered_index.hpp>

#include <boost/bimap/views/map_view.hpp>
#include <boost/bimap/views/set_view.hpp>

namespace boost {
namespace bimaps {

/// \brief Set Type Specification
/**
This struct is used to specify a set specification.
It is not a container, it is just a metaprogramming facility to
express the type of a set. Generally, this specification will
be used in other place to create a container.
It has the same syntax that an std::set instantiation, except
that the allocator cannot be specified. The rationale behind
this difference is that the allocator is not part of the set
type specification, rather it is a container configuration
parameter.
The first parameter is the type of the objects in the set, and
the second one is a Functor that compares them.
Bimap binding metafunctions can be used with this class in
the following way:

\code
using namespace support;

BOOST_STATIC_ASSERT( is_set_type_of< set_of<Type> >::value )

BOOST_STATIC_ASSERT
(
     is_same
     <
        set_of<Type,KeyCompare>::index_bind
        <
            KeyExtractor,
            Tag

        >::type,

        ordered_unique< tag<Tag>, KeyExtractor, KeyCompare >

    >::value
)

typedef bimap
<
    set_of<Type>, RightKeyType

> bimap_with_left_type_as_set;

BOOST_STATIC_ASSERT
(
    is_same
    <
        set_of<Type>::map_view_bind
        <
            member_at::left,
            bimap_with_left_type_as_set

        >::type,

        map_view< member_at::left, bimap_with_left_type_as_set >

    >::value
)

\endcode

See also set_of_relation.
                                                                        **/

template
<
    class KeyType,
    class KeyCompare = std::less< BOOST_DEDUCED_TYPENAME 
        ::boost::bimaps::tags::support::value_type_of<KeyType>::type >
>
struct set_of : public ::boost::bimaps::detail::set_type_of_tag
{
    /// User type, can be tagged
    typedef KeyType user_type;

    /// Type of the object that will be stored in the set
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::tags::support::
        value_type_of<user_type>::type value_type;

    /// Functor that compare two keys
    typedef KeyCompare key_compare;

    struct lazy_concept_checked
    {
        BOOST_CLASS_REQUIRE ( value_type,
                              boost, AssignableConcept );

        BOOST_CLASS_REQUIRE4( key_compare, bool, value_type, value_type,
                              boost, BinaryFunctionConcept );

        typedef set_of type;
    };

    BOOST_BIMAP_GENERATE_INDEX_BINDER_1CP(

        // binds to
        multi_index::ordered_unique,

        // with
        key_compare
    )

    BOOST_BIMAP_GENERATE_MAP_VIEW_BINDER(

        // binds to
        views::map_view
    )

    BOOST_BIMAP_GENERATE_SET_VIEW_BINDER(

        // binds to
        views::set_view
    )

    typedef mpl::bool_<false> mutable_key;
};


/// \brief Set Of Relation Specification
/**
This struct is similar to set_of but it is bind logically to a
relation. It is used in the bimap instantiation to specify the
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

See also set_of, is_set_type_of_relation.
                                                                **/

template< class KeyCompare = std::less< _relation > >
struct set_of_relation : public ::boost::bimaps::detail::set_type_of_relation_tag
{
    /// Functor that compare two keys
    typedef KeyCompare key_compare;

    BOOST_BIMAP_GENERATE_RELATION_BINDER_1CP(

        // binds to
        set_of,

        // with
        key_compare
    )

    typedef mpl::bool_<false>  left_mutable_key;
    typedef mpl::bool_<false> right_mutable_key;
};

} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_SET_OF_HPP

