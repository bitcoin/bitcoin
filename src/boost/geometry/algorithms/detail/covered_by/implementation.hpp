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

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_COVERED_BY_IMPLEMENTATION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_COVERED_BY_IMPLEMENTATION_HPP

#include <cstddef>
#include <boost/core/ignore_unused.hpp>
#include <boost/geometry/algorithms/detail/covered_by/interface.hpp>
#include <boost/geometry/algorithms/detail/within/implementation.hpp>
#include <boost/geometry/strategies/relate/cartesian.hpp>
#include <boost/geometry/strategies/relate/geographic.hpp>
#include <boost/geometry/strategies/relate/spherical.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace covered_by {

struct use_point_in_geometry
{
    template <typename Geometry1, typename Geometry2, typename Strategy>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        return detail::within::covered_by_point_geometry(geometry1, geometry2, strategy);
    }
};

struct use_relate
{
    template <typename Geometry1, typename Geometry2, typename Strategy>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2, Strategy const& strategy)
    {
        typedef typename detail::de9im::static_mask_covered_by_type
            <
                Geometry1, Geometry2
            >::type covered_by_mask;
        return geometry::relate(geometry1, geometry2, covered_by_mask(), strategy);
    }
};

}} // namespace detail::covered_by
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Point, typename Box>
struct covered_by<Point, Box, point_tag, box_tag>
{
    template <typename Strategy>
    static inline bool apply(Point const& point, Box const& box, Strategy const& strategy)
    {
        return strategy.covered_by(point, box).apply(point, box);
    }
};

template <typename Box1, typename Box2>
struct covered_by<Box1, Box2, box_tag, box_tag>
{
    template <typename Strategy>
    static inline bool apply(Box1 const& box1, Box2 const& box2, Strategy const& strategy)
    {
        assert_dimension_equal<Box1, Box2>();
        return strategy.covered_by(box1, box2).apply(box1, box2);
    }
};


// P/P

template <typename Point1, typename Point2>
struct covered_by<Point1, Point2, point_tag, point_tag>
    : public detail::covered_by::use_point_in_geometry
{};

template <typename Point, typename MultiPoint>
struct covered_by<Point, MultiPoint, point_tag, multi_point_tag>
    : public detail::covered_by::use_point_in_geometry
{};

template <typename MultiPoint, typename Point>
struct covered_by<MultiPoint, Point, multi_point_tag, point_tag>
    : public detail::within::multi_point_point
{};

template <typename MultiPoint1, typename MultiPoint2>
struct covered_by<MultiPoint1, MultiPoint2, multi_point_tag, multi_point_tag>
    : public detail::within::multi_point_multi_point
{};

// P/L

template <typename Point, typename Segment>
struct covered_by<Point, Segment, point_tag, segment_tag>
    : public detail::covered_by::use_point_in_geometry
{};

template <typename Point, typename Linestring>
struct covered_by<Point, Linestring, point_tag, linestring_tag>
    : public detail::covered_by::use_point_in_geometry
{};

template <typename Point, typename MultiLinestring>
struct covered_by<Point, MultiLinestring, point_tag, multi_linestring_tag>
    : public detail::covered_by::use_point_in_geometry
{};

template <typename MultiPoint, typename Segment>
struct covered_by<MultiPoint, Segment, multi_point_tag, segment_tag>
    : public detail::within::multi_point_single_geometry<false>
{};

template <typename MultiPoint, typename Linestring>
struct covered_by<MultiPoint, Linestring, multi_point_tag, linestring_tag>
    : public detail::within::multi_point_single_geometry<false>
{};

template <typename MultiPoint, typename MultiLinestring>
struct covered_by<MultiPoint, MultiLinestring, multi_point_tag, multi_linestring_tag>
    : public detail::within::multi_point_multi_geometry<false>
{};

// P/A

template <typename Point, typename Ring>
struct covered_by<Point, Ring, point_tag, ring_tag>
    : public detail::covered_by::use_point_in_geometry
{};

template <typename Point, typename Polygon>
struct covered_by<Point, Polygon, point_tag, polygon_tag>
    : public detail::covered_by::use_point_in_geometry
{};

template <typename Point, typename MultiPolygon>
struct covered_by<Point, MultiPolygon, point_tag, multi_polygon_tag>
    : public detail::covered_by::use_point_in_geometry
{};

template <typename MultiPoint, typename Ring>
struct covered_by<MultiPoint, Ring, multi_point_tag, ring_tag>
    : public detail::within::multi_point_single_geometry<false>
{};

template <typename MultiPoint, typename Polygon>
struct covered_by<MultiPoint, Polygon, multi_point_tag, polygon_tag>
    : public detail::within::multi_point_single_geometry<false>
{};

template <typename MultiPoint, typename MultiPolygon>
struct covered_by<MultiPoint, MultiPolygon, multi_point_tag, multi_polygon_tag>
    : public detail::within::multi_point_multi_geometry<false>
{};

// L/L

template <typename Linestring1, typename Linestring2>
struct covered_by<Linestring1, Linestring2, linestring_tag, linestring_tag>
    : public detail::covered_by::use_relate
{};

template <typename Linestring, typename MultiLinestring>
struct covered_by<Linestring, MultiLinestring, linestring_tag, multi_linestring_tag>
    : public detail::covered_by::use_relate
{};

template <typename MultiLinestring, typename Linestring>
struct covered_by<MultiLinestring, Linestring, multi_linestring_tag, linestring_tag>
    : public detail::covered_by::use_relate
{};

template <typename MultiLinestring1, typename MultiLinestring2>
struct covered_by<MultiLinestring1, MultiLinestring2, multi_linestring_tag, multi_linestring_tag>
    : public detail::covered_by::use_relate
{};

// L/A

template <typename Linestring, typename Ring>
struct covered_by<Linestring, Ring, linestring_tag, ring_tag>
    : public detail::covered_by::use_relate
{};

template <typename MultiLinestring, typename Ring>
struct covered_by<MultiLinestring, Ring, multi_linestring_tag, ring_tag>
    : public detail::covered_by::use_relate
{};

template <typename Linestring, typename Polygon>
struct covered_by<Linestring, Polygon, linestring_tag, polygon_tag>
    : public detail::covered_by::use_relate
{};

template <typename MultiLinestring, typename Polygon>
struct covered_by<MultiLinestring, Polygon, multi_linestring_tag, polygon_tag>
    : public detail::covered_by::use_relate
{};

template <typename Linestring, typename MultiPolygon>
struct covered_by<Linestring, MultiPolygon, linestring_tag, multi_polygon_tag>
    : public detail::covered_by::use_relate
{};

template <typename MultiLinestring, typename MultiPolygon>
struct covered_by<MultiLinestring, MultiPolygon, multi_linestring_tag, multi_polygon_tag>
    : public detail::covered_by::use_relate
{};

// A/A

template <typename Ring1, typename Ring2>
struct covered_by<Ring1, Ring2, ring_tag, ring_tag>
    : public detail::covered_by::use_relate
{};

template <typename Ring, typename Polygon>
struct covered_by<Ring, Polygon, ring_tag, polygon_tag>
    : public detail::covered_by::use_relate
{};

template <typename Polygon, typename Ring>
struct covered_by<Polygon, Ring, polygon_tag, ring_tag>
    : public detail::covered_by::use_relate
{};

template <typename Polygon1, typename Polygon2>
struct covered_by<Polygon1, Polygon2, polygon_tag, polygon_tag>
    : public detail::covered_by::use_relate
{};

template <typename Ring, typename MultiPolygon>
struct covered_by<Ring, MultiPolygon, ring_tag, multi_polygon_tag>
    : public detail::covered_by::use_relate
{};

template <typename MultiPolygon, typename Ring>
struct covered_by<MultiPolygon, Ring, multi_polygon_tag, ring_tag>
    : public detail::covered_by::use_relate
{};

template <typename Polygon, typename MultiPolygon>
struct covered_by<Polygon, MultiPolygon, polygon_tag, multi_polygon_tag>
    : public detail::covered_by::use_relate
{};

template <typename MultiPolygon, typename Polygon>
struct covered_by<MultiPolygon, Polygon, multi_polygon_tag, polygon_tag>
    : public detail::covered_by::use_relate
{};

template <typename MultiPolygon1, typename MultiPolygon2>
struct covered_by<MultiPolygon1, MultiPolygon2, multi_polygon_tag, multi_polygon_tag>
    : public detail::covered_by::use_relate
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_COVERED_BY_IMPLEMENTATION_HPP
