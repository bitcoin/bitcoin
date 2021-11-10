// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2021.
// Modifications copyright (c) 2013-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_POINT_IN_GEOMETRY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_POINT_IN_GEOMETRY_HPP


#include <boost/core/ignore_unused.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/assert.hpp>

#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/detail/interior_iterator.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/strategies/concepts/within_concept.hpp>

#include <boost/geometry/util/range.hpp>
#include <boost/geometry/views/detail/closed_clockwise_view.hpp>

namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace within {


template <typename Point, typename Range, typename Strategy> inline
int point_in_range(Point const& point, Range const& range, Strategy const& strategy)
{
    typename Strategy::state_type state;

    typedef typename boost::range_iterator<Range const>::type iterator_type;
    iterator_type it = boost::begin(range);
    iterator_type end = boost::end(range);

    for ( iterator_type previous = it++ ; it != end ; ++previous, ++it )
    {
        if ( ! strategy.apply(point, *previous, *it, state) )
        {
            break;
        }
    }

    return strategy.result(state);
}

}} // namespace detail::within

namespace detail_dispatch { namespace within {

// checks the relation between a point P and geometry G
// returns 1 if P is in the interior of G
// returns 0 if P is on the boundry of G
// returns -1 if P is in the exterior of G

template <typename Geometry,
          typename Tag = typename geometry::tag<Geometry>::type>
struct point_in_geometry
    : not_implemented<Tag>
{};

template <typename Point2>
struct point_in_geometry<Point2, point_tag>
{
    template <typename Point1, typename Strategy> static inline
    int apply(Point1 const& point1, Point2 const& point2, Strategy const& strategy)
    {
        typedef decltype(strategy.relate(point1, point2)) strategy_type;
        return strategy_type::apply(point1, point2) ? 1 : -1;
    }
};

template <typename Segment>
struct point_in_geometry<Segment, segment_tag>
{
    template <typename Point, typename Strategy> static inline
    int apply(Point const& point, Segment const& segment, Strategy const& strategy)
    {
        typedef typename geometry::point_type<Segment>::type point_type;
        point_type p0, p1;
// TODO: don't copy points
        detail::assign_point_from_index<0>(segment, p0);
        detail::assign_point_from_index<1>(segment, p1);

        auto const s = strategy.relate(point, segment);
        typename decltype(s)::state_type state;
        s.apply(point, p0, p1, state);
        int r = s.result(state);

        if ( r != 0 )
            return -1; // exterior

        // if the point is equal to the one of the terminal points
        if ( detail::equals::equals_point_point(point, p0, strategy)
          || detail::equals::equals_point_point(point, p1, strategy) )
            return 0; // boundary
        else
            return 1; // interior
    }
};


template <typename Linestring>
struct point_in_geometry<Linestring, linestring_tag>
{
    template <typename Point, typename Strategy> static inline
    int apply(Point const& point, Linestring const& linestring, Strategy const& strategy)
    {
        std::size_t count = boost::size(linestring);
        if ( count > 1 )
        {
            if ( detail::within::point_in_range(point, linestring,
                                    strategy.relate(point, linestring)) != 0 )
            {
                return -1; // exterior
            }

            typedef typename boost::range_value<Linestring>::type point_type;
            point_type const& front = range::front(linestring);
            point_type const& back = range::back(linestring);

            // if the linestring doesn't have a boundary
            if ( detail::equals::equals_point_point(front, back, strategy) )
            {
                return 1; // interior
            }
            // else if the point is equal to the one of the terminal points
            else if ( detail::equals::equals_point_point(point, front, strategy)
                   || detail::equals::equals_point_point(point, back, strategy) )
            {
                return 0; // boundary
            }
            else
            {
                return 1; // interior
            }
        }
// TODO: for now degenerated linestrings are ignored
//       throw an exception here?
        /*else if ( count == 1 )
        {
            if ( detail::equals::equals_point_point(point, front, strategy) )
                return 1;
        }*/

        return -1; // exterior
    }
};

template <typename Ring>
struct point_in_geometry<Ring, ring_tag>
{
    template <typename Point, typename Strategy> static inline
    int apply(Point const& point, Ring const& ring, Strategy const& strategy)
    {
        if ( boost::size(ring) < core_detail::closure::minimum_ring_size
                                    <
                                        geometry::closure<Ring>::value
                                    >::value )
        {
            return -1;
        }

        detail::closed_clockwise_view<Ring const> view(ring);
        return detail::within::point_in_range(point, view,
                                              strategy.relate(point, ring));
    }
};

// Polygon: in exterior ring, and if so, not within interior ring(s)
template <typename Polygon>
struct point_in_geometry<Polygon, polygon_tag>
{
    template <typename Point, typename Strategy>
    static inline int apply(Point const& point, Polygon const& polygon,
                            Strategy const& strategy)
    {
        int const code = point_in_geometry
            <
                typename ring_type<Polygon>::type
            >::apply(point, exterior_ring(polygon), strategy);

        if (code == 1)
        {
            typename interior_return_type<Polygon const>::type
                rings = interior_rings(polygon);
            
            for (typename detail::interior_iterator<Polygon const>::type
                 it = boost::begin(rings);
                 it != boost::end(rings);
                 ++it)
            {
                int const interior_code = point_in_geometry
                    <
                        typename ring_type<Polygon>::type
                    >::apply(point, *it, strategy);

                if (interior_code != -1)
                {
                    // If 0, return 0 (touch)
                    // If 1 (inside hole) return -1 (outside polygon)
                    // If -1 (outside hole) check other holes if any
                    return -interior_code;
                }
            }
        }
        return code;
    }
};

template <typename Geometry>
struct point_in_geometry<Geometry, multi_point_tag>
{
    template <typename Point, typename Strategy> static inline
    int apply(Point const& point, Geometry const& geometry, Strategy const& strategy)
    {
        typedef typename boost::range_value<Geometry>::type point_type;
        typedef typename boost::range_const_iterator<Geometry>::type iterator;
        for ( iterator it = boost::begin(geometry) ; it != boost::end(geometry) ; ++it )
        {
            int pip = point_in_geometry<point_type>::apply(point, *it, strategy);

            //BOOST_GEOMETRY_ASSERT(pip != 0);
            if ( pip > 0 ) // inside
                return 1;
        }

        return -1; // outside
    }
};

template <typename Geometry>
struct point_in_geometry<Geometry, multi_linestring_tag>
{
    template <typename Point, typename Strategy> static inline
    int apply(Point const& point, Geometry const& geometry, Strategy const& strategy)
    {
        int pip = -1; // outside

        typedef typename boost::range_value<Geometry>::type linestring_type;
        typedef typename boost::range_value<linestring_type>::type point_type;
        typedef typename boost::range_iterator<Geometry const>::type iterator;
        iterator it = boost::begin(geometry);
        for ( ; it != boost::end(geometry) ; ++it )
        {
            pip = point_in_geometry<linestring_type>::apply(point, *it, strategy);

            // inside or on the boundary
            if ( pip >= 0 )
            {
                ++it;
                break;
            }
        }

        // outside
        if ( pip < 0 )
            return -1;

        // TODO: the following isn't needed for covered_by()

        unsigned boundaries = pip == 0 ? 1 : 0;

        for ( ; it != boost::end(geometry) ; ++it )
        {
            if ( boost::size(*it) < 2 )
                continue;

            point_type const& front = range::front(*it);
            point_type const& back = range::back(*it);

            // is closed_ring - no boundary
            if ( detail::equals::equals_point_point(front, back, strategy) )
                continue;

            // is point on boundary
            if ( detail::equals::equals_point_point(point, front, strategy)
              || detail::equals::equals_point_point(point, back, strategy) )
            {
                ++boundaries;
            }
        }

        // if the number of boundaries is odd, the point is on the boundary
        return boundaries % 2 ? 0 : 1;
    }
};

template <typename Geometry>
struct point_in_geometry<Geometry, multi_polygon_tag>
{
    template <typename Point, typename Strategy> static inline
    int apply(Point const& point, Geometry const& geometry, Strategy const& strategy)
    {
        // For invalid multipolygons
        //int res = -1; // outside

        typedef typename boost::range_value<Geometry>::type polygon_type;
        typedef typename boost::range_const_iterator<Geometry>::type iterator;
        for ( iterator it = boost::begin(geometry) ; it != boost::end(geometry) ; ++it )
        {
            int pip = point_in_geometry<polygon_type>::apply(point, *it, strategy);

            // inside or on the boundary
            if ( pip >= 0 )
                return pip;

            // For invalid multi-polygons
            //if ( 1 == pip ) // inside polygon
            //    return 1;
            //else if ( res < pip ) // point must be inside at least one polygon
            //    res = pip;
        }

        return -1; // for valid multipolygons
        //return res; // for invalid multipolygons
    }
};

}} // namespace detail_dispatch::within

namespace detail { namespace within {

// 1 - in the interior
// 0 - in the boundry
// -1 - in the exterior
template <typename Point, typename Geometry, typename Strategy>
inline int point_in_geometry(Point const& point, Geometry const& geometry, Strategy const& strategy)
{
    concepts::within::check<Point, Geometry, Strategy>();

    return detail_dispatch::within::point_in_geometry<Geometry>::apply(point, geometry, strategy);
}

template <typename Point, typename Geometry, typename Strategy>
inline bool within_point_geometry(Point const& point, Geometry const& geometry, Strategy const& strategy)
{
    return point_in_geometry(point, geometry, strategy) > 0;
}

template <typename Point, typename Geometry, typename Strategy>
inline bool covered_by_point_geometry(Point const& point, Geometry const& geometry, Strategy const& strategy)
{
    return point_in_geometry(point, geometry, strategy) >= 0;
}

}} // namespace detail::within
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_WITHIN_POINT_IN_GEOMETRY_HPP
