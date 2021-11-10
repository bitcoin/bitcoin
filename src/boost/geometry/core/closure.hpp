// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_CORE_CLOSURE_HPP
#define BOOST_GEOMETRY_CORE_CLOSURE_HPP

#include <boost/range/value_type.hpp>

#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/util/type_traits_std.hpp>

namespace boost { namespace geometry
{


/*!
\brief Enumerates options for defining if polygons are open or closed
\ingroup enum
\details The enumeration closure_selector describes options for if a polygon is
    open or closed. In a closed polygon the very first point (per ring) should
    be equal to the very last point.
    The specific closing property of a polygon type is defined by the closure
    metafunction. The closure metafunction defines a value, which is one of the
    values enumerated in the closure_selector

\qbk{
[heading See also]
[link geometry.reference.core.closure The closure metafunction]
}
*/
enum closure_selector
{
    /// Rings are open: first point and last point are different, algorithms
    /// close them explicitly on the fly
    open = 0,
    /// Rings are closed: first point and last point must be the same
    closed = 1,
    /// (Not yet implemented): algorithms first figure out if ring must be
    /// closed on the fly
    closure_undertermined = -1
};

namespace traits
{

/*!
    \brief Traits class indicating if points within a
        ring or (multi)polygon are closed (last point == first point),
        open or not known.
    \ingroup traits
    \par Geometries:
        - ring
    \tparam G geometry
*/
template <typename G>
struct closure
{
    static const closure_selector value = closed;
};


} // namespace traits


#ifndef DOXYGEN_NO_DETAIL
namespace core_detail { namespace closure
{

struct closed
{
    static const closure_selector value = geometry::closed;
};


/// Metafunction to define the minimum size of a ring:
/// 3 for open rings, 4 for closed rings
template <closure_selector Closure>
struct minimum_ring_size {};

template <>
struct minimum_ring_size<geometry::closed>
    : std::integral_constant<std::size_t, 4>
{};

template <>
struct minimum_ring_size<geometry::open>
    : std::integral_constant<std::size_t, 3>
{};


}} // namespace core_detail::closure
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename Tag, typename Geometry>
struct closure
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry);
};

template <typename Box>
struct closure<point_tag, Box> : public core_detail::closure::closed {};

template <typename Box>
struct closure<box_tag, Box> : public core_detail::closure::closed {};

template <typename Box>
struct closure<segment_tag, Box> : public core_detail::closure::closed {};

template <typename LineString>
struct closure<linestring_tag, LineString>
    : public core_detail::closure::closed {};


template <typename Ring>
struct closure<ring_tag, Ring>
{
    static const closure_selector value
        = geometry::traits::closure<Ring>::value;
};

// Specialization for Polygon: the closure is the closure of its rings
template <typename Polygon>
struct closure<polygon_tag, Polygon>
{
    static const closure_selector value = core_dispatch::closure
        <
            ring_tag,
            typename ring_type<polygon_tag, Polygon>::type
        >::value ;
};

template <typename MultiPoint>
struct closure<multi_point_tag, MultiPoint>
    : public core_detail::closure::closed {};

template <typename MultiLinestring>
struct closure<multi_linestring_tag, MultiLinestring>
    : public core_detail::closure::closed {};

// Specialization for MultiPolygon: the closure is the closure of Polygon's rings
template <typename MultiPolygon>
struct closure<multi_polygon_tag, MultiPolygon>
{
    static const closure_selector value = core_dispatch::closure
        <
            polygon_tag,
            typename boost::range_value<MultiPolygon>::type
        >::value ;
};

} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief \brief_meta{value, closure (clockwise\, counterclockwise),
    \meta_geometry_type}
\tparam Geometry \tparam_geometry
\ingroup core

\qbk{[include reference/core/closure.qbk]}
*/
template <typename Geometry>
struct closure
{
    static const closure_selector value = core_dispatch::closure
        <
            typename tag<Geometry>::type,
            typename util::remove_cptrref<Geometry>::type
        >::value;
};


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template
<
    typename Geometry,
    closure_selector Closure = geometry::closure<Geometry>::value
>
using minimum_ring_size = core_detail::closure::minimum_ring_size<Closure>;

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_CORE_CLOSURE_HPP
