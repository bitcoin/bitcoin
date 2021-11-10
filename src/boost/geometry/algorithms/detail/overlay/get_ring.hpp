// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_RING_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_RING_HPP


#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/algorithms/detail/ring_identifier.hpp>
#include <boost/geometry/algorithms/detail/overlay/segment_identifier.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/util/range.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


template<typename Tag>
struct get_ring
{};

// A range of rings (multi-ring but that does not exist)
// gets the "void" tag and is dispatched here.
template<>
struct get_ring<void>
{
    template<typename Range>
    static inline typename boost::range_value<Range>::type const&
                apply(ring_identifier const& id, Range const& container)
    {
        return range::at(container, id.multi_index);
    }
};


template<>
struct get_ring<ring_tag>
{
    template<typename Ring>
    static inline Ring const& apply(ring_identifier const& , Ring const& ring)
    {
        return ring;
    }
};


template<>
struct get_ring<box_tag>
{
    template<typename Box>
    static inline Box const& apply(ring_identifier const& ,
                    Box const& box)
    {
        return box;
    }
};


template<>
struct get_ring<polygon_tag>
{
    template<typename Polygon>
    static inline typename ring_return_type<Polygon const>::type const apply(
                ring_identifier const& id,
                Polygon const& polygon)
    {
        BOOST_GEOMETRY_ASSERT
            (
                id.ring_index >= -1
                && id.ring_index < int(boost::size(interior_rings(polygon)))
            );
        return id.ring_index < 0
            ? exterior_ring(polygon)
            : range::at(interior_rings(polygon), id.ring_index);
    }
};


template<>
struct get_ring<multi_polygon_tag>
{
    template<typename MultiPolygon>
    static inline typename ring_type<MultiPolygon>::type const& apply(
                ring_identifier const& id,
                MultiPolygon const& multi_polygon)
    {
        BOOST_GEOMETRY_ASSERT
            (
                id.multi_index >= 0
                && id.multi_index < int(boost::size(multi_polygon))
            );
        return get_ring<polygon_tag>::apply(id,
                    range::at(multi_polygon, id.multi_index));
    }
};

// Returns the number of segments on a ring (regardless whether the ring is open or closed)
template <typename Geometry>
inline signed_size_type segment_count_on_ring(Geometry const& geometry,
                                              ring_identifier const& ring_id)
{
    using tag = typename geometry::tag<Geometry>::type;

    // A closed polygon, a triangle of 4 points, including starting point,
    // contains 3 segments. So handle as if it is closed, and subtract one.
    return geometry::num_points(detail::overlay::get_ring<tag>::apply(ring_id, geometry), true) - 1;
}

// Returns the number of segments on a ring (regardless whether the ring is open or closed)
template <typename Geometry>
inline signed_size_type segment_count_on_ring(Geometry const& geometry,
                                              segment_identifier const& seg_id)
{
    return segment_count_on_ring(geometry, ring_identifier(0, seg_id.multi_index, seg_id.ring_index));
}


// Returns the distance between the second and the first segment identifier (second-first)
// It supports circular behavior and for this it is necessary to pass the geometry.
// It will not report negative values
template <typename Geometry>
inline signed_size_type segment_distance(Geometry const& geometry,
        segment_identifier const& first, segment_identifier const& second)
{
    // It is an internal function, make sure the preconditions are met
    BOOST_ASSERT(second.source_index == first.source_index);
    BOOST_ASSERT(second.multi_index == first.multi_index);
    BOOST_ASSERT(second.ring_index == first.ring_index);

    signed_size_type const result = second.segment_index - first.segment_index;
    if (second.segment_index >= first.segment_index)
    {
        return result;
    }
    // Take wrap into account, counting segments on the ring (passing any of the ids is fine).
    // Suppose point_count=10 (10 points, 9 segments), first.seg_id=7, second.seg_id=2,
    // then distance=9-7+2=4, being segments 7,8,0,1
    return segment_count_on_ring(geometry, first) + result;
}

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_RING_HPP
