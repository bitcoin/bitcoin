// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2013-2021.
// Modifications copyright (c) 2013-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_IMPLEMENTATION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_IMPLEMENTATION_HPP

#include <cstddef>
#include <deque>

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/do_reverse.hpp>
#include <boost/geometry/algorithms/detail/within/interface.hpp>
#include <boost/geometry/algorithms/detail/within/multi_point.hpp>
#include <boost/geometry/algorithms/detail/within/point_in_geometry.hpp>
#include <boost/geometry/algorithms/relate.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/relate/cartesian.hpp>
#include <boost/geometry/strategies/relate/geographic.hpp>
#include <boost/geometry/strategies/relate/spherical.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/order_as_direction.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace within {

struct use_point_in_geometry
{
    template <typename Geometry1, typename Geometry2, typename Strategy>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        return detail::within::within_point_geometry(geometry1, geometry2, strategy);
    }
};

struct use_relate
{
    template <typename Geometry1, typename Geometry2, typename Strategy>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        typedef typename detail::de9im::static_mask_within_type
            <
                Geometry1, Geometry2
            >::type within_mask;
        return geometry::relate(geometry1, geometry2, within_mask(), strategy);
    }
};

}} // namespace detail::within
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Point, typename Box>
struct within<Point, Box, point_tag, box_tag>
{
    template <typename Strategy>
    static inline bool apply(Point const& point, Box const& box, Strategy const& strategy)
    {
        return strategy.within(point, box).apply(point, box);
    }
};

template <typename Box1, typename Box2>
struct within<Box1, Box2, box_tag, box_tag>
{
    template <typename Strategy>
    static inline bool apply(Box1 const& box1, Box2 const& box2, Strategy const& strategy)
    {
        assert_dimension_equal<Box1, Box2>();
        return strategy.within(box1, box2).apply(box1, box2);
    }
};

// P/P

template <typename Point1, typename Point2>
struct within<Point1, Point2, point_tag, point_tag>
    : public detail::within::use_point_in_geometry
{};

template <typename Point, typename MultiPoint>
struct within<Point, MultiPoint, point_tag, multi_point_tag>
    : public detail::within::use_point_in_geometry
{};

template <typename MultiPoint, typename Point>
struct within<MultiPoint, Point, multi_point_tag, point_tag>
    : public detail::within::multi_point_point
{};

template <typename MultiPoint1, typename MultiPoint2>
struct within<MultiPoint1, MultiPoint2, multi_point_tag, multi_point_tag>
    : public detail::within::multi_point_multi_point
{};

// P/L

template <typename Point, typename Segment>
struct within<Point, Segment, point_tag, segment_tag>
    : public detail::within::use_point_in_geometry
{};

template <typename Point, typename Linestring>
struct within<Point, Linestring, point_tag, linestring_tag>
    : public detail::within::use_point_in_geometry
{};

template <typename Point, typename MultiLinestring>
struct within<Point, MultiLinestring, point_tag, multi_linestring_tag>
    : public detail::within::use_point_in_geometry
{};

template <typename MultiPoint, typename Segment>
struct within<MultiPoint, Segment, multi_point_tag, segment_tag>
    : public detail::within::multi_point_single_geometry<true>
{};

template <typename MultiPoint, typename Linestring>
struct within<MultiPoint, Linestring, multi_point_tag, linestring_tag>
    : public detail::within::multi_point_single_geometry<true>
{};

template <typename MultiPoint, typename MultiLinestring>
struct within<MultiPoint, MultiLinestring, multi_point_tag, multi_linestring_tag>
    : public detail::within::multi_point_multi_geometry<true>
{};

// P/A

template <typename Point, typename Ring>
struct within<Point, Ring, point_tag, ring_tag>
    : public detail::within::use_point_in_geometry
{};

template <typename Point, typename Polygon>
struct within<Point, Polygon, point_tag, polygon_tag>
    : public detail::within::use_point_in_geometry
{};

template <typename Point, typename MultiPolygon>
struct within<Point, MultiPolygon, point_tag, multi_polygon_tag>
    : public detail::within::use_point_in_geometry
{};

template <typename MultiPoint, typename Ring>
struct within<MultiPoint, Ring, multi_point_tag, ring_tag>
    : public detail::within::multi_point_single_geometry<true>
{};

template <typename MultiPoint, typename Polygon>
struct within<MultiPoint, Polygon, multi_point_tag, polygon_tag>
    : public detail::within::multi_point_single_geometry<true>
{};

template <typename MultiPoint, typename MultiPolygon>
struct within<MultiPoint, MultiPolygon, multi_point_tag, multi_polygon_tag>
    : public detail::within::multi_point_multi_geometry<true>
{};

// L/L

template <typename Linestring1, typename Linestring2>
struct within<Linestring1, Linestring2, linestring_tag, linestring_tag>
    : public detail::within::use_relate
{};

template <typename Linestring, typename MultiLinestring>
struct within<Linestring, MultiLinestring, linestring_tag, multi_linestring_tag>
    : public detail::within::use_relate
{};

template <typename MultiLinestring, typename Linestring>
struct within<MultiLinestring, Linestring, multi_linestring_tag, linestring_tag>
    : public detail::within::use_relate
{};

template <typename MultiLinestring1, typename MultiLinestring2>
struct within<MultiLinestring1, MultiLinestring2, multi_linestring_tag, multi_linestring_tag>
    : public detail::within::use_relate
{};

// L/A

template <typename Linestring, typename Ring>
struct within<Linestring, Ring, linestring_tag, ring_tag>
    : public detail::within::use_relate
{};

template <typename MultiLinestring, typename Ring>
struct within<MultiLinestring, Ring, multi_linestring_tag, ring_tag>
    : public detail::within::use_relate
{};

template <typename Linestring, typename Polygon>
struct within<Linestring, Polygon, linestring_tag, polygon_tag>
    : public detail::within::use_relate
{};

template <typename MultiLinestring, typename Polygon>
struct within<MultiLinestring, Polygon, multi_linestring_tag, polygon_tag>
    : public detail::within::use_relate
{};

template <typename Linestring, typename MultiPolygon>
struct within<Linestring, MultiPolygon, linestring_tag, multi_polygon_tag>
    : public detail::within::use_relate
{};

template <typename MultiLinestring, typename MultiPolygon>
struct within<MultiLinestring, MultiPolygon, multi_linestring_tag, multi_polygon_tag>
    : public detail::within::use_relate
{};

// A/A

template <typename Ring1, typename Ring2>
struct within<Ring1, Ring2, ring_tag, ring_tag>
    : public detail::within::use_relate
{};

template <typename Ring, typename Polygon>
struct within<Ring, Polygon, ring_tag, polygon_tag>
    : public detail::within::use_relate
{};

template <typename Polygon, typename Ring>
struct within<Polygon, Ring, polygon_tag, ring_tag>
    : public detail::within::use_relate
{};

template <typename Polygon1, typename Polygon2>
struct within<Polygon1, Polygon2, polygon_tag, polygon_tag>
    : public detail::within::use_relate
{};

template <typename Ring, typename MultiPolygon>
struct within<Ring, MultiPolygon, ring_tag, multi_polygon_tag>
    : public detail::within::use_relate
{};

template <typename MultiPolygon, typename Ring>
struct within<MultiPolygon, Ring, multi_polygon_tag, ring_tag>
    : public detail::within::use_relate
{};

template <typename Polygon, typename MultiPolygon>
struct within<Polygon, MultiPolygon, polygon_tag, multi_polygon_tag>
    : public detail::within::use_relate
{};

template <typename MultiPolygon, typename Polygon>
struct within<MultiPolygon, Polygon, multi_polygon_tag, polygon_tag>
    : public detail::within::use_relate
{};

template <typename MultiPolygon1, typename MultiPolygon2>
struct within<MultiPolygon1, MultiPolygon2, multi_polygon_tag, multi_polygon_tag>
    : public detail::within::use_relate
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#include <boost/geometry/index/rtree.hpp>

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_IMPLEMENTATION_HPP
