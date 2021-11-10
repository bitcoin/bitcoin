// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_CORE_POINT_ORDER_HPP
#define BOOST_GEOMETRY_CORE_POINT_ORDER_HPP


#include <boost/range/value_type.hpp>

#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/util/type_traits_std.hpp>

namespace boost { namespace geometry
{

/*!
\brief Enumerates options for the order of points within polygons
\ingroup enum
\details The enumeration order_selector describes options for the order of
    points within a polygon. Polygons can be ordered either clockwise or
    counterclockwise. The specific order of a polygon type is defined by the
    point_order metafunction. The point_order metafunction defines a value,
    which is one of the values enumerated in the order_selector

\qbk{
[heading See also]
[link geometry.reference.core.point_order The point_order metafunction]
}
*/
enum order_selector
{
    /// Points are ordered clockwise
    clockwise = 1,
    /// Points are ordered counter clockwise
    counterclockwise = 2,
    /// Points might be stored in any order, algorithms will determine it on the
    /// fly (not yet supported)
    order_undetermined = 0
};

namespace traits
{

/*!
\brief Traits class indicating the order of contained points within a
    ring or (multi)polygon, clockwise, counter clockwise or not known.
\ingroup traits
\tparam Ring ring
*/
template <typename Ring>
struct point_order
{
    static const order_selector value = clockwise;
};


} // namespace traits


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace point_order
{

struct clockwise
{
    static const order_selector value = geometry::clockwise;
};


}} // namespace detail::point_order
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename Tag, typename Geometry>
struct point_order
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry);
};

template <typename Point>
struct point_order<point_tag, Point>
    : public detail::point_order::clockwise {};

template <typename Segment>
struct point_order<segment_tag, Segment>
    : public detail::point_order::clockwise {};


template <typename Box>
struct point_order<box_tag, Box>
    : public detail::point_order::clockwise {};

template <typename LineString>
struct point_order<linestring_tag, LineString>
    : public detail::point_order::clockwise {};


template <typename Ring>
struct point_order<ring_tag, Ring>
{
    static const order_selector value
        = geometry::traits::point_order<Ring>::value;
};

// Specialization for polygon: the order is the order of its rings
template <typename Polygon>
struct point_order<polygon_tag, Polygon>
{
    static const order_selector value = core_dispatch::point_order
        <
            ring_tag,
            typename ring_type<polygon_tag, Polygon>::type
        >::value ;
};

template <typename MultiPoint>
struct point_order<multi_point_tag, MultiPoint>
    : public detail::point_order::clockwise {};

template <typename MultiLinestring>
struct point_order<multi_linestring_tag, MultiLinestring>
    : public detail::point_order::clockwise {};


// Specialization for multi_polygon: the order is the order of its polygons
template <typename MultiPolygon>
struct point_order<multi_polygon_tag, MultiPolygon>
{
    static const order_selector value = core_dispatch::point_order
        <
            polygon_tag,
            typename boost::range_value<MultiPolygon>::type
        >::value ;
};

} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief \brief_meta{value, point order (clockwise\, counterclockwise),
    \meta_geometry_type}
\tparam Geometry \tparam_geometry
\ingroup core

\qbk{[include reference/core/point_order.qbk]}
*/
template <typename Geometry>
struct point_order
{
    static const order_selector value = core_dispatch::point_order
        <
            typename tag<Geometry>::type,
            typename util::remove_cptrref<Geometry>::type
        >::value;
};

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_CORE_POINT_ORDER_HPP
