// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file vector_of.hpp
/// \brief Include support for vector constrains for the bimap container

#ifndef BOOST_BIMAP_VECTOR_OF_HPP
#define BOOST_BIMAP_VECTOR_OF_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/bimap/detail/user_interface_config.hpp>

#include <boost/mpl/bool.hpp>

#include <boost/concept_check.hpp>

#include <boost/bimap/detail/concept_tags.hpp>

#include <boost/bimap/tags/support/value_type_of.hpp>

#include <boost/bimap/detail/generate_index_binder.hpp>
#include <boost/bimap/detail/generate_view_binder.hpp>
#include <boost/bimap/detail/generate_relation_binder.hpp>

#include <boost/multi_index/random_access_index.hpp>

#include <boost/bimap/views/vector_map_view.hpp>
#include <boost/bimap/views/vector_set_view.hpp>

namespace boost {
namespace bimaps {


/// \brief Set Type Specification
/**
This struct is used to specify a set specification.
It is not a container, it is just a metaprogramming facility to
express the type of a set. Generally, this specification will
be used in other place to create a container.
It has the same syntax that an std::vector instantiation, except
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

BOOST_STATIC_ASSERT( is_set_type_of< vector_of<Type> >::value )

BOOST_STATIC_ASSERT
(
     is_same
     <
        vector_of<Type>::index_bind
        <
            KeyExtractor,
            Tag

        >::type,

        random_access< tag<Tag>, KeyExtractor >

    >::value
)

typedef bimap
<
    vector_of<Type>, RightKeyType

> bimap_with_left_type_as_vector;

BOOST_STATIC_ASSERT
(
    is_same
    <
        vector_of<Type>::map_view_bind
        <
            member_at::left,
            bimap_with_left_type_as_vector

        >::type,

        vector_map_view< member_at::left, bimap_with_left_type_as_vector >

    >::value
)

\endcode

See also vector_of_relation.
                                                                        **/

template< class Type >
struct vector_of : public ::boost::bimaps::detail::set_type_of_tag
{
    /// User type, can be tagged
    typedef Type user_type;

    /// Type of the object that will be stored in the vector
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::tags::support::
        value_type_of<user_type>::type value_type;


    struct lazy_concept_checked
    {
        BOOST_CLASS_REQUIRE ( value_type,
                              boost, AssignableConcept );

        typedef vector_of type;
    };

    BOOST_BIMAP_GENERATE_INDEX_BINDER_0CP_NO_EXTRACTOR(

        // binds to
        multi_index::random_access
    )

    BOOST_BIMAP_GENERATE_MAP_VIEW_BINDER(

        // binds to
        views::vector_map_view
    )

    BOOST_BIMAP_GENERATE_SET_VIEW_BINDER(

        // binds to
        views::vector_set_view
    )

    typedef mpl::bool_<true> mutable_key;
};


/// \brief Set Of Relation Specification
/**
This struct is similar to vector_of but it is bind logically to a
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

See also vector_of, is_set_type_of_relation.
                                                                **/

struct vector_of_relation : public ::boost::bimaps::detail::set_type_of_relation_tag
{
    BOOST_BIMAP_GENERATE_RELATION_BINDER_0CP(

        // binds to
        vector_of
    )

    typedef mpl::bool_<true>  left_mutable_key;
    typedef mpl::bool_<true> right_mutable_key;
};


} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_VECTOR_OF_HPP

