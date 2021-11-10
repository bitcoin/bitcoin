// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2013-2020.
// Modifications copyright (c) 2013-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_SUB_RANGE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_SUB_RANGE_HPP

#include <type_traits>

#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/range.hpp>

namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace detail_dispatch {

template <typename Geometry,
          typename Tag = typename geometry::tag<Geometry>::type,
          bool IsMulti = std::is_base_of<multi_tag, Tag>::value>
struct sub_range : not_implemented<Tag>
{};

template <typename Geometry, typename Tag>
struct sub_range<Geometry, Tag, false>
{
    typedef Geometry & return_type;

    template <typename Id> static inline
    return_type apply(Geometry & geometry, Id const&)
    {
        return geometry;
    }
};

template <typename Geometry>
struct sub_range<Geometry, polygon_tag, false>
{
    typedef typename geometry::ring_return_type<Geometry>::type return_type;

    template <typename Id> static inline
    return_type apply(Geometry & geometry, Id const& id)
    {
        if ( id.ring_index < 0 )
        {
            return geometry::exterior_ring(geometry);
        }
        else
        {
            typedef typename boost::range_size
                <
                    typename geometry::interior_type<Geometry>::type
                >::type size_type;
            size_type const ri = static_cast<size_type>(id.ring_index);
            return range::at(geometry::interior_rings(geometry), ri);
        }
    }
};

template <typename Geometry, typename Tag>
struct sub_range<Geometry, Tag, true>
{
    typedef typename boost::range_value<Geometry>::type value_type;
    typedef std::conditional_t
        <
            std::is_const<Geometry>::value,
            typename std::add_const<value_type>::type,
            value_type
        > sub_type;

    typedef detail_dispatch::sub_range<sub_type> sub_sub_range;

    // TODO: shouldn't it be return_type?
    typedef typename sub_sub_range::return_type return_type;

    template <typename Id> static inline
    return_type apply(Geometry & geometry, Id const& id)
    {
        BOOST_GEOMETRY_ASSERT(0 <= id.multi_index);
        typedef typename boost::range_size<Geometry>::type size_type;
        size_type const mi = static_cast<size_type>(id.multi_index);
        return sub_sub_range::apply(range::at(geometry, mi), id);
    }
};

} // namespace detail_dispatch
#endif // DOXYGEN_NO_DISPATCH

namespace detail {

template <typename Geometry>
struct sub_range_return_type
{
    typedef typename detail_dispatch::sub_range<Geometry>::return_type type;
};

// This function also works for geometry::segment_identifier

template <typename Geometry, typename Id> inline
typename sub_range_return_type<Geometry>::type
sub_range(Geometry & geometry, Id const& id)
{
    return detail_dispatch::sub_range<Geometry>::apply(geometry, id);
}

template <typename Geometry, typename Id> inline
typename sub_range_return_type<Geometry const>::type
sub_range(Geometry const& geometry, Id const& id)
{
    return detail_dispatch::sub_range<Geometry const>::apply(geometry, id);
}

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_SUB_RANGE_HPP
