// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/set_view_base.hpp
/// \brief Helper base for the construction of the bimap views types.

#ifndef BOOST_BIMAP_DETAIL_SET_VIEW_BASE_HPP
#define BOOST_BIMAP_DETAIL_SET_VIEW_BASE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/bimap/relation/member_at.hpp>
#include <boost/bimap/relation/support/data_extractor.hpp>
#include <boost/bimap/detail/modifier_adaptor.hpp>
#include <boost/bimap/detail/set_view_iterator.hpp>
#include <boost/bimap/relation/support/get_pair_functor.hpp>
#include <boost/bimap/relation/detail/to_mutable_relation_functor.hpp>
#include <boost/bimap/relation/mutant_relation.hpp>
#include <boost/bimap/container_adaptor/support/iterator_facade_converters.hpp>

namespace boost {
namespace bimaps {
namespace detail {

template< class Key, class Value, class KeyToBase >
class set_view_key_to_base
{
    public:
    const Key operator()( const Value & v ) const
    {
        return keyToBase( v );
    }
    private:
    KeyToBase keyToBase;
};

template< class MutantRelationStorage, class KeyToBase >
class set_view_key_to_base<MutantRelationStorage,MutantRelationStorage,KeyToBase>
{
    typedef BOOST_DEDUCED_TYPENAME MutantRelationStorage::non_mutable_storage non_mutable_storage;
    public:
    const MutantRelationStorage & operator()( const non_mutable_storage & k ) const
    {
        return ::boost::bimaps::relation::detail::mutate<MutantRelationStorage>(k);
    }
    const MutantRelationStorage & operator()( const MutantRelationStorage & k ) const
    {
        return k;
    }
};


// The next macro can be converted in a metafunctor to gain code robustness.
/*===========================================================================*/
#define BOOST_BIMAP_SET_VIEW_CONTAINER_ADAPTOR(                               \
    CONTAINER_ADAPTOR, CORE_INDEX, OTHER_ITER, CONST_OTHER_ITER               \
)                                                                             \
::boost::bimaps::container_adaptor::CONTAINER_ADAPTOR                         \
<                                                                             \
    CORE_INDEX,                                                               \
    ::boost::bimaps::detail::                                                 \
              set_view_iterator<                                              \
                    BOOST_DEDUCED_TYPENAME CORE_INDEX::iterator         >,    \
    ::boost::bimaps::detail::                                                 \
        const_set_view_iterator<                                              \
                    BOOST_DEDUCED_TYPENAME CORE_INDEX::const_iterator   >,    \
    ::boost::bimaps::detail::                                                 \
              set_view_iterator<                                              \
                    BOOST_DEDUCED_TYPENAME CORE_INDEX::OTHER_ITER       >,    \
    ::boost::bimaps::detail::                                                 \
        const_set_view_iterator<                                              \
                    BOOST_DEDUCED_TYPENAME CORE_INDEX::CONST_OTHER_ITER >,    \
    ::boost::bimaps::container_adaptor::support::iterator_facade_to_base      \
    <                                                                         \
        ::boost::bimaps::detail::      set_view_iterator<                     \
            BOOST_DEDUCED_TYPENAME CORE_INDEX::iterator>,                     \
        ::boost::bimaps::detail::const_set_view_iterator<                     \
            BOOST_DEDUCED_TYPENAME CORE_INDEX::const_iterator>                \
                                                                              \
    >,                                                                        \
    ::boost::mpl::na,                                                         \
    ::boost::mpl::na,                                                         \
    ::boost::bimaps::relation::detail::                                       \
        get_mutable_relation_functor<                                         \
            BOOST_DEDUCED_TYPENAME CORE_INDEX::value_type >,                  \
    ::boost::bimaps::relation::support::                                      \
        get_above_view_functor<                                               \
            BOOST_DEDUCED_TYPENAME CORE_INDEX::value_type >,                  \
    ::boost::bimaps::detail::set_view_key_to_base<                            \
        BOOST_DEDUCED_TYPENAME CORE_INDEX::key_type,                          \
        BOOST_DEDUCED_TYPENAME CORE_INDEX::value_type,                        \
        BOOST_DEDUCED_TYPENAME CORE_INDEX::key_from_value                     \
    >                                                                         \
>
/*===========================================================================*/


/*===========================================================================*/
#define BOOST_BIMAP_SEQUENCED_SET_VIEW_CONTAINER_ADAPTOR(                     \
    CONTAINER_ADAPTOR, CORE_INDEX, OTHER_ITER, CONST_OTHER_ITER               \
)                                                                             \
::boost::bimaps::container_adaptor::CONTAINER_ADAPTOR                         \
<                                                                             \
    CORE_INDEX,                                                               \
    ::boost::bimaps::detail::                                                 \
              set_view_iterator<                                              \
                    BOOST_DEDUCED_TYPENAME CORE_INDEX::iterator         >,    \
    ::boost::bimaps::detail::                                                 \
        const_set_view_iterator<                                              \
                    BOOST_DEDUCED_TYPENAME CORE_INDEX::const_iterator   >,    \
    ::boost::bimaps::detail::                                                 \
              set_view_iterator<                                              \
                    BOOST_DEDUCED_TYPENAME CORE_INDEX::OTHER_ITER       >,    \
    ::boost::bimaps::detail::                                                 \
        const_set_view_iterator<                                              \
                    BOOST_DEDUCED_TYPENAME CORE_INDEX::CONST_OTHER_ITER >,    \
    ::boost::bimaps::container_adaptor::support::iterator_facade_to_base      \
    <                                                                         \
        ::boost::bimaps::detail::      set_view_iterator<                     \
            BOOST_DEDUCED_TYPENAME CORE_INDEX::iterator>,                     \
        ::boost::bimaps::detail::const_set_view_iterator<                     \
            BOOST_DEDUCED_TYPENAME CORE_INDEX::const_iterator>                \
                                                                              \
    >,                                                                        \
    ::boost::mpl::na,                                                         \
    ::boost::mpl::na,                                                         \
    ::boost::bimaps::relation::detail::                                       \
        get_mutable_relation_functor<                                         \
            BOOST_DEDUCED_TYPENAME CORE_INDEX::value_type >,                  \
    ::boost::bimaps::relation::support::                                      \
        get_above_view_functor<                                               \
            BOOST_DEDUCED_TYPENAME CORE_INDEX::value_type >                   \
>
/*===========================================================================*/


#if defined(BOOST_MSVC)
/*===========================================================================*/
#define BOOST_BIMAP_SET_VIEW_BASE_FRIEND(TYPE,INDEX_TYPE)                     \
    typedef ::boost::bimaps::detail::set_view_base<                           \
        TYPE< INDEX_TYPE >, INDEX_TYPE > template_class_friend;               \
    friend class template_class_friend;
/*===========================================================================*/
#else
/*===========================================================================*/
#define BOOST_BIMAP_SET_VIEW_BASE_FRIEND(TYPE,INDEX_TYPE)                     \
    friend class ::boost::bimaps::detail::set_view_base<                      \
        TYPE< INDEX_TYPE >, INDEX_TYPE >;
/*===========================================================================*/
#endif


/// \brief Common base for set views.

template< class Derived, class Index >
class set_view_base
{
    typedef ::boost::bimaps::container_adaptor::support::
    iterator_facade_to_base
    <
        ::boost::bimaps::detail::
                  set_view_iterator<BOOST_DEDUCED_TYPENAME Index::      iterator>,
        ::boost::bimaps::detail::
            const_set_view_iterator<BOOST_DEDUCED_TYPENAME Index::const_iterator>

    > iterator_to_base_;

    typedef BOOST_DEDUCED_TYPENAME Index::value_type::left_value_type          left_type_;

    typedef BOOST_DEDUCED_TYPENAME Index::value_type::right_value_type        right_type_;

    typedef BOOST_DEDUCED_TYPENAME Index::value_type                          value_type_;

    typedef ::boost::bimaps::detail::
                    set_view_iterator<BOOST_DEDUCED_TYPENAME Index::iterator>   iterator_;

    public:

    bool replace(iterator_ position,
                 const value_type_ & x)
    {
        return derived().base().replace(
            derived().template functor<iterator_to_base_>()(position),x
        );
    }

    template< class CompatibleLeftType >
    bool replace_left(iterator_ position,
                      const CompatibleLeftType & l)
    {
        return derived().base().replace(
            derived().template functor<iterator_to_base_>()(position),
            ::boost::bimaps::relation::detail::copy_with_left_replaced(*position,l)
        );
    }

    template< class CompatibleRightType >
    bool replace_right(iterator_ position,
                       const CompatibleRightType & r)
    {
        return derived().base().replace(
            derived().template functor<iterator_to_base_>()(position),
            ::boost::bimaps::relation::detail::copy_with_right_replaced(*position,r)
        );
    }

    /* This function may be provided in the future

    template< class Modifier >
    bool modify(iterator_ position,
                Modifier mod)
    {
        return derived().base().modify(

            derived().template functor<iterator_to_base_>()(position),

            ::boost::bimaps::detail::relation_modifier_adaptor
            <
                Modifier,
                BOOST_DEDUCED_TYPENAME Index::value_type,
                BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
                data_extractor
                <
                    ::boost::bimaps::relation::member_at::left,
                    BOOST_DEDUCED_TYPENAME Index::value_type

                >::type,
                BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
                data_extractor
                <
                    ::boost::bimaps::relation::member_at::right,
                    BOOST_DEDUCED_TYPENAME Index::value_type

                >::type

            >(mod)
        );
    }
    */
    /*
    template< class Modifier >
    bool modify_left(iterator_ position, Modifier mod)
    {
        typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
        data_extractor
        <
            BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::member_at::right,
            BOOST_DEDUCED_TYPENAME Index::value_type

        >::type left_data_extractor_;

        return derived().base().modify(

            derived().template functor<iterator_to_base_>()(position),

            // this may be replaced later by
            // ::boost::bind( mod, ::boost::bind(data_extractor_(),_1) )

            ::boost::bimaps::detail::unary_modifier_adaptor
            <
                Modifier,
                BOOST_DEDUCED_TYPENAME Index::value_type,
                left_data_extractor_

            >(mod)
        );
    }

    template< class Modifier >
    bool modify_right(iterator_ position, Modifier mod)
    {
        typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
        data_extractor
        <
            BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::member_at::right,
            BOOST_DEDUCED_TYPENAME Index::value_type

        >::type right_data_extractor_;

        return derived().base().modify(

            derived().template functor<iterator_to_base_>()(position),

            // this may be replaced later by
            // ::boost::bind( mod, ::boost::bind(data_extractor_(),_1) )

            ::boost::bimaps::detail::unary_modifier_adaptor
            <
                Modifier,
                BOOST_DEDUCED_TYPENAME Index::value_type,
                right_data_extractor_

            >(mod)
        );
    }
    */
    protected:

    typedef set_view_base set_view_base_;

    private:

    // Curiously Recurring Template interface.

    Derived& derived()
    {
        return *static_cast<Derived*>(this);
    }

    Derived const& derived() const
    {
        return *static_cast<Derived const*>(this);
    }
};



} // namespace detail
} // namespace bimaps
} // namespace boost

#endif // BOOST_BIMAP_DETAIL_SET_VIEW_BASE_HPP
