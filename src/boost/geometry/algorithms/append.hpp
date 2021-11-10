// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_APPEND_HPP
#define BOOST_GEOMETRY_ALGORITHMS_APPEND_HPP


#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/num_interior_rings.hpp>
#include <boost/geometry/algorithms/detail/convert_point_to_point.hpp>
#include <boost/geometry/algorithms/detail/signed_size_type.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>
#include <boost/geometry/geometries/adapted/boost_variant.hpp> // for backward compatibility
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/util/range.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace append
{

struct append_no_action
{
    template <typename Geometry, typename Point>
    static inline void apply(Geometry& , Point const& ,
                             signed_size_type = -1, signed_size_type = 0)
    {
    }
};

struct to_range_point
{
    template <typename Geometry, typename Point>
    static inline void apply(Geometry& geometry, Point const& point,
                             signed_size_type = -1, signed_size_type = 0)
    {
        typename geometry::point_type<Geometry>::type copy;
        geometry::detail::conversion::convert_point_to_point(point, copy);
        traits::push_back<Geometry>::apply(geometry, copy);
    }
};


struct to_range_range
{
    template <typename Geometry, typename Range>
    static inline void apply(Geometry& geometry, Range const& range,
                             signed_size_type = -1, signed_size_type = 0)
    {
        using point_type = typename boost::range_value<Range>::type;

        auto const end = boost::end(range);
        for (auto it = boost::begin(range); it != end; ++it)
        {
            to_range_point::apply<Geometry, point_type>(geometry, *it);
        }
    }
};


struct to_polygon_point
{
    template <typename Polygon, typename Point>
    static inline void apply(Polygon& polygon, Point const& point,
                             signed_size_type ring_index, signed_size_type = 0)
    {
        using ring_type = typename ring_type<Polygon>::type;
        using exterior_ring_type = typename ring_return_type<Polygon>::type;
        using interior_ring_range_type = typename interior_return_type<Polygon>::type;

        if (ring_index == -1)
        {
            exterior_ring_type ext_ring = exterior_ring(polygon);
            to_range_point::apply<ring_type, Point>(ext_ring, point);
        }
        else if (ring_index < signed_size_type(num_interior_rings(polygon)))
        {
            interior_ring_range_type int_rings = interior_rings(polygon);
            to_range_point::apply<ring_type, Point>(range::at(int_rings, ring_index), point);
        }
    }
};


struct to_polygon_range
{
    template <typename Polygon, typename Range>
    static inline void apply(Polygon& polygon, Range const& range,
                             signed_size_type ring_index, signed_size_type = 0)
    {
        using ring_type = typename ring_type<Polygon>::type;
        using exterior_ring_type = typename ring_return_type<Polygon>::type;
        using interior_ring_range_type = typename interior_return_type<Polygon>::type;

        if (ring_index == -1)
        {
            exterior_ring_type ext_ring = exterior_ring(polygon);
            to_range_range::apply<ring_type, Range>(ext_ring, range);
        }
        else if (ring_index < signed_size_type(num_interior_rings(polygon)))
        {
            interior_ring_range_type int_rings = interior_rings(polygon);
            to_range_range::apply<ring_type, Range>(range::at(int_rings, ring_index), range);
        }
    }
};


template <typename Policy>
struct to_multigeometry
{
    template <typename MultiGeometry, typename RangeOrPoint>
    static inline void apply(MultiGeometry& multigeometry,
                             RangeOrPoint const& range_or_point,
                             signed_size_type ring_index, signed_size_type multi_index)
    {
        Policy::template apply
            <
                typename boost::range_value<MultiGeometry>::type,
                RangeOrPoint
            >(range::at(multigeometry, multi_index), range_or_point, ring_index);
    }
};


}} // namespace detail::append
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry,
    typename RangeOrPoint,
    typename Tag = typename geometry::tag<Geometry>::type,
    typename OtherTag = typename geometry::tag<RangeOrPoint>::type
>
struct append
    : detail::append::append_no_action
{};

template <typename Geometry, typename Point>
struct append<Geometry, Point, linestring_tag, point_tag>
    : detail::append::to_range_point
{};

template <typename Geometry, typename Point>
struct append<Geometry, Point, ring_tag, point_tag>
    : detail::append::to_range_point
{};

template <typename Polygon, typename Point>
struct append<Polygon, Point, polygon_tag, point_tag>
        : detail::append::to_polygon_point
{};

template <typename Geometry, typename Range, typename RangeTag>
struct append<Geometry, Range, linestring_tag, RangeTag>
    : detail::append::to_range_range
{};

template <typename Geometry, typename Range, typename RangeTag>
struct append<Geometry, Range, ring_tag, RangeTag>
    : detail::append::to_range_range
{};

template <typename Polygon, typename Range, typename RangeTag>
struct append<Polygon, Range, polygon_tag, RangeTag>
        : detail::append::to_polygon_range
{};


template <typename Geometry, typename Point>
struct append<Geometry, Point, multi_point_tag, point_tag>
    : detail::append::to_range_point
{};

template <typename Geometry, typename Range, typename RangeTag>
struct append<Geometry, Range, multi_point_tag, RangeTag>
    : detail::append::to_range_range
{};

template <typename MultiGeometry, typename Point>
struct append<MultiGeometry, Point, multi_linestring_tag, point_tag>
    : detail::append::to_multigeometry<detail::append::to_range_point>
{};

template <typename MultiGeometry, typename Range, typename RangeTag>
struct append<MultiGeometry, Range, multi_linestring_tag, RangeTag>
    : detail::append::to_multigeometry<detail::append::to_range_range>
{};

template <typename MultiGeometry, typename Point>
struct append<MultiGeometry, Point, multi_polygon_tag, point_tag>
    : detail::append::to_multigeometry<detail::append::to_polygon_point>
{};

template <typename MultiGeometry, typename Range, typename RangeTag>
struct append<MultiGeometry, Range, multi_polygon_tag, RangeTag>
    : detail::append::to_multigeometry<detail::append::to_polygon_range>
{};


template <typename Geometry, typename RangeOrPoint, typename OtherTag>
struct append<Geometry, RangeOrPoint, dynamic_geometry_tag, OtherTag>
{
    static inline void apply(Geometry& geometry,
                             RangeOrPoint const& range_or_point,
                             signed_size_type ring_index, signed_size_type multi_index)
    {
        traits::visit<Geometry>::apply([&](auto & g)
        {
            append
                <
                    std::remove_reference_t<decltype(g)>, RangeOrPoint
                >::apply(g, range_or_point, ring_index, multi_index);
        }, geometry);
    }
};

// TODO: It's unclear how append should work for GeometryCollection because
//   it can hold multiple different geometries.

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief Appends one or more points to a linestring, ring, polygon, multi-geometry
\ingroup append
\tparam Geometry \tparam_geometry
\tparam RangeOrPoint Either a range or a point, fullfilling Boost.Range concept or Boost.Geometry Point Concept
\param geometry \param_geometry
\param range_or_point The point or range to add
\param ring_index The index of the ring in case of a polygon:
    exterior ring (-1, the default) or  interior ring index
\param multi_index The index of the geometry to which the points are appended

\qbk{[include reference/algorithms/append.qbk]}
}
 */
template <typename Geometry, typename RangeOrPoint>
inline void append(Geometry& geometry, RangeOrPoint const& range_or_point,
                   signed_size_type ring_index = -1, signed_size_type multi_index = 0)
{
    concepts::check<Geometry>();

    dispatch::append
        <
            Geometry, RangeOrPoint
        >::apply(geometry, range_or_point, ring_index, multi_index);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_APPEND_HPP
