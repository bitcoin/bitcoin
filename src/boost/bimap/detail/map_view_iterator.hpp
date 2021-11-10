// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/map_view_iterator.hpp
/// \brief Iterator adaptors from multi-index to bimap.

#ifndef BOOST_BIMAP_DETAIL_MAP_VIEW_ITERATOR_HPP
#define BOOST_BIMAP_DETAIL_MAP_VIEW_ITERATOR_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

// Boost

#ifndef BOOST_BIMAP_DISABLE_SERIALIZATION
  #include <boost/serialization/nvp.hpp>
  #include <boost/serialization/split_member.hpp>
#endif // BOOST_BIMAP_DISABLE_SERIALIZATION

#include <boost/iterator/detail/enable_if.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/bimap/relation/support/pair_by.hpp>

// check
#include <boost/bimap/relation/detail/metadata_access_builder.hpp>
#include <boost/bimap/relation/detail/static_access_builder.hpp>

namespace boost {
namespace bimaps {
namespace detail {

/** \brief Map View Iterator adaptors from multi index to bimap.

These classes are based on transform iterators from Boost.Iterator.
                                                                         **/

template< class Tag, class BimapCore > struct       map_view_iterator ;
template< class Tag, class BimapCore > struct const_map_view_iterator ;

template< class Tag, class BimapCore > struct       reverse_map_view_iterator ;
template< class Tag, class BimapCore > struct const_reverse_map_view_iterator ;

template< class Tag, class BimapCore > struct       local_map_view_iterator ;
template< class Tag, class BimapCore > struct const_local_map_view_iterator ;


#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

#ifndef BOOST_BIMAP_DISABLE_SERIALIZATION
/*===========================================================================*/
#define BOOST_BIMAP_MAP_VIEW_ITERATOR_SERIALIZATION_SUPPORT                   \
    BOOST_SERIALIZATION_SPLIT_MEMBER()                                        \
                                                                              \
    friend class ::boost::serialization::access;                              \
                                                                              \
    template< class Archive >                                                 \
    void save(Archive & ar, const unsigned int) const                         \
    {                                                                         \
        ar << ::boost::serialization::make_nvp("mi_iterator",this->base());   \
    }                                                                         \
                                                                              \
    template< class Archive >                                                 \
    void load(Archive & ar, const unsigned int)                               \
    {                                                                         \
        BOOST_DEDUCED_TYPENAME base_::base_type iter;                         \
        ar >> ::boost::serialization::make_nvp("mi_iterator",iter);           \
        this->base_reference() = iter;                                        \
    }
/*===========================================================================*/
#else
#define BOOST_BIMAP_MAP_VIEW_ITERATOR_SERIALIZATION_SUPPORT // None
#endif // BOOST_BIMAP_DISABLE_SERIALIZATION

/*===========================================================================*/
#define BOOST_BIMAP_CORE_ITERATOR_TYPE_BY_BUILDER( METANAME, ITERATOR )       \
BOOST_BIMAP_SYMMETRIC_STATIC_ACCESS_BUILDER( METANAME, BimapCore,             \
    typedef BOOST_DEDUCED_TYPENAME BimapCore::core_type::BOOST_NESTED_TEMPLATE\
        index<BOOST_DEDUCED_TYPENAME BimapCore::left_tag>                     \
            ::type::ITERATOR type,                                            \
    typedef BOOST_DEDUCED_TYPENAME BimapCore::core_type::BOOST_NESTED_TEMPLATE\
        index<BOOST_DEDUCED_TYPENAME BimapCore::right_tag>                    \
            ::type::ITERATOR type                                             \
)
/*===========================================================================*/


BOOST_BIMAP_CORE_ITERATOR_TYPE_BY_BUILDER( core_iterator_type_by
                                         , iterator )

BOOST_BIMAP_CORE_ITERATOR_TYPE_BY_BUILDER( reverse_core_iterator_type_by
                                         , reverse_iterator )

BOOST_BIMAP_CORE_ITERATOR_TYPE_BY_BUILDER( local_core_iterator_type_by
                                         , local_iterator )


// map_view_iterator

template< class Tag, class BimapCore >
struct map_view_iterator_adaptor {
    typedef iterator_adaptor<
        map_view_iterator<Tag,BimapCore>,
        BOOST_DEDUCED_TYPENAME core_iterator_type_by<Tag,BimapCore>::type,
        BOOST_DEDUCED_TYPENAME 
            ::boost::bimaps::support::value_type_by<Tag,BimapCore>::type
    > type;
};
template< class Tag, class BimapCore >
struct map_view_iterator : 
    public map_view_iterator_adaptor<Tag,BimapCore>::type
{
    typedef BOOST_DEDUCED_TYPENAME 
        map_view_iterator_adaptor<Tag,BimapCore>::type base_;
    public:

    map_view_iterator() {}
    map_view_iterator(BOOST_DEDUCED_TYPENAME base_::base_type const& iter)
      : base_(iter) {}

    BOOST_DEDUCED_TYPENAME base_::reference dereference() const
    {
        return ::boost::bimaps::relation::support::pair_by<Tag>(
            *const_cast<BOOST_DEDUCED_TYPENAME base_::base_type::value_type*>(
                &(*this->base())
            )
        );
    }
    private:
    friend class iterator_core_access;
    BOOST_BIMAP_MAP_VIEW_ITERATOR_SERIALIZATION_SUPPORT
};


template< class Tag, class BimapCore >
struct const_map_view_iterator_adaptor {
    typedef iterator_adaptor<
        const_map_view_iterator<Tag,BimapCore>,
        BOOST_DEDUCED_TYPENAME core_iterator_type_by<Tag,BimapCore>::type,
        const BOOST_DEDUCED_TYPENAME 
            ::boost::bimaps::support::value_type_by<Tag,BimapCore>::type
    > type;
};
template< class Tag, class BimapCore >
struct const_map_view_iterator : 
    public const_map_view_iterator_adaptor<Tag,BimapCore>::type
{
    typedef BOOST_DEDUCED_TYPENAME 
        const_map_view_iterator_adaptor<Tag,BimapCore>::type base_;
    public:

    const_map_view_iterator() {}
    const_map_view_iterator(
            BOOST_DEDUCED_TYPENAME base_::base_type const& iter)
      : base_(iter) {}
	const_map_view_iterator(map_view_iterator<Tag,BimapCore> i)
      : base_(i.base()) {}

    BOOST_DEDUCED_TYPENAME base_::reference dereference() const
    {
        return ::boost::bimaps::relation::support::pair_by<Tag>(*this->base());
    }
    private:
    friend class iterator_core_access;
    BOOST_BIMAP_MAP_VIEW_ITERATOR_SERIALIZATION_SUPPORT
};


// reverse_map_view_iterator

template< class Tag, class BimapCore >
struct reverse_map_view_iterator_adaptor {
    typedef iterator_adaptor<
        reverse_map_view_iterator<Tag,BimapCore>,
        BOOST_DEDUCED_TYPENAME 
            reverse_core_iterator_type_by<Tag,BimapCore>::type,
        BOOST_DEDUCED_TYPENAME 
            ::boost::bimaps::support::value_type_by<Tag,BimapCore>::type
    > type;
};
template< class Tag, class BimapCore >
struct reverse_map_view_iterator :
    public reverse_map_view_iterator_adaptor<Tag,BimapCore>::type
{
    typedef BOOST_DEDUCED_TYPENAME 
        reverse_map_view_iterator_adaptor<Tag,BimapCore>::type base_;
    public:

    reverse_map_view_iterator() {}
    reverse_map_view_iterator(
            BOOST_DEDUCED_TYPENAME base_::base_type const& iter)
      : base_(iter) {}

    BOOST_DEDUCED_TYPENAME base_::reference dereference() const
    {
        return ::boost::bimaps::relation::support::pair_by<Tag>(
            *const_cast<BOOST_DEDUCED_TYPENAME base_::base_type::value_type*>(
                &(*this->base())
            )
        );
    }
    private:
    friend class iterator_core_access;
    BOOST_BIMAP_MAP_VIEW_ITERATOR_SERIALIZATION_SUPPORT
};

template< class Tag, class BimapCore >
struct const_reverse_map_view_iterator_adaptor {
    typedef iterator_adaptor<
        const_reverse_map_view_iterator<Tag,BimapCore>,
        BOOST_DEDUCED_TYPENAME 
            reverse_core_iterator_type_by<Tag,BimapCore>::type,
        const BOOST_DEDUCED_TYPENAME 
            ::boost::bimaps::support::value_type_by<Tag,BimapCore>::type
    > type;
};
template< class Tag, class BimapCore >
struct const_reverse_map_view_iterator :
    public const_reverse_map_view_iterator_adaptor<Tag,BimapCore>::type
{
    typedef BOOST_DEDUCED_TYPENAME 
        const_reverse_map_view_iterator_adaptor<Tag,BimapCore>::type base_;
    
    public:

    const_reverse_map_view_iterator() {}
    const_reverse_map_view_iterator(
            BOOST_DEDUCED_TYPENAME base_::base_type const& iter)
      : base_(iter) {}
	const_reverse_map_view_iterator(reverse_map_view_iterator<Tag,BimapCore> i)
      : base_(i.base()) {}

    BOOST_DEDUCED_TYPENAME base_::reference dereference() const
    {
        return ::boost::bimaps::relation::support::pair_by<Tag>(*this->base());
    }
    private:
    friend class iterator_core_access;
    BOOST_BIMAP_MAP_VIEW_ITERATOR_SERIALIZATION_SUPPORT
};


// local_map_view_iterator

template< class Tag, class BimapCore >
struct local_map_view_iterator_adaptor {
    typedef iterator_adaptor<
        local_map_view_iterator<Tag,BimapCore>,
        BOOST_DEDUCED_TYPENAME 
            local_core_iterator_type_by<Tag,BimapCore>::type,
        BOOST_DEDUCED_TYPENAME 
            ::boost::bimaps::support::value_type_by<Tag,BimapCore>::type
    > type;
};
template< class Tag, class BimapCore >
struct local_map_view_iterator :
    public local_map_view_iterator_adaptor<Tag,BimapCore>::type
{
    typedef BOOST_DEDUCED_TYPENAME 
        local_map_view_iterator_adaptor<Tag,BimapCore>::type base_;
    public:

    local_map_view_iterator() {}
    local_map_view_iterator(
            BOOST_DEDUCED_TYPENAME base_::base_type const& iter)
      : base_(iter) {}

    BOOST_DEDUCED_TYPENAME base_::reference dereference() const
    {
        return ::boost::bimaps::relation::support::pair_by<Tag>(
            *const_cast<BOOST_DEDUCED_TYPENAME base_::base_type::value_type*>(
                &(*this->base())
            )
        );
    }
    private:
    friend class iterator_core_access;
    BOOST_BIMAP_MAP_VIEW_ITERATOR_SERIALIZATION_SUPPORT
};

template< class Tag, class BimapCore >
struct const_local_map_view_iterator_adaptor {
    typedef iterator_adaptor<
        const_local_map_view_iterator<Tag,BimapCore>,
        BOOST_DEDUCED_TYPENAME 
            local_core_iterator_type_by<Tag,BimapCore>::type,
        const BOOST_DEDUCED_TYPENAME 
            ::boost::bimaps::support::value_type_by<Tag,BimapCore>::type
    > type;
};
template< class Tag, class BimapCore >
struct const_local_map_view_iterator :
    public const_local_map_view_iterator_adaptor<Tag,BimapCore>::type
{
    typedef BOOST_DEDUCED_TYPENAME 
        const_local_map_view_iterator_adaptor<Tag,BimapCore>::type base_;
    public:

    const_local_map_view_iterator() {}
    const_local_map_view_iterator(
            BOOST_DEDUCED_TYPENAME base_::base_type const& iter)
      : base_(iter) {}
	const_local_map_view_iterator(local_map_view_iterator<Tag,BimapCore> i)
      : base_(i.base()) {}

    BOOST_DEDUCED_TYPENAME base_::reference dereference() const
    {
        return ::boost::bimaps::relation::support::pair_by<Tag>(*this->base());
    }
    private:
    friend class iterator_core_access;
    BOOST_BIMAP_MAP_VIEW_ITERATOR_SERIALIZATION_SUPPORT
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

} // namespace detail
} // namespace bimaps
} // namespace boost

#endif // BOOST_BIMAP_DETAIL_MAP_VIEW_ITERATOR_HPP


