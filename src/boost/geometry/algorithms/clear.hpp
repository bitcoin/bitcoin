// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020-2021.
// Modifications copyright (c) 2020-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CLEAR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CLEAR_HPP


#include <type_traits>

#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>
#include <boost/geometry/geometries/adapted/boost_variant.hpp> // for backward compatibility
#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace clear
{

template <typename Geometry>
struct collection_clear
{
    static inline void apply(Geometry& geometry)
    {
        traits::clear<Geometry>::apply(geometry);
    }
};

template <typename Polygon>
struct polygon_clear
{
    static inline void apply(Polygon& polygon)
    {
        traits::clear
            <
                typename std::remove_reference
                    <
                        typename traits::interior_mutable_type<Polygon>::type
                    >::type
            >::apply(interior_rings(polygon));
        traits::clear
            <
                typename std::remove_reference
                    <
                        typename traits::ring_mutable_type<Polygon>::type
                    >::type
            >::apply(exterior_ring(polygon));
    }
};

template <typename Geometry>
struct no_action
{
    static inline void apply(Geometry& )
    {
    }
};


}} // namespace detail::clear
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry,
    typename Tag = typename tag_cast<typename tag<Geometry>::type, multi_tag>::type
>
struct clear: not_implemented<Tag>
{};

// Point/box/segment do not have clear. So specialize to do nothing.
template <typename Geometry>
struct clear<Geometry, point_tag>
    : detail::clear::no_action<Geometry>
{};

template <typename Geometry>
struct clear<Geometry, box_tag>
    : detail::clear::no_action<Geometry>
{};

template <typename Geometry>
struct clear<Geometry, segment_tag>
    : detail::clear::no_action<Geometry>
{};

template <typename Geometry>
struct clear<Geometry, linestring_tag>
    : detail::clear::collection_clear<Geometry>
{};

template <typename Geometry>
struct clear<Geometry, ring_tag>
    : detail::clear::collection_clear<Geometry>
{};


// Polygon can (indirectly) use std for clear
template <typename Polygon>
struct clear<Polygon, polygon_tag>
    : detail::clear::polygon_clear<Polygon>
{};


template <typename Geometry>
struct clear<Geometry, multi_tag>
    : detail::clear::collection_clear<Geometry>
{};


template <typename Geometry>
struct clear<Geometry, dynamic_geometry_tag>
{
    static void apply(Geometry& geometry)
    {
        traits::visit<Geometry>::apply([](auto & g)
        {
            clear<std::remove_reference_t<decltype(g)>>::apply(g);
        }, geometry);
    }
};


template <typename Geometry>
struct clear<Geometry, geometry_collection_tag>
{
    static void apply(Geometry& geometry)
    {
        traits::clear<Geometry>::apply(geometry);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief Clears a linestring, ring or polygon (exterior+interiors) or multi*
\details Generic function to clear a geometry. All points will be removed from the collection or collections
    making up the geometry. In most cases this is equivalent to the .clear() method of a std::vector<...>. In
    the case of a polygon, this clear functionality is automatically called for the exterior ring, and for the
    interior ring collection. In the case of a point, boxes and segments, nothing will happen.
\ingroup clear
\tparam Geometry \tparam_geometry
\param geometry \param_geometry which will be cleared
\note points and boxes cannot be cleared, instead they can be set to zero by "assign_zero"

\qbk{[include reference/algorithms/clear.qbk]}
*/
template <typename Geometry>
inline void clear(Geometry& geometry)
{
    concepts::check<Geometry>();

    dispatch::clear<Geometry>::apply(geometry);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_CLEAR_HPP
