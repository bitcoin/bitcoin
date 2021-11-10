// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file bimap.hpp
/// \brief Includes the basic bimap container

/** \mainpage notitle
\n
\image html http://matias.capeletto.googlepages.com/boost.bimap.reference.logo.png

\section Introduction

This is the complete reference of Boost.Bimap.

After getting a good understanding of the library from a user perspective
the next step will be:

    - Understand the tagged idiom. (boost::bimaps::tags)
    - Understand the internals of the relation class (boost::bimaps::relation)
    - Read the container_adaptor toolbox docs (boost::bimaps::container_adaptor)
    - Understand the internals of the bimap class. (boost::bimaps, boost::bimaps::views
      and boost::bimaps::detail)


                                                                        **/

/** \defgroup mutant_group mutant idiom
\brief A safe wrapper around reinterpret_cast
                                                                        **/

/** \defgroup relation_group relation
\brief The relation
                                                                        **/

/** \defgroup tags_group tagged idiom
\brief The tagged idiom
                                                                        **/


#ifndef BOOST_BIMAP_BIMAP_HPP
#define BOOST_BIMAP_BIMAP_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/bimap/detail/user_interface_config.hpp>
#include <boost/mpl/aux_/na.hpp>

#ifndef BOOST_BIMAP_DISABLE_SERIALIZATION
    #include <boost/serialization/nvp.hpp>
#endif // BOOST_BIMAP_DISABLE_SERIALIZATION

// Boost.Bimap
#include <boost/bimap/detail/bimap_core.hpp>
#include <boost/bimap/detail/map_view_base.hpp>
#include <boost/bimap/detail/modifier_adaptor.hpp>
#include <boost/bimap/relation/support/data_extractor.hpp>
#include <boost/bimap/relation/support/member_with_tag.hpp>

#include <boost/bimap/support/map_type_by.hpp>
#include <boost/bimap/support/map_by.hpp>
#include <boost/bimap/support/iterator_type_by.hpp>

/// \brief The namespace where all the boost libraries lives.

namespace boost {

/// \brief Boost.Bimap library namespace
/**
All the entities in the library are defined in this namespace.
                                                                    **/
namespace bimaps {

/// \brief The bimap class is the entry point to the library.
/**
This class manages the instantiation of the desired bimap type.
As there are several types of bidirectional maps that can be
created using it. the main job of it is to find the desired
type. This is done using metaprogramming to obtain the relation
type that will be stored, the map_view type of each side and
the set_view type of the general relationship. The instantiation
is kept simple using an extended standard set theory, where a
bidirectional map type is defined by the set types it relates.
For example, a bidirectional map that has multimap semantics
viewed from both sides is defined by specifying that the two
keys sets are of \c multiset_of<Key> type.
This allows the bimap class to support seamingless N-N, 1-N,
ordered/unordered and even vector-list types of mapping.
The three last parameters are used to specify the set type of
the relation, an inplace hooked data class and the allocator
type. As a help to the bimap user, these parameters support
default types but use a special idiom that allow them to be
specified without interleaving the usual use_default keyword.
The possible bimap instantiation are enumerated here:
\c {Side}KeyType can be directly a type, this is default to
\c set_of<{Side}KeyType>, or can be a \c {SetType}_of<Type>
specification. Additionally this two parameters can be tagged
to specify others tags instead of the usual \c member_at::{Side}
ones.


\code

    typedef bimap
    <
        LeftCollectionType, RightCollectionType

        [ , SetTypeOfRelation  ]  // Default to left_based
        [ , info_hook< Info >  ]  // Default to no info
        [ , Allocator          ]  // Default to std::allocator<>

    > bm;

\endcode

                                                                       **/


template
<
    class KeyTypeA, class KeyTypeB,
    class AP1 = ::boost::mpl::na,
    class AP2 = ::boost::mpl::na,
    class AP3 = ::boost::mpl::na
>
class bimap
:
    // Bimap Core, use mpl magic to find the desired bimap type

    public ::boost::bimaps::detail::bimap_core<KeyTypeA,KeyTypeB,AP1,AP2,AP3>,

    // You can use bimap as a collection of relations

    public ::boost::bimaps::detail::bimap_core<KeyTypeA,KeyTypeB,AP1,AP2,AP3>
                ::relation_set,

    // Include extra typedefs (i.e. left_local_iterator for unordered_map)

    public ::boost::bimaps::detail:: left_map_view_extra_typedefs<
        BOOST_DEDUCED_TYPENAME ::boost::bimaps::detail::left_map_view_type<
            ::boost::bimaps::detail::bimap_core<KeyTypeA,KeyTypeB,AP1,AP2,AP3>
        >::type
    >,
    public ::boost::bimaps::detail::right_map_view_extra_typedefs< 
        BOOST_DEDUCED_TYPENAME ::boost::bimaps::detail::right_map_view_type<
            ::boost::bimaps::detail::bimap_core<KeyTypeA,KeyTypeB,AP1,AP2,AP3>
        >::type
    >
{
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::detail::
        bimap_core<KeyTypeA,KeyTypeB,AP1,AP2,AP3> base_;

    BOOST_DEDUCED_TYPENAME base_::core_type core;

    public:

    // metadata --------------------------------------------------------

    /*
    // The rest is computed in the core, because it is quite difficult to
    // expose a nice interface with so many metaprogramming stuff.
    
    // Map by {side} metadata

    typedef -unspecified- {side}_tag;
    typedef -unspecified- {side}_data_type;
    typedef -unspecified- {side}_value_type;
    typedef -unspecified- {side}_key_type;
    
    // There are other typedefs for definitions of different map views
    
    ------------------------------------------------------------------*/

    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::detail::
          left_map_view_type<base_>::type  left_map;
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::detail::
         right_map_view_type<base_>::type right_map;
	
    typedef BOOST_DEDUCED_TYPENAME
         left_map::iterator        left_iterator;
    typedef BOOST_DEDUCED_TYPENAME
         left_map::const_iterator  left_const_iterator;

    typedef BOOST_DEDUCED_TYPENAME
         right_map::iterator       right_iterator;
    typedef BOOST_DEDUCED_TYPENAME
         right_map::const_iterator right_const_iterator;

    typedef BOOST_DEDUCED_TYPENAME
         left_map::reference       left_reference;
    typedef BOOST_DEDUCED_TYPENAME
         left_map::const_reference left_const_reference;

    typedef BOOST_DEDUCED_TYPENAME
        right_map::reference       right_reference;
    typedef BOOST_DEDUCED_TYPENAME
        right_map::const_reference right_const_reference;

    typedef BOOST_DEDUCED_TYPENAME base_::relation::info_type info_type;

    typedef BOOST_DEDUCED_TYPENAME base_::core_type::allocator_type allocator_type; 
    
    /// Left map view
    left_map  left;

    /// Right map view
    right_map right;

    typedef BOOST_DEDUCED_TYPENAME base_::logic_relation_set_tag 
                                          logic_relation_set_tag;
    typedef BOOST_DEDUCED_TYPENAME base_::logic_left_tag logic_left_tag;
    typedef BOOST_DEDUCED_TYPENAME base_::logic_right_tag logic_right_tag;
    typedef BOOST_DEDUCED_TYPENAME base_::core_type::ctor_args_list 
                                                     ctor_args_list;

   bimap(const allocator_type& al = allocator_type()) :

       base_::relation_set(
           ::boost::multi_index::get<
               logic_relation_set_tag
           >(core)
       ),

       core(al),

       left (
           ::boost::multi_index::get<
               logic_left_tag
           >(core)
       ),
       right (
           ::boost::multi_index::get<
               logic_right_tag
           >(core)
       )

   {}

   template< class InputIterator >
   bimap(InputIterator first,InputIterator last,
         const allocator_type& al = allocator_type()) :

       base_::relation_set(
           ::boost::multi_index::get<
               BOOST_DEDUCED_TYPENAME base_::logic_relation_set_tag>(core)
       ),

       core(first,last,ctor_args_list(),al),

       left (
           ::boost::multi_index::get<
               BOOST_DEDUCED_TYPENAME base_::logic_left_tag>(core)
       ),
       right (
           ::boost::multi_index::get<
               BOOST_DEDUCED_TYPENAME base_::logic_right_tag>(core)
       )

   {}

   bimap(const bimap& x) :

       base_::relation_set(
           ::boost::multi_index::get<
               BOOST_DEDUCED_TYPENAME base_::logic_relation_set_tag>(core)
       ),

       core(x.core),

       left (
           ::boost::multi_index::get<
               BOOST_DEDUCED_TYPENAME base_::logic_left_tag>(core)
       ),
       right (
           ::boost::multi_index::get<
               BOOST_DEDUCED_TYPENAME base_::logic_right_tag>(core)
       )

   {}

    bimap& operator=(const bimap& x)
    {
        core = x.core;
        return *this;
    }

    // Projection of iterators

    template< class IteratorType >
    left_iterator project_left(IteratorType iter)
    {
        return core.template project<
            BOOST_DEDUCED_TYPENAME base_::logic_left_tag>(iter.base());
    }

    template< class IteratorType >
    left_const_iterator project_left(IteratorType iter) const
    {
        return core.template project<
            BOOST_DEDUCED_TYPENAME base_::logic_left_tag>(iter.base());
    }

    template< class IteratorType >
    right_iterator project_right(IteratorType iter)
    {
        return core.template project<
            BOOST_DEDUCED_TYPENAME base_::logic_right_tag>(iter.base());
    }

    template< class IteratorType >
    right_const_iterator project_right(IteratorType iter) const
    {
        return core.template project<
            BOOST_DEDUCED_TYPENAME base_::logic_right_tag>(iter.base());
    }

    template< class IteratorType >
    BOOST_DEDUCED_TYPENAME base_::relation_set::iterator
        project_up(IteratorType iter)
    {
        return core.template project<
            BOOST_DEDUCED_TYPENAME base_::logic_relation_set_tag>(iter.base());
    }

    template< class IteratorType >
    BOOST_DEDUCED_TYPENAME base_::relation_set::const_iterator
        project_up(IteratorType iter) const
    {
        return core.template project<
            BOOST_DEDUCED_TYPENAME base_::logic_relation_set_tag>(iter.base());
    }

    // Support for tags

    template< class Tag, class IteratorType >
    BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
    iterator_type_by<Tag,bimap>::type
        project(IteratorType iter)
    {
        return core.template project<Tag>(iter.base());
    }

    template< class Tag, class IteratorType >
    BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
    const_iterator_type_by<Tag,bimap>::type
        project(IteratorType iter) const
    {
        return core.template project<Tag>(iter.base());
    }

    template< class Tag >
    struct map_by :
        public ::boost::bimaps::support::map_type_by<Tag,bimap>::type
    {
        typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
            map_type_by<Tag,bimap>::type type;

        private: map_by() {}
    };

    template< class Tag >
    BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
    map_type_by<Tag,bimap>::type &by()
    {
        return ::boost::bimaps::support::map_by<Tag>(*this);
    }

    template< class Tag >
    const BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
    map_type_by<Tag,bimap>::type &by() const
    {
        return ::boost::bimaps::support::map_by<Tag>(*this);
    }


    #ifndef BOOST_BIMAP_DISABLE_SERIALIZATION

    // Serialization support

    private:

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int)
    {
        ar & serialization::make_nvp("mi_core",core);
    }

    #endif // BOOST_BIMAP_DISABLE_SERIALIZATION
};

} // namespace bimaps
} // namespace boost


/** \namespace boost::bimaps::support
\brief Metafunctions to help working with bimaps.
                                                            **/

/** \namespace boost::bimaps::views
\brief Bimap views.
                                                            **/

/** \namespace boost::bimaps::views::detail
\brief Bimap views details.
                                                            **/



// Include basic tools for user commodity

#include <boost/bimap/tags/tagged.hpp>
#include <boost/bimap/relation/member_at.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

// Bring the most used namespaces directly to the user main namespace
namespace boost {
namespace bimaps {

using ::boost::bimaps::tags::tagged;

namespace member_at = ::boost::bimaps::relation::member_at;

using ::boost::multi_index::unbounded;

} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_BIMAP_HPP
