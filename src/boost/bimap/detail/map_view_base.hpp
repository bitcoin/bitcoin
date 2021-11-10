// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/map_view_base.hpp
/// \brief Helper base for the construction of the bimap views types.

#ifndef BOOST_BIMAP_DETAIL_MAP_VIEW_BASE_HPP
#define BOOST_BIMAP_DETAIL_MAP_VIEW_BASE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <stdexcept>
#include <utility>

#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/mpl/if.hpp>

#include <boost/bimap/relation/support/get_pair_functor.hpp>
#include <boost/bimap/relation/detail/to_mutable_relation_functor.hpp>
#include <boost/bimap/container_adaptor/support/iterator_facade_converters.hpp>
#include <boost/bimap/relation/support/data_extractor.hpp>
#include <boost/bimap/relation/support/opposite_tag.hpp>
#include <boost/bimap/relation/support/pair_type_by.hpp>
//#include <boost/bimap/support/iterator_type_by.hpp>
#include <boost/bimap/support/key_type_by.hpp>
#include <boost/bimap/support/data_type_by.hpp>
#include <boost/bimap/support/value_type_by.hpp>
#include <boost/bimap/detail/modifier_adaptor.hpp>
#include <boost/bimap/detail/debug/static_error.hpp>
#include <boost/bimap/detail/map_view_iterator.hpp>

namespace boost {
namespace bimaps {

namespace detail {


// The next macro can be converted in a metafunctor to gain code robustness.
/*===========================================================================*/
#define BOOST_BIMAP_MAP_VIEW_CONTAINER_ADAPTOR(                               \
    CONTAINER_ADAPTOR, TAG, BIMAP, OTHER_ITER, CONST_OTHER_ITER               \
)                                                                             \
::boost::bimaps::container_adaptor::CONTAINER_ADAPTOR                         \
<                                                                             \
    BOOST_DEDUCED_TYPENAME BIMAP::core_type::                                 \
        BOOST_NESTED_TEMPLATE index<TAG>::type,                               \
    ::boost::bimaps::detail::      map_view_iterator<TAG,BIMAP>,              \
    ::boost::bimaps::detail::const_map_view_iterator<TAG,BIMAP>,              \
    ::boost::bimaps::detail::      OTHER_ITER<TAG,BIMAP>,                     \
    ::boost::bimaps::detail::CONST_OTHER_ITER<TAG,BIMAP>,                     \
    ::boost::bimaps::container_adaptor::support::iterator_facade_to_base      \
    <                                                                         \
        ::boost::bimaps::detail::      map_view_iterator<TAG,BIMAP>,          \
        ::boost::bimaps::detail::const_map_view_iterator<TAG,BIMAP>           \
    >,                                                                        \
    ::boost::mpl::na,                                                         \
    ::boost::mpl::na,                                                         \
    ::boost::bimaps::relation::detail::                                       \
        pair_to_relation_functor<TAG,BOOST_DEDUCED_TYPENAME BIMAP::relation>, \
    ::boost::bimaps::relation::support::                                      \
        get_pair_functor<TAG, BOOST_DEDUCED_TYPENAME BIMAP::relation >        \
>
/*===========================================================================*/


#if defined(BOOST_MSVC)
/*===========================================================================*/
#define BOOST_BIMAP_MAP_VIEW_BASE_FRIEND(TYPE,TAG,BIMAP)                      \
    typedef ::boost::bimaps::detail::map_view_base<                           \
        TYPE<TAG,BIMAP>,TAG,BIMAP > friend_map_view_base;                     \
    friend class friend_map_view_base;
/*===========================================================================*/
#else
/*===========================================================================*/
#define BOOST_BIMAP_MAP_VIEW_BASE_FRIEND(TYPE,TAG,BIMAP)                      \
    friend class ::boost::bimaps::detail::map_view_base<                      \
        TYPE<TAG,BIMAP>,TAG,BIMAP >;
/*===========================================================================*/
#endif


/// \brief Common base for map views.

template< class Derived, class Tag, class BimapType>
class map_view_base
{
    typedef ::boost::bimaps::container_adaptor::support::
        iterator_facade_to_base<
            ::boost::bimaps::detail::      map_view_iterator<Tag,BimapType>,
            ::boost::bimaps::detail::const_map_view_iterator<Tag,BimapType>
        > iterator_to_base_;

    typedef ::boost::bimaps::relation::detail::
        pair_to_relation_functor<Tag,
            BOOST_DEDUCED_TYPENAME BimapType::relation>      value_to_base_;

    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
                           key_type_by<Tag,BimapType>::type       key_type_;

    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
                          data_type_by<Tag,BimapType>::type      data_type_;

    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
           pair_type_by<Tag,
              BOOST_DEDUCED_TYPENAME BimapType::relation>::type value_type_;

    typedef 
        ::boost::bimaps::detail::map_view_iterator<Tag,BimapType> iterator_;

    public:

    bool replace(iterator_ position, const value_type_ & x)
    {
        return derived().base().replace(
            derived().template functor<iterator_to_base_>()(position),
            derived().template functor<value_to_base_>()(x)
        );
    }

    template< class CompatibleKey >
    bool replace_key(iterator_ position, const CompatibleKey & k)
    {
        return derived().base().replace(
            derived().template functor<iterator_to_base_>()(position),
            derived().template functor<value_to_base_>()(
                ::boost::bimaps::relation::detail::
                    copy_with_first_replaced(*position,k)
            )
        );
    }

    template< class CompatibleData >
    bool replace_data(iterator_ position, const CompatibleData & d)
    {
        return derived().base().replace(
            derived().template functor<iterator_to_base_>()(position),
            derived().template functor<value_to_base_>()(
                ::boost::bimaps::relation::detail::
                    copy_with_second_replaced(*position,d)
            )
        );
    }

    /* This function may be provided in the future

    template< class Modifier >
    bool modify(iterator_ position, Modifier mod)
    {
        return derived().base().modify(

            derived().template functor<iterator_to_base_>()(position),

            ::boost::bimaps::detail::relation_modifier_adaptor
            <
                Modifier,
                BOOST_DEDUCED_TYPENAME BimapType::relation,
                BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
                data_extractor
                <
                    Tag, BOOST_DEDUCED_TYPENAME BimapType::relation

                >::type,
                BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
                data_extractor
                <
                    BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
                        opossite_tag<Tag,BimapType>::type,
                    BOOST_DEDUCED_TYPENAME BimapType::relation

                >::type

            >(mod)
        );
    }
    */

    template< class Modifier >
    bool modify_key(iterator_ position, Modifier mod)
    {
        return derived().base().modify_key(
            derived().template functor<iterator_to_base_>()(position), mod
        );
    }

    template< class Modifier >
    bool modify_data(iterator_ position, Modifier mod)
    {
        typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
        data_extractor
        <
            BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
                        opossite_tag<Tag,BimapType>::type,
            BOOST_DEDUCED_TYPENAME BimapType::relation

        >::type data_extractor_;

        return derived().base().modify(

            derived().template functor<iterator_to_base_>()(position),

            // this may be replaced later by
            // ::boost::bind( mod, ::boost::bind(data_extractor_(),_1) )

            ::boost::bimaps::detail::unary_modifier_adaptor
            <
                Modifier,
                BOOST_DEDUCED_TYPENAME BimapType::relation,
                data_extractor_

            >(mod)
        );
    }

    protected:

    typedef map_view_base map_view_base_;

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




template< class Derived, class Tag, class BimapType>
class mutable_data_unique_map_view_access
{
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
                          data_type_by<Tag,BimapType>::type      data_type_;

    public:

    template< class CompatibleKey >
    data_type_ & at(const CompatibleKey& k)
    {
        typedef ::boost::bimaps::detail::
            map_view_iterator<Tag,BimapType> iterator;

        iterator iter = derived().find(k);
        if( iter == derived().end() )
        {
            ::boost::throw_exception(
                std::out_of_range("bimap<>: invalid key")
            );
        }
        return iter->second;
    }

    template< class CompatibleKey >
    const data_type_ & at(const CompatibleKey& k) const
    {
        typedef ::boost::bimaps::detail::
                const_map_view_iterator<Tag,BimapType> const_iterator;

        const_iterator iter = derived().find(k);
        if( iter == derived().end() )
        {
            ::boost::throw_exception(
                std::out_of_range("bimap<>: invalid key")
            );
        }
        return iter->second;
    }

    template< class CompatibleKey >
    data_type_ & operator[](const CompatibleKey& k)
    {
        typedef ::boost::bimaps::detail::
                      map_view_iterator<Tag,BimapType>          iterator;

        typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
                         value_type_by<Tag,BimapType>::type     value_type;

        iterator iter = derived().find(k);
        if( iter == derived().end() )
        {
            iter = derived().insert( value_type(k,data_type_()) ).first;
        }
        return iter->second;
    }

    protected:

    typedef mutable_data_unique_map_view_access
                mutable_data_unique_map_view_access_;

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


template< class Derived, class Tag, class BimapType>
class non_mutable_data_unique_map_view_access
{
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
                          data_type_by<Tag,BimapType>::type      data_type_;

    public:

    template< class CompatibleKey >
    const data_type_ & at(const CompatibleKey& k) const
    {
        typedef ::boost::bimaps::detail::
                const_map_view_iterator<Tag,BimapType> const_iterator;

        const_iterator iter = derived().find(k);
        if( iter == derived().end() )
        {
            ::boost::throw_exception(
                std::out_of_range("bimap<>: invalid key")
            );
        }
        return iter->second;
    }

    template< class CompatibleKey >
    data_type_ & operator[](const CompatibleKey&)
    {
        BOOST_BIMAP_STATIC_ERROR( OPERATOR_BRACKET_IS_NOT_SUPPORTED, (Derived));
    }

    protected:

    typedef non_mutable_data_unique_map_view_access
                non_mutable_data_unique_map_view_access_;

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


template< class Derived, class Tag, class BimapType>
struct unique_map_view_access
{
    private:
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::support::
        value_type_by<Tag,BimapType>::type value_type;

    public:
    typedef BOOST_DEDUCED_TYPENAME ::boost::mpl::if_
    <
        typename ::boost::is_const<
            BOOST_DEDUCED_TYPENAME value_type::second_type >::type,

        non_mutable_data_unique_map_view_access<Derived,Tag,BimapType>,
        mutable_data_unique_map_view_access<Derived,Tag,BimapType>

    >::type type;
};

// Map views specialize the following structs to provide to the bimap class
// the extra side typedefs (i.e. left_local_iterator for unordered_maps, 
// right_range_type for maps)

template< class MapView >
struct  left_map_view_extra_typedefs {};

template< class MapView >
struct right_map_view_extra_typedefs {};

} // namespace detail

// This function is already part of Boost.Lambda.
// They may be moved to Boost.Utility.

template <class T> inline const T&  make_const(const T& t) { return t; }

} // namespace bimaps
} // namespace boost


// The following macros avoids code duplication in map views
// Maybe this can be changed in the future using a scheme similar to
// the one used with map_view_base.

/*===========================================================================*/
#define BOOST_BIMAP_MAP_VIEW_RANGE_IMPLEMENTATION(BASE)                       \
                                                                              \
typedef std::pair<                                                            \
    BOOST_DEDUCED_TYPENAME base_::iterator,                                   \
    BOOST_DEDUCED_TYPENAME base_::iterator> range_type;                       \
                                                                              \
typedef std::pair<                                                            \
    BOOST_DEDUCED_TYPENAME base_::const_iterator,                             \
    BOOST_DEDUCED_TYPENAME base_::const_iterator> const_range_type;           \
                                                                              \
                                                                              \
template< class LowerBounder, class UpperBounder>                             \
range_type range(LowerBounder lower,UpperBounder upper)                       \
{                                                                             \
    std::pair<                                                                \
                                                                              \
        BOOST_DEDUCED_TYPENAME BASE::base_type::iterator,                     \
        BOOST_DEDUCED_TYPENAME BASE::base_type::iterator                      \
                                                                              \
    > r( this->base().range(lower,upper) );                                   \
                                                                              \
    return range_type(                                                        \
        this->template functor<                                               \
            BOOST_DEDUCED_TYPENAME BASE::iterator_from_base                   \
        >()                                         ( r.first ),              \
        this->template functor<                                               \
            BOOST_DEDUCED_TYPENAME BASE::iterator_from_base                   \
        >()                                         ( r.second )              \
    );                                                                        \
}                                                                             \
                                                                              \
template< class LowerBounder, class UpperBounder>                             \
const_range_type range(LowerBounder lower,UpperBounder upper) const           \
{                                                                             \
    std::pair<                                                                \
                                                                              \
        BOOST_DEDUCED_TYPENAME BASE::base_type::const_iterator,               \
        BOOST_DEDUCED_TYPENAME BASE::base_type::const_iterator                \
                                                                              \
    > r( this->base().range(lower,upper) );                                   \
                                                                              \
    return const_range_type(                                                  \
        this->template functor<                                               \
            BOOST_DEDUCED_TYPENAME BASE::iterator_from_base                   \
        >()                                         ( r.first ),              \
        this->template functor<                                               \
            BOOST_DEDUCED_TYPENAME BASE::iterator_from_base                   \
        >()                                         ( r.second )              \
    );                                                                        \
}
/*===========================================================================*/


/*===========================================================================*/
#define BOOST_BIMAP_VIEW_ASSIGN_IMPLEMENTATION(BASE)                          \
                                                                              \
template< class InputIterator >                                               \
void assign(InputIterator first,InputIterator last)                           \
{                                                                             \
    this->clear();                                                            \
    this->insert(this->end(),first,last);                                     \
}                                                                             \
                                                                              \
void assign(BOOST_DEDUCED_TYPENAME BASE::size_type n,                         \
            const BOOST_DEDUCED_TYPENAME BASE::value_type& v)                 \
{                                                                             \
    this->clear();                                                            \
    for(BOOST_DEDUCED_TYPENAME BASE::size_type i = 0 ; i < n ; ++i)           \
    {                                                                         \
        this->push_back(v);                                                   \
    }                                                                         \
}
/*===========================================================================*/


/*===========================================================================*/
#define BOOST_BIMAP_VIEW_FRONT_BACK_IMPLEMENTATION(BASE)                      \
                                                                              \
BOOST_DEDUCED_TYPENAME BASE::reference front()                                \
{                                                                             \
    return this->template functor<                                            \
        BOOST_DEDUCED_TYPENAME base_::value_from_base>()                      \
    (                                                                         \
        const_cast                                                            \
        <                                                                     \
            BOOST_DEDUCED_TYPENAME BASE::base_type::value_type &              \
                                                                              \
        > ( this->base().front() )                                            \
    );                                                                        \
}                                                                             \
                                                                              \
BOOST_DEDUCED_TYPENAME BASE::reference back()                                 \
{                                                                             \
    return this->template functor<                                            \
        BOOST_DEDUCED_TYPENAME base_::value_from_base>()                      \
    (                                                                         \
        const_cast                                                            \
        <                                                                     \
            BOOST_DEDUCED_TYPENAME BASE::base_type::value_type &              \
                                                                              \
        >( this->base().back() )                                              \
    );                                                                        \
}                                                                             \
                                                                              \
BOOST_DEDUCED_TYPENAME BASE::const_reference front() const                    \
{                                                                             \
    return this->template functor<                                            \
        BOOST_DEDUCED_TYPENAME BASE::value_from_base>()                       \
    (                                                                         \
        this->base().front()                                                  \
    );                                                                        \
}                                                                             \
                                                                              \
BOOST_DEDUCED_TYPENAME BASE::const_reference back() const                     \
{                                                                             \
    return this->template functor<                                            \
        BOOST_DEDUCED_TYPENAME BASE::value_from_base>()                       \
    (                                                                         \
        this->base().back()                                                   \
    );                                                                        \
}
/*===========================================================================*/


#endif // BOOST_BIMAP_DETAIL_MAP_VIEW_BASE_HPP
