// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/bimap_core.hpp
/// \brief Bimap base definition.

#ifndef BOOST_BIMAP_DETAIL_BIMAP_CORE_HPP
#define BOOST_BIMAP_DETAIL_BIMAP_CORE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/vector.hpp>

#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/concept_check.hpp>

// Boost.MultiIndex
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>

// Boost.Bimap
#include <boost/bimap/relation/mutant_relation.hpp>
#include <boost/bimap/relation/member_at.hpp>
#include <boost/bimap/relation/support/data_extractor.hpp>
#include <boost/bimap/tags/support/default_tagged.hpp>
#include <boost/bimap/tags/tagged.hpp>
#include <boost/bimap/detail/manage_bimap_key.hpp>
#include <boost/bimap/detail/manage_additional_parameters.hpp>
#include <boost/bimap/detail/map_view_iterator.hpp>
#include <boost/bimap/detail/set_view_iterator.hpp>

#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unconstrained_set_of.hpp>
#include <boost/core/allocator_access.hpp>

namespace boost {
namespace bimaps {

/// \brief Library details

namespace detail {

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template< class Type >
struct get_value_type
{
    typedef BOOST_DEDUCED_TYPENAME Type::value_type type;
};

struct independent_index_tag {};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES


/// \brief Base for the bimap class.
/**


See also bimap.
                                                                        **/


template< class LeftSetType, class RightSetType, class AP1, class AP2, class AP3 >
class bimap_core
{
    // Manage bimap key instantiation
    // --------------------------------------------------------------------
    public:

    typedef BOOST_DEDUCED_TYPENAME manage_bimap_key
    <
        LeftSetType

    >::type left_set_type;

    typedef BOOST_DEDUCED_TYPENAME manage_bimap_key
    <
        RightSetType

    >::type right_set_type;


    private:

    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::tags::support::default_tagged
    <
        BOOST_DEDUCED_TYPENAME left_set_type::user_type,
        ::boost::bimaps::relation::member_at::left

    >::type left_tagged_type;

    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::tags::support::default_tagged
    <
        BOOST_DEDUCED_TYPENAME right_set_type::user_type,
        ::boost::bimaps::relation::member_at::right

    >::type right_tagged_type;

    public:

    //@{

        typedef BOOST_DEDUCED_TYPENAME  left_tagged_type::tag  left_tag;
        typedef BOOST_DEDUCED_TYPENAME right_tagged_type::tag right_tag;

    //@}

    //@{

        typedef BOOST_DEDUCED_TYPENAME  left_set_type::value_type  left_key_type;
        typedef BOOST_DEDUCED_TYPENAME right_set_type::value_type right_key_type;

    //@}

    //@{

        typedef right_key_type  left_data_type;
        typedef  left_key_type right_data_type;

    //@}

    // Manage the additional parameters
    // --------------------------------------------------------------------
    private:

    typedef BOOST_DEDUCED_TYPENAME manage_additional_parameters<AP1,AP2,AP3>::type parameters;

    /// \brief Relation type stored by the bimap.
    // --------------------------------------------------------------------
    public:

    typedef ::boost::bimaps::relation::mutant_relation
    <

        ::boost::bimaps::tags::tagged<
            BOOST_DEDUCED_TYPENAME mpl::if_<
                mpl::and_
                <
                    BOOST_DEDUCED_TYPENAME left_set_type::mutable_key,
                    BOOST_DEDUCED_TYPENAME parameters::set_type_of_relation::left_mutable_key
                >,
            // {
                    left_key_type,
            // }
            // else
            // {
                    BOOST_DEDUCED_TYPENAME ::boost::add_const< left_key_type >::type
            // }

            >::type,
            left_tag
        >,

        ::boost::bimaps::tags::tagged<
            BOOST_DEDUCED_TYPENAME mpl::if_<
                mpl::and_
                <
                    BOOST_DEDUCED_TYPENAME right_set_type::mutable_key,
                    BOOST_DEDUCED_TYPENAME parameters::set_type_of_relation::right_mutable_key
                >,
            // {
                    right_key_type,
            // }
            // else
            // {
                    BOOST_DEDUCED_TYPENAME ::boost::add_const< right_key_type >::type
            // }

            >::type,
            right_tag
        >,

        // It is ::boost::mpl::na if no info_hook was included
        BOOST_DEDUCED_TYPENAME parameters::additional_info,

        // Force mutable keys
        true

    > relation;

    //@{

        typedef BOOST_DEDUCED_TYPENAME relation::left_pair  left_value_type;
        typedef BOOST_DEDUCED_TYPENAME relation::right_pair right_value_type;

    //@}

    // Bind the member of the relation, so multi_index can manage them
    // --------------------------------------------------------------------
    private:

    typedef BOOST_DEDUCED_TYPENAME relation::storage_base relation_storage_base;

    typedef BOOST_MULTI_INDEX_MEMBER(relation_storage_base, left_key_type, left)
        left_member_extractor;

    typedef BOOST_MULTI_INDEX_MEMBER(relation_storage_base,right_key_type,right)
        right_member_extractor;

    // The core indices are somewhat complicated to calculate, because they
    // can be zero, one, two or three indices, depending on the use of
    // {side}_based set type of relations and unconstrained_set_of and
    // unconstrained_set_of_relation specifications.

    typedef BOOST_DEDUCED_TYPENAME mpl::if_<
        ::boost::bimaps::detail::is_unconstrained_set_of< left_set_type >,
    // {
            mpl::vector<>,
    // }
    // else
    // {
            mpl::vector
            <
                BOOST_DEDUCED_TYPENAME left_set_type::
                BOOST_NESTED_TEMPLATE index_bind
                <
                    left_member_extractor,
                    left_tag

                >::type
            >
    // }
    >::type left_core_indices;

    typedef BOOST_DEDUCED_TYPENAME mpl::if_<
        ::boost::bimaps::detail::is_unconstrained_set_of< right_set_type >,
    // {
            left_core_indices,
    // }
    // else
    // {
            BOOST_DEDUCED_TYPENAME mpl::push_front
            <
                left_core_indices,

                BOOST_DEDUCED_TYPENAME right_set_type::
                BOOST_NESTED_TEMPLATE index_bind
                <
                    right_member_extractor,
                    right_tag

                >::type

            >::type
    // }
    >::type basic_core_indices;

    // If it is based either on the left or on the right, then only the side
    // indices are needed. But the set type of the relation can be completely
    // different from the one used for the sides in which case we have to add yet
    // another index to the core.

    // TODO
    // If all the set types are unsconstrained there must be readable compile
    // time error.

    typedef BOOST_DEDUCED_TYPENAME mpl::if_<

        is_same< BOOST_DEDUCED_TYPENAME parameters::set_type_of_relation, left_based >,
    // {
            ::boost::bimaps::tags::tagged< left_set_type, left_tag >,
    // }
    /* else */ BOOST_DEDUCED_TYPENAME mpl::if_<
            is_same< BOOST_DEDUCED_TYPENAME parameters::set_type_of_relation, right_based >,
    // {
            ::boost::bimaps::tags::tagged< right_set_type, right_tag >,
    // }
    // else
    // {
            tags::tagged
            <
                BOOST_DEDUCED_TYPENAME parameters::
                set_type_of_relation::BOOST_NESTED_TEMPLATE bind_to
                <
                    relation

                >::type,
                independent_index_tag
            >
    // }
    >::type
    >::type tagged_set_of_relation_type;

    protected:

    typedef BOOST_DEDUCED_TYPENAME tagged_set_of_relation_type::tag
                        relation_set_tag;

    typedef BOOST_DEDUCED_TYPENAME tagged_set_of_relation_type::value_type
                        relation_set_type_of;

    // Logic tags
    // This is a necessary extra level of indirection to allow unconstrained
    // sets to be plug in the design. The bimap constructors use this logic
    // tags.

    typedef BOOST_DEDUCED_TYPENAME mpl::if_<
        ::boost::bimaps::detail::is_unconstrained_set_of< left_set_type >,

        BOOST_DEDUCED_TYPENAME mpl::if_<
            ::boost::bimaps::detail::is_unconstrained_set_of< right_set_type >,

            independent_index_tag,
            right_tag

        >::type,

        left_tag

    >::type logic_left_tag;

    typedef BOOST_DEDUCED_TYPENAME mpl::if_<
        ::boost::bimaps::detail::is_unconstrained_set_of< right_set_type >,

        BOOST_DEDUCED_TYPENAME mpl::if_< 
            ::boost::bimaps::detail::is_unconstrained_set_of< left_set_type >,

            independent_index_tag,
            left_tag

        >::type,

        right_tag

    >::type logic_right_tag;

    typedef BOOST_DEDUCED_TYPENAME mpl::if_< 
        is_same< relation_set_tag, independent_index_tag >,

        BOOST_DEDUCED_TYPENAME mpl::if_< 
            ::boost::bimaps::detail::
                is_unconstrained_set_of< relation_set_type_of >,

            logic_left_tag,
            independent_index_tag

        >::type,

        BOOST_DEDUCED_TYPENAME mpl::if_<
            is_same< BOOST_DEDUCED_TYPENAME parameters::set_type_of_relation, left_based >,

            logic_left_tag,
            logic_right_tag

        >::type

    >::type logic_relation_set_tag;

    private:

    typedef BOOST_DEDUCED_TYPENAME mpl::if_<
        mpl::and_< is_same< relation_set_tag, independent_index_tag >,
                   mpl::not_<
                       ::boost::bimaps::detail::
                            is_unconstrained_set_of< relation_set_type_of > 
                   >
        >,
    // {
            BOOST_DEDUCED_TYPENAME mpl::push_front
            <
                basic_core_indices,

                BOOST_DEDUCED_TYPENAME relation_set_type_of::
                BOOST_NESTED_TEMPLATE index_bind
                <
                    ::boost::bimaps::relation::support::both_keys_extractor<relation>,
                    independent_index_tag

                >::type

            >::type,
    // }
    // else
    // {
            basic_core_indices
    // }

    >::type complete_core_indices;

    struct core_indices : public complete_core_indices {};

    // Define the core using compute_index_type to translate the
    // set type to an multi-index specification
    // --------------------------------------------------------------------
    public:

    typedef multi_index::multi_index_container
    <
        relation,
        core_indices,
        BOOST_DEDUCED_TYPENAME boost::allocator_rebind<BOOST_DEDUCED_TYPENAME
            parameters::allocator, relation>::type

    > core_type;

    // Core metadata
    // --------------------------------------------------------------------
    public:

    typedef BOOST_DEDUCED_TYPENAME ::boost::multi_index::
        index<core_type, logic_left_tag>::type  left_index;

    typedef BOOST_DEDUCED_TYPENAME ::boost::multi_index::
        index<core_type,logic_right_tag>::type right_index;

    typedef BOOST_DEDUCED_TYPENAME  left_index::iterator        left_core_iterator;
    typedef BOOST_DEDUCED_TYPENAME  left_index::const_iterator  left_core_const_iterator;

    typedef BOOST_DEDUCED_TYPENAME right_index::iterator       right_core_iterator;
    typedef BOOST_DEDUCED_TYPENAME right_index::const_iterator right_core_const_iterator;

    // Relation set view

    typedef BOOST_DEDUCED_TYPENAME ::boost::multi_index::index
    <
        core_type, logic_relation_set_tag

    >::type relation_set_core_index;

    typedef BOOST_DEDUCED_TYPENAME relation_set_type_of::
    BOOST_NESTED_TEMPLATE set_view_bind
    <
        relation_set_core_index

    >::type relation_set;

    public:

    typedef bimap_core bimap_core_;
};

// Two auxiliary metafunctions to compute the map view types
// The map view type can not be computed inside the bimap core because a 
// they need the bimap core to be parsed first.

template< class BimapBaseType >
struct left_map_view_type
{
    typedef BOOST_DEDUCED_TYPENAME BimapBaseType::left_set_type left_set_type;
    typedef BOOST_DEDUCED_TYPENAME
        left_set_type::BOOST_NESTED_TEMPLATE map_view_bind<
            BOOST_DEDUCED_TYPENAME BimapBaseType::left_tag, BimapBaseType
        >::type type;
};

template< class BimapBaseType >
struct right_map_view_type
{
    typedef BOOST_DEDUCED_TYPENAME BimapBaseType::right_set_type right_set_type;
    typedef BOOST_DEDUCED_TYPENAME
        right_set_type::BOOST_NESTED_TEMPLATE map_view_bind<
            BOOST_DEDUCED_TYPENAME BimapBaseType::right_tag, BimapBaseType
        >::type type;
};


} // namespace detail
} // namespace bimaps
} // namespace boost

#endif // BOOST_BIMAP_DETAIL_BIMAP_CORE_HPP
